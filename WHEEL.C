// Includes
// --------
   #define INCL_WIN
   #define INCL_GPI
   #define INCL_DOS

   #include <os2.h>

   #include "stdlib.h"                 // Changed from <..> for Eikon
   #include "math.h"                   // Changed from <..> for Eikon

   #include "Wheel.H"

   #include "AllocMem.H"


// Constants
// ---------
   #define COLOR_WHEEL_RADIUS 1024L    // ColorWheel radius in world units
   #define COLOR_WHEEL_RINGS     8     // Number of rings in ColorWheel
   #define COLOR_WHEEL_PIES     12     // Number of pies in ColorWheel

   const USHORT cbExtraData            // Extra window storage
              = (2 * sizeof (PVOID));


// Precomputed Data
// ----------------
   typedef struct _PRECOMP
     {FIXED fxTheta;                   // Pie start angle
      double sinTheta, cosTheta;       // Sine, cosine of above
     } PRECOMP;


// Intra-instance Data
// -------------------
   typedef struct _INST
     {HPS hps;                         // Presentation space handle
      BOOL fPolyChrome;                // TRUE==in polychrome mode
      BOOL fHourGlass;                 // TRUE==show hour glass pointer
      BOOL fCapture;                   // TRUE==mouse is captured
      SHORT cEatMove;                  // Count of WM_MOUSEMOVE messages to ignore
      PRECOMP prec[COLOR_WHEEL_PIES+1];// Precomputed values
      FIXED fxSweep;                   // Precomputed sweep angle (degrees)
      LONG lSweep;                     // Precomputed sweep angle (radians)
      LONG lWidth;                     // Precomputed ring width
      COLOR rgb;                       // Current color in RGB form
      double h, s, v;                  // Current color in HSV form
     } INST;

   typedef INST *PINST;


// Function Declarations
// ---------------------
   MRESULT FAR      ColorWheelClick         (PINST, HWND, SHORT, SHORT, BOOL);
   MRESULT FAR      ColorWheelCreate        (HWND);
   MRESULT FAR      ColorWheelDestroy       (PINST, HWND);
   MRESULT FAR      ColorWheelDoubleClick   (PINST, HWND);
   VOID    FAR      ColorWheelDraw          (PINST, HWND);
   VOID    EXPENTRY ColorWheelInitialize    (HAB);
   MRESULT FAR      ColorWheelMouseMove     (PINST, HWND, SHORT, SHORT);
   MRESULT FAR      ColorWheelPaint         (PINST, HWND);
   MRESULT FAR      ColorWheelRelease       (PINST, HWND);
   MRESULT FAR      ColorWheelSamplePaint   (HWND);
   MRESULT EXPENTRY ColorWheelSampleWndProc (HWND, USHORT, MPARAM, MPARAM);
   MRESULT FAR      ColorWheelSize          (PINST, HWND);
   MRESULT EXPENTRY ColorWheelWndProc       (HWND, USHORT, MPARAM, MPARAM);

   COLOR   FAR      HSVtoRGB                (double, double, double);
   VOID    FAR      RGBtoHSV                (COLOR, double *, double *, double *);


