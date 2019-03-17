/*
*******************************************************************************
* Quelle:       MIDILOG1.C
* Programm:     MIDISEQ.EXE
*------------------------------------------------------------------------------
*
* For TEST the MIDISEQ Programm will do a logging of all its activities. The
* activities will be logged into a file named MIDI.LOG. Any function of the
* program may copy a string to this file.
*
* This Source ONLY contains code, which will be needed for LOGGING purposes.
*
* Prozedur(en): LogOpenFile()
*               LogWriteFile()
*               LogCloseFile()
*
*******************************************************************************
*/

#define  INCL_DOSPROCESS
#define  INCL_DOSFILEMGR                /* Include for DosQueryFileInfo      */
#define  INCL_DOSMEMMGR                 /* for DosAllocMem                   */

#include <os2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "midiseq.h"                    /* Include defintions for project    */
#include "mididlg.h"                    /* Include for all dialog-windows    */

/*---------------------------------------------------------------------------*/ 
/* Global variable declarations                                              */
/*---------------------------------------------------------------------------*/ 

HFILE         hLogFile;                 /* Filehandle from OPEN              */

/*---------------------------------------------------------------------------*/
/* Function LogOpenFile()                                                    */
/*---------------------------------------------------------------------------*/
/* This function opens the LOG Dataset MIDI.LOG                              */
/*---------------------------------------------------------------------------*/

APIRET LogOpenFile(void)

{
APIRET rc=0;
ULONG  ulActionTaken;
ULONG  ulLogSize=120000;

rc=DosOpen("MIDI.LOG",                  /* File name                         */
           &hLogFile,                   /* handle returned by open           */
           &ulActionTaken,              /* returned action indicator         */
           ulLogSize,                   /* file size if created or truncated */
           FILE_NORMAL,                 /* File Attribute NORMAL File        */
           OPEN_ACTION_CREATE_IF_NEW |  /* Open Flag                         */
           OPEN_ACTION_REPLACE_IF_EXISTS,
           OPEN_ACCESS_READWRITE |      /* Open Mode                         */
           OPEN_SHARE_DENYREADWRITE,
           NULL);
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function LogWriteFile()                                                   */
/*---------------------------------------------------------------------------*/
/* This function writes a record to the Log dataset MIDI.LOG                 */
/*---------------------------------------------------------------------------*/

APIRET LogWriteFile(CHAR *pszLogRecord)
{
APIRET rc=0;
ULONG  ulNumWritten;

rc=DosWrite(hLogFile,                   /* Log File Handle                   */
            (PVOID)pszLogRecord,
            strlen(pszLogRecord),
            &ulNumWritten);
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function LogCloseFile()                                                   */
/*---------------------------------------------------------------------------*/
/* This function closes the log dataset MIDI.LOG                             */
/*---------------------------------------------------------------------------*/

APIRET LogCloseFile(void)
{                                      
APIRET rc=0;                  

rc=DosClose(hLogFile);
return(rc);
}
