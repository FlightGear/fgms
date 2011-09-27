@setlocal
@set NOPAUSE=yes
call zip-exe
call zip-src
@dir zips
@cd zips
@call genzipindex .
@cd ..