// Function: ColorWheelClick - Click on ColorWheel
// -----------------------------------------------
   MRESULT FAR ColorWheelClick (pinst, hwnd, x, y, fRound)

      PINST pinst;                     // -->instance data
      HWND hwnd;                       // Window handle
      SHORT x;                         // X coordinate of click
      SHORT y;                         // Y coordinate of click
      BOOL fRound;                     // TRUE==round color

   // Local data items:

     {POINTL ptl;                      // Cartesian co-ordinates of selected point
      double r, t;                     // Polar co-ordinates of selected point

      static double dR2D = 360.0 / (2.0 * 3.14159265359);

   // Capture the mouse

      pinst->fCapture = TRUE;
      WinSetCapture (HWND_DESKTOP, hwnd);

   // Determine selected point in presentation page units

      ptl.x = (LONG) x;
      ptl.y = (LONG) y;
      GpiConvert (pinst->hps, CVTC_DEVICE, CVTC_WORLD, 1L, &ptl);

   // Convert cartesian co-ordinates of selected point to polar co-ordinates

      r = sqrt ((double) ((ptl.x * ptl.x) + (ptl.y * ptl.y)));
      t = dR2D * atan2 ((double) ptl.y, (double) ptl.x);
      if (t < 0.0) t = 360.0 + t;

   // Exit immediately if click was outside of Color Wheel

      if (r > (double) COLOR_WHEEL_RADIUS)
         return (MRESULT) NULL;

   // If rounding is specified, round polar co-ordinates to nearest segment

      if (fRound)
        {r = (double) (((LONG) r / pinst->lWidth) * pinst->lWidth);
         t = (double) (((LONG) t / pinst->lSweep) * pinst->lSweep);
        }

   // Get selected HSV values in [0, 1] from polar co-ordinates and
   // convert to RGB values in [0, 256)

      if (pinst->fPolyChrome)
        {pinst->h = t / 360.0;
         pinst->s = r / (double) (COLOR_WHEEL_RADIUS - ((fRound)? pinst->lWidth : 0L));
         pinst->v = 1.0;
        }
      else
        {pinst->s = 1.0 - (t / (360.0 - (double) ((fRound)? pinst->lSweep : 0L)));
         pinst->v = r / (double) (COLOR_WHEEL_RADIUS - ((fRound)? pinst->lWidth : 0L));
        }
      pinst->rgb = HSVtoRGB (pinst->h, pinst->s, pinst->v);

   // Tell owner of new color.

      WinSendMsg (WinQueryWindow (hwnd, QW_OWNER, FALSE), WM_CONTROL,
        MPFROM2SHORT (WinQueryWindowUShort (hwnd, QWS_ID), COLOR_WHEEL_NOTIFY_NEWCOLOR),
        (MPARAM) pinst->rgb);

   // Indicate mouse click processing is complete

      return (MRESULT) NULL;

     }


// Function: ColorWheelCreate - Create ColorWheel
// ----------------------------------------------
   MRESULT FAR ColorWheelCreate (hwnd)

      HWND hwnd;                       // Window handle

   // Define function data

     {PINST pinst;                     // -->instance data
      SIZEL sizl;                      // Presentation space size
      HDC hdc;                         // Device context
      SHORT iPie;                      // Pie number
      SHORT cPies;                     // Pie count
      POINTL ptl;                      // Temporary point

      static double dD2R = (2.0 * 3.14159265359) / 360.0;

   // Allocate the Color Wheel instance data

      pinst = (PINST) AllocMemAlloc ((LONG) sizeof (*pinst));
      WinSetWindowULong (hwnd, QWL_USER + sizeof (PVOID), (ULONG) pinst);

   // Acquire a presentation space and initialize basic attributes

      hdc = WinOpenWindowDC (hwnd);
      sizl.cx = sizl.cy = 2L * COLOR_WHEEL_RADIUS;
      pinst->hps = GpiCreatePS (NULL, hdc, &sizl, PU_ARBITRARY | GPIF_LONG | GPIT_NORMAL | GPIA_ASSOC);

   // Initialize number of pies

      cPies =  COLOR_WHEEL_PIES;

   // Precompute the sweep angle and segment width

      pinst->fxSweep = MAKEFIXED (360 / cPies, 0);
      pinst->lSweep = 360L / (LONG) cPies;
      pinst->lWidth = COLOR_WHEEL_RADIUS / COLOR_WHEEL_RINGS;

   // Precompute angles for each pie

      for (iPie = 0; iPie < cPies; iPie++)
        {pinst->prec[iPie].fxTheta = (FIXED) ((LONG) pinst->fxSweep * (LONG) iPie);
         pinst->prec[iPie].sinTheta = sin (dD2R * (double) ((LONG) pinst->prec[iPie].fxTheta / 65536L));
         pinst->prec[iPie].cosTheta = cos (dD2R * (double) ((LONG) pinst->prec[iPie].fxTheta / 65536L));
        }

   // Add extra entry to make "next pie" computations easier

      pinst->prec[cPies].fxTheta = pinst->prec[0].fxTheta;
      pinst->prec[cPies].sinTheta = pinst->prec[0].sinTheta;
      pinst->prec[cPies].cosTheta = pinst->prec[0].cosTheta;

   // Draw initial Color Wheel in polychrome

      pinst->fPolyChrome = TRUE;
      ColorWheelDraw (pinst, hwnd);

   // Initialize selection by simulating a "click" on the origin

      ptl.x = ptl.y = 0L;
      GpiConvert (pinst->hps, CVTC_WORLD, CVTC_DEVICE, 1L, &ptl);
      ColorWheelClick (pinst, hwnd, (SHORT) ptl.x, (SHORT) ptl.y, TRUE);
      ColorWheelRelease (pinst, hwnd);

   // Indicate initialization complete

      return (MRESULT) NULL;

     }


