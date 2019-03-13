;*******************************************************************************
; Device Driver for Roland MPU401 intelligent MIDI Interface for OS/2 2.0/2.1
;
; Copyright  Herbert Mehlhose 1993
;
; Type of Driver: CHARACTER
; Version       ; 1.0
; Development Environment:  ESCOM 486 DX/33 ISA, 20MB, 540 MB HD
;                           OS/2 2.1 GA Code       XRG2010
;                           Developers Toolkit 2.1 XR02100
;                           Microsoft Segmented Executable Linker 5.10.005
;                           Assembler: Borland Turbo Assembler 1.01
;                           MPU HW: Midiman MM401 (Roland kompatibel)
;*******************************************************************************

MASM                                    ;Turbo Assembler 1.01 in MASM Mode
.MODEL  small

;*******************************************************************************
;  INCLUDE Files
;  All Include files are found in drive:\TOOLKT21\ASM\OS2INC Subdirectory
;*******************************************************************************

INCL_ERRORS     equ 1;                                                        
        .XCREF                          ; NO XREF and no LIST for the Includes
        .XLIST                                                                
        INCLUDE  bseerr.inc             ; ERROR CSEGS FOR DEVICES
        INCLUDE  basemaca.inc           ; VARIOUS MACRO'S                     
        INCLUDE  devsym.inc                                                   
        INCLUDE  devhlp.inc             ; DEFINITION OF DEVICE HELP CALLS     
;       INCLUDE  struc.inc              ; STRUCTURED MACRO SUPPORT            
        INCLUDE  infoseg.inc            ; STRUCTURES DEFINING THE INFOSEG     
        INCLUDE  gas.inc                ; GENERAL ALERT EQUATES               
        .LIST                                                                 
        .CREF                                                                 

        EXTRN   DosWrite:FAR            ; DosWrite Routine for Console Output
        PUBLIC  DeviceHelp              ; Entry Point for DevHlp Services
                                        ; passed by Kernel at INIT Time
 
drv_vers              equ  1            ; Driver Version and Revision level,
drv_rev               equ  0            ; can be queried by IOCTL Interface

stdin                 equ  0
stdout                equ  1
stderr                equ  2
                                          
cr                    equ  0dh          ; Carriage Return Code
lf                    equ  0ah          ; Line Feed Code
blank                 equ  020h         ; Blank Character

; Constants for beeps
beep_a2               equ  880          ; Frequency A
dur_1                 equ  500          ; 500 milliseconds duration

loop_wait             equ  0FFFFh       ; TIMEOUT value for LOOPS
rcv_ready             equ  01000000b    ; MPU Status when READY TO RECEIVE
data_ready            equ  10000000b    ; MPU Status when DATA ready to READ

mpu_category          equ  10000000b    ; Category code for MPU IoCtl calls

TIMING_OVERFLOW       equ  0F8h         ; MPU Timing overflow
NO_OPERATION          equ  0F8h         ; MPU Mark no operation
PLAY_STOP_CODE        equ  0FCh         ; stop code for track data request

NOTETAB_SIZE          equ  32768        ; Size of Notetable (8 GDT sel)
SEQ_TRACKS            equ  64           ; Number of tracks

; valid MPU Command codes
STOP_PLAY             equ  005h         ; Stop Play
START_PLAY            equ  00Ah         ; Start Play
STOP_REC              equ  011h         ; Stop Recording
STOP_OVERDUB          equ  015h         ; Stop Overdub
START_REC             equ  022h         ; Start Recording
START_OVERDUB         equ  02Ah         ; Start Overdub
UART_MODE             equ  03Fh         ; Set MPU to UART Mode
METRO_ON_NOACC        equ  083h         ; Metronome on w/o accents
METRO_OFF             equ  084h         ; Metronome off
METRO_ON_ACC          equ  085h         ; Metronome on with accents
DISABLE_BENDER        equ  086h         ; Disable Bender MSG to Host
ENABLE_BENDER         equ  087h         ; Enable Bender MSG to Host
DISABLE_THRU          equ  088h         ; Disable Midi Thru
ENABLE_THRU           equ  089h         ; Enable Midi Thru
DISABLE_MEAS_END      equ  08Ch         ; Disable Measure End To Host
ENABLE_MEAS_END       equ  08Dh         ; Enable Measure End To Host
DISABLE_CONDUCTOR     equ  08Eh         ; Disable Conductor
ENABLE_CONDUCTOR      equ  08Fh         ; Enable Conductor
DISABLE_REALTIME_AFF  equ  090h         ; Disable Realtime affection
ENABLE_REALTIME_AFF   equ  091h         ; Enable Realtime affection
DISABLE_CLOCK2HOST    equ  094h         ; Disable Clock to Host
ENABLE_CLOCK2HOST     equ  095h         ; Enable Clock to Host
DISABLE_EXCL2HOST     equ  096h         ; Disable Exclusive to Host
ENABLE_EXCL2HOST      equ  097h         ; Enable Exclusive to Host
DISABLE_CHREFTABA     equ  098h         ; Disable Channel Reference Table A
ENABLE_CHREFTABA      equ  099h         ; Enable Channel Reference Table A
DISABLE_CHREFTABB     equ  09Ah         ; Disable Channel Reference Table B
ENABLE_CHREFTABB      equ  09Bh         ; Enable Channel Reference Table B
DISABLE_CHREFTABC     equ  09Ch         ; Disable Channel Reference Table C
ENABLE_CHREFTABC      equ  09Dh         ; Enable Channel Reference Table C
DISABLE_CHREFTABD     equ  09Eh         ; Disable Channel Reference Table D
ENABLE_CHREFTABD      equ  09Fh         ; Enable Channel Reference Table D
REQUEST_VER           equ  0ACh         ; Request MPU Version
REQUEST_REV           equ  0ADh         ; Request MPU Revision
REQUEST_TEMPO         equ  0AFh         ; Request Tempo
CLR_PLAY_CNT          equ  0B8h         ; Clear play counters
WANT_TO_SEND_DATA     equ  0D0h         ; Want to send data command code
SET_TEMPO             equ  0E0h         ; Set Tempo command code
SET_REL_TEMPO         equ  0E1h         ; Set relative Tempo command code
SET_GRADUATION        equ  0E2h         ; Set graduation command code
SET_MIDI_PER_METRO    equ  0E4h         ; Set MIDI per METRO command code
SET_METRO_PER_MEASURE equ  0E6h         ; Set METRO per MEASURE
ACTIVATE_TRK          equ  0ECh         ; Activate track command code
RESET_CMD             equ  0FFh         ; Command Code for RESET

; Device Driver returncodes and errors
RP_ERR                equ  08000h       ; error bit
RP_DEVERR             equ  04000h       ; device error
RP_BUSY               equ  00200h       ; device busy code
RP_DONE               equ  00100h       ; device driver done bit
RP_FAIL               equ  00030h       ; general: operation failed
RP_IC_INV_CAT         equ  08102h       ; invalid category code
RP_IC_INV_FNC         equ  08103h       ; function code invalid in this mode
RP_RM_RES_F           equ  0810Ah       ; RESET command failed
RP_RM_RECAC           equ  0810Bh       ; Record Mode active while reset
RP_RM_PLYAC           equ  0810Ch       ; Play Mode active while reset
RP_SU_UART_F          equ  08114h       ; UART Mode command failed
RP_SU_RECAC           equ  08115h       ; Record Mode active while SET UART
RP_SU_PLYAC           equ  08116h       ; Play Mode active while SET UART
RP_RV_VER_F           equ  0811Eh       ; REQUEST Version command failed
RP_RV_REV_F           equ  0811Fh       ; REQUEST Revision command failed
RP_PP_PLIN            equ  08120h       ; Stop Play: Play not active
RP_WP_IN_TRK          equ  08121h       ; Requested track is not active
RP_WP_NOREF           equ  08122h       ; Requested buffer needs no refresh
RP_AT_PLAY            equ  08132h       ; MPU in PLAY Mode, act_trk invalid
RP_AT_ACT_F           equ  08133h       ; Activate track command failed
RP_AT_SND_F           equ  08134h       ; Send Data command failed
RP_PL_NOACT           equ  08137h       ; No active tracks when starting play
RP_PL_CLR_F           equ  08138h       ; Clear Play Counter command failed
RP_PL_FAIL            equ  08139h       ; Start Play command failed
RP_PL_GDTFAIL         equ  0813Ah       ; GDT selector mapping failed
RP_PL_V2P             equ  0813Bh       ; VirtToPhys failed
RP_PL_ACT_F           equ  0813Ch       ; Activate track command failed
RP_PL_SND_F           equ  0813Eh       ; Send Data command failed
RP_PL_INV_TDR         equ  0813Fh       ; Invalid Track Data Request found
RP_PL_SND_FAIL        equ  08140h       ; Send_data to MPU failed
RP_PL_STOP_F1         equ  08141h       ; stop play failed after ioctl stop
RP_PL_STOP_F2         equ  08142h       ; stop play failed after invalid TDR
RP_PL_UNLFAIL         equ  08143h       ; Unlock Databuffer failed at stop play
RP_SR_PLYAC           equ  08146h       ; Play active while start rec attempted
RP_SR_RECAC           equ  08147h       ; REC already active
RP_SR_GDTFAIL         equ  08148h       ; GDT selector mapping failed
RP_SR_FAIL            equ  08149h       ;
RP_SR_V2P             equ  0814Ah       ; VirtToPhys failed
RP_PR_UNLFAIL         equ  08150h       ; Unlock for STOP RECORD failed
RP_PR_RCIN            equ  08151h       ; Record not active at stop record
RP_PR_SCF             equ  08152h       ; Send command stop_record failed
RP_ME_MEAS_F          equ  08155h       ; Measure End send_command failed
RP_ME_ALRSET          equ  08156h       ; Measure End Mode already set
RP_LV_FAIL1           equ  08180h       ; Lock of IOCTL Buffer failed
RP_LV_FAIL2           equ  08181h       ; Lock ok, verify failed, unlock ok
RP_LV_FAIL3           equ  08182h       ; Lock ok, verify failed, unlock failed
RP_UNL_FAIL           equ  08185h       ; Unlock of virtual address failed
RP_BE_BEND_F          equ  08187h       ; Bender  send_command failed
RP_BE_ALRSET          equ  08188h       ; Bender Mode already set
RP_EX_EXCL_F          equ  0818Ah       ; Excl2Host send_command failed
RP_EX_ALRSET          equ  0818Bh       ; Excl2Host Mode already set
RP_RA_COMM_F          equ  0818Dh       ; Realtime affect. send_command failed
RP_RA_ALRSET          equ  0818Eh       ; Realtime affect. Mode already set
RP_MT_COMM_F          equ  0818Fh       ; MidiThru send_command failed
RP_MT_ALRSET          equ  08190h       ; MIDIThru Mode already set
RP_SF_ERROR           equ  08191h       ; Set Fader command failed
RP_OV_PLYAC           equ  08196h       ; Play already active
RP_OV_RECAC           equ  08197h       ; Record already active
RP_OV_V2P             equ  08198h       ; VirttoPhys error
RP_OV_GDTFAIL         equ  08199h       ; GDT selector mapping failed
RP_OV_SND_F           equ  0819Ah       ;
RP_OV_ACT_F           equ  0819Bh       ;
RP_OV_CLR_F           equ  0819Ch       ;
RP_OV_FAIL            equ  0819Dh       ;
RP_OV_INV_TDR         equ  0819Eh       ;
RP_OV_STOP_F2         equ  0819Fh       ;
RP_OV_SND_FAIL        equ  081A0h       ;
RP_OV_UNLFAIL         equ  081A1h       ;
RP_PO_RCIN            equ  081A5h       ; Stop overdub not in record mode
RP_PO_SCF             equ  081A6h       ; Stop overdub send command failed
RP_PO_UNLFAIL         equ  081A7h       ; Stop overdub unlock captbuf failed
SN_PARM_FAIL          equ  081AAh       ; Want to send data parm error
SN_CMD_FAIL           equ  081ABh       ; Want to send data send cmd fail
SN_DAT_FAIL           equ  081ACh       ; Want to send data send dat fail
ST_PARM_FAIL          equ  081ADh       ; Set tempo parm error
ST_CMD_FAIL           equ  081AEh       ; Set tempo send cmd fail
ST_DAT_FAIL           equ  081AFh       ; Set tempo send dat fail
SO_PARM_FAIL          equ  081B1h       ; Set rel tempo parm error
SO_CMD_FAIL           equ  081B2h       ; Set rel tempo send cmd fail
SO_DAT_FAIL           equ  081B3h       ; Set rel tempo send dat fail
SG_PARM_FAIL          equ  081B4h       ; Set graduation parm error
SG_CMD_FAIL           equ  081B5h       ; Set graduation send cmd fail
SG_DAT_FAIL           equ  081B6h       ; Set graduation send dat fail
MM_PARM_FAIL          equ  081B7h       ; Set midi per metro parm error
MM_CMD_FAIL           equ  081B8h       ; Set midi per metro send cmd fail
MM_DAT_FAIL           equ  081B9h       ; Set midi per metro send dat fail
RP_TB_CMDFAIL         equ  081BAh       ; Set Timebase command failed
RP_TB_INVPARM         equ  081BBh       ; Timebase: Invalid parm (not 1..7)
RP_CC_ERROR           equ  081BCh       ; Channel Change: error
RP_SA_ERROR           equ  081BDh       ; Set Active Tracks: error
RP_GN_V2P             equ  081C0h       ; Virt2Phys Notetable failed
RP_GN_GDTFAIL         equ  081C1h       ; Phys2GDT Notetable failed
RP_FN_UNLFAIL         equ  081C2h       ; Unlock Notetable failed

; Device Driver Attribute Word definitions
DAW_CHAR              equ  08000h       ; 1=char, 0=block
DAW_IDC               equ  04000h       ; 1=IDC capable
DAW_OPEN              equ  00800h       ; 1=open/close support
DAW_LEV1              equ  00080h       ; Level 1 Driver: OS/2 Driver
DAW_LEV2              equ  00100h       ; Level 2 Driver: IOCtl2 support
DAW_LEV3              equ  00180h       ; Level 3 Driver: Capab. Bit Strip

; MIDI Code Constants
NOTE_ON_1             equ  090h         ; Note ON CH1
VEL_MAX               equ  07fh         ; Max. Velocity
PITCH_MAX             equ  07fh         ; Max. Pitch
SYS_REALTIME          equ  0FEh         ; System Realtime Message

; MIDI System Messages
SM_MIDI_EXCL          equ  0F0h         ; System Exclusive       x Bytes
SM_SONG_POS           equ  0F2h         ; Song Position          3 Bytes
SM_SONG_SEL           equ  0F3h         ; Song select            2 Bytes
SM_TUNE_REQ           equ  0F6h         ; Tune request           1 Byte
SM_EOX                equ  0F7h         ; End of Exclusive       1 Byte
SM_TIM_CLK            equ  0F8h         ; Timing clock           1 Byte
SM_START              equ  0FAh         ; Midi Start             1 Byte
SM_CONT               equ  0FBh         ; Midi Continue          1 Byte
SM_STOP               equ  0FCh         ; Midi Stop              1 Byte
SM_ACT_SENS           equ  0FEh         ; Active Sensing         1 Byte
SM_SYS_RESET          equ  0FFh         ; System reset           1 Byte

; MPU Mode Byte Constants (for variable mpu_mode)
MOD_DEFAULT           equ  10000000b    ; Default for MPU_MODE
MOD_UART              equ  00000000b    ; UART Mode:         Bit 7=0
MOD_INT               equ  10000000b    ; Intelligent Mode:  Bit 7=1
MOD_OVERDUB           equ  01100000b    ; Overdub Mode:      REC and PLAY
MOD_REC               equ  01000000b    ; Record Mode:       Bit 6=1
MOD_PLAY              equ  00100000b    ; Play Mode:         Bit 5=1
MOD_DIS               equ  00010000b    ; Data in stop Mode: Bit 4=1

; MPU Mode Word Constants (for variable mpu_mode2)
MOD2_DEFAULT      equ 0000001000010010b ; Default for MPU_MODE2
MOD_EXCL2HOST     equ 0000000000000001b ; Excl. to Host      Bit 0=1
MOD_MEASEND       equ 0000000000000010b ; MEAS END to Host   Bit 1=1
MOD_CONDUCT       equ 0000000000000100b ; Conductor on       Bit 2=1
MOD_BENDER        equ 0000000000001000b ; Pitch Bender on    Bit 3=1
MOD_MIDITHRU      equ 0000000000010000b ; Midi Thru on       Bit 4=1
MOD_METROACC      equ 0000000000100000b ; METRO-   1 \/ 0 w/o 1 w   0 off
MOD_METRONOACC    equ 0000000001000000b ; NOME     1 /\ 1 acc 0 acc 0
MOD_xxx           equ 0000000010000000b ; not yet defined
MOD_CLK2HOST      equ 0000000100000000b ; Clock to Host on   Bit 8=1
MOD_REALTIME_AFF  equ 0000001000000000b ; Realtime affect.   Bit 9=1

; Mode Constants for functions MIDI_ANALYZE und MIDI_ANALYZE_WR
MA_NEW                equ  0            ; Begin of new MPU Data chain
MA_TIMING             equ  1            ; The tested byte was MPU Timing
MA_READY              equ  2            ; The tested byte was Last Data Byte
MA_SYSMSG             equ  3            ; The tested byte was MPU System MSG
MA_EXCL               equ  4            ; The tested byte belongs to MIDI EXCL
MA_TRKREQ             equ  5            ; The tested byte is a TRACK DATA REQ.
MA_LEN1               equ  6            ; 1 Byte will follow tested byte
MA_LEN2               equ  7            ; 2 Bytes will follow tested byte
MA_LEN3               equ  8            ; 3 Bytes will follow tested byte
MA_ERROR              equ  99           ; Invalid mode passed to CNT_LENGTH

;*******************************************************************************
;       Request Packet Format Definitions
;       Segment AT does not generate code
;*******************************************************************************

rp             segment at 0
rp_len         db  ?            ; length of request packet
rp_unit        db  ?            ; block device unit code (not used)
rp_command     db  ?            ; command code
rp_status      dw  ?            ; status word for Return Codes from Strategy
rp_rsvd        dw  2 dup(?)     ; reserved for DOS
rp_q_link      dw  2 dup(?)     ; request packet queue linkage
rp_DSEG        equ $            ; DSEG appropriate to the operation
                                                                        
;       Request Packet DSEG for INIT command (2 ways of interpretation)
;       1. part: variables at beginning of INIT section
            org rp_DSEG-rp      ; Offset fr rp_DSEG, Command Specific Data
rp_in_units    db  ?            ; number of units (only Block dev, not used)
rp_in_devhlp   dd  ?            ; address of devhlp interface, passed by OS/2
rp_in_args     dd  ?            ; address of argument list from CONFIG.SYS
rp_in_drive    db  ?            ; number of 1st block device unit
;       2. part: variables at completion of INIT section, passed back values
            org rp_DSEG-rp      ; Offset fr rp_DSEG, Command Specific Data 
rp_in_units    db  ?            ; identical usage to rp_in_units_a
rp_in_cseg_end dw  ?            ; offset to end of code segment
rp_in_dseg_end dw  ?            ; offset to end of data segment
rp_in_bpb      dd  ?            ; address of BPB array (not used)

;       Request Packet DSEG for OPEN command
            org rp_DSEG-rp      ; Offset fr rp_DSEG, Command Specific Data 
rp_op_sys_file dw  ?            ; System File Number
                                            
;       Request Packet DSEG for CLOSE command
            org rp_DSEG-rp      ; Offset fr rp_DSEG, Command Specific Data
rp_cl_sys_file dw  ?            ; System File Number

;       Request Packet DSEG for IOCTL command
            org rp_DSEG-rp      ; Offset fr rp_DSEG, Command Specific Data
rp_ioctl_categ db  ?            ; category CSEG                         
rp_ioctl_funct db  ?            ; function CSEG                         
rp_ioctl_pbuf  dd  ?            ; addr of parms (offset and selector)  VIRTUAL
rp_ioctl_dbuf  dd  ?            ; addr of buffer (offset and selector) VIRTUAL
rp_ioctl_sysfn dw  ?            ; system file number from open
rp_ioctl_pbufl dw  ?            ; Parameter Buffer length (ioctl2)
rp_ioctl_dbufl dw  ?            ; Data Buffer length (ioctl2)
                                                                        
rp          ends                                                        

;*******************************************************************************
;  DATASEGMENT Definition
;  The first element is the DEVICE DRIVER HEADER with a chain of DEVICE HEADERS.
;  In this driver there is only ONE device used, so the pointer to the next
;  DEVICE HEADER is set to -1
;*******************************************************************************

DSEG        SEGMENT   PUBLIC  'DATA'

               dd     -1               ; address of next device header
               dw     DAW_CHAR+DAW_OPEN+DAW_LEV2   ; Device Attribute Word
               dw     dev_strategy     ; address of device strategy entry point
               dw     ?                ; reserved
               db     'MPU401$ '       ; name of device
               dw     4 dup(?)         ; reserved

DeviceHelp     dd     ?                ; device help interface pointer, which
                                       ; is used to store the Entry Point for
                                       ; the Device Helper routine during INIT
InitParm       dd     ?                ; address for input parameters from
                                       ; CONFIG.SYS DEVICE=xxx statement
wlen           dw     ?
factor         dw     ?                ; Factor for ascii_2_int function
factor10       dw     10               ; Multiply Factor of 10
factor16       dw     16               ; Multiply Factor of 16

open_count     db     0                ; OPEN Counter


md_status      db     0                ; test midi_dataout
md_data1       db     0                ; test midi_dataout  
md_data2       db     0                ; test midi_dataout

; for use with Notetab functions
gn_sel_index   db     0                ; help variable with GDT Sel Idx (0..7)


; ioctl record function (ioc_start_rec)
midi_rec_bufsz dw     4096             ; Application buffer size for read
midi_rec_ptr   dw     0                ; MIDI IN read pointer

; Buffer and Pointer defintions for ioc_play function
midi_play_bufsz dw    16384            ; Play Buffer size
mpu_act_trk    db    00000001b         ; Active Track MAP (only 1 used)
trk_data_req   db     0                ; Track Data Request
pl_offset      dw     2                ; Play Buffer current offset
pl_trknum      db     0                ; Tracknumber of current event sent
pl_accu_time   dd     0                ; accumulated playback timer
pl_abs_time    dd     0                ; absolute timing from playbuffer
pl_delta_time  dd     0                ; delta from abs and accu time

