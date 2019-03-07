//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Directory utilities
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef DIRUTILS_H
#define DIRUTILS_H

/* -- Includes ------------------------------------------------------------ */
#include <string>


/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */

/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */
bool dirutils_directory_exists(const std::string& path);
int dirutils_file_size(const std::string& path); //returns size of file. return -1 if file does not exist


/* -- Implementation ------------------------------------------------------ */

class DirectoryNavigator
{
public:
   DirectoryNavigator(char delimiter = '/');
   std::string changeDirectory(std::string path = "");
   std::string getCurrentDirectory() const;

private:
   static std::string parsePath(std::string path, char delimiter);

   char delimiter;
   std::string current;
};




#endif