// Function: ColorWheelDestroy - Destroy ColorWheel
// ------------------------------------------------
   MRESULT FAR ColorWheelDestroy (pinst, hwnd)

      PINST pinst;                     // -->instance data
      HWND hwnd;                       // Window handle

   // HWND is available for future use

     {hwnd = hwnd;

   // Destroy our presentation space

      GpiDestroyPS (pinst->hps);

   // Free the Color Wheel instance data

      AllocMemFree (pinst);

   // Indicate termination complete

      return (MRESULT) NULL;

     }


// Function: ColorWheelDoubleClick - Double Click on ColorWheel
// ------------------------------------------------------------
   MRESULT FAR ColorWheelDoubleClick (pinst, hwnd)

      PINST pinst;                     // -->instance data
      HWND hwnd;                       // Window handle

   // Reverse poly/monochrome Color Wheel status

     {pinst->fPolyChrome = !(pinst->fPolyChrome);

   // Rebuild and redraw Color Wheel

      ColorWheelDraw (pinst, hwnd);
      WinInvalidateRect (hwnd, NULL, FALSE);

   // Indicate mouse double click processing is complete

      return (MRESULT) NULL;

     }


// Function: ColorWheelDraw - Draw ColorWheel
// ------------------------------------------------
   VOID FAR ColorWheelDraw (pinst, hwnd)

      PINST pinst;                     // -->instance data
      HWND hwnd;                       // Window handle

   // Local function storage

     {ARCPARAMS arcp;                  // Arc parameters
      POINTL ptlStart, ptlEnd;         // Start, end point of arc
      POINTL ptlCenter;                // Center point of arc
      ULONG idSegment;                 // Segment identifier
      SHORT iPie, iRing;               // Pie, ring number
      SHORT cPies, cRings;             // Pie, ring count
      double h, s, v;                  // HSV values in [0, 1]
      COLOR rgb;                       // RGB values in [0, 256)

   // Indicate that the hour glass pointer should be displayed
   // and display it.

      pinst->fHourGlass = TRUE;
      WinSetPointer (HWND_DESKTOP, WinQuerySysPointer (HWND_DESKTOP, SPTR_WAIT, FALSE));

   // Initialize number of pies, rings and segment identifier

      cPies =  COLOR_WHEEL_PIES;
      cRings = COLOR_WHEEL_RINGS;
      idSegment = 0L;

   // Reset the presentation space to delete any existing segments

      GpiResetPS (pinst->hps, GRES_ALL);

   // Set initial attributes

      GpiSetDrawingMode (pinst->hps, DM_RETAIN);
      GpiCreateLogColorTable (pinst->hps, LCOL_RESET, LCOLF_RGB, NULL, NULL, NULL);

   // Initialize arc parameters

      arcp.lP = arcp.lQ = COLOR_WHEEL_RADIUS / cRings;
      arcp.lR = arcp.lS = 0L;
      GpiSetArcParams (pinst->hps, &arcp);

   // Initialize center point

      ptlCenter.x = ptlCenter.y = 0L;

   // Draw pies clockwise

      for (iPie = 0; iPie < cPies; iPie++)

      // Calculate start, end of innermost ring for this arc

        {ptlStart.x = (LONG) (pinst->prec[iPie].cosTheta * arcp.lP);
         ptlStart.y = (LONG) (pinst->prec[iPie].sinTheta * arcp.lQ);
         ptlEnd.x = (LONG) (pinst->prec[iPie+1].cosTheta * arcp.lP);
         ptlEnd.y = (LONG) (pinst->prec[iPie+1].sinTheta * arcp.lQ);

      // Draw rings from the outside in

         for (iRing = cRings-1; iRing >= 0; iRing--)

         // Build HSV values in [0, 1] from pie, ring number and
         // convert to RGB values in [0, 256)

           {if (pinst->fPolyChrome)
              {h = (double) iPie / (double) cPies;
               s = (double) iRing / (double) (cRings-1);
               v = 1.0;
              }
            else
              {h = pinst->h;
               s = 1.0 - ((double) iPie / (double) (cPies-1));
               v = (double) iRing / (double) (cRings-1);
              }
            rgb = HSVtoRGB (h, s, v);

         // Initialize arc segment

            GpiOpenSegment (pinst->hps, ++idSegment);
            GpiSetTag (pinst->hps, (idSegment << 24) + rgb);

         // Draw arc segment in color

            GpiSetColor (pinst->hps, rgb);
            GpiBeginArea (pinst->hps, BA_NOBOUNDARY);
            GpiMove (pinst->hps, &ptlCenter);
            GpiPartialArc (pinst->hps, &ptlCenter, MAKEFIXED ((SHORT) iRing+1, 0),
              pinst->prec[iPie].fxTheta, pinst->fxSweep);
            GpiEndArea (pinst->hps);

         // Outline the arc segment

            GpiSetColor (pinst->hps, 0x00000000L);
            GpiMove (pinst->hps, &ptlStart);
            GpiPartialArc (pinst->hps, &ptlCenter, MAKEFIXED ((SHORT) iRing+1, 0),
              pinst->prec[iPie].fxTheta, pinst->fxSweep);
            GpiLine (pinst->hps, &ptlEnd);

         // Terminate arc segment

            GpiCloseSegment (pinst->hps);

         // Loop until all rings in this pie are drawn

           }

      // Loop until all pies in the wheel are drawn

        }

   // Indicate that the regular pointer should be displayed
   // and display it.

      pinst->fHourGlass = FALSE;
      WinSetPointer (HWND_DESKTOP, WinQuerySysPointer (HWND_DESKTOP, SPTR_ARROW, FALSE));

   // Force a resize of the window

      ColorWheelSize (pinst, hwnd);

     }


