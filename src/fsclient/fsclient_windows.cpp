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
const FSClient::FileHandle_t FSClient::INVALID_FILE_HANDLE = FSCLIENT_INVALID_FILE_HANDLE;

/* -- Module Global Function Prototypes ----------------------------------- */


/* -- Implementation ------------------------------------------------------ */

FSClient::FSClient()
{
   this->root = "\\";
}



string FSClient::workingDirectory()
{
   return root + currentDir.getCurrentDirectory();
}


bool FSClient::changeDirectory(const string& path)
{
   //try to change directory ..
   DirectoryNavigatorWindows tmp = currentDir; //use a copy for the try
   string newDir = tmp.changeDirectory(path);
   if (newDir.length() == 0) //Root existiert immer - hier die Auflistung
   {
      currentDir = tmp;
      return true;
   }
   else //if (newDir.length() >= 2) //es muss mindestens mit einem Laufwerksbuchstaben + ':' beginnen (z.B: "c:")
   {
      if (DirectoryNavigatorWindows::directoryExists(newDir))
      {
         currentDir = tmp;
         return true;
      }
   }
   return false;
}



string FSClient::listDirectory()
{
   string directory = currentDir.getCurrentDirectory();
   if (directory.length() == 0) //Laufwerke auflisten?
   {
      return _listDrives();
   }
   //othwise
   return _listDirectory(directory);
}



string FSClient::_listDirectory(string& directory)
{
   char buffer[64];
   WIN32_FIND_DATA fd;
   string list;
   string item;

   //first list entry, ist current directory
   list = ".,";
   list += root;
   list += directory;
   list += ",,\n"; //no size, no date

   //Sonderbehandlung um Basisverzeichnis eines Laufwerks wieder zuruck zur Laufwerksauswahl zu gelangen
   if (directory.length() <= 3) //z.B. "C:\"
   {
      list += "d,..,,\n"; //no size, no date for directories
   }


   //search for all files/directories in current directory
   directory = directory + "*";
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




string FSClient::_listDrives()
{
   char drivePath[4] = { 0, ':', 0 };
   // char buffer[64];
   // WIN32_FIND_DATA fd;
   DWORD driveMask;
   char driveLetter;
   string list;
   string item;

   //first list entry, ist current directory
   list = ".,";
   list += root;
   list += ",,\n"; //no size, no date


   //get bitmask of the available drives (bit 0 -> drive A, bit 1 -> drive B, bit 2 -> drive C, ...)
   driveMask = GetLogicalDrives();
   driveLetter = 'A';
   while (driveMask != 0) //until all drives was tested
   {
      if ((driveMask & 1) != 0) //is this drive present?
      {
         //set "drive-item"
         drivePath[0] = driveLetter;
         item = "d,";
         item += drivePath;
         item += ",,\n"; //no size, no date for directories

         //add to file list
         list += item;
      }
      //prepare next
      driveMask >>= 1;
      driveLetter++;
   }
   return list;
}



bool FSClient::makeDirectory(const string& dir)
{
   string sysPath = makeSystemPath(dir);
   if (sysPath.length() > 0)
   {
      int status = CreateDirectoryA(sysPath.c_str(), NULL);
      return (status != 0);
   }
   return false;
}


bool FSClient::removeFile(const string& file)
{
   string sysPath = makeSystemPath(file);
   if (sysPath.length() > 0)
   {
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
   return false;
}



bool FSClient::openFileForRead(const string& file, FSClient::FileHandle_t * handle)
{
   string sysPath = makeSystemPath(file);
   if (sysPath.length() > 0)
   {
      HANDLE hFile = CreateFileA(sysPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, 0, NULL);
      if (hFile != INVALID_FILE_HANDLE)
      {
         *handle = (FSClient::FileHandle_t)hFile;
         return true;
      }
   }
   *handle = INVALID_FILE_HANDLE;
   return false;
}


bool FSClient::openFileForWrite(const string& file, FSClient::FileHandle_t * handle)
{
   string sysPath = makeSystemPath(file);
   if (sysPath.length() > 0)
   {
      HANDLE hFile = CreateFileA(sysPath.c_str(), GENERIC_WRITE, 0, NULL,
                                 CREATE_ALWAYS, 0, NULL);
      if (hFile != INVALID_FILE_HANDLE)
      {
         *handle = (FSClient::FileHandle_t)hFile;
         return true;
      }
   }
   *handle = INVALID_FILE_HANDLE;
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


size_t FSClient::writeToFile(FSClient::FileHandle_t file, const unsigned char * data, size_t length)
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
   int pos = path.find_first_of(':');
   if (pos > 0) //string contain ':' => it is an system-absolute path (with directory like C:\....)
   {
      return &path[pos - 1]; //Vollstandiger Pfad beginnt ein Zeichen vor dem Doppelpunkt
   }
   //Ansonsten ist es ein wohl ein relativer Pfad
   string cdir = currentDir.getCurrentDirectory();
   if (cdir.length() >= 2) //aktueller Pfad muss in einem Laufwerk sein. Also mindestens sowas wie "C:"
   {
      return cdir + path;
   }
   return ""; //auf Root kann ich nicht schreiben!!!
}