// Constants
// ---------
   #define COLOR_WHEEL_CLASS           "CanyonColorWheel"
   #define COLOR_WHEEL_SAMPLE_CLASS    "CanyonColorWheelSample"
   #define COLOR_WHEEL_MODNAME         "WHEEL.DLL"


// WM_CONTROL Notification codes
// -----------------------------
   #define COLOR_WHEEL_NOTIFY_NEWCOLOR 1
                                       // mp1 : MPFROM2SHORT (id, COLORWHEEL_NOTIFY_NEWCOLOR)
                                       // mp2 : ULONG rgbColor
