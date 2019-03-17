/*                                                                             
*******************************************************************************
* Quelle:       MIDIMIX1.C
* Programm:     MIDISEQ.EXE                                                    
*--------------------------------------------------------------------------    
* Procedures:   MixDlgProc          Mixpult Dialog
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
#define  INCL_WIN

#include <os2.h>
 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
 
 
#include "midiseq.h"
#include "mididlg.h"               /* dialog window constants                */
                            
extern HAB     hAB;                     /* Anchor Block Handle               */
extern WRKBUF WrkBufMap[SEQ_TRACKS];    /* Administration Structure          */                                                   
extern HWND   hWndMainDialog;           /* Handle of Main Dialog Window      */
extern UCHAR  ucFader[SEQ_TRACKS];      /* Fader value table                 */

HWND          hWndMixDialog;            /* Mixer Dialog handle               */
                                                                             
/* Level Display                                                             */
HBITMAP hBmpLevel[17];                                                         
UCHAR   ucIndex;                        /* Help variable                     */
HPS     hPSLevel;                              

/*                                                                                                             
*******************************************************************************
* Call       : MRESULT  EXPENTRY   MixDlgProc(hWnd, Msg, mp1, mp2)             
*                                                                              
* Parameter  : HWND       hWnd     Handle of Mixer Dialog                      
*              ULONG      msg      message                                     
*              MPARAM     mp1      message parameter 1                         
*              MPARAM     mp2      message parameter 2                         
*                                                                              
* Return     : MRESULT                                                         
*                                                                              
* Description: This procedure is used to set the record filters                
*******************************************************************************
*/                                                                             
                                                                               
MRESULT  EXPENTRY   MixDlgProc(HWND       hWnd,                                
                               ULONG      Msg,                                 
                               MPARAM     mp1,                                 
                               MPARAM     mp2)                                 
                                                                               
{                                                                              
APIRET rc;
UCHAR ucLoop;        
ULONG ulSliderId;                       /* Help for ID of Slidercontrol      */
UCHAR ucTrkIndex;                       /* Trackindex for slider change      */
ULONG ulSlFaderValue;
CHAR  DlgText[40];
              
switch (Msg)                                                                   
    {                                                                          
    case WM_INITDLG:                    /* INIT Dialog Controls              */
       /* Code for INITIALIZATION                                            */
       hPSLevel = WinBeginPaint(hWnd, (HPS)0UL, NULL);   /* Load Bitmaps     */
       for(ucIndex=0;ucIndex<17;ucIndex++)
          {                                                                    
          hBmpLevel[ucIndex]=GpiLoadBitmap(hPSLevel,                           
                                           NULLHANDLE,                         
                                           ID_BMP_LEV0+ucIndex,                
                                           12L,        /* width              */
                                           50L);       /* height             */
          }                                                                    
       ucIndex=0;                                                              
       WinEndPaint(hPSLevel);       
 
       /* Set the Slider arm positions to default values, which are found    */
       /* in Workbuffer array..                                              */
       for(ucLoop=0;ucLoop<16;ucLoop++)
          {
          WinSendDlgItemMsg(hWnd,
                            DID_SL_LEV1+ucLoop,
                            SLM_SETSLIDERINFO,
                            MPFROM2SHORT(SMA_SLIDERARMPOSITION,
                                         SMA_INCREMENTVALUE),
                            MPFROMSHORT((USHORT)(63)));
          }
                                    
       hWndMixDialog=hWnd;              /* Store Handle for other purposes   */
       
   /*  ulLevTimerID=WinStartTimer(hAB,
                                  hWnd,                                        
                                  ID_LEV_TIMER,                                
                                  50);                                       */
    break;                              /* end of case INITDLG               */
                                                                               
    case WM_COMMAND:                    /* Pushbuttons send WM_COMMAND       */
        switch (SHORT1FROMMP(mp1))      /* MP1 contains ID of control        */
            {                                                                  
            default:                                                           
                return (WinDefDlgProc(hWnd, Msg, mp1, mp2));                   
            }                                                                  
        break;                                                                 
                                                                               
    case WM_CONTROL:                    /* other controls send WM_CONTROL    */
        switch (SHORT1FROMMP(mp1))      /* MP1 contains ID of control        */
            {
            case DID_SL_LEV1:
            case DID_SL_LEV2:
            case DID_SL_LEV3:
            case DID_SL_LEV4:
            case DID_SL_LEV5:
            case DID_SL_LEV6:
            case DID_SL_LEV7:
            case DID_SL_LEV8:
            case DID_SL_LEV9:
            case DID_SL_LEV10:
            case DID_SL_LEV11:
            case DID_SL_LEV12:
            case DID_SL_LEV13:
            case DID_SL_LEV14:
            case DID_SL_LEV15:
            case DID_SL_LEV16:          /* WM_CONTROL came from Fader Slider */
               /* Check 2nd SHORT value for notifycode in a second switch    */
               /* case construct                                             */
               switch(SHORT2FROMMP(mp1))
                  {
                  case SLN_CHANGE:      /* Slider changed position           */
                  case SLN_SLIDERTRACK:
                     ulSliderId=(ULONG)SHORT1FROMMP(mp1);
                     ucTrkIndex=(UCHAR)(ulSliderId-DID_SL_LEV1);
                     ulSlFaderValue=(ULONG) WinSendDlgItemMsg(hWnd,
                                    ulSliderId,
                                    SLM_QUERYSLIDERINFO,
                                    MPFROM2SHORT(SMA_SLIDERARMPOSITION,
                                                 SMA_INCREMENTVALUE),
                                    NULL);
                     ucFader[ucTrkIndex]=(UCHAR)ulSlFaderValue*4;
                     rc=MPU_Fader();
                  break;
 
                  default:              /* IMPORTANT: do not forget default  */
                     return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
                  }                    
 
            default:
                return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
 
            }                           /* end of switch mp1                 */
        break;                                                                 
                                                                               
    case WM_TIMER:                                                             
        switch (SHORT1FROMMP(mp1))                                             
            {                                                                  
            case ID_LEV_TIMER:                                                 
               for(ucLoop=0;ucLoop<17;ucLoop++)                                
                  {                                                            
                  WinSendDlgItemMsg(hWnd,                                      
                                    ID_MX_LEVBMP1+ucLoop,                      
                                    SM_SETHANDLE,                              
                                    MPFROMHWND(hBmpLevel[ucIndex]),            
                                    0L);                                       
                  }                                                            
               ucIndex++;                                                      
               if(ucIndex==17) ucIndex=0;                                      
               break;                                                          
            }                                                                  
        break;                                                                 

    case WM_CLOSE:
        DosBeep(3000,500); 
/*      WinStopTimer(hAB,
                     hWnd,
                     ulLevTimerID);       */
        return (WinDefDlgProc(hWnd, Msg, mp1, mp2));
        break;
                                                                   
    default:                                                                   
        return (WinDefDlgProc(hWnd, Msg, mp1, mp2));                           
    }                                                                          
return ((MRESULT)0L);                                                          
}                                                                              
