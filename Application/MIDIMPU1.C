/*                                                                             
*******************************************************************************
* Quelle:       MIDIMPU1.C
* Programm:     MIDISEQ.EXE
*------------------------------------------------------------------------------
* Beschreibung: Diese Datei enthÑlt alle Funktionen, die direkt Aufrufe zum
*               MPU401 Treiber machen. Dies sind alle MPU_xxx Funktionen sowie
*               die Playback Thread Funktion.
*                                                                              
*------------------------------------------------------------------------------
* Prozedur(en): THR_Playback       (ULONG)
*               MPU_Start_Rec      (USHORT,PCHAR)
*               MPU_Reset          (void)
*               MPU_Open           (void)
*               MPU_GetVer         (PVOID)
*               MPU_MeasEnd        (UCHAR)
*               MPU_Bender         (UCHAR)
*               MPU_MidiThru       (UCHAR)
*               MPU_Stop_Rec       (VOID)
*               MPU_Playback       (VOID)
*               MPU_Stop_Play      (VOID)
*               MPU_Fader          (VOID)
*               MPU_Overdub
*               THR_Overdub
*               MPU_Stop_Overdub
*               MPU_Send_data      (UCHAR,UCHAR,UCHAR,UCHAR)
*               MPU_Set_Tempo      (UCHAR)
*               MPU_Set_Rel_Tempo  (UCHAR)
*               MPU_Set_Graduation (UCHAR)
*               MPU_Midi_Per_Metro (UCHAR)
*               MPU_Set_Timebase   (UCHAR)
*               MPU_Excl_To_Host   (UCHAR)
*               MPU_Realtime_Aff   (UCHAR)
*               MPU_ChannelChange  (VOID)
*               MPU_SetActiveTrk   (VOID)
*               MPU_GDT_Notetable  (VOID)
*               MPU_Free_Notetable (VOID)
*
*******************************************************************************
*/
 
/*---- Includes -------------------------------------------------------------*/

#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES
#define INCL_BASE

#define   INCL_WINWINDOWMGR      /* zugefÅgt  */
#define   INCL_WINWINDOWMGR                     
#define   INCL_WINBUTTONS                       
#define   INCL_WINENTRYFIELDS                   
#define   INCL_WINDIALOGS                       
#define   INCL_WININPUT                         
#define   INCL_WINLISTBOXES                     
#define   INCL_WINTIMER        

#include <os2.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <bsememf.h>                    /* Memory alloc flags                */

#include "midiseq.h"                    /* Include Definitionen fÅr Projekt  */
#include "mididlg.h"                    /* Include fÅr alle Dialogfenster    */


/*---- Global Variables -----------------------------------------------------*/

HFILE   DrvHandle;                      /* MPU401 Filehandle                 */
VDATA   MPU_Vers;                       /* MPU and Driver Version structure  */

PCHAR   pCaptBuf;                       /* Record: CAPTURE BUFFER Pointer    */
USHORT  usCaptRd;                       /* Read Pointer to Capture Buffer    */
USHORT  usCaptWr;                       /* Write Pointer to Capture Buffer   */

PCHAR   pPlayBuf;                       /* Play: Play Buffer Pointer         */
USHORT  usPlayRd;                       /* Read Pointer to Play Buffer       */
USHORT  usPlayWr;                       /* Write Pointer to Play Buffer      */

PCHAR   pNoteTable[8];                  /* Pointers to Notetable in driver   */

TID     PlayThread;                     /* PLAYBACK Thread Variables         */
PFNTHREAD PlayThAddr;
ULONG   PlayThArg;
ULONG   PlayThFlags;
ULONG   PlayThStack;
PLARG   PlayThArgs;                     /* Argumentstructure for PLAY Thread */

extern HWND    hWndMainDialog;        /* TEST */
CHAR   DlgText[45];
extern WRKBUF WrkBufMap[SEQ_TRACKS];    /* Administration Structure          */

/*---------------------------------------------------------------------------*/
/* Function: THR_Playback()                                                  */
/*---------------------------------------------------------------------------*/
/* Aufgabe: Wiedergabe von MIDI Daten auf MIDI OUT                           */
/* Parameter: ULONG Wert, der einen Zeiger auf eine Struktur mit öbergabe-   */
/*            daten enthÑlt. funktioniert noch nicht, solange wird auf die   */
/*            globalen Daten pCaptBuf zugegriffen.                           */
/* Funktion is started as THREAD !!!                                         */
/*---------------------------------------------------------------------------*/

