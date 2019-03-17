/*
*******************************************************************************
* Quelle:       MIDIDLG1.C
* Programm:     MIDISEQ.EXE
*-------------------------------------------------------------------------- 
* Prozedur(en): MainDlgProc         Hauptdialogfenster-Prozedur (REC/PLAY)
*               VersDlgProc         Get Version Dialog
*               ChanDlgProc         Channel Mapper Dialog
*               FiltDlgProc         Record Filter Dialog

*
*-------------------------------------------------------------------------- 
*                                                                           
*******************************************************************************
*/


#define  INCL_WINWINDOWMGR
#define  INCL_WINBUTTONS
#define  INCL_WINENTRYFIELDS
#define  INCL_WINDIALOGS
#define  INCL_WININPUT                  /* VK_DOWN etc.                      */
#define  INCL_WINLISTBOXES
#define  INCL_WINTIMER                  /* Without: NO Compiler Warnings     */
#define  INCL_WINMESSAGEMGR
#define  INCL_WINSTDSPIN                /* Spinbutton Controls               */
#define  INCL_WINSTDSLIDER              /* Slider Controls                   */
#define  INCL_WINSCROLLBARS             /* Scrollbar control                 */

#include <os2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "midiseq.h"
#include "mididlg.h"               /* Dialogfenster Konstanten               */

ULONG ulAccDriverTime=0;           /* actual accu timing from driver         */
extern ULONG  ulTotalMaxTime;      /* Max timing of all tracks          */

ULONG ulPlayTimerId;               /* Ids returned from WinStartTimer        */
ULONG ulRecTimerId;

/* Level Display */
ULONG   ulLevTimerID;
SHORT   sScrollbarPos;             /* Scrollbar position for Progress ind.   */

UCHAR ucStartPlayTimerFlag;        /* Flag to determine, if Timer necessary  */
UCHAR ucRunningStatus;             /* For AnalyzeCapturedMidiEvent           */
UCHAR ucRunStatLength;             /* Running Status and length              */
ULONG ulAccuTiming;                /* for record: accumulator for timings    */
extern WRKBUF WrkBufMap[SEQ_TRACKS];    /* Administration Structure          */
extern HWND   hWndMainDialog;           /* Handle of Main Dialog Window      */
extern HWND   hWndMixDialog;            /* Mixer Dialog handle               */
extern UCHAR  ucFader[SEQ_TRACKS];      /* Fader value table                 */

UCHAR ucCommand;                   /* Testvariables for MPU_Send_Data()      */
UCHAR ucPitch;
UCHAR ucVelocity;
UCHAR ucChannel;

extern UCHAR  ucTaktmassZaehler;
extern UCHAR  ucTaktmassNenner;
extern UCHAR  ucTicksPerQuarter;

/* MIDI CHANNEL specific                                                     */
PSZ    pszChan[]={"1","2","3","4","5","6","7","8","9",
                  "10","11","12","13","14","15","16","17"}; /* 17 = MULT     */
UCHAR  ucChanTab[SEQ_TRACKS];           /* MIDI Channel table                */

/* Test for Slider control                                                   */
ULONG ulSlFaderValue=238;               /* Output slider to listbox          */
ULONG ulSlTempoValue=100;               /* Tempo Slider value                */
ULONG ulSlRelTmpValue=40;               /* Relative Tempo Slider value       */

/* Variables for different modes of MPU    d=default                         */
extern UCHAR  ucBender;                 /* Pitch Bend   1=act 0=inact (d=0)  */
extern UCHAR  ucMidiThru;               /* Midi Thru    1=act 0=inact (d=1)  */
extern UCHAR  ucMeasEnd;                /* Measure End  1=act 0=inact (d=1)  */
extern UCHAR  ucExcl2Host;              /* Excl2Host    1=act 0=inact (d=0)  */
extern UCHAR  ucTempo;                  /* Default Tempo                     */
extern UCHAR  ucRelTempo;               /* Default Relative Tempo            */
extern UCHAR  ucGraduation;             /* Default Graduation                */
extern UCHAR  ucMidiPerMetro;           /* Default MIDI per Metro            */

/* Variables for MIDI File processing                                        */
extern CHAR   szSelectedFile[20];       /* Name of selected file from Dialog */
extern TRKCONTROL    *pTrkChunk;        /* pointer to Track Chunk Control    */

extern USHORT usSMFType;                /* Header Chunk data: SMF type       */
extern USHORT usNumTrkChunks;           /* Header Chunk data: Track Chunks   */
extern USHORT usTicksPerQuarter;        /* Header Chunk data: Ticks p. Quart.*/


/* TEST ONLY for GetMidiEvent function.. output to listbox */
extern UCHAR         ucMidiStatus;
extern UCHAR         ucMidiVal1;
extern UCHAR         ucMidiVal2;

UCHAR  ucInitCnt;                       /* Initcounter for CHANCHANGE DLG    */

/*
*******************************************************************************
* Aufruf     : MRESULT  EXPENTRY   MainDlgProc(hWnd, Msg, mp1, mp2)
*
* Eingabe    : HWND       hWnd     Handle des Dialoges
*              ULONG      msg      Nachricht
*              MPARAM     mp1      Nachrichtenparameter 1
*              MPARAM     mp2      Nachrichtenparameter 1
*
* Ausgabe    : MRESULT
*
* Beschreib. : Diese Prozedur verarbeitet die Nachrichten fÅr das Haupt-
*              dialogfenster. Aufruf bei REC/PLAY aus Menueleiste 'Funktionen'
*
*******************************************************************************
*/

MRESULT  EXPENTRY   MainDlgProc(HWND       hWnd,
                                ULONG      Msg,
                                MPARAM     mp1,
                                MPARAM     mp2)

