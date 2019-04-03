//-----------------------------------------------------------------------------
/*!
   \file
   \brief Directory utilities
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <dirent.h>
#include "dirutils.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;


/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */



bool DirectoryNavigatorLinux::directoryExists(const string& path)
{
   DIR* dir = opendir(path.c_str());
   if (dir != NULL)
   {
      closedir(dir); //yes, directory exists -> can be closed again
      return true;
   }
   return false;
}