// Function: ColorWheelInitialize - Initialize ColorWheel processing
// -----------------------------------------------------------------
   VOID EXPENTRY ColorWheelInitialize (hab)

      HAB hab;                         // Application anchor block

   // Define function data

     {static DOSFSRSEM dosfsrs;        // Semaphore to block registration
      static BOOL fRegistered = FALSE; // TRUE if already registered

   // Block the actions that follow such that only one process at a
   // time executes them.

      DosFSRamSemRequest (&dosfsrs, SEM_INDEFINITE_WAIT);

   // Once only, perform initialization.

      if (! fRegistered)

      // Register ColorWheel window class

        {WinRegisterClass (hab, COLOR_WHEEL_CLASS, ColorWheelWndProc, /*CS_PUBLIC |*/ CS_CLIPCHILDREN | CS_SIZEREDRAW, cbExtraData);

      // Register ColorWheelSample window class

         WinRegisterClass (hab, COLOR_WHEEL_SAMPLE_CLASS, ColorWheelSampleWndProc, /*CS_PUBLIC |*/ CS_CLIPCHILDREN | CS_SIZEREDRAW | CS_SYNCPAINT, cbExtraData);

      // Indicate initialization complete

       /*fRegistered = TRUE;*/
        }

   // Release the block and return

      DosFSRamSemClear (&dosfsrs);

     }


