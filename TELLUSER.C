// Includes
// --------
   #define  INCL_WIN
   #define  INCL_DOS
   #include <os2.h>

   #include "stdio.h"                  // Changed from <..> for Eikon
   #include "stdarg.h"                 // Changed from <..> for Eikon
   #include "string.h"                 // Changed from <..> for Eikon

   #include "TellUser.H"


// Constants
// ---------
   #define MAX_STRING 128              // Maximum string size


// Function Declarations (see also TellUser.H)
// -------------------------------------------
   VOID EXPENTRY TellUserInitialize (VOID);


// Function: TellUserInitialize - Initialize TellUser processing
// -------------------------------------------------------------
   VOID EXPENTRY TellUserInitialize (VOID)

   // Define function data

     {static DOSFSRSEM dosfsrs;         // Semaphore to block registration
      static BOOL fRegistered = FALSE;  // TRUE if already registered

   // Block the actions that follow such that only one process at a
   // time executes them.

      DosFSRamSemRequest (&dosfsrs, SEM_INDEFINITE_WAIT);

   // Perform dummy initialization

      if (! fRegistered)
        {
         fRegistered = TRUE;
        }

   // Release the block and return

      DosFSRamSemClear (&dosfsrs);
     }


// Function: TellUser - Issue message to User
// ------------------------------------------
   USHORT FAR TellUser (idMsg, pszDLLName, flStyle, ...)

      USHORT idMsg;                    // Message identifier
      PCHAR pszDLLName;                // Module name where resource exists
      USHORT flStyle;                  // Message style

   // Define function data

     {CHAR szFormat[MAX_STRING];       // Message format
      CHAR szBuffer[MAX_STRING];       // Message buffer
      CHAR szCaption[MAX_STRING];      // Message caption
      USHORT usResponse;               // Message response
      HMODULE hmod = NULL;             // Requesting module handle
      va_list pArgs;                   // String substitution arguments
      PCHAR pszDot;                    // Temporary pointer
      int i;                           // Temporary integer

   // Extract the message caption from the DLL name.

      strcpy (szCaption, pszDLLName);
      if ((pszDot = strrchr (szCaption, '.')) != NULL)
        *pszDot = 0;

   // Get the handle of the module holding the message string

      i = strlen (pszDLLName);
      if ((i > 4) && (strcmpi (pszDLLName+i-4, ".DLL") == 0))
        {if (DosLoadModule (szBuffer, sizeof (szBuffer), szCaption, &hmod))
            hmod = NULL;
        }

   // Load the message string

      WinLoadString (NULL, hmod, idMsg, sizeof (szFormat), szFormat);

   // Free the handle of the module holding the message string

      if (hmod != NULL)
         DosFreeModule (hmod);

   // Build the message text.

      va_start (pArgs, flStyle);
      vsprintf (szBuffer, szFormat, pArgs);
      va_end (pArgs);

   // Display the message and capture response

      usResponse = WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, szBuffer,
         szCaption, idMsg, flStyle);

   // Terminate if response is "abort"

      if (usResponse == MBID_ABORT)
         DosExit (EXIT_PROCESS, 0);

   // Return response

      return usResponse;
     }