; Definitions for MIDI_ANALYZE function
ma_mode_rec    db     0                ; Mode for use by Int Hdlr
ma_mode_play   db     0                ; Mode for use by ioc_wrt_play_buf
ma_runst_play  db     0                ; Value of running status for track n
ma_cnt_runst   dw     0                ; bytes to transf. with running status
mr_pitch       db     0                ; status, pitch and velocity are stored
mr_status      db     0                ; by midi_analyze for update of notetab
mr_velocity    db     0                ; in midi_dataout

; Variables for IOCtl Parms in ioc_meas_end function
measend_switch db     0                ; Measure End to host 1=ON, 0=OFF

; Variables for IOCtl Parms in ioc_timebase function
tb_timebase    db     078h             ; Default Timebase value

; Variables for IOCtl Parms in ioc_bender function
bender_switch  db     0                ; Bender Msg to host 1=ON, 0=OFF

; Variables for IOCtl Parms in ioc_excl2host function
excl_switch    db     0                ; Exclusive to host 1=ON, 0=OFF

; Variables for IOCtl Parms in ioc_realtime_aff function
realt_switch   db     0                ; Realtime affection 1=ON, 0=OFF

; Variables for IOCtl Parms in ioc_midi_thru function
thru_switch    db     0                ; MIDI Thru 1=ON, 0=OFF

; Variables for IOCtl Parms in ioc_set_fader function
pl_fader       db     64 dup(255)      ; 64 Fader values for all tracks

; Variables for IOCtl Parms in ioc_send_data function
sn_command     db     0                ; command code (on/off/change)
sn_pitch       db     0                ; new pitch value
sn_velocity    db     0                ; new velocity value
sn_channel     db     0                ; channel to send

; Variables for IOCtl Parms in ioc_set_tempo function
st_tempo       db     064h             ; Tempo, default value

; Variables for IOCtl Parms in ioc_set_rel_tempo function
so_tempo       db     040h             ; relative tempo, default value 1:1

; Variables for IOCtl Parms in ioc_set_graduation function
sg_graduation  db     0                ; graduation, default value = 0

; Variables for IOCtl Parms in ioc_midi_per_metro function
mm_midimetro   db     0                ; MIDI per METRO, default = 0C0h

; Variables for IOCtl Parms in ioc_chan_change function
cc_chantab     db     64 dup(16)       ; Midi Channel value for each track
                                       ; default is 16, which means NO CHANGE

; Variables for IOCtl Parms in ioc_set_acttrk function
sa_acttrk      db     64 dup(0)        ; Active tracks array

; MPU401 status and setup information
mpu_data_port   dw    0330h            ; Mpu data port address (default)
mpu_command_port LABEL  WORD           ; Mpu command port address.
mpu_status_port dw    0331h            ; Mpu status port address (default)
mpu_irq         db    02h              ; Mpu interrupt level (default)
mpu_mode        db    MOD_DEFAULT      ; Actual Mode of MPU (def.: int.Mode)
mpu_mode2       dw    MOD2_DEFAULT     ; Additional Mode Information (def.)
mpu_vers_hi     db    ?                ; MPU Version higher part
mpu_vers_lo     db    ?                ; MPU Version lower part
mpu_rev_hi      db    ?                ; MPU Revision level higher part
mpu_rev_lo      db    ?                ; MPU Revision level lower part

; These are GDT Selectors to be allocated at INIT time.
Num_GDT_Sel    equ    10               ; Allocation of 10 GDT Selectors
GDT_Select     equ    $
Sel_REC        dw     0000h            ; Selector for ioc_start_rec function
Sel_PLAY       dw     0000h            ; Selector for ioc_play function
Sel_NOTETAB    dw     8 dup(0000h)     ; 8 Selectors for note table functions

; IOCTL Command table, index is the IOCTL function code
ioctl_tabsize  equ    25                 ; Number of function entries
ioctl_functab  dw     ioc_reset_mpu      ; Reset the MPU
               dw     ioc_set_uart       ; Set MPU to UART mode
               dw     ioc_req_version    ; Request Version and Revision data
               dw     ioc_stop_play      ; Stop Playback
               dw     ioc_bender         ; Set Bender ON/OFF
               dw     ioc_play           ; Playback of recorded data
               dw     ioc_start_rec      ; Start Recording
               dw     ioc_stop_rec       ; Stop Recording
               dw     ioc_meas_end       ; Send Measure End F9 to Host on/off
               dw     ioc_timebase       ; Set MPU timebase (C2-C8)
               dw     ioc_excl2host      ; Exclusive to Host ON/OFF
               dw     ioc_realtime_aff   ; Realtime Affection ON/OFF
               dw     ioc_midi_thru      ; MIDI Thru on/off
               dw     ioc_set_fader      ; Set fader values for all tracks
               dw     ioc_overdub        ; Start Overdub
               dw     ioc_stop_overdub   ; Stop Overdub
               dw     ioc_send_data      ; Want to send data
               dw     ioc_set_tempo      ; Set tempo command
               dw     ioc_set_rel_tempo  ; Set relative tempo command
               dw     ioc_set_graduation ; Set graduation
               dw     ioc_midi_per_metro ; Set MIDI per Metro
               dw     ioc_chan_change    ; Online MIDI Channel mapping
               dw     ioc_set_acttrk     ; Set active tracks mask
               dw     ioc_gdt_notetab    ; Setup GDT selector for notetable
               dw     ioc_free_notetab   ; Free GDT Selector for notetable
;               dw     ioc_metronome
;               dw     ioc_req_tmp
;               dw     ioc_conductor

; Handle variables for DevHlp_ProcBlock and DevHlp_Lock calls
block_hnd_play dd     ?                ; Block Handle for ioc_play function
ioc_lock_hnd1  dd     ?                ; Lock Handle used in ioc_req_version
ioc_lock_hnd2  dd     ?                ; Lock Handle for ioc_play/overdub
ioc_lock_hnd3  dd     ?                ; Lock Handle for ioc_start_rec
ioc_lock_hnd4  dd     8 dup(?)         ; Lock Handle array for Notetable

; INIT Messages on Console. All Data beyond this line will be discarded by the
; OS/2 Kernel after INIT of the Driver during BOOT.

; This is the text to be written to the Console, if the Driver is installed
; successfully
dsegend        label      byte         ; End of Datasegment, this Offset is
                                       ; passed to the Kernel via Request 
                                       ; Packet in the INIT routine
IdIn           db   cr,lf
               db   "Roland MPU401 Device Driver Version 1.0 for OS/2 2.0"
               db   cr,lf                                          
               db   "Copyright Herbert Mehlhose  1993"
               db   cr,lf
               db   "Driver INSTALLED SUCCESSFULLY                   "
               db   cr,lf
               db   "Interrupt level is "
int_lev_ascii  db   "02 "              ; default interrupt level
               db   "and IO-address is "
addr_ascii     db   "330 "             ; default  IO address
               db   cr,lf
IdIn_len       equ  $-IdIn             ; Length of Output Data to Console

; This is the text to be written to the Console, if the Driver cannot be in-
; stalled, because the MPU RESET did not succeed.
ResetFail      db   cr,lf
               db   "Roland MPU401 Device Driver   Initialization ERROR"
               db   cr,lf
               db   "The initial RESET of the MPU401 failed....     "
               db   cr,lf
RF_len         equ  $-ResetFail        ; Length of Output Data to Console

; This is the text to be written to the Console, if the Interrupt Handler
; fails to register (DevHlp Service Call during init after RESET)
RegisFail      db   cr,lf
               db   "Roland MPU401 Device Driver   Initialization ERROR"    
               db   cr,lf                                                   
               db   "The Interrupt Hander could not be registered..."
               db   cr,lf                                                   
RS_len         equ  $-RegisFail        ; Length of Output Data to Console

; This is the text to be written to the Console, if the Test Data Outputs fail
TestFail       db   cr,lf
               db   "Roland MPU401 Device Driver   DATA output test...."
               db   cr,lf                                                  
TF_len         equ  $-TestFail         ; Length of Output Data to Console

DSEG            ends                                                     

;*******************************************************************************
; Code Segment Definition
;*******************************************************************************
                                                                                
CSEG        SEGMENT  PUBLIC 'CODE'
                                                                           
        assume  cs:CSEG,ds:DSEG,es:rp                                           
;-------------------------------------------------------------------------------
; Entry Point to STRATEGY ROUTINE of the Device Driver
; Any Command Code in the Request packet above x'10' (generic IOCtl) will
; be recognized as invalid.
; Register pair ES:BX will contain the address of the RequestPacket when the
; strategy routine is called by the OS/2 system.
;-------------------------------------------------------------------------------
                                                                                
dev_strategy proc far
        mov     rp_status[bx],RP_DONE    ; request complete, success
        mov     al,rp_command[bx]        ; route control based on
        cmp     al,10h                   ; command CSEG                    
        ja      invalid_command          ; invalid if above generic ioctl       
        sal     al,1                     ; set up to                            
        cbw                              ; take the                           
        mov     si,ax                    ; correct jump into the table     
        jmp     short route              ; short Jump, only Byte offset
        page                                                                    
function_table label word                ; word alignment
        dw      initialize               ; INIT Function Code in Request Packet
        dw      media_check              ; These Words contain the addresses of
        dw      build_bpb                ; the labels
        dw      reserved_3                                                      
        dw      dev_read                 ; READ command
        dw      non_destruc_read                                                
        dw      input_status                                                    
        dw      input_flush                                                     
        dw      dev_write                ; Write command
        dw      write_verify                                                    
        dw      output_status                                                   
        dw      output_flush                                                    
        dw      reserved_c                                                      
        dw      dev_open                 ; open                                 
        dw      dev_close                ; close                                
        dw      remov_media                                                     
        dw      ioctl                    ; GENERIC IOCTL Function Code
route:  jmp     function_table[si]       ; Jump to requested Function
     
; all unsupported commands follow
                                                                                
invalid_command:                                                                
media_check:                                                                    
build_bpb:                                                                      
reserved_3:                                                                     
dev_read:
non_destruc_read:                                                               
input_status:                                                                   
input_flush:                                                                    
dev_write:
write_verify:                                                                   
output_status:                                                                  
output_flush:                                                                   
reserved_c:                                                                     
remov_media:                                                                    
; dev_open: 
; dev_close:
; ioctl:
        mov     rp_status[bx],8103h      ; request complete, unknown command
        ret                                                                     
dev_strategy endp                        ; End of Strategy Procedure
                                                                    
;-------------------------------------------------------------------------------
; Prozedur:  SEND_COMMAND
; PASSED Data:  MPU Command Code, passed in register AH
; ACTION     :  send the command to the MPU and receive the Acknowledge
;               and, if applicable to the command, a RETURN VALUE.
;               Return values are expected for commands A0-AF.
; RETURN     :  a)  Return Value from the MPU for the executed command (A0-AF)
;               b)  0 when no return expected, but successful command completion
;               c) -1 when error during execution (3 possibilities):
;                     - MPU does not get Ready to RECEIVE Commands
;                     - No ACKNOWLEDGE within TIMEOUT loop_wait
;                     - an expected Return Value from the MPU is not received
;               d) -2 when other command than RESET are sent while MPU is in
;                     UART Mode.
; IMPORTANT  :  When the MPU is in UART Mode, it will only accept the RESET
;               command code. This is also the only situation, in which the
;               command will NOT generate an acknowledge!
;               Therefore the procedure ignores an Acknowledge timeout in UART
;               Mode.
;-------------------------------------------------------------------------------
send_command    proc
;        pushf                           ; Save flags (interrupt flag)
        push    cx
        push    dx
        push    bx

        mov     bh,ah                   ; Command to BH for later use

        mov     al,mpu_mode             ; first filter all commands except RESET
        test    al,MOD_INT              ; when MPU is in UART mode.
        jnz     sc_2                    ; if intelligent Mode, proceed...
        mov     ax,-2                   ; if UART mode: check command...
        cmp     bh,RESET_CMD
        je      sc_2
        jmp     sc_13                   ; exit, when other command than RESET
; this part SENDS the COMMAND to the MPU401. In UART mode, only the RESET
; command arrives at this point
sc_2:   mov     cx,5000                 ; Wait for MPU to be ready to receive
        mov     dx,mpu_status_port      ; a command byte by
sc_3:   in      al,dx                   ; 1.reading the status port and
        test    al,rcv_ready            ; 2.test bit 6 for rcv_ready
        jz      sc_4                    ; if READY: proceed
        loop    sc_3                    ; else: READ again, max. loop_wait times
        mov     ax,-1                   ; TIMEOUT: Set Returncode to -1
        jmp     sc_13                   ; Jump to END of Procedure
sc_4:   ; cli                             ; Suspend interrupts
;        cmp     trk_data_req,0F0h       ; wenn KEIN TDR in variable ansteht:
;        jne     sc__12                  ; -> senden cmd
;        jmp     sc_12    ; Was, wenn FC End ????, was bei RECORD data???
; FC end sollte auch unterbunden werden, es sei denn, es kommt von STOP
; OVERDUB. Auf jeden Fall: es kann sich immer ein FC end w„hrend eines
; Commands einschleichen, wie auch bei TDRs.

; Ansonsten: bisher ist noch kein TDR eingetroffen. Interrupts sind disabled.
; d.h. ein TDR kann ankommen, l”st aber keinen INT aus, sondern setzt nur den
; Statusport auf data_ready und legt TDR an Dataport.
sc__12: mov     al,bh                   ; Put Command Code to AL for OUTPUT
        mov     dx,mpu_status_port      ; dx explizit setzen  ?????
        cli
        out     dx,al                   ; Command to Comm. Port (=Status Port)
        mov     al,mpu_mode             ; check MPU Mode to determine, if we
        test    al,MOD_INT              ; must wait for an acknowledge
        jnz     sc_5                    ; if int. Mode: Acknowledge expected
        xor     ax,ax                   ; else: set Returncode to 0
        jmp     sc_12                   ; restore interrupts and exit
; This part processes the ACKNOWLDEDGE which must come from the MPU401 for any
; command sent, except for the RESET Command, when sent while the MPU is in
; UART Mode. Statt ACK kann aber auch inzwischen ein TDR eingetroffen sein.
sc_5:   mov     cx,loop_wait            ; Wait for the COMMAND ACKNOWLEDGE from
        mov     dx,mpu_status_port      ; the MPU by
sc_6:   in      al,dx                   ; 1.reading the status port and
        test    al,data_ready           ; 2.test for DATA Ready
        jz      sc_7                    ; if DATA READY:  PROCEED
        loop    sc_6                    ; else: try again, max loop_delay times

; hier: Fehler, Timout bei Warten auf ACK. Statt data_ready ist rcv_ready
; aktiv (0). D.h. es scheint ein TDR anzustehen, der bearbeitet werden muá.
; dies wird zum Test einfach einmal angenommen.

        jmp     sc_7        ; 'normales' processing trotz overflow
                            ; dies macht den folgenden code unwirksam der nur
                            ; zum tracen dieses events dient.

        mov     ah,al          ; save mpu status port value
        mov     cx,dx          ; save PORT to cx, to which cmd has been sent
        mov     dx,mpu_data_port        ; lies daten TROTZ timeout
        in      al,dx
        int     3                       ; ACK timeout processing
        jmp     sc_ts1


; hier: nach send command wurde DATA READY Status am Status port gefunden.
; oder aber nach overflow von loop_wait wird das byte trotzdem gelesen.
sc_7:   mov     dx,mpu_data_port        ; Set DATA Port address
        in      al,dx                   ; Read in the return data

sc_ts1: cmp     al,0FEh                 ; Is it an acknowledge?
        je      sc_8                    ; if ACK: proceed cmd processing

        cmp     al,0        ; seltsamerweise steht beim BOOT 0 im return
        je      sc_5        ; CX ist FFFF, dh kein warten war n”tig!
 
        cmp     al,0F0h                 ; is it an TDR?
        je      sc_t1                   ; jump to TDR processing

;       cmp     al,0FCh                 ; FRAGE: WAS bei FC End??
;       je      xxxxx                   ; recht komplex, je nach mode

        int     3           ; trap the unknown CODE
        mov     ax,-4
        jmp     sc_12

; sc_t1 label:
; hier: neuer seltsamer effekt: bei Command codes, die keine Daten verlangen
; darf nicht nochmals die warteschlange angesprungen werden, die auf das
; ACK wartet (sc_5). Dies verursachte eine Endloswiedergabe einiger noten
; und unbedienbarkeit der Maus - Genaue ursache noch ungekl„rt, aber ohne
; dieses Abholen eines ACK arbeitet die Funktion.
; Bei den command codes, die nochmals daten erwarten: Schleife nochmals
; anspringen. Dies sind die En commands, z.B. SetTempo

sc_t1:  push    bx                      ; save command code
        call    midi_dataout            ; send the midi event for tdr
        pop     bx

;        jmp     sc_7       ; vorerst TEST: OHNE Warten das ACK abholen!!!!
                           ; d.h. statt sc_5 auf sc_7 springen!!!
 
        cmp     bh,0E0h    ; wenn kein Datenbyte n”tig fr command
        jb      sc_8       ; kein Warten auf ACK (seltsam)
        cmp     bh,0EFh    ; wenn kein Datenbyte n”tig fr command
        jg      sc_8       ; kein Warten auf ACK (seltsam)

;       jmp     sc_7   ; test: statt sc_5 (warten): einfach lesen (sc_7) !!!

        jmp     sc_5       ; wenn Datenbyte n”tig: warten auf ACK

; this part receives the RETURN Values from the MPU401, if any are expected for
; the command type sent before.
sc_8:   xor     ax,ax                   ; Set AX to 0 for returncode 0!!
        cmp     bh,0A0h                 ; Commands codes less than x'A0' or
        jb      sc_12                   ; higher than x'AF' do not return any
        cmp     bh,0AFh                 ; data. Commands from A0 to AF DO!!
        ja      sc_12                   ; IF NO Return: Jump to END with AX=0
        mov     cx,loop_wait            ; Wait for the Data being READY at the
        mov     dx,mpu_status_port      ; Dataport by Checking the Statusport
sc_9:   in      al,dx                   ; for DATA_READY
        test    al,data_ready
        jz      sc_10                   ; if DATA READY: Proceed
        loop    sc_9                    ; else: test status again (loop_wait)
        mov     ax,-1                   ; TIMEOUT: Set Returncode to -1
        jmp     short sc_12             ; Jump to END of Procedure
; this part is referenced, when a command completed successfully
sc_10:  mov dx,mpu_data_port            ; Get the Return Value
        in      al,dx                   ; into Register AX
        xor     ah,ah                   ; AH = 0 !!
sc_12:  sti                             ; Restore interrupts
sc_13:  pop     bx
        pop     dx
        pop     cx
;        popf                            ; Restore flags
        ret                             ; RETURN with Return Value from Command
                                        ; in AX, or if not Successful: AX=-1
                                        ; or AX=0 if successful and no Return
send_command    endp                    ; E N D    of   S e n d _ C o m m a n d

;-------------------------------------------------------------------------------
; Prozedur:  SEND_DATA
; PASSED Data:  BYTE with data to be sent to MPU401 in Register AH
; ACTION     :  send the databyte to the MPU. No Acknowledge is sent back.
; RETURN     :   0 if ok
;               -1 if timeout occurred
; Remark     :  Returncode 0 does not guarantee, that the Byte will really be
;               delivered to the MIDI out of the MPU. It only indicates that
;               the MPU was ready to receive and the Byte has been written to
;               the Dataport.
;-------------------------------------------------------------------------------

send_data       proc 
;        pushf                           ; Save flags
        push    cx
        push    dx
; this part SENDS the DATABYTE to the MPU401
        mov     cx,loop_wait            ;Wait for MPU to be ready to receive
        mov     dx,mpu_status_port      ;a command byte by
sd_1:   in      al,dx                   ;1.reading the status port and
        test    al,rcv_ready            ;2.test bit 6 for rcv_ready
        jz      sd_snd                  ;if READY: proceed
        loop    sd_1                    ;else: READ again, max. loop_wait times
        mov     ax,-1                   ;TIMEOUT: Set Returncode to -1
        jmp     short sd_end            ;Jump to END of Procedure
sd_snd: ;cli                             ;Disable Interrupts
        mov     dx,mpu_data_port
        mov     al,ah
        out     dx,al                   ;Output to Dataport of MPU
;        sti
        mov     ax,0                    ;AX=Returncode 0
sd_end: pop     dx
        pop     cx
;        popf
        ret
send_data       endp

;-------------------------------------------------------------------------------
; Entry Point to INTERRUPT HANDLER ROUTINE
; The Interrupt handler is responsible to read all data, the MPU sends with an
; interrupt from the DATA PORT.
; Actions:
; 1. Test, in which mode the MPU works: RECORD or PLAY or OVERDUB
;
; The first 2 Bytes in Capture Buffer are used to enable the application to
; read the actual position, where the last byte has been written into the
; capture buffer.
;-------------------------------------------------------------------------------

interrupt_hdlr  proc FAR
; 1. Test MPU, if Interrupt came from there
        push    si
        push    ds
        push    dx
        push    cx
        push    ax
        mov     dx,mpu_status_port
        in      al,dx
        test    al,data_ready
        jz      int_1                   ; If Data available from MPU:
        stc                             ; Set Carry Flag
        jmp     int_en                  ; E X I T  Interrupt Handler
; 2. Receive Data Byte, if MPU initiated the Interrupt
int_1:  mov     dx,mpu_data_port        ; DX = Dataport to receive data
        in      al,dx                   ; Get the Data Byte from MPU
        push    bx
        mov     ah,mpu_mode             ; Test MPU_MODE byte:
        test    ah,MOD_PLAY             ; Play Mode active?
        jz      int_x                   ; if not: test record mode
        cmp     al,0F0h                 ; Check, if Byte is TDR
        jb      int_x                   ; if no TDR: check for REC mode

; NEU: INT hdlr gibt selbst direkt die daten aus, wenn ein TDR einen INT
; erzeugt.
        je      int_da                  ; ausgabe daten bei TDR direkt

        cmp     al,0FCh                 ; MPU DATA END Mark FC? Pass to
        je      int_2                   ; ioc_play to stop playthread
        cmp     al,0F7h
        jg      int_x

        int     3         ; hier sollte code nie ankommen

int_2:  mov     trk_data_req,al         ; store TDR in Variable
; now unblock the waiting PLAY thread (ioc_play function)
; AX returns the number of actually awakened threads
        push    di
        mov     di,2
        mov     bx,word ptr block_hnd_play[di]
        mov     ax,word ptr block_hnd_play
        pop     di
        mov     dl,DevHlp_ProcRun       ; Run Opcode
        call    dword ptr [DeviceHelp]  ; unblock waiting thread
        jmp     int_3                   ; exit int routine

