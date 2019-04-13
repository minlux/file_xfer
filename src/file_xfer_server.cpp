//-----------------------------------------------------------------------------
/*!
   \file
   \brief File transfer server
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include "file_xfer_server.h"
#include "file_xfer.h"
#include "stdutils.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */
const unsigned char FileXferServer::ACK = FILE_XFER_CMD_ACK;
const unsigned char FileXferServer::NACK = FILE_XFER_CMD_NACK;
const unsigned char FileXferServer::ZERO = 0;


/* -- Module Global Function Prototypes ----------------------------------- */
static unsigned int timespec2str(char *buf, uint len, struct timespec *ts); //utility function


/* -- Implementation ------------------------------------------------------ */

FileXferServer::FileXferServer(Slay2Channel * ctrl, Slay2Channel * data, const char * root)
{
   //init control channel
   ctrlChannel = ctrl;
   ctrlChannel->setReceiver(FileXferServer::onCtrlFrame, this);
   //init data channel
   dataChannel = data;
   dataChannel->setReceiver(FileXferServer::onDataFrame, this);

   //set members
   rootDir = root;
   state = FILE_XFER_SERVER_STATE_IDLE;
   listDirectory = NULL;
   uploadFile = NULL;
   downloadFile = NULL;
}



//-------------------------------------------------------------------------------------------------
/*
   \brief Handle reception of ctrl frames.

   There are no seqmented control frames. That means there is exactly one control frame per command!
   Format:
   data[0]:          command specifier
   data[1 .. len-1]: optional command arguments (must be zero terminated)

   Function returns with
   ACK (+ optional arguments)
   or NACK
   depending, wheather the command can be executed or not!
*/
//-------------------------------------------------------------------------------------------------
void FileXferServer::onCtrlFrame(void * const obj, const unsigned char * const data, const unsigned int len)
{
   /* ensure zero termination: not needed, as SLAY2 data ARE zero terminated!
   ((unsigned char *)data)[len] = 0; //thats a hack ... but works with SLAY2
   */
   // cout << "onCtrlFrame is called. len=" << len << endl;
   // cout << data << endl;

   //forward to member function
   ((FileXferServer *)obj)->onCtrlFrame(data, len);
}
void FileXferServer::onCtrlFrame(const unsigned char * const data, const unsigned int len)
{
   unsigned char command = '?';
   //there must be at least 1 bytes in a command frame - error otherwise
   if (len >= 1)
   {
      command = data[0];
      switch (command)
      {
         //print working directory
         //REQ: W
         //RES: a<current-dir>\0
         case FILE_XFER_CMD_PWD:
         {
            onPWD_Command();
            return;
         }

         //change directory
         //REQ: C<path>\0
         //RES: a<new-current-dir>\0
         //on error: n
         case FILE_XFER_CMD_CD:
         {
            //ensure the given string is zero terminated
            if (data[len - 1] == 0)
            {
               const char * path = (const char *)(data + 1);
               bool stat = onCD_Command(path, false);
               if (stat)
               {
                  onPWD_Command();
                  return;
               }
            }
            break;
         }

         //list directory
         //REQ: L
         //RES: a
         //on error: n
         //list will be sent via data-channel
         case FILE_XFER_CMD_LS:
         {
            if (state == FILE_XFER_SERVER_STATE_IDLE) //server must be idle to accept that command
            {
               bool stat = onLS_Command();
               if (stat)
               {
                  return;
               }
            }
            break;
         }

         //chang and list directory. this command is a shortcut for CD + LS command!
         //REQ: I<path>\0
         //RES: a<new-current-dir>\0
         //on error: n
         //list will be sent via data-channel
         case FILE_XFER_CMD_DIR:
         {
            if (state == FILE_XFER_SERVER_STATE_IDLE) //server must be idle to accept that command
            {
               //ensure the given string is zero terminated
               if (data[len - 1] == 0)
               {
                  const char * path = (const char *)(data + 1);
                  bool stat = onCD_Command(path, false);
                  if (stat)
                  {
                     stat = onLS_Command(false);
                     if (stat)
                     {
                        onPWD_Command();
                        return;
                     }
                  }
               }
            }
            break;
         }

         //make directory
         //REQ: M<dir-name>\0
         //RES: a
         //on error: n
         case FILE_XFER_CMD_MKDIR:
         {
            //ensure the given file string is zero terminated
            if (data[len - 1] == 0)
            {
               const char * directory = (const char *)(data + 1); //first argument is directory
               bool stat = onMKDIR_Command(directory);
               if (stat)
               {
                  return;
               }
            }
            break;
         }

         //remove file or directory
         //REQ: R<file/dir-name>\0
         //RES: a
         //on error: n
         case FILE_XFER_CMD_RM:
         {
            //ensure the given file string is zero terminated
            if (data[len - 1] == 0)
            {
               const char * fileName = (const char *)(data + 1); //first argument is filename
               bool stat = onRM_Command(fileName);
               if (stat)
               {
                  return;
               }
            }
            break;
         }

         //upload to server
         //REQ: U<filename>,<filesize>\0  /*filesize as decimal ascii number*/
         //RES: a
         //on error: n
         //data are expected to be received on data-channel.
         case FILE_XFER_CMD_UPLOAD:
         {
            if (state == FILE_XFER_SERVER_STATE_IDLE) //server must be idle to accept that command
            {
               //ensure the given string is zero terminated
               if (data[len - 1] == 0)
               {
                  char * it = (char *)(data + 1); //i'm going to iterate through the string (and modifing it...)
                  const char * fileName = it; //first argument is upload file name
                  int fileSize = 0;
                  char c;

                  //search for KOMMA
                  while (((c = *it) != 0) && (c != ',')) ++it; //this loop ends, if "it" points to ZERO or KOMMA
                  *it = 0; //replace KOMMA by ZERO to terminate file name

                  //determien fileSize
                  if (c != 0) //did i found the KOMMA above
                  {
                     //yes - the data following, is the file size
                     fileSize = atoi(it + 1);
                  }

                  //schedule file upload
                  if (fileSize > 0)
                  {
                     bool stat = onUPLOAD_Command(fileName, (unsigned int)fileSize);
                     if (stat)
                     {
                        return;
                     }
                  }
               }
            }
            break;
         }

         //download file from server
         //REQ: D<filename>\0
         //RES: a<filesize>\0   /*Success: filesize as decimal ascii number*/
         //on error: n
         //data are sent on data-channel.
         case FILE_XFER_CMD_DOWNLOAD:
         {
            if (state == FILE_XFER_SERVER_STATE_IDLE) //server must be idle to accept that command
            {
               //ensure the given string is zero terminated
               if (data[len - 1] == 0)
               {
                  const char * fileName = (const char *)(data + 1);
                  bool stat = onDOWNLOAD_Command(fileName);
                  if (stat)
                  {
                     return;
                  }
               }
            }
            break;
         }

         //abort/cancel/quit an ongoin command and reset server into idle state
         //REQ: Q
         //RES: a
         case FILE_XFER_CMD_QUIT:
         {
            state = FILE_XFER_SERVER_STATE_IDLE; //set server into idle state
            if (listDirectory != NULL) //close directory (in case a list-directory command was canceled)
            {
               closedir(listDirectory);
               listDirectory = NULL;
            }
            if (uploadFile != NULL) //close upload file (in caste a upload command was canceled)
            {
               fclose(uploadFile);
               uploadFile = NULL;
               uploadFileSize = 0;
            }
            if (downloadFile != NULL) //close download file (in caste a download command was canceled)
            {
               fclose(downloadFile);
               downloadFile = NULL;
            }
            dataChannel->flushTxBuffer(); //flush data channel
            ctrlChannel->send(&ACK, 1); //acknowledge quit (cancel) command
            std::cout << "QUIT command received. Server reset to IDLE!" << endl;
            return;
         }

         default:
         {
            break;
         }
      }
   }

   //error - reply with NACK
   ctrlChannel->send(&NACK, 1);
   std::cout << "Failed to execute command: " << (char)command << endl;
}



