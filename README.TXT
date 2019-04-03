WHEEL.ARC
---------

WHEEL.ARC contains the source and executables for these components:

  CRTLIB.DLL   : Reentrant C functions DLL
  CRTLIB.LIB   : Reentrant C functions LIB
  CRTEXE.OBJ   : Reentrant C start-up code for EXE
  CRTLIB.OBJ   : Reentrant C start-up code for DLL
  TELLUSER.DLL : Internal user notification DLL
  ALLOCMEM.DLL : Internal memory management DLL
  WHEEL.DLL    : Color Wheel DLL
  TEST.EXE     : Test program

To install the executables, copy the DLLs to a directory referenced by
LIBPATH then execute TEST. To compile, edit the following files to match
your configuration:

  COMPILE.CMD  : Compile batch file
  TELLUSER     : Internal user notification DLL MAKE file
  ALLOCMEM     : Internal memory management DLL MAKE file
  WHEEL        : Color Wheel DLL DLL MAKE file
  TEST         : Test program MAKE file

Then, run COMPILE.CMD.

Note: the Color Wheel executables were compiled with the 1.2 bindings; they
will need to be re-compiled with the 1.1 bindings in order to run under 1.1.


User Interface
--------------

1) To select a displayed color, simply click a segment

2) To fine tune a selection, click and drag the mouse anywhere
   in the wheel until the desired color is displayed

3) To display the grey scales of a particular hue, double-click the
   desired pie; use (1) and (2) to select a color

4) To return to polychrome mode, double-click anywhere in the wheel


Programmer's Interface
----------------------

Refer to TEST.C and TEST.RC for a working example of how to code for the Color Wheel.

1) All programs that use a Color Wheel must #include "WHEEL.H"

2) Once per process, call ColorWheelInitialize (hab) [see Enhancements below]

3) To create a Color Wheel, use WinCreateWindow specifying the window class
   as COLOR_WHEEL_CLASS (#defined in WHEEL.H). The Color Wheel is drawn as a
   circle centered in the window rectangle with min(cx,cy) as its diameter.

4) Alternatively, a Color Wheel can be created using a CONTROL statement
   within a dialog box defined to the resource compiler

5) Similarly, a Color Wheel Sample can be created using techniques (4) and (5)
   above. Use COLOR_WHEEL_SAMPLE_CLASS (#defined in WHEEL.H) as the window class.

6) The Color Wheel owner will receive WM_CONTROL messages with the
   COLOR_WHEEL_NOTIFY_NEWCOLOR (#defined in WHEEL.H) notification code whenever
   a new color is selected; it is the responsibility of the owner to re-send this
   message to any appropriate Color Wheel Sample window(s)


Enhancements
------------

The COLOR_WHEEL_CLASS and COLOR_WHEEL_SAMPLE_CLASS window classes can be made
public by removing the comments in the ColorWheelInitialize function in WHEEL.C.
Then, use a utility such as INIEDIT (supplied as a sample program with OS/2)
to add WHEEL.DLL to the SYSDLLS entry. Processes must not call ColorWheelInitialize;
instead, OS/2 invokes the function at start up, registering the window classes.