; TEST: INT HDLR reagiert selbst auf den TDR, nur UNBLOCK bei FC end,
; PLAYback code hierher holen.
int_da: call    midi_dataout            ; ausgabe midi daten direkt
        jmp     int_3                   ; exit int routine

int_x:  mov     ah,mpu_mode             ; CHECK for RECORD MODE
        test    ah,MOD_REC
        jz      int_3                   ; return, if REC not active
; now store data to the Memory accessed by the GDT Selector which was
; set up by ioc_start_rec. The first two bytes of the buffer contain the
; pointer, at which the last byte was written. The initial pointer is 2
; and must be set up at start record time by ioc_start_rec function!!
        mov     cx,midi_rec_bufsz       ; CX = MIDI IN BufferSIZE
        dec     cx                      ; CX = Maximum OFFSET (size-1!)
        push    ds
        push    si
        cli               ; ????
        mov     ds,[Sel_REC]            ; establish GDT Selector access
        mov     si,0
        mov     dx,word ptr [ds:si]     ; DX = pointer to last written byte
        cmp     cx,dx                   ; compare to maximum offset
        jne     int_a                   ; if equal: turnaround, next byte
        mov     dx,1                    ; will be written at offset 2
int_a:  inc     dx                      ; increase by one for new position
        mov     si,dx                   ; DX = new offset          
        mov     byte ptr [ds:si],al     ; store MIDI Byte to Buffer
        mov     si,0
        mov     word ptr [ds:si],dx     ; store DX as new pointer
        pop     si
        pop     ds                      ; restore Datasegment
        sti              ; ??????
; 5. Acknowledge the 8259 Interrupt Controller Chips
int_3:  mov     al,mpu_irq
        mov     dl,DevHlp_EOI           ; EOI Function Code
        cli                             ; Disable interrupts
        call    dword ptr [DeviceHelp]  ; Call Device Help Routine
;       sti                             ; Enable Interrupts           ??????
        clc                             ; Clear Carry Flag!!
        pop     bx
int_en: pop     ax
        pop     cx
        pop     dx   
        pop     ds
        pop     si
        ret                             ; E X I T  Interrupt Handler
interrupt_hdlr  endp


;-------------------------------------------------------------------------------
; Entry Point to DEV_OPEN Routine of the Device Driver
; 1. Check open_oount : If not 0 : Driver already open, failure!!
; 2. DevOpen issues a RESET command to the MPU
; 3. DevOpen resets the pointers for the midi input buffer
;-------------------------------------------------------------------------------

dev_open    proc FAR
        push    bx                            ; Save Request packet offset
        mov     al,byte ptr [open_count]      ; Get open count value
        cmp     al,0
        jne     op_err                        ; If not 0: Error, already open
        mov     ah,RESET_CMD                  ; RESET Command code to AH
        call    send_command                  ; for passing to SEND_COMMAND
        cmp     ax,0                          ; check return code from Sendcomm.
        je      op_ok                         ; if not o.k. -> error
        pop     bx                            ; error in send_command
        mov     rp_status[bx],08121h          ; Error code returned to appl: 21h
        jmp     op_end                        ; Results in SYS0021 Message
op_err: mov     byte ptr [open_count],0       ; Set open count to 0
        pop     bx                            ; already open!!!
        mov     rp_status[bx],08120h          ; Error code returned to appl: 20h
        jmp     op_end                        ; Results in SYS0020 Message
op_ok:  mov     byte ptr [open_count],1       ; Set open count to 1
        pop     bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE
op_end: ret                                   ; end of strategy routine

dev_open        endp

;-------------------------------------------------------------------------------
; Entry Point to DEV_CLOSE Routine of the Device Driver
; 1. Check open count: if 0: Error, device not open
; 2. DevClose resets the MPU
;-------------------------------------------------------------------------------
                                                                                
dev_close   proc FAR
        push    bx                            ; Save Request packet offset 
        mov     al,byte ptr [open_count]      ; Get open count value          
        cmp     al,0
        je      cl_err                        ; If 0: Error
        mov     ah,RESET_CMD                  ; RESET Command to AH for passing
        call    send_command                  ; to SEND_COMMAND
        cmp     ax,0                          ; check return code               
        je      cl_ok                         ; if o.k. -> ready
cl_err: mov     byte ptr [open_count],0      
        pop     bx
        mov     rp_status[bx],08150h          ; Error code
        jmp     cl_end
cl_ok:  mov     byte ptr [open_count],0
        pop     bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE
cl_end: ret                                   ; end of strategy routine
dev_close       endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Routine of the Device Driver
; CATEGORY Code is 128. (10000000)
; Supported Function codes: 0  RESET MPU         ioc_reset_mpu       rm
;                           1  SET UART MODE     ioc_set_uart        su
;                           2  REQUEST VERSION   ioc_req_version     rv
;                           3  STOP PLAY         ioc_stop_play       pp
;                           4  BENDER ON/OFF     ioc_bender          be
;                           5  PLAYBACK          ioc_play            pl
;                           6  START RECORD      ioc_start_rec       sr
;                           7  STOP RECORD       ioc_stop_rec        pr
;                           8  MEASURE END       ioc_meas_end        me
;                           9  SET TIMEBASE      ioc_timebase        tb
;                           10 EXCLUSIVE TO HOST ioc_excl2host       ex
;                           11 REALTIME AFF. ON  ioc_realtime_aff    rt
;                           12 MIDI THRU ON/OFF  ioc_midi_thru       mt
;                           13 SET FADER VALUES  ioc_set_fader       sf
;                           14 OVERDUB           ioc_overdub         ov
;                           15 STOP OVERDUB      ioc_stop_overdub    po
;                           16 SEND DATA         ioc_send_data       sn
;                           17 SET TEMPO         ioc_set_tempo       st
;                           18 SET REL TEMPO     ioc_set_rel_tempo   so
;                           19 SET GRADUATION    ioc_set_graduation  sg
;                           20 MIDI PER METRO    ioc_midi_per_metro  mm
;                           21 CHANNEL_CHANGE    ioc_chan_change     cc
;                           22 SET ACTIVE TRK    ioc_set_acttrk      sa
;                           23 SETUP NOTETAB GDT ioc_gdt_notetab     gn
;                           24 FREE NOTETAB GDT  ioc_free_notetab    fn
;
; If the MPU is in UART Mode, only the reset command can be passed to the MPU.
; Trying to send any other command results in a return code.
;-------------------------------------------------------------------------------
                                                                                
ioctl       proc FAR
        ASSUME  CS:cseg,DS:dseg,ES:rp         ;ES:BX is the Reqest Packet Addr. 
        push    es                            ; Save request packet address
        push    bx
        mov     al,rp_ioctl_categ[bx]         ; get category code
        cmp     al,mpu_category               ; check for correct category code,
        jz      ic_2                          ; if o.k.  proceed
        mov     rp_status[bx],RP_IC_INV_CAT   ; error: invalid category code
        jmp     ic_end
ic_2:   mov     al,rp_ioctl_funct[bx]         ; get function code
        cmp     al,ioctl_tabsize              ; compare code to tabsize
        jbe     ic_3                          ; if code <= tabsize: funktion ok
        mov     rp_status[bx],RP_IC_INV_FNC   ; error: invalid function code
        jmp     ic_end                                                          
ic_3:   cbw                                   ; convert to word
        shl     ax,1                          ; double value for index to tab
        mov     di,ax                         ; build index
        call    word ptr ioctl_functab[di]    ; call the ioctl subfunction
ic_end: pop     bx
        pop     es
        ret
ioctl           endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Reset MPU function
; IOCtl function Code is 0
; This function resets the MPU to an initial state.
; Also all variables are reset to default values                        !!!!!!
; No Parameters are passed to this function, no results are given back. 
;-------------------------------------------------------------------------------

ioc_reset_mpu proc NEAR
        push    es                            ; Save Request Packet address
        push    dx
        push    cx
        push    bx
; first check, if RECORD or PLAY are active, this results in error
        mov     al,mpu_mode
        test    al,MOD_REC
        jnz     rm_e1
        test    al,MOD_PLAY
        jnz     rm_e2
        mov     ah,RESET_CMD                  ; Reset MPU to intelligent Mode
        call    send_command                  ; with all defaults
        cmp     ax,0
        jne     rm_err
        mov     ah,DISABLE_CHREFTABA          ; Disable Channel Reference Table
        call    send_command          
        cmp     ax,0                                                          
        jne     rm_err               
        mov     ah,DISABLE_CHREFTABB          ; Disable Channel Reference Table
        call    send_command                                                   
        cmp     ax,0                                                           
        jne     rm_err                                                         
        mov     ah,DISABLE_CHREFTABC          ; Disable Channel Reference Table
        call    send_command                                                   
        cmp     ax,0                                                           
        jne     rm_err                                                                                                  
        mov     ah,DISABLE_CHREFTABD          ; Disable Channel Reference Table
        call    send_command          
        cmp     ax,0                                                          
        je      rm_ok
rm_err: mov     rp_status[bx],RP_RM_RES_F     ; error: reset command failed
        jmp     rm_end
rm_e1:  mov     rp_status[bx],RP_RM_RECAC
        jmp     rm_end
rm_e2:  mov     rp_status[bx],RP_RM_PLYAC
        jmp     rm_end
rm_ok:  mov     rp_status[bx],RP_DONE         ; RP_DONE
        mov     mpu_mode,MOD_DEFAULT          ; restore actual mode to default
        mov     mpu_mode2,MOD2_DEFAULT        ; restore Mode2 to default
        mov     mpu_act_trk,00000000b         ; reset active tracks
        mov     ma_mode_rec,MA_NEW            ; reset MIDI Analyze Mode
        mov     ma_mode_play,MA_NEW
        mov     st_tempo,064h                 ; reset tempo to default
        mov     so_tempo,040h                 ; reset relative tempo to default
        mov     sg_graduation,0               ; reset graduation to default
        mov     mm_midimetro,0C0h             ; MIDI per METRO default
        mov     tb_timebase,078h              ; Timebase default value
        mov     cx,16
        mov     bx,16
        mov     dl,16
rm_lp:  dec     bx                            ; init channel tab (loop 16)
        dec     dl
        loop    rm_lp
        mov     cx,64
        mov     bx,64
rm_lp2: dec     bx
        mov     cc_chantab[bx],16             ; init channel table to MULT
        mov     sa_acttrk[bx],0               ; init active track mask
        loop    rm_lp2
rm_end: pop     bx
        pop     cx
        pop     dx
        pop     es
        ret
ioc_reset_mpu endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Set UART Mode function
; IOCtl function Code is 1
; This Function sets the MPU to the UART Mode. This mode can only be left by
; sending a reset command to the MPU.
; No Parameters are passed to this function, no results are given back.
;-------------------------------------------------------------------------------

ioc_set_uart proc NEAR
        push    es                            ; Save Request Packet address
        push    bx
; first check, if RECORD or PLAY are active, this results in error
        mov     al,mpu_mode
        test    al,MOD_REC
        jnz     su_e1
        test    al,MOD_PLAY
        jnz     su_e2
        mov     ah,UART_MODE                  ; Set MPU to UART mode
        call    send_command                  
        cmp     ax,0
        je      su_ok
su_err: mov     rp_status[bx],RP_SU_UART_F    ; error: UART Mode command failed
        jmp     su_end
su_e1:  mov     rp_status[bx],RP_SU_RECAC
        jmp     su_end
su_e2:  mov     rp_status[bx],RP_SU_PLYAC
        jmp     su_end
su_ok:  mov     rp_status[bx],RP_DONE         ; RP_DONE
        mov     mpu_mode,MOD_UART             ; UART MODE, no play or record
su_end: pop     bx
        pop     es
        ret
ioc_set_uart    endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Request Version Function
; IOCtl function Code is 2
; 1. Get VERSION and REVISION from MPU by send_command
; 2. Lock the virtual address of the data buffer and verify access 6 bytes R/W
; 3. Copy requested data to application buffer
; 4. Unlock the address
; Passed parameters: Virtual address of 6 Byte data structure of application
; Return values    : 6 Bytes with the requested data written to this address
;                    MPU Version (high/low),
;                    MPU Revision (high/low),
;                    Driver Version and Driver Revision
;-------------------------------------------------------------------------------

ioc_req_version proc NEAR
        push    dx
        push    di
        push    cx
        push    es                            ; Save Request Packet address
        push    bx 
; 1. Get Version and Revision from MPU401
        mov     ah,REQUEST_VER                ; Request Version Command Code
        call    send_command                                                    
        cmp     ax,-1                         ; timeout error from send_command
        jne     rv_a
        mov     rp_status[bx],RP_RV_VER_F     ; error: Request version failed
        pop     bx
        jmp     rv_end
rv_a:   mov     ah,al                         
        and     al,00001111b                  ; Mask out higher nibble
        mov     mpu_vers_lo,al                ; Store mpu version, lower part
        mov     al,ah
        and     al,11110000b                  ; Mask out lower nibble
        mov     cl,4                          ; shift right 4 bits
        shr     al,cl
        mov     mpu_vers_hi,al                ; Store mpu version, higher part
        mov     ah,REQUEST_REV                ; Request Revision Command Code
        call    send_command                                                    
        cmp     ax,-1                         ; timeout error from send_command 
        jne     rv_b
        mov     rp_status[bx],RP_RV_REV_F     ; error: Request revision failed
        pop     bx                                                              
        jmp     rv_end 
rv_b:   mov     ah,al
        and     al,00001111b                  ; Mask out higher nibble         
        mov     mpu_rev_lo,al                 ; Store MPU revision, lower part
        mov     al,ah
        and     al,11110000b                  ; Mask out lower nibble
        mov     cl,4                          ; shift right 4 bits             
        shr     al,cl                                                          
        mov     mpu_rev_hi,al                 ; Store mpu revision, higher part
; 2. LOCK and VERIFY the address of data buffer passed by request packet
        mov     di,word ptr rp_ioctl_dbuf[bx+2] ; get Selector of virtual addr
        mov     bl,0                          ; block until available
        mov     bh,0                          ; short term, any memory
        mov     cx,6                          ; length of 6 bytes
        mov     dh,1                          ; read and write access
        call    lock_verify                   ; call lock_verify subfunction
        cmp     ax,RP_DONE
        je      rv_2                          ; if o.k...
        pop     bx                            ; restore BX
        mov     rp_status[bx],ax              ; set request packet status
        jmp     rv_end                        ; End function with error
rv_2:   mov     word ptr ioc_lock_hnd1,cx     ; store lock handle from cx:dx
        mov     di,2                          ; returned by lock_verify for
        mov     word ptr ioc_lock_hnd1[di],dx ; unlock after writing data
; 3. Store the requested data in the Application buffer
        pop     bx
        push    bx
        push    ds
        push    si
        mov     ch,mpu_vers_hi
        mov     cl,mpu_vers_lo
        mov     ah,mpu_rev_hi
        mov     al,mpu_rev_lo
        cli
        lds     si,rp_ioctl_dbuf[bx]          ; address to ds:si
        mov     byte ptr ds:[si],ch           ; store mpu version high
        mov     byte ptr ds:[si+1],cl         ; store mpu version low
        mov     byte ptr ds:[si+2],ah         ; store mpu revision high
        mov     byte ptr ds:[si+3],al         ; store mpu revision low
        mov     byte ptr ds:[si+4],drv_vers   ; store driver version
        mov     byte ptr ds:[si+5],drv_rev    ; store driver revision
        pop     si
        pop     ds
        sti
; 4. Unlock the address
; This part is processed, if lock and verify access were successful and the
; driver has put values into the memory
        mov     di,2
        mov     ax,word ptr ioc_lock_hnd1     ; restore lock handle
        mov     bx,word ptr ioc_lock_hnd1[di]
        mov     dl,DevHlp_Unlock
        call    dword ptr [DeviceHelp]        ; Call lock function             
        jnc     rv_ok
rv_err: pop     bx
        mov     rp_status[bx],RP_UNL_FAIL     ; error: Unlock failed
        jmp     rv_end
rv_ok:  pop     bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE
        mov     rp_ioctl_dbufl[bx],4          ; T E S T
rv_end: pop     es
        pop     cx
        pop     di
        pop     dx
        ret
ioc_req_version    endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Stop Play function
; IOCtl function Code is 3
; This Function stops the playback from MPU, started by ioc_play.
; The playback is stopped by 
; 1. reset mpu_mode to inactive playback: this prevents the int-hdlr from
;    processing Track Data Requests.
; 2. Copy stop value to Track Data Request field: 0FCh
;    This is also the value, the MPU sends to the HOST, when the last track has
;    normally ended playing without interrupting STOP_RECORD command.
; 3. Unblock the ioc_play thread: the stop code will be found by ioc_play
;    and the function will properly terminate
;-------------------------------------------------------------------------------
 
ioc_stop_play    proc NEAR
        push    dx
        push    cx
        push    es                            ; Save Request Packet address
        push    di
        push    bx
; Check if PLAY mode is active (bit 5 in MPU_MODE = 1). If not: ERROR
        test    mpu_mode,MOD_PLAY             ; test the bit 5 in mpu_mode
        jnz     pp_2                          ; if not in play mode (0)
        pop     bx
        mov     rp_status[bx],RP_PP_PLIN      ; error code, PLAY Mode inact
        jmp     pp_end
pp_2:   mov     al,MOD_PLAY                   ; reset mpu_mode to prevent, that
        not     al                            ; the interrupt handler will copy
        and     mpu_mode,al                   ; more track data requests
        mov     trk_data_req,PLAY_STOP_CODE   ; Stop Code to TDR
; now unblock the waiting PLAY thread (ioc_play function)
        push    di
        mov     di,2
        mov     bx,word ptr block_hnd_play[di]
        mov     ax,word ptr block_hnd_play
        pop     di
        mov     dl,DevHlp_ProcRun             ; AX will contain no. of threads
        call    dword ptr [DeviceHelp]        ; unblock waiting thread
pp_ok:  pop     bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE       
pp_end: pop     di                                            
        pop     es                                            
        pop     cx                                             
        pop     dx
        ret
ioc_stop_play    endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Set Bender on/off Function
; IOCtl function Code is 4
; This Function activates or deactivates the MPU BENDER Messages (Exh) to be
; sent to the host with a leading timing byte.                                  
; Activation is done with MPU command ENABLE_BENDER (87h)
; Deactivation is done with MPU command DISABLE_BENDER (86h)
; Parameters passed: 1 Byte in IOCTL Parms: 1: Activate, 0: Deactivate          
;-------------------------------------------------------------------------------

ioc_bender      proc NEAR
        push    es                            ; Save Request Packet address
        push    di
        push    cx
        push    dx
        push    bx
; call get_ioctl_parms function: DI = Offset of variable-chain where
; the parms must be stored, CX = number of bytes in Parmbuf
        lea     di,bender_switch              ; offset to DI (destination)
        mov     cx,1                          ; 1 byte to transfer
        call    get_ioctl_parms               ; get parameters
; The request packet parameters are now stored. Check with actual Mode
; and refuse command, if command refers to a state already set
        pop     bx                            ; RP Address restore!
        cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms
        je      be_5
        mov     rp_status[bx],ax              ; Set RP error status
        jmp     be_end                        ; return
be_5:   mov     dh,bender_switch              ; DH = passed parameter switch
        mov     ax,mpu_mode2                  ; actual mode
        test    ax,MOD_BENDER
        jnz     be_ac                         ; if not 0: BENDER active
be_in:  cmp     dh,1                          ; checking for inactive
        jnz     be_e1                         ; if not enable cmd: error
; calculate the new mpu_mode2 word and store in CX until send_command o.k.
        mov     di,mpu_mode2
        or      di,MOD_BENDER                 ; set the bit in mpu_mode2
        mov     ah,ENABLE_BENDER              ; else: enable bender
        call    send_command                  ; send command to MPU
        cmp     ax,0
        je      be_ok
        jmp     be_err                        ; send command failed
be_ac:  cmp     dh,0                          ; checking for active, if not
        jnz     be_e1                         ; disable cmd: error
; calculate the new mpu_mode2 word and store in CX until send_command o.k.
        mov     cx,MOD_BENDER
        not     cx                            ; invert the mask
        mov     di,mpu_mode2
        and     di,cx                         ; clear the bit in mpu_mode2
        mov     ah,DISABLE_BENDER             ; else; disable bender
        call    send_command                  ; send command to MPU
        cmp     ax,0
        je      be_ok
be_err: mov     rp_status[bx],RP_BE_BEND_F    ; error: Bender comm. fail.
        jmp     be_end
be_e1:  mov     rp_status[bx],RP_BE_ALRSET    ; error: BENDER already set
        jmp     be_end
be_ok:  mov     rp_status[bx],RP_DONE         ; RP_DONE
; now set the MPU_MODE2 word to the new state calculated before
        mov     mpu_mode2,di                  ; activate new mpu_mode2
be_end: pop     dx
        pop     cx
        pop     di
        pop     es
        ret
ioc_bender      endp
 
;-------------------------------------------------------------------------------
; Entry Point to IOCTL Play function
; IOCtl function Code is 5
; This function is responsible for playback of recorded midi data.
; The function sets the playmode active and the interrupt handler will transmit
; the track data requests (TDR) from the mpu.
; Passed parameters: IOCtl Parms: Active Track Mask
;                                 PLAY Buffer Size
; Return values: NONE
; Actions taken in this function:
; 1. No more parameters are used.
; 2. Map the Virtual address of the Application Data Buffer to a GDT Selector
;    which was allocated at INIT time. So the driver has access to the buffer
;    at any time. (see also ioc_start_rec) Use Sel_PLAY.
; 3. Set mode to PLAY active (mpu_mode). This enables the interrupt handler to
;    send TDR received from the MPU to the TDR queue.
; 4. Set the active tracks to the desired values (send command to MPU)
; 5. Send clear play counters to MPU.
; 6. Start the PLAY function by sending a command to the MPU. This starts the
;    playback and the MPU will begin to send TDRs.
; 7. Now begin the TDR Queue processing: If no request in Queue: BLOCK
; 8. When unblocked, take TDR and process the output to the MPU.
; 9. Stop play processing, if STOPCODE is found in TDR Queue.
;10. After processing a TDR: look for other TDR in queue -> goto 6.
;-------------------------------------------------------------------------------

ioc_play        proc NEAR
        push    dx                                                         
        push    cx                                                         
        push    es                            ; Save Request Packet address
        push    di
        push    bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE   default, if all o.k.
; Check if PLAY mode is active (bit 5 in MPU_MODE = 1)