//handle reception of data frames
//currently only used, when client uploads a file
void FileXferServer::onDataFrame(void * const obj, const unsigned char * const data, const unsigned int len)
{
   /* ensure zero termination: not needed, as SLAY2 data ARE zero terminated!
   ((unsigned char *)data)[len] = 0; //thats a hack ... but works with SLAY2
   */
   // cout << "onDataFrame is called. len=" << len << endl;
   // cout << data << endl;

   //forward to member function
   ((FileXferServer *)obj)->onDataFrame(data, len);
}
void FileXferServer::onDataFrame(const unsigned char * const data, const unsigned int len)
{
   switch (state)
   {
      //receiving file-upload from client
      case FILE_XFER_SERVER_STATE_UPLOADING:
         execUPLOAD_Command(data, len);
         break;

      default:
         break;
   }
}


//handle transmission of data frames
//currently only usewd, for "ls" and file download
void FileXferServer::task(void)
{
   switch (state)
   {
      //sending directory listing to client
      case FILE_XFER_SERVER_STATE_LISTING:
         execLS_Command();
         break;

      //sending file to client
      case FILE_XFER_SERVER_STATE_DOWNLOADING:
         execDOWNLOAD_Command();
         break;

      default:
         break;
   }
}





//-------------------------------------------------------------------------------------------------
/*
   \brief Reply currenct working directory.

   Requested on control channel: W
   Response on control channel:
   - on success: a<current-dir>\0

   No response on data channel!

   \retval true   on success
*/
//-------------------------------------------------------------------------------------------------
bool FileXferServer::onPWD_Command()
{
   const string& cwd = currentDir.getCurrentDirectory();
   ctrlChannel->send(&ACK, 1, true); //acknowledge command
   ctrlChannel->send((unsigned char *)"/", 1, true); //leading directory slash
   ctrlChannel->send((unsigned char *)cwd.c_str(), cwd.length() + 1);
   std::cout << "Working directory is /" << cwd << endl;
   return true;
}


