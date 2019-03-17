/*                                                                             
*******************************************************************************
* Quelle:       MIDIHLP1.C
* Programm:     MIDISEQ.EXE
*------------------------------------------------------------------------------
* Beschreibung: Diese Datei enthÑlt Hilfsfunktionen, die an vielen Stellen im
*               Programm zur UnterstÅtzung gebraucht werden, d.h. die Datei ist
*               eine Art 'Werkzeugkiste'.
*                                                                              
*------------------------------------------------------------------------------
* Prozedur(en): Alloc_Buffer   (ULONG,PCHAR *)
*               Free_Buffer
*               Display_Msg    (??)
*               GetLatestCapturedMidiEvents
*               AnalyzeCapturedMidiEvent
*               AppendMidiEvent
*               InitWrkBufMap  (WRKBUF)
*               CreateWrkBufExtent
*               DeleteAllTrackExtents
*               DeleteSpecificTrackExtent
*               FillPlayBuffer
*               BuildMPUPlayData
*******************************************************************************
*/
 
/*---- Includes -------------------------------------------------------------*/

#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES
#define INCL_DOSMEMMGR

#define   INCL_WINWINDOWMGR      /* zugefÅgt  */
#define   INCL_WINWINDOWMGR
#define   INCL_WINBUTTONS
#define   INCL_WINENTRYFIELDS
#define   INCL_WINDIALOGS
#define   INCL_WININPUT
#define   INCL_WINLISTBOXES
#define   INCL_WINTIMER

#define INCL_BASE
#include <os2.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <bsememf.h>                    /* Memory alloc flags                */

#include "midiseq.h"                    /* Include Definitionen fÅr Projekt  */
#include "mididlg.h"                    /* Include fÅr alle Dialogfenster    */

/*---- Global Variables -----------------------------------------------------*/

UCHAR  ucTurnaroundInd=0;               /* Turnaroundindicator  for test!!!  */
CHAR   DlgText[45];       
extern UCHAR   ucNumMemoryObjects;      /* Memory objects alloc counter      */

extern USHORT  usPlayRd;                /* Next offset to play               */
extern USHORT  usPlayWr;                /* Next offset to copy data to       */
UCHAR  ucPlayRunStat;                   /* Runningstatus for Playback        */
UCHAR  ucPlayRunStatLength;             /* Length of Event Playback          */

extern USHORT  usCaptRd;                /* Last offset read from             */
extern USHORT  usCaptWr;                /* Last offset data captured tod     */
UCHAR  ucRecRunStat;                    /* Runningstatus for Recording       */
UCHAR  ucRecRunStatLength;              /* Length of Event Recording         */

extern HWND    hWndMainClient;          /* main client window handle         */
extern WRKBUF  WrkBufMap[SEQ_TRACKS];   /* Administration Structure          */
extern PCHAR   pCaptBuf;                /* Record Capture Buffer             */
extern PCHAR   pPlayBuf;                /* Play Buffer                       */
extern HWND    hWndMainDialog;          /* Handle of Main Dialog Window      */

extern UCHAR   ucDispatchList[65];      /* FillPlayBuffer Dispatcher List    */

extern UCHAR  ucTaktmassZaehler;        /* For test MIDI Timing conversion   */
extern UCHAR  ucTaktmassNenner; 
extern UCHAR  ucTicksPerQuarter;


/*---------------------------------------------------------------------------*/
/* Function: Alloc_Buffer()                                                  */
/*---------------------------------------------------------------------------*/
/* Diese Funktion allokiert einen Puffer gewÅnschter Grî·e                   */
/* öbergabe: USHORT buffersize                                               */
/*           PVOID  Zeiger auf Variable, die den Pointer, der von AllocMem   */
/*                  zurÅckkommt, aufnimmt.                                   */
/*---------------------------------------------------------------------------*/

