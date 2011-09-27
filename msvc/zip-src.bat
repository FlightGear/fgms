@echo Zip source, excluding make and executable...
@setlocal
@set PROJ=fgms
@set TEMPS=..\msvc\zip-src.bat
@if NOT EXIST %TEMPS% goto ERR2
@set TEMPV=
@call zip-ver
@if "%TEMPV%x" == "x" goto ERR4


@set TEMPD=zips
@set TEMPZ0=%TEMPD%\%PROJ%-srcs-%TEMPV%.zip
@set TEMPZ=msvc\%TEMPZ0%
@set TEMPX=bin\%PROJ%.exe

@if NOT EXIST %TEMPX% goto ERR3

@if NOT EXIST %TEMPD%\. (
md %TEMPD%
@if NOT EXIST %TEMPD%\. (
@goto ERR1
)
)

@cd ..
@set TEMPO=-a
@if NOT EXIST %TEMPZ% goto GOTOPT
@set TEMPO=-u
@echo Doing an UPDATE...
:GOTOPT
@set TEMPO=%TEMPO% -o

@echo Will do zip to %TEMPZ% %TEMPO%
@if "%NOPAUSE%x" == "yesx" goto DNPAUSE
@echo *** CONTINUE? *** Only Ctrl+C to abort. All others continue...
@cd msvc
@pause
@cd ..
:DNPAUSE

call zip8 %TEMPO% -p -r -xmsvc\*.* -xtemp\*.* %TEMPZ% *.*

@cd msvc
@if NOT EXIST %TEMPZ0% goto ERR5
@call dirmin %TEMPZ0%
@echo Zipping is done...
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
@echo ERROR: Unable to create %TEMPZ0% file...
@goto WAIT

:WAIT
@pause

:END