// Function: ColorWheelMouseMove - Mouse Move on ColorWheel
// --------------------------------------------------------
   MRESULT FAR ColorWheelMouseMove (pinst, hwnd, x, y)

      PINST pinst;                     // -->instance data
      HWND hwnd;                       // Window handle
      SHORT x;                         // X coordinate of click
      SHORT y;                         // Y coordinate of click

   // Display appropriate pointer

     {WinSetPointer (HWND_DESKTOP, WinQuerySysPointer (HWND_DESKTOP,
        (pinst->fHourGlass)? SPTR_WAIT : SPTR_ARROW, FALSE));

   // If we have captured the mouse, simulate a mouse click

      if ((pinst->fCapture) && (pinst->cEatMove-- <= 0))
         ColorWheelClick (pinst, hwnd, x, y, FALSE);

   // Indicate mouse move processing is complete

      return (MRESULT) NULL;

     }


// Function: ColorWheelPaint - Paint ColorWheel
// --------------------------------------------
   MRESULT FAR ColorWheelPaint (pinst, hwnd)

      PINST pinst;                     // -->instance data
      HWND hwnd;                       // Window handle

   // Define function data

     {HPS hps;                         // Presentation space

   // Indicate that the hour glass pointer should be displayed
   // and display it.

      pinst->fHourGlass = TRUE;
      WinSetPointer (HWND_DESKTOP, WinQuerySysPointer (HWND_DESKTOP, SPTR_WAIT, FALSE));

   // Initialize painting, draw all segments and terminate painting

      hps = WinBeginPaint (hwnd, pinst->hps, NULL);
      GpiDrawChain (pinst->hps);
      WinEndPaint (hps);

   // Indicate that the regular pointer should be displayed
   // and display it.

      pinst->fHourGlass = FALSE;
      WinSetPointer (HWND_DESKTOP, WinQuerySysPointer (HWND_DESKTOP, SPTR_ARROW, FALSE));

   // Indicate painting is complete

      return (MRESULT) NULL;

     }


// Function: ColorWheelRelease - Release ColorWheel
// ------------------------------------------------
   MRESULT FAR ColorWheelRelease (pinst, hwnd)

      PINST pinst;                     // -->instance data
      HWND hwnd;                       // Window handle

   // HWND is available for future use

     {hwnd = hwnd;

   // Release the mouse

      pinst->fCapture = FALSE;
      WinSetCapture (HWND_DESKTOP, NULL);

   // Indicate mouse release processing is complete

      return (MRESULT) NULL;

     }


// Function: ColorWheelSamplePaint - Paint ColorWheelSample
// --------------------------------------------------------
   MRESULT FAR ColorWheelSamplePaint (hwnd)

      HWND hwnd;                       // Window handle

   // Define function data

     {HPS hps;                         // Presentation space
      RECTL rcl;                       // Window rectangle

   // Initialize painting

      hps = WinBeginPaint (hwnd, NULL, NULL);
      GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, NULL, NULL, NULL);
      WinQueryWindowRect (hwnd, &rcl);

   // Draw outline rectangle

      rcl.xRight--; rcl.yTop--;
      GpiMove (hps, (PPOINTL) &rcl.xLeft);
      GpiSetColor (hps, 0L);
      GpiBox (hps, DRO_OUTLINE, (PPOINTL) &rcl.xRight, NULL, NULL);

   // Fill rectangle with current color

      rcl.xLeft++; rcl.yBottom++; rcl.xRight--; rcl.yTop--;
      GpiMove (hps, (PPOINTL) &rcl.xLeft);
      GpiSetColor (hps, WinQueryWindowULong (hwnd, QWL_USER + sizeof (PVOID)));
      GpiBox (hps, DRO_FILL, (PPOINTL) &rcl.xRight, NULL, NULL);

   // Terminate painting

      WinEndPaint (hps);

   // Indicate painting is complete

      return (MRESULT) NULL;

     }


