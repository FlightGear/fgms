##################################################
#
#	main makefile for fg_server
#
##################################################

SUBDIRS = src

clean-deps:
	rm -rf .deps \
	src/.deps \
	src/*/.deps \
	src/*/*/.deps 

clean-libs:
	rm -rf .libs \
	src/.libs \
	src/*/.libs \
	src/*/*/.libs 

clean-make:
	rm -rf Makefile.in \
	src/Makefile src/Makefile.in \
	src/*/Makefile src/*/Makefile.in \
	src/*/*/Makefile src/*/*/Makefile.in

clean-autoconf:	clean-make 
	-rm -r aclocal.m4 autom4te.cache config.* configure
	-rm -r Makefile stamp-h stamp-h.in 
	-rm -r install-sh libtool ltmain.sh
	-rm -r missing mkinstalldirs *.tar.gz

clean-dist: clean clean-make clean-libs clean-deps clean-autoconf



