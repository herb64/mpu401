/*
*******************************************************************************
* Source:       MIDICNT.H
* Program:      MIDISEQ.EXE
*------------------------------------------------------------------------------
* Description:  Header File, containing all container specific data
*
*******************************************************************************
*/

/*---------------------------------------------------------------------------*/
/* Definitions for TrackList container                                       */
/*---------------------------------------------------------------------------*/

#define  ID_TL_CONTAINER     70            /* ID of Tracklist Container win  */
#define  TL_NUMFIELDS        7             /* Number of Fields (columns)     */
#define  CCL_CONTAINER       "TrkListCont" /* Classname Container control    */
#define  TL_NUMRECORDS       64            /* Alloc 64 Records for 64 tracks */

/*---------------------------------------------------------------------------*/
/* Definition of RECORD Structure for the Tracklist Container                */
/*---------------------------------------------------------------------------*/
typedef struct _TLRECORD
   {                                                                           
   RECORDCORE RecordCore;               /* Recordcore Structure              */
   ULONG      ulTrkNum;                 /* Track Number (1-64)               */
   PSZ        pszTrkName;               /* Track NAME (META Event 03h)       */
   PSZ        pszInstrument;            /* Instrument NAME (META Event 04h)  */
   ULONG      ulTrkSize;                /* Track Size in MIDI Events         */
   PSZ        pszMidiChan;              /* Midi Channel as string            */
   PSZ        pszVelocity;              /* Velocity as string (editable)     */
   PSZ        pszText3;                 /* Data 3                            */
   } TLRECORD, FAR *PTLRECORD;
