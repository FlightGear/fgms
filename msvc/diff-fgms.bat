@echo Diff fgms...
@setlocal
@set DIR1=C:\fgcvs\fgms-0-x
@set DIR2=C:\Projects\fgms2\fgms-0-x
@set FILE=C:\Projects\fgms2\tempdiff.txt

@if NOT EXIST %DIR1%\. goto ERR1
@if NOT EXIST %DIR2%\. goto ERR2

call diff -ur %DIR1% %DIR2% >%FILE%

call np %FILE%

@goto END

:ERR1
@echo ERROR: Can NOT locate folder [%DIR1%]! Check name, location, and FIX
@goto END

:ERR2
@echo ERROR: Can NOT locate folder [%DIR2%]! Check name, location, and FIX
@goto END

:END
