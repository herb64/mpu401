/*
*******************************************************************************
* Quelle:       MIDICNT1.C
* Programm:     MIDISEQ.EXE
*------------------------------------------------------------------------------
*
* This Source contains code for displaying container windows.
* Container windows are planned to be used for display of tracks overview and
* for display of event list windows.
*
* Procedures  : CreateTrkList()         Create Tracklist Container window
*               QueryTLRecordState()    Help: Query Selected State of Track
*               UpdateTrkList()
*
*******************************************************************************
*/

#define  INCL_WINWINDOWMGR 
#define  INCL_WINMESSAGEMGR
#define  INCL_WINFRAMEMGR  
#define  INCL_WINSYS     
#define  INCL_WINSTDDLGS                /* CUA controls and dialogs          */
#define  INCL_WINSTDCNR                 /* container control class           */
#define  INCL_DOSPROCESS
#define  INCL_DOSMEMMGR                 /* for DosAllocMem                   */
#define  INCL_DOSMODULEMGR              /* DosLoadModule                     */
#define  INCL_WINMENUS                  /* Menu specific                     */
#define  INCL_PM

#include <os2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "midiseq.h"                    /* Include defintions for project    */
#include "mididlg.h"                    /* Include for all dialog-windows    */
#include "midicnt.h"                    /* Include container specific        */

/*---------------------------------------------------------------------------*/ 
/* Global variable declarations                                              */
/*---------------------------------------------------------------------------*/ 

extern WRKBUF WrkBufMap[SEQ_TRACKS];    /* Administration Structure          */
extern UCHAR  ucFader[SEQ_TRACKS];      /* Fader value table                      */
extern HWND   hWndMainClient;           /* main client window handle         */
extern HAB    hAB;                      /* Handle of anchorblock             */

extern RECTL  rclClientWindow;
SHORT  cnrxLeft;
SHORT  cnryBottom;
SHORT  cnrWidth;
SHORT  cnrHeight;

/*---------------------------------------------------------------------------*/
/* Variables needed for TRACKLIST Container                                  */
/*---------------------------------------------------------------------------*/

PFNWP         pfnTLContainer;           /* Pointer to hold org. windowproc.  */
CNRINFO       cnrTL;                    /* Tracklist Container info          */
PFIELDINFO    pfiTL;                    /* Fieldinfo Structures (1 per col.) */
ULONG         ulTLContStyles;           /* Container styles Track list       */
HWND          hWndTLContainer;          /* Tracklist Container Window handle */
ERRORID       errorcode;                /* For WinGetLastError               */
UCHAR         ucLoop;

/*-RECORD specific data------------------------------------------------------*/
PTLRECORD     pTLRecord;                /* Pointer to Trklist Record struct  */
PTLRECORD     pTLFirstRecord;
USHORT        usRecords;                /* number of records to allocate     */
RECORDINSERT  recordinsert;             /* Record Insert structure           */
ULONG         cbRecordData;             /* Size of user specific in TLRECORD */

/*-FIELDINFO specific data---------------------------------------------------*/
PFIELDINFO    pFieldInfo;               /* Fieldinfo chain list pointer      */
PFIELDINFO    pFirstFieldInfo;
FIELDINFOINSERT FieldInfoInsert;        /* control for insert of field info  */
CHAR          Title1[]="Spur";          /* Column Titles for Container       */
CHAR          Title2[]="Name";
CHAR          Title3[]="Instr.";
CHAR          Title4[]="Size";
CHAR          Title5[]="MIDI";
CHAR          Title6[]="VEL";
CHAR          Title7[]="Titel6";

CHAR          pszIcon1Txt[]="ICON 1";   /* Test for ICON and TEXT Views      */
CHAR          pszIcon2Txt[]="ICON 2";
CHAR          pszText1Txt[]="TEXT 1";
CHAR          pszText2Txt[]="TEXT 2";

CHAR          pszTest3[]="test 3";

CHAR          pszMidiChan[18][3]={"","1","2","3","4","5",
                                 "6","7","8","9","10","11",
                                 "12","13","14","15","16","M"};

CHAR          pszVelocity[]="000";      /* Velocity Default                  */

/* Data for Nofifications of emphasis change in container -------------------*/
NOTIFYRECORDEMPHASIS NotifyEmp;

