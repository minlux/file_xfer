//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief File transfer client
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef FILE_XFER_CLIENT_H
#define FILE_XFER_CLIENT_H

/* -- Includes ------------------------------------------------------------ */
#include "slay2.h"


/* -- Defines ------------------------------------------------------------- */


/* -- Types --------------------------------------------------------------- */


class FileXferClient
{
public:
   FileXferClient(Slay2Channel * ctrl, Slay2Channel * data, const char * root = "/");
   void task();

#if 0
   //cd <dir>
   bool changeLocalDirectory(std::string dir);
   todo changeRemoteDirectory(std::string dir);

   //ls
   list<FileXferStat> listLocalDirectory();
   todo listRemoteDirectory();

   //pwd
   std::string printLocalDirectory();
   todo printRemoteDirectory();

   //mkdir
   bool makeLocalDirecotry(std::string dir);
   todo makeRemoteDirectory(std::string dir);

   //rm
   bool removeLocalFile(std::string file);
   todo removeRemoteFile(std::string file);

   //download
   todo downloadFile(std::string source, std::string destination);
   todo uploadFile(std::string source, std::string destination);
#endif

protected:

private:
   static void onCtrlFrame(void * const obj, const unsigned char * const data, const unsigned int len); //wrapper to forward to member function
   void onCtrlFrame(const unsigned char * const data, const unsigned int len);
   static void onDataFrame(void * const obj, const unsigned char * const data, const unsigned int len); //wrapper to forward to member function
   void onDataFrame(const unsigned char * const data, const unsigned int len);


   Slay2Channel * ctrlChannel;
   Slay2Channel * dataChannel;
};





/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
