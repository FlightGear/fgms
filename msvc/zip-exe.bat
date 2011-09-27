@echo Zip make and executable...
@setlocal
@set PROJ=fgms
@set TEMPS=..\msvc\zip-exe.bat
@if NOT EXIST %TEMPS% goto ERR2
@set TEMPV=
@call zip-ver
@if "%TEMPV%x" == "x" goto ERR4

@set TEMPD=zips
@set TEMPZ1=%TEMPD%\%PROJ%-msvc-%TEMPV%.zip
@set TEMPZ2=%TEMPD%\%PROJ%-exe-%TEMPV%.zip
@set TEMPX=bin\%PROJ%.exe

@if NOT EXIST %TEMPX% goto ERR3

@if NOT EXIST %TEMPD%\. (
md %TEMPD%
@if NOT EXIST %TEMPD%\. (
@goto ERR1
)
)

@set TEMPO=-a
@if NOT EXIST %TEMPZ1% goto GOTOPT
@set TEMPO=-u
@echo Doing an UPDATE...
:GOTOPT
@set TEMPO=%TEMPO% -o

@echo Will do a clean, and zip to %TEMPO% %TEMPZ1% %TEMPZ2% 
@if "%NOPAUSE%x" == "yesx" goto DNPAUSE
@echo *** CONTINUE? *** Only Ctrl+C to abort. All others continue...
@pause
:DNPAUSE

@call clobnow

call zip8 %TEMPO% -o %TEMPZ1% *.*
@if NOT EXIST %TEMPZ1% goto ERR5
call zip8 %TEMPO% -P -o %TEMPZ2% %TEMPX%
@if NOT EXIST %TEMPZ2% goto ERR6
@call dirmin %TEMPZ1%
@call dirmin %TEMPZ2%
@echo Done zip-exe with above results...

@goto END

:ERR1
@echo ERROR: Unable to create %TEMPD% folder...
@goto WAIT

:ERR2
@echo ERROR: Unable to find SELF as %TEMPS%...
@goto WAIT

:ERR3
@echo ERROR: Unable to find EXE %TEMPS%...
@goto WAIT

:ERR4
@echo ERROR: TEMPV NOT set in environment...
@goto WAIT

:ERR5
@echo ERROR: Unable to create %TEMPZ1% file...
@goto WAIT

:ERR5
@echo ERROR: Unable to create %TEMPZ2% file...
@goto WAIT

:WAIT
@pause
:END
