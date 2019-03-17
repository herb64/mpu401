/*
*******************************************************************************
* Quelle:       MIDISEQ.H
* Programm:     MIDISEQ.EXE
*------------------------------------------------------------------------------
* Beschreibung: allg. Header-File fÅr MIDISEQ.EXE
*
*------------------------------------------------------------------------------
*
*
*******************************************************************************
*/

/*---------------------------------------------------------------------------*/
/* Allgemeine Define Statements                                              */
/*---------------------------------------------------------------------------*/
#define BOOL char                       /* Boolean-Variablen-Typ             */

/* definitions for GetCaptureBufferEntry function                            */
#define MA_NEW       1
#define MA_TIMING    2

#define SEQ_TRACKS   64                 /* Max. Number of Sequencer Tracks   */
#define TRKNAME_MAXLEN 30               /* Maximum length of Track name      */
#define INST_MAXLEN    20               /* Maximum length of Instrument name */

/*---------------------------------------------------------------------------*/
/* Fenster-Klassen                                                           */
/*---------------------------------------------------------------------------*/
#define  PWC_CLIENT_A  "Clientclass A"  /* Classname for main window         */
#define  PWC_THREAD2   "Thread2class "

/*---------------------------------------------------------------------------*/
/* Fenster-ID's                                                              */
/*---------------------------------------------------------------------------*/
#define  ID_FRAME_A         10          /* ID of Frame-Window A              */
#define  ID_POPUP_TRK       20          /* ID of POPUP Menue Tracklist cont. */

/*---------------------------------------------------------------------------*/
/* Konstanten fÅr Message Dialogbox                                          */
/*---------------------------------------------------------------------------*/
#define  ERROR               1          /*  ERROR                            */
#define  WARNING             2          /*  WARNING                          */
#define  INFO                3          /*  INFORMATION                      */

/*---------------------------------------------------------------------------*/
/* Konstanten fÅr MPU_MODE Variable                                          */
/*---------------------------------------------------------------------------*/
#define  Inact               0
#define  Record_Mode         2
#define  Play_Mode           1
#define  Overdub_Mode        3

/*---------------------------------------------------------------------------*/
/* Konstanten fÅr MPU401 Driver IOCtl Command Codes                          */
/*---------------------------------------------------------------------------*/
#define  Category          128          /* Category code for MPU Driver      */
#define  RESET_MPU           0          /* Reset MPU                         */
#define  UART_Mode           1          /* Set MPU to UART Mode              */
#define  GET_VERSION         2          /* Get MPU and Driver Version        */
#define  STOP_PLAY           3          /* Stop PLAYBACK                     */
#define  BENDER              4          /* Enable/disable Bender to Host     */
#define  PLAY                5          /* Playback                          */
#define  START_REC           6          /* Start Recording                   */
#define  STOP_REC            7          /* Stop Recording                    */
#define  MEAS_END            8          /* Enable/disable MEAS END to Host   */
#define  SET_TIMEBASE        9          /* Set MPU Timebase value            */
#define  EXCL_TO_HOST       10          /* Enable/disable Exclusive to Host  */
#define  REALTIME_AFF       11          /* Enable/disable Realtime affection */
#define  MIDI_THRU          12          /* Enable/disable MIDI Thru          */
#define  FADER              13          /* Set fader values for playback     */
#define  START_OVERDUB      14          /* Start Overdub function            */
#define  STOP_OVERDUB       15          /* Stop Overdub function             */
#define  SEND_DATA          16          /* MPU Want to send data             */
#define  SET_TEMPO          17          /* MPU Set Tempo command             */
#define  SET_REL_TEMPO      18          /* MPU Set Relative Tempo command    */
#define  SET_GRADUATION     19          /* MPU Set Graduation command        */
#define  MIDI_PER_METRO     20          /* MPU Set MIDI per METRO command    */
#define  MIDI_CHANNEL_TAB   21          /* Set MIDI channel mapper table     */
#define  SET_ACTIVE_TRACKS  22          /* Set Active tracks (0..63)         */
#define  GDT_NOTETAB        23          /* GDT Mapping of Notetable          */
#define  FREE_NOTETAB       24          /* Free Notetable                    */

/*---------------------------------------------------------------------------*/
/* Constants for META Events                                                 */
/*---------------------------------------------------------------------------*/

