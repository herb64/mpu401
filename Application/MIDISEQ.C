/*
*******************************************************************************
* Quelle:       MIDISEQ.C
* Programm:     MIDISEQ.EXE
*------------------------------------------------------------------------------
* Prozedur(en): main()
*               MainWinProc()
*               SecondThread()
*               Thr2WinProc()
*------------------------------------------------------------------------------
* 
*******************************************************************************
*/

#define  INCL_WINWINDOWMGR
#define  INCL_WINMESSAGEMGR
#define  INCL_WINFRAMEMGR
#define  INCL_WINSYS
#define  INCL_DOSPROCESS
#define  INCL_WINSTDDLGS                /* CUA controls and dialogs          */
#define  INCL_WINSTDCNR                 /* container control class           */
#define  INCL_WINMENUS                  /* Menu specific                     */
#define  INCL_PM

#include <os2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "midiseq.h"                    /* Include Definitionen fÅr Projekt  */
#include "mididlg.h"                    /* Include fÅr alle Dialogfenster    */
#include "midicnt.h"

/*---------------------------------------------------------------------------*/ 
/* Globale Variablen Deklarationen                                           */
/*---------------------------------------------------------------------------*/ 

UCHAR MPU_MODE;
HAB        hAB;                         /* Handle des Ankerblocks            */
UCHAR ucNumMemoryObjects;               /* Memory Object Allocation counter  */

/* Screen data and main window size                                          */
SWP    WinFrA;                          /* Window data  Framewindow A        */
LONG   scr_width;                       /* screen width                      */
LONG   scr_height;                      /* screen height                     */

POINTL ptl;

SHORT  dlgxLeft;
SHORT  dlgyBottom;
SHORT  dlgWidth;
SHORT  dlgHeight;

/* Variablen                                                                 */
UCHAR  ucRecordTrk;                     /* active track for capturing data   */
extern TRKCONTROL    *pTrkChunk;        /* pointer to Track Chunk Control    */
WRKBUF WrkBufMap[SEQ_TRACKS];           /* Administration Struct. Workbuffer */
extern PCHAR  pNoteTable[8];            /* Pointer to Notetable in driver    */
TID    tidSecThr;                       /* Thread ID fÅr Objectwindow        */
UCHAR  ucFader[SEQ_TRACKS];             /* Fader values for playback         */
HWND   hWndThr2;
HWND   hWndMainFrame;                   /* Handle of Frame Window            */
HWND   hWndMainClient;                  /* Handle of Child Window            */
HWND   hWndMainDialog;                  /* Handle of Main Dialog Window      */
HWND   hWndPopup;                       /* Popup Menue for test (Tracklist)  */
extern HWND   hWndTLContainer;          /* Tracklist Container Window handle */ 
RECTL  rclClientWindow;
extern SHORT  cnrxLeft;  
extern SHORT  cnryBottom;
extern SHORT  cnrWidth;
extern SHORT  cnrHeight;

ULONG  ulTotalMaxTime;                  /* Max timing of all tracks          */
ULONG  ulTimeFactor;                    /* Factor for normalizing (scrollbar)*/
ULONG  ulStartTime;                     /* Timing for playback start         */

/* Global Variable which affect TIMING, MEASURES and BEATS                   */
UCHAR  ucTaktmassZaehler;
UCHAR  ucTaktmassNenner;
UCHAR  ucTicksPerQuarter;

/* Variables for different modes of MPU    d=default                         */
UCHAR  ucBender=0;                      /* Pitch Bend   1=act 0=inact (d=0)  */
UCHAR  ucMidiThru=1;                    /* Midi Thru    1=act 0=inact (d=1)  */
UCHAR  ucMeasEnd=1;                     /* Measure End  1=act 0=inact (d=1)  */
UCHAR  ucExcl2Host=0;                   /* Excl2Host    1=act 0=inact (d=0)  */
UCHAR  ucTempo=0x64;                    /* Default Tempo                     */
UCHAR  ucRelTempo=0x40;                 /* Default Relative Tempo            */
UCHAR  ucGraduation=0;                  /* Default Graduation                */
UCHAR  ucMidiPerMetro=0x0C;             /* Default MIDI per Metro            */

