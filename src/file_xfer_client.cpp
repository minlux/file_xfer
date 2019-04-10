//-----------------------------------------------------------------------------
/*!
   \file
   \brief File transfer client
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <stdio.h>
#include <iostream>
#include "file_xfer_client.h"
#include "file_xfer.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */


FileXferClient::FileXferClient(FileXferClientApp * app)
{
   init();
   this->app = app;
}


FileXferClient::FileXferClient(Slay2Channel * ctrl, Slay2Channel * data, FileXferClientApp * app)
{
   init();
   this->app = app;
   use(ctrl, data);
}


void FileXferClient::init()
{
   app = NULL;
   ctrlChannel = NULL;
   dataChannel = NULL;
   srcDstFile = FILE_XFER_CLIENT_INVALID_FILE_HANDLE;
   ctrlState = 0;
   dataState = 0;
   time1ms = 0;
   timeout1ms = 0;
   uploadFileSize = 0;
   downloadFileSize = 0;
}


void FileXferClient::use(Slay2Channel * ctrl, Slay2Channel * data)
{
   //init control channel
   ctrlChannel = ctrl;
   ctrlChannel->setReceiver(FileXferClient::_onCtrlFrameAsync, this);
   //init data channel
   dataChannel = data;
   dataChannel->setReceiver(FileXferClient::_onDataFrameAsync, this);
}




//request working directory of server
//return:
//0, on success
//-1, failed to send request (not enough TX buffer)
int FileXferClient::workingDirectory()
{
   if (ctrlChannel->getTxBufferSize() >= 1)
   {
      const unsigned char command = FILE_XFER_CMD_PWD;
      ctrlChannel->send(&command, 1);
      ctrlState = FILE_XFER_CMD_PWD;
      return 0;
   }
   return -1;
}


//request server to change (working) directory to the given <path>
//return:
//0, on success
//-1, failed to send request (not enough TX buffer)
int FileXferClient::changeDirectory(const std::string& path)
{
   int pathLength = path.length() + 1; //one more for the zero termination
   if (ctrlChannel->getTxBufferSize() > pathLength) //one more for the leading command byte
   {
      const unsigned char command = FILE_XFER_CMD_CD;
      ctrlChannel->send(&command, 1, true);
      ctrlChannel->send((const unsigned char *)path.c_str(), pathLength);
      ctrlState = FILE_XFER_CMD_CD;
      return 0;
   }
   return -1;
}



//request server to list (content of working) directory
//return:
//0, on success
//-1, failed to send request (not enough TX buffer)
//-2, failed, because client isn't idle!
int FileXferClient::listDirectory()
{
   //check for idle condition
   if (dataState != 0) //not idle?
   {
      return -2;
   }
   //check for enough tx buffer
   if (ctrlChannel->getTxBufferSize() >= 1)
   {
      const unsigned char command = FILE_XFER_CMD_LS;
      ctrlChannel->send(&command, 1);
      ctrlState = FILE_XFER_CMD_LS;
      dataState = FILE_XFER_CMD_LS;
      directoryList = "";
      return 0;
   }
   return -1;
}


//request server to change (working) directory and list its content
//return:
//0, on success
//-1, failed to send request (not enough TX buffer)
//-2, failed, because client isn't idle!
int FileXferClient::changeListDirectory(const std::string& path)
{
   //check for idle condition
   if (dataState != 0) //not idle?
   {
      return -2;
   }
   //check for enough tx buffer
   int pathLength = path.length() + 1; //one more for the zero termination
   if (ctrlChannel->getTxBufferSize() > pathLength) //one more for the leading command byte
   {
      const unsigned char command = FILE_XFER_CMD_DIR;
      ctrlChannel->send(&command, 1, true);
      ctrlChannel->send((const unsigned char *)path.c_str(), pathLength);
      ctrlState = FILE_XFER_CMD_DIR;
      dataState = FILE_XFER_CMD_DIR;
      directoryList = "";
      return 0;
   }
   return -1;
}



