//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief File transfer base
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef FILE_XFER_H
#define FILE_XFER_H

/* -- Includes ------------------------------------------------------------ */
#include <string>


/* -- Defines ------------------------------------------------------------- */
//command requests
#define FILE_XFER_CMD_PWD        ((unsigned char)'W')
#define FILE_XFER_CMD_CD         ((unsigned char)'C')
#define FILE_XFER_CMD_LS         ((unsigned char)'L')
#define FILE_XFER_CMD_DIR        ((unsigned char)'I') //change directory and list its content (in one command request)
#define FILE_XFER_CMD_MKDIR      ((unsigned char)'M')
#define FILE_XFER_CMD_RM         ((unsigned char)'R')
#define FILE_XFER_CMD_UPLOAD     ((unsigned char)'U') //client sends, server receive
#define FILE_XFER_CMD_DOWNLOAD   ((unsigned char)'D') //client receive, server sends
#define FILE_XFER_CMD_QUIT       ((unsigned char)'Q')
//command responses
#define FILE_XFER_CMD_ACK        ((unsigned char)'a')
#define FILE_XFER_CMD_NACK       ((unsigned char)'n')


/* -- Types --------------------------------------------------------------- */



typedef struct
{
   //file-name
   //file-path
   //file-size
   //file-date
} FileXferStat;





/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