; I M P O R T A N T: Also error, if in Record Mode!!!

        test    mpu_mode,MOD_PLAY             ; test the bit 5 in mpu_mode
        jz      pl_6                          ; if not in play mode (0)
        pop     bx
        mov     rp_status[bx],RP_AT_PLAY      ; error code, already in PLAY Mode
        jmp     pl_end
; now map the data buffer address to the GDT selector Sel_PLAY
; 1. Lock the data buffer address and verify access for midi_play_bufsz bytes
pl_6:   pop     bx                            ; restore BX register for request
        push    bx                            ; packet access
        mov     di,word ptr rp_ioctl_dbuf[bx+2] ; get Selector of virtual addr
        mov     bl,0                          ; block until available
        mov     bh,1                          ; long term, any memory
        mov     cx,midi_play_bufsz            ; length of buffersize
        mov     dh,0                          ; read access
        call    lock_verify                   ; call lock_verify subfunction
        cmp     ax,RP_DONE
        je      pl_7                          ; if o.k...
        pop     bx                            ; restore BX
        mov     rp_status[bx],ax              ; set request packet status
        jmp     pl_end                        ; End function with error
pl_7:   mov     word ptr ioc_lock_hnd2,cx     ; store lock handle from cx:dx
        mov     di,2                          ; returned by lock_verify for
        mov     word ptr ioc_lock_hnd2[di],dx ; unlock in ioc_stop_play
; lock and verify access are done, now virtophys
; IMPORTANT!!: cause DS is used for parameter passing, the addressing of
; DeviceHelp is realized by ES
        pop     bx                            ; restore BX register for request
        push    bx                            ; packet access
        push    si                            ; S T A R T  Block VirtToPhys
        push    ds
        push    es
        push    ds                            ; Save DS for exchange
        cli
        lds     si,rp_ioctl_dbuf[bx]          ; MIDI buffer address to DS:SI
        pop     es                            ; CHANGE ES to DS contents
        ASSUME  es:DSEG,ds:NOTHING            ; ES accesses DSEG for call
        mov     dl,DevHlp_VirtToPhys          ; Function code
        call    dword ptr [DeviceHelp]        ; Call VirtToPhys function
        ASSUME  ds:DSEG,es:rp                 ; VirtToPhys returns the
        pop     es                            ; physical address in AX:BX
        pop     ds
        sti
        pop     si                            ; E N D   Block   Virt2phys
        jnc     pl_8                          ; if O.K. proceed
        pop     bx
        mov     rp_status[bx],RP_PL_V2P       ; VirtToPhys failed
        jmp     pl_end
; Virt to Phys done, now map to GDT Selector. PhysToGDTSelector expects the
; physical address to be mapped in AX:BX register pair
; first store the physical address as block handle in block_hnd_play
pl_8:   mov     word ptr block_hnd_play,ax    ; first store the phys. address
        mov     di,2                          ; for use as block handle
        mov     word ptr block_hnd_play[di],bx
        mov     cx,midi_play_bufsz            ; Segment Size
        push    si
        mov     si,Sel_PLAY                   ; GDT Selector to be setup
        mov     dl,DevHlp_PhysToGDTSelector   ; function code
        call    dword ptr [DeviceHelp]        ; call PhysToGDTSelector
        pop     si                            ; if error: AX contains code
        jnc     pl_9
        pop     bx                            ; PhysToGDTSelector failed
        mov     rp_status[bx],RP_PL_GDTFAIL
        jmp     pl_end
; GDT Selector Mapping done. Now the Playbuffer is accessible through Sel_PLAY
; now send activate track command to MPU with active track mask
pl_9:   mov     ah,ACTIVATE_TRK               ; Activate Tracks command code
        call    send_command                  ; send to MPU
        cmp     ax,0
        jne     pl_a                          ; if error:
        mov     ah,1                          ; else: active track always 1
        cli
        call    send_data
        sti
        cmp     ax,0
        je      pl_b                          ; if o.k. -> proceed
        pop     bx
        mov     rp_status[bx],RP_PL_SND_F     ; error: Send Data failed
        jmp     pl_end
pl_a:   pop     bx
        mov     rp_status[bx],RP_PL_ACT_F     ; error: Activate Track failed
        jmp     pl_end                        ; error in ACTIVATE_TRK command
; activate track command has completed
; now send clear play counter command to mpu
pl_b:   mov     ah,CLR_PLAY_CNT               ; 'Clear Play Counter' command
        call    send_command
        cmp     ax,0
        je      pl_c
        pop     bx
        mov     rp_status[bx],RP_PL_CLR_F     ; Clear Play Counter failed
        jmp     pl_end
; now initialize value for track data request byte to 0 and reset runstat.
pl_c:   mov     trk_data_req,0
        mov     byte ptr ma_runst_play,0
        .386P
        mov     dword ptr pl_delta_time,0
        mov     dword ptr pl_abs_time,0
        .286P
; now set mpu_mode variable: bit 5 = 1. Int Hdlr now reacts upon TDRs from MPU
        or      mpu_mode,MOD_PLAY
; now read start offset from bytes 1+2 (must be set by application!!!).
; For normal start, this will be 6. Copy the value to pl_offset, a working
; variable. (bytes 1+2 only used for feedback to application)
; NEW: also read bytes 3-6, containing accumulated timing value. This is also
; initialized by the application at start playback. Copy this value to
; pl_accu_time.
        .386P
        push    ecx
        push    es
        mov     es,[Sel_PLAY]                 ; establish GDT Selector access
        mov     di,0                          ; trough ES:DI register pair
        mov     ax,word ptr [es:di]           ; AX = initial offset
        mov     di,2                          ; offset accu timing
        mov     ecx,dword ptr [es:di]         ; initial timing value from appl
        pop     es                            ; restore ES register
        mov     pl_offset,ax                  ; store pl_offset
        mov     pl_accu_time,ecx
        pop     ecx  
        .286P
; now start MPU playback (MPU will begin to send Track Data Requests)
        mov     ah,START_PLAY                 ; 'Start Play' command
        call    send_command
        cmp     ax,0
        je      pl_d
        pop     bx
        mov     rp_status[bx],RP_PL_FAIL      ; Start Play Command failed
        jmp     pl_end                        ; pl_end also resets mode
; MPU recording started, now wait for TDR and output data. If TDR = 0FCh
; return from function: FCh is the Stop code set by ioc_stop_play
; S T A R T  OF ENDLESS LOOP

; -----------------------------------------------------------------------------
;       \\//     E N D L E S S    L O O P
;        \/
; This is the entrypoint to an endless-loop. This code runs in a second thread
; at ring 0 and can only be stopped by an unblock with x'FC' in trk_data_req.

pl_d:   mov     al,trk_data_req               ; check before BLOCK
        cmp     al,0
        jnz     pl_e                          ; request or stopcode
; at this point: no TDR is outstanding, also no stop code: BLOCK the thread
; and wait, until the interrupt handler passes data or the ioc_stop_rec
; passes a stop code
        mov     ax,word ptr block_hnd_play
        mov     di,2
        mov     bx,word ptr block_hnd_play[di]
        mov     di,-1
        mov     cx,-1                         ; block until run
        mov     dh,0                          ; interruptible
        cli                                   ; disable interrupts !!!!!
        mov     dl,DevHlp_ProcBlock           ; Block reenables interrupts
        call    dword ptr [DeviceHelp]        ; Block the thread....

; -----------------------------------------------------------------------------
;            \      /
;             \    /        Entry Point at Unblock from Interrupt Hander
;              \  /         when Track Data Request occurs or at Unblock from
;               \/          ioc_stop_play when Stop Play is requested

; At this point: some data is in the TDR field: TDR or stop code. Otherwise
; generate error message. This point is reached, if UNBLOCK occurs.
pl_e:   mov     al,trk_data_req               ; store datafield
        mov     trk_data_req,0                ; clear datafield
        cmp     al,PLAY_STOP_CODE
        je      pl_h                          ; jump to stop processing
; at this point: possible to check for other data, error if unexpected
        pop     bx                            ; e r r o r  unexpected data
        push    bx
        mov     rp_status[bx],RP_PL_INV_TDR   ; invalid TDR (unknown data)
        mov     ah,STOP_PLAY
        call    send_command
        cmp     ax,0
        jz      pl_k                          ; UNLOCK Buffer Address
        pop     bx
        mov     rp_status[bx],RP_PL_STOP_F2   ; error code: stop play failed
        jmp     pl_end
; here: possible to process other data (maybe unblock at accutiming update)
; to enable the application to directly get active in this case.
        jmp     pl_d                          ; E N D L E S S  Loop

;        /\      E N D L E S S    L O O P
;       //\\
; -----------------------------------------------------------------------------

; STOP CODE found in Track Data field: STOP Play processing. This is the only
; way to leave the pl_d loop (except for an error in send_data)
; just send a stop play command to MPU
pl_h:   mov     ah,STOP_PLAY                  ; AH = stop play command
        call    send_command                  ; send stop play command
        cmp     ax,0
        jz      pl_k
        pop     bx
        mov     rp_status[bx],RP_PL_STOP_F1   ; Stop play failed
        jmp     pl_end

; now Unlock the address of the application buffer
pl_k:   mov     ax,word ptr ioc_lock_hnd2     ; restore lock handle
        mov     di,2
        mov     bx,word ptr ioc_lock_hnd2[di] ; DI = 2
        mov     dl,DevHlp_Unlock
        call    dword ptr [DeviceHelp]        ; Call unlock function
        jc      pl_k1                         ; if error..
        pop     bx
        jmp     pl_end

; error processing from send_data  copied from before pl_h (jumps out of range)
pl_i:   pop     bx                                                            
        mov     rp_status[bx],RP_PL_SND_FAIL  ; error code                    
        jmp     pl_end                                                        


pl_k1:  pop     bx                            ; Restore Request packet Address
        mov     rp_status[bx],RP_PL_UNLFAIL   ; error: Unlock failed
        jmp     pl_end
pl_ok:  pop     bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE
pl_end: mov     al,MOD_PLAY                   ; reset mpu_mode
        not     al
        and     mpu_mode,al                   ; reset bit 5
        pop     di
        pop     es
        pop     cx
        pop     dx
        ret
ioc_play        endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Start Record function
; IOCtl function Code is 6
; This function STARTS the record function of MPU401. It enables the Interrupt
; Handler to process Data sent from the MPU by activating the REC-Bit in the
; MPU_MODE variable and establishing access to the CAPTURE Buffer through a
; GDT Selector.
; Passed parameters:
; WORD  Appl. buffer size in IOCtl Parms, store in variable midi_rec_bufsz
; Return values: NONE                                                           
; Actions taken in this function:                                               
; 1. Check MPU_MODE: when in PLAY or RECORD Mode: error to application
; 2. Check, that at least one track is set to active                            
;-------------------------------------------------------------------------------
                                                                                
ioc_start_rec   proc NEAR
        push    dx
        push    cx
        push    es                            ; Save Request Packet address
        push    di
        push    bx
; 1. Check if PLAY mode is active (bit 5 in MPU_MODE = 1)
        test    mpu_mode,MOD_PLAY             ; test the bit 5 in mpu_mode
        jz      sr_1                          ; if not in play mode (0)
        pop     bx
        mov     rp_status[bx],RP_SR_PLYAC     ; error code, MPU in PLAY Mode
        jmp     sr_end
sr_1:   test    mpu_mode,MOD_REC              ; test bit 6 in mpu_mode
        jz      sr_4                   ;!!!!    if not in record mode (0)
        pop     bx
        mov     rp_status[bx],RP_SR_RECAC     ; error code, already in REC Mode
        jmp     sr_end
; call get_ioctl_parms function: DI = Offset of variable-chain where
; the parms must be stored, CX = number of bytes in Parmbuf
; sr_2:   lea     di,midi_rec_bufsz             ; offset to DI (destination)
;         mov     cx,2                          ; 2 bytes to transfer
;         call    get_ioctl_parms               ; get parameters
;         cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms
;         je      sr_4
;         pop     bx                            ; RP Address restore!
;         mov     rp_status[bx],ax              ; Set RP error status
;         jmp     sr_end                        ; return
; At this point, the Application Buffer Size is stored in midi_rec_bufsz
; Now the Databuffer must be mapped to the GDT selector Sel_REC
; 1. Lock the data buffer address and verify access for midi_rec_bufsz bytes
sr_4:   pop     bx                            ; restore BX register for request
        push    bx                            ; packet access
        mov     di,word ptr rp_ioctl_dbuf[bx+2] ; get Selector of virtual addr
        mov     bl,0                          ; block until available           
        mov     bh,1                          ; long term, any memory
        mov     cx,midi_rec_bufsz             ; length of buffersize
        mov     dh,1                          ; write access
        call    lock_verify                   ; call lock_verify subfunction
        cmp     ax,RP_DONE
        je      sr_5                          ; if o.k...
        pop     bx                            ; restore BX
        mov     rp_status[bx],ax              ; set request packet status
        jmp     sr_end                        ; End function with error
sr_5:   mov     word ptr ioc_lock_hnd3,cx     ; store lock handle from cx:dx
        mov     di,2                          ; returned by lock_verify for
        mov     word ptr ioc_lock_hnd3[di],dx ; unlock after stop record
; lock and verify access are done, now virtophys
; IMPORTANT!!: cause DS is used for parameter passing, the addressing of
; DeviceHelp is realized by ES
        pop     bx                            ; restore BX register for request
        push    bx                            ; packet access
        push    si                            ; S T A R T  Block VirtToPhys
        push    ds
        push    es
        push    ds                            ; Save DS for exchange
        cli
        lds     si,rp_ioctl_dbuf[bx]          ; MIDI buffer address to DS:SI
        pop     es                            ; CHANGE ES to DS contents
        ASSUME  es:DSEG,ds:NOTHING            ; ES accesses DSEG for call
        mov     dl,DevHlp_VirtToPhys          ; Function code
        call    dword ptr [DeviceHelp]        ; Call VirtToPhys function
        ASSUME  ds:DSEG,es:rp                 ; VirtToPhys returns the
        pop     es                            ; physical address in AX:BX
        pop     ds
        sti
        pop     si                            ; E N D   Block   Virt2phys
        jnc     sr_6                          ; if O.K. proceed
        pop     bx
        mov     rp_status[bx],RP_SR_V2P       ; VirtToPhys failed
        jmp     sr_end
; Virt to Phys done, now map to GDT Selector. PhysToGDTSelector expects the
; physical address to be mapped in AX:BX register pair
sr_6:   mov     cx,midi_rec_bufsz             ; Segment Size
        push    si
        mov     si,Sel_REC                    ; GDT Selector to be setup
        mov     dl,DevHlp_PhysToGDTSelector   ; function code
        call    dword ptr [DeviceHelp]        ; call PhysToGDTSelector
        pop     si                            ; if error: AX contains code
        jnc     sr_7
        pop     bx                            ; PhysToGDTSelector failed 
        mov     rp_status[bx],RP_SR_GDTFAIL
        jmp     sr_end
; GDT Selector Mapping done, now init initial offset and start MPU Record
sr_7:
; now first copy 1 to the first two bytes in databuffer, this is the initial
; offset for use by the interrupt handler. Then send START_REC command to the
; MPU. If START_REC succeeds: set MOD_REC bit to active. This enables the
; interrupt handler collecting data to the MIDI buffer.
        push    ds
        cli
        mov     ds,[Sel_REC]                  ; establish GDT Selector access
        mov     si,0
        mov     ax,1
        mov     word ptr [ds:si],ax           ; copy 1 to initial offset
        pop     ds
        sti
; die n„chsten 4 Zeilen auf kommentar halten den int handler ruhig
        mov     ah,START_REC                  ; 'Start Record' command
        call    send_command
        cmp     ax,0
        je      sr_ok
; Unlock the address of the data buffer, if START_REC command in error
        mov     ax,word ptr ioc_lock_hnd3     ; restore lock handle 
        mov     di,2
        mov     bx,word ptr ioc_lock_hnd3[di] ; DI = 2                        
        mov     dl,DevHlp_Unlock                                              
        call    dword ptr [DeviceHelp]        ; Call unlock function          
        jnc     sr_g                          ; if o.k. proceed
        pop     bx                            ; Restore Request packet Address
        mov     rp_status[bx],RP_UNL_FAIL     ; error: Unlock failed          
        jmp     sr_end                                                        
sr_g:   pop     bx
        mov     rp_status[bx],RP_SR_FAIL      ; Start Record Command failed
        jmp     sr_end
sr_ok:  pop     bx
        or      mpu_mode,MOD_REC              ; set record mode active
        mov     rp_status[bx],RP_DONE         ; RP_DONE
sr_end: pop     di 
        pop     es
        pop     cx 
        pop     dx
        ret
ioc_start_rec   endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Stop Record function
; IOCtl function Code is 7
; This function stops the record function of MPU401.
; Passed parameters: NONE
; Return values: The first 2 bytes in the CAPTURE BUFFER point to the last byte
;                written by the Int-Hdlr.
;-------------------------------------------------------------------------------

ioc_stop_rec    proc NEAR
        push    dx
        push    cx
        push    es                            ; Save Request Packet address
        push    di
        push    si
        push    bx
; Check if REC mode is active (bit 6 in MPU_MODE = 1). If not: ERROR
        test    mpu_mode,MOD_REC              ; test the bit 6 in mpu_mode
        jnz     pr_2                          
        pop     bx                            ; if not in rec mode (0)
        mov     rp_status[bx],RP_PR_RCIN      ; error code, REC Mode inact
        jmp     pr_end
; now send stop_record command to mpu
pr_2:   mov     ah,STOP_REC
        call    send_command
        cmp     ax,0
        je      pr_3
        pop     bx                            ; if not in rec mode (0)
        mov     rp_status[bx],RP_PR_SCF       ; send command failed
        jmp     pr_end
pr_3:   mov     cl,MOD_REC
        not     cl                            ; invert mask
        and     mpu_mode,cl                   ; reset record mode bit
; now copy final timing byte and FC Data End to Buffer, because the data
; cannot be received from MPU. MPU DOES send timing and FCh, but int Hdlr is
; disabled, so that data is thrown away. For this reason, the function must
; append the data 'manually'. Final Timing byte chosen to be 120.
        mov     cx,midi_rec_bufsz       ; CX = MIDI IN BufferSIZE
        dec     cx                      ; CX = Maximum OFFSET (size-1!)
        push    ds
        cli
        mov     ds,[Sel_REC]            ; establish GDT Selector access
        mov     si,0
        mov     dx,word ptr [ds:si]     ; DX = pointer in bytes 1+2
        inc     dx                      ; increase by one for new position
        mov     si,dx                   ; DX = new offset
        mov     byte ptr [ds:si],120    ; store final timing byte to Buffer
        cmp     cx,dx                   ; Also check for turnaround
        jne     pr_4
        mov     dx,1
pr_4:   inc     dx
        mov     si,dx                   ; DX = new offset
        mov     byte ptr [ds:si],0FCh   ; store DATA END Code to Buffer
        mov     si,0
        mov     word ptr [ds:si],dx     ; store DX as new pointer
        pop     ds                      ; restore Datasegment
        sti
; now Unlock the address of the application buffer
        mov     ax,word ptr ioc_lock_hnd3     ; restore lock handle
        mov     di,2
        mov     bx,word ptr ioc_lock_hnd3[di] ; DI = 2
        mov     dl,DevHlp_Unlock
        call    dword ptr [DeviceHelp]        ; Call unlock function
        jnc     pr_ok                         ; if o.k. proceed
        pop     bx                            ; Restore Request packet Address
        mov     rp_status[bx],RP_PR_UNLFAIL   ; error: Unlock failed
        jmp     pr_end
pr_ok:  pop     bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE
pr_end: pop     si
        pop     di
        pop     es
        pop     cx
        pop     dx
        ret
ioc_stop_rec    endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Measure End function
; IOCtl function Code is 8
; This Function activates or deactivates the MPU MARKS MEASURE END (F9h) to be
; sent to the host with a leading timing byte.
; Activation is done with MPU command ENABLE_MEAS_END (8Dh)
; Deactivation is done with MPU command DISABLE_MEAS_END (8Ch)
; Parameters passed: 1 Byte in IOCTL Parms: 1: Activate, 0: Deactivate
;-------------------------------------------------------------------------------
                                                                                
ioc_meas_end proc NEAR
        push    es                            ; Save Request Packet address
        push    di
        push    cx
        push    dx
        push    bx
; call get_ioctl_parms function: DI = Offset of variable-chain where
; the parms must be stored, CX = number of bytes in Parmbuf
        lea     di,measend_switch             ; offset to DI (destination)
        mov     cx,1                          ; 1 byte to transfer
        call    get_ioctl_parms               ; get parameters
; The request packet parameters are now stored. Check with actual Mode
; and refuse command, if command refers to a state already set
        pop     bx                            ; RP Address restore!
        cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms
        je      me_5
        mov     rp_status[bx],ax              ; Set RP error status
        jmp     me_end                        ; return
me_5:   mov     dh,measend_switch             ; DH = passed parameter switch
        mov     ax,mpu_mode2                  ; actual mode
        test    ax,MOD_MEASEND
        jnz     me_ac                         ; if not 0: MEASEND active
me_in:  cmp     dh,1                          ; checking for inactive
        jnz     me_e1                         ; if not enable cmd: error
; calculate the new mpu_mode2 word and store in CX until send_command o.k.
        mov     di,mpu_mode2
        or      di,MOD_MEASEND                ; set the bit in mpu_mode2
        mov     ah,ENABLE_MEAS_END            ; else: enable measure end
        call    send_command                  ; send command to MPU
        cmp     ax,0
        je      me_ok
        jmp     me_err                        ; send command failed
me_ac:  cmp     dh,0                          ; checking for active, if not
        jnz     me_e1                         ; disable cmd: error
; calculate the new mpu_mode2 word and store in CX until send_command o.k.
        mov     cx,MOD_MEASEND
        not     cx                            ; invert the mask
        mov     di,mpu_mode2
        and     di,cx                         ; clear the bit in mpu_mode2
        mov     ah,DISABLE_MEAS_END           ; else; disable measure end
        call    send_command                  ; send command to MPU
        cmp     ax,0
        je      me_ok
me_err: mov     rp_status[bx],RP_ME_MEAS_F    ; error: Measure end comm. fail.
        jmp     me_end
me_e1:  mov     rp_status[bx],RP_ME_ALRSET    ; error: MEASEND already set
        jmp     me_end
me_ok:  mov     rp_status[bx],RP_DONE         ; RP_DONE
; now set the MPU_MODE2 word to the new state calculated before
        mov     mpu_mode2,di                  ; activate new mpu_mode2
me_end: pop     dx
        pop     cx
        pop     di
        pop     es
        ret