{
extern PCHAR   pCaptBuf;                /* Record Capture Buffer             */
extern USHORT  usCaptRd;                /* Read Pointer to Capture Buffer    */
extern USHORT  usCaptWr;                /* Write Pointer to Capture Buffer   */

extern PCHAR   pPlayBuf;                /* Play Buffer                       */
extern USHORT  usPlayRd;                /* Read Pointer to Play Buffer       */
extern USHORT  usPlayWr;                /* Write Pointer to Play Buffer      */
extern UCHAR   ucPlayRunStat;           /* Runningstatus for Playback        */

extern UCHAR   MPU_MODE;
extern HAB     hAB;                     /* Anchor Block Handle               */

extern UCHAR   ucRecordTrk;             /* Record track                      */

CHAR   DlgText[45];                     /* Dialogbox-Text                    */
APIRET rc;                              /* Returncode Variable               */
INT    i,k;                             /* help counter                      */
UCHAR  ucActHelpIdx;                    /* Help index for track activation   */
UCHAR  ucChanOld;                       /* Help for MIDI Channel changer     */
USHORT usLoop;                          /* Help variable for loop            */

switch (Msg)
    {
    case WM_INITDLG:                    /* INIT Dialog Controls              */
       ucRecordTrk=0;                   /* init: 1st track selected          */
       WinSendDlgItemMsg(hWnd,          /* Check Radiobutton 0 as default    */
                         DID_MD_RAD1,
                         BM_SETCHECK,
                         MPFROMSHORT(1),
                         0L);
       WinSendDlgItemMsg(hWnd,          /* Check Default Timebase            */
                         DID_MD_TIME4,  /* Radiobutton                       */
                         BM_SETCHECK,                                          
                         MPFROMSHORT(1),                                       
                         0L);                                        
       WinSendDlgItemMsg(hWnd,          /* Init Progress scrollbar           */
                         DID_SB_TIME,          
                         SBM_SETSCROLLBAR,     
                         MPFROMSHORT(0),     
                         MPFROM2SHORT(0,1000));
          
/*     for(i=0;i<8;i++)                    INIT Spinbutton Controls
          {
          WinSendDlgItemMsg(hWnd,          
                            DID_MD_SPIN1+i,          ÅberflÅssig geworden
                            SPBM_SETARRAY,
                            MPFROMP(pszChan),
                            MPFROMLONG(16));
          WinSendDlgItemMsg(hWnd,
                            DID_MD_SPIN1+i,
                            SPBM_SETCURRENTVALUE,
                            MPFROMLONG(0),
                            MPFROMLONG(0));
          sprintf(DlgText,"%5d",WrkBufMap[i].ulNumMidiEvents);
          WinSetDlgItemText(hWnd,               init Size textfields
                            DID_MD_SIZE1+i,
                            DlgText);
          WinSendDlgItemMsg(hWnd,               init active checkbuttons
                            DID_MD_ACT1+i,
                            BM_SETCHECK,   
                            MPFROMCHAR(WrkBufMap[i].ucTrkPlayStatus),
                            0L);           
          }                                                                  */

       WinSendDlgItemMsg(hWnd,                    /* Slider Ticks settings   */
                         DID_SL_TEMPO,
                         SLM_SETTICKSIZE,
                         MPFROM2SHORT(0,5),       /* IMPORTANT: SET SLIDERS  */
                         NULL);                   /* TO DEFAULT POSITION !!! */
       WinSendDlgItemMsg(hWnd,
                         DID_SL_TEMPO,     
                         SLM_SETTICKSIZE,  
                         MPFROM2SHORT(24,5),
                         NULL);           
       WinSendDlgItemMsg(hWnd,
                         DID_SL_FADER,
                         SLM_SETTICKSIZE,  
                         MPFROM2SHORT(0,5),
                         NULL);
       /* Set the Slider arm positions to default values                     */
       WinSendDlgItemMsg(hWnd,                    /* TEMPO                   */
                         DID_SL_TEMPO,                                         
                         SLM_SETSLIDERINFO,
                         MPFROM2SHORT(SMA_SLIDERARMPOSITION,
                                      SMA_INCREMENTVALUE),
                         MPFROMSHORT((USHORT)ucTempo/2));
       WinSendDlgItemMsg(hWnd,                    /* FADER                   */
                         DID_SL_FADER,
                         SLM_SETSLIDERINFO,                                    
                         MPFROM2SHORT(SMA_SLIDERARMPOSITION,                   
                                      SMA_INCREMENTVALUE),                     
                         MPFROMSHORT((USHORT)119));
       WinSendDlgItemMsg(hWnd,                    /* RELATIVE TEMPO          */
                         DID_SL_RELTEMP,
                         SLM_SETSLIDERINFO,                                    
                         MPFROM2SHORT(SMA_SLIDERARMPOSITION,                   
                                      SMA_INCREMENTVALUE),                     
                         MPFROMSHORT((USHORT)ucRelTempo/2));

       /* Set the MPU relevant Checkbutton states to default values          */
       WinSendDlgItemMsg(hWnd,                    /* DEFAULT MPU state       */
                         DID_MD_BENDER,
                         BM_SETCHECK,
                         MPFROMSHORT((USHORT)ucBender),
                         NULL);             
       WinSendDlgItemMsg(hWnd,                    /* DEFAULT MPU state       */
                         DID_MD_MIDITHRU,
                         BM_SETCHECK,                                          
                         MPFROMSHORT((USHORT)ucMidiThru),
                         NULL);                                                
       WinSendDlgItemMsg(hWnd,                    /* DEFAULT MPU state       */
                         DID_MD_MEASEND,
                         BM_SETCHECK,                                          
                         MPFROMSHORT((USHORT)ucMeasEnd),
                         NULL);                                                
       WinSendDlgItemMsg(hWnd,                    /* DEFAULT MPU state       */
                         DID_MD_EXCL2HOST,
                         BM_SETCHECK,                                          
                         MPFROMSHORT((USHORT)ucExcl2Host),
                         NULL);   
       hWndMainDialog=hWnd;             /* Store Handle for other purposes   */
/*T*/  ulAccDriverTime=0;               /* init accum. driver absolute time  */
    break;

    case WM_COMMAND:                    /* Drucktasten senden WM_COMMAND     */
        switch (SHORT1FROMMP(mp1))      /* MP1 EnthÑlt die ID des Controls   */
            {
            case DID_MD_RECORD:/********** R E C O R D  Pushbutton ***********/
            /* START RECORD:    **********************************************/
            /* This function passes the Capture Buffer Address and the size  */
            /* of the capture buffer to the device driver and enables the    */
            /* interrupt handler to copy the bytes received from the MPU to  */
            /* the Capture Buffer in turnaround mode.                        */
            /* This function runs until STOP-RECORD IOCTL Command is sent to */
            /* the driver.                                                   */
            /* The application is responsible for fetching data from capture */
            /* buffer before turnarounds occur.                              */
            /*****************************************************************/

            ulAccuTiming=0;             /* Init Accumulated Timer to 0       */
            usCaptRd=1;                 /* Init Read Pointer to Capture Buf. */
            rc=DeleteAllTrackExtents(ucRecordTrk); /* del all track extents  */
            if (rc>0)                                              
               {                                                   
               Display_Msg((USHORT)rc,hWnd,"Fehler in DelTrkExtents");
               }                                                   
            rc=MPU_Start_Rec(CAPTURE_SIZE,pCaptBuf);
            MPU_MODE=Record_Mode;
            sprintf(DlgText,"Record started for track %2d",ucRecordTrk);
            WriteListBox(hWnd,
                         DID_MD_LISTBOX,
                         DlgText);
            if (rc>0)
               {
               Display_Msg((USHORT)rc,hWnd,"Fehler in START REC");
               MPU_MODE=Inact;
               }
            WinEnableControl(hWnd,      /* Disable Play Button               */
                             DID_MD_PLAY,
                             FALSE);
            WinEnableControl(hWnd,      /* Disable RECORD Button             */
                             DID_MD_RECORD,
                             FALSE);
            WinEnableControl(hWnd,      /* Disable Overdub Button            */
                             DID_MD_OVERDUB,                                   
                             FALSE);

            ulRecTimerId=WinStartTimer(hAB,  /* Start Data Fetch timer       */
                          hWnd,                                                
                          ID_REC_TIMER, /* Record timer ID                   */
                          500);         /* Interval 500 ms                   */

            sprintf(DlgText,"Rectimer gestartet mit id %8d",ulRecTimerId);
            WriteListBox(hWnd,          
                         DID_MD_LISTBOX,
                         DlgText);      
            break;

            case DID_MD_STOP:/************ STOP Pushbutton *******************/
            /* STOP:          ************************************************/
            /* This function checks, if RECORD or PLAY is active and stops   */
            /* the appropriate function.                                     */
            /* Important: If RECORD is stopped, the last bytes written       */
            /*            between last timer event and stop record must be   */
            /*            copied to the Workbuffer to prevent data loss!!!   */
            /* This is done by sending another timer event AFTER sending the */
            /* STOP Record command to the MPU and after stopping the timer.  */
            /* This will cause another call to GetLatestCapturedMidiEvents.  */
            /*****************************************************************/
            WinEnableControl(hWnd,      /* reenable Play Button              */
                             DID_MD_PLAY,                                   
                             TRUE);               
            WinEnableControl(hWnd,      /* reenable Record Button            */
                             DID_MD_RECORD,                                 
                             TRUE);   
            WinEnableControl(hWnd,      /* reenable Overdub Button           */
                             DID_MD_OVERDUB,
                             TRUE);                                            

            if (MPU_MODE==Record_Mode)  /***** S T O P   R E C O R D *********/
               {
               WinStopTimer(hAB,        /* STOP Record Timer                 */
                            hWnd,
                            ulRecTimerId); /* Record timer ID                */
               Display_Msg((USHORT)rc,hWnd,"rc von STOP REC TIMER");
               MPU_MODE=Inact;          /* Int Hdlr, size must be +1!!!!!    */
               rc=MPU_Stop_Rec();
               if (rc>0)
                  {
                  Display_Msg((USHORT)rc,hWnd,"Fehler in STOP REC");
                  }     
                                                       
               WinPostMsg(hWnd,         /* read rest of capturebuffer !!     */
                          WM_TIMER,     /* kleiner beep am ende!!            */
                          MPFROM2SHORT(ID_REC_TIMER,0),
                          MPFROM2SHORT(0,0));

               sprintf(DlgText,"Recording stopped for track %2d",ucRecordTrk);
               WriteListBox(hWnd,          
                            DID_MD_LISTBOX,
                            DlgText);      
               }

/* Achtung bei STOP Play: Thread THR_Play... setzt bei seiner automatischen  */
/* Beendigung auch MPU_MODE zurÅck. Das bewirkt, da· aber kein STOP Button   */
/* mehr arbeitet, so da· auch der Timer nicht beendet werden kann            */
/* Daher das zurÅcksetzen auf inact abgestellt. D.h. erst der STOP Button    */
/* setzt auch den MPU_MODE wieder auf inact zurÅck.                          */

            if (MPU_MODE==Play_Mode)    /***** S T O P   P L A Y B A C K *****/
               {
               WinStopTimer(hAB,        /* STOP Level Display Timer          */
                            hWnd,
                            ulLevTimerID);
               if(ucStartPlayTimerFlag==1)
                  {
                  rc=WinStopTimer(hAB,  /* STOP Playbacktimer                */
                                  hWnd,
                                  ulPlayTimerId);
                  Display_Msg((USHORT)rc,hWnd,"rc von STOP PLAY TIMER");
                  }
               rc=MPU_Stop_Play();
               if (rc>0)                /* rc also 0 if Play was not active  */
                  {                     /* see MPU_Stop_Play function        */
                  Display_Msg((USHORT)rc,hWnd,"Fehler in STOP PLAY");
                  }
               MPU_MODE=Inact;  
/*             for(i=0;i<SEQ_TRACKS;i++)        reset status to active
                  {
                  if(WrkBufMap[i].ucTrkPlayStatus==2)
                     WrkBufMap[i].ucTrkPlayStatus=1;
                  }              
               MPU_SetActiveTrk();         Set driver active mask            */
               }

            if (MPU_MODE==Overdub_Mode) /***** S T O P   O V E R D U B *******/
               {                                                               
               if(ucStartPlayTimerFlag==1)                                     
                  {                                                            
                  rc=WinStopTimer(hAB,  /* STOP Playbacktimer                */
                                  hWnd,                                        
                                  ulPlayTimerId);                              
                  Display_Msg((USHORT)rc,hWnd,"rc von STOP PLAY TIMER");       
                  }                  
               WinStopTimer(hAB,        /* STOP Record Timer                 */
                            hWnd,                                              
                            ulRecTimerId); /* Record timer ID                */

               rc=MPU_Stop_Overdub();
               if (rc>0)
                  {     
                  Display_Msg((USHORT)rc,hWnd,"Fehler in STOP OVERDUB");
                  }                             
 
               WinPostMsg(hWnd,         /* read rest of capturebuffer !!     */
                          WM_TIMER,
                          MPFROM2SHORT(ID_REC_TIMER,0),                        
                          MPFROM2SHORT(0,0));                                  
                                                                               
               sprintf(DlgText,"overdub stopped for track %2d",ucRecordTrk);
               WriteListBox(hWnd,          
                            DID_MD_LISTBOX,
                            DlgText);      
               MPU_MODE=Inact;                       
/*             for(i=0;i<SEQ_TRACKS;i++)        reset status to active
                  {                                                            
                  if(WrkBufMap[i].ucTrkPlayStatus==2)     temp. inact.
                     WrkBufMap[i].ucTrkPlayStatus=1;                           
                  }             
               MPU_SetActiveTrk();                                           */
               }                                                               

            break;

            case DID_MD_PLAY:/************ Play Pushbutton pressed ***********/
            /* Start PLAY:    ************************************************/
            /* Playback Buffer already allocated at program start            */
            /* During this test phase, the play buffer will be filled from   */
            /* a test workbuffer, which has no specific format like the      */
            /* final workbuffer design. The format is only like the capture  */
            /* buffer, i.e. data sent from MPU. Data is converted to PLAY    */
            /* buffer format by using GetCaptureBufferEntry, which will later*/
            /* be used to transfer from Capture Buffer to Work Buffer.       */
            /* The application is responsible for copying data to the PLAY   */
            /* buffer in turnaround mode, which will be taken by the drivers */
            /* playback function and sent to the MPU.                        */
            /*****************************************************************/

            *(pPlayBuf)=8;              /* initial offset = 8 !!!            */
            *(pPlayBuf+1)=0;
            usPlayWr=8;                 /* initial fill offset               */
            usPlayRd=8;                 /* for initial fill only             */
            ucStartPlayTimerFlag=1;     /* initial: timer needed             */
            /* runstat=0: avoids, that BuildMPUPlayData will find running    */
            /* status at first event to copy to playbuffer. This would cause */
            /* errors. 0 does not exist as status in MIDI.                   */
            ucPlayRunStat=0;            /* init: runstat to 0                */
            /* Initialize the index values for the Working Buffer to the     */
            /* values at which Playback will start: here: 0                  */

            SetPlayStartPoint((ULONG)0);          /* PLAYBACK Start Time     */

            MPU_SetActiveTrk();         /* Set driver active track mask      */
 
            ResetNoteTable(); /* TEST */

            BuildDispatchList();        /* Build Playback dispatch list      */

            /* Fill Playbuffer: zusÑtzlich rc fÅr KEINE DATEN ???            */
            rc=FillPlayBuffer();        /* Initial Filling of Playbuffer     */
            if(rc==11)                  /* If all MIDI events fit into the   */
               ucStartPlayTimerFlag=0;  /* Playbuffer: Timer not needed      */

            MPU_MODE=Play_Mode;
            rc=MPU_Playback();          /*  Start Playback Thread            */
            if(rc!=0)                   /* rc from DosCreateThread           */
               {                      
               Display_Msg((USHORT)rc,hWnd,"PLAY: Fehler in DosCreateThread");
               MPU_MODE=Inact;
               }
            sprintf(DlgText,"MPU_Playback started, rc=%4d",
                            rc);
            WriteListBox(hWnd,          
                         DID_MD_LISTBOX,
                         DlgText);      
            WinEnableControl(hWnd,           /* Disable Record Button        */
                             DID_MD_RECORD,
                             FALSE);
            WinEnableControl(hWnd,           /* Disable Play Button          */
                             DID_MD_PLAY,                                      
                             FALSE);
            WinEnableControl(hWnd,           /* Disable Overdub Button       */
                             DID_MD_OVERDUB,
                             FALSE);                                           
            if(ucStartPlayTimerFlag==1)
               {
               ulPlayTimerId=WinStartTimer(hAB,
                                           hWnd,
                                           ID_PLAY_TIMER,
                                           2000);
               sprintf(DlgText,"Playtimer gestartet. ID=%8d",ulPlayTimerId);
               WriteListBox(hWnd,          
                            DID_MD_LISTBOX,
                            DlgText);      
               }
            ulLevTimerID=WinStartTimer(hAB,  
                                       hWnd,
                                       ID_LEV_TIMER,                                
                                       200);
            break;

            case DID_MD_OVERDUB: /******** Overdub Pushbutton pressed ********/
            /* Start Overdub: ************************************************/
            /* First: PREPARE PLAYBUFFER and PLAYBACK init                   */
            *(pPlayBuf)=8;              /* initial offset = 8 !!!            */
            *(pPlayBuf+1)=0;                                                   
            usPlayWr=8;                 /* initial fill offset               */
            usPlayRd=8;                 /* for initial fill only             */
            ucStartPlayTimerFlag=1;     /* initial: timer needed             */
            /* runstat=0: avoids, that BuildMPUPlayData will find running    */
            /* status at first event to copy to playbuffer. This would cause */
            /* errors. 0 does not exist as status in MIDI.                   */
            ucPlayRunStat=0;            /* init: runstat to 0                */
            /* Initialize the index values for the Working Buffer to the     */
            /* values at which Playback will start: here: 0                  */

            SetPlayStartPoint((ULONG)0);          /* PLAYBACK Start Time     */
 
            MPU_SetActiveTrk();         /* Set driver active track mask      */

            BuildDispatchList();        /* Build Playback dispatch list      */

            /* Fill Playbuffer: zusÑtzlich rc fÅr KEINE DATEN ???            */
            rc=FillPlayBuffer();        /* Initial Filling of Playbuffer     */
            if(rc==11)                  /* If all MIDI events fit into the   */
               ucStartPlayTimerFlag=0;  /* Playbuffer: Timer not needed      */

            /* NOW PREPARE RECORDING                                         */

            ulAccuTiming=0;             /* Init Accumulated Timer to 0       */
            usCaptRd=1;                 /* Init Read Pointer to Capture Buf. */
            rc=DeleteAllTrackExtents(ucRecordTrk); /* del all track extents  */
            if (rc>0)                                                          
               {                                                               
               Display_Msg((USHORT)rc,hWnd,"Fehler in DelTrkExtents");         
               }                                                               

            MPU_MODE=Overdub_Mode;
            rc=MPU_Overdub();           /*  Start Overdub  Thread            */
            if(rc!=0)                   /* rc from DosCreateThread           */
               {                                                               
               Display_Msg((USHORT)rc,hWnd,"PLAY: Fehler in DosCreateThread"); 
               MPU_MODE=Inact;                                                 
               }                                                               
            sprintf(DlgText,"MPU_Overdub started",
                             0);        
            WriteListBox(hWnd,          
                         DID_MD_LISTBOX,
                         DlgText);  
            WinEnableControl(hWnd,           /* Disable Record Button        */
                             DID_MD_RECORD,                                    
                             FALSE);                                           
            WinEnableControl(hWnd,           /* Disable Play Button          */
                             DID_MD_PLAY,                                      
                             FALSE);                                           
            WinEnableControl(hWnd,           /* Disable Overdub Button       */
                             DID_MD_OVERDUB,                                   
                             FALSE);                                           
            if(ucStartPlayTimerFlag==1)                                     
               {                                                            
               ulPlayTimerId=WinStartTimer(hAB,                             
                                           hWnd,                            
                                           ID_PLAY_TIMER,                   
                                           2000);                           
               sprintf(DlgText,"Playtimer gestartet. ID=%8d",ulPlayTimerId);
               WriteListBox(hWnd,          
                            DID_MD_LISTBOX,
                            DlgText);      
               }                                                            

            ulRecTimerId=WinStartTimer(hAB,  /* Start Data Fetch timer       */
                          hWnd,                                                
                          ID_REC_TIMER, /* Record timer ID                   */
                          500);         /* Interval 500 ms                   */
                                                                               
            sprintf(DlgText,"Rectimer gestartet mit id %8d",ulRecTimerId);
            WriteListBox(hWnd,           
                         DID_MD_LISTBOX, 
                         DlgText);       
            break;

            case DID_MD_STORE:          /* increase rel tempo                */

            break;

            case DID_MD_DELETE:         /* Test: decrease rel tempo          */

            break;                                                             

            case DID_MD_CLEARLIST:      /* Listbox Clear Button pressed      */
               WinSendDlgItemMsg(hWnd,  /* Clear Listbox contents            */
                                 DID_MD_LISTBOX,                               
                                 LM_DELETEALL,
                                 0,
                                 0);
            break;

            case DID_MD_LOADFILE:       /* LOAD MIDIFILE Dialog              */
               for(usLoop=0;usLoop<SEQ_TRACKS;usLoop++)     /* aufrÑumen     */
                  {
                  rc=DeleteAllTrackExtents((UCHAR)usLoop);
                  }
               InitChannelTable();      /* Reset channel table               */
               rc=LoadMidiFile();       /* call loader function              */
               UpdateTrkList();         /* Update container with Tracklist   */
               for(usLoop=0;usLoop<8;usLoop++)    /* UPDATE Size fields      */
                  {
                  sprintf(DlgText,"%5d",
                          WrkBufMap[usLoop].ulNumMidiEvents);
                  WinSetDlgItemText(hWnd,                             
                                    DID_MD_SIZE1+usLoop,
                                    DlgText);   
                  WrkBufMap[usLoop].ucTrkPlayStatus=0;
                  WinSendDlgItemMsg(hWnd,               /* update active but */
                                    DID_MD_ACT1+usLoop,
                                    BM_SETCHECK,                                       
                             MPFROMCHAR(WrkBufMap[usLoop].ucTrkPlayStatus),
                                    0L);                                               
                  }
               MPU_SetActiveTrk();      /* Update driver active mask         */
               MPU_ChannelChange();     /* Update driver channel info        */

               for(usLoop=0;usLoop<usNumTrkChunks;usLoop++)
                  {                                        
                  sprintf(DlgText,"TrkChunk: %2d length: %4x",
                          usLoop,
                          (pTrkChunk+usLoop)->ulTrkLength);
                  WriteListBox(hWnd,          
                               DID_MD_LISTBOX,
                               DlgText);      
                  strcpy(DlgText,(pTrkChunk+usLoop)->szTrkName);
                  WriteListBox(hWnd,          
                               DID_MD_LISTBOX,
                               DlgText);      
                  }
                  rc=Free_Buffer((PVOID)pTrkChunk);
                  if (rc>0)
                     {
                     Display_Msg((USHORT)rc,hWnd,
                                 "Fehler in FREE TRKCHUNK BUFFER");
                     }
            break;                                                             

            case DID_MD_RECFILTER:  /* Set Recording Filters                 */
            WinDlgBox(HWND_DESKTOP, /* Eltern-Fenster                        */
                 hWnd,              /* Besitzer-Fenster                      */
                 FiltDlgProc,       /* Channelmapping Dialog procedure       */
                 (HMODULE)0UL,      /* DLL-Handle (falls Res. dort)          */
                 ID_DLG_RECFILTER,  /* ID of Record filter dialog window     */
                 NULL);             /* Zeiger auf Daten, die mitgegeben      */
                                    /* werden kînnen                         */
            break;

            case DID_MD_UPDATECHAN:     /* Update Midi Channels for playback */
            WinDlgBox(HWND_DESKTOP, /* Eltern-Fenster                        */
                 hWnd,              /* Besitzer-Fenster                      */
                 ChanDlgProc,       /* Channelmapping Dialog procedure       */
                 (HMODULE)0UL,      /* DLL-Handle (falls Res. dort)          */
                 ID_DLG_CHANNELMAP, /* ID of channelmapper dialog window     */
                 NULL);             /* Zeiger auf Daten, die mitgegeben      */
                                    /* werden kînnen                         */
            break;                                                             

            default:
                return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
            }
    break;

    case WM_CONTROL:                    /* other controls send WM_CONTROL    */
        switch (SHORT1FROMMP(mp1))      /* MP1 EnthÑlt die ID des Controls   */ 
            {
            case DID_MD_RAD1:           /* Radiobutton for Track selection   */
            case DID_MD_RAD2:           /* was pressed                       */
            case DID_MD_RAD3:           /* Calculate Track-Number from       */
            case DID_MD_RAD4:           /* Button ID                         */
            case DID_MD_RAD5:
            case DID_MD_RAD6:
            case DID_MD_RAD7:
            case DID_MD_RAD8:
               ucRecordTrk=(UCHAR)(SHORT1FROMMP(mp1))-DID_MD_RAD1;
            break;                                                             

            case DID_MD_TIME1:          /* Radiobutton for Timebase          */
            case DID_MD_TIME2:          /* was pressed                       */
            case DID_MD_TIME3:          /* Calculate Timebase Index from     */
            case DID_MD_TIME4:          /* Button ID                         */
            case DID_MD_TIME5:
            case DID_MD_TIME6:
            case DID_MD_TIME7:
               rc=MPU_Set_Timebase((UCHAR)(SHORT1FROMMP(mp1))-TIME_BASE);
               sprintf(DlgText,"Timebase=%3d, rc=%4x",
                                SHORT1FROMMP(mp1),rc);
               WriteListBox(hWnd,          
                            DID_MD_LISTBOX,
                            DlgText);      
            break;                                                             

            case DID_SB_TIME:           /* Scrollbar for PLAY progress       */
               DosBeep(1000,50);



            break;

            case DID_MD_TST1:           /* Checkbutton for Test 1            */
               ucCommand=0x90;          /* Note on channel 1                 */
               ucPitch=0x50;
               ucVelocity=0x7F;
               ucChannel=0;
               MPU_Send_Data(ucCommand,           /* SEND NOTE ON 50h        */
                             ucPitch,   
                             ucVelocity,
                             ucChannel);
            break;

            case DID_MD_TST2:           /* Checkbutton for Test 2            */
               ucCommand=0x90;          /* Note on channel 1                 */
               ucPitch=0x50;                                                   
               ucVelocity=0x00;         /* vel 0                             */
               ucChannel=0;                                                    
          /*   MPU_Send_Data(ucCommand,              SEND NOTE OFF 50h
                             ucPitch,                                    
                             ucVelocity,                                 
                             ucChannel);               */

/*T*/          CreateTrkList();

            break;                                                             

            case DID_MD_TST3:           /* Checkbutton for Test 3            */
               ucRelTempo=0xFF;         /* change relative tempo             */
               rc=MPU_Set_Rel_Tempo(ucRelTempo);  /* SET relative tempo      */
               if(rc!=0)
                  {
                  Display_Msg((USHORT)rc,hWnd,
                               "MPU_Set_Rel_Tempo() error");
                  }
            break;

            case DID_MD_BENDER:         /* Checkbutton PITCH BEND on/off     */
               ucBender=1-ucBender;     /* Toggle Mode                       */
               rc=MPU_Bender(ucBender); /**Enable/Disable Bender to Host *****/
               if (rc>0)                                                              
                  {                                                                   
                  Display_Msg((USHORT)rc,hWnd,"Fehler in Set BENDER");  
                  ucBender=1-ucBender;  /* reset Mode if error               */
                  break;                                                              
                  }                                                                   
            break;

            case DID_MD_MIDITHRU:       /* Checkbutton MIDI Thru  on/off     */
               ucMidiThru=1-ucMidiThru; /* Toggle Mode                       */
               rc=MPU_MidiThru(ucMidiThru);
               if (rc>0)
                  {
                  Display_Msg((USHORT)rc,hWnd,"Fehler in Set MidiThru");
                  ucMidiThru=1-ucMidiThru;
                  break;
                  }
            break;

            case DID_MD_MEASEND:        /* Checkbutton MeasureEnd on/off     */
               ucMeasEnd=1-ucMeasEnd;   /* Toggle Mode                       */
               rc=MPU_MeasEnd(ucMeasEnd);
               if (rc>0)
                  {
                  Display_Msg((USHORT)rc,hWnd,"Fehler in Set Measure End");
                  ucMeasEnd=1-ucMeasEnd;
                  break;
                  }
            break;                                                             

            case DID_MD_EXCL2HOST:      /* Checkbutton Exclusive  on/off     */
               ucExcl2Host=1-ucExcl2Host;  /* Toggle Mode                    */
               rc=MPU_Excl_To_Host(ucExcl2Host);
               if (rc>0)                                                       
                  {                                                            
                  Display_Msg((USHORT)rc,hWnd,"Fehler in Set Excl2Host");
                  ucExcl2Host=1-ucExcl2Host;
                  break;                                                       
                  }                                                            
            break;                                                             

            case DID_SL_FADER:          /* WM_CONTROL came from Fader Slider */
               /* Check 2nd SHORT value for notifycode in a second switch    */
               /* case construct                                             */
               switch(SHORT2FROMMP(mp1))
                  {
                  case SLN_CHANGE:      /* Slider changed position           */
                  case SLN_SLIDERTRACK:
                     ulSlFaderValue=(ULONG) WinSendDlgItemMsg(hWnd,
                                    DID_SL_FADER,
                                    SLM_QUERYSLIDERINFO,
                                    MPFROM2SHORT(SMA_SLIDERARMPOSITION,
                                                 SMA_INCREMENTVALUE),
                                    NULL);
                     for(i=0;i<64;i++)                                      
                        ucFader[i]=(UCHAR)ulSlFaderValue*2;
                     rc=MPU_Fader();                                        
                     sprintf(DlgText,"Faderwert=%3d, rc=%4x",ucFader[0],rc);
                     WriteListBox(hWnd,          
                                  DID_MD_LISTBOX,
                                  DlgText);      
                  break;

                  default:              /* IMPORTANT: do not forget default  */
                     return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
                  }
            break;

            case DID_SL_TEMPO:          /* WM_CONTROL came from Tempo Slider */
               /* Check 2nd SHORT value for notifycode in a second switch    */
               /* case construct                                             */
               switch(SHORT2FROMMP(mp1))                                       
                  {                                                            
                  case SLN_CHANGE:      /* Slider changed position           */
                  case SLN_SLIDERTRACK:
                     ulSlTempoValue=(ULONG) WinSendDlgItemMsg(hWnd,
                                    DID_SL_TEMPO,
                                    SLM_QUERYSLIDERINFO,                       
                                    MPFROM2SHORT(SMA_SLIDERARMPOSITION,        
                                                 SMA_INCREMENTVALUE),          
                                    NULL)*2;
                     rc=MPU_Set_Tempo((UCHAR)ulSlTempoValue);
                     sprintf(DlgText,"TEMPO=%3d,rc=%2d",
                             ulSlTempoValue,rc);
                     WriteListBox(hWnd,          
                                  DID_MD_LISTBOX,
                                  DlgText);      
                  break;                                                       
                                                                               
                  default:              /* IMPORTANT: do not forget default  */
                     return (WinDefDlgProc(hWnd, Msg, mp1, mp2));              
                  }                                                            
            break;                                                             

            case DID_SL_RELTEMP:        /* WM_CONTROL Relative Tempo Slider  */
               /* Check 2nd SHORT value for notifycode in a second switch    */
               /* case construct                                             */
               switch(SHORT2FROMMP(mp1))                                       
                  {                                                            
                  case SLN_CHANGE:      /* Slider changed position           */
                  case SLN_SLIDERTRACK:
                     ulSlRelTmpValue=(ULONG) WinSendDlgItemMsg(hWnd,
                                    DID_SL_RELTEMP,
                                    SLM_QUERYSLIDERINFO,                       
                                    MPFROM2SHORT(SMA_SLIDERARMPOSITION,        
                                                 SMA_INCREMENTVALUE),          
                                    NULL)*2;                                   
                     rc=MPU_Set_Rel_Tempo((UCHAR)ulSlRelTmpValue);
                     sprintf(DlgText,"Rel. TEMPO=%3d,rc=%2d",
                             ulSlRelTmpValue,rc);
                     WriteListBox(hWnd,          
                                  DID_MD_LISTBOX,
                                  DlgText);      
                  break;                                                       
                                                                               
                  default:              /* IMPORTANT: do not forget default  */
                     return (WinDefDlgProc(hWnd, Msg, mp1, mp2));              
                  }                                                            
            break;                                                             

            default:
                return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
            }                                          /* Ende Switch mp1    */
    break;

    case WM_TIMER:                      /* Timer Message                     */
        switch (SHORT1FROMMP(mp1))      /* MP1 contains ID of timer          */
            {
            case ID_REC_TIMER:/*********** RECORD TIMER EVENT ****************/
            /* Copy new MIDI events since last timer event to the working    */
            /* buffer in the Working Buffer Format. Copy is done by          */
            /* GetCaptureBufferEntry() which also does overflow processing.  */
            /* All COMPLETE!! Midi events added to Capture buffer by the     */
            /* driver to the capture buffer will be copied to the Working    */
            /* Buffer. If an event is incomplete: usCaptRd will be set to    */
            /* start offset of this incomplete MIDI Event, so that at next   */
            /* Timer Message this event will be copied.                      */
            /* usCaptWr: Offset of last byte written by interrupt handler    */
            /*           contained in bytes 1+2 of Capture Buffer            */
            /* usCaptRd: Read Pointer, containing last offset read from      */
            /*****************************************************************/
         /* usCaptWr=*pCaptBuf+256*(*(pCaptBuf+1));        get last offset   */
         /*                                                as snapshot       */

            usCaptWr=*((USHORT*)pCaptBuf);        /* last offset as snapshot */
            DosBeep(6000,20);

            if(usCaptRd!=usCaptWr)                /* if offsets differ       */
               {
               rc=GetLatestCapturedMidiEvents(ucRecordTrk); /*  Midi Data    */
               if((rc!=0)&&(rc!=10))              /* into track selected     */
                  {
                  Display_Msg((USHORT)rc,hWnd,
                               "GetLatestCapturedMidiEvents() error");
                  }
               sprintf(DlgText,"%5d",             /* UPDATE Size of recdata  */
                       WrkBufMap[ucRecordTrk].ulNumMidiEvents);
               WinSetDlgItemText(hWnd,                    
                                 DID_MD_SIZE1+ucRecordTrk,
                                 DlgText);                
               }
            break;

            case ID_PLAY_TIMER: /********* PLAYBACK TIMER EVENT **************/
            /* Copy new MIDI events since last timer event to the playback   */
            /* buffer in the MPU playable format.  Copy is done by           */
            /* FillPlayBuffer() which also does overflow processing.         */
            /* MIDI events are only added to the playback buffer, if the     */
            /* COMPLETE event can be placed in the play-buffer.              */
            /* If there's not enough free space: FillPlayBuffer returns and  */
            /* this data will be filled in at next timer event.              */
            /* usPlayWr: Offset at which FillPlayBuffer will write the next  */
            /*           byte to the Playback Buffer.                        */
            /* usPlayRd: Offset, at which the Playback Thread will read      */
            /*           next data from Playback Buffer.                     */
            /*****************************************************************/
         /* usPlayRd=*pPlayBuf+256*(*(pPlayBuf+1));        get next offset   */
         /*                                                as snapshot       */
                             
            usPlayRd=*((USHORT*)pPlayBuf);        /* next offset as snapshot */
            DosBeep(6000,20);

            if(usPlayRd!=usPlayWr)                /* if offsets differ       */
               {                                                               
               ulAccDriverTime=*((ULONG*)(pPlayBuf+2));
               sprintf(DlgText,"ACCU Drivertime: %8d",
                                ulAccDriverTime);                            
               WriteListBox(hWnd,                                     
                            DID_MD_LISTBOX,                           
                            DlgText);                                 
               rc=FillPlayBuffer();               /* Fill playbuffer         */
               /* returncode 10 is o.k., means that data didn't match,       */
               /* rc=11 means, that FC end has been copied to the Buffer,    */
               /* Timer can be stopped now. What happens at further calls to */
               /* FillPlayBuffer??  ERRORS??                                 */
               if((rc!=0)&&(rc!=10)&&(rc!=11))
                  {                                                            
                  Display_Msg((USHORT)rc,hWnd,                                 
                               "FillPlayBuffer() error");
                  }
               if(rc==10)               /* usPlayRd erreicht beim schreiben  */
                  {
                  sprintf(DlgText,"usPlayRd beim kopieren erreicht: %5d",
                                   usPlayRd);
                  WriteListBox(hWnd,
                               DID_MD_LISTBOX,
                               DlgText);
                  }
               if(rc==11)               /* all data now copied               */
                  {
                  sprintf(DlgText,"FC end copied, no more playoverflow",0);
                  WriteListBox(hWnd,
                               DID_MD_LISTBOX,
                               DlgText);
 
                  /* hier kînnte der Timer nun auch gestoppt werden          */
 
                  rc=WinStopTimer(hAB,     /* STOP Playbacktimer             */
                                  hWnd,                                           
                                  ulPlayTimerId);                                 
                  Display_Msg((USHORT)rc,hWnd,"rc von STOP PLAY TIMER");          

                  }                                                      
               }                                                               
            break;                                                             

            case ID_LEV_TIMER: /********** Level Timer Event *****************/
            /* Update Infos for Playback position                            */
            /*****************************************************************/

               sScrollbarPos= (SHORT) ((*((ULONG*)(pPlayBuf+2))*1000)
                                       /ulTotalMaxTime);

               WinSendDlgItemMsg(hWnd,            /* Update scrollbar pos.   */
                                 DID_SB_TIME,
                                 SBM_SETPOS,
                                 MPFROMSHORT(sScrollbarPos),
                                 0L);

               sprintf(DlgText,"Time: %6d",*((ULONG*)(pPlayBuf+2)));
               WinSetDlgItemText(hWnd,
                                 DID_TXT_TIME,
                                 DlgText);                               

            break;

            default:
                return (WinDefDlgProc(hWnd, Msg, mp1, mp2));  
            }
    break;

    case WM_HSCROLL:                    /* Horizontal scroll                 */
       switch (SHORT1FROMMP(mp1))       /* MP1 contains ID of scrollbar      */
            {
            case DID_SB_TIME:
      /*       DosBeep(2000,100);
               WinSendDlgItemMsg(hWnd,             
                                 DID_SB_TIME,
                                 SBM_SETPOS,
                                 MPFROMSHORT(100),
                                 0L);                      */

               sprintf(DlgText,"SCROLLBAREVENT",0);
               WriteListBox(hWnd,                                       
                            DID_MD_LISTBOX,                             
                            DlgText);                                   
            break;

            default:
               return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
            break;
            }
    break;

    default:
        return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
    }
return ((MRESULT)0L);
}

