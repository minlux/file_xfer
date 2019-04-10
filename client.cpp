//-----------------------------------------------------------------------------
/*!
   \file
   \brief Demo of file transfer server

   Use Linux socat tool to create 2 virtual TTY devices, connected to each other.
   E.g. socat -d -d pty,raw,echo=0 pty,raw,echo=0
   Will create /dev/pts/1, and /dev/pts/2
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "slay2.h"
#include "slay2_linux.h"
#include "file_xfer.h"
#include "file_xfer_client.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

#define CTRL_CHANNEL    (5)
#define DATA_CHANNEL    (6)

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */
static int ctrlC;

/* -- Module Global Function Prototypes ----------------------------------- */
static void upload_alphabet(const unsigned char * const data, const unsigned int len,
                            Slay2Channel * dataChannel);

/* -- Implementation ------------------------------------------------------ */

static void m_signal_handler(int a)
{
   ctrlC = 1;
}




class DummyClient : public FileXferClientApp
{
public:
   static string statusText(int status)
   {
      if (status == 0)
      {
         return "NACK";
      }
      else
      {
         return "ACK";
      }
   }

   static string errorText(int status)
   {
      if (status == 0)
      {
         return "OK";
      }
      else
      {
         return "FEHLER";
      }
   }



public:
   void onPwdResponse(int status, const std::string& dir)
   {
      cout << "onPwdResponse: " << statusText(status) << endl;
      cout << dir << endl;
      cout << endl;
   }


   void onCdResponse(int status, const std::string& dir)
   {
      cout << "onCdResponse: " << statusText(status) << endl;
      cout << dir << endl;
      cout << endl;
   }


   void onLsResponse(int status, const std::string& dir)
   {
      cout << "onLsResponse: " << statusText(status) << endl;
      cout << dir << endl;
      cout << endl;
   }


   void onDirResponse(int status, const std::string& dir)
   {
      cout << "onDirResponse: " << statusText(status) << endl;
      cout << dir << endl;
      cout << endl;
   }


   void onMkdirResponse(int status)
   {
      cout << "onMkdirResponse: " << statusText(status) << endl;
      cout << endl;
   }


   void onRmResponse(int status)
   {
      cout << "onRmResponse: " << statusText(status) << endl;
      cout << endl;
   }


   void onDownloadResponse(int status)
   {
      cout << "onDownloadResponse: " << statusText(status) << endl;
      cout << endl;
   }


   void onUploadResponse(int status)
   {
      cout << "onUploadResponse: " << statusText(status) << endl;
      cout << endl;
   }


   void onQuitResponse(int status)
   {
      cout << "onQuitResponse: " << statusText(status) << endl;
      cout << endl;
   }



   //file operation
   bool openFileForRead(const std::string& file, FileHandle_t * handle)
   {
      cout << "openFileForRead: " << file << endl;
      *handle = (FileHandle_t)1;
      return true;
   }


   bool openFileForWrite(const std::string& file, FileHandle_t * handle)
   {
      cout << "openFileForWrite: " << file << endl;
      *handle = (FileHandle_t)2;
      return true;
   }


   size_t getFileSize(FileHandle_t file)
   {
      cout << "getFileSize=26" << endl;
      return 26;
   }


   size_t readFromFile(FileHandle_t file, unsigned char * buffer, size_t bufferSize)
   {
      for (int i = 0; i < 26; ++i)
      {
         buffer[i] = 'a' + i;
      }
      cout << "readFromFile=26" << endl;
      return 26;
   }


   size_t writeToFile(FileHandle_t file, const unsigned char * data, size_t length)
   {
      char * hack = (char *)data;
      hack[length] = 0;
      cout << "writeToFile: " << hack << endl;
      return length;
   }


   FileHandle_t closeFile(FileHandle_t file)
   {
      if (file != FILE_XFER_CLIENT_INVALID_FILE_HANDLE)
      {
         cout << "closeFile" << endl;
      }
      return FILE_XFER_CLIENT_INVALID_FILE_HANDLE;
   }
};















