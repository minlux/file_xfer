//-----------------------------------------------------------------------------
/*!
   \file
   \brief File-System Client.

*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include "fsclient.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;



/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */
const FSClient::FileHandle_t FSClient::INVALID_FILE_HANDLE = INVALID_FILE_HANDLE;

/* -- Module Global Function Prototypes ----------------------------------- */


/* -- Implementation ------------------------------------------------------ */

FSClient::FSClient(const string& root)
{
   this->root = root;
}



string FSClient::workingDirectory()
{
#if 0
   return (string)"\\" + currentDir.getCurrentDirectory();
#else
   return root + currentDir.getCurrentDirectory();
#endif
}


bool FSClient::changeDirectory(const string& path)
{
   //try to change directory ..
   DirectoryNavigatorWindows tmp = currentDir; //use a copy for the try
   string newDir = tmp.changeDirectory(path);
   if (DirectoryNavigatorWindows::directoryExists(root + newDir))
   {
      currentDir = tmp;
      return true;
   }
   return false;
}


string FSClient::listDirectory()
{
   string directory = currentDir.getCurrentDirectory();
   char buffer[64];
   WIN32_FIND_DATA fd;
   string list;
   string item;

   //first list entry, ist current directory
   list = ".,";
#if 0
   list += "\\";
#else
   list += root;
#endif
   list += directory;
   list += ",,\n"; //no size, no date


   //search for all files/directories in current directory (prefixed by "root-path")
   directory = root + directory + "*";
   HANDLE hFind = ::FindFirstFile(directory.c_str(), &fd);
   if(hFind != INVALID_FILE_HANDLE)
   {
      do
      {
         //directory
         if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
         {
            if (strcmp(fd.cFileName, ".") != 0) //skip directory "."
            {
               item = "d,";
               item += fd.cFileName;
               item += ",,\n"; //no size, no date for directories
            }
         }
         //file
         else
         {
            item = "f,";
            item += fd.cFileName;
            item += ",";
            //size of file
            item += itoa(fd.nFileSizeLow, buffer, 10);
            item += ",";
            //last modification date
            SYSTEMTIME sysTime;
            FileTimeToSystemTime(&fd.ftLastWriteTime, &sysTime);
            sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d\n",
               sysTime.wYear,
               sysTime.wMonth,
               sysTime.wDay,
               sysTime.wHour,
               sysTime.wMinute,
               sysTime.wSecond
            );
            item += buffer;
         }

         //add to file list
         list += item;
      } while (::FindNextFile(hFind, &fd));
      ::FindClose(hFind);
   }
   return list;
}


bool FSClient::makeDirectory(const string& dir)
{
   string sysPath = makeSystemPath(dir);
   int status = CreateDirectoryA(sysPath.c_str(), NULL);
   return (status != 0);
}


bool FSClient::removeFile(const string& file)
{
   string sysPath = makeSystemPath(file);
   DWORD attr = GetFileAttributesA(sysPath.c_str());
   if (attr == (DWORD)-1) //INVALID_FILE_ATTRIBUTES
   {
      return false;
   }

   //otherwise
   int status;
   if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) //directory
   {
      status = RemoveDirectoryA(sysPath.c_str());
   }
   else //file
   {
      status = DeleteFileA(sysPath.c_str());
      //int err = remove(sysPath.c_str());
      //return (err == 0);
   }
   return (status != 0);
}



bool FSClient::openFileForRead(string& file, FSClient::FileHandle_t * handle)
{
   string sysPath = makeSystemPath(file);
   HANDLE hFile = CreateFileA(sysPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, 0, NULL);
   if (hFile != INVALID_FILE_HANDLE)
   {
      *handle = (FSClient::FileHandle_t)hFile;
      return true;
   }
   return false;
}


bool FSClient::openFileForWrite(string& file, FSClient::FileHandle_t * handle)
{
   string sysPath = makeSystemPath(file);
   HANDLE hFile = CreateFileA(sysPath.c_str(), GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, 0, NULL);
   if (hFile != INVALID_FILE_HANDLE)
   {
      *handle = (FSClient::FileHandle_t)hFile;
      return true;
   }
   return false;
}


size_t FSClient::getFileSize(FSClient::FileHandle_t file)
{
   if (file != INVALID_FILE_HANDLE)
   {
      return GetFileSize(file, NULL);
   }
   return 0;
}


size_t FSClient::readFromFile(FSClient::FileHandle_t file, unsigned char * buffer, size_t bufferSize)
{
   DWORD numBytesRead = 0;
   if (file != INVALID_FILE_HANDLE)
   {
      ReadFile((HANDLE)file, buffer, bufferSize, &numBytesRead, NULL);
   }
   return numBytesRead;
}


size_t FSClient::writeToFile(FSClient::FileHandle_t file, unsigned char * data, size_t length)
{
   DWORD numBytesWritten = 0;
   if (file != INVALID_FILE_HANDLE)
   {
      WriteFile((HANDLE)file, data, length, &numBytesWritten, NULL);
   }
   return numBytesWritten;
}


FSClient::FileHandle_t FSClient::closeFile(FSClient::FileHandle_t file)
{
   if (file != INVALID_FILE_HANDLE)
   {
      CloseHandle((HANDLE)file);
   }
   return INVALID_FILE_HANDLE;
}









//internal helper function
string FSClient::makeSystemPath(const string& path)
{
   string fn;
   //fn = path;
   //if (path.find_first_of(':') < 0) //string does not contain ':' => it is not an system-absolute path (with directory like C:\....)
   {
      if (path[0] == '\\') //relative to root?
      {
         fn = root + &path[1]; //prefix root
      }
      else //relative to current directory
      {
         fn = root + currentDir.getCurrentDirectory() + path; //prefix root + current directory
      }
   }
   return fn;
}