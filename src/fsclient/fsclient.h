//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief File-System Client
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef FSCLIENT_H
#define FSCLIENT_H

/* -- Includes ------------------------------------------------------------ */
#include <string>
#include "dirutils.h"


/* -- Defines ------------------------------------------------------------- */
#define  FSCLIENT_INVALID_FILE_HANDLE    ((void *)-1)

/* -- Types --------------------------------------------------------------- */


class FSClient
{
public:
   typedef void * FileHandle_t;
   static const FileHandle_t INVALID_FILE_HANDLE;


public:
   FSClient(const std::string& root = "C:\\");

   std::string workingDirectory(); //PWD
   bool changeDirectory(const std::string& path = ""); //CD<path>
   std::string listDirectory(); //LS
   bool makeDirectory(const std::string & dir); //MKDIR<dir>
   bool removeFile(const std::string& file); //RM<file>

   bool openFileForRead(const std::string& file, FileHandle_t * handle);
   bool openFileForWrite(const std::string& file, FileHandle_t * handle);
   size_t getFileSize(FileHandle_t file);

   size_t readFromFile(FileHandle_t file, unsigned char * buffer, size_t bufferSize);
   size_t writeToFile(FileHandle_t file, const unsigned char * data, size_t length);

   FileHandle_t closeFile(FileHandle_t file = FSCLIENT_INVALID_FILE_HANDLE);

   std::string makeSystemPath(const std::string& path);

private:
   std::string root;
   DirectoryNavigatorWindows currentDir;
};





/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