#define  MET_SEQNO           0          /* Sequence Number                   */
#define  MET_GENTEXT         1          /* General Text                      */
#define  MET_COPYRIGHT       2          /* Copyright Text                    */
#define  MET_TRKNAME         3          /* Track Name                        */
#define  MET_INSTRUMENT      4          /* Instrument Name                   */
#define  MET_SONGTEXT        5          /* SONG text                         */
#define  MET_MARK            6          /* Marker                            */
#define  MET_CUEPOINT        7          /* CUE point                         */
#define  MET_CHANPREFIX     32          /* Channel prefix                    */
#define  MET_ENDOFTRACK     47          /* End of Track                      */
#define  MET_SETTEMPO       81          /* Set Tempo                         */
#define  MET_SMPTE          84          /* SMPTE offset                      */
#define  MET_TIMESIG        88          /* Time signature                    */
#define  MET_SEQUENCER     127          /* Sequencer specific                */

/*---------------------------------------------------------------------------*/
/* Buffer size constants                                                     */
/*---------------------------------------------------------------------------*/

#define  CAPTURE_SIZE     4096          /* Size of Record CAPTURE Buffer     */
#define  PLAY_SIZE        16384         /* Size of Playback Buffer (FIX!!!)  */
#define  NOTETAB_SIZE     32768         /* Size of Notetable segment         */

/*---------------------------------------------------------------------------*/
/* Constants for user defined PM messages                                    */
/*---------------------------------------------------------------------------*/

#define  MSG_START_REC     WM_USER+1;   /* Message to start recording        */
#define  MSG_STOP_REC      WM_USER+2;   /* Message to stop recording         */
#define  MSG_START_PLAY    WM_USER+3;   /* Message to start playback         */
#define  MSG_STOP_PLAY     WM_USER+4;   /* Message to stop playback          */
#define  MSG_LEVEL_SET     WM_USER+5;   /* Message to set level              */

/*---------------------------------------------------------------------------*/ 
/* Constants for menue bar main window                                       */
/*---------------------------------------------------------------------------*/ 

#define  MID_FILE          100          /* File-Submenu                      */
#define  MID_NEW           101
#define  MID_OPEN          102
#define  MID_SAVE          103
#define  MID_SAVEAS        104
#define  MID_EXIT          105
                               
#define  MID_FUNCS         110          /* Functions Submenu                 */
#define  MID_GETVER        111
#define  MID_METRONOME     112
#define  MID_RECPLAY       113

#define  MID_HELP          120          /* Help Submenu                      */
#define  MID_HELPHELP      121
#define  MID_HELPEXT       122
#define  MID_HELPKEYS      123
#define  MID_HELPINDEX     124

#define  SID_QUERYEXIT    1000 

#define  ID_REC_TIMER      333          /* Timer ID for Record Function      */
#define  ID_PLAY_TIMER     334          /* Timer ID for Playback Function    */
#define  ID_LEV_TIMER      335          /* Timer ID for Level Display Update */
                               
/*---------------------------------------------------------------------------*/
/* Definitions of POPUP Menue constants for Tracklist popup                  */
/*---------------------------------------------------------------------------*/
#define  MID_TL_MIXER     1100
#define  MID_TL_NOTE      1101
#define  MID_TL_PIANO     1102
#define  MID_TL_EVENTLIST 1103

/*---------------------------------------------------------------------------*/
/* Definitions of Mixer Bitmaps for LEVEL display                            */
/*---------------------------------------------------------------------------*/
#define  ID_BMP_LEV0       900
#define  ID_BMP_LEV1       901
#define  ID_BMP_LEV2       902
#define  ID_BMP_LEV3       903
#define  ID_BMP_LEV4       904
#define  ID_BMP_LEV5       905
#define  ID_BMP_LEV6       906
#define  ID_BMP_LEV7       907
#define  ID_BMP_LEV8       908
#define  ID_BMP_LEV9       909
#define  ID_BMP_LEV10      910
#define  ID_BMP_LEV11      911
#define  ID_BMP_LEV12      912
#define  ID_BMP_LEV13      913
#define  ID_BMP_LEV14      914
#define  ID_BMP_LEV15      915
#define  ID_BMP_LEV16      916

/*---------------------------------------------------------------------------*/
/* Deklaration von Variablen und Typendefinitionen                           */
/*---------------------------------------------------------------------------*/