int main(int argc, char * argv[])
{
   char buffer[512];
   unsigned int count;
   int status;
   const char * path;

   //check command line arguments
   if (argc < 2)
   {
      cout << "Missing argument!" << endl;
      cout << "Usage: ./fx_client <tty-dev>" << endl;
      return -1;
   }

   //init communiction driver
   Slay2Linux slay2;    //serial layer 2 protocol driver
   bool stat = slay2.init(argv[1]);
   if (stat == false)
   {
      cout << "Failed to open tty: " << argv[1] << endl;
      return -2;
   }

   //open control channel
   Slay2Channel * controlChannel = slay2.open(CTRL_CHANNEL);
   if (controlChannel == NULL)
   {
      cout << "Failed to open control channel: " << CTRL_CHANNEL << endl;
      slay2.shutdown();
      return -3;
   }

   //open control channel
   Slay2Channel * dataChannel = slay2.open(DATA_CHANNEL);
   if (dataChannel == NULL)
   {
      cout << "Failed to open data channel: " << DATA_CHANNEL << endl;
      slay2.close(controlChannel);
      slay2.shutdown();
      return -4;
   }


   //fxClient
   DummyClient appClient;
   FileXferClient fxClient(controlChannel, dataChannel, &appClient);

   //start application
   cout << "fx_client is using " << argv[1] << endl;
   cout << "Control-Channel: " << CTRL_CHANNEL << endl;
   cout << "Data-Channel: " << DATA_CHANNEL << endl;
   cout << "Use CTRL+C to quit!" << endl << endl;

   //register signal handler, to quit program usin CTRL+C
   signal(SIGINT, &m_signal_handler);

   buffer[0] = 0;
   while (!ctrlC && (buffer[0] != 'X'))
   {
      cout << endl << "------------------------------" << endl;
      cout << "Enter command" << endl;
      cin >> buffer;
      count = strlen(buffer);
      buffer[count++] = 0; //add zero termination

      switch (buffer[0])
      {
      case FILE_XFER_CMD_PWD:
         status = fxClient.workingDirectory();
         cout << "PWD" << endl;
         cout << DummyClient::errorText(status) << endl;
         break;

      case FILE_XFER_CMD_CD:
         path = (const char *)&buffer[1];
         status = fxClient.changeDirectory(path);
         cout << "CD " << path << endl;
         cout << DummyClient::errorText(status) << endl;
         break;

      case FILE_XFER_CMD_LS:
         status = fxClient.listDirectory();
         cout << "LS" << endl;
         cout << DummyClient::errorText(status) << endl;
         break;

      case FILE_XFER_CMD_DIR:
         path = (const char *)&buffer[1];
         status = fxClient.changeListDirectory(path);
         cout << "DIR " << path << endl;
         cout << DummyClient::errorText(status) << endl;
         break;

      case FILE_XFER_CMD_MKDIR:
         path = (const char *)&buffer[1];
         status = fxClient.makeDirectory(path);
         cout << "MKDIR " << path << endl;
         cout << DummyClient::errorText(status) << endl;
         break;

      case FILE_XFER_CMD_RM:
         path = (const char *)&buffer[1];
         status = fxClient.removeFile(path);
         cout << "RM " << path << endl;
         cout << DummyClient::errorText(status) << endl;
         break;

      case FILE_XFER_CMD_DOWNLOAD:
         path = (const char *)&buffer[1];
         status = fxClient.downloadFile(path, path);
         cout << "DOWNLOAD" << endl;
         cout << DummyClient::errorText(status) << endl;
         break;

      case FILE_XFER_CMD_UPLOAD:
         path = (const char *)&buffer[1];
         status = fxClient.uploadFile(path, path);
         cout << "UPLOAD" << endl;
         cout << DummyClient::errorText(status) << endl;
         break;

      case FILE_XFER_CMD_QUIT:
         status = fxClient.quit();
         cout << "QUIT" << endl;
         cout << DummyClient::errorText(status) << endl;
         break;

      default:
         break;
      }


      //run app
      while (!fxClient.isIdle())
      {
         slay2.task();
         fxClient.task(slay2.getTime1ms());
         usleep(300); //5 chars @ 115200 bps takes about 300us
      }
      usleep(300); //5 chars @ 115200 bps takes about 300us
   }

   //shut down application
   slay2.close(dataChannel);
   slay2.close(controlChannel);
   slay2.shutdown();
   return 0;
}


#if 0
static void upload_alphabet(const unsigned char * const data, const unsigned int len,
                            Slay2Channel * dataChannel)
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
      static const char * const alphabet = "abcdefghijklmnopqrstuvwxyz" \
                                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      unsigned int count = 0;
      while (fileSize > 0)
      {
         dataChannel->send((const unsigned char *)&alphabet[count % (2*26)], 1, (fileSize != 1));
         ++count;
         --fileSize;
      }
   }
}
#endif