/* Needed for Tracklistcontainer processing                                  */
extern NOTIFYRECORDEMPHASIS NotifyEmp;  /* Notification                      */
PTLRECORD pNotifyRecord;
PTLRECORD pNotifyRecord2;
UCHAR  ucNotifyTrk;                     /* Test for active nofification      */
UCHAR  ucTestTrkState;                  /* TEST                              */

/*---------------------------------------------------------------------------*/
/* Function MAIN()                                                           */
/*---------------------------------------------------------------------------*/

void main()                            /* Hauptfunktion                      */

{
         HMQ        hmq;            /* Handle der Nachrichtenenwarteschlange */
         QMSG       qMsg;           /* Struktur mit Nachrichten-Parametern   */
         ULONG      ulFrameFlags;   /* bestimmt Einzelkomonenten des Rahmens */

/* Initialisierung der PM - Anwendung, wir erhalten ein Anchor Block Handle  */
if (!(hAB = WinInitialize(0)))
    DosExit(EXIT_PROCESS, 1);       /* Anwendung beenden                     */

/* Anlegen einer Message Queue, die Fkt. gibt ein Message Queue Handle zurÅck*/
if (!(hmq = WinCreateMsgQueue(
                              hAB,  /* Anchor block handle                   */
                              0     /* Message Queue Grî·e                   */
                             )))
    {
    WinTerminate(hAB);              /* Anchor Block "zurÅckgeben"            */
    DosExit(EXIT_PROCESS, 1);
    }


/* Registrierung der Window-Klasse fÅr den Arbeitsbereich des Hauptfensters  */
if (!WinRegisterClass(
                      hAB,                    /* Ankerblock Handle           */
                      PWC_CLIENT_A,           /* Kind-Fenster Klasse         */
                      MainWinProc,            /* Fenster-Prozedur            */
                      CS_SIZEREDRAW,          /* Klassen-Attribute           */
                      0L                      /* Fenster-Speicher            */
                     ))
    {
    WinDestroyMsgQueue(hmq);            /* Message Queue abbbauen            */
    WinTerminate(hAB);                  /* Anchorblock freigeben             */
    DosExit(EXIT_PROCESS, 1);           /* Proze· beenden                    */
    }

/* Initialisierung der Variablen, die die Rahmenkomponenten festlegt (FCF_)  */
ulFrameFlags = FCF_BORDER        |      /* Window has no sizeborder          */
               FCF_TITLEBAR      |      /* Window has title                  */
               FCF_MINMAX        |      /* Window has MINIMIZE/MAXIMIZE but. */
               FCF_SHELLPOSITION |      /* Standard-Positioning              */
               FCF_MENU          |      /* Window has a menue                */
               FCF_ICON          |      /* Window has an icon                */
               FCF_SYSMENU       |      /* Window has a system menu          */
               FCF_TASKLIST;            /* Add window to TASKLIST            */

if (!(hWndMainFrame = WinCreateStdWindow(       /* Anlegen Hauptfenster A    */
                               HWND_DESKTOP,    /* Handle Desktop (Parent)   */
                               WS_VISIBLE |     /* Rahmen Attribut (WS.,FS..)*/
                               WS_MAXIMIZED,
                               &ulFrameFlags,   /* Rahmenzusammensetzung     */
                               PWC_CLIENT_A,    /* Klassenname               */
                               "MIDISEQ",       /* Text in Kopfzeile         */
                               0UL,             /* Kindfenster-Attribut      */
                               (HMODULE)0UL,    /* Ressource Handle          */
                               ID_FRAME_A,      /* Rahmen- u. Ressourcen-ID  */
                               &hWndMainClient  /* Handle des CLIENT A       */
                              )))               /* wird zurÅckgegeben !      */
    {
    WinDestroyMsgQueue(hmq);    /* Nachrichtenwarteschlange wird abgebaut    */
    WinTerminate(hAB);          /* Ankerblock wird freigegeben               */
    DosExit(EXIT_PROCESS, 1);   /* Proze· wird beendet                       */
    }

scr_width=WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN); 
scr_height=WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);
                                                      
