//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief File transfer client
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef FILE_XFER_CLIENT_H
#define FILE_XFER_CLIENT_H

/* -- Includes ------------------------------------------------------------ */
#include <string.h>
#include "slay2.h"


/* -- Defines ------------------------------------------------------------- */


/* -- Types --------------------------------------------------------------- */
//application must implement this interface!
class FileXferClientApp
{
public:
   typedef long FileHandle_t;

public:
   virtual void onPwdResponse(int status, const std::string& dir) = 0;
   virtual void onCdResponse(int status, const std::string& dir) = 0;
   virtual void onLsResponse(int status, const std::string& dir) = 0;
   virtual void onDirResponse(int status, const std::string& dir) = 0;
   virtual void onMkdirResponse(int status) = 0;
   virtual void onRmResponse(int status) = 0;
   virtual void onDownloadResponse(int status) = 0;
   virtual void onUploadResponse(int status) = 0;
   virtual void onQuitResponse(int status) = 0;

   //file operation
   virtual bool openFileForRead(const std::string& file, FileHandle_t * handle) = 0;
   virtual bool openFileForWrite(const std::string& file, FileHandle_t * handle) = 0;
   virtual size_t getFileSize(FileHandle_t file) = 0;
   virtual size_t readFromFile(FileHandle_t file, unsigned char * buffer, size_t bufferSize) = 0;
   virtual size_t writeToFile(FileHandle_t file, const unsigned char * data, size_t length) = 0;
   virtual void closeFile(FileHandle_t file) = 0;
};



class FileXferClient
{
public:
   FileXferClient(Slay2Channel * ctrl, Slay2Channel * data, FileXferClientApp * app);
   void task(unsigned long time1ms);

   //pwd
   int workingDirectory();

   //cd <path>
   int changeDirectory(const std::string& path);

   //ls
   int listDirectory();

   //dir <path>
   int changeListDirectory(const std::string& path); //change and list directory


   //mkdir <path>
   int makeDirectory(const std::string& path);

   //rm <path>
   int removeFile(const std::string& path);

   //download <file>
   int downloadFile(const std::string& source, const std::string& destination);

   //upload <file>
   int uploadFile(const std::string& source, const std::string& destination);

   //quit ongoing transfer/operation
   int quit();


   bool isIdle();


protected:

private:
   static void onCtrlFrame(void * const obj, const unsigned char * const data, const unsigned int len); //wrapper to forward to member function
   void onCtrlFrame(const unsigned char * const data, const unsigned int len);
   static void onDataFrame(void * const obj, const unsigned char * const data, const unsigned int len); //wrapper to forward to member function
   void onDataFrame(const unsigned char * const data, const unsigned int len);

   void doFileUpload();
   void doQuit();

   Slay2Channel * ctrlChannel;
   Slay2Channel * dataChannel;

   FileXferClientApp * app;
   FileXferClientApp::FileHandle_t srcDstFile;

   unsigned char ctrlState;
   unsigned char dataState;
   unsigned long time1ms;
   unsigned long timeout1ms;
   std::string directoryList;
   size_t uploadFileSize;
   size_t downloadFileSize;
};





/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
