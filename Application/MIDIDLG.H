/*
*******************************************************************************
* Quelle:       MIDIDLG.H
* Programm:     MIDISEQ.EXE
*------------------------------------------------------------------------------
* Beschreibung: allg. Header-File fÅr ALLE Dialogfenster des MIDISEQ Projektes
*               Include in .RC Datei fÅr das Projekt und in .C Code
*------------------------------------------------------------------------------
*
*
*******************************************************************************
*/

/*---------------------------------------------------------------------------*/
/* Definitionen fÅr Dialogfenster                                            */
/*---------------------------------------------------------------------------*/
#define  ID_DLG_MAIN         200           /* ID des Hauptdialogfensters     */
#define  DID_MD_RECORD       201           /* PUSHBUTTON RECORD              */
#define  DID_MD_PLAY         202           /* PUSHBUTTON PLAY                */
#define  DID_MD_STOP         203           /* PUSHBUTTON STOP                */
#define  DID_MD_FADER        204           /* PUSHBUTTON FADER               */
#define  DID_MD_SETTINGS     205           /* PUSHBUTTON Settings            */
#define  DID_MD_UPDATECHAN   206           /* PUSHBUTTON Update MIDI Channel */
#define  DID_MD_CLEARLIST    207           /* PUSHBUTTON Clear Listbox       */
#define  DID_MD_LISTBOX      208           /* Listbox for Output in Record   */
#define  DID_MD_STORE        209
#define  DID_MD_DELETE       210
#define  DID_MD_OVERDUB      211           /* PUSHBUTTON Overdub             */
#define  DID_MD_LOADFILE     212           /* PUSHBUTTON Load MIDI file      */
#define  DID_MD_RECFILTER    213           /* PUSHBUTTON Filter Dialog       */

#define  DID_MD_BENDER       260           /* Checkbutton Bender             */
#define  DID_MD_MIDITHRU     261           /* Checkbutton Midi Thru          */
#define  DID_MD_MEASEND      262           /* Checkbutton Measure end to h.  */
#define  DID_MD_EXCL2HOST    263           /* Checkbutton Bender             */

#define  ID_DLG_FADER        250           /* ID des Fader Dialogfensters    */

/*---------------------------------------------------------------------------*/
/* Definitions for Channelmapper dialog                                      */
/*---------------------------------------------------------------------------*/
#define  ID_DLG_CHANNELMAP   300           /* ID des Channel Mapper Dialogf. */
#define  DID_CM_SPIN1        311           /* Spinbutton Control channel 1   */
#define  DID_CM_SPIN2        312           /* Spinbutton Control channel 2   */
#define  DID_CM_SPIN3        313           /* Spinbutton Control channel 3   */
#define  DID_CM_SPIN4        314           /* Spinbutton Control channel 4   */
#define  DID_CM_SPIN5        315           /* Spinbutton Control channel 5   */
#define  DID_CM_SPIN6        316           /* Spinbutton Control channel 6   */
#define  DID_CM_SPIN7        317           /* Spinbutton Control channel 7   */
#define  DID_CM_SPIN8        318           /* Spinbutton Control channel 8   */
#define  DID_CM_SPIN9        319           /* Spinbutton Control channel 9   */
#define  DID_CM_SPIN10       320           /* Spinbutton Control channel 10  */
#define  DID_CM_SPIN11       321           /* Spinbutton Control channel 11  */
#define  DID_CM_SPIN12       322           /* Spinbutton Control channel 12  */
#define  DID_CM_SPIN13       323           /* Spinbutton Control channel 13  */
#define  DID_CM_SPIN14       324           /* Spinbutton Control channel 14  */
#define  DID_CM_SPIN15       325           /* Spinbutton Control channel 15  */
#define  DID_CM_SPIN16       326           /* Spinbutton Control channel 16  */
#define  DID_CM_CHANGE       327           /* Store changes in WRKBUF        */
#define  DID_CM_DEFAULT      328           /* Reset Channels to default      */

/*---------------------------------------------------------------------------*/
/* Definitions for Mixer Dialog                                              */
/*---------------------------------------------------------------------------*/
#define  ID_DLG_MIXER        700           /* ID of Mixer Dialog             */
#define  ID_MX_LEVBMP1       701           /* Bitmap for level Display TRK 1 */
#define  ID_MX_LEVBMP2       702           /* Bitmap for level Display TRK 2 */
#define  ID_MX_LEVBMP3       703           /* Bitmap for level Display TRK 3 */
#define  ID_MX_LEVBMP4       704           /* Bitmap for level Display TRK 4 */
#define  ID_MX_LEVBMP5       705           /* Bitmap for level Display TRK 5 */
#define  ID_MX_LEVBMP6       706           /* Bitmap for level Display TRK 6 */
#define  ID_MX_LEVBMP7       707           /* Bitmap for level Display TRK 7 */
#define  ID_MX_LEVBMP8       708           /* Bitmap for level Display TRK 8 */
#define  ID_MX_LEVBMP9       709           /* Bitmap for level Display TRK 9 */
#define  ID_MX_LEVBMP10      710           /* Bitmap for level Display TRK 10*/
#define  ID_MX_LEVBMP11      711           /* Bitmap for level Display TRK 11*/
#define  ID_MX_LEVBMP12      712           /* Bitmap for level Display TRK 12*/
#define  ID_MX_LEVBMP13      713           /* Bitmap for level Display TRK 13*/
#define  ID_MX_LEVBMP14      714           /* Bitmap for level Display TRK 14*/
#define  ID_MX_LEVBMP15      715           /* Bitmap for level Display TRK 15*/
#define  ID_MX_LEVBMP16      716           /* Bitmap for level Display TRK 16*/

