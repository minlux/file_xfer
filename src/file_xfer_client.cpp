//-----------------------------------------------------------------------------
/*!
   \file
   \brief File transfer client
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <iostream>
#include "file_xfer_client.h"
#include "file_xfer.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */


FileXferClient::FileXferClient(Slay2Channel * ctrl, Slay2Channel * data, const char * root)
{
   //init control channel
   ctrlChannel = ctrl;
   ctrlChannel->setReceiver(FileXferClient::onCtrlFrame, this);
   //init data channel
   dataChannel = data;
   dataChannel->setReceiver(FileXferClient::onDataFrame, this);
}


void FileXferClient::task(void)
{
}


void FileXferClient::onCtrlFrame(void * const obj, const unsigned char * const data, const unsigned int len)
{
   //forward to member function
   ((FileXferClient *)obj)->onCtrlFrame(data, len);
}
void FileXferClient::onCtrlFrame(const unsigned char * const data, const unsigned int len)
{
   ((unsigned char *)data)[len] = 0; //thats a hack, to add zero termination
   cout << "CTRL (" << len << ")" << endl;
   cout << (char *)data << endl;
}


void FileXferClient::onDataFrame(void * const obj, const unsigned char * const data, const unsigned int len)
{
   //forward to member function
   ((FileXferClient *)obj)->onDataFrame(data, len);
}
void FileXferClient::onDataFrame(const unsigned char * const data, const unsigned int len)
{
   ((unsigned char *)data)[len] = 0; //thats a hack, to add zero termination
   cout << "DATA (" << len << ")" << endl;
   cout << (char *)data << endl;
}