/*
*******************************************************************************
* Quelle:       MIDIHLP2.C
* Programm:     MIDISEQ.EXE
*------------------------------------------------------------------------------
* Prozedur(en): SetTrkMidiChannel()
*               AbsTime2MidiTime()
*               MidiTime2AbsTime()
*               ExtractMidiTiming()
*               MergeSequencerTracks()
*               FindPartnerNoteOff()
*               CalcMidiDeltaTime()
*               WriteListBox()
*               InitChannelTable()
*               BuildDispatchList()
*               UpdateDispatchList()
*               ResetNoteTable()
*               ChangePlayTrkStatus()
*               SetPlayStartPoint()
*               GetMaximumTiming()
*
*******************************************************************************
*/

#define  INCL_WINWINDOWMGR
#define  INCL_WINMESSAGEMGR
#define  INCL_WINFRAMEMGR
#define  INCL_WINSYS
#define  INCL_WINLISTBOXES              /* For WriteListBox() function       */
#define  INCL_WINDIALOGS                /* WriteListBox does not work without*/
                                        /* although no compiler errors.      */
#define  INCL_WININPUT                  /* VK_DOWN etc.                      */
#define  INCL_DOSPROCESS

#include <os2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "midiseq.h"                    /* Include defintions for project    */
#include "mididlg.h"                    /* Include for all dialog-windows    */

/*---------------------------------------------------------------------------*/ 
/* Global variable declarations                                              */
/*---------------------------------------------------------------------------*/ 

extern WRKBUF WrkBufMap[SEQ_TRACKS];    /* Administration Structure          */
extern UCHAR  ucChanTab[64];            /* MIDI Channel Table                */
extern PCHAR  pNoteTable[8];            /* Pointer to Notetable in driver    */
UCHAR  ucDispatchList[65];              /* FillPlayBuffer Dispatcher List    */
UCHAR  ucBitTable[8]={1,2,4,8,16,32,    /* General purpose bittable          */
                      64,128};                                                
extern HWND    hWndMainDialog;          /* Handle of Main Dialog Window      */

/*---------------------------------------------------------------------------*/
/* Function SetTrkMidiChannel()                                              */
/*---------------------------------------------------------------------------*/
/* This function changes the MIDI Events in a specified track to the MIDI    */
/* channel specified.                                                        */
/* Take first index values from WrkBufMap and change all MIDI Status values  */
/* in all extents.                                                           */

/* maybe: include check, if this track already works with this MIDI channel  */
/* and set an appropriate returncode.                                        */

/*---------------------------------------------------------------------------*/

APIRET SetTrkMidiChannel(UCHAR ucTrkNum,
                         UCHAR ucChannel)