/*---------------------------------------------------------------------------*/
/* Function CreateTrkList()                                                  */
/*---------------------------------------------------------------------------*/
/* This function initializes needed data structures for the Tracklist        */
/* container window.                                                         */
/* The Tracklist container is planned to be opened in the Clientarea of the  */
/* main application window.                                                  */
/*---------------------------------------------------------------------------*/

APIRET CreateTrkList(void)
{
APIRET rc=0;                            /* Returncode                        */

/*---------------------------------------------------------------------------*/
/* Create the window                                                         */
/*---------------------------------------------------------------------------*/

/* Query size from main client window for adjusting the container to this    */
/* area.                                                                     */



cnrxLeft=(SHORT)rclClientWindow.xLeft;
cnryBottom=(SHORT)rclClientWindow.yBottom;
cnrWidth=(SHORT)(rclClientWindow.xRight-rclClientWindow.xLeft);
cnrHeight=(SHORT)(rclClientWindow.yTop-rclClientWindow.yBottom);
cnrHeight=cnrHeight/2;                  /* use only Bottom half of client    */

ulTLContStyles=CCS_AUTOPOSITION |       /* Customize container Styles        */
               CCS_MULTIPLESEL;

if (!
(hWndTLContainer = WinCreateWindow(
                  hWndMainClient,       /* Parent window handle              */
                  WC_CONTAINER,
                  "TESTTITEL",          /* No window text                    */
                  ulTLContStyles,       /* Container styles                  */
                  cnrxLeft,             /* Horizontal position of window     */
                  cnryBottom,           /* Vertical position of window       */
                  cnrWidth,             /* Window width                      */
                  cnrHeight,            /* Window height                     */
                  hWndMainClient,       /* Owner window handle               */
                  HWND_TOP,             /* Sibling window handle             */
                  ID_TL_CONTAINER,      /* Container window ID               */
                  (PVOID)NULL,          /* No control data                   */
                  (PVOID)NULL))         /* No presentation parameters        */
   )  /* end of if */
   {
   errorcode=WinGetLastError(hAB);

   DosBeep(3000,300);                   /* Beep if error in create window    */
   }

/* NOW do a subclassing of the container window for own processing...        */
/* pfnTLContainer will get passed the original window procedure              */
/* SubclassTLContainer is the new Window procedure...                        */
pfnTLContainer=WinSubclassWindow(hWndTLContainer,
                                 (PFNWP)SubclassTLContainer);

if(!(
WinShowWindow(hWndTLContainer,          /* Container window handle           */
              TRUE)                     /* Make the window visible           */
))
   {
   errorcode=WinGetLastError(hAB);
   DosBeep(3000,300);
   }

/*---------------------------------------------------------------------------*/
/* ALLOCATE storage for the records in the container (lines in detail view)  */
/* and fill with data from WRKBUF Structure                                  */
/*---------------------------------------------------------------------------*/

usRecords=TL_NUMRECORDS;              /* Records to allocate                 */
cbRecordData=(LONG) (sizeof(TLRECORD)-sizeof(RECORDCORE)); /* User data size */
                                                    
pTLRecord =                           /* allocate storage for records        */
    (PTLRECORD)WinSendMsg (
    hWndTLContainer,                  /* Container window handle             */
    CM_ALLOCRECORD,                   /* Message for allocating the record   */
    MPFROMLONG(cbRecordData),         /* size of additional user data in rec */
    MPFROMSHORT(usRecords));          /* Number of records to be allocated   */

pTLFirstRecord=pTLRecord;

for(ucLoop=0;ucLoop<TL_NUMRECORDS;ucLoop++)       /* FILL Record structures  */
   {
   pTLRecord->RecordCore.cb = sizeof(RECORDCORE);
   pTLRecord->RecordCore.pszIcon=pszIcon1Txt;     /* for ICON View           */
   pTLRecord->RecordCore.pszText=pszText1Txt;     /* for TEXT View           */
/* pTLRecord->RecordCore.flRecordAttr=CRA_SELECTED;                     TEST */
   pTLRecord->ulTrkNum=(ULONG)(ucLoop+1);         /* fixed                   */
   pTLRecord->pszTrkName=WrkBufMap[ucLoop].chTrkName;
   pTLRecord->pszInstrument=WrkBufMap[ucLoop].chInstrument;
   pTLRecord->ulTrkSize=WrkBufMap[ucLoop].ulNumMidiEvents;
   pTLRecord->pszMidiChan=pszMidiChan[WrkBufMap[ucLoop].ucMidiChannel];
   sprintf(pszVelocity,"%3d",ucFader[ucLoop]);
   pTLRecord->pszVelocity=pszVelocity;            /* velocity                */
   pTLRecord->pszText3=pszTest3;
   pTLRecord=(PTLRECORD)(pTLRecord->RecordCore.preccNextRecord);    /* NEXT  */
   }

/*---------------------------------------------------------------------------*/
/* Prepare structures for INSERTRECORD function                              */
/*---------------------------------------------------------------------------*/
                                                                               
recordinsert.cb = sizeof(RECORDINSERT);                                        
recordinsert.pRecordParent= NULL;                                              
recordinsert.pRecordOrder = (PRECORDCORE)CMA_END;                              
recordinsert.zOrder = CMA_TOP;                                                 
recordinsert.cRecordsInsert = TL_NUMRECORDS;                                   
recordinsert.fInvalidateRecord = TRUE;                                         

/*---------------------------------------------------------------------------*/ 
/* Insert records  into the container                                        */
/*---------------------------------------------------------------------------*/ 
                                                                                
WinSendMsg(hWndTLContainer,                                                     
           CM_INSERTRECORD,                                                     
           MPFROMP((PRECORDCORE)pTLFirstRecord),
           MPFROMP(&recordinsert));           
                                  
/*---------------------------------------------------------------------------*/
/* ALLOCATE field info structures for DETAIL view and init field types.      */
/* The container returns a chained list of storage for the requested number  */
/* of columns (fields)                                                       */
/*---------------------------------------------------------------------------*/

pFieldInfo =                          /* Allocate storage for field info     */
    (PFIELDINFO)WinSendMsg (
    hWndTLContainer,                  /* Container window handle             */
    CM_ALLOCDETAILFIELDINFO,          /* Message for alloc field info        */
    MPFROMLONG(TL_NUMFIELDS),         /* number of fields (columns)          */
    0L);

/* Initialize the Fieldinfo structure with needed values                     */
pFirstFieldInfo = pFieldInfo;         /* store start pointer for structure   */
                                                                  
pFieldInfo->cb = sizeof(FIELDINFO);     /**** TRACKNUMBER COLUMN *************/
pFieldInfo->flData = CFA_ULONG |
                     CFA_HORZSEPARATOR | 
                     CFA_RIGHT |
                     CFA_SEPARATOR;
pFieldInfo->flTitle = CFA_CENTER;       
pFieldInfo->pTitleData = (PVOID) Title1;
pFieldInfo->offStruct = FIELDOFFSET(TLRECORD,ulTrkNum);     /* where to find */
pFieldInfo = pFieldInfo->pNextFieldInfo;                    /* CHAINED list  */

pFieldInfo->cb = sizeof(FIELDINFO);     /**** TRACKNAME COLUMN ***************/
pFieldInfo->flData = CFA_STRING |
                     CFA_HORZSEPARATOR |
                     CFA_LEFT |
                     CFA_SEPARATOR;                             
pFieldInfo->flTitle = CFA_CENTER;       
pFieldInfo->pTitleData = (PVOID) Title2;
pFieldInfo->offStruct = FIELDOFFSET(TLRECORD,pszTrkName);
pFieldInfo = pFieldInfo->pNextFieldInfo;

pFieldInfo->cb = sizeof(FIELDINFO);     /**** INSTRUMENT NAME COLUMN *********/
pFieldInfo->flData = CFA_STRING |                                              
                     CFA_HORZSEPARATOR |                                       
                     CFA_LEFT |                                                
                     CFA_SEPARATOR;                                            
pFieldInfo->flTitle = CFA_CENTER;                                              
pFieldInfo->pTitleData = (PVOID) Title3;
pFieldInfo->offStruct = FIELDOFFSET(TLRECORD,pszInstrument);
pFieldInfo = pFieldInfo->pNextFieldInfo;                                       

pFieldInfo->cb = sizeof(FIELDINFO);     /**** TRACKSIZE   COLUMN *************/
pFieldInfo->flData = CFA_ULONG |
                     CFA_HORZSEPARATOR |
                     CFA_RIGHT |
                     CFA_SEPARATOR;
pFieldInfo->flTitle = CFA_CENTER;                                              
pFieldInfo->pTitleData = (PVOID) Title4;
pFieldInfo->offStruct = FIELDOFFSET(TLRECORD,ulTrkSize);
pFieldInfo = pFieldInfo->pNextFieldInfo;

pFieldInfo->cb = sizeof(FIELDINFO);     /**** MIDI CHANNEL COLUMN ************/
pFieldInfo->flData = CFA_STRING |
                     CFA_HORZSEPARATOR |
                     CFA_RIGHT |
                     CFA_SEPARATOR;
pFieldInfo->flTitle = CFA_CENTER;
pFieldInfo->pTitleData = (PVOID) Title5;
pFieldInfo->offStruct = FIELDOFFSET(TLRECORD,pszMidiChan);
pFieldInfo = pFieldInfo->pNextFieldInfo;

pFieldInfo->cb = sizeof(FIELDINFO);     /**** VELOCITY COLUMN ****************/
pFieldInfo->flData = CFA_STRING |
                     CFA_HORZSEPARATOR |
                     CFA_RIGHT |
                     CFA_SEPARATOR;
pFieldInfo->flTitle = CFA_CENTER;
pFieldInfo->pTitleData = (PVOID) Title6;
pFieldInfo->offStruct = FIELDOFFSET(TLRECORD,pszVelocity);
pFieldInfo = pFieldInfo->pNextFieldInfo;

pFieldInfo->cb = sizeof(FIELDINFO);     /****             COLUMN *************/
pFieldInfo->flData = CFA_STRING |
                     CFA_HORZSEPARATOR |
                     CFA_CENTER |
                     CFA_SEPARATOR;                             
pFieldInfo->flTitle = CFA_CENTER;
pFieldInfo->pTitleData = (PVOID) Title7;
pFieldInfo->offStruct = FIELDOFFSET(TLRECORD,pszText3);
pFieldInfo = pFieldInfo->pNextFieldInfo;                          

FieldInfoInsert.cb = (ULONG)(sizeof(FIELDINFOINSERT));      /* Control the   */
FieldInfoInsert.pFieldInfoOrder = (PFIELDINFO)CMA_FIRST;    /* insertion     */
FieldInfoInsert.cFieldInfoInsert = TL_NUMFIELDS,            /* with this     */
FieldInfoInsert.fInvalidateFieldInfo = TRUE;                /* structure     */
                
WinSendMsg(hWndTLContainer,             /* Send Field info to container      */
           CM_INSERTDETAILFIELDINFO,
           MPFROMP(pFirstFieldInfo),              
           MPFROMLONG(&FieldInfoInsert));

/*---------------------------------------------------------------------------*/
/* Set view to DETAILS view and initialize parameters in CONTROL structure   */
/*---------------------------------------------------------------------------*/

cnrTL.cFields=TL_NUMFIELDS;             /* number of fields                  */
cnrTL.cRecords=TL_NUMRECORDS;
cnrTL.pszCnrTitle="Gesamter Titel fr den Container, z.B. MIDI File Name";
cnrTL.flWindowAttr=CV_DETAIL |
                   CA_CONTAINERTITLE |
                   CA_DETAILSVIEWTITLES |
                   CA_TITLESEPARATOR;
cnrTL.xVertSplitbar=400;
cnrTL.pFieldInfoLast=pFirstFieldInfo; 
cnrTL.cyLineSpacing=0;

WinSendMsg(hWndTLContainer,                                                    
           CM_SETCNRINFO,
           MPFROMP(&cnrTL),
           MPFROMLONG((LONG)(CMA_FLWINDOWATTR |
                             CMA_XVERTSPLITBAR |
                             CMA_LINESPACING |
                             CMA_CNRTITLE)));

return(rc); 
}