WinFrA.x=0;                             /* xmin                              */
WinFrA.y=0;                             /* ymin                              */
WinFrA.cx=scr_width;                    /* width                             */
WinFrA.cy=scr_height;                   /* height                            */

WinSetWindowPos(hWndMainFrame,          /* Handle of Main window             */
                0L,                     /* Positioning Z-Axis                */
                (SHORT)WinFrA.x,        /* x-Position                        */
                (SHORT)WinFrA.y,        /* Y-Position                        */
                (SHORT)WinFrA.cx,       /* width                             */
                (SHORT)WinFrA.cy,       /* height                            */
                SWP_MOVE     |                                                 
                SWP_SIZE     |                                                 
/*                SWP_MAXIMIZE | */                                            
                SWP_SHOW);                                                     

WinQueryWindowRect(hWndMainClient,      /* get Client Window data            */
                   &rclClientWindow);
CreateTrkList();                        /* Create Tracklist container        */

dlgxLeft=(SHORT)rclClientWindow.xLeft;  /* prepare main dialog window pos    */
dlgyBottom=(SHORT)rclClientWindow.yTop/2+1;
dlgWidth=(SHORT)(rclClientWindow.xRight-rclClientWindow.xLeft);
dlgHeight=(SHORT)(rclClientWindow.yTop-rclClientWindow.yBottom);
dlgHeight=dlgHeight/2;                  /* use only Top half of client       */

WinLoadDlg(hWndMainClient,              /* Parent Window     TEST NON MODAL  */
      hWndMainClient,                   /* Owner Window (receives messages)  */
      MainDlgProc,                      /* Address of window procedure       */
      (HMODULE)0UL,                     /* DLL-Handle (if resource there)    */
      ID_DLG_MAIN,                      /* ID of main dialog window          */
      NULL);                            /* Pointer to optional data          */

WinSetWindowPos(hWndMainDialog,         /* Handle of Main dialog window      */
                0L,                     /* Positioning Z-Axis                */
                (SHORT)dlgxLeft,        /* x-Position                        */
                (SHORT)dlgyBottom,      /* Y-Position                        */
                (SHORT)dlgWidth,        /* width                             */
                (SHORT)dlgHeight,       /* height                            */
                SWP_MOVE     |                                                 
                SWP_SIZE     |                                                 
/*                SWP_MAXIMIZE | */                                            
                SWP_SHOW);                                                     

/* Aktivierung des Dispatchers und Abarbeitung der Nachrichtenschlange       */
while(WinGetMsg(hAB, &qMsg, (HWND)0UL, 0UL, 0UL)) /* Nachricht holen  ...    */
    WinDispatchMsg(hAB, &qMsg );                  /* ...und verteilen        */

WinDestroyWindow(hWndMainFrame);                 /* Abbau des Eltern-Fensters*/
WinDestroyMsgQueue(hmq);
WinTerminate(hAB);
}                                                /* Ende der Main - Prozedur */

/*---------------------------------------------------------------------------*/ 
/* Function: MainWinProc()                                                   */
/*---------------------------------------------------------------------------*/
/* Window procedure of main window.                                          */
/* Mainly messages from menue will occur here.                               */
/*---------------------------------------------------------------------------*/ 

MRESULT EXPENTRY MainWinProc(HWND    hWnd,  /* Handle des Arbeitsbereiches A */
                             ULONG   Msg,   /* Nachricht                     */
                             MPARAM  mp1,   /* Nachrichtenparameter 1        */
                             MPARAM  mp2)   /* Nachrichtenparameter 2        */

