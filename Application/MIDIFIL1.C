/*
*******************************************************************************
* Quelle:       MIDIFIL1.C
* Programm:     MIDISEQ.EXE
*------------------------------------------------------------------------------
* Prozedur(en): LoadMidiFile()
*               GetHeaderChunkData()       Get important Header Chunk Infos
*               GetDecodedValue()          Decode Delta Time or Length values
*               GetMetaEvent()             Get META event from Track Chunk
*               GetSysexEvent()            Get Sysex event from Track Chunk
*               GetMidiEvent()             Get MIDI event from Track Chunk
*               StoreMidiEvent()           Store MIDI event to Working Buffer
*               UpdateMidiChannelInfo()    Update WRKBUF channel information
*
*******************************************************************************
*/

#define  INCL_WINWINDOWMGR
#define  INCL_WINMESSAGEMGR
#define  INCL_WINFRAMEMGR
#define  INCL_WINSYS
#define  INCL_WINSTDFILE                /* Include for standard file dialog  */
#define  INCL_DOSFILEMGR                /* Include for DosQueryFileInfo      */
#define  INCL_DOSPROCESS
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

extern UCHAR  ucChanTab[SEQ_TRACKS];    /* MIDI Channel table                */

extern WRKBUF WrkBufMap[SEQ_TRACKS];    /* Administration Structure          */
extern HWND   hWndMainClient;           /* main client window handle         */
extern HWND   hWndMainDialog;           /* main Dialog window handle         */
CHAR          szSelectedFile[20];       /* name of selected file             */
PBYTE         pFileBegin=NULL;          /* beginning of MIDI file data       */
PBYTE         pFileWork=NULL;           /* Working Pointer for MIDI File     */
PBYTE         pFileWork2=NULL;          /* Working Pointer for MIDI File     */
ULONG         ulMidiFileSize;           /* Number of bytes read by DosRead.  */
HFILE         hMidiFile;                /* Filehandle from OPEN              */
                                        /* seltsam: wenn handle nicht global */
                                        /* geht es bei DosQueryFileInfo      */
                                        /* verloren.                         */

TRKCONTROL    *pTrkChunk;               /* pointer to Track Chunk Control    */

CHAR    szLogTxt[45];                   /* Text for Logging                  */

/* MIDI HEADER CHUNK VARIABLES AFTER CONVERSION TO INTEL-FORMAT              */
USHORT        usSMFType;                /* Type of MIDI File                 */
USHORT        usNumTrkChunks;           /* Number of Track Chunks            */
USHORT        usTicksPerQuarter;        /* Ticks per Quarter Note            */

/* MIDI TRACK CHUNK VARIABLES FOR META EVENTS                                */

/* Variables for GetMidiEvent                                                */
UCHAR         ucLoadRunStat=0;          /* Running Status for LOAD Midi file */
UCHAR         ucLoadRunStatDataLen=0;   /* Number of DATA bytes for runstat  */
UCHAR         ucMidiStatus;             /* MIDI Status Byte                  */
UCHAR         ucMidiVal1;               /* MIDI Values 1st byte              */
UCHAR         ucMidiVal2;               /* MIDI Values 2nd byte              */

/* Variables for StoreMidiEvent                                              */
ULONG         ulStoreAccTiming;         /* accumulated Timing values         */

/*---------------------------------------------------------------------------*/
/* Function LoadMidiFile()                                                   */
/*---------------------------------------------------------------------------*/
/* This function calls the standard file dialog to process the LOAD request  */
/*---------------------------------------------------------------------------*/