//request server to create the given directory (given by path)
//return:
//0, on success
//-1, failed to send request (not enough TX buffer)
int FileXferClient::makeDirectory(const std::string& path)
{
   int pathLength = path.length() + 1; //one more for the zero termination
   if (ctrlChannel->getTxBufferSize() > pathLength) //one more for the leading command byte
   {
      const unsigned char command = FILE_XFER_CMD_MKDIR;
      ctrlChannel->send(&command, 1, true);
      ctrlChannel->send((const unsigned char *)path.c_str(), pathLength);
      ctrlState = FILE_XFER_CMD_MKDIR;
      return 0;
   }
   return -1;
}

//request server to delete the given file or directory (given by path)
//return:
//0, on success
//-1, failed to send request (not enough TX buffer)
int FileXferClient::removeFile(const std::string& path)
{
   int pathLength = path.length() + 1; //one more for the zero termination
   if (ctrlChannel->getTxBufferSize() > pathLength) //one more for the leading command byte
   {
      const unsigned char command = FILE_XFER_CMD_RM;
      ctrlChannel->send(&command, 1, true);
      ctrlChannel->send((const unsigned char *)path.c_str(), pathLength);
      ctrlState = FILE_XFER_CMD_RM;
      return 0;
   }
   return -1;
}


//request to download given source-file from server and store it to the given destination
//return:
//0, on success
//-1, failed to send request (not enough TX buffer)
//-2, failed, because client isn't idle!
//-3, failed, because destination file not writeable
int FileXferClient::downloadFile(const std::string& source, const std::string& destination)
{
   //check for idle condition
   if (dataState != 0) //not idle?
   {
      return -2;
   }
   //check for enough tx buffer
   int srcLength = source.length() + 1; //one more for the zero termination
   if (ctrlChannel->getTxBufferSize() > srcLength) //one more for the leading command byte
   {
      //open destination file
      FileXferClientApp::FileHandle_t dstFile;
      if (app->openFileForWrite(destination, &dstFile))
      {
         const unsigned char command = FILE_XFER_CMD_DOWNLOAD;
         ctrlChannel->send(&command, 1, true);
         ctrlChannel->send((const unsigned char *)source.c_str(), srcLength);
         ctrlState = FILE_XFER_CMD_DOWNLOAD;
         dataState = FILE_XFER_CMD_DOWNLOAD;
         downloadFileSize = 0; //will be set in the response
         srcDstFile = dstFile; //
         return 0;
      }
      return -3;
   }
   return -1;
}


//request to upload given source-file to server and store it there to the given destination
//return:
//0, on success
//-1, failed to send request (not enough TX buffer)
//-2, failed, because client isn't idle!
//-3, failed, because source file can'b be read
int FileXferClient::uploadFile(const std::string& source, const std::string& destination)
{
   //check for idle condition
   if (dataState != 0) //not idle?
   {
      return -2;
   }
   //check for enough tx buffer
   int dstLength = destination.length();
   if (ctrlChannel->getTxBufferSize() > (dstLength + 11)) //one more for the leading command byte and about 11 bytes to specify the length of the file (in bytes)
   {
      //open source file
      FileXferClientApp::FileHandle_t srcFile;
      if (app->openFileForRead(source, &srcFile))
      {
         const unsigned char command = FILE_XFER_CMD_UPLOAD;
         char buffer[16];
         int len;

         ctrlChannel->send(&command, 1, true);
         ctrlChannel->send((const unsigned char *)destination.c_str(), dstLength, true);
         uploadFileSize = app->getFileSize(srcFile);
         len = sprintf(buffer, ",%d", (int)uploadFileSize);
         ctrlChannel->send((const unsigned char *)buffer, len + 1); //include zero termination
         ctrlState = FILE_XFER_CMD_UPLOAD;
         dataState = FILE_XFER_CMD_UPLOAD;
         srcDstFile = srcFile; //
         return 0;
      }
      return -3;
   }
   return -1;
}