{
         HPS        hPS;            /* Presentation Space Handle             */
         RECTL      rctl;           /* Struktur mit Rechteckkoordinaten      */
         APIRET     rc;             /* Returncode Variable                   */
         CHAR       mess[25];       /* Testmessage                           */
         INT        i;              /* hilfsvariable                         */
         UCHAR      ucLoop;         /* loop variable                         */
extern   PCHAR   pCaptBuf;
extern   PCHAR   pPlayBuf;
extern   VDATA   MPU_Vers;          /* MPU and Driver Version structure      */
         CHAR       DlgText[45];    /* test for textoutput                   */

/* Die switch-case Konstruktion filtert die Nachrichten fÅr das Kind-Fenster */
switch (Msg)                        /* Abarbeiten aller Nachrichten          */
    {
    case WM_CREATE:                 /* Erzeugen des Fensters (1.Nachricht)   */
        /* At WM_CREATE of the MAIN Window, the MPU Hardware is initialized  */
        /* and all needed structures of the program (buffers, variables...)  */
        /* are allocated and set to initial values.                          */
        ucNumMemoryObjects=0;       /**INIT memory objects to 0   ************/
        rc=MPU_Open();              /**Open the MPU Device Driver ************/
        if (rc>0)
           {
           Display_Msg((USHORT)rc,hWnd,"Fehler in OPEN");
           break;
           }

        rc=MPU_Reset();             /**Reset the MPU**************************/
        if (rc>0)
           {                 
           Display_Msg((USHORT)rc,hWnd,"Fehler in RESET");
           break;            
           }
 
        ucBender=0;                 /** Set default mode values for MPU ******/
        ucMidiThru=1; 
        ucMeasEnd=1;  
        ucExcl2Host=0;
        ucTempo=0x64;       
        ucRelTempo=0x40;    
        ucGraduation=0;     
        ucMidiPerMetro=0x0C;

/*      rc=MPU_MeasEnd(0);             Disable Measure End to Host***********
        if (rc>0)
           {
           Display_Msg((USHORT)rc,hWnd,"Fehler in MEASURE END");
           break;
           }

        rc=MPU_Bender(1);              Enable Bender to Host*****************
        if (rc>0)
           {
           Display_Msg((USHORT)rc,hWnd,"Fehler in Set BENDER");
           break;
           }

        rc=MPU_MidiThru(0);            Disable MidiThru**********************
        if (rc>0)
           {
           Display_Msg((USHORT)rc,hWnd,"Fehler in Disable MidiThru");
           break;
           }                                                                 */
 
        rc=Alloc_Buffer(CAPTURE_SIZE,&pCaptBuf);  /* Allocate CAPTUREBuffer **/
        if (rc>0)
           {
           Display_Msg((USHORT)rc,hWnd,"Fehler in ALLOC CAPTURE BUFFER");
           break;
           }

        rc=Alloc_Buffer(PLAY_SIZE,&pPlayBuf);     /* Allocate PLAYBuffer *****/
        if (rc>0)
           {
           Display_Msg((USHORT)rc,hWnd,"Fehler in ALLOC PLAYBACK BUFFER");
           break;
           }

        for(ucLoop=0;ucLoop<8;ucLoop++)           /* Allocate Notetables *****/
           {
           rc=Alloc_Buffer(NOTETAB_SIZE,&pNoteTable[ucLoop]);
           if (rc>0)
              {
              DosBeep(300,500);
              Display_Msg((USHORT)rc,hWnd,"Fehler in ALLOC NOTETABLE()");
              break;
              }
           else DosBeep(4000,100);
           }
        rc=MPU_GDT_Notetable();
        if (rc>0)                                                   
           {                                                        
           Display_Msg((USHORT)rc,hWnd,"Fehler in MPU_GDT_Notetable()");
           break;                                                   
           }
                 
        ResetNoteTable();               /* reset the notetable               */

        /* Nun wird der 2.Thread erzeugt, Funktion SecondThread()            */
        /* Dieser Thread unterhÑlt ein Fenster, Åber welches er mit den      */
        /* anderen Fenstern (hWndMainClient) kommunizieren kann.             */
        tidSecThr=_beginthread(SecondThread,      /* Thread-Prozedur         */
                               NULL,              /* ???                     */
                               18000,             /* Stack                   */
                               (void *)NULL);     /* ???                     */

        rc=InitWrkBufMap();         /** Initialize Working Buffer ************/
        if (rc>0)                                                         
           {                                                              
           Display_Msg((USHORT)rc,hWnd,"Fehler in INITIALIZE WRKBUFMAP");
           break;                                                         
           }             
                                                 
        for(i=0;i<SEQ_TRACKS;i++)   /** Initialize Fader Values **************/
           ucFader[i]=255;

        rc=InitChannelTable();      /** Initialize MIDI Channel Table ********/
        if (rc>0)                                                        
           {                                                             
           Display_Msg((USHORT)rc,hWnd,"Fehler in INITIALIZE ChannelTab");
           break;                                                        
           }                                                             

        ucTaktmassZaehler=3;        /** Initialize Measure Defaults **********/
        ucTaktmassNenner=4;
        ucTicksPerQuarter=120;      /* Set Timebase default     ???          */

        Display_Msg(0,hWnd,"INITIALIZING done");
        rc=LogOpenFile();
        if(rc!=0) Display_Msg((USHORT)rc,hWnd,"Error OPEN LOG FILE");
        if(rc==0) rc=LogWriteFile("INITIALIZING done\n\0");
                  rc=LogWriteFile("und nochn text\n\0");
        if(rc!=0) Display_Msg(0,hWnd,"Error Write to LOG FILE");
        break;

    case WM_PAINT:
        /* Cached Presentation Space besorgen                                */
        hPS = WinBeginPaint(hWnd, (HPS)0UL, &rctl); 
        /* Fenster ganz oder teilweise einfÑrben                             */
     /* rctl.yBottom=rctl.yTop/2+1;    only upper half                       */
        WinFillRect(hPS, &rctl, CLR_BLUE);
        /* PS wieder freigeben                                               */
        WinEndPaint(hPS);                            
        break;

    case WM_SIZE:                   /* Size of client window changed         */
        WinQueryWindowRect(hWnd,
                           &rclClientWindow);
        cnrxLeft=(SHORT)rclClientWindow.xLeft;
        cnryBottom=(SHORT)rclClientWindow.yBottom;
        cnrWidth=(SHORT)(rclClientWindow.xRight-rclClientWindow.xLeft);
        cnrHeight=(SHORT)(rclClientWindow.yTop-rclClientWindow.yBottom);
        cnrHeight=cnrHeight/2;

        dlgxLeft=(SHORT)rclClientWindow.xLeft;
        dlgyBottom=(SHORT)rclClientWindow.yTop/2+1;
        dlgWidth=(SHORT)(rclClientWindow.xRight-rclClientWindow.xLeft);
        dlgHeight=(SHORT)(rclClientWindow.yTop-rclClientWindow.yBottom);
        dlgHeight=dlgHeight/2;

        WinSetWindowPos(hWndTLContainer,     /* Handle of TrkList Container  */
                        0L,                  /* Positioning Z                */
                        cnrxLeft,            /* x-Position                   */
                        cnryBottom,          /* Y-Position                   */
                        cnrWidth,            /* Width                        */
                        cnrHeight,           /* Height                       */
                        SWP_MOVE     |                                                 
                        SWP_SIZE     |                                                 
                        SWP_SHOW); 
        WinSetWindowPos(hWndMainDialog,      /* TEST fÅr Dialog              */
                        0L,                  /* Positioning Z                */
                        (SHORT)dlgxLeft,     /* x-Position                   */
                        (SHORT)dlgyBottom,   /* Y-Position                   */
                        (SHORT)dlgWidth,     /* width                        */
                        (SHORT)dlgHeight,    /* height                       */
                        SWP_MOVE     |                                         
                        SWP_SIZE     |                                         
                        SWP_SHOW);                                             
                                                   
        break;                                                                 

    case WM_CONTROL:                /* Test for Message from Container       */
    switch (SHORT2FROMMP(mp1))
        {                     
        case CN_SCROLL:
           break;

        case CN_BEGINEDIT:
           DosBeep(2000,100);
           break;            

        case CN_INITDRAG:
           DosBeep(1000,100);
           break;            

        case CN_CONTEXTMENU:            /* Test: display a popup menue       */
           hWndPopup=WinLoadMenu(hWnd,            /* Owner window            */
                                 (HMODULE) NULL,  /* Resource is in EXE      */
                                 ID_POPUP_TRK);   /* ID of popup menu        */

           WinQueryPointerPos(HWND_DESKTOP,       /* Query Pointer pos       */
                              &ptl);

           WinPopupMenu(HWND_DESKTOP,             /* Parent                  */
                        hWnd,                     /* Owner                   */
                        hWndPopup,                /* Handle of popup menu    */
                        ptl.x,                    /* x position              */
                        ptl.y,                    /* y position              */
                        (ULONG)MID_TL_EVENTLIST,  /* idItem                  */
                     /* PU_POSITIONONITEM   | */
                        PU_HCONSTRAIN       |
                        PU_VCONSTRAIN       |
                        PU_NONE             |
                        PU_MOUSEBUTTON1);
           break;

        case CN_ENTER:                  /* good for mouse doubleclick        */
           DosBeep(5000,300);
           break;            

        case CN_EMPHASIS:           /* warum kommt die msg mehrmals ??       */
        /* ErklÑrung: mehrere Wechsel mîglich, Siehe NotifyRecordEmphasis    */
        /* Struktur, deren Zeiger in dieser Msg Åbergeben wird.              */
        /* Frage: vermutlich ist das senden von mehreren MSG nicht zu unter- */
        /* drÅcken?? daher hier filtern, und nur auf die interessanten msg   */
        /* reagieren.                                                        */
        /* hier: set und reset der active flags fÅr den treiber, sowie       */
        /* der active/inact Flags in dem entsprechenden Record (toggle)      */
        /* prevent toggle and try to query state: not directly possible, so  */
        /* use QUERYRECORDEMPHASIS..                                         */
           NotifyEmp=*((PNOTIFYRECORDEMPHASIS)PVOIDFROMMP(mp2));
           if(NotifyEmp.fEmphasisMask==CRA_SELECTED)   /* only react if      */
              {                                        /* selection changed  */
              QueryTLRecordState(NotifyEmp.pRecord,
                                 &ucNotifyTrk,
                                 &ucTestTrkState);
              ChangePlayTrkStatus(ucTestTrkState,
                                  ucNotifyTrk);
              }
           break;            

        default:
           break;
        }
        break;

    case WM_COMMAND:                /* Menu Nachricht empfangen......        */
    switch (SHORT1FROMMP(mp1))      /* Menuauswahl-Filter                    */
        {                                                                  
        case MID_NEW:               /* Test Tracklist in container control   */
/*T        CreateTrkList();    */
           break;

        case MID_OPEN:              /* open for MIDI file                    */
           for(ucLoop=0;ucLoop<SEQ_TRACKS;ucLoop++)         /* cleanup       */
              {                                                            
              rc=DeleteAllTrackExtents((UCHAR)ucLoop);
              }                           
           InitChannelTable();          /* Reset channel table               */
           rc=LoadMidiFile();           /* call loader function in MIDIFIL1  */

           ulTotalMaxTime=GetMaximumTiming();
           sprintf(DlgText,"MAXTIME: %6d",ulTotalMaxTime);
           WriteListBox(hWndMainDialog,                
                        DID_MD_LISTBOX,                
                        DlgText);             
         
           UpdateTrkList();             /* Update container with Tracklist   */
           MPU_SetActiveTrk();          /* Update driver active mask         */
           MPU_ChannelChange();         /* Update driver channel info        */
           rc=Free_Buffer((PVOID)pTrkChunk);                
           if (rc>0)                                        
              {                                             
              Display_Msg((USHORT)rc,hWnd,                  
                          "Fehler in FREE TRKCHUNK BUFFER");
              }                                             
           break;

        case MID_GETVER:            /* Request MPU and Driver Version        */
            rc=MPU_GetVer((PVOID)&MPU_Vers); /* Get Version to Structure     */
            WinDlgBox(HWND_DESKTOP, /* Eltern-Fenster                        */
                 hWnd,              /* Besitzer-Fenster                      */
                 VersDlgProc,       /* Adr. der zustÑndigen Prozedur         */
                 (HMODULE)0UL,      /* DLL-Handle (falls Res. dort)          */
                 ID_DLG_VERSION,    /* ID des Versions Dialogfensters        */
                 NULL);             /* Zeiger auf Daten, die mitgegeben      */
                                    /* werden kînnen                         */
            break;                                                             
          
        /* "Ende": wir simulieren die Beendigung durch das System-Menu,      */
        /* indem wir uns "selbst" WM_CLOSE schicken                          */
        case MID_EXIT:              /* Beenden per Menue angefordert         */
            for(ucLoop=0;ucLoop<SEQ_TRACKS;ucLoop++)   /* Extent deletion    */
               {
               rc=DeleteAllTrackExtents(ucLoop);  /* Delete Track Extents    */
               if (rc>0)
                  {
                  Display_Msg((USHORT)rc,hWnd,
                              "Fehler in DeleteAllTrackExtents");
                  }
               }                                                            
            rc=Free_Buffer((PVOID)pCaptBuf);      /* Free Capture Buffer  ****/
            if (rc>0)
               {                                    
               Display_Msg((USHORT)rc,hWnd,"Fehler in FREE CAPTURE BUFFER");
               }     
            rc=Free_Buffer((PVOID)pPlayBuf);      /* Free Playback Buffer ****/
            if (rc>0)                                                       
               {                                                            
               Display_Msg((USHORT)rc,hWnd,"Fehler in FREE PLAYBACK BUFFER");
               }                     
            rc=MPU_Free_Notetable();              /* Unlock in driver        */
            if (rc>0)                                                  
               {                                                       
               Display_Msg((USHORT)rc,hWnd,"Fehler in MPU_Free_Notetable");
               } 
            for(ucLoop=0;ucLoop<8;ucLoop++)       /* Free Notetable Buffers **/
               {
               rc=Free_Buffer((PVOID)pNoteTable[ucLoop]);
               if (rc>0)
                  {
                  Display_Msg((USHORT)rc,hWnd,"Fehler in FREE Notetable");
                  }
               }
            rc=LogCloseFile();
            if (rc>0)                                                        
               {                                                             
               Display_Msg((USHORT)rc,hWnd,"Fehler in CLOSE LOGFILE");
               }
            if (ucNumMemoryObjects!=0)
               {                                                      
               Display_Msg((USHORT)ucNumMemoryObjects,
                           hWnd,"Fehler: Speicherobjekte ohne FreeMem...");
               }              
            WinDestroyWindow(hWndTLContainer);
            WinDestroyWindow(hWndMainDialog);
            WinPostMsg(hWnd, WM_CLOSE, (MPARAM)0, (MPARAM)0);              
            break;                                                         

        case MID_TL_MIXER:              /* TEST: Popup Menu from Tracklist   */
            DosBeep(2200,50);
       /*     WinLoadDlg(hWndMainClient,   Parent Window     TEST NON MODAL  */
       /*           hWndMainClient,        Owner Window (receives messages)  */
       /*           MixDlgProc,            Address of window procedure       */
       /*           (HMODULE)0UL,          DLL-Handle (if resource there)    */
       /*           ID_DLG_MIXER,          ID of Mixer dialog window         */
       /*           NULL);                 Pointer to optional data          */
            WinLoadDlg(HWND_DESKTOP,/* Eltern-Fenster                        */
                 hWnd,              /* Besitzer-Fenster                      */
                 MixDlgProc,        /* Adr. der zustÑndigen Prozedur         */
                 (HMODULE)0UL,      /* DLL-Handle (falls Res. dort)          */
                 ID_DLG_MIXER,      /* ID des Versions Dialogfensters        */
                 NULL);             /* Zeiger auf Daten, die mitgegeben      */

            break;
                                                                           
        default:                                                           
            break;                                                         
        }                                                                  

    default:                        /* Default Aktion fÅr nicht abgefangene  */
        return(WinDefWindowProc(hWnd, Msg, mp1, mp2));    /* Nachrichten     */
    }
return ((MRESULT)0L);
}