APIRET THR_Playback(ULONG PlayThAr)
{

extern UCHAR   MPU_MODE;
extern HWND    hWndMainDialog;

APIRET  rc;

ULONG ParmLengthInOut;
ULONG DataLengthInOut;
ULONG DataLengthMax;

/* rc=DosSetPriority(2,3,31,PlayThread);   Set Priority to Time Critical     */

DataLengthInOut=PLAY_SIZE;
DataLengthMax=PLAY_SIZE;
ParmLengthInOut=0;                      /* INIT to 0, otherwise rc=x57 from  */
                                        /* DosDevIOCtl is possible!!!        */
rc=DosDevIOCtl(DrvHandle,                                                    
               Category,                                                     
               PLAY,                    /* Command PLAY                      */
               NULL,                    /* Pointer to Parmlist structure     */
               0,                       /* ParmLengthMax                     */
               &ParmLengthInOut,
               pPlayBuf,                /* Data Area = Play Buffer           */
               DataLengthMax,           /* DataLengthMax                     */
               &DataLengthInOut);

if(rc!=0)
   {
   sprintf(DlgText,"Fehler aus PlayThread Ioctl, rc=%8x",
                    rc);
   WriteListBox(hWndMainDialog,                                    
                DID_MD_LISTBOX,                                    
                DlgText);                                          
   DosBeep(5000,2000);
   DosBeep(3000,300);
   }

/* MPU_MODE=Inact;                         Inact: Stop Play disabled         */

DosExit(EXIT_THREAD,0);                 /* Stop execution of thread          */
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_Start_Rec()                                                 */
/*---------------------------------------------------------------------------*/
/* Aufgabe: Starten der Aufnahme von Midi Daten                              */
/* Parameter: ULONG wert mit Zeiger auf öbergabe-Struktur                    */
/*---------------------------------------------------------------------------*/

APIRET MPU_Start_Rec(USHORT usRecBufsz,PCHAR pRecBuf)
{                                                                            
APIRET  rc;                                                                  
ULONG ParmLengthInOut=0;
ULONG DataLengthInOut;
USHORT ParmList;

ParmList=CAPTURE_SIZE;                  /* Size of Capture Buffer            */
DataLengthInOut=(ULONG)CAPTURE_SIZE;

rc=DosDevIOCtl(DrvHandle,
               Category,
               START_REC,               /* Command Start Record              */
               NULL,                    /* Parameter: NONE                   */
               0,                       /* Length of Parameters              */
               &ParmLengthInOut,
               pRecBuf,                 /* Data Area = allocated Memory      */
               CAPTURE_SIZE,            /* DataLengthMax                     */
               &DataLengthInOut);
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_Reset()                                                     */
/*---------------------------------------------------------------------------*/
/* Aufgabe: Reset der MPU401 Karte                                           */
/* Parameter: keine                                                          */
/* Return:    rc:                                                            */
/*---------------------------------------------------------------------------*/

APIRET MPU_Reset(void)
{                                                                            
APIRET  rc;                                                                  
ULONG ParmLengthInOut=0;
ULONG DataLengthInOut=0;
                          
rc=DosDevIOCtl(DrvHandle,
               Category,
               RESET_MPU,               /* Command Reset MPU                 */
               NULL,                    /* NULL Pointer (parameters)         */
               0,                       /* ParmLengthMax                     */
               &ParmLengthInOut,
               NULL,                    /* NULL Pointer (data)               */
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_Open()                                                      */
/*---------------------------------------------------------------------------*/

APIRET MPU_Open(void)
{
APIRET  rc;
ULONG   ActionTaken; 
ULONG   ulFileAttribute;                /* File Attributes                   */
ULONG   ulOpenFlag;                     /* Open Flag                         */
ULONG   ulOpenMode;                                                          
          
ulFileAttribute=FILE_NORMAL;                                                 
ulOpenFlag=OPEN_ACTION_OPEN_IF_EXISTS;                                       
ulOpenMode=OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE;                 
 
rc=DosOpen("MPU401$",
           &DrvHandle,
           &ActionTaken,                /* Return value                      */
           0L,                          /* Filesize, only for create, 0      */
           ulFileAttribute,
           ulOpenFlag,
           ulOpenMode,
           0L);                         /* No Extended Attributes            */
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_GetVer()                                                    */
/*---------------------------------------------------------------------------*/

APIRET MPU_GetVer(PVOID pData)
{
APIRET  rc;
ULONG ParmLengthInOut=0;
ULONG DataLengthInOut=6;
 
rc=DosDevIOCtl(DrvHandle,
               Category,
               GET_VERSION,             /* Command Get Version               */
               NULL,                    /* Parmlist                          */
               0,                       /* ParmLengthMax                     */
               &ParmLengthInOut,
               pData,                   /* passed Pointer                    */
               6,                       /* max. bytes returned by driver     */
               &DataLengthInOut);       /* In: Number of Bytes passed,       */
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_MeasEnd()                                                   */
/*---------------------------------------------------------------------------*/
 
APIRET MPU_MeasEnd(UCHAR ucFlag)
{
APIRET  rc;
ULONG   ParmLengthInOut=0;
ULONG   DataLengthInOut=0;
CHAR    DlgText[30];

rc=DosDevIOCtl(DrvHandle,                                                    
               Category,
               MEAS_END,                /* Command Measure End               */
               (PVOID)&ucFlag,          /* Parameter: 1=on  0=off            */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,
               NULL,
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);

strcpy(DlgText,"Test, ob das ankommt");
WriteListBox(hWndMainDialog,
             DID_MD_LISTBOX,                                  
             DlgText);                                        

return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_Bender()                                                    */
/*---------------------------------------------------------------------------*/

APIRET MPU_Bender(UCHAR ucFlag)
{
APIRET  rc;
ULONG   ParmLengthInOut=0;
ULONG   DataLengthInOut=0;
 
rc=DosDevIOCtl(DrvHandle,
               Category,
               BENDER,                  /* Command Measure End               */
               (PVOID)&ucFlag,          /* Parameter: 1=on  0=off            */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,
               NULL,
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_MidiThru()                                                  */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_MidiThru(UCHAR ucFlag)
{                                                                              
APIRET  rc;                                                                    
ULONG   ParmLengthInOut=0;                                                     
ULONG   DataLengthInOut=0;                                                     
                                                                               
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               MIDI_THRU,               /* Command MidiThru                  */
               (PVOID)&ucFlag,          /* Parameter: 1=on  0=off            */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                                                           
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
}                                                                              

/*---------------------------------------------------------------------------*/
/* Function: MPU_StopRec()                                                   */
/*---------------------------------------------------------------------------*/
/* Stop Recording                                                            */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_Stop_Rec(VOID)
{
APIRET  rc;
ULONG ParmLengthInOut=0;
ULONG DataLengthInOut=0;

rc=DosDevIOCtl(DrvHandle,
               Category,
               STOP_REC,                /* Command Stop Record               */
               NULL,                    /* Parmlist                          */
               0,                       /* ParmLengthMax                     */
               &ParmLengthInOut,
               NULL,
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_Playback                                                    */
/*---------------------------------------------------------------------------*/
/* Start Playback Thread (Function THR_Playback)                             */
/* öbergabe: Keine explizite öbergabe, in einer Struktur vom Typ 'playargs'  */
/* mÅssen die Puffergrî·e und Adresse sowie die active track mask stehen     */
/* NAME: PLAYTHARGS      (d.h. öber globale variable)                        */
/* Ursache: Parameter ULONG öbergabe an Thread (Adresse) funktioniert nicht  */
/*---------------------------------------------------------------------------*/
 
APIRET MPU_Playback(VOID)
{
APIRET  rc;
PFNTHREAD  PlayThAddr;
ULONG      PlayThArg;
ULONG      PlayThFlags;
ULONG      PlayThStack;

PlayThAddr=(PFNTHREAD)THR_Playback;     /* Thread Function address           */
PlayThArg=(ULONG)&PlayThArgs;           /* GLOBAL Argument structure         */
PlayThFlags=0;                          /* Start Thread immediately          */
PlayThStack=4096;                       /* Stacksize=4096                    */
 
rc=DosCreateThread(&PlayThread,         /* Returnvalue: ThreadID(GLOBAL)     */
                   PlayThAddr,
                   PlayThArg,           /* Arguments Pointer not used!!!     */
                   PlayThFlags,
                   PlayThStack);
return(rc);
}          

/*---------------------------------------------------------------------------*/
/* Function: MPU_Stop_Play                                                   */
/*---------------------------------------------------------------------------*/
/* This function sends a STOP_PLAY Ioctl to the driver. This will only be    */
/* sent, if Play_Mode is active. If not, no command will be sent, and        */
/* rc will be set to 0.                                                      */
/*---------------------------------------------------------------------------*/

APIRET MPU_Stop_Play(VOID)
{
APIRET  rc=0;
ULONG ParmLengthInOut=0;
ULONG DataLengthInOut=0;
extern UCHAR   MPU_MODE;

if(MPU_MODE==Play_Mode)                 /* only call driver if play active   */
   rc=DosDevIOCtl(DrvHandle,
                  Category,
                  STOP_PLAY,            /* Command Stop Playback             */
                  NULL,                 /* Parmlist                          */
                  0,                    /* ParmLengthMax                     */
                  &ParmLengthInOut,
                  NULL,
                  0,                    /* DataLengthMax                     */
                  &DataLengthInOut);
return(rc);
}


/*---------------------------------------------------------------------------*/
/* Function: MPU_Fader()                                                     */
/*---------------------------------------------------------------------------*/
/* This function passes the 64 fader values to the driver, which copies      */
/* them to his working array, which is used during playback.                 */
/*---------------------------------------------------------------------------*/
                          
APIRET MPU_Fader(void)
{                                                                              
extern  UCHAR ucFader[64];              /* Fader value table                 */
 
APIRET  rc;
ULONG   ParmLengthInOut=SEQ_TRACKS;     /* length of 64 bytes                */
ULONG   DataLengthInOut=0;
 
rc=DosDevIOCtl(DrvHandle,
               Category,
               FADER,                   /* Command Set Fader values          */
               (PVOID)ucFader,          /* 64 bytes with fader values        */
               SEQ_TRACKS,              /* ParmLengthMax                     */
               &ParmLengthInOut,
               NULL,
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                          
return(rc);
}         
                                                                     
/*---------------------------------------------------------------------------*/
/* Function: MPU_Overdub()                                                   */
/*---------------------------------------------------------------------------*/
/* This function starts the overdub. The address of the capture buffer is    */
/* passed by ioctl PARM, the address of the play buffer is passed by the     */
/* ioctl DATA.                                                               */
/*---------------------------------------------------------------------------*/
 
APIRET MPU_Overdub(VOID)
{                                                                              
APIRET  rc;                                                                    
PFNTHREAD  PlayThAddr;                                                         
ULONG      PlayThArg;                                                          
ULONG      PlayThFlags;                                                        
ULONG      PlayThStack;                                                        
                                                                               
PlayThAddr=(PFNTHREAD)THR_Overdub;      /* Thread Function address           */
PlayThArg=(ULONG)&PlayThArgs;           /* GLOBAL Argument structure         */
PlayThFlags=0;                          /* Start Thread immediately          */
PlayThStack=4096;                       /* Stacksize=4096                    */
                                                                               
rc=DosCreateThread(&PlayThread,         /* Returnvalue: ThreadID(GLOBAL)     */
                   PlayThAddr,                                                 
                   PlayThArg,           /* Arguments Pointer not used!!!     */
                   PlayThFlags,                                                
                   PlayThStack);                                               
return(rc);                                                                    
}                                                                              

/*---------------------------------------------------------------------------*/
/* Function: THR_Overdub()                                                   */
/*---------------------------------------------------------------------------*/
/* Aufgabe: Wiedergabe von MIDI Daten auf MIDI OUT                           */
/* Parameter: ULONG Wert, der einen Zeiger auf eine Struktur mit öbergabe-   */
/*            daten enthÑlt. funktioniert noch nicht, solange wird auf die   */
/*            globalen Daten pCaptBuf zugegriffen.                           */
/* Function is started as THREAD !!!                                         */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET THR_Overdub(ULONG PlayThAr)
{                                                                              
                                                                               
extern UCHAR   MPU_MODE;                                                       
extern HWND    hWndMainDialog;                                                 
                                                                               
APIRET  rc;                                                                    
                                                                               
ULONG ParmLengthInOut;                                                         
ULONG DataLengthInOut;                                                         
ULONG DataLengthMax;                                                           
                                                                               
rc=DosSetPriority(2,3,31,PlayThread);   /* Set Priority to Time Critical     */

DataLengthInOut=(ULONG)PLAY_SIZE;                                              
ParmLengthInOut=(ULONG)CAPTURE_SIZE;                                           
                                                                               
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               START_OVERDUB,           /* Command Start Overdub             */
               pCaptBuf,                /* Capture buffer address            */
               CAPTURE_SIZE,            /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               pPlayBuf,                /* Play buffer address               */
               PLAY_SIZE,               /* DataLengthMax                     */
               &DataLengthInOut);                                              
                                                            
if(rc!=0)                                                                      
   {                                                     
   sprintf(DlgText,"Fehler aus OverdubThread Ioctl, rc=%8x",
                    rc);                                 
   WriteListBox(hWndMainDialog,                          
                DID_MD_LISTBOX,                          
                DlgText);                                
   DosBeep(5000,2000);
   DosBeep(3000,300);                                    
   }                                                     
                                                                               
/* MPU_MODE=Inact;                         Inact: Stop Play disabled         */
                                                                               
DosExit(EXIT_THREAD,0);                 /* Stop execution of thread          */
}                                                                              
                                                                               

/*---------------------------------------------------------------------------*/
/* Function: MPU_StopOverdub()                                               */
/*---------------------------------------------------------------------------*/
/* Stop Overdub                                                              */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_Stop_Overdub(void)
{                                                                              
APIRET  rc;                                                                    
ULONG ParmLengthInOut=0;                                                       
ULONG DataLengthInOut=0;                                                       
                                                                               
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               STOP_OVERDUB,            /* Command Stop Record               */
               NULL,                    /* Parmlist                          */
               0,                       /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                                                           
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
} 

/*---------------------------------------------------------------------------*/
/* Function: MPU_Send_Data()                                                 */
/* This function is used to send a MIDI event to the MPU without using the   */
/* Playback Buffer. E.G. the Editor uses this function to play a note just   */
/* by mouseklick. The Driver calls WANT TO SEND DATA command.                */
/*---------------------------------------------------------------------------*/
 
APIRET MPU_Send_Data(UCHAR ucCommand,
                     UCHAR ucPitch,
                     UCHAR ucVelocity,
                     UCHAR ucChannel)
{
APIRET  rc;
ULONG   ParmLengthInOut=0;
ULONG   DataLengthInOut=0;
UCHAR   ucParms[4];

ucParms[0]=ucCommand;
ucParms[1]=ucPitch;
ucParms[2]=ucVelocity;
ucParms[3]=ucChannel;

rc=DosDevIOCtl(DrvHandle,
               Category,
               SEND_DATA,               /* Command Send Data                 */
               (PVOID)ucParms,          /* Parameters (Data to send)         */
               4,                       /* ParmLengthMax                     */
               &ParmLengthInOut,
               NULL,                    /* no data                           */
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);
return(rc);
}                                           

/*---------------------------------------------------------------------------*/
/* Function: MPU_Set_Tempo()                                                 */
/* This function is used to set the MPU401's internal clock tempo.           */
/* Parameters: 1 byte tempo                                                  */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_Set_Tempo(UCHAR ucTempo)
{                                                                              
APIRET  rc;                                                                    
ULONG   ParmLengthInOut=0;                                                     
ULONG   DataLengthInOut=0;                                                     
 
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               SET_TEMPO,               /* Command Set Tempo                 */
               (PVOID)&ucTempo,         /* Parameters                        */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                    /* no data                           */
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_Set_Rel_Tempo()                                             */
/* This function is used to set the MPU401's internal clock tempo.           */
/* Parameters: 1 byte tempo                                                  */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_Set_Rel_Tempo(UCHAR ucRelTempo)
{                                                                              
APIRET  rc;                                                                    
ULONG   ParmLengthInOut=0;                                                     
ULONG   DataLengthInOut=0;                                                     
                                                                               
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               SET_REL_TEMPO,           /* Command Set relative Tempo        */
               (PVOID)&ucRelTempo,      /* Parameters                        */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                    /* no data                           */
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
}                    

/*---------------------------------------------------------------------------*/                                                          
/* Function: MPU_Set_Graduation()                                            */
/* This function is used to set the MPU401's internal clock tempo.           */
/* Parameters: 1 byte tempo                                                  */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_Set_Graduation(UCHAR ucGraduation)
{                                                                              
APIRET  rc;                                                                    
ULONG   ParmLengthInOut=0;                                                     
ULONG   DataLengthInOut=0;                                                     
                                                                               
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               SET_GRADUATION,          /* Command Set Graduation            */
               (PVOID)&ucGraduation,    /* Parameters                        */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                    /* no data                           */
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
}                                

/*---------------------------------------------------------------------------*/                                              
/* Function: MPU_Midi_Per_Metro()                                            */
/* This function is used to set the MPU401's internal clock tempo.           */
/* Parameters: 1 byte tempo                                                  */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_Midi_Per_Metro(UCHAR ucMidiPerMetro)
{                                                                              
APIRET  rc;                                                                    
ULONG   ParmLengthInOut=0;                                                     
ULONG   DataLengthInOut=0;                                                     
                                                                               
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               MIDI_PER_METRO,          /* Command Set MIDI per Metro        */
               (PVOID)&ucMidiPerMetro,  /* Parameters                        */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                    /* no data                           */
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
}                                                             

/*---------------------------------------------------------------------------*/                 
/* Function: MPU_Set_Timebase                                                */
/*---------------------------------------------------------------------------*/
/* Parameter: UCHAR ucTimebase, value 1 .. 7, depending on this value, the   */
/*            MPU will set its Timebase. Following values will be set:       */
/*            ucTimebase  ->   1     2     3     4     5     6     7         */
/*            Timebase    ->  48    72    96   120   144   168   192         */
/*            MPU-Command ->  C2    C3    C4    C5    C6    C7    C8         */
/*---------------------------------------------------------------------------*/

APIRET MPU_Set_Timebase(UCHAR ucTimebase)
{                                                                              
APIRET  rc;                                                                    
ULONG   ParmLengthInOut=0;                                                     
ULONG   DataLengthInOut=0;                                                     
                                                                               
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               SET_TIMEBASE,            /* Command Set Timebase              */
               (PVOID)&ucTimebase,      /* Parameter: 1-7                    */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                                                           
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_Excl_To_Host                                                */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_Excl_To_Host(UCHAR ucFlag)
{                                                                              
APIRET  rc;                                                                    
ULONG   ParmLengthInOut=0;                                                     
ULONG   DataLengthInOut=0;                                                     
                                                                               
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               EXCL_TO_HOST,            /* Command Exclusive to Host         */
               (PVOID)&ucFlag,          /* Parameter: 1=on  0=off            */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                                                           
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
}                                                                          

/*---------------------------------------------------------------------------*/    
/* Function: MPU_Realtime_Aff()                                              */
/*---------------------------------------------------------------------------*/
 
APIRET MPU_Realtime_Aff(UCHAR ucFlag)
{
APIRET  rc;
ULONG   ParmLengthInOut=0;
ULONG   DataLengthInOut=0;
 
rc=DosDevIOCtl(DrvHandle,
               Category,
               REALTIME_AFF,            /* Command Realtime Affection        */
               (PVOID)&ucFlag,          /* Parameter: 1=on  0=off            */
               1,                       /* ParmLengthMax                     */
               &ParmLengthInOut,
               NULL,
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);
return(rc);
}
 
/*---------------------------------------------------------------------------*/                      
/* Function: MPU_ChannelChange                                               */
/*---------------------------------------------------------------------------*/
/* This function passes the 64 Channel table values to the driver, which     */
/* them to his working array, which is used during playback for online       */
/* change of the channel.                                                    */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_ChannelChange(void)
{                                                                              
extern  UCHAR ucChanTab[64];            /* Channel table                     */
                                                                               
APIRET  rc;                                                                    
ULONG   ParmLengthInOut=64;             /* length of 64 bytes                */
ULONG   DataLengthInOut=0;                                                     
                                                                               
rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               MIDI_CHANNEL_TAB,        /* Command Set Channel Table         */
               (PVOID)ucChanTab,        /* 64 bytes with fader values        */
               64,                      /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                                                           
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
}                                      

/*---------------------------------------------------------------------------*/                                        
/* Function: MPU_SetActiveTrk()                                              */
/*---------------------------------------------------------------------------*/
/* This function passes the 64 active track indicators to the driver, which  */
/* copies them to his working array, which is used during playback.          */
/* Parameters: none                                                          */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET MPU_SetActiveTrk(void)
{                                                                              
UCHAR   ucActTrk[64];                   /* Working table                     */
UCHAR   i;                              /* loop variable                     */
APIRET  rc;                                                                    
ULONG   ParmLengthInOut=SEQ_TRACKS;     /* length of 64 bytes                */
ULONG   DataLengthInOut=0;                                                     
                        

for(i=0;i<SEQ_TRACKS;i++)               /* copy to array                     */
   {
   ucActTrk[i]=WrkBufMap[i].ucTrkPlayStatus;
   }

rc=DosDevIOCtl(DrvHandle,                                                      
               Category,                                                       
               SET_ACTIVE_TRACKS,       /* Command Set Active tracks         */
               (PVOID)ucActTrk,         /* 64 bytes with track values        */
               SEQ_TRACKS,              /* ParmLengthMax                     */
               &ParmLengthInOut,                                               
               NULL,                                                           
               0,                       /* DataLengthMax                     */
               &DataLengthInOut);                                              
return(rc);                                                                    
}                                                     

/*---------------------------------------------------------------------------*/
/* Function: MPU_GDT_Notetable()                                             */
/*---------------------------------------------------------------------------*/
/* This function makes the notetable accessible to the device driver through */
/* 8 GDT Selectors. The memory is locked. To unlock: MPU_Free_Notetable().   */
/* Call a loop 8 Times, each giving access to a 32kbyte portion for 8 tracks */
/* to each GDT_Selector in the driver.                                       */
/*---------------------------------------------------------------------------*/

APIRET MPU_GDT_Notetable(void)
{                                                                              
APIRET  rc;      
UCHAR   ucIndex;
                                                                               
ULONG ParmLengthInOut;
ULONG DataLengthInOut;                                                         
ULONG DataLengthMax;                                                           

DataLengthInOut=32768;
DataLengthMax=32768;
ParmLengthInOut=1;                      /* One parm byte with index          */

for(ucIndex=0;ucIndex<8;ucIndex++)
   {
   DataLengthInOut=32768;
   DataLengthMax=32768;
   ParmLengthInOut=1;                   /* One parm byte with index          */
   rc=DosDevIOCtl(DrvHandle,
                  Category,
                  GDT_NOTETAB,          /* Command GDT Mapping notetable     */
                  (PVOID)&ucIndex,      /* Pointer to Parmlist structure     */
                  1,                    /* ParmLengthMax                     */
                  &ParmLengthInOut,
                  (PVOID)pNoteTable[ucIndex],  /* Data Area = Note Table     */
                  DataLengthMax,        /* DataLengthMax                     */
                  &DataLengthInOut);
   }
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function: MPU_Free_Notetable()                                            */
/*---------------------------------------------------------------------------*/
/* This function frees the notetable from the access of the driver.          */
/* An unlock is done to the Notetable memory.                                */
/*---------------------------------------------------------------------------*/

APIRET MPU_Free_Notetable(void)
{                                                                              
APIRET  rc,maxrc;
UCHAR   ucIndex;
                                            
ULONG ParmLengthInOut=1;
ULONG DataLengthInOut=0;                                                       
maxrc=0;

for(ucIndex=0;ucIndex<8;ucIndex++)
   {
   rc=DosDevIOCtl(DrvHandle,
                  Category,
                  FREE_NOTETAB,         /* Command Free Notetable            */
                  (PVOID)&ucIndex,      /* Parmlist                          */
                  1,                    /* ParmLengthMax                     */
                  &ParmLengthInOut,
                  NULL,
                  0,                    /* DataLengthMax                     */
                  &DataLengthInOut);
   if(rc>maxrc) maxrc=rc;
   if(rc!=0) DosBeep(2000,100);
   }
return(maxrc);
}                                                                              