#define  DID_SL_LEV1         721           /* Slider Controls in Mixerdialog */
#define  DID_SL_LEV2         722
#define  DID_SL_LEV3         723
#define  DID_SL_LEV4         724
#define  DID_SL_LEV5         725
#define  DID_SL_LEV6         726
#define  DID_SL_LEV7         727
#define  DID_SL_LEV8         728
#define  DID_SL_LEV9         729
#define  DID_SL_LEV10        730
#define  DID_SL_LEV11        731
#define  DID_SL_LEV12        732
#define  DID_SL_LEV13        733
#define  DID_SL_LEV14        734
#define  DID_SL_LEV15        735
#define  DID_SL_LEV16        736

/*---------------------------------------------------------------------------*/
/* Definitions for Record Filter dialog                                      */
/*---------------------------------------------------------------------------*/
#define  ID_DLG_RECFILTER    600           /* ID of Record Filter Dialog     */
#define  ID_RF_NOTE          601           /* Check Button Note On/Off       */
#define  ID_RF_AFTERTOUCH    602           /* Check Button Aftertouch        */
#define  ID_RF_CONTROLLER    603           /* Check Button Controller        */
#define  ID_RF_PATCHCHG      604           /* Check Button Patch Changes     */
#define  ID_RF_CHANNELAT     605           /* Check Button Channel AT        */
#define  ID_RF_PITCHWHEEL    606           /* Check Button Pitch Wheel       */
#define  ID_RF_APPLY         607           /* Pushbutton APPLY Filters       */

/*---------------------------------------------------------------------------*/
/* Definitions for Version Dialog                                            */
/*---------------------------------------------------------------------------*/
#define  ID_DLG_VERSION      350           /* Request Version Box            */
#define  DID_RV_OK           351           /* O.K. Button                    */
#define  DID_RV_MPUVERS      352           /* Text for MPU Version (e.g. 1.5)*/
#define  DID_RV_MPUREV       353           /* MPU Revision                   */
#define  DID_RV_DRVVERS      354           /* Driver Version                 */
#define  DID_RV_DRVREV       355           /* Driver Revision                */

#define  DID_ALL_HELP        299           /* Hilfe Funktionstaste (alle DLG)*/



#define ACT_BASE                    220 /* Basiswert fÅr Checkbuttons        */
#define DID_MD_ACT1                 221
#define DID_MD_ACT2                 222
#define DID_MD_ACT3                 223
#define DID_MD_ACT4                 224
#define DID_MD_ACT5                 225
#define DID_MD_ACT6                 226
#define DID_MD_ACT7                 227
#define DID_MD_ACT8                 228

#define RADIO_BASE                  230 /* Basiswert fÅr Radiobuttons        */
#define DID_MD_RAD1                 231
#define DID_MD_RAD2                 232
#define DID_MD_RAD3                 233
#define DID_MD_RAD4                 234
#define DID_MD_RAD5                 235
#define DID_MD_RAD6                 236
#define DID_MD_RAD7                 237
#define DID_MD_RAD8                 238

#define SIZE_BASE                   240 /* Basiswert fÅr Size Angaben        */
#define DID_MD_SIZE1                241
#define DID_MD_SIZE2                242
#define DID_MD_SIZE3                243
#define DID_MD_SIZE4                244
#define DID_MD_SIZE5                245
#define DID_MD_SIZE6                246
#define DID_MD_SIZE7                247
#define DID_MD_SIZE8                248

#define SPIN_BASE                   500 /* Basiswert fÅr Spinbuttons (CHAN)  */
#define DID_MD_SPIN1                501                                        
#define DID_MD_SPIN2                502                                        
#define DID_MD_SPIN3                503                                        
#define DID_MD_SPIN4                504                                        
#define DID_MD_SPIN5                505                                        
#define DID_MD_SPIN6                506                                        
#define DID_MD_SPIN7                507                                        
#define DID_MD_SPIN8                508                                        

#define DID_MD_TST1                 400 /* Test Ids fÅr diverse aufgaben     */
#define DID_MD_TST2                 401
#define DID_MD_TST3                 402
#define DID_MD_TST4                 403

#define TIME_BASE                   450 /* Basiswert fÅr Timebase Checkbutt. */
#define DID_MD_TIME1                451
#define DID_MD_TIME2                452
#define DID_MD_TIME3                453
#define DID_MD_TIME4                454
#define DID_MD_TIME5                455
#define DID_MD_TIME6                456
#define DID_MD_TIME7                457
#define DID_MD_TIME8                458

#define DID_SL_FADER                460 /* Fader Slider ID                   */
#define DID_SL_TEMPO                461 /* Tempo Slider ID                   */
#define DID_SL_RELTEMP              462 /* Relative Tempo Slider ID          */

#define DID_SB_TIME                 470 /* Scrollbar for timing              */
#define DID_TXT_TIME                480 /* Text output time                  */
