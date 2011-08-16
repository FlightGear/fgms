@setlocal
@set PROJ=fgms
@set NCBFILE=%PROJ%.ncb
@set INPFILE=%PROJ%.inp
@set DSWFILE=%PROJ%.dsw

@echo 2011-03-24 - build file for %PROJ% project
@if NOT EXIST %INPFILE% goto ERR1

@call chkinps %PROJ%

@if EXIST %NCBFILE% @del %NCBFILE% >nul 2>&1
@if EXIST %NCBFILE% (
@echo.
@echo ERROR: Can NOT delete %NCBFILE%
@echo This suggest MSVC is still open on this project...
@echo Close MSVC before continuing...
@echo.
@pause
)

@call killall NOPAUSE

@call amsrcs -d extra -r %INPFILE%
@if ERRORLEVEL 1 goto FAILED

@if EXIST %DSWFILE% @del %DSWFILE% >nul

@REM copy stuff ...
@call copyit NOPAUSE
@if EXIST C:\GTools\perl\temp.amsrcs04.pl.txt (
@copy C:\GTools\perl\temp.amsrcs04.pl.txt . > nul
)

@if NOT EXIST %DSWFILE% goto ERR2
@start %DSWFILE%

@call updinps %PROJ%

@goto END

:FAILED
@echo ERROR: amsrcs exited with ERRORLEVEL %ERRORLEVEL%...
@goto END

:ERR1
@echo.
@echo ERROR: Can NOT locate input file [%INPFILE%]!
@echo.
@goto END

:ERR2
@echo.
@echo ERROR: Failed to create a DSW file [%DSWFILE%]!
@echo.
@goto END

:END
@endlocal