APIRET Alloc_Buffer(ULONG ulBufSize,PCHAR *ppBufAddr)
{
APIRET  rc;
PCHAR   pBufAddr;
ULONG   ulAllocFlags;                   /* Allocation Flags                  */
ULONG   num;
ulAllocFlags=PAG_WRITE | PAG_READ | PAG_EXECUTE | PAG_COMMIT;                  
rc=DosAllocMem((PVOID)&pBufAddr,        /* Midi Buffer Address (returned)    */
               ulBufSize,               /* Midi Buffer Size                  */
               ulAllocFlags);           /* Read,Write,Execute,COMMIT!!!      */
if(rc==0)
   {
   for(num=0;num<ulBufSize;num++)       /* init allocated memory to 0        */
      {
      *(pBufAddr+num)=0;                /* Wert  0  speichern                */
      }
   *ppBufAddr=pBufAddr;                 /* Copy Buffer Address to result     */
   ucNumMemoryObjects++;                /* increment memory object counter   */
   }
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: Free_Buffer()                                                   */
/*---------------------------------------------------------------------------*/
/* This functions frees allocated memory if no longer needed.                */
/* Parameter: PVOID Pointer to memory object                                 */
/*---------------------------------------------------------------------------*/

APIRET Free_Buffer(PVOID pBufAddr)
{                                                    
APIRET  rc;                                          
rc=DosFreeMem(pBufAddr);
if(rc==0)
   {
   ucNumMemoryObjects--;
   }
return(rc); 
}           

/*---------------------------------------------------------------------------*/
/* Function: Display_Msg()                                                   */
/* Ausgabe einer Messagebox, abhÑngig von Åbergebenem Code                   */
/*---------------------------------------------------------------------------*/

void Display_Msg(USHORT usCode,HWND hHandle,CHAR* pchText)
{
APIRET rc;
CHAR code[5];
sprintf(code,"%x",usCode);
rc=WinMessageBox(HWND_DESKTOP,
                 hHandle,               /* Dialog Handle                     */
                 pchText,
                 code,                  /* Titel ist errorcode               */
                 0,
                 MB_OK          |
                 MB_INFORMATION |
                 MB_CUANOTIFICATION);
}

/*---------------------------------------------------------------------------*/
/* Function: GetLatestCapturedMidiEvents()                                   */
/*---------------------------------------------------------------------------*/
/* This function is called during recording when RECORD timer events occur.  */
/* The function reads all COMPLETE Midi events which have been received from */
/* the MPU by the interrupt handler since the last call of this function.    */
/* The function uses usCaptRd and usCaptWr in combination with the functions */
/* AnalyzeCapturedMidiEvent() and AppendMidiEvent() to copy the MIDI Events  */
/* from the Capture Buffer to the Working Buffer in final format.            */
/* Extents are automatically allocated by AppendMidiEvent().                 */
/* Parameters: NONE                                                          */
/*---------------------------------------------------------------------------*/

APIRET GetLatestCapturedMidiEvents(UCHAR ucTrkNum)
{
APIRET rc=0;
APIRET rc2;

UCHAR  ucTiming;
UCHAR  ucStatus;
UCHAR  ucValue1;
UCHAR  ucValue2;
UCHAR  ucOffsetIncrement;

/* start a loop, which calls AnalyzeCapturedMidiEvent(). This Loop runs,     */
/* until 1. Analyze found no more complete event                             */
/*       2. The last complete event ended with the last copied byte          */
/*          pointed to by usCaptWr, so no new event will follow, even if     */
/*          rc from analyze was 0.                                           */
/* for   1. usCaptRd remains in the state BEFORE the last call.              */
/*       2. usCaptRd is set to new position, but NO further call to Analyze  */
/*          this case is not recognized by Analyze, and GetLatestCaptured... */
/*          must compare usCaptRd and usCaptWr to recognize this. It leaves  */
/*          the loop by force in this case.                                  */
 
ucTurnaroundInd=0;       /* just for test          */


do {                                    /* Loop                              */
   ucTiming=0;                          /* initialize values to 0            */
   ucStatus=0;
   ucValue1=0;
   ucValue2=0;
   ucOffsetIncrement=0;

   rc=AnalyzeCapturedMidiEvent(&ucTiming,         /* Call Analzye            */
                               &ucStatus,         /* to get the MIDI Data    */
                               &ucValue1,
                               &ucValue2,
                               &ucOffsetIncrement);
   if(rc==0)                            /* if COMPLETE MIDI event found, add */
      {                                 /* this event to the working buffer  */
      usCaptRd+=ucOffsetIncrement;      /* set new usCaptRd                  */
      if(usCaptRd>=CAPTURE_SIZE)        /* correction for turnaround  !!!    */
         usCaptRd=usCaptRd-CAPTURE_SIZE+2;


/*T*/ if(ucTurnaroundInd==1)            /* output, if Analyze found turnar.  */
/*T*/    {                                                                     
/*T*/    strcpy(DlgText,"Capture Turnaround in Analyze");
/*T*/    WriteListBox(hWndMainDialog,
/*T*/                 DID_MD_LISTBOX,
/*T*/                 DlgText);      
/*T*/    }                                                                     

      rc2=AppendMidiEvent(ucTrkNum,     /* Tracknumber to append to          */
                          ucTiming,     /* Data to append to WrkBuffer       */
                          ucStatus,      
                          ucValue1,      
                          ucValue2);     
      if(rc2==5)
         {
         Display_Msg((USHORT)rc2,hWndMainClient,
                     "AppendMidiEvent(): Extent Alloc Failed");
         return(8);
         }
      if(usCaptRd==usCaptWr)            /* test for LAST complete event      */
         break;                         /* force exit out of loop (see above)*/
      }
   }while(rc==0);

if((rc!=10)&&(rc!=0))                   /* only rc=10 or rc=0 are o.k.       */
   {
   Display_Msg((USHORT)rc,hWndMainClient,"Fehler bei Aufzeichnung");
   }
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: AppendMidiEvent()                                               */
/*---------------------------------------------------------------------------*/
/* This function appends the MIDI Data to the Working Buffer Extents for the */
/* specified track. If no more extents are free, the function dynamically    */
/* allocates space.                                                          */
/* Timing values are accumulated. If F8 is found, this means, that the MPU   */
/* had a Timing overflow, i.e. only 240 are added to the timing value of the */
/* currently built Midi Event.                                               */
/*---------------------------------------------------------------------------*/

APIRET AppendMidiEvent(UCHAR ucTrackNum,
                       UCHAR ucTiming,
                       UCHAR ucStatus,
                       UCHAR ucValue1,
                       UCHAR ucValue2)
{
USHORT usActExtIdx;                     /* some help variables               */
USHORT usActEvtIdx;
PTRKDATA pTrkExt;
APIRET rc;

extern ULONG ulAccuTiming;              /* accumulated Timing values         */

ULONG  ulMeasure=0;                     /* for test of MIDI timing output    */
ULONG  ulTaktmass=0;
ULONG  ulTicks=0;   
ULONG  ulMidiTiming=0;                  /* MIDI Timing LONG format           */
ULONG  ulRecalcTiming=0;                /* for test of recalc.               */

/*---------------------------------------------------------------------------*/
/* Possible combinations  :                                                  */
/*                               pTrkExt            usActEvtIdx              */
/*                                                                           */
/* NO EXTENT ALLOCATED    :       NULL               don't care              */
/*                                                                           */
/* EXTENT HAS FREE EVENTS :       not NULL           0-408                   */
/*                                                                           */
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
   usActEvtIdx=0;                       /* After creation: Event Idx = 0     */
 
/*T*/ usActExtIdx=WrkBufMap[ucTrackNum].usActExtentIdx;
/*T*/ sprintf(DlgText,"Workbufextent angelegt, addr=%8x",
/*T*/                  WrkBufMap[ucTrackNum].pTrkExtent[usActExtIdx]);
/*T*/ WriteListBox(hWndMainDialog, 
/*T*/              DID_MD_LISTBOX,
/*T*/              DlgText);      
   }

usActExtIdx=WrkBufMap[ucTrackNum].usActExtentIdx;      /* refresh index      */
pTrkExt=WrkBufMap[ucTrackNum].pTrkExtent[usActExtIdx]; /* refresh address    */


/* Now copy the MIDI event to the working buffer at the event pointed to     */
/* by the indices.                                                           */
/* 1.Timingbyte: If 0xF8 is contained, this is a timing overflow, so only    */
/*               240 need to be added to the previous ulTiming.              */
if(ucTiming==0xF8)                      /* timing overflow message           */
   {
   ulAccuTiming+=240;
   (pTrkExt+usActEvtIdx)->ulTiming=ulAccuTiming;
   }
else                                    /* normal midi message               */
   {
   ulAccuTiming+=ucTiming;                             /* ADD !!!            */
   (pTrkExt+usActEvtIdx)->ulTiming=ulAccuTiming;
   (pTrkExt+usActEvtIdx)->ucMidiStatus=ucStatus;
   (pTrkExt+usActEvtIdx)->ucData1=ucValue1;
   (pTrkExt+usActEvtIdx)->ucData2=ucValue2;

   /* Testoutput of ulTiming in MIDI Format to Listbox                       */
   /* first convert ulTiming to MIDI Timing in ULONG format (ulMidiTiming)   */
   /* then run extract and output to listbox. Also test reconversion to abs. */

   WrkBufMap[ucTrackNum].ulNumMidiEvents+=1;      /* INC MIDI event count    */

/* AbsTime2MidiTime(ulAccuTiming,
                    ucTaktmassZaehler, 
                    ucTaktmassNenner, 
                    ucTicksPerQuarter,
                    &ulMidiTiming);    
   ExtractMidiTiming(ulMidiTiming,     
                     &ulMeasure,       
                     &ulTaktmass,      
                     &ulTicks);        
   MidiTime2AbsTime(ulMidiTiming,      
                    ucTaktmassZaehler, 
                    ucTaktmassNenner,  
                    ucTicksPerQuarter, 
                    &ulRecalcTiming);  
   sprintf(DlgText,"Abs: %5x MIDI: %4x|%1x|%3x Neu: %5x",
                    ulAccuTiming,
                    ulMeasure,
                    ulTaktmass,
                    ulTicks,
                    ulRecalcTiming);
   WriteListBox(hWndMainDialog,
                DID_MD_LISTBOX,
                DlgText);                                        */

   /**************************************************************************/
   /* should pointer chain also be built here ?                              */
   /**************************************************************************/

   WrkBufMap[ucTrackNum].usActEventIdx++;    /* Event Index to next event??? */
   }                                         /* end ELSE normal midi msg     */
}


/*---------------------------------------------------------------------------*/
/* Function: AnalyzeCapturedMidiEvent()                                      */
/*---------------------------------------------------------------------------*/
/* Analyzes ONE Midi Event!!                                                 */
/* Reads data in MPU format from the Capture buffer beginning at the address */
/* indicated by pStart. pStart contains the address of the first byte to be  */
/* copied. This is offset at usCaptRd. ucMaxBytes indicates the limit of     */
/* bytes to analyze. It is calculated with use of usCaptWr, the offset of    */
/* the last byte copied by the interrupt handler which is valid.             */
/* If the MIDI event cannot COMPLETELY!!! be analyzed, because usCaptWr is   */
/* reached BEFORE the event is comlete, a bad return code is returned.       */
/* This event will be analyzed again at next timer processing.               */
/* öberlegung: eine spezielle record track aufbauen????                      */
/*             erst nach der aufnahme in eine Track einkopieren?             */
/*             Vorteil: Vereinfachte aufnahme ohne allocations...            */

/* Parameters:   noch ausfÅllen                                              */
/* Return:     RC,  0 if ok.                           o.k.                  */
/*                  1 if TDR in data                   E R R O R             */
/*                  2 if unknown Data                  E R R O R             */
/*                 10 if incomlete MIDI event          o.k.                  */
/* Results:    The variables pointed to when calling are filled with found   */
/*             data: Timing, Status, Value1, Value2, length of MIDI data.    */
/*             Length is overall length incl. MPU timing value.              */
/* Length is used by the calling function to determine the offset usCaptRd   */
/* at which the next Buffer Entry is located.                                */
/* If running status is found, the MIDI Status Byte is restored and set as   */
/* return Value. So the result will contain always FULL MIDI messages.       */
/* This is, because when merging multiple tracks (0-63), running status must */
/* be calculated at merging time.                                            */
/* Unused Bytes are set to 0.  (Vorerst nicht, u.U. nur performanceverlust   */
/*---------------------------------------------------------------------------*/

/* feststellen, ob event komplett: wenn RC=0: auf jeden fall.                */
/* Wie kann Åberlauf festgestellt werden? einfach am Ende der schleife:      */
/* wenn bedingung nicht mehr erfÅllt: programm lÑuft weiter, setzen rc=10    */
/* damit Åberlauf erkannt, unterschieden von Fehler returncodes              */


APIRET AnalyzeCapturedMidiEvent(UCHAR  *pTiming,            /* return-value  */
                                UCHAR  *pStatus,            /* return-value  */
                                UCHAR  *pValue1,            /* return-value  */
                                UCHAR  *pValue2,            /* return-value  */
                                UCHAR  *pOffsetIncrement)   /* return-value  */
{
UCHAR ucMode=MA_NEW;
UCHAR *pWork;                           /* Work Pointer                      */
USHORT usCaptRdHelp;                    /* Working Variable                  */
APIRET rc=0;

ucTurnaroundInd=0;                      /* for test    init indicator to 0   */

usCaptRdHelp=usCaptRd;                  /* Initial Value to Work Read Offset */
do                                      /* 'ENDLESS' LOOP, which runs until  */
   {                                    /* usCaptWr is reached.              */
   usCaptRdHelp++;                      /* Set Start Address for Analyze     */
   if(usCaptRdHelp==CAPTURE_SIZE)       /* with respect to possible turn-    */
      {                                 /* around.                           */ 
      usCaptRdHelp=2;
      ucTurnaroundInd=1;
      }
   pWork=pCaptBuf+usCaptRdHelp;         /* usCaptRd remains unchanged        */

switch(ucMode)
   {
   case MA_NEW:                         /* if this is the first byte...      */
   if(*pWork<0xF0)                      /* if TimingByte..                   */
      {
      *pTiming=*pWork;                  /* store the TIMING Value            */
      ucMode=MA_TIMING;
      }
   else if(*pWork<=0xF7)                /* if F0..F7: Trk Data Request       */
      {                                 /* should not occur                  */
      return(1);
      }
   else if(*pWork==0xF8)                /* Timing overflow Byte from MPU     */
      {                                 /* Timing overflow is sent without   */
      *pTiming=*pWork;                  /* Timing byte from MPU, so it re-   */
      *pOffsetIncrement=1;              /* places the timing byte in buffer  */
      return(0);
      }
   else                                 /* should not reach this point       */
      {
      return(2);
      }
   break;

   /* if previous byte was Timing Byte from MPU: analyze status byte...      */
   case MA_TIMING:                      /* if previous byte was timing byte  */
   if(*pWork<0x80)                      /* if < 0x80: Assume running status  */
      {
      *pStatus=ucRecRunStat;            /* restore from last runningstatus   */
      *pOffsetIncrement=ucRecRunStatLength-1;
      if(ucRecRunStatLength>2) *pValue1=*pWork;   /* first databyte          */
 
      usCaptRdHelp++;                   /* second Databyte...                */
      if(usCaptRdHelp==CAPTURE_SIZE)    /* Turnaround                        */
         {                 
         usCaptRdHelp=2;   
         ucTurnaroundInd=1;
         }                 
      if(usCaptRdHelp==usCaptWr) return(10);           /* does not fit       */
      pWork=pCaptBuf+usCaptRdHelp;                     /* new address        */
      if(ucRecRunStatLength>3) *pValue2=*(pWork);

      return(0);
      }
   else if(*pWork<=0xBF)                /* 4 Bytes incl. Timingbyte          */
      {
      *pStatus=*pWork;                  /* Store Statusbyte                  */
      /* save running status information for next event analyze              */
      ucRecRunStatLength=4;             /* Save length of MIDI Message       */
      ucRecRunStat=*pWork;              /* save as running Status            */

      usCaptRdHelp++;                                                          
      if(usCaptRdHelp==CAPTURE_SIZE)    /* Turnaround                        */
         {                 
         usCaptRdHelp=2;   
         ucTurnaroundInd=1;
         }                 
      if(usCaptRdHelp==usCaptWr) return(10);           /* does not fit       */
      pWork=pCaptBuf+usCaptRdHelp;                     /* new address        */

      *pValue1=*(pWork);                /* Store 1st databyte                */

      usCaptRdHelp++;                                                          
      if(usCaptRdHelp==CAPTURE_SIZE)    /* Turnaround                        */
         {                 
         usCaptRdHelp=2;   
         ucTurnaroundInd=1;
         }                 
      if(usCaptRdHelp==usCaptWr) return(10);           /* does not fit       */
      pWork=pCaptBuf+usCaptRdHelp;                     /* new address        */

      *pValue2=*(pWork);                /* Store 2nd databyte                */
      *pOffsetIncrement=4;              /* 4 Bytes length                    */

      return(0);
      }
   else if(*pWork<=0xDF)                /* 3 Bytes incl. Timingbyte          */
      {                                                                        
      *pStatus=*pWork;                  /* Store Statusbyte                  */
      /* save running status information for next event analyze              */
      ucRecRunStat=*pWork;              /* save as running Status            */
      ucRecRunStatLength=3;             /* Save length of MIDI Message       */

      usCaptRdHelp++;                                                          
      if(usCaptRdHelp==CAPTURE_SIZE)    /* Turnaround                        */
         {                 
         usCaptRdHelp=2;   
         ucTurnaroundInd=1;
         }                 
      if(usCaptRdHelp==usCaptWr) return(10);      /* incomplete midi event   */
      pWork=pCaptBuf+usCaptRdHelp;                /* new address             */

      *pValue1=*(pWork);                /* Store 1st databyte                */
      *pOffsetIncrement=3;

      return(0);
      }  
   else if(*pWork<=0xEF)                /* 4 Bytes incl. Timingbyte          */
      {                                                                        
      *pStatus=*pWork;                  /* Store Statusbyte                  */
      /* save running status information for next event analyze              */
      ucRecRunStat=*pWork;              /* save as running Status            */
      ucRecRunStatLength=4;             /* Save length of MIDI Message       */

      usCaptRdHelp++;                                                          
      if(usCaptRdHelp==CAPTURE_SIZE)    /* Turnaround                        */
         {
         usCaptRdHelp=2;   
         ucTurnaroundInd=1;
         }                 
      if(usCaptRdHelp==usCaptWr) return(10);      /* incomplete midi event   */
      pWork=pCaptBuf+usCaptRdHelp;                /* new address             */ 

      *pValue1=*(pWork);                /* Store 1st databyte                */

      usCaptRdHelp++;                                                          
      if(usCaptRdHelp==CAPTURE_SIZE)    /* Turnaround                        */
         {                 
         usCaptRdHelp=2;   
         ucTurnaroundInd=1;
         }                 
      if(usCaptRdHelp==usCaptWr) return(10);      /* incomplete midi event   */
      pWork=pCaptBuf+usCaptRdHelp;                /* new address             */

      *pValue2=*(pWork);                /* Store 2nd databyte                */
      *pOffsetIncrement=4;

      return(0);
      }                                                                        
   else                                 /* only F9 and FC are valid          */
      {
      *pStatus=*pWork;                  /* Store Statusbyte                  */
      /* save running status information for next event analyze              */ 
      ucRecRunStat=*pWork;              /* save as running Status            */
      ucRecRunStatLength=2;             /* Save length of MIDI Message       */
      *pOffsetIncrement=2;

      return(0);
      }
   break;

   default:
   break;          
   }                                    /* end of switch UCMODE              */

   }while(usCaptRdHelp!=usCaptWr);      /* End of DO Loop if End reached     */
rc=10;                                  /* incomplete MIDI event             */

return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: InitWrkBufMap()                                                 */
/*---------------------------------------------------------------------------*/
/* This function initializes the administration structure for the working    */
/* buffer. It is called at startup of the program.                           */
/* Passed parameters: NONE                                                   */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET InitWrkBufMap(void)
{                                                                              
INT i,k;                                /* i: Trackindex  k: extentindex     */
for(i=0;i<SEQ_TRACKS;i++)               /* do for all TRACKS                 */
   {
   for(k=0;k<256;k++)                   /* do for all EXTENTS per TRACK      */
      {
      WrkBufMap[i].pTrkExtent[k]=NULL;  /* Extentpointer Array               */
      }                                 /* NULL means Extent is free         */
   WrkBufMap[i].ulNumMidiEvents=0;      /* Number of MIDI Events             */
   WrkBufMap[i].ulMaxTiming=0;          /* Maximum Timing value (FC End)     */
   WrkBufMap[i].usNextExtentToAllocate=0;    /* First extent to allocate     */
   WrkBufMap[i].usActExtentIdx=0;       /* Actual Extent Index               */
   WrkBufMap[i].usActEventIdx=0;        /* Actual Event Index                */
   strcpy(WrkBufMap[i].chTrkName,"");   /* Name of Track                     */
   strcpy(WrkBufMap[i].chInstrument,"");/* Name of Instrument                */
   WrkBufMap[i].ucMidiChannel=0;        /* Storage for Midi Channel Number   */
   WrkBufMap[i].ucTrkPlayStatus=0;      /* Playstatus inactive for track     */
   WrkBufMap[i].ucPriority=0;           /* Priority default = 0              */
   }
return(0);
}

/*---------------------------------------------------------------------------*/
/* Function: CreateWrkBufExtent()                                            */
/*---------------------------------------------------------------------------*/
/* Diese Funktion erzeugt einen Working Buffer Extent in der Grî·e 4KByte,   */
/* registriert diesen in der WrkBufMap-Struktur und initialisiert dessen     */
/* Inhalt. Danach ist dieser Extent benutzbar und bietet Platz fÅr 409       */
/* MIDI Events.                                                              */
/* öbergabeparameter: Platzhalter fÅr Adresse des neuen Extents              */
/*                    Tracknummer, fÅr die ein Extent angefordert wird (0-63)*/
/* Return values:     0 = O.K.                                               */
/*                    1 = maximum of 256 extents for this track reached      */
/*                    2 = invalid Track Number requested                     */
/*                    3 = allocate buffer failed                             */
/* IMPORTANT: Entry size of 10 Bytes assumed!!!!                             */
/*---------------------------------------------------------------------------*/

UCHAR CreateWrkBufExtent(UCHAR ucTrkNum)          /* Track-Number            */

{
PCHAR pchBuffer;                        /* returned Bufferaddress            */
PTRKDATA ptrkWork;                      /* Working-Pointer                   */
APIRET rc;
USHORT usIdx;                           /* Index of Extent to create         */
USHORT i;                               /* Help-variable                     */

usIdx=WrkBufMap[ucTrkNum].usNextExtentToAllocate; /* Idx of extent to alloc  */

if((ucTrkNum<0)||(ucTrkNum>=SEQ_TRACKS))          /* invalid Tracknumber ?   */
   return(2);

if(usIdx>255)                                     /* Extent overflow ?       */
   return(1);

rc=Alloc_Buffer(4096,&pchBuffer);       /* Allocate 1 Page for the extent    */
if(rc!=0)
   return(3);

/* Store the needed data to the administration structure                     */
ptrkWork=(PTRKDATA)pchBuffer;
WrkBufMap[ucTrkNum].pTrkExtent[usIdx]=ptrkWork;   /* store Extent Address    */
WrkBufMap[ucTrkNum].usActExtentIdx=usIdx;         /* set to new Extent Index */
WrkBufMap[ucTrkNum].usActEventIdx=0;              /* init to first Event     */
WrkBufMap[ucTrkNum].usNextExtentToAllocate++;     /* Next extent allocation  */

for(i=0;i<409;i++)                      /* init all 409 events in the extent */
   {
   (ptrkWork+i)->ulTiming=0;            /* accumulated timing = 0            */
   (ptrkWork+i)->usNextEventIdx=i+1;    /* following event in chain (linear) */
   (ptrkWork+i)->ucNextExtentIdx=usIdx; /* ext in which next event is found  */
   (ptrkWork+i)->ucMidiStatus=0;
   (ptrkWork+i)->ucData1=0;
   (ptrkWork+i)->ucData2=0;
   }
(ptrkWork+408)->usNextEventIdx=0;       /* follow on of last event in extent */
(ptrkWork+408)->ucNextExtentIdx=usIdx+1;/* is FIRST event in NEXT extent     */

return(0);
}
                         
/*---------------------------------------------------------------------------*/
/* Function: DeleteAllTrackExtents()                                         */
/*---------------------------------------------------------------------------*/
/* This function deletes ALL Working Buffer Extents for a specified track.   */
/* All pointers pTrkExtent are deleted and reset to NULL. All contents are   */
/* lost.                                                                     */
/* Parameters:  UCHAR    Track, from which all extents are to be deleted     */
/*---------------------------------------------------------------------------*/
 
APIRET DeleteAllTrackExtents(UCHAR ucTrkNum)
{
USHORT i;                               /* Loop variable                     */
APIRET rc=0;
for(i=0;i<256;i++)                      /* search all extents for valid addr */
   {
   if(WrkBufMap[ucTrkNum].pTrkExtent[i]!=NULL)    /* if extent is allocated  */
      {
      rc=DeleteSpecificTrackExtent(ucTrkNum,i);   /* delete specific extent  */
      if(rc>0)                                                
         Display_Msg((USHORT)rc,hWndMainClient,              
                     "DeleteAllTrackExtents(): Ext Del Fail");
      }
   }
return(rc);
}

                                                  
/*---------------------------------------------------------------------------*/
/* Function: DeleteSpecificTrackExtent()                                     */
/*---------------------------------------------------------------------------*/
/* This function deletes the specific Working buffer extent for the track    */
/* indicated by ucTrackNum. ucExtentIdx is the index to the Extent to be     */
/* deleted. All data in this extent will be lost.                            */
/* Parameters: UCHAR     Track, for which an extent is to be deleted         */
/*             USHORT    Index of extent to be deleted                       */
/*---------------------------------------------------------------------------*/

APIRET DeleteSpecificTrackExtent(UCHAR ucTrkNum,USHORT usExtentIdx)
{
APIRET rc;
rc=Free_Buffer((PVOID)(WrkBufMap[ucTrkNum].pTrkExtent[usExtentIdx]));
if(rc==0)
   {
   WrkBufMap[ucTrkNum].pTrkExtent[usExtentIdx]=NULL;   /* reset address      */
   WrkBufMap[ucTrkNum].ulNumMidiEvents=0;              /* reset midi events  */
   WrkBufMap[ucTrkNum].ulMaxTiming=0;                  /* reset max timing   */
   WrkBufMap[ucTrkNum].usActEventIdx=0;                /* reset event idx    */
   WrkBufMap[ucTrkNum].usActExtentIdx=0;               /* reset extent idx   */
   WrkBufMap[ucTrkNum].usNextExtentToAllocate=0;       /* reset nxtextent    */
   WrkBufMap[ucTrkNum].ucTrkPlayStatus=0;              /* reset Playstatus   */
   WrkBufMap[ucTrkNum].ucPriority=0;                   /* reset Trk Priority */
   strcpy(WrkBufMap[ucTrkNum].chTrkName,"");           /* reset Trackname    */
   strcpy(WrkBufMap[ucTrkNum].chInstrument,"");        /* reset Instrument   */
   WrkBufMap[ucTrkNum].ucMidiChannel=0;                /* reset MIDI channel */
   }
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: FillPlayBuffer()                                                */
/*---------------------------------------------------------------------------*/
/* This function is called when recording starts once for an initial filling.*/
/* From then on it is called at PLAY Timer events. The function fills        */
/* the Playback Buffer (pPlayBuf) with MIDI events from the Working Buffer.  */
/* The function uses BuildMPUPlayData() to copy MIDI events to the playback  */
/* buffer. BuildMPUPlayData() returns rc=0, if it was able to copy a         */
/* complete MIDI Message to the buffer in MPU Format.                        */
/* Then the Loop will run again, except for the case, that the last byte of  */
/* the MIDI event has been written to the last byte which was invalid.       */
/* RC=10 means, thats complete, nothing will be copied and usPlayWr will     */
/* not be changed.                                                           */
/*                                                                           */
/* Testphase: only track 0 is used, so no track masks will be used.          */
/*                                         LATER: multiple tracks with       */
/*                                                track merger function      */
/* in this function also the merger will be realized. THIS FUNCTION ONLY HAS */
/* TO DETERMINE THE NEXT MIDI EVENT TO BE SENT FROM ALL ACTIVE TRACKS BY     */
/* USING MERGE AND TO CALL BUILD WITH THIS DATA. THEN IT MUST BE ABLE TO     */
/* DETERMINE IF ANOTHER CALL TO BUILD IS POSSIBLE.                           */
/* possible parms: Active track mask                                         */
/*---------------------------------------------------------------------------*/


/* Frage: was geschieht bei Timerevents, die kommen, nachdem das FC schon    */
/* kopiert worden ist, aber PLAY noch den buffer abarbeitet?                 */


APIRET FillPlayBuffer(void)
{
USHORT usActExtIdx;                     /* Working index for fetching data   */
USHORT usActEvtIdx;                     /* from Working buffer               */
PTRKDATA pTrkExt;
APIRET rc;
UCHAR  ucTrkNum;    /* TEST */          /* Tracknumber, test always 0        */
UCHAR  ucOffsetIncrement;
UCHAR  i,k;                             /* Help loop variable                */

ULONG  ulTiming;                        /* Parameters for BuildMPUPlayData   */
UCHAR  ucTiming;     /* kann gelîscht werden ?? */
UCHAR  ucStatus;
UCHAR  ucValue1;
UCHAR  ucValue2;

ULONG  ulTimingWrk;                     /* Workingvariable for track merge   */
ulTimingWrk=0xFFFFFFFF;                 /* set to maximum                    */

do {                                    /* Loop                              */
   /* At this point: Find the next track from which data should be copied    */
   /* this is the MERGER function, which decides by help of all timer values */
   /* Check current event of all tracks contained in dispatch list. Choose   */
   /* the Track with the smallest timing value as next event to copy.        */
 
   ulTimingWrk=0xFFFFFFFF;              /* set to maximum for init...        */

   for(k=1;k<=ucDispatchList[0];k++)    /* if Track active for dispatching.. */
      {
      i=ucDispatchList[k];              /* i = Track index                   */
      usActExtIdx=WrkBufMap[i].usActExtentIdx;              /* TEMP          */
      usActEvtIdx=WrkBufMap[i].usActEventIdx;               /* TEMP          */
      pTrkExt=WrkBufMap[i].pTrkExtent[usActExtIdx];         /* TEMP          */
      if(((pTrkExt+usActEvtIdx)->ulTiming)<=ulTimingWrk)
         {
         ulTimingWrk=((pTrkExt+usActEvtIdx)->ulTiming);     /* save for comp */
         ucTrkNum=i;
         }
      }
   /* After this loop: ucTrkNum contains the tracknumber containing the      */
   /* event with the lowest absolute timing-value                            */

   usActExtIdx=WrkBufMap[ucTrkNum].usActExtentIdx; /* Get current index for  */
   usActEvtIdx=WrkBufMap[ucTrkNum].usActEventIdx;  /* this track.            */
 
   pTrkExt=WrkBufMap[ucTrkNum].pTrkExtent[usActExtIdx];  /* get extentaddr.  */
   ulTiming=(pTrkExt+usActEvtIdx)->ulTiming;             /* accu abs timing  */
   ucStatus=(pTrkExt+usActEvtIdx)->ucMidiStatus;         /* get MIDI status  */
   ucValue1=(pTrkExt+usActEvtIdx)->ucData1;              /* get Data 1       */
   ucValue2=(pTrkExt+usActEvtIdx)->ucData2;              /* get Data 2       */

   /* Now check, if code is FC End Stop code: If NO: copy to the playbuffer  */
   /* using BuildMPUPlayData(). If YES: Check, if this track is last active. */
   /* Check for Last trk is done by using counter in DispatchList.           */
   /* If NO: do not copy this event to playbuffer and deactivate the track.  */
   /*        by deleting it from the Dispatcher List.                        */
   /* if YES: copy FC End to playbuffer. This will end playback.             */
   /* FC End is copied instead of Track Number. This makes it independent    */
   /* from the currently active playback tracks.                             */

   if((ucStatus==0xFC)&&(ucDispatchList[0]>1)) /* if FC end but not last trk */
      {
      DosBeep(1000,50);
      sprintf(DlgText,"Dispatch %2d %2d %2d %2d %2d %2d %2d %2d",
                       ucDispatchList[1],
                       ucDispatchList[2],
                       ucDispatchList[3],
                       ucDispatchList[4],
                       ucDispatchList[5],
                       ucDispatchList[6],
                       ucDispatchList[7],
                       ucDispatchList[8]);
      WriteListBox(hWndMainDialog, 
                   DID_MD_LISTBOX, 
                   DlgText);       
      sprintf(DlgText,"FC : ACTIVE: %2d TRK: %2d",
                       ucDispatchList[0],
                       ucTrkNum);
      WriteListBox(hWndMainDialog,                       
                   DID_MD_LISTBOX,                       
                   DlgText);                             

      UpdateDispatchList(ucTrkNum);          /* delete from dispatch list    */

      sprintf(DlgText,"Dispatch %2d %2d %2d %2d %2d %2d %2d %2d", 
                       ucDispatchList[1],                         
                       ucDispatchList[2],                         
                       ucDispatchList[3],                         
                       ucDispatchList[4],                         
                       ucDispatchList[5],                         
                       ucDispatchList[6],                         
                       ucDispatchList[7],                         
                       ucDispatchList[8]);                        
      WriteListBox(hWndMainDialog,                                
                   DID_MD_LISTBOX,                                
                   DlgText);                                      
      sprintf(DlgText,"FC : ACTIVE: %2d TRK: %2d",                
                       ucDispatchList[0],                         
                       ucTrkNum);                                 
      rc=0;                                  /* prevent from leaving loop    */
      }

   else                                 /* no FC-End OR FC-End in track,     */
      {                                 /* which is the last active one so   */
      if(ucStatus==0xFC)                /* FC End needs to be copied         */
         {
         DosBeep(500,50);

      sprintf(DlgText,"Dispatch %2d %2d %2d %2d %2d %2d %2d %2d",
                       ucDispatchList[1],                        
                       ucDispatchList[2],                        
                       ucDispatchList[3],                        
                       ucDispatchList[4],                        
                       ucDispatchList[5],                        
                       ucDispatchList[6],                        
                       ucDispatchList[7],                        
                       ucDispatchList[8]);                       
      WriteListBox(hWndMainDialog,                               
                   DID_MD_LISTBOX,                               
                   DlgText);                                     

         sprintf(DlgText,"FC : ACTIVE: %2d TRK: %2d Tim: %8x",
                          ucDispatchList[0],
                          ucTrkNum,
                          ulTiming);
         WriteListBox(hWndMainDialog,                
                      DID_MD_LISTBOX,                
                      DlgText);                 
 
         ucTrkNum=0xFC;                 /* STOP Code to track number         */
         }

      rc=BuildMPUPlayData(ucTrkNum,     /* NEU: Timing ist nun ABSOLUT !!    */
                          ulTiming,     /* Call build function to add these  */
                          ucStatus,     /* data to the play buffer in MPU    */
                          ucValue1,     /* playable format.                  */
                          ucValue2,
                          &ucOffsetIncrement);  /* for finding next offset   */

      if(rc==0)                         /* if COMPLETE MIDI event copied...  */
         {
         usPlayWr+=8;                   /* set next offset to write at       */
         if(usPlayWr==PLAY_SIZE)        /* correction for turnaround  !!!    */
            {
            usPlayWr=8;                 /* 8 Bytes for feedback  NEU         */
            sprintf(DlgText,"Playback Turnaround in Fill: %5d",
                             usPlayWr);
            WriteListBox(hWndMainDialog,
                         DID_MD_LISTBOX,
                         DlgText);      
            }
         WrkBufMap[ucTrkNum].usActExtentIdx=         /* Build chain pointer  */
            (pTrkExt+usActEvtIdx)->ucNextExtentIdx;  /* for NEXT event to    */
         WrkBufMap[ucTrkNum].usActEventIdx=          /* read in current trk  */
            (pTrkExt+usActEvtIdx)->usNextEventIdx;
         }                              /* end if rc==0                      */
      }                                 /* end else                          */

      if(usPlayWr==usPlayRd)            /* test for LAST complete event      */
         {                              /* if equal: no more call needed,    */
                                        /* because it will fail with rc10    */
         break;                         /* force exit out of loop (see above)*/
         }                                                                     

   }while(rc==0);           

/* if RC was not 0: old ActExtent and ActEvent indices remain stored in      */
/* WrkBufMap, and are read at next timer event again                         */

/* store index values for NEXT read back to Working buffer, so that they can */
/* be restored at next call to FillPlayBuffer at PLAY timer event.           */
/* WrkBufMap[ucTrkNum].usActExtentIdx=usActExtIdx;   only track 0 for test   */
/* WrkBufMap[ucTrkNum].usActEventIdx=usActEvtIdx;      ?????                 */
                             
if((rc!=10)&&(rc!=0)&&(rc!=11))         /* only rc=10,11 or 0 are o.k.       */
   {                                                                           
   Display_Msg((USHORT)rc,hWndMainClient,"FillPlayBuffer() Fehler");
   }                                                                           

return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: BuildMPUPlayData()                                              */
/*---------------------------------------------------------------------------*/
/* This function builds an entry for the MPU Playbuffer from the Data passed */
/* from the Working Buffer.                                                  */
/* Timing value is ABSOLUTE Timing in ULONG format                           */
/* NEU Playbuffer Entry Format: FIXED size of 8 Bytes                        */
/*                                                                           */
/* +--------+--------+--------+--------+--------+--------+--------+--------+ */
/* | TRACK- |        :        :        :        | MIDI   | MIDI   | MIDI   | */
/* | NUMBER | ABSOLUTE TIMING in 32 bit Format  | STATUS | DATA   | DATA   | */
/* | 0 - 63 |        :        :        :        | BYTE   | BYTE 1 | BYTE 2 | */
/* +--------+--------+--------+--------+--------+--------+--------+--------+ */
/*                                                                           */
/* It copies these bytes to the Playbackbuffer at the offset usPlayWr, which */
/* indicates the next offset, where this function should write data.         */
/* The function begins the filling at usPlayWr and fills, until usPlayRd-1   */
/* is reached or until FC End mark has been copied. (as far as possible)     */
/* If a MIDI Event is complete and the last Byte has been copied to          */
/* usPlayRd-1, it also stops.                                                */
/* Only COMPLETE MIDI events may be copied. If overflow with usPlayRd occurs */
/* before an event is complete, nothing will be copied and usPlayWr will     */
/* not be changed.                                                           */

/* NEU Running Status is not sent to the playbuffer, it is generated by the  */
/* driver, online while sending data to the MPU by comparing to last status  */
/* sent.                                                                     */
/* this function must analyze the data sent, to determine, how much MIDI     */
/* bytes should be copied.                                                   */
/* rc=10 is set, when event does not match to the rest of free buffer,       */
/* rc=11 is set, when an FC end mark has been copied, i.e. buffer complete.  */

/*---------------------------------------------------------------------------*/ 

APIRET BuildMPUPlayData(UCHAR ucTrkNum, /* Track, this event belongs to      */
                        ULONG ulTiming, /* accumulated timingvalue           */
                        UCHAR ucStatus,
                        UCHAR ucValue1,
                        UCHAR ucValue2,
                        UCHAR *pOffsetIncrement)  /* return value (NEU: 8)   */
{

APIRET rc=0;
USHORT usPlayWrHelp;                    /* Working Variable                  */
USHORT usNumTimOverflow;                /* Number of timing overflow F8      */
UCHAR  i;                               /* help variable for loops           */

usPlayWrHelp=usPlayWr;                  /* initialize to startoffset         */
(*pOffsetIncrement)=0;                  /* initialize Offsetincrement        */

ucTurnaroundInd=0;                      /* for test    init indicator to 0   */

/* loop: exit loop from within at completion. Equal pointer end is only      */
/* reached when event does not match to remaining buffer space. Then rc is   */
/* set to 10.                                                                */
/* also FC end is checked within the loop, if found: exit with rc=11         */

/*  NEU: F8 wird nicht mehr geschickt, statt dessen das gesamte ACCU Timing  */

/* Now a normal midi event is to be sent, first copy the Tracknumber of this */
/* event to the Playbuffer BEFORE the Timing Data.                           */
/* NEU: Da eine feste LÑnge der Daten vorliegt, nÑmlich 8 Bytes, kann schon  */
/* VORHER festgestellt werden, ob die Daten Åberhaupt passen.                */
/* Turnaround check nicht nîtig, da in FillPlayBuffer() das Setzen auf den   */
/* nÑchsten offset erfolgt, und dort korrigiert wird. Aufgrund der festen    */
/* LÑnge des events von 8 bytes ist auch sichergestellt, da· wÑhrend des     */
/* Kopierens zum Playbuffer KEIN overflow stattfinden kann.                  */
/*    Wenn usPlayWrHelp = usPlayRd : event passt nicht, also rc10            */
/*    usPlayRd ist der offset, an dem der Treiber die nÑchsten Daten LESEN   */
/*    WIRD, d.h. hier darf noch nicht geschrieben werden.                    */

/* if(usPlayWrHelp==usPlayRd) return(10);  Event will not fit to Buffer...   */

/* At this point: usPlayWrHelp points to an offset, at which 8 Bytes will    */
/* fit without reaching usPlayRd. No more checking required, only copy the   */
/* needed data.                                                              */

*(pPlayBuf+usPlayWrHelp)=ucTrkNum;             /* TRACK NUMBER as first byte */
usPlayWrHelp++;                                /* next write offset          */

if(ucTrkNum==0xFC) return(11);                 /* FC End in Trk number       */

*((ULONG*)(pPlayBuf+usPlayWrHelp))=ulTiming;   /* ABSOLUTE TIMING Data ULONG */
usPlayWrHelp+=4;                               /* next write offset          */

/* NEU: no more Running Status processing, copy ALL Status bytes.            */
/*      Generation Running Status will be done by the driver PLAY function.  */
/* Also no checking for LENGTH is needed, copy both MIDI Data1 and Data2.    */
/* The driver will analyze the data and determine, how many bytes have to be */
/* sent to the MPU and ignore any bytes not needed.                          */

/* NEU: if ucStatus=NOTE OFF: convert to NOTE ON with velocity=0: this will  */
/*      generate more Running Status during Playback, which put less load on */
/* the MIDI out Port. Also, analyzing will be easier within the driver ??    */

if((ucStatus>=0x80)&&(ucStatus<=0x8F))  /* NOTE OFF to NOTE ON conversion    */
   {
   ucStatus+=0x10;                             /* Convert to MIDI ON         */
   ucValue2=0;                                 /* with Velocity 0            */
   }

*(pPlayBuf+usPlayWrHelp)=ucStatus;             /* MIDI STATUS Byte           */
usPlayWrHelp++;                                /* next write offset          */

*(pPlayBuf+usPlayWrHelp)=ucValue1;             /* MIDI Value1 Byte           */
usPlayWrHelp++;                                /* next write offset          */

*(pPlayBuf+usPlayWrHelp)=ucValue2;             /* MIDI Value2 Byte           */

/* if(ucStatus==0xFC) return(11);          rc=11 if FC-End has been copied   */
return(0);
}