/*
*******************************************************************************
* Aufruf     : MRESULT  EXPENTRY   VersDlgProc(hWnd, Msg, mp1, mp2)
*
* Eingabe    : HWND       hWnd     Handle des Demo-Dialoges
*              ULONG      msg      Nachricht
*              MPARAM     mp1      Nachrichtenparameter 1
*              MPARAM     mp2      Nachrichtenparameter 2
*
* Ausgabe    : MRESULT
*
* Beschreib. : Diese Prozedur dient dazu, die Versions und Releasedaten der
*              MPU Karte sowie des MPU Treibers anzuzeigen.
*              Die auszugebenden Daten werden in der Struktur MPU_Vers vom
*              Type VDATA erwartet, d.h. vor Aufruf mu· ein entprechender
*              Command an die MPU gesendet worden sein.
*              Funktion: MPU_GetVer in MIDIMPU1.C.
*
*******************************************************************************
*/
 
MRESULT  EXPENTRY   VersDlgProc(HWND       hWnd,
                                ULONG      Msg,
                                MPARAM     mp1,
                                MPARAM     mp2)
 
{
SHORT usHiddenIndex = 1;                /* Index auf Hidden Layer            */
CHAR MPUVersion[10];                    /* Strings for Output                */
CHAR MPURevision[10];
CHAR DRVVersion[10];
CHAR DRVRevision[10];

extern VDATA   MPU_Vers;                /* MPU and Driver Version structure  */

switch (Msg)
    {
    case WM_INITDLG:                    /* INIT Dialog Controls              */
       sprintf(MPUVersion,"%d.%d",MPU_Vers.ucVers_MPU_high,
                                  MPU_Vers.ucVers_MPU_low);
       sprintf(MPURevision,"%d.%d",MPU_Vers.ucRev_MPU_high,
                                   MPU_Vers.ucRev_MPU_low);
       sprintf(DRVVersion,"%d",MPU_Vers.ucVers_DRV);
       sprintf(DRVRevision,"%d",MPU_Vers.ucRev_DRV);

       WinSetDlgItemText(hWnd,
                         DID_RV_MPUVERS,
                         MPUVersion);
       WinSetDlgItemText(hWnd,          
                         DID_RV_MPUREV,
                         MPURevision);
       WinSetDlgItemText(hWnd,          
                         DID_RV_DRVVERS,
                         DRVVersion);
       WinSetDlgItemText(hWnd,          
                         DID_RV_DRVREV,
                         DRVRevision);
       break;
 
    case WM_COMMAND:                    /* Drucktasten senden WM_COMMAND     */
        switch (SHORT1FROMMP(mp1))      /* MP1 EnthÑlt die ID des Controls   */
            {
            default:
                return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
            }
        break;
 
    case WM_CONTROL:                    /* öbrige Controls senden WM_CONTROL */
        break;
 
    default:
        return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
    }
return ((MRESULT)0L);
}

