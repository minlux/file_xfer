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


/* -- Implementation ------------------------------------------------------ */

class DirectoryNavigator
{
public:
   DirectoryNavigator(char delimiter = '/');
   std::string changeDirectory(std::string path = "");
   std::string getCurrentDirectory() const;
   bool isAncestorOrSelf(const DirectoryNavigator rhs);
   static bool directoryExists(const std::string& path);


private:
   static std::string parsePath(std::string path, char delimiter);

   char delimiter;
   std::string current;
};


//implementation for Linux
class DirectoryNavigatorLinux : public DirectoryNavigator
{
public:
   static bool directoryExists(const std::string& path);
};


//implementation for Windows
class DirectoryNavigatorWindows : public DirectoryNavigator
{
public:
   DirectoryNavigatorWindows() : DirectoryNavigator('\\') { }
   static bool directoryExists(const std::string& path);
};



#endif