typedef struct                          /* Structure for Version Data        */
   {
   UCHAR ucVers_MPU_high;               /* MPU Version higher part           */
   UCHAR ucVers_MPU_low;                /* MPU Version lower part            */
   UCHAR ucRev_MPU_high;                /* MPU Revision higher part          */
   UCHAR ucRev_MPU_low;                 /* MPU Revision lower part           */
   UCHAR ucVers_DRV;                    /* Driver Version                    */
   UCHAR ucRev_DRV;                     /* Driver Revision                   */
   } VDATA;

typedef struct                          /* Arguments for Playthread          */
   {
   INT   BufSize;                       /* Buffersize for Playbuffer         */
   PVOID pMidiBuf;                      /* Bufferaddress for Playbuffer      */
   UCHAR TrackMask;                     /* Active Track Mask                 */
   } PLARG;

/*---------------------------------------------------------------------------*/
/* Working Buffer Definitionen   (12.06.94   13.7.94  1.11.94)               */
/* The Working Buffer is dynamically allocated.                              */
/* For each of the 64 Tracks a structure of type WRKBUF exists. This entry   */
/* contains pointers to up to 256 EXTENTS. Each extent is one page in size,  */
/* containing 409 events of type TRKDATA.                                    */
/*---------------------------------------------------------------------------*/

typedef struct                          /* This is the format of an Entry    */
   {                                    /* in a Working Buffer extent.       */
   ULONG      ulTiming;                 /* Each Entry has a size of 10 bytes */
   USHORT     usNextEventIdx;           /* The Event-Entries are connected   */
   UCHAR      ucNextExtentIdx;          /* by indicex to a chain.            */
   UCHAR      ucMidiStatus;             /* This chaining is possible over    */
   UCHAR      ucData1;                  /* several EXTENT boundaries         */
   UCHAR      ucData2;                  /* 409 of those entries are placed   */
   } TRKDATA;                           /* in one extent.                    */

typedef TRKDATA *PTRKDATA;

typedef struct                          /* This is the structure of data     */
   {                                    /* needed to manage a track.         */
   PTRKDATA   pTrkExtent[256];          /* For each of the 64 tracks, this   */
   ULONG      ulNumMidiEvents;
   ULONG      ulMaxTiming;
   USHORT     usNextExtentToAllocate;   /* structure of type WRKBUF is       */
   USHORT     usActExtentIdx;           /* allocated. This structure is used */
   USHORT     usActEventIdx;            /* as a map through the working      */
   CHAR       chTrkName[TRKNAME_MAXLEN];/* buffer.                           */
   CHAR       chInstrument[INST_MAXLEN];
   UCHAR      ucMidiChannel;            /* 1..16: Chn FFh: Mult. 0: no midi  */
   UCHAR      ucTrkPlayStatus;          /* Playback-Status for track (0,1,2) */
   UCHAR      ucPriority;               /* Track Priority for FillPlayBuffer */
   } WRKBUF;

/*---------------------------------------------------------------------------*/
/* Definitions for MIDI File specific Structures                             */
/* These Structures are used to store information from MIDI Files            */
/* Important: Word and Doublewords are not stored in the INTEL Format, i.e.  */
/*            conversion is needed. That's why the structures do not use     */
/*            USHORT and ULONG, because this would result in errors.         */
/*---------------------------------------------------------------------------*/

typedef struct                          /* HEADER CHUNK DATA                 */
   {
   CHAR       ucHeader[4];              /* MThd..                            */
   UCHAR      ucHeadLength[4];          /* Header Chunk length               */
   UCHAR      ucSMFType[2];             /* Type of MIDI File                 */
   UCHAR      ucNumTrkChunks[2];        /* Number of Track Chunks            */
   UCHAR      ucTicksPerQuarter[2];     /* Ticks per Quarter Note            */
   } HDCHUNKDATA;

/*---------------------------------------------------------------------------*/
/* An array of type TRKCONTROL is dynamically allocated with an index of the */
/* number of found Track-Chunks in the file. This is the main working        */
/* structure for the MIDI File. It contains all data which is needed to      */
/* work with the MIDI File.                                                  */
/*---------------------------------------------------------------------------*/

typedef struct                          /* TRACK CHUNK DATA (array type)     */
   {
   CHAR       *pTrkChunkStart;          /* Startaddress of TrkChunk data     */
   ULONG      ulTrkLength;              /* Length of Track Data              */
   ULONG      ulTempo;                  /* META Event  81 Set Tempo          */
   CHAR       szTrkName[TRKNAME_MAXLEN];/* META Event   3 Track Name         */
   CHAR       szInstrument[INST_MAXLEN];/* META Event   4 Instrument Name    */
   UCHAR      ucTimeSig1;
   UCHAR      ucTimeSig2;
   UCHAR      ucMidiFlag;               /* Flag, if MIDI Events contained    */
                                        /* or only META Events               */
   } TRKCONTROL;                        /* may be more than 64 chunks !!!    */