//
//request server to quit ongoing transfer/operation
//return:
//0, on success
//-1, failed to send request (not enough TX buffer)
int FileXferClient::quit()
{
   if (ctrlChannel->getTxBufferSize() >= 1)
   {
      const unsigned char command = FILE_XFER_CMD_QUIT;
      ctrlChannel->send(&command, 1);
      ctrlState = FILE_XFER_CMD_QUIT;
      timeout1ms = time1ms + 3000; //force quit, if there is no response withing 3 seconds
      return 0;
   }
   return -1;
}




void FileXferClient::task(unsigned long time1ms)
{
   //set current time
   this->time1ms = time1ms;

   //handle reception
   //check for received control bytes
   if (ctrlRxBuffer.getCount()) //some pending control bytes?
   {
      unsigned char * data;
      unsigned int len;
      //process buffered data - within a critial section
      ctrlChannel->enterCritical();
      len = ctrlRxBuffer.top(&data); //assert len > 0 (as getCount returned > 0)
      onCtrlFrame(data, len);
      ctrlRxBuffer.pop(len); //drop that bytes away
      ctrlChannel->leaveCritical();
   }
   //check for received data bytes
   if (dataRxBuffer.getCount()) //some pending control bytes?
   {
      unsigned char * data;
      unsigned int len;
      //process buffered data - within a critial section
      dataChannel->enterCritical();
      len = dataRxBuffer.top(&data); //assert len > 0 (as getCount returned > 0)
      onDataFrame(data, len);
      dataRxBuffer.pop(len); //drop that bytes away
      dataChannel->leaveCritical();
   }

   //handle transmission
   switch (dataState)
   {
   case FILE_XFER_CMD_UPLOAD:
      doFileUpload();
      break;

   default:
      break;
   }

   //check for timeout
   if ((timeout1ms != 0) && (time1ms > timeout1ms))
   {
      doQuit();
   }
}


//The async functions are within the execution context of "Slay2" (slay2.task)!
//This may be different to the context of "FileXferClient" (fileXferClient.task).
//That means, these functions may be called aysnchronously!
void FileXferClient::_onCtrlFrameAsync(void * const obj, const unsigned char * const data, const unsigned int len)
{
   if (len > 0)
   {
      //forward to member function
      ((FileXferClient *)obj)->onCtrlFrameAsync(data, len);
   }
}
void FileXferClient::onCtrlFrameAsync(const unsigned char * const data, const unsigned int len)
{
   //push data into buffer - within a critial section
   ctrlChannel->enterCritical();
   ctrlRxBuffer.push(data, len);
   ctrlChannel->leaveCritical();
}

//this method is called "synchronously" by method "task()". It takes its data out of the buffer,
//that was filled asynchronously!
void FileXferClient::onCtrlFrame(const unsigned char * const data, const unsigned int len)
{
   const int ack = (data[0] == FILE_XFER_CMD_ACK);

   switch (ctrlState)
   {
   case FILE_XFER_CMD_PWD:
      app->onPwdResponse(ack, (const char *)&data[1]);
      break;

   case FILE_XFER_CMD_CD:
      app->onCdResponse(ack, (const char *)&data[1]);
      break;

   case FILE_XFER_CMD_LS:
      if (ack == 0) //negative acknowledge?
      {
         app->onLsResponse(ack, (const char *)&data[1]);
         dataState = 0;
      }
      //there is nothing todo here, in case of positive ACK (see "onDataFrame" for this case)
      break;

   case FILE_XFER_CMD_DIR:
      if (ack == 0) //negative acknowledge?
      {
         app->onDirResponse(ack, (const char *)&data[1]);
         dataState = 0;
      }
      //there is nothing todo here, in case of positive ACK (see "onDataFrame" for this case)
      break;

   case FILE_XFER_CMD_MKDIR:
      app->onMkdirResponse(ack);
      break;

   case FILE_XFER_CMD_RM:
      app->onRmResponse(ack);
      break;

   case FILE_XFER_CMD_DOWNLOAD:
      if (ack == 0) //negative acknowledge?
      {
         app->onDownloadResponse(ack);
         dataState = 0;
      }
      else
      {
         downloadFileSize = atoi((const char *)&data[1]);
      }
      break;

   case FILE_XFER_CMD_UPLOAD:
      if (ack == 0) //negative acknowledge?
      {
         app->onUploadResponse(ack);
         dataState = 0;
      }
      //there is nothing todo here, in case of positive ACK (see "onDataFrame" for this case)
      break;

   case FILE_XFER_CMD_QUIT:
      doQuit();
      break;

   default: //IDLE
      break;
   }

   //set ctrl state to IDLE
   ctrlState = 0;
}