ioc_meas_end    endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Timebase function
; IOCtl function Code is 9
; This Function sets the timebase value of the MPU.
; Passed parameter: value from 1 to 7, indicating the timebase value (byte)
; The function sends commands C2 to C8 to the MPU, depending on the passed
; parameter.
;-------------------------------------------------------------------------------

ioc_timebase proc NEAR
        push    es                            ; Save Request Packet address
        push    di
        push    cx
        push    dx
        push    bx
; call get_ioctl_parms function: DI = Offset of variable-chain where         
; the parms must be stored, CX = number of bytes in Parmbuf                  
        lea     di,tb_timebase                ; offset to DI (destination)
        mov     cx,1                          ; 1 byte to transfer           
        call    get_ioctl_parms               ; get parameters               
; The request packet parameters are now stored. Check with actual Mode       
; and refuse command, if command refers to a state already set               
        pop     bx                            ; RP Address restore!          
        cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms
        je      tb_1
        mov     rp_status[bx],ax              ; Set RP error status          
        jmp     tb_end                        ; return
tb_1:   mov     ah,tb_timebase                ; First check, if passed parm
        cmp     ah,1                          ; is valid and can be used for
        jnb     tb_2                          ; building the command code.
        mov     rp_status[bx],RP_TB_INVPARM   ; Parm must be 1..7, this will
        jmp     tb_end                        ; result in MPU command codes
tb_2:   cmp     ah,7                          ; C2..C8 for setting the timebase
        jng     tb_3
        mov     rp_status[bx],RP_TB_INVPARM
        jmp     tb_end                     
tb_3:   add     ah,0C1h                       ; calculate Command from given
        call    send_command                  ; parm and send to MPU
        cmp     ax,0                                                    
        je      tb_ok
        mov     rp_status[bx],RP_TB_CMDFAIL
        jmp     tb_end
tb_ok:  mov     rp_status[bx],RP_DONE         ; RP_DONE
tb_end: pop     dx
        pop     cx  
        pop     di  
        pop     es  
        ret         
ioc_timebase    endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Exclusive to Host on/off
; IOCtl function Code is 10
; This Function activates or deactivates Exclusive Messages to be sent to the
; host.
; Activation is done with MPU command ENABLE_EXCL2HOST (97h)
; Deactivation is done with MPU command DISABLE_EXCL2HOST (96h)
; Parameters passed: 1 Byte in IOCTL Parms: 1: Activate, 0: Deactivate          
;-------------------------------------------------------------------------------
 
ioc_excl2host  proc NEAR
        push    es                            ; Save Request Packet address
        push    di
        push    cx
        push    dx
        push    bx
; call get_ioctl_parms function: DI = Offset of variable-chain where
; the parms must be stored, CX = number of bytes in Parmbuf
        lea     di,excl_switch                ; offset to DI (destination)
        mov     cx,1                          ; 1 byte to transfer
        call    get_ioctl_parms               ; get parameters
; The request packet parameters are now stored. Check with actual Mode
; and refuse command, if command refers to a state already set
        pop     bx                            ; RP Address restore!
        cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms
        je      ex_5
        mov     rp_status[bx],ax              ; Set RP error status
        jmp     ex_end                        ; return
ex_5:   mov     dh,excl_switch                ; DH = passed parameter switch
        mov     ax,mpu_mode2                  ; actual mode
        test    ax,MOD_EXCL2HOST
        jnz     ex_ac                         ; if not 0: Excl2host active
ex_in:  cmp     dh,1                          ; checking for inactive
        jnz     ex_e1                         ; if not enable cmd: error        
; calculate the new mpu_mode2 word and store in CX until send_command o.k.
        mov     di,mpu_mode2
        or      di,MOD_EXCL2HOST              ; set the bit in mpu_mode2
        mov     ah,ENABLE_EXCL2HOST           ; else: enable excl2host
        call    send_command                  ; send command to MPU
        cmp     ax,0
        je      ex_ok
        jmp     ex_err                        ; send command failed
ex_ac:  cmp     dh,0                          ; checking for active, if not
        jnz     ex_e1                         ; disable cmd: error
; calculate the new mpu_mode2 word and store in CX until send_command o.k.
        mov     cx,MOD_EXCL2HOST
        not     cx                            ; invert the mask
        mov     di,mpu_mode2
        and     di,cx                         ; clear the bit in mpu_mode2
        mov     ah,DISABLE_EXCL2HOST          ; else; disable excl2host
        call    send_command                  ; send command to MPU
        cmp     ax,0
        je      ex_ok
ex_err: mov     rp_status[bx],RP_EX_EXCL_F    ; error: excl to host comm. fail.
        jmp     ex_end
ex_e1:  mov     rp_status[bx],RP_EX_ALRSET    ; error: excl to host already set
        jmp     ex_end
ex_ok:  mov     rp_status[bx],RP_DONE         ; RP_DONE
; now set the MPU_MODE2 word to the new state calculated before
        mov     mpu_mode2,di                  ; activate new mpu_mode2
ex_end: pop     dx
        pop     cx
        pop     di
        pop     es
        ret
ioc_excl2host   endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Realtime Affection on/off
; IOCtl function Code is 11
; This Function activates or deactivates Realtime Affection for MPU
; Activation is done with MPU command ENABLE_REALTIME_AFF (91h)
; Deactivation is done with MPU command DISABLE_REALTIME_AFF (90h)
; Parameters passed: 1 Byte in IOCTL Parms: 1: Activate, 0: Deactivate
;-------------------------------------------------------------------------------
 
ioc_realtime_aff proc NEAR
        push    es                            ; Save Request Packet address
        push    di
        push    cx
        push    dx
        push    bx
; call get_ioctl_parms function: DI = Offset of variable-chain where
; the parms must be stored, CX = number of bytes in Parmbuf
        lea     di,realt_switch               ; offset to DI (destination)
        mov     cx,1                          ; 1 byte to transfer
        call    get_ioctl_parms               ; get parameters
; The request packet parameters are now stored. Check with actual Mode
; and refuse command, if command refers to a state already set
        pop     bx                            ; RP Address restore!
        cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms
        je      ra_5
        mov     rp_status[bx],ax              ; Set RP error status
        jmp     ra_end                        ; return
ra_5:   mov     dh,realt_switch               ; DH = passed parameter switch
        mov     ax,mpu_mode2                  ; actual mode
        test    ax,MOD_REALTIME_AFF
        jnz     ra_ac                         ; if not 0: Excl2host active
ra_in:  cmp     dh,1                          ; checking for inactive
        jnz     ra_e1                         ; if not enable cmd: error
; calculate the new mpu_mode2 word and store in CX until send_command o.k.
        mov     di,mpu_mode2
        or      di,MOD_REALTIME_AFF           ; set the bit in mpu_mode2
        mov     ah,ENABLE_REALTIME_AFF        ; else: enable realtime aff.
        call    send_command                  ; send command to MPU
        cmp     ax,0
        je      ra_ok
        jmp     ra_err                        ; send command failed
ra_ac:  cmp     dh,0                          ; checking for active, if not
        jnz     ra_e1                         ; disable cmd: error
; calculate the new mpu_mode2 word and store in CX until send_command o.k.
        mov     cx,MOD_REALTIME_AFF
        not     cx                            ; invert the mask
        mov     di,mpu_mode2
        and     di,cx                         ; clear the bit in mpu_mode2
        mov     ah,DISABLE_REALTIME_AFF       ; else; disable realtime aff.
        call    send_command                  ; send command to MPU
        cmp     ax,0
        je      ra_ok
ra_err: mov     rp_status[bx],RP_RA_COMM_F    ; error: realtime aff comm. fail.
        jmp     ra_end
ra_e1:  mov     rp_status[bx],RP_RA_ALRSET    ; error: realtime aff already set
        jmp     ra_end
ra_ok:  mov     rp_status[bx],RP_DONE         ; RP_DONE
; now set the MPU_MODE2 word to the new state calculated before
        mov     mpu_mode2,di                  ; activate new mpu_mode2
ra_end: pop     dx
        pop     cx
        pop     di
        pop     es
        ret
ioc_realtime_aff endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL MIDI Thru on/off
; IOCtl function Code is 12
; This Function activates or deactivates MIDI Thru for MPU
; Activation is done with MPU command ENABLE_THRU (89h)
; Deactivation is done with MPU command DISABLE_THRU (88h)
; Parameters passed: 1 Byte in IOCTL Parms: 1: Activate, 0: Deactivate          
;-------------------------------------------------------------------------------
                                                                                
ioc_midi_thru    proc NEAR
        push    es                            ; Save Request Packet address    
        push    di                                                             
        push    cx                                                             
        push    dx                                                             
        push    bx                                                             
; call get_ioctl_parms function: DI = Offset of variable-chain where           
; the parms must be stored, CX = number of bytes in Parmbuf                    
        lea     di,thru_switch                ; offset to DI (destination)
        mov     cx,1                          ; 1 byte to transfer             
        call    get_ioctl_parms               ; get parameters                 
; The request packet parameters are now stored. Check with actual Mode         
; and refuse command, if command refers to a state already set                 
        pop     bx                            ; RP Address restore!            
        cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms  
        je      mt_5                                                           
        mov     rp_status[bx],ax              ; Set RP error status            
        jmp     mt_end                        ; return                         
mt_5:   mov     dh,thru_switch                ; DH = passed parameter switch
        mov     ax,mpu_mode2                  ; actual mode                    
        test    ax,MOD_MIDITHRU
        jnz     mt_ac                         ; if not 0: MIDIThru active
mt_in:  cmp     dh,1                          ; checking for inactive          
        jnz     mt_e1                         ; if not enable cmd: error       
; calculate the new mpu_mode2 word and store in CX until send_command o.k.     
        mov     di,mpu_mode2                                                   
        or      di,MOD_MIDITHRU               ; set the bit in mpu_mode2
        mov     ah,ENABLE_THRU                ; else: enable MIDIThru
        call    send_command                  ; send command to MPU            
        cmp     ax,0
        je      mt_ok
        jmp     mt_err                        ; send command failed
mt_ac:  cmp     dh,0                          ; checking for active, if not
        jnz     mt_e1                         ; disable cmd: error
; calculate the new mpu_mode2 word and store in CX until send_command o.k.
        mov     cx,MOD_MIDITHRU
        not     cx                            ; invert the mask
        mov     di,mpu_mode2
        and     di,cx                         ; clear the bit in mpu_mode2
        mov     ah,DISABLE_THRU               ; else; disable MIDIThru
        call    send_command                  ; send command to MPU
        cmp     ax,0
        je      mt_ok
mt_err: mov     rp_status[bx],RP_MT_COMM_F    ; error: MIDIThru command failed
        jmp     mt_end                                                         
mt_e1:  mov     rp_status[bx],RP_MT_ALRSET    ; error: MIDIThru already set
        jmp     mt_end                                                         
mt_ok:  mov     rp_status[bx],RP_DONE         ; RP_DONE                        
; now set the MPU_MODE2 word to the new state calculated before                
        mov     mpu_mode2,di                  ; activate new mpu_mode2         
mt_end: pop     dx                                                             
        pop     cx                                                             
        pop     di                                                             
        pop     es                                                             
        ret                                                                    
ioc_midi_thru    endp
                                          
;-------------------------------------------------------------------------------
; Entry Point to IOCTL Set Fader function
; IOCtl function Code is 13
; This Function sets the fader values for all 64 tracks.
; Passed parameter: 64 bytes from 0..255 indicating the fader value, which
; is used to multiply VELOCITY in midi_analyze during playback.
; The function just stores those 64 values to the array pl_fader
;-------------------------------------------------------------------------------
 
ioc_set_fader proc NEAR
        push    es                            ; Save Request Packet address     
        push    di
        push    cx
        push    dx
        push    bx
; call get_ioctl_parms function: DI = Offset of variable-chain where         
; the parms must be stored, CX = number of bytes in Parmbuf                  
        lea     di,pl_fader                   ; offset to DI (destination)
        mov     cx,SEQ_TRACKS                 ; 64 byte to transfer
        call    get_ioctl_parms               ; get parameters               
; The request packet parameters are now stored. Check with actual Mode       
; and refuse command, if command refers to a state already set               
        pop     bx                            ; RP Address restore!          
        cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms
        je      sf_end                        ; if o.k.   END
        mov     rp_status[bx],RP_SF_ERROR     ; else: set RP error status
sf_end: pop     dx
        pop     cx
        pop     di
        pop     es
        ret
ioc_set_fader    endp
         
;-------------------------------------------------------------------------------
; Entry Point to IOCTL Overdub function
; IOCtl function Code is 14
; This Function starts the overdub function. This enables recording while
; playback is active.
; Passed parameters: rp_ioctl_pbuf: Selector of Capture Buffer
;                    rp_ioctl_dbuf: Selector of Play Buffer
;-------------------------------------------------------------------------------
                                                                                
ioc_overdub  proc NEAR
        push    es                            ; Save Request Packet address     
        push    di                                                              
        push    cx                                                              
        push    dx                                                              
        push    bx                                                              
; 1. Check if PLAY or RECORD Mode is active, if yes: error
        test    mpu_mode,MOD_PLAY             ; test the bit 5 in mpu_mode     
        jz      ov_1                          ; if not in play mode (0)
        pop     bx                                                             
        mov     rp_status[bx],RP_OV_PLYAC     ; error code, MPU in PLAY Mode
        jmp     ov_end
ov_1:   test    mpu_mode,MOD_REC              ; test bit 6 in mpu_mode
        jz      ov_2                          ; if not in record mode (0)
        pop     bx                                                             
        mov     rp_status[bx],RP_OV_RECAC     ; error code, already in REC Mode
        jmp     ov_end
; 2. Prepare the GDT-Selector for CAPTURE Buffer and initialize startoffset
;    Capture Buffer Selector is passed in rp_ioctl_pbuf.
;    Lock the parm buffer address and verify access for midi_rec_bufsz bytes
ov_2:   pop     bx                            ; restore BX register for request
        push    bx                            ; packet access                  
        mov     di,word ptr rp_ioctl_pbuf[bx+2] ; get Selector of virtual addr
        mov     bl,0                          ; block until available          
        mov     bh,1                          ; long term, any memory          
        mov     cx,midi_rec_bufsz             ; length of buffersize (4096)
        mov     dh,1                          ; write access                   
        call    lock_verify                   ; call lock_verify subfunction   
        cmp     ax,RP_DONE                                                     
        je      ov_3                          ; if o.k...
        pop     bx                            ; restore BX                     
        mov     rp_status[bx],ax              ; set request packet status      
        jmp     ov_end                        ; End function with error
ov_3:   mov     word ptr ioc_lock_hnd3,cx     ; store lock handle from cx:dx
        mov     di,2                          ; returned by lock_verify for    
        mov     word ptr ioc_lock_hnd3[di],dx ; unlock after stop record       
; lock and verify access are done, now virtophys                               
; IMPORTANT!!: cause DS is used for parameter passing, the addressing of       
; DeviceHelp is realized by ES                                                 
        pop     bx                            ; restore BX register for request
        push    bx                            ; packet access                  
        push    si                            ; S T A R T  Block VirtToPhys    
        push    ds                                                             
        push    es                                                             
        push    ds                            ; Save DS for exchange
        cli
        lds     si,rp_ioctl_pbuf[bx]          ; MIDI buffer address to DS:SI
        pop     es                            ; CHANGE ES to DS contents       
        ASSUME  es:DSEG,ds:NOTHING            ; ES accesses DSEG for call      
        mov     dl,DevHlp_VirtToPhys          ; Function code                  
        call    dword ptr [DeviceHelp]        ; Call VirtToPhys function       
        ASSUME  ds:DSEG,es:rp                 ; VirtToPhys returns the         
        pop     es                            ; physical address in AX:BX      
        pop     ds
        sti
        pop     si                            ; E N D   Block   Virt2phys      
        jnc     ov_4                          ; if O.K. proceed
        pop     bx                                                             
        mov     rp_status[bx],RP_OV_V2P       ; VirtToPhys failed
        jmp     ov_end
; Virt to Phys done, now map to GDT Selector. PhysToGDTSelector expects the    
; physical address to be mapped in AX:BX register pair                         
ov_4:   mov     cx,midi_rec_bufsz             ; Segment Size
        push    si                                                             
        mov     si,Sel_REC                    ; GDT Selector to be setup       
        mov     dl,DevHlp_PhysToGDTSelector   ; function code                  
        call    dword ptr [DeviceHelp]        ; call PhysToGDTSelector         
        pop     si                            ; if error: AX contains code     
        jnc     ov_5
        pop     bx                            ; PhysToGDTSelector failed       
        mov     rp_status[bx],RP_OV_GDTFAIL
        jmp     ov_end
; GDT Selector Mapping done, now init initial offset
ov_5:
; now first copy 1 to the first two bytes in capturebuffer, this is the initial
; offset for use by the interrupt handler. Then prepare the PLAY Buffer
; access through GDT selector Sel_PLAY.
        push    ds
        cli
        mov     ds,[Sel_REC]                  ; establish GDT Selector access  
        mov     si,0                                                           
        mov     ax,1                                                           
        mov     word ptr [ds:si],ax           ; copy 1 to initial offset       
        pop     ds
        sti
; AT THIS POINT: CAPTURE Buffer is accessed and prepared for recording.
; 3. Prepare the GDT-Selector for PLAY Buffer.
;    Play Buffer Selector is passed in rp_ioctl_dbuf.
;    Lock the parm buffer address and verify access for midi_play_bufsz bytes
        pop     bx                            ; restore BX register for request
        push    bx                            ; packet access                  
        mov     di,word ptr rp_ioctl_dbuf[bx+2] ; get Selector of virtual addr
        mov     bl,0                          ; block until available          
        mov     bh,1                          ; long term, any memory          
        mov     cx,midi_play_bufsz            ; length of buffersize (4096)
        mov     dh,0                          ; read access
        call    lock_verify                   ; call lock_verify subfunction   
        cmp     ax,RP_DONE                                                     
        je      ov_6                          ; if o.k...
        pop     bx                            ; restore BX                     
        mov     rp_status[bx],ax              ; set request packet status      
        jmp     ov_end                        ; End function with error        
ov_6:   mov     word ptr ioc_lock_hnd2,cx     ; store lock handle from cx:dx
        mov     di,2                          ; returned by lock_verify for    
        mov     word ptr ioc_lock_hnd2[di],dx ; unlock after stop play
; lock and verify access are done, now virtophys                               
; IMPORTANT!!: cause DS is used for parameter passing, the addressing of       
; DeviceHelp is realized by ES                                                 
        pop     bx                            ; restore BX register for request
        push    bx                            ; packet access                  
        push    si                            ; S T A R T  Block VirtToPhys    
        push    ds                                                             
        push    es                                                             
        push    ds                            ; Save DS for exchange
        cli
        lds     si,rp_ioctl_dbuf[bx]          ; MIDI buffer address to DS:SI
        pop     es                            ; CHANGE ES to DS contents       
        ASSUME  es:DSEG,ds:NOTHING            ; ES accesses DSEG for call      
        mov     dl,DevHlp_VirtToPhys          ; Function code                  
        call    dword ptr [DeviceHelp]        ; Call VirtToPhys function       
        ASSUME  ds:DSEG,es:rp                 ; VirtToPhys returns the         
        pop     es                            ; physical address in AX:BX      
        pop     ds
        sti
        pop     si                            ; E N D   Block   Virt2phys      
        jnc     ov_7                          ; if O.K. proceed
        pop     bx                                                             
        mov     rp_status[bx],RP_OV_V2P       ; VirtToPhys failed              
        jmp     ov_end                                                         
; Virt to Phys done, now map to GDT Selector. PhysToGDTSelector expects the    
; physical address to be mapped in AX:BX register pair                         
ov_7:   mov     cx,midi_play_bufsz            ; Segment Size (4096)
        push    si                                                             
        mov     si,Sel_PLAY                   ; GDT Selector to be setup
        mov     dl,DevHlp_PhysToGDTSelector   ; function code                  
        call    dword ptr [DeviceHelp]        ; call PhysToGDTSelector         
        pop     si                            ; if error: AX contains code     
        jnc     ov_8
        pop     bx                            ; PhysToGDTSelector failed       
        mov     rp_status[bx],RP_OV_GDTFAIL                                    
        jmp     ov_end                                                         
; GDT Selector Mapping done. Now the Playbuffer is accessible through Sel_PLAY
; now send activate track command to MPU with active track mask               
ov_8:   mov     ah,ACTIVATE_TRK               ; Activate Tracks command code
        call    send_command                  ; send to MPU                   
        cmp     ax,0                                                          
        jne     ov_9                          ; if error:
        mov     ah,1                          ; else: active track always 1  
        cli
        call    send_data
        sti
        cmp     ax,0                                                          
        je      ov_a                          ; if o.k. -> proceed
        pop     bx                                                            
        mov     rp_status[bx],RP_OV_SND_F     ; error: Send Data failed
        jmp     ov_end
ov_9:   pop     bx
        mov     rp_status[bx],RP_OV_ACT_F     ; error: Activate Track failed
        jmp     ov_end                        ; error in ACTIVATE_TRK command
; activate track command has completed                                        
; now send clear play counter command to MPU
ov_a:   mov     ah,CLR_PLAY_CNT               ; 'Clear Play Counter' command
        call    send_command                                                   
        cmp     ax,0                                                           
        je      ov_b
        pop     bx                                                             
        mov     rp_status[bx],RP_OV_CLR_F     ; Clear Play Counter failed
        jmp     ov_end
; now initialize value for track data request byte to 0 and reset runstat
ov_b:   mov     trk_data_req,0
        mov     byte ptr ma_runst_play,0
        .386P
        mov     dword ptr pl_delta_time,0
        mov     dword ptr pl_abs_time,0
        .286P
; now set mpu_mode variable to MOD_OVERDUB.
; bit 5 = 1 (PLAY): Int Hdlr now reacts upon TDRs from MPU
; bit 6 = 1 (REC) : Int Hdlr copies all other data to CAPTURE Buffer, if
;                   no TDR has been found during PLAY test.
        or      mpu_mode,MOD_OVERDUB
; now read start offset from bytes 1+2 in Play buffer (must be set by 
; application!!!).
; For normal start, this will be 6. Copy the value to pl_offset, a working
; variable. (bytes 1+2 only used for feedback to application)
; NEW: also read bytes 3-6, containing accumulated timing value. This is also 
; initialized by the application at start playback.                           
        .386P                                 ; enable 80386 specific
        push    ecx
        push    es
        mov     es,[Sel_PLAY]                 ; establish GDT Selector access 
        mov     di,0                          ; trough ES:DI register pair    
        mov     ax,word ptr [es:di]           ; AX = initial offset           
        mov     di,2                          ; offset accu timing            
        mov     ecx,dword ptr [es:di]         ; initial value from appl       
        pop     es                            ; restore ES register           
        mov     pl_offset,ax                  ; store pl_offset               
        mov     pl_accu_time,ecx
        pop     ecx
        .286P