APIRET LoadMidiFile(void)
{
APIRET  rc=0;
USHORT  usLoop=0;                       /* loop variable for track chunks    */

FILESTATUS FileInfo;                    /* File Information from Query       */
FILEDLG    pfdFileDlg;                  /* FILEDLG: standard dialog struct.  */
CHAR       pszDialogTitle[11]="LOAD MIDI";        /* dialog title            */
CHAR       pszFileSpec[CCHMAXPATH] ="*.MID";      /* File specification      */
ULONG      ulActionTaken;               /* for OPEN File                     */

ULONG      ulWrkDeltaTime;              /* Workvariable for DeltaTime        */
UCHAR      ucWrkEvent;                  /* Workvariable for Event in Chunk   */

memset(&pfdFileDlg,0,sizeof(FILEDLG));  /* Init of structure FILEDLG         */
pfdFileDlg.cbSize = sizeof(FILEDLG);    /* Size of structure FILEDLG         */
pfdFileDlg.fl = FDS_HELPBUTTON    |     /* HELP Button                       */
                FDS_CENTER        |                                
                FDS_OPEN_DIALOG;        /* OPEN dialog type                  */
pfdFileDlg.pszTitle=pszDialogTitle;     /* Title string                      */
strcpy(pfdFileDlg.szFullFile,pszFileSpec);        /* File Filter             */

WinFileDlg(HWND_DESKTOP,                /* Display Dialog and load file      */
           hWndMainClient,                                                   
           &pfdFileDlg);                                           

if(pfdFileDlg.lReturn==DID_OK)
   strcpy(szSelectedFile,pfdFileDlg.szFullFile);
else
   return(10);

/* szSelectedFile now contains the selected filename from dialog             */
/* now this file has to be processed                                         */
/* First load the File into a buffer, pointed to by pFileBegin.              */

rc=DosOpen(szSelectedFile,              /* File name                         */
           &hMidiFile,                  /* handle returned by open           */
           &ulActionTaken,              /* returned action indicator         */
           0,                           /* file size if created or truncated */
           FILE_NORMAL,                 /* File Attribute NORMAL File        */
           FILE_OPEN,                   /* Open Flag                         */
           OPEN_ACCESS_READONLY |       /* Open Mode                         */
           OPEN_SHARE_DENYWRITE,
           NULL);

rc=DosQueryFileInfo(hMidiFile,          /* Get File Size                     */
                    FIL_STANDARD,       /* Level 1 file information          */
                    &FileInfo,          /* returnvalues from query           */
                    sizeof(FileInfo));

rc=Alloc_Buffer((ULONG)FileInfo.cbFile, /* Allocate MIDIFILE Buffer **********/
                (PCHAR*)&pFileBegin);
if (rc>0)                                                              
   {                                                                   
   Display_Msg((USHORT)rc,hWndMainClient,"Fehler in ALLOC MIDIFILE BUFFER");
   }       
                                                            
rc=DosRead(hMidiFile,                   /* Copy File to allocated memory     */
           (PVOID)pFileBegin,           /* Address where to copy data        */
           FileInfo.cbFile,             /* Number of bytes to copy           */
           &ulMidiFileSize);            /* number of bytes actually read     */

/* now read the header chunk data from the file and convert to INTEL format  */

rc=GetHeaderChunkData((PVOID)pFileBegin,          /* File start pointer      */
                      &usSMFType,                 /* Return value            */
                      &usNumTrkChunks,            /* Return value            */
                      &usTicksPerQuarter);        /* Return value            */

/* now get the track chunk data from the file and store to the               */
/* Extents of the Working Buffer.                                            */
/* 1. Allocate a structure of type TRKCONTROL with usNumTrkChunks entries    */
/*    fill this structure with length of track data                          */
/* 2. Determine the number of MIDI Events in each Track Chunk, which is      */
/*    needed for allocation of correct number of extents.                    */
/*    -> Help function for this purpose needed                               */
/* 3. Convert the data from the Track Chunks to WorkBuffer format and store  */
/*    them.                                                                  */
/* IMPORTANT: Translate META Events and store appropriate data               */

/* Allocate MEMORY for TRKCONTROL Structure                                  */
rc=Alloc_Buffer((ULONG) (usNumTrkChunks * sizeof(TRKCONTROL)),
                (PCHAR*)&pTrkChunk);
if (rc>0)                                                                      
   {                                                                           
   Display_Msg((USHORT)rc,hWndMainClient,"Fehler in ALLOC TRKCONTROL BUFFER");
   }                                                                           

/* Check, if more than a specified number of track-Chunks. This is needed,   */
/* if more than 64 track chunks are contained, which cannot be loaded        */

/* Start a loop which runs usNumTrkChunks times, fill in size values         */
pFileWork=pFileBegin+14;                /* Skip MIDI Header chunk info       */
for(usLoop=0;usLoop<usNumTrkChunks;usLoop++)
   {                                    /* SCAN ALL TRACK CHUNKS             */
   (pTrkChunk+usLoop)->ulTrkLength=     /* First get Chunk Length Info       */
                (*(pFileWork+4))*16777216
               +(*(pFileWork+5))*65536
               +(*(pFileWork+6))*256
               +(*(pFileWork+7));
   
     /* maybe also store Chunk Start addresses??? (for nav. functions??)   */

   pFileWork2=pFileWork+8;              /* Skip header+length of track chunk */
   ulStoreAccTiming=0;                  /* Reset Accu Timing for new track ! */
   rc=DeleteAllTrackExtents((UCHAR)usLoop);       /* Delete Extents          */

   while(pFileWork2<pFileWork+8+(pTrkChunk+usLoop)->ulTrkLength)
      {                                    /* SCAN ALL EVENTS IN THIS CHUNK  */
      rc=GetDecodedValue(&pFileWork2,      /* Get decoded DELTA TIME. The    */
                         &ulWrkDeltaTime); /* pointer will be updated.       */
      ulStoreAccTiming+=ulWrkDeltaTime;    /* Calculate ABSOLUTE Time        */

      /* Now analyze the Event Code Byte following the Delta Time:           */
      /* FFh:  META EVENT                                                    */
      /* F0h:  System Exclusive Event                                        */
      /* else: MIDI Event or running status assumed                          */
      ucWrkEvent=*(pFileWork2);            /* Get Event code                 */
      switch(ucWrkEvent)
         {
         case 0xFF:                        /* META EVENT                     */
            GetMetaEvent(&pFileWork2,      /* pointer will be updated        */
                         usLoop);
         break;

         case 0xF0:                        /* System Exclusive Event         */
/*T*/       sprintf(szLogTxt,"Found SYSEX...\n\0",0);
/*T*/       LogWriteFile(szLogTxt);
/*          GetSysexEvent(&pFileWork2);       pointer will be updated        */
/*T*/       /* End the processing, if sysex found by leaving loop            */
/*T*/       pFileWork2=pFileWork+8+(pTrkChunk+usLoop)->ulTrkLength;  /* END  */
         break;

         default:                          /* MIDI Event or runstat          */
/*T*/       if(usLoop<64)                  /* Test, copy only for Track 0-7  */
/*T*/       {
            rc=GetMidiEvent(&pFileWork2,   /* get the MIDI event data        */
                            &ucMidiStatus, /* pointer update!!               */
                            &ucMidiVal1,
                            &ucMidiVal2);
            rc=StoreMidiEvent((UCHAR)usLoop,      /* Tracknumber             */
                              ulStoreAccTiming,   /* Abs.Time to store       */
                              ucMidiStatus,  
                              ucMidiVal1,  
                              ucMidiVal2);
            UpdateMidiChannelInfo((UCHAR)usLoop,  /* Update WRKBUF Channel   */
                                  ucMidiStatus);
            WrkBufMap[usLoop].ulNumMidiEvents+=1; /* INC MIDI event count    */
/*T*/       }
/*T*/       else
/*T*/       {
/*T*/       pFileWork2=pFileWork+8+(pTrkChunk+usLoop)->ulTrkLength;  /* END  */
/*T*/       }
         break;
         }                                 /* End SWITCH                     */
      }                                    /* End WHILE (end of trkchunk)    */
   /* now first store FC end with leading timing byte !!!!                   */
   if(usLoop<64)
      {
/*    if(WrkBufMap[usLoop].ulNumMidiEvents>0)     FC-End only needed, if     */
/*       {                                        other MIDI events found    */
         ulStoreAccTiming+=120;
         WrkBufMap[usLoop].ulMaxTiming=ulStoreAccTiming;
         rc=StoreMidiEvent((UCHAR)usLoop,      /* Tracknumber             */
                           ulStoreAccTiming,   /* Delta Time value        */
                           0xFC,
                           0,                  /* value not important     */
                           0);
         sprintf(szLogTxt,"FC End written for trk &2d...\n\0",usLoop);
         LogWriteFile(szLogTxt);
/*       }   */
      }
   /* set pFileWork to the next Track Chunk Header                           */
   pFileWork=pFileWork+8+(pTrkChunk+usLoop)->ulTrkLength; /* an sich unn”tig */
   }

rc=Free_Buffer((PVOID)pFileBegin);      /* Free MIDIFILE Buffer **************/
if (rc>0)                                                                      
   {                                                                           
   Display_Msg((USHORT)rc,hWndMainClient,"Fehler in FREE MIDIFILE BUFFER");
   }                                                                           

return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function GetHeaderChunkData()                                             */
/*---------------------------------------------------------------------------*/
/* This function is called to extract the Header Chunk Data from a MIDI file */
/* which has been loaded.                                                    */
/* Passed parameters: Pointer PVOID   to Buffer containing the file data     */
/*                    Pointer PULONG  to return value HeaderLength           */
/*                    Pointer PUSHORT to return value SMF type               */
/*                    Pointer PUSHORT to return value Number of Track Chunks */
/*                    Pointer PUSHORT to return value Ticks per Quarter      */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET GetHeaderChunkData(PVOID    pFileStart,
                          USHORT   *pusSMFType,
                          USHORT   *pusNumTrkChunks,
                          USHORT   *pusTicksPerQuarter)
{                                                                              
APIRET  rc=0;
HDCHUNKDATA   *pHeadChunk;              /* Header Chunk variable             */
UCHAR         ucHeader[4];              /* Work Variable for MThd..          */
ULONG         ulHeadLength;             /* length of Header chunk            */

pHeadChunk=(HDCHUNKDATA*)pFileStart;    /* Pointer to file as type headchnk  */
 
ucHeader[0]=pHeadChunk->ucHeader[0];    /* Get MThd. This is not given back  */
ucHeader[1]=pHeadChunk->ucHeader[1];    /* but can be used to determine, if  */
ucHeader[2]=pHeadChunk->ucHeader[2];    /* pointer points to valid header..  */
ucHeader[3]=pHeadChunk->ucHeader[3];       

/* at this point: check for ucHeader - MThd, if not: error                   */
                                     
ulHeadLength=(pHeadChunk->ucHeadLength[0]*16777216)   /* not given back, the */
            +(pHeadChunk->ucHeadLength[1]*65536)      /* value isn't impor-  */
            +(pHeadChunk->ucHeadLength[2]*256)        /* tant for the appl.  */
            +(pHeadChunk->ucHeadLength[3]);       
                            
/* at this point: if not 6: MIDI file has not format, which is known by      */
/* the program. the program expects length of 6 bytes for header data.       */

*pusSMFType=(pHeadChunk->ucSMFType[0]*256)            /* SMF Type            */
           +(pHeadChunk->ucSMFType[1]);             
                         
*pusNumTrkChunks=(pHeadChunk->ucNumTrkChunks[0]*256)  /* number of track     */
                +(pHeadChunk->ucNumTrkChunks[1]);     /* chunks              */

*pusTicksPerQuarter=(pHeadChunk->ucTicksPerQuarter[0]*256)  /* ticks per     */
                   +(pHeadChunk->ucTicksPerQuarter[1]);     /* quarter note  */
 
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function GetDecodedValue()                                                */
/*---------------------------------------------------------------------------*/
/* This function is called to calculate the MIDI Delta Time value or the     */
/* coded length field in a Track Chunk, pointed to by the passed pointer.    */
/* It decodes the variable format and sets the pointer to the offset of the  */
/* Event Code which follows this variable length info.                       */
/* Passed parameters: Pointer PPVOID  to Pointer with Offset of Delta Time   */
/*                    Pointer PULONG  to return the decoded value            */
/*---------------------------------------------------------------------------*/

APIRET GetDecodedValue(PCHAR *ppOffset,
                       ULONG *pDecodedValue)
{
APIRET rc=0;
ULONG  ulWork=0;                        /* Working variable                  */
CHAR   *pOffset;

pOffset=*ppOffset;
do {
   ulWork=ulWork*128;
   ulWork+=(*(pOffset)) - (128*((*pOffset)>=128));
   pOffset++;
   } while(*(pOffset-1) >= 128);        /* only proceed while bit 7 set...   */
*pDecodedValue=ulWork;
*ppOffset=pOffset;                      /* return new offset pointer         */
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function GetMetaEvent()                                                   */
/*---------------------------------------------------------------------------*/
/* This function translates the META event, which is found at the offset of  */
/* the passed pointer.                                                       */
/* Passed parameters: - Pointer to the pointer which indicates the offset    */
/*                    of the META event code. The Pointer will be set to the */
/*                    next offset following this event.                      */
/*                    - INDEX of currently analyzed Track chunk              */
/*---------------------------------------------------------------------------*/

APIRET GetMetaEvent(PCHAR  *ppOffset,
                    USHORT usChunkIdx)
{                                          
APIRET rc=0;                               
CHAR   *pOffset;
CHAR   *pMetaEventDATA;                 /* Save Pointer for META Event DATA  */
UCHAR  ucWrkMetaEventType;              /* Workvariable for META Event Type  */
ULONG  ulMetaEventLength;               /* Length of META Event              */
USHORT i;                               /* Help variable                     */

pOffset=*ppOffset;                      /* Offset to META Event code         */
pOffset++;                              /* Offset to META Event TYPE         */
ucWrkMetaEventType=*(pOffset);
pOffset++;                              /* Offset to META Event length       */

rc=GetDecodedValue(&pOffset,            /* decode the META Event lentgh data */
                   &ulMetaEventLength); /* to ulMetaEventLength              */

pMetaEventDATA=pOffset;                 /* Save offset                       */

switch(ucWrkMetaEventType)              /* Analyze TYPE of META Event        */
   {                                    /* pOffset is workvariable           */
   case MET_SEQNO:                      /* Sequence Number                   */
/*T*/ sprintf(szLogTxt,"Found META SEQNO...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                  
   break;                                                                

   case MET_GENTEXT:                    /*                                   */
/*T*/ sprintf(szLogTxt,"Found META GENTEXT...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                                  
   break;                                                                

   case MET_COPYRIGHT:                  /*                                   */
/*T*/ sprintf(szLogTxt,"Found META GENTEXT...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                         
   break;            

   case MET_TRKNAME:                    /* Track Name/sequence descr.(SMF2)  */
/*T*/ sprintf(szLogTxt,"Found META TRKNAME...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                         
      if(ulMetaEventLength<TRKNAME_MAXLEN)        /* only copy if fits       */
         {
         for(i=0;i<ulMetaEventLength;i++)
            {
            (pTrkChunk+usChunkIdx)->szTrkName[i]=
            *(pOffset+i);
            }
         (pTrkChunk+usChunkIdx)->szTrkName[i]=0;  /* String delimiter        */
         strcpy(WrkBufMap[usChunkIdx].chTrkName,      /* Chunk = Track !     */
                (pTrkChunk+usChunkIdx)->szTrkName);
         }
   break;     

   case MET_INSTRUMENT:                 /*                                   */
/*T*/ sprintf(szLogTxt,"Found META INSTRUMENT...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                         
      if(ulMetaEventLength<INST_MAXLEN)           /* only copy if fits       */
         {                                                                     
         for(i=0;i<ulMetaEventLength;i++)                                      
            {                                                                  
            (pTrkChunk+usChunkIdx)->szInstrument[i]=
            *(pOffset+i);                                                      
            }                                                                  
         (pTrkChunk+usChunkIdx)->szInstrument[i]=0;   /* String delimiter    */
         strcpy(WrkBufMap[usChunkIdx].chInstrument,   /* Chunk = Track !     */
                (pTrkChunk+usChunkIdx)->szInstrument);
         }                                                                     
   break;     

   case MET_SONGTEXT:                   /*                                   */
/*T*/ sprintf(szLogTxt,"Found META SONGTEXT...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                                          
   break;     

   case MET_MARK:                       /*                                   */
/*T*/ sprintf(szLogTxt,"Found META MARK...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                          
   break;     

   case MET_CUEPOINT:                   /*                                   */
/*T*/ sprintf(szLogTxt,"Found META CUEPOINT...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                          
   break;         

   case MET_CHANPREFIX:                 /*                                   */
/*T*/ sprintf(szLogTxt,"Found META CHANPREFIX...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                            
   break;         

   case MET_ENDOFTRACK:                 /*                                   */
/*T*/ sprintf(szLogTxt,"Found META ENDOFTRACK...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                            
   break;         

   case MET_SETTEMPO:                   /*                                   */
/*T*/ sprintf(szLogTxt,"Found META SETTEMPO... at %5d\n\0",
                       ulStoreAccTiming);
/*T*/ LogWriteFile(szLogTxt);                             
   break;         

   case MET_SMPTE:                      /*                                   */
/*T*/ sprintf(szLogTxt,"Found META SMPTE...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                           
   break;            

   case MET_TIMESIG:                    /*                                   */
/*T*/ sprintf(szLogTxt,"Found META TIMESIG...  at %5d\n\0",
                       ulStoreAccTiming); 
/*T*/ LogWriteFile(szLogTxt);                          
   break;            

   case MET_SEQUENCER:                  /*                                   */
/*T*/ sprintf(szLogTxt,"Found META SEQUENCER...\n\0",0);
/*T*/ LogWriteFile(szLogTxt);                            
   break;            

   default:                             /* unrecognized Meta Event Type      */
/*T*/ sprintf(szLogTxt,"Found unknown META: %2x\n\0",ucWrkMetaEventType);
/*T*/ LogWriteFile(szLogTxt);                           
                                        /* Those types are skipped           */
   break;
   }                                    /* End switch                        */

/* calculate the new offset using ulMetaEventLength, which also can be read  */
/* for unknown event types                                                   */
pMetaEventDATA+=ulMetaEventLength;      /* Offset to next Delta Time         */

*ppOffset=pMetaEventDATA;               /* return new offset pointer         */
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: GetMidiEvent()                                                  */
/*---------------------------------------------------------------------------*/
/* The function is called, if during LOAD of a MIDI File, a MIDI event type  */
/* is found in the current Track Chunk. The function analyzes this event and */
/* returns the data found. The passed pointer is set to the delta-time of    */
/* the next Event.                                                           */
/* Parameters: - Pointer to the Startpointer of the Event                    */
/*             - Index of track where to copy ???                            */
/*             - pointers to the result variables                            */
/* Return:     RC,  0 if ok.                           o.k.                  */
/*                  2 if unknown Data                  E R R O R             */
/* Results:    The variables pointed to when calling are filled with found   */
/*             data: Timing, Status, Value1, Value2.                         */
/*             The offset is actualized to next event.                       */
/* If running status is found, the MIDI Status Byte is restored and set as   */
/* return Value. So the result will contain always FULL MIDI messages.       */
/* This is, because when merging multiple tracks (0-63), running status must */
/* be calculated at merging time.                                            */
/* Unused Bytes are set to 0.  (Vorerst nicht, u.U. nur performanceverlust   */
                                                                               
/* IMPORTANT: TIMING is already analyzed by calling function. The pointer    */
/*            passed really points to the MIDI Status or running status!!!   */
/*            so only this data has to be analyzed and copied                */
                                                                               
/*---------------------------------------------------------------------------*/
                                                                               
                                                                               
APIRET GetMidiEvent(PCHAR  *ppOffset,   /* ptr to offset ptr                 */
                    UCHAR  *pStatus,    /* return-value                      */
                    UCHAR  *pValue1,    /* return-value                      */
                    UCHAR  *pValue2)                                           
{                                                                              
CHAR *pOffset;                          /* Work Pointer                      */
APIRET rc=0;                                                                   
                                                                               
pOffset=*ppOffset;                      /* init pOffset                      */
                                                                               
if(*pOffset<0x80)                       /* if < 0x80: Assume running status  */
   {                                                                           
   *pStatus=ucLoadRunStat;              /* restore from last runningstatus   */
                                                                               
   if(ucLoadRunStatDataLen>0)           /* first databyte                    */
      {                                                                        
      *pValue1=*(pOffset);                                                     
      }                                                                        
   if(ucLoadRunStatDataLen>1)           /* second databyte                   */
      {                                                                        
      pOffset++;                                                               
      *pValue2=*(pOffset);                                                     
      }                                                                        
                                                                               
   *ppOffset=pOffset+1;                 /* NEXT EVENT                        */
   return(0);                                                                  
   }                                                                           
else if(*pOffset<=0xBF)                 /* 4 Bytes incl. Timingbyte          */
   {                                                                           
   *pStatus=*pOffset;                   /* Store Statusbyte                  */
   /* save running status information for next event analyze                 */
   ucLoadRunStatDataLen=2;              /* 2 Databytes for MIDI Message      */
   ucLoadRunStat=*pOffset;              /* save as running Status            */
                                                                               
   pOffset++;                           /* offset of 1st databyte            */
   *pValue1=*(pOffset);                 /* Store 1st databyte                */
                                                                               
   pOffset++;                                                                  
   *pValue2=*(pOffset);                 /* Store 2nd databyte                */
                                                                               
   *ppOffset=pOffset+1;                 /* NEXT EVENT                        */
   return(rc);
   }                                                                           
else if(*pOffset<=0xDF)                 /* 3 Bytes incl. Timingbyte          */
   {                                                                           
   *pStatus=*pOffset;                   /* Store Statusbyte                  */
   /* save running status information for next event analyze                 */
   ucLoadRunStat=*pOffset;              /* save as running Status            */
   ucLoadRunStatDataLen=1;              /* 1 Databyte for MIDI Message       */
                                                                               
   pOffset++;                                                                  
   *pValue1=*(pOffset);                 /* Store 1st databyte                */
                                                                               
   *ppOffset=pOffset+1;                 /* NEXT EVENT                        */
   return(rc);
   }                                                                           
else if(*pOffset<=0xEF)                 /* 4 Bytes incl. Timingbyte          */
   {                                                                           
   *pStatus=*pOffset;                   /* Store Statusbyte                  */
   /* save running status information for next event analyze                 */
   ucLoadRunStat=*pOffset;              /* save as running Status            */
   ucLoadRunStatDataLen=2;              /* 2 Databytes for MIDI Message      */
                                                                               
   pOffset++;                                                                  
   *pValue1=*(pOffset);                 /* Store 1st databyte                */
                                                                               
   pOffset++;                                                                  
   *pValue2=*(pOffset);                 /* Store 2nd databyte                */
                                                                               
   *ppOffset=pOffset+1;                 /* NEXT EVENT                        */
   return(rc);
   }                                                                           
else                                    /* only F9 and FC are valid          */
   {                                                                           
   *pStatus=*pOffset;                   /* Store Statusbyte                  */
   /* save running status information for next event analyze                 */
   ucLoadRunStat=*pOffset;              /* save as running Status            */
   ucLoadRunStatDataLen=0;              /* No Databytes for MIDI Message     */
                                                                               
   *ppOffset=pOffset+1;                 /* NEXT EVENT                        */
   return(rc);
   }                                                                           
/* This point should not be reached                                          */
rc=2;
return(rc);
}                                      

/*---------------------------------------------------------------------------*/
/* Function: StoreMidiEvent()                                                */
/*---------------------------------------------------------------------------*/
/* This function stores the analyzed MIDI event data to the Working buffer.  */
/* Passed parameters: - Destination Track UCHAR                              */
/*                    - Absolute Time     ULONG                              */
/*                    - MIDI Status byte  UCHAR                              */
/*                    - Data 1            UCHAR                              */
/*                    - Data 2            UCHAR                              */
/* Data is stored to Workbuf like this also happens while recording with     */
/* dynamic extent allocation.                                                */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET StoreMidiEvent(UCHAR ucTrackNum,                                        
                      ULONG ulTiming,   /* Absoute!! time value              */
                      UCHAR ucStatus,                                          
                      UCHAR ucValue1,                                          
                      UCHAR ucValue2)                                          
{                                                                              
USHORT usActExtIdx;                     /* some help variables               */
USHORT usActEvtIdx;                                                            
PTRKDATA pTrkExt;                                                              
APIRET rc=0;                                                                   

/*---------------------------------------------------------------------------*/
/* Possible combinations  :                                                  */
/*                               pTrkExt            usActEvtIdx              */
/*                                                                           */
/* NO EXTENT ALLOCATED    :       NULL               don't care              */
/* EXTENT HAS FREE EVENTS :       not NULL           0-408                   */
/* EXTENT IS FULL         :       not NULL           409                     */
/*---------------------------------------------------------------------------*/
/* If no more extents are available, CreateWrkBufExtent function will fail   */
/* and indicate this. So ActExtIdx is not important for this function        */
                                                                               
/* First check the current extent indicated by WrkBufMap. Check, if it is    */
/* already allocated (Pointer not NULL) and if it has free events (Event-    */
/* index <409). If extent not allocated or full: allocate new extent.        */
                                                                               
usActExtIdx=WrkBufMap[ucTrackNum].usActExtentIdx;                              
usActEvtIdx=WrkBufMap[ucTrackNum].usActEventIdx;                               
pTrkExt=WrkBufMap[ucTrackNum].pTrkExtent[usActExtIdx];                         
                                                                               
if((pTrkExt==NULL)||(usActEvtIdx>408))  /* if extent is not allocated or full*/
   {                                                                           
   if(CreateWrkBufExtent(ucTrackNum)!=0)                                       
      {                                                                        
      return(5);                                                               
      }                                                                        
   DosBeep(6000,100);                   /* Extent angelegt, testbeep         */
   usActEvtIdx=0;                       /* After creation: Event Idx = 0     */
   }                                                                           
                                                                               
usActExtIdx=WrkBufMap[ucTrackNum].usActExtentIdx;      /* refresh index      */
pTrkExt=WrkBufMap[ucTrackNum].pTrkExtent[usActExtIdx]; /* refresh address    */
                                                                               
/* Now copy the MIDI event to the working buffer at the event pointed to     */
/* by the indices.                                                           */

(pTrkExt+usActEvtIdx)->ulTiming=ulTiming;                                  
(pTrkExt+usActEvtIdx)->ucMidiStatus=ucStatus;                                  
(pTrkExt+usActEvtIdx)->ucData1=ucValue1;                                       
(pTrkExt+usActEvtIdx)->ucData2=ucValue2;                                       
                                                                               
                                                                               
/**************************************************************************/   
/* should pointer chain also be built here ?                              */   
/**************************************************************************/   
                                                                               
WrkBufMap[ucTrackNum].usActEventIdx++;    /* Event Index to next event??? */   
                                                                               
return(rc);                                                                    
}

/*---------------------------------------------------------------------------*/
/* Function: UpdateMidiChannelInfo()                                         */
/*---------------------------------------------------------------------------*/
/* This function does an update to the ucMidiChannel Byte in the Working     */
/* buffer for the actual Track passed.                                       */
/* Following is set: if only ONE Midi Channel in Track: set to this channel  */
/*                   if multiple Midi Channels        : set to 0xFF          */
/* Also an update to ucChanTab is done, which is used by the driver to do an */
/* online channel change while playback is active. This Table needs an       */
/* update call to the driver after setting the values.                       */
/*---------------------------------------------------------------------------*/

APIRET UpdateMidiChannelInfo(UCHAR ucTrkNum,
                             UCHAR ucStatus)
{
APIRET rc=0;
UCHAR  ucMidiChannel;

if(WrkBufMap[ucTrkNum].ucMidiChannel==17)         /* MULTIPLE already set    */
   return(rc);

ucMidiChannel=(ucStatus&0x0F)+1;        /* extract MIDI Channel Number 1-16  */
if(ucMidiChannel==WrkBufMap[ucTrkNum].ucMidiChannel)
   {
   return(rc);
   }
/* if this point is reached: different channel in Status than in Wrkbuf.     */
/* Check: if Wrkbuf Channel = 0 (after init): store channel, if != 0:        */
/* another channel has already been stored, i.e. this is MULT.               */

if(WrkBufMap[ucTrkNum].ucMidiChannel!=0)
   {
   WrkBufMap[ucTrkNum].ucMidiChannel=17;
   ucChanTab[ucTrkNum]=16;              /* set ucChanTab to 16 (MULT)        */
   MPU_ChannelChange();                 /* write to driver                   */
   return(rc);
   }
WrkBufMap[ucTrkNum].ucMidiChannel=ucMidiChannel;  /* value 1..16             */
ucChanTab[ucTrkNum]=ucMidiChannel-1;    /* set ucChanTab to 0..15            */
MPU_ChannelChange();                    /* write to driver                   */
return(rc);
}