/*                                                                             
*******************************************************************************
* Aufruf     : MRESULT  EXPENTRY   ChanDlgProc(hWnd, Msg, mp1, mp2)
*                                                                              
* Parameters : HWND       hWnd     Handle des Demo-Dialoges
*              ULONG      msg      Nachricht                                   
*              MPARAM     mp1      Nachrichtenparameter 1                      
*              MPARAM     mp2      Nachrichtenparameter 2                      
*                                                                              
* Return     : MRESULT
*                                                                              
* Description: This procedure is used to assign a channel change matrix to
*              the playback function. Each  actual channel can be changed
*              to a new value by using the spinbuttons. These changes take
*              effect immediately during playback.
*                                                                              
*******************************************************************************
*/                                                                             
                                                                               
MRESULT  EXPENTRY   ChanDlgProc(HWND       hWnd,
                                ULONG      Msg,                                
                                MPARAM     mp1,                                
                                MPARAM     mp2)                                
                                                                               
{                                                                              
UCHAR  i;                               /* loop variable                     */
CHAR   szSpinTxt[5];                    /* Spinbutton content field          */
UCHAR  ucSpinValue;                     /* Read Spinbutton value             */
USHORT usSpinId;                        /* Id of spinbutton                  */
CHAR   DlgText[45];                     /* local Textvariable for testoutput */
APIRET rc;                              /* returncode from MPU command       */

switch (Msg)                                                                   
    {                                                                          
    case WM_INITDLG:                    /* INIT Dialog Controls              */
       ucInitCnt=32;                    /* 16 Messages to skip               */
       for(i=0;i<16;i++)                /* INIT Spinbutton Controls          */
          {                             /* to current Channeltable settings  */
          WinSendDlgItemMsg(hWnd,
                            DID_CM_SPIN1+i,
                            SPBM_SETARRAY,
                            MPFROMP(pszChan),
                            MPFROMLONG(17));
          WinSendDlgItemMsg(hWnd,       /* Spinbutton reflects CURRENT chn.  */
                            DID_CM_SPIN1+i,
                            SPBM_SETCURRENTVALUE,
                            MPFROMLONG((ULONG)(ucChanTab[i])),   /* INDEX!!  */
                            MPFROMLONG(0));
          }
    break;                                                                  
                                                                               
    case WM_COMMAND:                    /* Pushbuttons send   WM_COMMAND     */
        switch (SHORT1FROMMP(mp1))      /* MP1 contains the ID of Control    */
            {                                                                  
            default:                                                           
               return (WinDefDlgProc(hWnd, Msg, mp1, mp2));                   
            }                                                                  
    break;                                                                 
                                                                               
    case WM_CONTROL:                    /* Other controls send    WM_CONTROL */
        usSpinId=SHORT1FROMMP(mp1);
        switch(usSpinId)                /* MP1 contains ID of control        */
           {
           case DID_CM_SPIN1:           /* Spinbutton has been changed       */
           case DID_CM_SPIN2:           /* Action: UPDATE Channeltable and   */
           case DID_CM_SPIN3:           /* send to MPU                       */
           case DID_CM_SPIN4:
           case DID_CM_SPIN5:
           case DID_CM_SPIN6:
           case DID_CM_SPIN7:
           case DID_CM_SPIN8:
           case DID_CM_SPIN9:
           case DID_CM_SPIN10:
           case DID_CM_SPIN11:
           case DID_CM_SPIN12:
           case DID_CM_SPIN13:
           case DID_CM_SPIN14:
           case DID_CM_SPIN15:
           case DID_CM_SPIN16:
              /* TRICK: the WM_INITDLG processing also causes SPBN_CHANGE    */
              /* Messages to be sent. These cause errors, so suppress the    */
              /* first 32 Messages sent.                                     */
              if(ucInitCnt!=0)
                 {
                 ucInitCnt--;
                 return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
                 }
              else                      /* if no message from INIT           */
                 {                      /* process                           */
                 switch(SHORT2FROMMP(mp1))
                    {
                    case SPBN_CHANGE:   /* Spin field value changed          */
                       WinSendDlgItemMsg(hWnd,
                                         (ULONG)usSpinId,
                                         SPBM_QUERYVALUE,
                                         MPFROMP((PVOID)szSpinTxt),
                                         MPFROM2SHORT(4,SPBQ_UPDATEIFVALID));
                       ucChanTab[usSpinId-DID_CM_SPIN1]=atoi(szSpinTxt)-1;
                       rc=MPU_ChannelChange();
                       if(rc!=0)
                          Display_Msg((USHORT)rc,hWnd,
                                      "rc von MPU_ChannelChange");
                    break;

                    default:
                       return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
                    }                   /* end of switch short2frommp(mp1)   */
                 }                      /* end of else                       */
           break;                       /* end of case DID_CM_SPINx          */
           default:
              return (WinDefDlgProc(hWnd, Msg, mp1, mp2)); 
           }                            /* end of switch short1frommp(mp1)   */
    break;                              /* end of CASE WM_CONTROL            */
                                                                               
    default:                                                                   
        return (WinDefDlgProc(hWnd, Msg, mp1, mp2));                           
    }                                   /* end of switch(Msg)                */
return ((MRESULT)0L);                                                          
}                        