/*---------------------------------------------------------------------------*/
/* Function: SecondThread()                                                  */
/*---------------------------------------------------------------------------*/

void _Optlink SecondThread(void * pv)   /* Optlink wegen Parm öbergabe       */
{                                                                              
HAB        hAB2;                        /* Handle Anchor Block               */
HMQ        hmq2;                        /* Message Queue Handle              */
QMSG       qMsg2;                       /* Struktur Nachrichtenparameter     */
BOOL       fRc;                         /* returncode                        */
                                                                               
hAB2 = WinInitialize(0);                                                       
hmq2 = WinCreateMsgQueue(hAB2,0);                                              
fRc=WinCancelShutdown(hmq2,TRUE);       /* Verhindern von WM_QUIT Messages   */
fRc=WinRegisterClass(hAB2,                                                     
                     PWC_THREAD2,       /* Kind Fenster Klasse               */
                     Thr2WinProc,       /* Fenster Prozedur                  */
                     0L,                /* Class Attributes                  */
                     0L);               /* Fenster Speicher                  */
hWndThr2=WinCreateWindow(HWND_OBJECT,   /* Definierte Konstante              */
                         PWC_THREAD2,                                          
                         "",                                                   
                         0,             /* Style                             */
                         0,             /* Position x                        */
                         0,             /* Position y                        */
                         0,             /* Breite                            */
                         0,             /* Hîhe                              */
                         hWndMainClient,/* Owner Window                      */
                         HWND_BOTTOM,                                          
                         0,             /* window ID                         */
                         NULL,          /* Control Data (global data)        */
                         NULL);         /* Presentation Parameter            */
                                                                               
DosSetPriority(PRTYS_THREAD,            /* Setzen Priority des Threads       */
               PRTYC_IDLETIME,                                                 
               +31,                                                            
               tidSecThr);                                                     
                                                                               
while(WinGetMsg(hAB2,&qMsg2,(HWND)0UL,0UL,0UL))                                
   WinDispatchMsg(hAB2,&qMsg2);                                                
                                                                               
WinPostMsg(hWndMainClient,                                                     
           WM_QUIT,                                                            
           0,                                                                  
           0);                                                                 
WinDestroyWindow(hWndThr2);                                                    
WinDestroyMsgQueue(hmq2);                                                      
WinTerminate(hAB2);                                                            
return;                                                                        
}                                                                              
                                                                               
/*---------------------------------------------------------------------------*/
/* Function: Thr2WinProc()                                                   */
/*---------------------------------------------------------------------------*/
/* Window Procedure of objectwindow in second thread                         */
/*---------------------------------------------------------------------------*/
MRESULT EXPENTRY Thr2WinProc(HWND    hWnd,   /* Handle des Window            */
                             ULONG   Msg,    /* Nachricht                    */
                             MPARAM  mp1,    /* Nachrichtenparameter 1       */
                             MPARAM  mp2)    /* Nachrichtenparameter 2       */
{                                                                              
switch (Msg)                                                                   
    {                                                                          
    case WM_CREATE:                                                            
        break;                                                                 

    default:                            /* nicht abgefangene Nachrichten     */
        return(WinDefWindowProc(hWnd,Msg,mp1,mp2));                            
    }                                                                          
return((MRESULT)0L);                                                           
}                                                                              
