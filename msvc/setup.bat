@setlocal
@set FILE1=config.h.msvc
@set FILE2=config.h
@set DIR3RD=C:\Projects\3rdparty

@if NOT EXIST %FILE1% goto ERR1
@if NOT EXIST %FILE2% goto SETUP

@echo WARNING: %FILE2% already exists...
@echo To copy, this file must be deleted, moved, renames first..

@goto DONE

:SETUP
@echo Copying %FILE1% to %FILE2%...

Copy %FILE1% %FILE2%

@goto DONE


:ERR1
@echo ERROR: Can NOT locate file %FILE1%! Check name, location, and FIX
@goto END

:DONE
@if NOT EXIST %DIR3RD%\. (
@echo.
@echo WARNING: Can NOT locate %DIR3RD%!
@echo This is a 3rdparty folder containing include/headers like pthread.h
@echo and lib/libs like pthreadVC2.lib...
@echo IT WILL BE NECESSARY TO EITHER ESTABLISH THIS DIRECTORY
@echo OR AMENDS THE MSVC DSP BUILD FILES ACCORDINGLY
@echo.
) else (
@echo Found 3rdparty folder %DIR3RD%... good...
@if NOT EXIST %DIR3RD%\include\pthread.h (
@echo BUT CAN NOT LOCATE the file [%DIR3RD%\include\pthread.h]
@echo IT WILL BE NECESSARY TO EITHER ESTABLISH THIS DIRECTORY/FILE
@echo OR AMENDS THE MSVC DSP BUILD FILES ACCORDINGLY
)
@if NOT EXIST %DIR3RD%\lib\pthreadVC2.lib (
@echo BUT CAN NOT LOCATE the file [%DIR3RD%\lib\pthreadVC2.lib]
@echo IT WILL BE NECESSARY TO EITHER ESTABLISH THIS DIRECTORY/FILE
@echo OR AMENDS THE MSVC DSP BUILD FILES ACCORDINGLY
)
)
@goto END

:END