/*                                                                             
*******************************************************************************                                                      
* Call       : MRESULT  EXPENTRY   FiltDlgProc(hWnd, Msg, mp1, mp2)
*                                                                              
* Parameter  : HWND       hWnd     Handle of Filter Dialog
*              ULONG      msg      message
*              MPARAM     mp1      message parameter 1
*              MPARAM     mp2      message parameter 2
*                                                                              
* Return     : MRESULT
*                                                                              
* Description: This procedure is used to set the record filters
*******************************************************************************
*/                                                                             
                                                                               
MRESULT  EXPENTRY   FiltDlgProc(HWND       hWnd,
                                ULONG      Msg,                                
                                MPARAM     mp1,                                
                                MPARAM     mp2)                                
                                                                               
{                                                                              
switch (Msg)                                                                   
    {                                                                          
    case WM_INITDLG:                    /* INIT Dialog Controls              */
       /* Code for INITIALIZATION                                            */
    break;                              /* end of case INITDLG               */
                                                                               
    case WM_COMMAND:                    /* Pushbuttons send WM_COMMAND       */
        switch (SHORT1FROMMP(mp1))      /* MP1 EnthÑlt die ID des Controls   */
            {                                                                  
            default:                                                           
                return (WinDefDlgProc(hWnd, Msg, mp1, mp2));                   
            }                                                                  
        break;                                                                 
                                                                               
    case WM_CONTROL:                    /* other controls send WM_CONTROL    */
        break;                                                                 
                                                                               
    default:                                                                   
        return (WinDefDlgProc(hWnd, Msg, mp1, mp2));                           
    }                                                                          
return ((MRESULT)0L);                                                          
}                                


