@setlocal
@set FILE1=config.h.msvc
@set FILE2=config.h

@if NOT EXIST %FILE1% goto ERR1
@if NOT EXIST %FILE2% goto SETUP

@echo WARNING: %FILE2% already exists...
@echo To copy, this file must be deleted, moved, renames first..

@goto END

:SETUP
@echo Copying %FILE1% to %FILE2%...

Copy %FILE1% %FILE2%

@goto END


:ERR1
@echo ERROR: Can NOT locate file %FILE1%! Check name, location, and FIX
@goto END

:END

