//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief File transfer server

   The file transfer server is using two *slay2* communiction channel:
   - control channe
   - data channel

   Commands are sent via control channel. Commands are acknowledged or negative-acknowledge on control channel.
   "Big" data transfer occurs on data channel.

   Command                             Ctrl-Ch-Rx       Ctrl-Ch-Tx      Data-Ch-Rx        Data-Ch-Tx
                                       (Request)        (Response)      (Server-Rx)       (Server-Send)
   ---------------------------------------------------  -----------------------------------------------
   Print working directory             W                 a<dir>\0           -                  -
   Change directory                    C<path>           a<dir>\0           -                  -
   List directory                      L                 a                  -             <listing>\0
   Change and list directory           I<path>           a<dir>\0           -             <listing>\0
   Make directory                      M<path>           a                  -                  -
   Remove dir/file                     R<path>           a                  -                  -
   Upload file                         U<name>,<size>\0  a               <binary-data>    a *on completion*
   Download file                       D<name>           a<size>\0          -             <binary-data>
   Quit/Canel operation                Q                 a                  -             *fill by flushed*

   See the "switch-case" description and function header of CPP module for a more detailed protocol description.
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef FILE_XFER_SERVER_H
#define FILE_XFER_SERVER_H

/* -- Includes ------------------------------------------------------------ */
#include <dirent.h>
#include <cstdio>
#include "dirutils.h"
#include "slay2.h"


/* -- Defines ------------------------------------------------------------- */


/* -- Types --------------------------------------------------------------- */


class FileXferServer
{
public:
   FileXferServer(Slay2Channel * ctrl, Slay2Channel * data, const char * root = "/");
   void task();

protected:

private:
   static void onCtrlFrame(void * const obj, const unsigned char * const data, const unsigned int len); //wrapper to forward to member function
   void onCtrlFrame(const unsigned char * const data, const unsigned int len);
   static void onDataFrame(void * const obj, const unsigned char * const data, const unsigned int len); //wrapper to forward to member function
   void onDataFrame(const unsigned char * const data, const unsigned int len);

   bool onPWD_Command();

   bool onCD_Command(const char * path, bool response = true);

   bool onLS_Command(bool response = true);
   void execLS_Command();

   bool onMKDIR_Command(const char * directory);
   bool onRM_Command(const char * filename);

   bool onUPLOAD_Command(const char * filename, unsigned int size);
   void execUPLOAD_Command(const unsigned char * const data, const unsigned int len);

   bool onDOWNLOAD_Command(const char * filename);
   void execDOWNLOAD_Command();


   static const unsigned char ACK;
   static const unsigned char NACK;
   static const unsigned char ZERO;

   enum
   {
      FILE_XFER_SERVER_STATE_IDLE = 0,       //server is idle. no data-transfer in progress
      FILE_XFER_SERVER_STATE_LISTING,        //data-transfer in response to LS command
      FILE_XFER_SERVER_STATE_UPLOADING,      //data-transfer in response to UPLOAD command
      FILE_XFER_SERVER_STATE_DOWNLOADING     //data-transfer in response to DOWNLOAD command
   } state;

   std::string rootDir;
   DirectoryNavigator currentDir;
   DIR * listDirectory;
   std::string listDir;
   FILE *uploadFile;
   unsigned int uploadFileSize;
   FILE *downloadFile;


   Slay2Channel * ctrlChannel;
   Slay2Channel * dataChannel;

};





/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