//-------------------------------------------------------------------------------------------------
/*
   \brief Change currenct working directory.

   Requested on control channel: C<path>\0
   Optional response on control channel:
   - on success: a<new-current-dir>\0

   <path> may be empty. In this case, change to root directory.
   <path> may contain './' and '../'

   If <path> starts with '/', change relative to root directory.
   Otherwise, change relative to current working directory.

   If parameter "response" equals true, this function sends (in case of success) a response on
   control channel. Otherwise not!

   \retval true   if dir was changed successfully.
   \retval false  otherwise
*/
//-------------------------------------------------------------------------------------------------
bool FileXferServer::onCD_Command(const char * path, bool response)
{
   //cd command with empty path changes to root-directory
   if (path[0] == 0)
   {
      currentDir.changeDirectory();
      if (response)
      {
         ctrlChannel->send(&ACK, 1); //acknowledge command
         std::cout << "Working directory changed to: /" << currentDir.getCurrentDirectory() << endl;
      }
      return true;
   }
   //try to change current directory
   DirectoryNavigator tmp = currentDir; //use a tmp copy
   tmp.changeDirectory(path);
   //check if that directory exists ...
   if (DirectoryNavigatorLinux::directoryExists(rootDir + tmp.getCurrentDirectory())) //prefix root directory
   {
      currentDir = tmp;
      if (response)
      {
         ctrlChannel->send(&ACK, 1); //acknowledge command
         std::cout << "Working directory changed to: /" << currentDir.getCurrentDirectory() << endl;
      }
      return true;
   }
   //error
   return false;
}