/*---------------------------------------------------------------------------*/
/* Function QueryTLRecordState()                                             */
/*---------------------------------------------------------------------------*/
/* This function returns the Selected state of a record (i.e. Track) in the  */
/* TrackList container. The PM Interface does not allow to query the select  */
/* state of a record DIRECTLY, so this help function is used, which does     */
/* implement this function.                                                  */
/* Parameters: 1. Pointer to the RECORDCORE structure of the record to be    */
/*                queried.                                                   */
/* Return:     1  if selected, 0 if not selected                             */
/*---------------------------------------------------------------------------*/

UCHAR QueryTLRecordState(PRECORDCORE pQueryRecord,
                         UCHAR*      ucTrkNum,
                         UCHAR*      ucTrkSel)
{
UCHAR     ucQueryTrk;
UCHAR     ucTrkState;

ucQueryTrk=(UCHAR)((PTLRECORD)pQueryRecord)->ulTrkNum;
if(pQueryRecord==(PRECORDCORE)pTLFirstRecord)
   {                                                             
   pQueryRecord=(PRECORDCORE)CMA_FIRST;
   }                                                             
else                                    /* get previous RECORDCORE pointer   */
   {                                                             
   pQueryRecord=WinSendMsg(hWndTLContainer,                                   
                           CM_QUERYRECORD,
                           MPFROMP(pQueryRecord),
                           MPFROM2SHORT((SHORT)CMA_PREV,
                                        (SHORT)CMA_ITEMORDER));
   }                                                             
pQueryRecord=WinSendMsg(hWndTLContainer,                                      
                        CM_QUERYRECORDEMPHASIS,
                        MPFROMP(pQueryRecord),
                        MPFROMSHORT((SHORT)CRA_SELECTED));
if(pQueryRecord!=NULL)
   {                                                             
   ucTrkState=
      (UCHAR) (((UCHAR)((PTLRECORD)pQueryRecord)->ulTrkNum)==ucQueryTrk);
   }                                                             
else                                                             
   {                                                             
   ucTrkState=0;                                             
   }             
*ucTrkNum=ucQueryTrk;
*ucTrkSel=ucTrkState;
return(ucTrkState);
}