// Function: ColorWheelSampleWndProc - ColorWheelSample Window Procedure
// ---------------------------------------------------------------------
   MRESULT EXPENTRY ColorWheelSampleWndProc (hwnd, msg, mp1, mp2)

      HWND hwnd;                       // Window handle
      USHORT msg;                      // PM message
      MPARAM mp1;                      // Message parameter
      MPARAM mp2;                      // Message parameter

   // Analyze type of message from PM

     {switch (msg)

       {case WM_CONTROL:
          if (SHORT2FROMMP (mp1) == COLOR_WHEEL_NOTIFY_NEWCOLOR)
            {WinSetWindowULong (hwnd, QWL_USER + sizeof (PVOID), (ULONG) mp2);
             WinInvalidateRect (hwnd, NULL, FALSE);
            }
          return (MRESULT) NULL;

        case WM_CREATE:
          WinSetWindowULong (hwnd, QWL_USER + sizeof (PVOID), 0x00FFFFFFL);
          return (MRESULT) NULL;

        case WM_PAINT:
          return ColorWheelSamplePaint (hwnd);

       }

  // Return to PM.

     return WinDefWindowProc (hwnd, msg, mp1, mp2);

    }


// Function: ColorWheelSize - Size ColorWheel
// ------------------------------------------
   MRESULT FAR ColorWheelSize (pinst, hwnd)

      PINST pinst;                     // -->instance data
      HWND hwnd;                       // Window handle

   // Define function data

     {SWP swp;                         // Current screen position
      MATRIXLF matlf;                  // Transformation matrix
      POINTL ptl;                      // Temporary point

   // Save the new display location.

      WinQueryWindowPos (hwnd, &swp);

   // Convert screen width units

      ptl.x = (LONG) ((swp.cx-1) / 2);
      ptl.y = (LONG) ((swp.cy-1) / 2);
      GpiConvert (pinst->hps, CVTC_DEVICE, CVTC_PAGE, 1L, &ptl);

   // Determine the transformation matrix

      /* A */ matlf.fxM11 = (FIXED) (65536.0 * ((double) min (ptl.x, ptl.y) / COLOR_WHEEL_RADIUS));
      /* B */ matlf.fxM12 = (FIXED) (0);
      /* C */ matlf.fxM21 = (FIXED) (0);
      /* D */ matlf.fxM22 = (FIXED) (65536.0 * ((double) min (ptl.x, ptl.y) / COLOR_WHEEL_RADIUS));
      /* E */ matlf.lM31 = ptl.x;
      /* F */ matlf.lM32 = ptl.y;
              matlf.lM13 = 0L;
              matlf.lM23 = 0L;
              matlf.lM33 = 1L;
      GpiSetDefaultViewMatrix (pinst->hps, 9L, &(matlf), TRANSFORM_REPLACE);

   // Indicate new size acknowledged

      return (MRESULT) NULL;

     }


