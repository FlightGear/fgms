@setlocal
@if NOT EXIST version goto NOFILE
@doxygen --help >nul 2>&1
@if ERRORLEVEL 2 goto NODOXY
@set TMPFIL=docs\doxy.conf
@if NOT EXIST %TMPFIL% goto NOCONF

@set VER=
@set /P VER= < version

@if "%VER%x" == "x" goto FAILED

@echo Setting version VER=%VER%
@copy %TMPFIL% temp.conf >nul
@if NOT EXIST temp.conf goto NOCOPY

@echo PROJECT_NUMBER=%VER% >>temp.conf

doxygen temp.conf
@del temp.conf >nul
@echo Looks to be a successful generation...

@goto END

:NOCOPY
@echo ERROR: Unable to copy %TMPFIL% to temp.conf!
@goto ISERR

:NOFILE
@echo ERROR: Can NOT locate 'version' file!
@goto ISERR

:NOCONF
@echo ERROR: Unable to locate file %TMPFIL%!
@echo Where is the doxygen config file?
@goto ISERR

:NODOXY
@echo ERROR: Unable to run doxygen app! Is it installed in the PATH?
@goto ISERR

:FAILED
@echo Failed to SET VER=%VER%
@goto ISERR

:ISERR
@endlocal
@exit /b 1


:END
@endlocal
@exit /b 0

@REM eof