; now start MPU overdub. (MPU will begin to send Track Data Requests along 
; with data to be captured. Int Hdlr will analyze received data and either
; unblock the playback thread in case of TDR or copy the received byte to
; the CAPTURE Buffer. In case of FC: Int Hdlr will set stop code to
; the TDR field.
        mov     ah,START_OVERDUB              ; 'Start Overdub' command
        call    send_command                                                   
        cmp     ax,0                                                           
        je      ov_c
        pop     bx                                                             
        mov     rp_status[bx],RP_OV_FAIL      ; Start Play Command failed
        jmp     ov_end                        ; ov_end also resets mode
; MPU recording started, now wait for TDR and output data. If TDR = 0FCh        
; return from function: FCh is the Stop code set by ioc_stop_overdub
; S T A R T  OF ENDLESS LOOP                                                    
                                                                                
; ----------------------------------------------------------------------------- 
;       \\//     E N D L E S S    L O O P                                       
;        \/                                                                     
; This is the entrypoint to an endless-loop. This code runs in a second thread  
; at ring 0 and can only be stopped by an unblock with x'FC' in trk_data_req.   
                                                                                
ov_c:   mov     al,trk_data_req               ; check before BLOCK
        cmp     al,0                                                            
        jnz     ov_d                          ; request or stopcode
; at this point: no TDR is outstanding, also no stop code: BLOCK the thread     
; and wait, until the interrupt handler passes data or the ioc_stop_overdub
; passes a stop code                                                            
        mov     ax,word ptr block_hnd_play                                      
        mov     di,2                                                            
        mov     bx,word ptr block_hnd_play[di]                                  
        mov     di,-1
        mov     cx,-1                         ; block until run                 
        mov     dh,0                          ; interruptible                   
        cli                                   ; disable interrupts !!!!!        
        mov     dl,DevHlp_ProcBlock           ; Block reenables interrupts      
        call    dword ptr [DeviceHelp]        ; Block the thread....            
                                                                                
; ----------------------------------------------------------------------------- 
;            \      /                                                           
;             \    /        Entry Point at Unblock from Interrupt Hander        
;              \  /         when Track Data Request occurs or at Unblock from   
;               \/          ioc_stop_overdub when Stop Overdub is requested
                                                                                
; At this point: some data is in the TDR field: TDR or stop code. Otherwise     
; generate error message. This point is reached, if UNBLOCK occurs.             
ov_d:   mov     al,trk_data_req               ; store datafield
        mov     trk_data_req,0                ; clear datafield !!!
        cmp     al,PLAY_STOP_CODE                                               
        je      ov_g                          ; jump to stop processing
        pop     bx                            ; e r r o r  if unknown data
        push    bx                            ; TDR F0                          
        mov     rp_status[bx],RP_OV_INV_TDR   ; unknown data at unblock
        mov     ah,STOP_OVERDUB               ; stop MPU
        call    send_command                                                    
        cmp     ax,0                                                            
        jz      ov_g                          ; UNLOCK Buffer Address
        pop     bx                                                              
        mov     rp_status[bx],RP_OV_STOP_F2   ; error code: stop play failed
        jmp     ov_end
; at this point: maybe other data processing for unblock purpose
        jmp     ov_c                          ; E N D L E S S  Loop
                                                                                
;        /\      E N D L E S S    L O O P                                       
;       //\\                                                                    
; ----------------------------------------------------------------------------- 
                                                                                
; STOP CODE found in Track Data field: STOP Play processing. This is the only
; way to leave the overdub endless playback loop.
; if this occurs: do not send a stop command to the mpu, because recording
; must not be interrupted. Just reset mpu_mode to single record mode.
; This will disable the interrupt handler from recognizing TDRs and trying
; to unblock the thread. 
; Then release the PLAYBuffer (unlock) and terminate the thread.

; this point (ov_g) is reached if PLAY Stop code has been found in TDR or
; an invalid TDR was detected.
; Reset Mode to RECORD_MODE 
ov_g:   mov     al,MOD_PLAY                   ; reset MPU Mode from OVERDUB
        not     al                            ; to RECORD
        and     mpu_mode,al
; now Unlock the address of the playback buffer
        mov     ax,word ptr ioc_lock_hnd2     ; restore lock handle
        mov     di,2                                                
        mov     bx,word ptr ioc_lock_hnd2[di] ; DI = 2              
        mov     dl,DevHlp_Unlock                                    
        call    dword ptr [DeviceHelp]        ; Call unlock function
        jc      ov_k1                         ; if error..
        pop     bx                            ; assume default RP_STATUS
        jmp     ov_end                        ; as RP_DONE

; error processing ( later: also unlock to all buffers)
ov_i:   pop     bx
        mov     rp_status[bx],RP_OV_SND_FAIL  ; ERROR: Send Data failed
        jmp     ov_end
ov_k1:  pop     bx                   
        mov     rp_status[bx],RP_OV_UNLFAIL   ; ERROR: Unlock failed
ov_end: mov     dx,rp_status[bx]              ; check for good returnvalue
        cmp     dx,RP_DONE                    ; if errors: reset both play
        je      ov_en2                        ; AND record mode
        mov     al,MOD_OVERDUB 
        not     al             
        and     mpu_mode,al                                                                              
ov_en2: pop     dx
        pop     cx                                                              
        pop     di                                                              
        pop     es                                                              
        ret                                                                     
ioc_overdub     endp


;-------------------------------------------------------------------------------
; Entry Point to IOCTL STOP Overdub function
; IOCtl function Code is 15
; This Function stops overdub.
; Passed parameter:     
; 
;                                             
;-------------------------------------------------------------------------------
                                                                                
ioc_stop_overdub  proc NEAR
        push    es                            ; Save Request Packet address     
        push    di                                                              
        push    cx                                                              
        push    dx                                                              
        push    bx                                                              
; Check, if record mode active: it must be active, otherwise error
        test    mpu_mode,MOD_REC              ; test bit 6 in mpu_mode
        jnz     po_s                          ; if in record mode
        pop     bx                                                             
        mov     rp_status[bx],RP_PO_RCIN      ; error code, not in REC Mode
        jmp     po_end

; 1. Check, if PLAY Mode is active: if YES: Playback didn't finish while
;    recording and must be stopped now. If NO: Playback already has stopped
;    automatically because of FC end mark in play buffer. In this case the
;    playbuffer is already unlocked and nothing is to be done to stop play.

po_s:   test    mpu_mode,MOD_PLAY             ; test the bit 5 in mpu_mode
        jz      po_a                          ; if not in play mode (0)
; playmode found active, so first stop playback by copying stop code to the
; TDR field and unblock the thread.
        mov     al,MOD_PLAY                   ; reset mpu_mode to prevent, that
        not     al                            ; the interrupt handler will copy
        and     mpu_mode,al                   ; more track data requests       
        mov     trk_data_req,PLAY_STOP_CODE   ; Stop Code to TDR               
; now unblock the waiting PLAY thread (ioc_overdub function)
        push    di                                                             
        mov     di,2                                                           
        mov     bx,word ptr block_hnd_play[di]                                 
        mov     ax,word ptr block_hnd_play                                     
        pop     di                                                             
        mov     dl,DevHlp_ProcRun             ; AX will contain no. of threads 
        call    dword ptr [DeviceHelp]        ; unblock waiting thread         
        pop     bx
        push    bx
; now send stop_overdub command to mpu
po_a:   mov     ah,STOP_OVERDUB
        call    send_command                                                   
        cmp     ax,0                                                           
        je      po_2
        pop     bx                            ; if not in rec mode (0)         
        mov     rp_status[bx],RP_PO_SCF       ; send command failed
        jmp     po_end
po_2:   mov     cl,MOD_OVERDUB                ; reset both for security
        not     cl                            ; invert mask                    
        and     mpu_mode,cl                   ; reset overdub mode
; now copy final timing byte and FC Data End to Buffer, because the data       
; cannot be received from MPU. MPU DOES send timing and FCh, but int Hdlr is   
; disabled, so that data is thrown away. For this reason, the function must    
; append the data 'manually'. Final Timing byte chosen to be 120.              
        mov     cx,midi_rec_bufsz       ; CX = MIDI IN BufferSIZE              
        dec     cx                      ; CX = Maximum OFFSET (size-1!)        
        push    ds
        cli
        mov     ds,[Sel_REC]            ; establish GDT Selector access        
        mov     si,0                                                           
        mov     dx,word ptr [ds:si]     ; DX = pointer in bytes 1+2            
        inc     dx                      ; increase by one for new position     
        mov     si,dx                   ; DX = new offset                      
        mov     byte ptr [ds:si],120    ; store final timing byte to Buffer    
        cmp     cx,dx                   ; Also check for turnaround            
        jne     po_4
        mov     dx,1                                                           
po_4:   inc     dx
        mov     si,dx                   ; DX = new offset                      
        mov     byte ptr [ds:si],0FCh   ; store DATA END Code to Buffer        
        mov     si,0                                                           
        mov     word ptr [ds:si],dx     ; store DX as new pointer              
        pop     ds                      ; restore Datasegment  
        sti
; now Unlock the address of the capture buffer
        mov     ax,word ptr ioc_lock_hnd3     ; restore lock handle            
        mov     di,2                                                           
        mov     bx,word ptr ioc_lock_hnd3[di] ; DI = 2                         
        mov     dl,DevHlp_Unlock                                               
        call    dword ptr [DeviceHelp]        ; Call unlock function           
        jnc     po_ok                         ; if o.k. proceed
        pop     bx                            ; Restore Request packet Address
        mov     rp_status[bx],RP_PO_UNLFAIL   ; error: Unlock failed
        jmp     po_end
po_ok:  pop     bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE                        
po_end: pop     dx
        pop     cx                                                              
        pop     di                                                              
        pop     es                                                              
        ret                                                                     
ioc_stop_overdub  endp

;------------------------------------------------------------------------------
; Entry Point to IOCTL Send Data Function
; IOCtl function Code is 16
; This function is used to send note on/off commands directly to the MPU 
; without using the Playbuffer.
; Passed parameters: 1. command code
;                    2. Pitch
;                    3. velocity
;                    4. midi channel
; maximum: 10 Notes can be used this way (for test: only one note)
; The function keeps a table, where all 10 notes status is stored
;------------------------------------------------------------------------------
                                                                                
ioc_send_data   proc NEAR
        push    dx                                                              
        push    di                                                              
        push    cx                                                              
        push    es                            ; Save Request Packet address     
        push    bx                                                              
; 1. Store the passed parameters to variables
        lea     di,sn_command                 ; offset of first variable
        mov     cx,4                          ; number of bytes to transfer
        call    get_ioctl_parms               ; get the parameters
        cmp     ax,RP_DONE
        je      sn_1
        pop     bx
        mov     rp_status[bx],SN_PARM_FAIL    ; error
        jmp     sn_end
; 2. Build the appropriate MIDI Event to send to the MPU by analyzing the
;    command and data sent as parameters
sn_1:   mov     ah,WANT_TO_SEND_DATA          ; Send DATA
        call    send_command                                                   
        cmp     ax,0
        je      sn_2
        pop     bx
        mov     rp_status[bx],SN_CMD_FAIL     ; error
        jmp     sn_end
; 3. Send 3 Bytes MIDI Event to the MPU using send_data
sn_2:   mov     ah,sn_command
        cli
        call    send_data
        sti
        cmp     ax,0
        je      sn_3                          ; if o.k. -> proceed
        pop     bx
        mov     rp_status[bx],SN_DAT_FAIL     ; error
        jmp     sn_end
sn_3:   mov     ah,sn_pitch
        cli
        call    send_data
        sti
        cmp     ax,0
        je      sn_4                          ; if o.k. -> proceed
        pop     bx                                                
        mov     rp_status[bx],SN_DAT_FAIL     ; error
        jmp     sn_end
sn_4:   mov     ah,sn_velocity
        cli
        call    send_data
        sti
        cmp     ax,0
        je      sn_ok                         ; if o.k. -> proceed
        pop     bx
        mov     rp_status[bx],SN_DAT_FAIL     ; error
        jmp     sn_end
sn_ok:  pop     bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE                         
sn_end: pop     es
        pop     cx                                                              
        pop     di                                                              
        pop     dx                                                              
        ret                                                                     
ioc_send_data      endp

;------------------------------------------------------------------------------
; Entry Point to IOCTL Set Tempo
; IOCtl function Code is 17
; This function sets the MPU401 internal clock in (beats per minute??)
; Passed parameters: 1. Tempo       1 byte
;------------------------------------------------------------------------------
                                                                               
ioc_set_tempo   proc NEAR
        push    dx                                                             
        push    di                                                             
        push    cx                                                             
        push    es                            ; Save Request Packet address    
        push    bx                                                             
; 1. Store the passed parameters to variables                                  
        lea     di,st_tempo                   ; offset of first variable
        mov     cx,1                          ; number of bytes to transfer
        call    get_ioctl_parms               ; get the parameters             
        cmp     ax,RP_DONE                                                     
        je      st_1
        pop     bx                                                             
        mov     rp_status[bx],ST_PARM_FAIL    ; error
        jmp     st_end
; 2. Send the command to MPU (E0h) followed by data byte which contains tempo
st_1:   mov     ah,SET_TEMPO                  ; MPU command code
        call    send_command                                                   
        cmp     ax,0                                                           
        je      st_2
        pop     bx                                                             
        mov     rp_status[bx],ST_CMD_FAIL     ; error
        jmp     st_end
st_2:   mov     ah,st_tempo
        cli
        call    send_data    
        sti
        cmp     ax,0                                                           
        je      st_ok                         ; if o.k. -> proceed
        pop     bx                                                             
        mov     rp_status[bx],ST_DAT_FAIL     ; error
        jmp     st_end
st_ok:  pop     bx
        mov     rp_status[bx],RP_DONE         ; RP_DONE                        
st_end: pop     es
        pop     cx                                                             
        pop     di                                                             
        pop     dx                                                             
        ret                                                                    
ioc_set_tempo      endp

;------------------------------------------------------------------------------
; Entry Point to IOCTL Set Relative Tempo
; IOCtl function Code is 18
; This function sets the MPU401 relative tempo. This is used for temporary
; changes of speed in play mode. Default: 040h for a ratio of 1:1.
; Passed parameters: 1. Rel Tempo   1 byte
;------------------------------------------------------------------------------
                                                                               
ioc_set_rel_tempo proc NEAR
        push    dx                                                             
        push    di                                                             
        push    cx                                                             
        push    es                            ; Save Request Packet address    
        push    bx                                                             
; 1. Store the passed parameters to variables                                  
        lea     di,so_tempo                   ; offset of first variable       
        mov     cx,1                          ; number of bytes to transfer    
        call    get_ioctl_parms               ; get the parameters             
        cmp     ax,RP_DONE                                                     
        je      so_1                                                           
        pop     bx                                                             
        mov     rp_status[bx],SO_PARM_FAIL    ; error
        jmp     so_end                                                         
; 2. Send the command to MPU (E1h) followed by relative tempo byte
so_1:   mov     ah,SET_REL_TEMPO              ; MPU command code
        call    send_command                                                   
        cmp     ax,0                                                           
        je      so_2                                                           
        pop     bx                                                             
        mov     rp_status[bx],SO_CMD_FAIL     ; error
        jmp     so_end                                                         
so_2:   mov     ah,so_tempo 
        cli
        call    send_data     
        sti
        cmp     ax,0                                                           
        je      so_ok                         ; if o.k. -> proceed             
        pop     bx                                                             
        mov     rp_status[bx],SO_DAT_FAIL     ; error
        jmp     so_end                                                         
so_ok:  pop     bx                                                             
        mov     rp_status[bx],RP_DONE         ; RP_DONE                        
so_end: pop     es                                                             
        pop     cx                                                             
        pop     di                                                             
        pop     dx                                                             
        ret                                                                    
ioc_set_rel_tempo  endp

;------------------------------------------------------------------------------
; Entry Point to IOCTL Set Graduation
; IOCtl function Code is 19
; This function sets the MPU401 relative tempo change.
; 00: immediate, 01: slowest, FF: fastest
; Passed parameters: 1. graduation  1 byte
;------------------------------------------------------------------------------
                                                                               
ioc_set_graduation proc NEAR
        push    dx                                                             
        push    di                                                             
        push    cx                                                             
        push    es                            ; Save Request Packet address    
        push    bx                                                             
; 1. Store the passed parameters to variables                                  
        lea     di,sg_graduation              ; offset of first variable
        mov     cx,1                          ; number of bytes to transfer    
        call    get_ioctl_parms               ; get the parameters             
        cmp     ax,RP_DONE                                                     
        je      sg_1                                                           
        pop     bx                                                             
        mov     rp_status[bx],SG_PARM_FAIL    ; error
        jmp     sg_end                                                         
; 2. Send the command to MPU (E2h) followed by graduation
sg_1:   mov     ah,SET_GRADUATION             ; MPU command code
        call    send_command                                                   
        cmp     ax,0                                                           
        je      sg_2                                                           
        pop     bx                                                             
        mov     rp_status[bx],SG_CMD_FAIL     ; error
        jmp     sg_end                                                         
sg_2:   mov     ah,sg_graduation
        cli
        call    send_data                 
        sti
        cmp     ax,0                                                           
        je      sg_ok                         ; if o.k. -> proceed             
        pop     bx                                                             
        mov     rp_status[bx],SG_DAT_FAIL     ; error
        jmp     sg_end                                                         
sg_ok:  pop     bx                                                             
        mov     rp_status[bx],RP_DONE         ; RP_DONE                        
sg_end: pop     es                                                             
        pop     cx                                                             
        pop     di                                                             
        pop     dx                                                             
        ret                                                                    
ioc_set_graduation endp

;------------------------------------------------------------------------------ 
; Entry Point to IOCTL Set MIDI PER METRO
; IOCtl function Code is 20
; This function sets the MPU401 MIDI per METRO
; Passed parameters: 1. MIDI per Metro   1 byte
;------------------------------------------------------------------------------ 
                                                                                
ioc_midi_per_metro proc NEAR
        push    dx                                                              
        push    di                                                              
        push    cx                                                              
        push    es                            ; Save Request Packet address     
        push    bx                                                              
; 1. Store the passed parameters to variables                                   
        lea     di,mm_midimetro               ; offset of first variable
        mov     cx,1                          ; number of bytes to transfer     
        call    get_ioctl_parms               ; get the parameters              
        cmp     ax,RP_DONE                                                      
        je      mm_1                                                            
        pop     bx                                                              
        mov     rp_status[bx],MM_PARM_FAIL    ; error
        jmp     mm_end                                                          
; 2. Send the command to MPU (E2h) followed by graduation                       
mm_1:   mov     ah,SET_MIDI_PER_METRO         ; MPU command code
        call    send_command                                                    
        cmp     ax,0                                                            
        je      mm_2                                                            
        pop     bx                                                              
        mov     rp_status[bx],MM_CMD_FAIL     ; error
        jmp     mm_end                                                          
mm_2:   mov     ah,mm_midimetro
        cli
        call    send_data                
        sti
        cmp     ax,0                                                            
        je      mm_ok                         ; if o.k. -> proceed              
        pop     bx                                                              
        mov     rp_status[bx],MM_DAT_FAIL     ; error
        jmp     mm_end                                                          
mm_ok:  pop     bx                                                              
        mov     rp_status[bx],RP_DONE         ; RP_DONE                         
mm_end: pop     es                                                              
        pop     cx                                                              
        pop     di                                                              
        pop     dx                                                              
        ret                                                                     
ioc_midi_per_metro endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Channel Changer function
; IOCtl function Code is 21
; This Function sets the MIDI Channel change values for online change during
; playback.
; Passed parameter: 16 bytes from 0..16 indicating the MIDI channel number, to
; which a given MIDI channel has to be changed to.
; The function just stores those 16 values to the array cc_chantab.
;-------------------------------------------------------------------------------
                                                                                
ioc_chan_change proc NEAR
        push    es                            ; Save Request Packet address     
        push    di                                                              
        push    cx                                                              
        push    dx                                                              
        push    bx                                                              
; call get_ioctl_parms function: DI = Offset of variable-chain where            
; the parms must be stored, CX = number of bytes in Parmbuf                     
        lea     di,cc_chantab                 ; offset to DI (destination)
        mov     cx,SEQ_TRACKS                 ; 64 byte to transfer
        call    get_ioctl_parms               ; get parameters                  
; The request packet parameters are now stored. Check with actual Mode          
; and refuse command, if command refers to a state already set                  
        pop     bx                            ; RP Address restore!             
        cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms   
        je      cc_end                        ; if o.k.   END
        mov     rp_status[bx],RP_CC_ERROR     ; else: set RP error status
cc_end: pop     dx
        pop     cx                                                              
        pop     di                                                              
        pop     es                                                              
        ret                                                                     
ioc_chan_change  endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Set Active Tracks function
; IOCtl function Code is 22
; This Function sets the active track mask for all 64 tracks. These values
; are used by midi_dataout to determine, if the data for this track needs to
; be played. In case of deactivation the function sets a counter, containing
; the number of open NOTE ONs for the track to be deactivated.
; Passed parameter: 65 bytes:
;                   1-64 : status for each track: 0=inactive, 1=active
;                     65 : FF: no deactivation, 0..63: deactivated track
;-------------------------------------------------------------------------------
                                                                                
ioc_set_acttrk proc NEAR
        push    es                            ; Save Request Packet address     
        push    di  
        push    si
        push    cx                                                              
        push    dx                                                              
        push    bx                                                              
; call get_ioctl_parms function: DI = Offset of variable-chain where            
; the parms must be stored, CX = number of bytes in Parmbuf                     
        lea     di,sa_acttrk                  ; offset to DI (destination)
        mov     cx,64                         ; 64 byte to transfer
        call    get_ioctl_parms               ; get parameters                  
        cmp     ax,RP_DONE                    ; check RC from get_ioctl_parms
        jne     sa_err                        ; if error: end
        pop     bx
        jmp     sa_end
sa_err: pop     bx
        mov     rp_status[bx],RP_SA_ERROR     ; else: set RP error status
sa_end: pop     dx
        pop     cx  
        pop     si
        pop     di                                                              
        pop     es                                                              
        ret                                                                     
ioc_set_acttrk  endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Setup GDT Selector for Note Table
; IOCtl function Code is 23
; This function sets up the GDT Selector Sel_NOTETAB for access to the
; applications notetable. This Selector remains valid, until Free is called
; with function ioc_free_notetab.
; The function is called up to 8 times to do the access to 8 blocks of each
; 32kbyte, containing 8 tracks each.
;-------------------------------------------------------------------------------

ioc_gdt_notetab proc NEAR
        push    es                            ; Save Request Packet address
        push    di                                                         
        push    cx                                                         
        push    dx                                                         
        push    bx           
; call get_ioctl_parms function: DI = Offset of variable-chain where        
; the parms must be stored, CX = number of bytes in Parmbuf                 
        lea     di,gn_sel_index               ; offset to DI (destination)
        mov     cx,1                          ; 1 byte to transfer
        call    get_ioctl_parms               ; get parameters                                                            
; 1. Lock the data buffer address and verify access for NOTETAB_SIZE bytes
        mov     di,word ptr rp_ioctl_dbuf[bx+2] ; get Selector of virtual addr
        mov     bl,0                          ; block until available          
        mov     bh,1                          ; long term, any memory          
        mov     cx,NOTETAB_SIZE               ; length of buffersize
        mov     dh,1                          ; write access
        call    lock_verify                   ; call lock_verify subfunction   
        cmp     ax,RP_DONE                                                     
        je      gn_7                          ; if o.k...                      
        pop     bx                            ; restore BX                     
        mov     rp_status[bx],ax              ; set request packet status      
        jmp     gn_end                        ; End function with error        
gn_7:   mov     bl,gn_sel_index
        shl     bl,2                          ; multiply by 4, doubleword
        xor     bh,bh                         ; BX = lock handle to segment
        mov     word ptr ioc_lock_hnd4[bx],cx ; store lock handle from cx:dx
        add     bx,2                          ; returned by lock_verify for
        mov     word ptr ioc_lock_hnd4[bx],dx ; unlock in ioc_stop_play
; lock and verify access are done, now virtophys                               
; IMPORTANT!!: cause DS is used for parameter passing, the addressing of       
; DeviceHelp is realized by ES                                                 
        pop     bx                            ; restore BX register for request
        push    bx                            ; packet access                  
        push    si                            ; S T A R T  Block VirtToPhys    
        push    ds                                                             
        push    es                                                             
        push    ds                            ; Save DS for exchange           
        cli                                                                    
        lds     si,rp_ioctl_dbuf[bx]          ; NOTETAB address to DS:SI
        pop     es                            ; CHANGE ES to DS contents       
        ASSUME  es:DSEG,ds:NOTHING            ; ES accesses DSEG for call      
        mov     dl,DevHlp_VirtToPhys          ; Function code                  
        call    dword ptr [DeviceHelp]        ; Call VirtToPhys function       
        ASSUME  ds:DSEG,es:rp                 ; VirtToPhys returns the         
        pop     es                            ; physical address in AX:BX      
        pop     ds                                                             
        sti                                                                    
        pop     si                            ; E N D   Block   Virt2phys      
        jnc     gn_8                          ; if O.K. proceed                
        pop     bx                                                             
        mov     rp_status[bx],RP_GN_V2P       ; VirtToPhys failed
        jmp     gn_end                                                         
; Virt to Phys done, now map to GDT Selector. PhysToGDTSelector expects the    
; physical address to be mapped in AX:BX register pair                         
gn_8:   mov     cx,NOTETAB_SIZE               ; Segment Size
        push    bx
        mov     bl,gn_sel_index
        shl     bl,1                          ; 2 Bytes length
        xor     bh,bh                         ; BX = Index to GDT_Selector
        mov     di,bx
        pop     bx
        push    si                                                             
        mov     si,Sel_NOTETAB[di]            ; GDT Selector to be setup
        mov     dl,DevHlp_PhysToGDTSelector   ; function code                  
        call    dword ptr [DeviceHelp]        ; call PhysToGDTSelector         
        pop     si                            ; if error: AX contains code 
        pop     bx
        jnc     gn_end
;       pop     bx                            ; PhysToGDTSelector failed
        mov     rp_status[bx],RP_GN_GDTFAIL
; GDT Selector Mapping done. Now the Playbuffer is accessible through Sel_PLAY 
gn_end: pop     dx
        pop     cx  
        pop     di  
        pop     es  
        ret         
ioc_gdt_notetab  endp

;-------------------------------------------------------------------------------
; Entry Point to IOCTL Free GDT Selector for Note Table
; IOCtl function Code is 24
; This function frees the GDT Selector Sel_NOTETAB for access to the
; applications notetable.
; Passed parameters: PARM 1 byte with index (0..7) which Selector has to
; be freed.
;-------------------------------------------------------------------------------
                                                                                
ioc_free_notetab proc NEAR
        push    es                            ; Save Request Packet address     
        push    di                                                              
        push    cx                                                              
        push    dx                                                              
        push    bx         
; call get_ioctl_parms function: DI = Offset of variable-chain where      
; the parms must be stored, CX = number of bytes in Parmbuf               
        lea     di,gn_sel_index               ; offset to DI (destination)
        mov     cx,1                          ; 1 byte to transfer        
        call    get_ioctl_parms               ; get parameters            
 ; Unlock the address of the notetable
        mov     bl,gn_sel_index                                             
        shl     bl,2                          ; multiply by 4, doubleword   
        xor     bh,bh
        mov     di,bx                         ; DI = offset lock handle
        mov     ax,word ptr ioc_lock_hnd4[di] ; restore lock handle
        add     di,2
        mov     bx,word ptr ioc_lock_hnd4[di] ; DI = 2
        mov     dl,DevHlp_Unlock                                     
        call    dword ptr [DeviceHelp]        ; Call unlock function 
        jnc     fn_end                        ; if no error: end...
        pop     bx                            ; Restore Request packet Address
        mov     rp_status[bx],RP_FN_UNLFAIL   ; error: Unlock failed
        jmp     fn_err
fn_end: pop     bx
fn_err: pop     dx
        pop     cx                                                              
        pop     di                                                              
        pop     es                                                              
        ret                                                                     
ioc_free_notetab  endp

;------------------------------------------------------------------------------
; At this point, help functions for the above functions begin
;------------------------------------------------------------------------------
; Entry Point to GET_IOCTL_PARMS Function
; This function is used to get the Parameters passed by an IOCtl request packet
; Passed parameters:
;   CX: Parameter Buffer length in bytes
;   DI: offset, parameters should be copied to (DSEG)
; Default processing: Lock always short term, any memory
;                     Block until available
;                     Verify access for read
; Return values:  AX contains the RP_xxx code
; The Lock handle is stored in registers 
; IMPORTANT: The variables to which the parameters will be copied must be
;            declared in one block because the transfer will be done by one
;            rep movsb command
;------------------------------------------------------------------------------

get_ioctl_parms proc NEAR
        push    es                            ; Save Request Packet address
        push    dx
        push    cx
        push    bx
; 1. Lock the parameter buffer address and verify access for read (1 byte)
;    Lock Handle is returned in CX and DX!!
        push    di                            ; Save offset of target memory
        mov     di,word ptr rp_ioctl_pbuf[bx+2] ; get Selector of virtual addr
        mov     bl,0                          ; block until available
        mov     bh,0                          ; short term, any memory
        mov     dh,0                          ; read access
        call    lock_verify                   ; call lock_verify subfunction
        pop     di                            ; restore DI
        cmp     ax,RP_DONE
        jne     gp_end                        ; error code in AX
; save lock handle: DX will not be used, ok, CX saved in AX
        mov     ax,cx                         ; save cx in ax
; 2. Get the parameters sent by DosDevIOCtl to the driver
        pop     bx                            ; restore BX register for request
        pop     cx                            ; packet access and CX register
        push    cx                            ; for the parmbuffer length
        push    bx
        push    es
        push    si
        push    ds
        push    ds
        cli
        lds     si,rp_ioctl_pbuf[bx]          ; DS:SI addresses parm buffer
        pop     es                            ; ES:DI addresses target
        rep     movsb                         ; CX bytes from DS:SI -> ES:DI
        pop     ds                            ; restore DS
        sti
        pop     si
        pop     es                            ; restore ES
; 3. Unlock the address of the parameter buffer (AX contains part of handle)
        mov     bx,dx                         ; restore lock handle
        mov     dl,DevHlp_Unlock
        call    dword ptr [DeviceHelp]        ; Call unlock function
        mov     ax,RP_DONE
        jnc     gp_end                        ; if o.k. proceed
        mov     ax,RP_UNL_FAIL                ; error: Unlock failed
gp_end: pop     bx
        pop     cx
        pop     dx
        pop     es
        ret
get_ioctl_parms endp

;-------------------------------------------------------------------------------
; Entry Point to LOCK_VERIFY Function
; This function is a general help function which performs the following tasks:
; 1. Lock the virtual address for parameter or data buffer passed by the
;    request packet of an DosDevIOCtl command.
; 2. Verify the access to a defined number of bytes beginning at this address
; 3. Set appropriate return codes to indicate success of operation
; Passed parameters: DI = selector of virtual address to be locked and verified
;                    BL = 1=Block until avail.   0=Return if not immed. avail.
;                    BH = 0=short term, any memory 1=long term, any memory
;                         3=long term, high memory 4=short term, any, verifylock
;                    CX = Number of bytes to be verified (0 - 64k)
;                    DH = Type of access: 0=READ 1=READ/WRITE
; Return values    : CX and DX: Lock handle
;                    AX = return code (if not 0: CX and DX invalid!!)
;-------------------------------------------------------------------------------
                                                                                
lock_verify     proc NEAR
        push    di
        mov     ax,di                         ; AX = Selector
        mov     dl,DevHlp_Lock
        call    dword ptr [DeviceHelp]        ; call lock function
        jnc     lv_1
        mov     ax,RP_LV_FAIL1                ; error: lock failed
        jmp     lv_end
lv_1:   push    ax                            ; save the lock handle on stack
        push    bx
        mov     ax,di                         ; AX = Selector
        mov     di,0                          ; Offset for verify
        mov     dl,DevHlp_VerifyAccess
        call    dword ptr [DeviceHelp]
        jnc     lv_2                          ; if o.k. -> proceed
; verify failed, now try to unlock the address
        pop     bx                            ; restore handle for UNLOCK
        pop     ax
        mov     dl,DevHlp_Unlock      
        call    dword ptr [DeviceHelp]
        jnc     lv_3
; lock o.k., verify failed and unlock failed
        mov     ax,RP_LV_FAIL3                ; error
        jmp     lv_end
; lock o.k., verify failed and unlock o.k.
lv_3:   mov     ax,RP_LV_FAIL2                ; error
        jmp     lv_end
; lock and verify o.k. -> set up return values
lv_2:   pop     dx                            ; restore lock handle to
        pop     cx                            ; cx:dx register pair for return
        mov     ax,RP_DONE                    ; returncode ok (rp_done)
lv_end: pop     di
        ret
lock_verify        endp

;------------------------------------------------------------------------------
; Entry Point to MIDI_DATAOUT function (help function)
; This function sends data to the MPU from the Playbuffer. It uses midi_analyze
; to determine the number of bytes to transfer. pl_offset is used as
; pointer into the Playbuffer.
; pl_accu_time is accumulated with timing from each data sent (F8 or MIDI).
; It is initially filled by the application using bytes 2-5 in the buffer.
; After that: these bytes are used as feedback to the application of the last
; events abs time, which has been sent comletely (incl. F8 splitting).
; pl_accu_time is used with the passed absolute timing value from the playbuf
; to calculate the delta time needed. if delta >= 240: split into F8 sent to
; the MPU.
; Call: -from interrupt handler during processing TDRs
;       -from send_command, if TDR is received instead of expected ACK
;------------------------------------------------------------------------------

midi_dataout  proc NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
; Normal procesing of Track Data Request:
; pl_offset points to the PlayBuffer data to be sent to the MPU. This is data
; of fixed length 8 bytes. F8 timing overflows no longer stored in Buffer and
; must be generated by analyzing the delta time, which is the difference bet-
; ween the passed abs_time and the accu_time value work variable.
; All Play Buffer data is assumed to have the same format:
; Byte 1:   Track Number (0..63)
; Byte 2-5: 32 Bit Absolute Timing
; Byte 6:   MIDI Status Byte (no running status is used in Playbuffer)
; Byte 7-8: MIDI Data Bytes

        mov     si,pl_offset                  ; offset of next data to send
        .386P
        push    ecx
        push    ds                            ; get data from playbuffer
        mov     ds,[Sel_PLAY]
        mov     ah,byte ptr [ds:si]           ; AH = track number
        inc     si                            ; skip TRK number
        mov     ecx,dword ptr [ds:si]         ; ECX = absolute timing value
        inc     si
        inc     si
        inc     si
        inc     si                            ; SI = offset of status byte
        mov     dl,byte ptr [ds:si]           ; status byte
        mov     al,byte ptr [ds:si+1]         ; data1 byte (pitch?)
        mov     dh,byte ptr [ds:si+2]         ; data2 byte (velocity?)
        pop     ds
        mov     md_status,dl                ;N  save status
        mov     md_data1,al                 ;N  save data 1  (pitch?)
        mov     md_data2,dh                 ;N  save data 2  (velocity?)

; check: if trknum = 0FCh (Stop code): only output timing and 0FCh to MPU
; and nothing else. This will stop playback. In new design, FC is made inde-
; pendent from track, because otherwise, a FC will not be recognized, if the
; track containing the FC is flagged inactive in sa_acttrk.
; i.e. FC end is found in play buffer instead of trknum byte
        cmp     ah,PLAY_STOP_CODE
        jne     md_a                          ; if no stopcode
        pop     ecx
        mov     ah,078h                       ; AH = Timing byte for FC End
        call    send_data                                              
        cmp     ax,0
        jne     md_i                          ; end, AX as return value
        mov     ah,PLAY_STOP_CODE             ; AH = FC End stop code
        call    send_data                                                   
        jmp     md_i                          ; end, AX as return value

; normal track processing
md_a:   mov     pl_trknum,ah                  ; store Tracknumber (VALIDATE ?)
        mov     pl_abs_time,ecx               ; store absolute timing
        sub     ecx,dword ptr pl_accu_time    ; calculate delta time and store
        mov     dword ptr pl_delta_time,ecx   ; to pl_delta_time

; NOW: check, if timing overflow must be generated: if pl_delta_time >= 240:
; generate F8, i.e.: only output F8 to the MPU, do NOT update pl_offset and  
; increase pl_accu_time by 240. Then leave midi_dataout and wait for next
; processing. Then a new delta will be calculated with the increased accu
; time value.
        cmp     dword ptr pl_delta_time,240
        jb      md_q                          ; if delta <240: normal proc.
; at this point: Delta Time >=240, so a timing overflow F8 must be sent.
        add     dword ptr pl_accu_time,240    ; increase accu timing !!!
        pop     ecx
        .286P
        mov     ah,TIMING_OVERFLOW            ; AH = Timing overflow
        call    send_data
        jmp     md_i                          ; end, AX as return value

; at this point: delta time <240, so normal processing with call to 
; midi_analyze.
md_q:   .386P
        add     dword ptr pl_accu_time,ecx    ; increase accu timing !!!
        pop     ecx
        .286P
; analyze tracknumber: if track flagged active: do a normal send of data
        xor     bh,bh                                                   
        mov     bl,ah                         ; BX = Index to sa_acttrk 
        mov     al,sa_acttrk[bx]              ; AL = Trackstatus        
        cmp     al,1                          ; active status? (1)
        je      md_q2                         ; OK, analyze and output
; else: track is flagged inact: if no NOTE or NOTE on: skip and send NOP
;       NOTE OFF: Check, if this note is active in notetab:
;                 if active  : NOTE OFF senden
;                 if inactive: skip and send NOP to MPU
        and     dl,11110000b                  ; strip off channel info
        cmp     dl,090h
        jne     md_nop                        ; if no NOTE: send nop
        cmp     dh,0                          ; if note: test velocity
        jne     md_nop                        ; if note on: send nop
; erster testcode, nicht optimiert, nur zum testen, ob die logik stimmt
        push    bx                            ; check, if note is active
        push    dx
        mov     bh,pl_trknum
        mov     dl,md_data1                   ; pitch from playbuffer
        mov     bl,md_status                  ; status
        and     bl,00001111b                  ; get channel number
        call    query_note_table              ; AH returns status info: 
        pop     dx                            ; 0 = inact, !=0: active
        pop     bx
        cmp     ah,0                          ; if note not set active:
        jne     md_q2                         ; do not send noteoff

md_nop: mov     ah,byte ptr pl_delta_time     ; AH = least sign. byte delta
        call    send_data                     ; SEND delta_time to MPU     
        cmp     ax,0                                                       
        jne     md_i                          ; end, AX as return value    
        mov     ah,NO_OPERATION               ; AH = No operation MPU mark
        call    send_data                                              
        cmp     ax,0                                                   
        jne     md_i                          ; end, AX as return value
        jmp     md_k1                         ; update offset and feedback
; not updating the offset could result in errors, because if no more active
; tracks would be found in play buffer, no update of the offset would stop
; fill play buffer from adding new events. this would result in an endless
; loop, and no following active track events would be played.

md_q2:  call    midi_analyze                  ; CX returns number of bytes
                                              ; CX = 0 means error !!!
; midi_analyze has different jobs to do:
; 1. Set CX to correct number of bytes to be sent to the MPU
; 2. Set SI to the start offset to begin the output
; 3. Determine, if running status can be used to decrease MIDI load and set
;    SI and CX according to these results. So after analyze only CX bytes
;    starting from SI must be copied and then update feedback and pl_offset.
; 4. Do online changes to velocity data (write back to playbuffer)
; 5. Determine the correct MIDI channel (write back to playbuffer). This
;    covers cc_chantab processing and problems of note off processing.
; 6. return mr_status, which indicates, if update of note table needed

; important: first output the delta time to the MPU, because this is not
; contained in the PLAY Buffer directly.
        mov     ah,byte ptr pl_delta_time     ; AH = least sign. byte delta
        call    send_data                     ; SEND delta_time to MPU
        cmp     ax,0                                                     
        jne     md_i                          ; end, AX as return value

; now start a loop, which sends CX bytes to the MPU via send_data, beginning
; at the offset in SI, as calculated by midi_analyze
md_lp:  push    ds                            ; start sending data
        mov     ds,[Sel_PLAY]                 ; establish GDT Selector access  
        mov     ah,byte ptr [ds:si]           ; AH = byte to be sent to MPU    
        pop     ds
        inc     si                            ; SI = offset of next byte
        call    send_data                     ; send the byte in AH
        cmp     ax,0
        jnz     md_i                          ; error in send_data: return AX
        loop    md_lp                         ; if CX > 0: LOOP
; now all bytes needed for the event are sent to the MPU. Update the Note Table
; now according to the event sent, i.e. check the event: if note ON/OFF: update
; else (controller...) no update. mr_status is set to 0 by midi_analyze, if no
; NOTE ON or NOTE OFF event has been processed.
; velocity is without fader, i.e.: if fader is set to 0: the bit will be set
; to active, even if actually a note off has been sent. This is, because the
; appl itself will stay synchronized with the notetab.
        cmp     mr_status,0                   ; check Status byte
        je      md_k1                         ; if 0 : no table update needed
        mov     bl,md_status                  ; BL = Status Byte
        and     bl,00001111b                  ; BL = channel index (original)
        mov     bh,pl_trknum                  ; BH = Tracknumber (0..63)
        mov     dl,md_data1                   ; DL = PITCH (original)
        mov     dh,md_data2                   ; DH = VELOCITY (original)
        call    set_note_table                ; call update note table

; now all bytes needed are sent to the MPU. Set the index SI to the offset
; where the next data will be read from. This will also be the Feedback value
; to the application used for refilling the play buffer.
md_k1:  mov     di,pl_offset                  ; DI: Startoffset of last event
        add     di,8
        cmp     di,midi_play_bufsz            ; turnaround ?
        jne     md_k
        mov     di,8                          ; turnaround....
; now copy new offset value to play buffer bytes 1+2 
; NEW: also copy the pl_abs_time value and output to the playbuffer at
; offset 2. This is for feedback to the appl via the playbuffer.
md_k:   .386P
        push    ecx                                                            
        mov     ecx,pl_abs_time               ; abs time of last event
        push    ds
        mov     ds,[Sel_PLAY]                 ; establish GDT Selector access
        mov     si,0                          ; to bytes 1+2 in playbuffer.
        mov     word ptr [ds:si],di           ; write new offset
        mov     si,2                          ; offset accu timing
        mov     dword ptr [ds:si],ecx         ; write absolute timing
        pop     ds                            ; restore ES register
        mov     pl_offset,di                  ; actualize pl_offset
        pop     ecx                                                            
        .286P   
md_i:   pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
midi_dataout endp

;-------------------------------------------------------------------------------
; Entry Point to MIDI_ANALYZE function (help function)
; This function analyzes Data Bytes contained in the MIDI data buffer during
; the play function.
; INPUT PARAMETER: SI = Offset in the buffer, at which analyze starts
;                  Also used: MPU_MODE Variable and ma_mode_play
; RETURN VALUES  : CX = number of Bytes which belong together (0=error)
; The function is used by ioc_play.
; The Function is only designed for use with intelligent MODE of the MPU401.
; Perhaps change to include check for MPU_MODE UART/INT??
;-------------------------------------------------------------------------------
 
midi_analyze    proc NEAR
;       push    si
        push    di
        push    es
        push    dx
        push    bx
        mov     ah,0                          ; default returncode
; get MIDI Status byte from offset SI to AL
        push    ds
        mov     ds,[Sel_PLAY]                 ; establish GDT Selector access
        mov     al,byte ptr [ds:si]           ; AL = MIDI Status byte
        mov     dh,al                         ; save in DH
        mov     dl,byte ptr [ds:si+1];        ; DL = data 1 (PITCH??)
        mov     ch,byte ptr [ds:si+2];        ; CH = data 2 (VELOCITY??)
        pop     ds
        mov     mr_status,0                   ; init return status to 0
                                              ; 0 indicates: no notetable upd.

; now analyze Statusvalue: 80-EF: MIDI data to be processed, get length and
; check for possible running status
; if value <80: This is no midi data, reserved for special processing!!!

mr_ti0: cmp     al,080h                       ; if status < 80h :
        jnb     mr__sp                        ; first: check channel change
        jmp     mr_spc                        ; jump to special processing
 
; NNN here first do a channel change check for online channel change. If 
; a change is needed: change in playbuffer before rest of processing.
mr__sp: xor     bh,bh
        mov     bl,pl_trknum                  ; BX = Track idx for chantab
        mov     dh,cc_chantab[bx]             ; DH = NEW channel from table
                                              ; 0-15=channel, 16=MULT
        mov     cl,al
        and     cl,00001111b                  ; CL = original Status Channel

; CHECK STATUS: if note off: change channel info in status byte to value from
;                            query_notetab_chan
;               else:        change channel info in status byte to cc_chantab
;                            value or do no change, if cc_chantab = 16

        mov     bh,bl                         ; BH = Trackindex (0..63)
        mov     bl,cl                         ; BL = Channelindex ORG
        mov     cl,al                         ; CL = Statusbyte from buffer
        and     cl,11110000b                  ; extract status type
        cmp     cl,090h                       ; if no NOTE status byte
        jne     mr_ch2                        ; CHANGE with no more checks
        cmp     ch,0                          ; check velocity
        jne     mr_ch2                        ; if NOTE ON: CHANGE no checks

; => note off: send note off, but use the channel stored from last note on
; for this note, which is found in notetable at the ORG channel from status
; in playbuffer. Do not use the new channel info from cc_chantab, nor use the
; ORG channel from status!!!

; query_note_table: BH=TrkIndex, BL=Channelindex ORG, DL=Pitch of Note
        call    query_notetab_chan            ; AH returns channelnumber
        mov     dh,ah                         ; DH = chan from last NOTE ON
        jmp     mr_qq

; now change channel in status and write back to playbuffer
; only change channel to cc_chantab value, if it is not 16. If 16: MULT,
; so use the original value from status, i.e. no change
mr_ch2: cmp     dh,15                         ; NO CHANGE in this case (MULT)
        jg      mr_ti1                        ; normal processing

mr_qq:  and     al,11110000b                  ; clear channel info
        or      al,dh                         ; set new channel
        push    ds           
        mov     ds,[Sel_PLAY]
        mov     byte ptr [ds:si],al           ; write back new status
        pop     ds
; normal processing: at this point: AL contains status with eventual changed
; channel info.
mr_ti1: mov     dh,al                         ; copy status also to DH
        cmp     al,0BFh                       ; value is >= 80
        jg      mr_ti2                        ; if >  BF: proceed              
        mov     cx,3                          ; if <= BF: 3 bytes to send
; additional checking for fader function                                       
        cmp     al,09Fh                       ; Check, if NOTE ON/OFF          
        jg      mr_rsc                        ; if not: runstat check
        mov     mr_status,al                  ; store status for notetab
        inc     si                            ; if yes:
        xor     bh,bh                         ; BX = current tracknum as index
        mov     bl,pl_trknum                  ; into fader values array        
        mov     dl,pl_fader[bx]               ; DL = fader value to use        
        push    ds                                                             
        mov     ds,[Sel_PLAY]
        mov     bh,byte ptr [ds:si]           ; BH = pitch
        inc     si
        mov     al,byte ptr [ds:si]           ; AL = current velocity value  
        mov     bl,al                         ; save for mr_velocity storage
        cmp     al,0                          ; check velocity for 0           
        jne     mr_tis                        ; if > 0 : proceed               
        mov     ah,0                          ; if 0: value will be 0          
        jmp     mr_tid                                                         
mr_tis: mul     dl                            ; Multiply AL with fader (DL)    
        cmp     dl,0                          ; Do a correction of the result  
        je      mr_tid                        ; but only for fader values      
        inc     ah                            ; which are > 0                  
mr_tid: mov     byte ptr [ds:si],ah           ; result in AH, write back  
        pop     ds                            ; velocity to play buffer.
        mov     mr_pitch,bh                   ; store pitch for notetab
        mov     mr_velocity,bl                ; store org velocity for notetab
        sub     si,2                          ; reset SI to offset STATUS
        jmp     mr_rsc

mr_ti2: cmp     al,0DFh                       ; value is >  BF                 
        jg      mr_ti3                        ; if >  DF: proceed              
        mov     cx,2                          ; if <= DF: 2 bytes to send
        jmp     mr_rsc

mr_ti3: cmp     al,0EFh                       ; value is >  DF                 
        jg      mr_ti4                        ; if >  EF: proceed              
        mov     cx,3                          ; if <= EF: 3 bytes to send
        jmp     mr_rsc

mr_ti4: mov     cx,1                          ; value is >  EF
        jmp     mr_end                        ; only F9 and FC are valid
                                              ; no Runstat check in this case

; now do a check, if status must be sent, or if running status may be 
; generated. Compare to ma_runst_play, if equal: dec cx and inc si and store
; new runstatus to ma_runst_play for next data.
mr_rsc: cmp     dh,ma_runst_play
        jne     mr_en1                        ; if != : save runst and return
        inc     si                            ; if running status: inc offset
        dec     cx                            ; and dec number of bytes
        jmp     mr_end                        ; return

mr_spc:                                       ; special processing, not yet
                                              ; implemented, run on error
; error: a byte could not be analyzed
mr_err: mov     cx,0                          ; CX=0 means error in analyze
        jmp     mr_end
mr_en1: mov     ma_runst_play,dh              ; save running status
mr_end: pop     bx
        pop     dx
        pop     es
        pop     di
;       pop     si
        ret
midi_analyze    endp

;------------------------------------------------------------------------------- 
; Entry Point to SetNoteTable function (help function)
; This function is called, after a NOTE ON or NOTE OFF has been sent by
; midi_dataout. It actualizes the NOTE table to reflect the new status.
; INPUT PARAMETER: BH = Track Index of NOTE SENT (0..63)
;                  BL = original Channel Index of note event (0..15)
;                  DL = original Pitch of note event
;                  DH = original Velocity of note event (0 : off, >0 : on)
; RETURN VALUES  : none, only updated note table as result.
;
;-------------------------------------------------------------------------------

set_note_table  proc NEAR
        push    di
        push    si
        push    dx
        push    cx
        push    bx

; first get selector index of Sel_NOTETAB[idx] for this track.
; use trackindex for this purpose:
        mov     cl,bh
        shr     cl,3
        shl     cl,1                          ; correction for WORD index
        xor     ch,ch
        mov     di,cx                         ; use DI as GDT index

; now get offset of word for specific note within above GDT Selector
        and     bh,00000111b                  ; BH = Trackidx within Selector
        shl     bh,4                          ; factor 4096
        add     bh,bl                         ; BH += Channelidx (factor 256)
        shl     dl,1                          ; Pitch correction to word idx
        mov     ch,dh                         ; CH = saved original velocity
        xor     dh,dh
        add     bx,dx                         ; BX = index to note word
        mov     si,bx                         ; SI = Word Offset within sel.

        mov     dl,mr_pitch                   ; updated Pitch value
        mov     dh,mr_status                  ; updated MIDI status
        and     dh,00001111b;                 ; DH = actual send channel

        push    ds
        mov     ds,Sel_NOTETAB[di]            ; address notetab through GDTSEL
; now process word in notetable: set bit for note on, reset for note off
; for distinction: use velocity byte passed. The appl never passes NOTE OFF
; explicitly, but always uses NOTE ON with velocity 0, which is conform to
; the MIDI Standard.
        mov     byte ptr [ds:si+1],dl         ; copy updated pitch info
        cmp     ch,0                          ; velocity 0: NOTE OFF
        jne     nt_on                         ; velocity != 0: NOTE ON
        and     byte ptr [ds:si+1],01111111b  ; reset bit for note off
        jmp     nt_end
nt_on:  mov     bh,byte ptr [ds:si+1]
        test    bh,10000000b
        jz      nt_aa
        int 3                  ; double active interrupt
nt_aa:  or      byte ptr [ds:si+1],10000000b  ; set bit for note on
nt_end: mov     byte ptr [ds:si],dh           ; save actual NOTE ON channel
        pop     ds
        pop     bx
        pop     cx
        pop     dx
        pop     si
        pop     di
        ret
set_note_table  endp

;------------------------------------------------------------------------------
; Entry Point to QueryNoteTable function (help function)
; This function determines, if a note in a track for a specified MIDI channel
; is active by querying the appropriate bit in the note table.
; The function is used for online midi channel change.
; INPUT PARAMETER: BH = Track Index of NOTE SENT (0..63)                       
;                  BL = Channel Index of NOTE SENT (0..15)                     
;                  DL = Pitch of NOTE SENT                                     
; RETURN VALUES  : AH = 0, if requested bit not set
;                  AH any other value (1,2,4,8,16,32,64,128) if bit set
;------------------------------------------------------------------------------
                                                                               
query_note_table  proc NEAR
        push    di
        push    si                                                             
        push    dx                                                             
        push    cx                                                             
        push    bx             

; first get selector index of Sel_NOTETAB[idx] for this track.                
; use trackindex for this purpose:                                            
        mov     cl,bh                                                         
        shr     cl,3                                                          
        shl     cl,1                          ; correction for WORD index     
        xor     ch,ch                                                         
        mov     di,cx                         ; use DI as GDT index           
                                                                              
; now get offset of word for specific note within above GDT Selector        
        and     bh,00000111b                  ; BH = Trackidx within Selector 
        shl     bh,4                          ; factor 4096                   
        add     bh,bl                         ; BH += Channelidx (factor 256) 
        shl     dl,1                          ; Pitch correction to word idx  
        mov     ch,dh                         ; CH = saved original velocity  
        xor     dh,dh                                                         
        add     bx,dx                         ; BX = index to note word       
        mov     si,bx                         ; SI = Word Offset within sel.  

        push    ds                                                             
        mov     ds,Sel_NOTETAB[di]            ; address notetab through GDTSEL
; now check the bit in notetable using the BIT set in AH:
        mov     ah,10000000b
        and     ah,byte ptr [ds:si+1]         ; AH = 0 if bit not set
qt_end: pop     ds                            ; else: AH != 0
        pop     bx                                                             
        pop     cx                                                             
        pop     dx                                                             
        pop     si     
        pop     di
        ret                                                                    
query_note_table  endp

;------------------------------------------------------------------------------
; Entry Point to QueryNoteTableChannel function (help function)
; This function determines the Channelnumber, at which the note, which is
; stored in the note table at channel BL, has been actually sent with NOTE ON.
; The function is used for online midi channel change.                         
; INPUT PARAMETER: BH = Track Index of NOTE SENT (0..63)                       
;                  BL = Channel Index of NOTE SENT (0..15) (where stored)
;                  DL = Pitch of NOTE SENT                                     
; RETURN VALUES  : AH = the actually used channel for the NOTE ON
;------------------------------------------------------------------------------
                                                                               
query_notetab_chan proc NEAR
        push    di                                                             
        push    si                                                             
        push    dx                                                             
        push    cx                                                             
        push    bx                                                             
                                                                               
; first get selector index of Sel_NOTETAB[idx] for this track.                 
; use trackindex for this purpose:                                             
        mov     cl,bh                                                          
        shr     cl,3                                                           
        shl     cl,1                          ; correction for WORD index      
        xor     ch,ch                                                          
        mov     di,cx                         ; use DI as GDT index            
                                                                               
; now get offset of word for specific note within above GDT Selector       
        and     bh,00000111b                  ; BH = Trackidx within Selector 
        shl     bh,4                          ; factor 4096                   
        add     bh,bl                         ; BH += Channelidx (factor 256) 
        shl     dl,1                          ; Pitch correction to word idx  
        mov     ch,dh                         ; CH = saved original velocity  
        xor     dh,dh                                                         
        add     bx,dx                         ; BX = index to note word       
        mov     si,bx                         ; SI = Word Offset within sel.  
 
        push    ds                                                             
        mov     ds,Sel_NOTETAB[di]            ; address notetab through GDTSEL
; now check the bit in notetable using the BIT set in AH:                      
        mov     ah,byte ptr [ds:si]           ; AH = Channel of last NOTE ON
        pop     ds
        pop     bx                                                             
        pop     cx                                                             
        pop     dx                                                             
        pop     si                                                             
        pop     di                                                             
        ret                                                                    
query_notetab_chan endp

;-------------------------------------------------------------------------------
; Entry Point to INITIALIZE ROUTINE of the Device Driver
; Initialize does the following:
; 1. Get Entry Point for DevHelp Services of Kernel and store in DeviceHelp
; 2. Process DEVICE= parameters from CONFIG.SYS:
;    - Interrupt Level (/I:xx)  important: always use 2 digits (05, not 5)
;    - IO Address      (/A:xxx) important: always use 3 digits (033, not 33)
;    Get String to write to console and calculate the values for
;    mpu_irq, mpu_data_port and mpu_status_port with function ascii_2_int
; 3. Allocate a GDT Selector table for further use by the driver
; 4. Try an RESET to the MPU Card. If successful: OK
;                                  If not: Driver load error.
; 5. Register the Interrupt Handler Routine to the OS/2 Kernel by using a
;    Device Helper Service         If successful: OK
;                                  If not: Interrupt error
; 6. Output the initial Message to the Console, including the VERSION and
;    Revision Level of the installed MPU Card.
; 7. Store the ending address of the CODE and DATA Segments of the Driver to
;    the Request Packet. The Kernel removes all data beyond in the Segments.
;    The Data and Code for the Initialization must therefore be the last in 
;    the Data and the Code Segments!!!!                                     
;
; The INIT Section is entered, when the DEVICE = MPU401$.SYS Statement is
; processed during Boot.
;
;
; IN CASE OF FAILURE     DEALLOC GDT Selectos  !!!!!!!!!!!!!!!!!!!
;
;
;-------------------------------------------------------------------------------

end_of_driver   label  byte              ; Last Byte of Device Driver, the Code
                                         ; of INITIALIZE will be discarded after
                                         ; INIT is ready. Kernel will do no more
                                         ; Call to it.
initialize  proc FAR                                                        
        ASSUME  CS:cseg,DS:dseg,ES:rp    ; ES:BX = Request Packet Address !!
        mov     ax, word ptr rp_in_devhlp[bx]   ; Get address for devhlp
        mov     word ptr DeviceHelp,ax          ; routines from request packet
        mov     ax, word ptr rp_in_devhlp+2[bx] ; passed by the kernel and
        mov     word ptr DeviceHelp+2,ax        ; store in DeviceHelp variable
; Test: work with input parameters from config.sys
        push    si
        push    cx
        push    bx
        push    es
        push    di
        les     di,rp_in_args[bx]               ; Address of parms in es:di 
        mov     cx,50                           ; maximum 50 characters
init_0: mov     al,byte ptr es:[di]
; Now scan the line and search interrupt level and address (/I: and /A:)
        cmp     al,02Fh                         ; character = / ?
        jne     init_n
        mov     al,byte ptr es:[di+1]           ; get next char
        cmp     al,049h                         ; char = I ?
        je      init_i
        cmp     al,069h                         ; char = i ?
        je      init_i
        cmp     al,041h                         ; char = A ?
        je      init_a
        cmp     al,061h                         ; char = a ?
        je      init_a
        jmp     init_n
init_i: mov     al,byte ptr es:[di+3]           ; jump over ':'
        mov     bx,0
        mov     byte ptr int_lev_ascii[bx],al   ; write first char to string
        mov     al,byte ptr es:[di+4]
        inc     bx
        mov     byte ptr int_lev_ascii[bx],al   ; write second char to string
        lea     si,int_lev_ascii                ; offset of string to SI
        call    ascii_2_int                     ; call conversion to integer
        mov     mpu_irq,al                      ; store interrupt level
        jmp     init_n
init_a: mov     al,byte ptr es:[di+3]           ; jump over ':'
        mov     bx,0 
        mov     byte ptr addr_ascii[bx],al      ; write first char to string
        mov     al,byte ptr es:[di+4]
        inc     bx
        mov     byte ptr addr_ascii[bx],al      ; write second char to string
        mov     al,byte ptr es:[di+5]
        inc     bx 
        mov     byte ptr addr_ascii[bx],al      ; write third char to string
        lea     si,addr_ascii                   ; offset of string to SI
        call    ascii_2_int                     ; call conversion to integer
        mov     mpu_data_port,ax                ; store IO address of data port
        inc     ax
        mov     mpu_status_port,ax              ; store IO address of stat port
init_n: inc     di
        loop    init_0
        pop     di
        pop     es
        pop     bx
        pop     cx
        pop     si
; This part allocates a GDT Selector for further use by the driver
        push    es
        push    bx
        mov     ax,ds                         ; First store the address of
        mov     es,ax                         ; the Selector Array into
        mov     di,OFFSET GDT_Select          ; ES:DI register pair
        mov     cx,Num_GDT_Sel                ; Number of Selectors to allocate
        mov     dl,DevHlp_AllocGDTSelector    ; DevHlp command code in DL
        call    dword ptr [DeviceHelp]
        jc      fail                          ; if error....
        pop     bx
        pop     es
; This part sends a RESET command to the MPU, using the given Data for 
; Port Addresses. It tries the RESET 2 times!
init_1: mov     ah,RESET_CMD                  ; AH = Command Code RESET (FFh)
        call    send_command                  ; Call to Send_Command Procedure
        cmp     ax,0                          ; Returncode = 0?
        je init_2                             ; if YES: proceed with INIT
        mov     ah,RESET_CMD                  ; else....
        call    send_command                  ;      TRY RESET again
        cmp     ax,0                          ; Returncode = 0?                
        je init_2                             ; if YES: proceed with INIT
; this Part is processed, if the initial Reset of MPU fails.
        push    es
        push    bx                                                             
fail:  .386P                                  ; Output Failure Message
        push stdout                           ; to the Console. DosWrite is    
        push ds                               ; usable during INIT.            
        push offset ds:ResetFail              ; INIT runs at RING 3
        push RF_len
        push ds                                                                
        push offset ds:wlen                                                    
        call DosWrite                         ; Write to Console               
        mov word ptr es: [bx + 3 ],RP_DONE
        .286P                                                                  
        pop     bx                                                             
        pop     es                                                             
        jmp     init_e                        ; Jump to End of INIT
; This part registers the Interrupt Handler Routine for the MPU Card to the
; OS/2 Kernel.
init_2: push    bx                            ; Proceed here after RESET OK 
        push    dx
        mov     ax,offset CS:interrupt_hdlr   ; AX=Interrupt Hdlr address
        mov     bh,0
        mov     bl,mpu_irq                    ; BX=Interrupt Level
        mov     dh,0                          ; Non Shared Int
        mov     dl,DevHlp_SetIRQ              ; Function Code for registr.
        call    dword ptr [DeviceHelp]        ; Aufruf
        jnc     init_3                        ; If OK -> Label Init_3
; this Part is processed, if the Interrupt Hander register fails
        pop     dx
        push    es                                                          
       .386P                                  ; Output Failure Message      
        push stdout                           ; to the Console. DosWrite is 
        push ds                               ; usable during INIT.         
        push offset ds:RegisFail              ; INIT runs at RING 3
        push RS_len
        push ds                                                             
        push offset ds:wlen                                                 
        call DosWrite                         ; Write to Console            
        mov word ptr es: [bx + 3 ],RP_DONE
        .286P     
        pop     es    
        pop     bx                                                      
        jmp short init_e                      ; Jump to End of INIT         
; This part writes the Initial Message to the Console if the initialization
; was o.k. (RESET and Register Interrupt Handler)
init_3: push    es                            ; Proceed here after RESET OK  
       .386P                                  ; Output Initial Message
        push stdout                           ; to the Console. DosWrite is
        push ds                               ; usable during INIT.
        push offset ds:Idin                   ; INIT runs at RING 3
        push IdIn_len                                                          
        push ds                                                                
        push offset ds:wlen                                                    
        call DosWrite                         ; Write to Console
        mov word ptr es: [bx + 3 ],RP_DONE
        .286P
        push    cx                            ; generate beep with
        mov     bx,beep_a2                    ; 880 hertz
        mov     cx,dur_1                      ; 300 milliseconds
        mov     dl,DevHlp_Beep                ; Select Device Helper Function
        call    dword ptr [DeviceHelp]        ; Call Device Helper Service
        pop     cx
        pop     es
        pop     dx
        pop     bx
; Now the offsets to the end of the Code and the DATA Segments are passed to
; the Kernel via the Request Packet fields.
init_e: mov     word ptr rp_in_cseg_end[bx],offset end_of_driver ; CODE Seg. END
        mov     word ptr rp_in_dseg_end[bx],offset dsegend       ; DATA Seg. END
        ret                                                                     
initialize   endp                      ; E N D  of  I n i t i a l i z e

;-------------------------------------------------------------------------------  
; Entry Point to ascii_2_int function (help function for initialize)
; This function converts the string, DS:SI points to, to an integer value which
; is returned in register AX. The string is assumed to be in HEX (A-F or a-f).
; The end delimiter for the string MUST be a BLANK. No check for 0-9 or a-f
; characters is done, also no string length restrictions are checked!
; The function is only used during initialization, it is purged after init is
; complete.
;-------------------------------------------------------------------------------

ascii_2_int  proc
        push    bx                            ; Save registers
        push    cx
        push    dx
        mov     cx,0                          ; Determine length of string by
ai_1:   mov     al,byte ptr[si]               ; searching the blank delimiter 
        cmp     al,blank                      ; or carriage return (for last
        je      ai_2                          ; parameter in config.sys line
        cmp     al,cr
        je      ai_2
        inc     cx                            ; increment length counter
        inc     si                            ; pointer to next character
        jmp     ai_1
ai_2:   dec     si                            ; SI: points to last valid char
        mov     factor,1                      ; initialize factor to 1
        mov     dx,0                          ; initialize dx
ai_3:   xor     ah,ah
        mov     al,byte ptr[si]               ; get character value of string,
        sub     al,48                         ; transform to decimal value,
        cmp     al,9                          ; compare: if value>9, a higher
        jna     ai_4                          ; char than 9 has been entered
        sub     al,7                          ; interpret as A-F
        cmp     al,15                         ; compare: if value>15, a higher
        jna     ai_4                          ; char than F has been entered
        sub     al,32                         ; interpret as a-f
ai_4:   push    dx                            ; save DX during multiply
        mul     factor                        ; multiply and
        pop     dx
        add     ax,dx                         ; accumulate values in dx
        xchg    ax,dx
        push    dx                            ; save DX, mul will change it
        mov     ax,factor                     ; adjust factor for next
        mul     factor16                      ; conversion
        mov     factor,ax
        pop     dx
        dec     si                            ; pointer to previous character
        loop    ai_3                          ; Repeat until CX=0
        mov     ax,dx                         ; copy result to ax
ai_e:   pop     dx                            ; restore used registers
        pop     cx
        pop     bx
        ret
ascii_2_int     endp

CSEG    ends                                  ; END OF CODE SEGMENT
        end                                   ; End of the Source Code
;*******************************************************************************