//-------------------------------------------------------------------------------------------------
/*
   \brief List files and directories in currenct working directory.

   Requested on control channel: L
   Optional response on control channel:
   - on success: a

   Files and directory list is responsed on data channel.
   A listing may look like this:

   <type>,<name>,<size>,<date>\n    <-- 1st line contains the current directory (type = '.'). Sent by this function!
   <type>,<name>,<size>,<date>\n    <-- the other lines, are sent by execLS_Command function.
   ...
   <type>,<name>,<size>,<date>\n
   \0                               <-- end of list


   - files/directories are separated by '\n'.
   - list ends with '\0'
   - a file/directory entry is given by 4 values, separated by ',':

   <type>:
   - '.'  the current directory
   - 'd'  a directory entry
   - 'f'  a file entry

   <name>:
   - the file or directory name
   - in case of current directory (type = '.'), the (full path of) current directory

   <size>
   - size of file, in bytes
   - empty, for "non-file-types"

   <date>
   - last modification date of file. format: "YYYY-mm-dd HH:MM:SS"
   - empty, for "non-file-types"

   If parameter "response" equals true, this function sends (in case of success) a response on
   control channel. Otherwise not!

   \retval true   on success
   \retval false  otherwise
*/
//-------------------------------------------------------------------------------------------------
bool FileXferServer::onLS_Command(bool response)
{
   const string& cwd = currentDir.getCurrentDirectory();
   listDir = rootDir + cwd; //prefix root director
   listDirectory = opendir(listDir.c_str());
   if (listDirectory != NULL)
   {
      //schedule LS command
      state = FILE_XFER_SERVER_STATE_LISTING; //set server into listing state
      if (response)
      {
         ctrlChannel->send(&ACK, 1); //acknowledge command
      }
      std::cout << "LS command scheduled!" << endl;

      //output first LS entry (the current directory)
      dataChannel->send((const unsigned char *)".,/", 3, true); //current directory
      dataChannel->send((const unsigned char *)cwd.c_str(), cwd.length(), true); //name
      dataChannel->send((const unsigned char *)",,\n", 3, true); //no size, no date
      return true;
   }
   return false;
}

//send the file/directory list entries on data channel. terminate list with ZERO.
//return to IDLE state when done.
void FileXferServer::execLS_Command()
{
   //there must be enough buffer space (for at least one more entry)
   while (dataChannel->getTxBufferSpace() >= 300) //assumption: max length of one file entry may be 300 bytes
   {
      struct dirent *ent = readdir(listDirectory);
      if (ent == NULL) //end of directory listing
      {
         dataChannel->send(&ZERO, 1); //send termination
         closedir(listDirectory);
         listDirectory = NULL;
         state = FILE_XFER_SERVER_STATE_IDLE;
         std::cout << "LS command completed!" << endl;
         return;
      }
      //otherwise
      //          directory                 regular file      (other files will be skipped)
      if ((ent->d_type == DT_DIR) || (ent->d_type == DT_REG))
      {
         if (ent->d_type == DT_DIR) //directory
         {
            if (strcmp(ent->d_name, ".") != 0) //skip "." directory
            {
               dataChannel->send((const unsigned char *)"d,", 2, true); //directory
               dataChannel->send((const unsigned char *)ent->d_name, strlen(ent->d_name), true); //name
               dataChannel->send((const unsigned char *)",,\n", 3, true); //no size, no date
               // std::cout << "LS <dir>: " << ent->d_name << endl;
            }
         }
         else // regular file
         {
            char listEntry[32];
            unsigned int listEntryLen;
            string fileName;
            struct stat fileStat;
            int fileStatErr;


            //read further file information
            fileName = listDir + ent->d_name;
            fileStatErr = stat(fileName.c_str(), &fileStat);

            //assembly entry
            dataChannel->send((const unsigned char *)"f,", 2, true); //file
            dataChannel->send((const unsigned char *)ent->d_name, strlen(ent->d_name), true); //name
            dataChannel->send((const unsigned char *)",", 1, true);
            if (fileStatErr == 0)
            {
               listEntryLen = snprintf(listEntry, sizeof(listEntry), "%d", (int)fileStat.st_size); //size
               dataChannel->send((const unsigned char *)listEntry, listEntryLen, true);
            }
            dataChannel->send((const unsigned char *)",", 1, true);
            if (fileStatErr == 0)
            {
               listEntryLen = timespec2str(listEntry, sizeof(listEntry), &fileStat.st_mtim);
               dataChannel->send((const unsigned char *)listEntry, listEntryLen, true);
            }
            dataChannel->send((const unsigned char *)"\n", 1, true);
            // std::cout << "LS <file>: " << ent->d_name << endl;
         }
      }
   }
}



