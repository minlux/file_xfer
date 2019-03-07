//-----------------------------------------------------------------------------
/*!
   \file
   \brief Directory utilities
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <dirent.h>
#include <vector>
#include "dirutils.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;


/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */



bool dirutils_directory_exists(const string& path)
{
   DIR* dir = opendir(path.c_str());
   if (dir != NULL)
   {
      closedir(dir); //yes, directory exists -> can be closed again
      return true;
   }
   return false;
}


//returns size of file. return -1 if file does not exist
int dirutils_file_size(const std::string& path)
{
   return -1;
}




DirectoryNavigator::DirectoryNavigator(char delimiter)
{
   this->delimiter = delimiter;
}


string DirectoryNavigator::changeDirectory(std::string path)
{
   //change to root
   if (path.length() == 0)
   {
      current.clear();
      return current;
   }

   //relative path ?
   if (path[0] != delimiter)
   {
      path = current + path;
   }

   //parse path
   current = parsePath(path, delimiter);
   return current;
}


string DirectoryNavigator::getCurrentDirectory() const
{
   return current;
}


string DirectoryNavigator::parsePath(string path, const char delimiter)
{
   const unsigned int pathLen = path.length(); //length
   unsigned int it;  //iterator
   unsigned int prev;  //iterator
   unsigned int idx; //index
   vector<string> directories;
   string self(".");
   string parent("..");

   //setup self and parent
   self = self + delimiter;
   parent = parent + delimiter;

   //explode path
   idx = 0;
   prev = 0;
   for (it = 0; it < pathLen; ++it)
   {
      if (path[it] == delimiter)
      {
         directories.push_back(path.substr(prev, (it+1) - prev));
         prev = it + 1;
         ++idx;
      }
   }
   if (prev < it) //add the last one
   {
      directories.push_back(path.substr(prev) + delimiter);
      ++idx;
   }

   //evaluate vector
   for (it = 0; it < idx; ++it)
   {
      //empty
      if (directories[it].length() == 1)
      {
         //delete this entry
         directories[it].clear();
         continue;
      }
      //self
      if (directories[it] == self)
      {
         //delete this entry
         directories[it].clear();
         continue;
      }
      //parent
      if (directories[it] == parent)
      {
         //delete this entry
         directories[it].clear();
         //delete also the previous, non-empty entry
         for (prev = it; prev > 0; --prev)
         {
            if (directories[prev - 1].length() > 0)
            {
               directories[prev - 1].clear();
               break;
            }
         }
         continue;
      }
   }

   //assemble new path
   path.clear();
   for (it = 0; it < idx; ++it)
   {
      path += directories[it];
   }
   return path;
}