/*---------------------------------------------------------------------------*/
/* Funktions-Prototypen                                                      */
/*---------------------------------------------------------------------------*/

/* Source-Datei: *************************************** MIDISEQ.C  **********/ 
void   main(void);
MRESULT  EXPENTRY   MainWinProc (HWND, ULONG, MPARAM, MPARAM);
void _Optlink SecondThread(void *);          /* A R B E I T S P F E R D ???  */
MRESULT  EXPENTRY   Thr2WinProc (HWND, ULONG, MPARAM, MPARAM);

/* Source-Datei: *************************************** MIDIDLG1.C **********/
MRESULT  EXPENTRY   MainDlgProc (HWND, ULONG, MPARAM, MPARAM);
MRESULT  EXPENTRY   VersDlgProc (HWND, ULONG, MPARAM, MPARAM);
MRESULT  EXPENTRY   ChanDlgProc (HWND, ULONG, MPARAM, MPARAM);
MRESULT  EXPENTRY   FiltDlgProc (HWND, ULONG, MPARAM, MPARAM);
MRESULT  EXPENTRY   MixDlgProc  (HWND, ULONG, MPARAM, MPARAM);
/*       USHORT     ReadFile(CHAR *, USHORT, CHAR *);              */
/*       USHORT     WriteFile(CHAR *, USHORT, CHAR *);             */
/*       PVOID      AllocMem(ULONG);                               */
/*       VOID       DisplayMsg(SHORT, CHAR *, LONG);               */
/*       VOID       GetErrInf(HWND, CHAR *);                       */

/* Source-Datei: *************************************** MIDIMPU1.C **********/
APIRET MPU_Start_Rec(USHORT,PCHAR);     /* Start recording                   */
APIRET MPU_Reset(void);                 /* Reset MPU                         */
APIRET MPU_Open(void);                  /* Open MPU device driver            */
APIRET MPU_GetVer(PVOID);               /* Get Driver and MPU Version        */
APIRET MPU_MeasEnd(UCHAR);
APIRET MPU_Bender(UCHAR);               /* Enable/Disable Pitch Bend         */
APIRET MPU_MidiThru(UCHAR);             /* Enable/Disable MIDI Thru          */
APIRET MPU_Stop_Rec(VOID);              /* Start recording                   */
APIRET MPU_Playback(VOID);              /* Start Playback                    */
APIRET MPU_Stop_Play(VOID);             /* Stop Playback                     */
APIRET THR_Playback(ULONG);             /* Thread, started by MPU_Playback() */
APIRET MPU_Fader(void);                 /* Set fader values for playback     */
APIRET MPU_Overdub(VOID);               /* MPU overdub function              */
APIRET MPU_Stop_Overdub(VOID);          /* Set fader values for playback     */
APIRET THR_Overdub(ULONG);              /* Thread, started by MPU_Overdub()  */
APIRET MPU_Send_Data(UCHAR,UCHAR,UCHAR, /* MPU Want to send data command     */
                     UCHAR);
APIRET MPU_Set_Tempo(UCHAR);            /* MPU Set Tempo                     */
APIRET MPU_Set_Rel_Tempo(UCHAR);        /* MPU Set relative Tempo            */
APIRET MPU_Set_Graduation(UCHAR);       /* MPU Set Graduation                */
APIRET MPU_Midi_Per_Metro(UCHAR);       /* MPU Set MIDI per Metro            */
APIRET MPU_Set_Timebase(UCHAR);         /* MPU Set Timebase                  */
APIRET MPU_Excl_To_Host(UCHAR);         /* MPU Set Exclusive to Host on/off  */
APIRET MPU_Realtime_Aff(UCHAR);         /* MPU Set Realtime Affection on/off */
APIRET MPU_ChannelChange(VOID);         /* Set MIDI Channel table            */
APIRET MPU_SetActiveTrk(void);          /* Set MIDI Active Track mask        */
APIRET MPU_GDT_Notetable(void);         /* Map Notetable to GDT Selector     */
APIRET MPU_Free_Notetable(void);        /* Free Notetable from driver access */

