//-----------------------------------------------------------------------------
/*!
   \file
   \brief Directory utilities
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <windows.h>
#include "dirutils.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;


/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */



bool DirectoryNavigatorWindows::directoryExists(const string& path)
{
   DWORD ftyp = GetFileAttributesA(path.c_str());
   if (ftyp == (DWORD)-1) //INVALID_FILE_ATTRIBUTES
   {
      return false; //something is wrong with the path
   }

   if ((ftyp & FILE_ATTRIBUTE_DIRECTORY) != 0)
   {
      return true; //that is a directory
   }

   return false; //this is not a directory
}


