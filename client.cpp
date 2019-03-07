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
#include "file_xfer_client.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

#define CTRL_CHANNEL    (5)
#define DATA_CHANNEL    (6)

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */
static int ctrlC;

/* -- Module Global Function Prototypes ----------------------------------- */
static void upload_alphabet(const unsigned char * const data, const unsigned int len,
                            Slay2Channel * dataChannel);

/* -- Implementation ------------------------------------------------------ */

static void m_signal_handler(int a)
{
   ctrlC = 1;
}



int main(int argc, char * argv[])
{
   char buffer[512];
   unsigned int count;

   //check command line arguments
   if (argc < 2)
   {
      cout << "Missing argument!" << endl;
      cout << "Usage: ./fx_client <tty-dev>" << endl;
      return -1;
   }

   //init communiction driver
   Slay2Linux slay2;    //serial layer 2 protocol driver
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

   //fxClient
   FileXferClient fxClient(controlChannel, dataChannel);

   //start application
   cout << "fx_client is using " << argv[1] << endl;
   cout << "Control-Channel: " << CTRL_CHANNEL << endl;
   cout << "Data-Channel: " << DATA_CHANNEL << endl;
   cout << "Use CTRL+C to quit!" << endl << endl;

   //register signal handler, to quit program usin CTRL+C
   signal(SIGINT, &m_signal_handler);

   buffer[0] = 0;
   while (!ctrlC && (buffer[0] != 'x'))
   {
      cout << "Enter command" << endl;
      cin >> buffer;
      count = strlen(buffer);
      buffer[count++] = 0; //add zero termination
      controlChannel->send((const unsigned char *)buffer, count);

      //fake an upload
      if (buffer[0] == 'U')
      {
         upload_alphabet((const unsigned char *)buffer, count, dataChannel);
      }

      //enter super-loop
      unsigned int looper = 0;
      while (looper < 5000)
      {
         slay2.task();
         fxClient.task();
         usleep(300); //5 chars @ 115200 bps takes about 300us
         ++looper;
      }
   }

   //shut down application
   slay2.close(dataChannel);
   slay2.close(controlChannel);
   slay2.shutdown();
   return 0;
}


static void upload_alphabet(const unsigned char * const data, const unsigned int len,
                            Slay2Channel * dataChannel)
{
   //ensure the given string is zero terminated
   if (data[len - 1] == 0)
   {
      char * it = (char *)(data + 1); //i'm going to iterate through the string (and modifing it...)
      const char * fileName = it; //first argument is upload file name
      int fileSize = 0;
      char c;

      //search for KOMMA
      while (((c = *it) != 0) && (c != ',')) ++it; //this loop ends, if "it" points to ZERO or KOMMA
      *it = 0; //replace KOMMA by ZERO to terminate file name

      //determien fileSize
      if (c != 0) //did i found the KOMMA above
      {
         //yes - the data following, is the file size
         fileSize = atoi(it + 1);
      }

      //schedule file upload
      static const char * const alphabet = "abcdefghijklmnopqrstuvwxyz" \
                                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      unsigned int count = 0;
      while (fileSize > 0)
      {
         dataChannel->send((const unsigned char *)&alphabet[count % (2*26)], 1, (fileSize != 1));
         ++count;
         --fileSize;
      }
   }
}
