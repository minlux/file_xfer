//-----------------------------------------------------------------------------
/*!
   \file
   \brief Demo of file transfer server

   Use Linux socat tool to create 2 virtual TTY devices, connected to each other.
   E.g. socat -d -d pty,raw,echo=0 pty,raw,echo=0
   Will create /dev/pts/1, and /dev/pts/2
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "slay2.h"
#include "slay2_linux.h"
#include "file_xfer.h"
#include "file_xfer_server.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

#define CTRL_CHANNEL    (5)
#define DATA_CHANNEL    (6)

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */
static int ctrlC;

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */

void m_signal_handler(int a)
{
   ctrlC = 1;
}


int main(int argc, char * argv[])
{
   Slay2Linux slay2;    //serial layer 2 protocol driver

   if (argc < 2)
   {
      cout << "Missing argument!" << endl;
      cout << "Usage: ./fx_server <tty-dev>" << endl;
      return -1;
   }

   //init communiction driver
   bool stat = slay2.init(argv[1]);
   if (stat == false)
   {
      cout << "Failed to open tty: " << argv[1] << endl;
      return -2;
   }

   //open control channel
   Slay2Channel * controlChannel = slay2.open(CTRL_CHANNEL);
   if (controlChannel == NULL)
   {
      cout << "Failed to open control channel: " << CTRL_CHANNEL << endl;
      slay2.shutdown();
      return -3;
   }

   //open control channel
   Slay2Channel * dataChannel = slay2.open(DATA_CHANNEL);
   if (dataChannel == NULL)
   {
      cout << "Failed to open data channel: " << DATA_CHANNEL << endl;
      slay2.close(controlChannel);
      slay2.shutdown();
      return -4;
   }

   //fxServer
   FileXferServer fxServer(controlChannel, dataChannel, "/home/manuel/");

   //start application
   cout << "fx_server is using " << argv[1] << endl;
   cout << "Control-Channel: " << CTRL_CHANNEL << endl;
   cout << "Data-Channel: " << DATA_CHANNEL << endl;
   cout << "Use CTRL+C to quit!" << endl << endl;

   //register signal handler, to quit program usin CTRL+C
   signal(SIGINT, &m_signal_handler);

   //enter super-loop
   while (!ctrlC)
   {
      slay2.task();
      fxServer.task();
      usleep(300); //5 chars @ 115200 bps takes about 300us
   }

   //shut down application
   slay2.close(dataChannel);
   slay2.close(controlChannel);
   slay2.shutdown();
   return 0;
}
