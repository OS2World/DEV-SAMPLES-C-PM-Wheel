// Includes
// --------
   #define  INCL_WIN
   #define  INCL_DOS
   #include <os2.h>

   #include "malloc.h"                 // Changed from <..> for Eikon
   #include "memory.h"                 // Changed from <..> for Eikon

   #include "AllocMem.H"
   #include "AllocMem.RH"

   #include "TellUser.H"


// Constants
// ---------
   const PCHAR pszMemErr = "Insufficient memory to continue";


// Function Declarations (see also AllocMem.H)
// -------------------------------------------
   VOID EXPENTRY AllocMemInitialize (VOID);


// Function: AllocMemInitialize - Initialize AllocMem processing
// -------------------------------------------------------------
   VOID EXPENTRY AllocMemInitialize (VOID)

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


// Function: AllocMemAlloc - Allocate memory
// -----------------------------------------
   PVOID EXPENTRY AllocMemAlloc (cbSize)

      LONG cbSize;                     // Size of memory to allocate

   // Define function data

     {PVOID pvData;                    // Pointer to new memory

   // Verify that request is for les than 65535 bytes (temporary restriction)

      if (cbSize > 65535L)
        {TellUser (ERROR_TOO_LARGE, ALLOCMEM_MODNAME, MB_OK | MB_ICONHAND, cbSize);
         DosExit (EXIT_PROCESS, 0);
        }

   // Attempt to allocate new memory; issue an error message on failure

      while ((pvData = malloc ((size_t) cbSize)) == NULL)
        {if (WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, pszMemErr, NULL, 0,
          MB_ABORTRETRYIGNORE | MB_ICONHAND) == MBID_ABORT)
           DosExit (EXIT_PROCESS, 0);
        }

   // Initialize new memory

      memset (pvData, 0, (size_t) cbSize);

   // Return new memory pointer

      return pvData;

     }


// Function: AllocMemFree - Free memory
// ------------------------------------
   VOID EXPENTRY AllocMemFree (pvData)

      PVOID pvData;                    // -->memory to free

   // Free memory (assuming current restriction of 65535 maximum)

     {if (pvData) free (pvData);

     }


// Function: AllocMemQuerySize - Query Size of Memory
// --------------------------------------------------
   ULONG EXPENTRY AllocMemQuerySize (pvData)

      PVOID pvData;                    // -->memory to query

   // Query memory size

     {return (pvData == NULL)? 0L : (ULONG) _msize (pvData);

     }


// Function: AllocMemReAlloc - Reallocate memory
// ---------------------------------------------
   PVOID EXPENTRY AllocMemReAlloc (pvOld, cbSize)

      PVOID pvOld;                     // Pointer to old memory
      LONG cbSize;                     // Size of memory to allocate

   // Define function data

     {PVOID pvNew;                     // Pointer to new memory

   // If no "old" memory supplied, simply allocate as normal

      if (pvOld == NULL)
         return AllocMemAlloc (cbSize);

   // Verify that request is for les than 65535 bytes (temporary restriction)

      if (cbSize > 65535L)
        {TellUser (ERROR_TOO_LARGE, ALLOCMEM_MODNAME, MB_OK | MB_ICONHAND, cbSize);
         DosExit (EXIT_PROCESS, 0);
        }

   // Attempt to allocate new memory; issue an error message on failure

      while ((pvNew = realloc (pvOld, (size_t) cbSize)) == NULL)
        {if (WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, pszMemErr, NULL, 0,
          MB_ABORTRETRYIGNORE | MB_ICONHAND) == MBID_ABORT)
           DosExit (EXIT_PROCESS, 0);
        }

   // Return new memory pointer

      return pvNew;

     }