//-------------------------------------------------------------------------------------------------
/*
   \brief Make directory on server.

   Requested on control channel: M<directory>\0
   Response on control channel:
   - on success: a

   <directory> shall contain the directory name for creation.
   If it starts with '/' it is expected to be "root-based" path to the directory.
   Otherwise it is expected to be a path relative to current directory.

   \retval true   if directory was successfully created.
   \retval false  otherwise
*/
//-------------------------------------------------------------------------------------------------
bool FileXferServer::onMKDIR_Command(const char * directory)
{
   string fn = rootDir; //prefix root director
   if (directory[0] == '/') //relative to root?
   {
      fn += &directory[1]; //append to root directory
   }
   else //relative to current directory
   {
      fn += currentDir.getCurrentDirectory() + directory; //append current directory to file-name
   }
   //try to make the given directory
   int err = mkdir(fn.c_str(), 0777);
   if (err == 0)
   {
      ctrlChannel->send(&ACK, 1); //acknowledge command
      std::cout << "Directory " << fn << " created!" << endl;
      return true;
   }
   return false;
}


//-------------------------------------------------------------------------------------------------
/*
   \brief Delete/remove file from server.

   Requested on control channel: R<filename>\0
   Response on control channel:
   - on success: a

   <filename> shall contain the filename of the file to remove.
   If it starts with '/' it is expected to be "root-based" path to the file.
   Otherwise it is expected to be a path relative to current directory.

   \retval true   if file was successfully removed.
   \retval false  otherwise
*/
//-------------------------------------------------------------------------------------------------
bool FileXferServer::onRM_Command(const char * filename)
{
   string fn = rootDir; //prefix root director
   if (filename[0] == '/') //relative to root?
   {
      fn += &filename[1]; //append to root directory
   }
   else //relative to current directory
   {
      fn += currentDir.getCurrentDirectory() + filename; //append current directory to file-name
   }
   //try to delete the given file
   int err = remove(fn.c_str());
   if (err == 0)
   {
      ctrlChannel->send(&ACK, 1); //acknowledge command
      std::cout << "File " << fn << " removed!" << endl;
      return true;
   }
   return false;
}



//-------------------------------------------------------------------------------------------------
/*
   \brief Upload file to server.

   Requested on control channel: U<filename>,<size>\0
   Response on control channel:
   - on success: a

   <filename> shall contain the filename of the uploaded file.
   If it starts with '/' it is expected to be "root-based" path to the file.
   Otherwise it is expected to be a path relative to current directory.

   <size> is the file size. It is given as a decimal ascii number.
   The server expects to receive exactly that number of bytes on the data channel.

   \retval true   if file was successfully opend for write.
   \retval false  otherwise
*/
//-------------------------------------------------------------------------------------------------
bool FileXferServer::onUPLOAD_Command(const char * filename, unsigned int size)
{
   string fn = rootDir; //prefix root director
   if (filename[0] == '/') //relative to root?
   {
      fn += &filename[1]; //append to root directory
   }
   else //relative to current directory
   {
      fn += currentDir.getCurrentDirectory() + filename; //append current directory to file-name
   }
   uploadFile = fopen(fn.c_str(), "w");
   if (uploadFile != NULL)
   {
      //schedule UPLOAD command
      state = FILE_XFER_SERVER_STATE_UPLOADING; //set server into uploading state
      uploadFileSize = size; //store number of bytes for upload
      ctrlChannel->send(&ACK, 1); //acknowledge command
      std::cout << "UPLOAD command scheduled! Len=" << size << endl;
      return true;
   }
   return false;
}