// Function: ColorWheelWndProc - ColorWheel Window Procedure
// ---------------------------------------------------------
   MRESULT EXPENTRY ColorWheelWndProc (hwnd, msg, mp1, mp2)

      HWND hwnd;                       // Window handle
      USHORT msg;                      // PM message
      MPARAM mp1;                      // Message parameter
      MPARAM mp2;                      // Message parameter

   // Define function data

     {PINST pinst;                     // -->instance data

   // The location of the Color Wheel instance data
   // is retrieved from the Color Wheel window.

      pinst = (PINST) WinQueryWindowULong (hwnd, QWL_USER + sizeof (PVOID));

   // Analyze type of message from PM

      switch (msg)

       {case WM_BUTTON1DOWN:
          pinst->cEatMove = 2;
          ColorWheelClick (pinst, hwnd, SHORT1FROMMP (mp1), SHORT2FROMMP (mp1), TRUE);
          return WinDefWindowProc (hwnd, msg, mp1, mp2);

        case WM_BUTTON1DBLCLK:
          return ColorWheelDoubleClick (pinst, hwnd);

        case WM_BUTTON1UP:
          return ColorWheelRelease (pinst, hwnd);

        case WM_CREATE:
          return ColorWheelCreate (hwnd);

        case WM_DESTROY:
          return ColorWheelDestroy (pinst, hwnd);

        case WM_PAINT:
          return ColorWheelPaint (pinst, hwnd);

        case WM_MOUSEMOVE:
          return ColorWheelMouseMove (pinst, hwnd, SHORT1FROMMP (mp1), SHORT2FROMMP (mp1));

        case WM_SIZE:
          return ColorWheelSize (pinst, hwnd);

       }

  // Return to PM.

     return WinDefWindowProc (hwnd, msg, mp1, mp2);

    }


// Function: HSVtoRGB
// ------------------
   COLOR FAR HSVtoRGB (h, s, v)

      double h;                        // H value
      double s;                        // S value
      double v;                        // V value

   // Define function data

     {double r, g, b;                  // Corresponding RGB value
      LONG R, G, B;                    // Integer RGB values
      double p, q, t, f, i;            // Temporary values

   // Convert HSV to RGB

      if (s == 0.0)
         r = g = b = v;
      else
        {if (h == 1.0) h = 0.0;
         h = h * 6.0;
         i = floor (h);
         f = h - i;
         p = v * (1 - s);
         q = v * (1 - (s * f));
         t = v * (1 - (s * (1 - f)));
         if (i == 0.0)
           {r = v; g = t; b = p;}
         else if (i == 1.0)
           {r = q; g = v; b = p;}
         else if (i == 2.0)
           {r = p; g = v; b = t;}
         else if (i == 3.0)
           {r = p; g = q; b = v;}
         else if (i == 4.0)
           {r = t; g = p; b = v;}
         else if (i == 5.0)
           {r = v; g = p; b = q;}
        }

   // Convert RGB values now [0, 1] to [0, 255]

      R = (LONG) floor (r * 255.9);
      G = (LONG) floor (g * 255.9);
      B = (LONG) floor (b * 255.9);

   // Return color to caller

      return (COLOR) ((R << 16) + (G << 8) + B);

     }


// Function: RGBtoHSV
// ------------------
   VOID FAR RGBtoHSV (rgb, ph, ps, pv)

      COLOR rgb;                       // RGB value
      double *ph;                      // H value
      double *ps;                      // S value
      double *pv;                      // V value

   // Define function data

     {double r, g, b;                  // RGB values in [0, 1]
      double rc, gc, bc;               // "Distance" of color from primary
      double minRGB, maxRGB;           // Min, max RGB values

   // Extract RGB values in [0, 1]

      r = (double) ((rgb & 0x00FF0000L) >> 16) / 255.0;
      g = (double) ((rgb & 0x0000FF00L) >>  8) / 255.0;
      b = (double) (rgb & 0x000000FFL) / 255.0;

   // Get min, max RGB values

      minRGB = min (r, min (g, b));
      maxRGB = max (r, max (g, b));

   // Convert RGB to HSV

      *pv = maxRGB;
      if (maxRGB > 0.0)
         *ps = (maxRGB - minRGB) / maxRGB;
      else *ps = *ph = 0.0;
      if (*ps > 0.0)
        {rc = (maxRGB - r) / (maxRGB - minRGB);
         gc = (maxRGB - g) / (maxRGB - minRGB);
         bc = (maxRGB - b) / (maxRGB - minRGB);
         if (r == maxRGB)
            *ph = bc - gc;
         else if (g == maxRGB)
            *ph = 2 + rc - bc;
         else if (b == maxRGB)
            *ph = 4 + gc - rc;
         *ph = *ph / 6.0;
            if (*ph < 0.0) *ph += 1.0;
        }
     }