{
APIRET rc=0;
PTRKDATA pTrkExt; 
USHORT usActExtIdx;
USHORT usActEvtIdx;
UCHAR  ucStatus=0;                      /* Workingvariable for MIDI Status   */

if((ucChannel>16)||                     /* invalid channel number            */
   (ucChannel<1))   return(1);

if(ucTrkNum>63) return(2);              /* invalid track number              */

WrkBufMap[ucTrkNum].usActExtentIdx=0;   /* start value for track index       */
WrkBufMap[ucTrkNum].usActEventIdx=0;    /* start value for event index       */

do {
   usActExtIdx=WrkBufMap[ucTrkNum].usActExtentIdx;
   usActEvtIdx=WrkBufMap[ucTrkNum].usActEventIdx; 
   pTrkExt=WrkBufMap[ucTrkNum].pTrkExtent[usActExtIdx];  /* get extentaddr.  */
   ucStatus=(pTrkExt+usActEvtIdx)->ucMidiStatus;         /* get MIDI status  */

   /* now check the MIDI Status, if MIDI channel is to be changed.           */
   /* if YES: CHANGE and write back to the TrackBuffer                       */
   /* if NO:  Do nothing                                                     */
   /* if FC-END: Loop will terminate                                         */
   /* Events to change: NOTE ON:             9x                              */
   /*                   NOTE OFF:            8x                              */
   /*                   POLYPRESSURE:        Ax                              */
   /*                   CONTROL CHANGE:      Bx  Bx has more interpretations */
   /*                   PROGRAM CHANGE:      Cx                              */
   /*                   CHANNEL PRESSURE:    Dx                              */
   /*                   PITCH WHEEL:         Ex                              */
   /* i.e. all values between 8x and Ex have to be changed in their second   */
   /* nibble to the new MIDI channel value.                                  */

   if((ucStatus>=0x80)&&(ucStatus<=0xEF)) /* if Status contains channel info */
      {
      ucStatus=ucStatus & 0xF0;         /* clear channel bits                */
      ucStatus=ucStatus | (ucChannel-1);  /* set new channel bits            */
      }

   (pTrkExt+usActEvtIdx)->ucMidiStatus=ucStatus;       /* WRITE BACK         */

   WrkBufMap[ucTrkNum].usActExtentIdx=          /* get the next index values */
      (pTrkExt+usActEvtIdx)->ucNextExtentIdx;   /* in the chain              */
   WrkBufMap[ucTrkNum].usActEventIdx=
      (pTrkExt+usActEvtIdx)->usNextEventIdx;
  }while(ucStatus!=0xFC);               /* Do until END reached              */

return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function AbsTime2MidiTime()                                               */
/*---------------------------------------------------------------------------*/
/* This function calculates from a given absolute Timing (ulong) a MIDI      */
/* Timing, based on Timing-value and Measure related data.                   */
/* The result is: USHORT usMeasure               Measure Number              */
/*                USHORT usCombination            4 Bytes Taktmaá            */
/*                                               12 Bytes Ticks              */
/* Result is given back as ULONG to the passed pointer and must be inter-    */
/* preted.                                                                   */
/*---------------------------------------------------------------------------*/

APIRET AbsTime2MidiTime(ULONG ulTiming,
                        UCHAR ucTaktmassZaehler,
                        UCHAR ucTaktmassNenner,
                        UCHAR ucTicksPerQuarter,
                        ULONG *pulMidiTime)
{
APIRET rc=0;

USHORT usTicksPerNenner;                /* Ticks per given Taktmass          */
USHORT usTicksPerMeasure;               /* Ticks per complete measure        */
ULONG  ulMidiTiming=0;                  /* Result: complete value            */
ULONG  ulMeasure=0;                     /* Result: MEASURE   (0-65535)       */
ULONG  ulTaktmass=0;                    /* Result: TAKTMASS  (0-15)          */
ULONG  ulTicks=0;                       /* Result: TICKS     (0-4095)        */

/* calculate help variables first                                            */
usTicksPerNenner=ucTicksPerQuarter * 4 / ucTaktmassNenner;
usTicksPerMeasure=usTicksPerNenner * ucTaktmassZaehler;

/* now Calculate result values                                               */
ulMeasure=ulTiming/usTicksPerMeasure;                      /* max. 65535     */
ulTaktmass=(ulTiming%usTicksPerMeasure)/usTicksPerNenner;  /* max. 15        */
ulTicks=(ulTiming%usTicksPerMeasure)%usTicksPerNenner;     /* max. 4095      */

/* now Build ULONG Value from those results, which can be given back to      */
/* ulMidiTime. ULONG is built by OR and SHIFT operations with the three      */
/* result values of type ULONG.                                              */
ulMeasure=ulMeasure<<16;                /* Shift left by 16 Bytes            */
ulTaktmass=ulTaktmass<<12;              /* Shift left by 12 Bytes            */
ulMidiTiming=0;                         /* RESET to 0                        */
ulMidiTiming=ulMidiTiming | ulMeasure;
ulMidiTiming=ulMidiTiming | ulTaktmass;
ulMidiTiming=ulMidiTiming | ulTicks;

/* Write back result                                                         */
*pulMidiTime=ulMidiTiming;
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function MidiTime2AbsTime()                                               */
/*---------------------------------------------------------------------------*/
/* This function calculates from a given MIDI Timing (ULONG format) the abs. */
/* Timing, based on Timing-value and Measure related data.                   */
/* The result is: ULONG  ulTiming                Timing value                */
/* Result is given back as ULONG to the passed pointer.                      */
/*---------------------------------------------------------------------------*/
 
APIRET MidiTime2AbsTime(ULONG ulMidiTiming,
                        UCHAR ucTaktmassZaehler,
                        UCHAR ucTaktmassNenner,
                        UCHAR ucTicksPerQuarter,
                        ULONG *pulTiming)
{
APIRET rc=0;
USHORT usTicksPerNenner;                /* Ticks per given Taktmass          */
USHORT usTicksPerMeasure;               /* Ticks per complete measure        */
ULONG  ulAbsTiming=0;                   /* Result value                      */
ULONG  ulMeasure=0;                     /* Parm:   MEASURE   (0-65535)       */
ULONG  ulTaktmass=0;                    /* Parm:   TAKTMASS  (0-15)          */
ULONG  ulTicks=0;                       /* Parm:   TICKS     (0-4095)        */

/* calculate help variables first                                            */
usTicksPerNenner=ucTicksPerQuarter * 4 / ucTaktmassNenner;
usTicksPerMeasure=usTicksPerNenner * ucTaktmassZaehler;

rc=ExtractMidiTiming(ulMidiTiming,      /* Extract Timing values from LONG   */
                     &ulMeasure,
                     &ulTaktmass,
                     &ulTicks);

/* calculate ulAbsTiming from the three components                           */
ulAbsTiming=ulTicks;
ulAbsTiming+=(ulTaktmass*usTicksPerNenner);
ulAbsTiming+=(ulMeasure*usTicksPerMeasure);

*pulTiming=ulAbsTiming;                 /* return the Absolute Timing value  */
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function ExtractMidiTiming()                                              */
/*---------------------------------------------------------------------------*/
/* This function extracts the MEASURE, TAKTMASS and TICKS Values from the    */
/* given MIDI Timing in ULONG Format. Results are in ULONG Format.           */
/*---------------------------------------------------------------------------*/

APIRET ExtractMidiTiming(ULONG   ulMidiTiming,
                         ULONG   *pulMeasure,
                         ULONG   *pulTaktmass,
                         ULONG   *pulTicks)
{
APIRET rc=0;

*pulMeasure=ulMidiTiming & 0xFFFF0000;
*pulMeasure=*pulMeasure >> 16;
*pulTaktmass=ulMidiTiming & 0x0000F000;
*pulTaktmass=*pulTaktmass >> 12;
*pulTicks=ulMidiTiming & 0x00000FFF;

return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function FindPartnerNoteOff()                                             */
/*---------------------------------------------------------------------------*/
/* This function finds the NOTE OFF to a given NOTE ON within a specified    */
/* track. (Both: NOTE OFF command or NOTE ON with velocity 0)                */
/* Parameters: Tracknumber                                                   */
/*             Extent- and Event Index of given NOTE ON                      */
/* Results:    Extent- and Event Index of found NOTE OFF                     */
/*             Delta-Time between both events, i.e. length of note           */

/* This function must be designed for MIDI Timing Format                     */

/*---------------------------------------------------------------------------*/

APIRET FindPartnerNoteOff(UCHAR  ucTrkNum,
                          UCHAR  ucOnExtIdx,
                          USHORT usOnEvtIdx,
                          UCHAR  *pucOffExtIdx,
                          USHORT *pusOffEvtIdx,
                          ULONG  *pulDeltaTime)
{
APIRET rc=0;


return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function CalcMidiDeltaTime()                                              */
/*---------------------------------------------------------------------------*/
/* This function calculates the DELTA Time between two given Timing values   */
/* in MIDI Timing Format.                                                    */
/* Parameters: MIDI Timing 1 : ULONG                                         */
/*             MIDI Timing 2 : ULONG (smaller value)                         */
/* Results:    Delta Time    : ULONG (in absolute Format)                    */

/* Maybe a function with MIDI format result is also needed                   */

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Function WriteListBox()                                                   */
/*---------------------------------------------------------------------------*/
/* This function writes a given string to a selectable listbox.              */
/* Parameters: WindowHandle of Window which owns the destination listbox     */
/*             ID of the LISTBOX-control                                     */
/*             String to be written (null terminated)                        */
/*---------------------------------------------------------------------------*/

APIRET WriteListBox(HWND   hWndDestWindow,
                    ULONG  ulListBoxId,
                    CHAR   *pszString)
{
APIRET rc=0;

WinSendDlgItemMsg(hWndDestWindow,                      /* OUTPUT String      */
                  ulListBoxId,
                  LM_INSERTITEM,                 
                  MPFROMSHORT(LIT_END),          
                  MPFROMP(pszString));
WinSendDlgItemMsg(hWndDestWindow,                      /* SCROLL DOWN        */
                  ulListBoxId,
                  WM_CHAR,
                  MPFROM2SHORT(KC_VIRTUALKEY, 0),
                  MPFROM2SHORT(0, VK_DOWN));     

return(rc);
} 

/*---------------------------------------------------------------------------*/
/* Function InitChannelTable()                                               */
/*---------------------------------------------------------------------------*/
/* This function initializes the MIDI Channeltable in the application and    */
/* in the Driver by sending MPU_ChannelChange() to the driver.               */
/* INIT values is no change to the channels.                                 */
/* Parameters: none                                                          */
/*---------------------------------------------------------------------------*/
                                                                               
APIRET InitChannelTable(void)
{
APIRET rc;
UCHAR  i;
for(i=0;i<SEQ_TRACKS;i++)
   {
   ucChanTab[i]=16;                     /* Default: 16, driver does no chg   */
   }
rc=MPU_ChannelChange();                 /* also write to driver              */
return(rc);
}
                                                                              
/*---------------------------------------------------------------------------*/
/* Function BuildDispatchList()                                              */
/*---------------------------------------------------------------------------*/
/* This function build the Dispatch List for FillPlayBuffer function, which  */
/* contains the number of tracks to be dispatched, and an array of 64 bytes  */
/* containing the Track Numbers to be analyzed and copied to the play buffer */
/* Format of Dispatch List: ucDispatchList[0]    : number of valid entries   */
/*                          ucDispatchList[1-64] : array of Track numbers    */
/* The tracknumbers will be ordered by their priority ascending.             */
/* So the FillPlayBuffer function will take the higher (later) event, if     */
/* timings are equal. (check, if even higher timings could be taken prior to */
/* lower ones, if priority is high enough (regarding quantize functions!)    */
/*---------------------------------------------------------------------------*/ 

APIRET BuildDispatchList(void)
{
APIRET rc=0;
UCHAR  ucPriority[65];                  /* Help array for Sort algorithm     */
UCHAR  i;                               /* loop variable                     */

ucDispatchList[0]=0;                    /* init number of tracks to dispatch */
for(i=0;i<64;i++)
   {
   if(WrkBufMap[i].ulNumMidiEvents > 0) /* if Track contains data, copy to   */
      {                                 /* dispatch list                     */
      ucDispatchList[0]++;
      ucDispatchList[ucDispatchList[0]]=i;
      ucPriority[i+1]=WrkBufMap[i].ucPriority;
      }
   }                                   
/* now: Dispatch list contains all Tracks containing MIDI Data, ordered by   */
/* ascending tracknumber. The ucPriority array contains the corresponding    */
/* track priority values. Now do an ordering of the dispatch list with       */
/* ascending priority values.                                                */

/* here follows the sort algorithm */

return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function UpdateDispatchList()                                             */
/*---------------------------------------------------------------------------*/
/* This function updates the Dispatch List for FillPlayBuffer function, if   */
/* a track is ready with playback because of End of Track indicator 0xFC.    */
/* The track is removed from the list and the other tracks are shifted left  */
/* by one position. The counter in ucDispatchList[0] is decremented by 1.    */
/* Parameters: UCHAR ucTrkNum = Track to be removed from list.               */
/*---------------------------------------------------------------------------*/

APIRET UpdateDispatchList(UCHAR ucTrkNum)
{                                                                              
APIRET rc=0;
UCHAR  i,k;

for(i=1;i<=ucDispatchList[0];i++)
   {             
   if(ucDispatchList[i]==ucTrkNum)
      break;
   }

/* i now contains the index of the track to be removed. If this indes grows  */
/* above ucDispatchList[0]: the track to be removed is not in the list. If   */
/* it is the last entry in the list, only a decrement of ucDispatchList[0]   */
/* without copying bytes is needed. Else: shift remaining bytes              */

if(i>ucDispatchList[0]) return(5);      /* Track not in list...              */
if(i<ucDispatchList[0])                 /* if not last entry in list...      */
   {
   for(k=i;k<ucDispatchList[0]+i;k++)
      {
      ucDispatchList[k]=ucDispatchList[k+1];
      }
   }
ucDispatchList[0]--;                    /* decrement count dispatchable trk  */ 
return(rc);
}          

/*---------------------------------------------------------------------------*/
/* Function ResetNoteTable()                                                 */
/*---------------------------------------------------------------------------*/
/* This function resets the Note Table to all notes inactive                 */
/*---------------------------------------------------------------------------*/

APIRET ResetNoteTable(void)
{
APIRET  rc=0;
USHORT  i;
UCHAR   k;
UCHAR   ucLoop;
for(ucLoop=0;ucLoop<8;ucLoop++)         /* Table segments                    */
   {                                    /* For test: set all bytes to 0      */
   for(k=0;k<8;k++)                     /* Track within segment              */
      {
      for(i=0;i<4096;i++)               /* Byte within trackbuffer           */
         *(pNoteTable[ucLoop]+i+(k*4096))=0;
      }
   }
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function ChangePlayTrkStatus()                                            */
/*---------------------------------------------------------------------------*/
/* This function is called, when the user changes the track selection.       */
/* The Track is updated in WRKBUF and in the driver active mask.             */
/*---------------------------------------------------------------------------*/

APIRET ChangePlayTrkStatus(UCHAR ucNewState, UCHAR ucTrkNum)
{
APIRET  rc=0;
 
ucTrkNum--;                             /* Trknum -> Index to WRKBUF         */
WrkBufMap[ucTrkNum].ucTrkPlayStatus=ucNewState;
MPU_SetActiveTrk();                     /* update driver array               */
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function SetPlayStartPoint()                                              */
/*---------------------------------------------------------------------------*/
/* This function sets the Playback start point to a specified timing value   */
/* at which FillPlayBuffer will start to build the Playback Buffer.          */
/*---------------------------------------------------------------------------*/
                                      
/* NOCH INAKTIV */
                                         
APIRET SetPlayStartPoint(ULONG ulTiming)
{                                                                              
extern ULONG ulAccDriverTime;           /* actual accu timing from driver    */
extern PCHAR pPlayBuf;                  /* Play Buffer                       */

APIRET  rc=0;                                                                  
UCHAR   ucL1;
UCHAR   ucL2;
for(ucL1=0;ucL1<SEQ_TRACKS;ucL1++)
   {
   WrkBufMap[ucL1].usActExtentIdx=0;   /* initialize values              */
   WrkBufMap[ucL1].usActEventIdx=0;

/* for(ucL2=0;ucL2<WrkBufMap[ucL1].ulNumMidiEvents;ucL2++)
      {
      WrkBufMap[ucL1].usActExtentIdx=0;
      WrkBufMap[ucL1].usActEventIdx=0;
      }                                                   */
   }

ulAccDriverTime=0;                      /* Test: always start at 0           */
*((ULONG*)(pPlayBuf+2))=ulAccDriverTime;              /* init accu drv time  */
return(rc);
}

/*---------------------------------------------------------------------------*/
/* Function GetMaximumTiming()                                               */
/*---------------------------------------------------------------------------*/
/* This function returns the maximum value from WrkBufMap[].ulMaxTiming      */
/* it finds for all tracks. This is the length of the music in wrkbuf.       */
/* 0 is returned, if no data is found.                                       */
/*---------------------------------------------------------------------------*/

ULONG GetMaximumTiming(void)
{                                                                              
ULONG   ulMaxTiming;
UCHAR   ucL1;                                                                  
UCHAR   ucL2;                                                                  

ulMaxTiming=0;

for(ucL1=0;ucL1<SEQ_TRACKS;ucL1++)
   {                                                                           
   if(WrkBufMap[ucL1].ulMaxTiming>ulMaxTiming)
      ulMaxTiming=WrkBufMap[ucL1].ulMaxTiming;
   }                                                                           
return(ulMaxTiming);
}                                                                              