/*---------------------------------------------------------------------------*/ 
/* Function UpdateTrkList()                                                  */
/*---------------------------------------------------------------------------*/ 
/* This function updates the Tracklist Container by invalidating all 64      */
/* records. Call: after loading a MIDI File to update info                   */
/*---------------------------------------------------------------------------*/

APIRET UpdateTrkList(void)
{
APIRET rc=0;
UCHAR  ucLoop;

pTLRecord=pTLFirstRecord;
 
for(ucLoop=0;ucLoop<TL_NUMRECORDS;ucLoop++)       /* FILL Record structures  */
   {
   pTLRecord->ulTrkSize=WrkBufMap[ucLoop].ulNumMidiEvents;
   pTLRecord->pszMidiChan=pszMidiChan[WrkBufMap[ucLoop].ucMidiChannel];
   sprintf(pszVelocity,"%3d",ucFader[ucLoop]);
   pTLRecord->pszVelocity=pszVelocity;            /* velocity                */
   pTLRecord=(PTLRECORD)(pTLRecord->RecordCore.preccNextRecord);    /* NEXT  */
   }                                                                           

WinSendMsg(hWndTLContainer,                    
           CM_INVALIDATERECORD,
           MPFROMP(NULL),
           MPFROM2SHORT((SHORT)0,                 /* ALL records             */
                        (SHORT)CMA_TEXTCHANGED));
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function SubclassTLContainer()                                            */
/*---------------------------------------------------------------------------*/

MRESULT EXPENTRY SubclassTLContainer(HWND hwnd,
                                     ULONG msg,
                                     MPARAM mp1,
                                     MPARAM mp2)

/* Problem: Messages kommen nur, wenn nicht im arbeitsbereich der Records    */
/* geklickt wird.                                                            */

{                                                                               
switch (msg)
   {
   case WM_BUTTON1UP:
   case WM_CONTEXTMENU:
      DosBeep(1000, 500);
      break;
 
   case WM_BUTTON2UP:
      DosBeep(2000, 500);
      break;              

   case WM_CONTROL:
      DosBeep(5000, 200);
      break;             

   default:
      break;
    }

/* Pass all messages to the original window procedure (returned by subclass) */
return (MRESULT) pfnTLContainer(hwnd, msg, mp1, mp2);
}                                                                               