//The async functions are within the execution context of "Slay2" (slay2.task)!
//This may be different to the context of "FileXferClient" (fileXferClient.task).
//That means, these functions may be called aysnchronously!
void FileXferClient::_onDataFrameAsync(void * const obj, const unsigned char * const data, const unsigned int len)
{
   if (len > 0)
   {
      //forward to member function
      ((FileXferClient *)obj)->onDataFrameAsync(data, len);
   }
}
void FileXferClient::onDataFrameAsync(const unsigned char * const data, const unsigned int len)
{
   //push data into buffer - within a critial section
   dataChannel->enterCritical();
   dataRxBuffer.push(data, len);
   dataChannel->leaveCritical();
}


//this method is called "synchronously" by method "task()". It takes its data out of the buffer,
//that was filled asynchronously!
void FileXferClient::onDataFrame(const unsigned char * const data, const unsigned int len)
{
   switch (dataState)
   {
      case FILE_XFER_CMD_LS:
      {
         directoryList += (const char *)data;
         if (data[len -1] == 0) //end of listing
         {
            app->onLsResponse(1, directoryList);
            dataState = 0;
         }
         break;
      }


      case FILE_XFER_CMD_DIR:
      {
         directoryList += (const char *)data;
         if (data[len -1] == 0) //end of listing
         {
            app->onDirResponse(1, directoryList);
            dataState = 0;
         }
         break;
      }


      case FILE_XFER_CMD_DOWNLOAD:
      {
         //do limitation
         size_t dataLen = len;
         if (dataLen > downloadFileSize)
         {
            dataLen = downloadFileSize;
         }
         //write data to file
         app->writeToFile(srcDstFile, data, dataLen);
         downloadFileSize -= dataLen;
         if (downloadFileSize == 0) //end of data
         {
            //close file
            srcDstFile = app->closeFile(srcDstFile);
            //notify application about end of download
            app->onDownloadResponse(1);
            dataState = 0;
         }
         break;
      }


      case FILE_XFER_CMD_UPLOAD:
      {
         //acknowledge of file upload expected here
         //if (data[0] == FILE_XFER_CMD_ACK) {
         //notify application, that upload has completed
         app->onUploadResponse(1);
         dataState = 0;
         break;
      }
   }
}



void FileXferClient::doFileUpload()
{
   //if there are data for upload, we must ensure that there is enough free space in tx buffer
   while ((uploadFileSize != 0) &&
          (dataChannel->getTxBufferSpace() >= 256))
   {
      unsigned char buffer[256];
      unsigned int count;

      //read out file and send data to server
      count = app->readFromFile(srcDstFile, buffer, sizeof(buffer));
      if (count > 0)
      {
         dataChannel->send(buffer, count);
      }

      //handle end of file
      if (uploadFileSize > count)
      {
         uploadFileSize -= count;
      }
      else
      {
         uploadFileSize = 0;
      }
      if ((count == 0) || (uploadFileSize == 0))
      {
         srcDstFile = app->closeFile(srcDstFile); //close file
      }
   }
}


void FileXferClient::doQuit()
{
   timeout1ms = 0;
   ctrlState = 0;
   dataState = 0;
   uploadFileSize = 0;
   downloadFileSize = 0;
   //close file (if open)
   srcDstFile = app->closeFile(srcDstFile);
   //flush communication channels
   ctrlChannel->flushTxBuffer();
   dataChannel->flushTxBuffer();
   //notify application
   app->onQuitResponse(1);
}


bool FileXferClient::isIdle()
{
   return ((ctrlState == 0) && (dataState == 0));
}

