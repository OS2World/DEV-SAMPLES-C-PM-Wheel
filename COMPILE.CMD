setlocal

rem Change the first "set include" below to point to the drive and
rem directory where you keep your MSC multithread includes
rem Change the second "set include" below to point to the drive and
rem directory where you keep your OS/2 C binding includes
set include=d:\sdk\include\mt;d:\sdk\include

rem Change the "set tmp" below to point to a scratch drive and directory
set tmp=l:\

rem Note that compiler error C4058 is expected. It is a known bug in MSC 5.1.
rem The message is spurious; no run-time error condition exists.
make telluser
make allocmem
make wheel
make test

rem Remember to either (a) copy *.dll to a directory named in your LIBPATH
rem or (b) update your LIBPATH (and re-boot) to point to this directory.
rem Alternatively, you can use "LIBPATH=.;etc" where the initial ".;"
rem signifies the current directory.
endlocal
