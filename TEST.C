#define INCL_WIN
#define INCL_GPI
#include <os2.h>

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "TEST.H"
#include "WHEEL.H"

VOID EXPENTRY ColorWheelInitialize    (HAB);
MRESULT EXPENTRY TestDlgProc (HWND hwnd, USHORT usMsg, MPARAM mp1, MPARAM mp2);

VOID main( VOID )
{
        HAB hab;
        HMQ hmq;
        HWND hwnd;
        HPOINTER hptr;

        hab = WinInitialize (0);
        hmq = WinCreateMsgQueue (hab, 99);
        ColorWheelInitialize (hab);
        hwnd = WinLoadDlg (HWND_DESKTOP, HWND_DESKTOP, TestDlgProc, NULL, DLG_PRESPARAM, NULL);
        hptr = WinLoadPointer (HWND_DESKTOP, NULL, DLG_PRESPARAM);
        WinSendMsg (hwnd, WM_SETICON, (MPARAM)hptr, NULL);
        WinProcessDlg (hwnd);
        WinDestroyWindow (hwnd);
        WinDestroyPointer (hptr);
        WinDestroyMsgQueue (hmq);
        WinTerminate (hab);
}

MRESULT EXPENTRY TestDlgProc (HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
        CHAR szText[12];
        COLOR clr;

        switch ( msg )

               {case WM_INITDLG:
                        return (MRESULT) NULL;

                case WM_CONTROL:
                        switch (SHORT1FROMMP (mp1))
                               {case DLGITEM_WHEEL:
                                        if (SHORT2FROMMP (mp1) == COLOR_WHEEL_NOTIFY_NEWCOLOR)
                                          {clr = (COLOR) mp2;
                                           WinSendDlgItemMsg (hwnd, DLGITEM_SAMPLE, msg, mp1, mp2);
                                           ltoa (((0x00FF0000L & clr) >> 16), szText, 10);
                                           WinSetDlgItemText (hwnd, DLGITEM_RED, szText);
                                           ltoa (((0x0000FF00L & clr) >> 8), szText, 10);
                                           WinSetDlgItemText (hwnd, DLGITEM_GREEN, szText);
                                           ltoa ((0x000000FFL & clr), szText, 10);
                                           WinSetDlgItemText (hwnd, DLGITEM_BLUE, szText);
                                          }
                                        break;
                               }
                        return (MRESULT) NULL;

                case WM_COMMAND:
                        switch (SHORT1FROMMP (mp1))
                               {case DLGITEM_ABOUT:
                                        WinDlgBox( HWND_DESKTOP, hwnd, WinDefDlgProc, NULL, DLG_ABOUT, NULL );
                                        break;
                                case DLGITEM_QUIT:
                                        WinDismissDlg( hwnd, TRUE );
                                        break;
                               }
                        return (MRESULT) NULL;
               }

        return WinDefDlgProc( hwnd, msg, mp1, mp2 );

}