//recieve the file upload data on data channel. as far as all data was received:
// - the file is closed
// - a ACK ('a') is returned on the data channel, to let the client know, that all data was process
// - return to IDLE state
void FileXferServer::execUPLOAD_Command(const unsigned char * const data, const unsigned int len)
{
   //determine number of bytes to write into file
   unsigned int count = len;
   if (count > uploadFileSize)
   {
      count = uploadFileSize; //limitation: do not write more bytes than expected
   }
   //write data into buffer
   fwrite(data, 1, count, uploadFile);
   //reduce number of remaining bytes to write
   if (uploadFileSize >= count)
   {
      uploadFileSize -= count;
   }
   //handle end of file
   if (uploadFileSize == 0)
   {
      fclose(uploadFile); //close file
      uploadFile = NULL;
      state = FILE_XFER_SERVER_STATE_IDLE; //set server into IDLE state
      dataChannel->send(&ACK, 1); //finally reply ACK on data-channel, to indicate that server has completed
      std::cout << "UPLOAD has completed!" << endl;
   }
}



//-------------------------------------------------------------------------------------------------
/*
   \brief Download file from server.

   Requested on control channel: U<filename>\0
   Response on control channel:
   - on success: a<filesize>

   <filename> shall contain the filename of the file for download.
   If it starts with '/' it is expected to be "root-based" path to the file.
   Otherwise it is expected to be a path relative to current directory.

   If file exist, the command is acknowledged together with the size of the file <filesize>
   on the control channel. <filesize< is given as a decimal ascii number.

   \retval true   if file was successfully opend for read.
   \retval false  otherwise
*/
//-------------------------------------------------------------------------------------------------
bool FileXferServer::onDOWNLOAD_Command(const char * filename)
{
   string fn = rootDir; //prefix root director
   if (filename[0] == '/') //relative to root?
   {
      fn += &filename[1]; //append to root directory
   }
   else //relative to current directory
   {
      fn += currentDir.getCurrentDirectory() + filename; //append current directory to file-name
   }
   downloadFile = fopen(fn.c_str(), "r");
   if (downloadFile != NULL)
   {
      unsigned int fileSize;
      char fileSizeStr[16];
      int fileSizeStrLen;

      //determine file size
      fseek(downloadFile, 0L, SEEK_END); //go to end of file
      fileSize = ftell(downloadFile); //read file pointer - that the file size
      rewind(downloadFile); //go back to begin of file

      //schedule DOWNLOAD command
      state = FILE_XFER_SERVER_STATE_DOWNLOADING; //set server into downloading state
      ctrlChannel->send(&ACK, 1, true); //acknowledge command
      fileSizeStrLen = snprintf(fileSizeStr, sizeof(fileSizeStr), "%d", fileSize); //size
      ctrlChannel->send((const unsigned char *)fileSizeStr, fileSizeStrLen + 1);
      std::cout << "DOWNLOAD command scheduled!" << endl;
      return true;
   }
   return false;
}

//send file download data on data channel. as far as all data was sent:
// - the file is closed
// - return to IDLE state
void FileXferServer::execDOWNLOAD_Command()
{
   //there must be enough buffer space
   while (dataChannel->getTxBufferSpace() >= 256)
   {
      unsigned char buffer[256];
      unsigned int count;

      //read out file and send data to client
      count = fread(buffer, 1, sizeof(buffer), downloadFile);
      if (count > 0)
      {
         dataChannel->send(buffer, count);
      }

      //handle end of file
      if ((count == 0) || (feof(downloadFile)))
      {
         fclose(downloadFile); //close file
         downloadFile = NULL;
         state = FILE_XFER_SERVER_STATE_IDLE; //set server into IDLE state
         std::cout << "DOWNLOAD has completed!" << endl;
         return;
      }
   }
}








//format into YYYY-mm-dd HH:MM:SS
//buf needs to store 30 characters
static unsigned int timespec2str(char *buf, uint len, struct timespec *ts)
{
    int ret;
    struct tm t;

    tzset();
    if (localtime_r(&(ts->tv_sec), &t) == NULL)
        return 0;

    ret = strftime(buf, len, "%F %T", &t);
    return ret;
}