/* Source-Datei: *************************************** MIDIHLP1.C **********/
APIRET Alloc_Buffer(ULONG,PCHAR *);          /* Allocate Buffer              */
APIRET Free_Buffer(PVOID);                   /* Free Memory Object           */
void   Display_Msg(USHORT,HWND,CHAR*);       /* Display a message box        */
APIRET GetLatestCapturedMidiEvents(UCHAR);   /* Get latest captured events   */
APIRET AppendMidiEvent(UCHAR,UCHAR,UCHAR,UCHAR,UCHAR);
APIRET AnalyzeCapturedMidiEvent(UCHAR*,UCHAR*,UCHAR*,UCHAR*,UCHAR*);
APIRET InitWrkBufMap(void);                  /* Init Workbuffer admin struct.*/
UCHAR  CreateWrkBufExtent(UCHAR);            /* Create an Workbufferextent   */
APIRET DeleteAllTrackExtents(UCHAR);         /* Delete all extents of a trk  */
APIRET DeleteSpecificTrackExtent(UCHAR,USHORT);  /* Del spec. ext. of a trk  */
APIRET FillPlayBuffer(void);
APIRET BuildMPUPlayData(UCHAR,ULONG,UCHAR,UCHAR,UCHAR,UCHAR*);

/* Source-Datei: *************************************** MIDIHLP2.C **********/
APIRET SetTrkMidiChannel(UCHAR,UCHAR);  /* Change MIDI Channel for a track   */
APIRET AbsTime2MidiTime(ULONG,UCHAR,UCHAR,   /* Convert Time abs. to MIDI    */
                        UCHAR,ULONG*);
APIRET MidiTime2AbsTime(ULONG,UCHAR,UCHAR,   /* Convert Time MIDI to Abs.    */
                        UCHAR,ULONG*);
APIRET ExtractMidiTiming(ULONG,ULONG*,       /* Extract MIDI Timing from LONG*/
                         ULONG*,ULONG*);
APIRET FindPartnerNoteOff(UCHAR,UCHAR,       /* Find corresponding note off  */
                          USHORT,UCHAR*,
                          USHORT*,ULONG*);
APIRET WriteListBox(HWND,ULONG,CHAR*);       /* Listbox Output               */
APIRET InitChannelTable();                   /* initialize MIDI Channel Tab  */
APIRET BuildDispatchList(void);              /* Build playback dispatch list */
APIRET UpdateDispatchList(UCHAR);            /* update dispatch list         */
APIRET ResetNoteTable(void);                 /* reset complete note table    */
APIRET ChangePlayTrkStatus(UCHAR,UCHAR);     /* Trk Start/Stop while playing */
APIRET SetPlayStartPoint(ULONG);             /* Set Playback startpoint      */
ULONG GetMaximumTiming(void);                /* Get max. Timing from WRKBUF  */

/* Source-Datei: *************************************** MIDIFIL1.C **********/
APIRET LoadMidiFile(void);                   /* Load MIDI File data          */
APIRET GetHeaderChunkData(PVOID,USHORT*,     /* extract Header chunk data    */
                          USHORT*,USHORT*);
APIRET GetDecodedValue(PCHAR*,ULONG*);       /* Get coded data from CHUNK    */
APIRET GetMetaEvent(PCHAR*,USHORT);          /* analyze META Event           */
APIRET GetMidiEvent(PCHAR*,UCHAR*,           /* Analyze MIDI Event           */
                    UCHAR*,UCHAR*);
APIRET StoreMidiEvent(UCHAR,ULONG,UCHAR,     /* Store MIDI Event to WrkBUF   */
                      UCHAR,UCHAR);
APIRET UpdateMidiChannelInfo(UCHAR,UCHAR);   /* Update WRKBUF MIDI channels  */

/* Source-Datei: *************************************** MIDILOG1.C **********/
APIRET LogOpenFile(void);                    /* Open Log Dataset             */
APIRET LogWriteFile(CHAR*);                  /* Write LOG entry              */
APIRET LogCloseFile(void);                   /* Close Log Dataset            */

/* Source-Datei: *************************************** MIDICNT1.C **********/
APIRET CreateTrkList(void);                  /* Create Tracklist Container   */
/* UCHAR  QueryTLRecordState(PRECORDCORE);      Query record selection state */
APIRET UpdateTrkList(void);                  /* Update Tracklist Container   */
MRESULT EXPENTRY SubclassTLContainer(HWND, ULONG, MPARAM, MPARAM);
