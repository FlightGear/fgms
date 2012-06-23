dnl ######################################################################
dnl #####
dnl #####       For information about autoconf, see:
dnl #####
dnl #####       http://www.gnu.org/software/autoconf/
dnl #####
dnl #####       The recommended order is:
dnl #####
dnl #####       AC_INIT(file)
dnl #####       0. checks for compiler, libtool, and command line options
dnl #####       1. checks for programs
dnl #####       2. checks for libraries
dnl #####       3. checks for header files
dnl #####       4. checks for typedefs
dnl #####       5. checks for structures and functions
dnl #####       6. checks for compiler characteristics
dnl #####       7. checks for library functions
dnl #####       8. checks for system services
dnl #####       AC_OUTPUT([file...])
dnl #####
dnl ######################################################################

dnl ######################################################################
dnl #####
dnl #####       initialize autoconf
dnl #####
dnl ######################################################################
AC_INIT(src/server/main.cxx)

dnl ##################################################
dnl #####       the version of the program
dnl #####
dnl #####       Increment REVISION if source code
dnl #####       has changed at all
dnl #####
dnl #####       If the API has changed, increment
dnl #####       CURRENT and set REVISION to 0
dnl #####
dnl #####       If the API changes compatibly (i.e.
dnl #####       simply adding a new function without
dnl #####       changing or removing earlier
dnl #####       interfaces), then increment AGE.
dnl #####
dnl #####       If the API changes incompatibly set
dnl #####       AGE back to 0
dnl #####
dnl ##################################################
CURRENT="0"
REVISION="10"
AGE="16"
AC_SUBST(CURRENT)
AC_SUBST(REVISION)
AC_SUBST(AGE)
VERSION="$CURRENT.$REVISION.$AGE"
AC_SUBST(VERSION)
dnl ##################################################
dnl #####
dnl #####       initialize automake
dnl #####
dnl ##################################################
AM_INIT_AUTOMAKE(fgms, "$CURRENT.$REVISION.$AGE")
AM_CONFIG_HEADER(config.h)
dnl AM_MAINTAINER_MODE

dnl ###########################################################################
dnl #####
dnl #####       0. Checks for compiler, libtool, and command line options.
dnl #####
dnl ###########################################################################
AC_PREFIX_DEFAULT(/usr)
dnl SHELL="/bin/sh"
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_LIBTOOL
AC_C_BIGENDIAN
dnl AC_SUBST(ac_aux_dir)
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl ##################################################
dnl #####
dnl #####       extra argument: --with-debug
dnl #####
dnl ##################################################
WITH_DEBUG="no"
AC_ARG_WITH(debug,
[  --with-debug			Use $(CC) -g  (default=no) ],
[ case "$withval" in
	yes)
		WITH_DEBUG="yes"
		;;
	*)
		WITH_DEBUG="no"
	esac ])

dnl ##################################################
dnl #####
dnl #####       extra argument: --with-profiler
dnl #####
dnl ##################################################
WITH_PROF="no"
AC_ARG_WITH(prof,
[  --with-prof			Use $(CC) -pg  (default=no) ],
[ case "$withval" in
	yes)
		WITH_PROF="yes"
		;;
	*)
		WITH_PROF="no"
	esac ])

dnl ##################################################
dnl #####
dnl #####       extra argument: --with-optimization
dnl #####
dnl ##################################################
WITH_OPTIMIZATION="no"
AC_ARG_WITH(optimization,
[  --with-optimization=[LEVEL]     Use $(CC) -O[LEVEL]  (default=no -O) ],
[  WITH_OPTIMIZATION="$withval" ])

dnl ###########################################################################
dnl #####
dnl #####       1. checks for programs
dnl #####
dnl ###########################################################################
dnl ##################################################
dnl #####
dnl #####       find gnu make
dnl #####
dnl ##################################################
AC_CHECK_PROG(GMAKE, gmake, yes, no)
if test "x$GMAKE" = "xno"; then
        AC_PATH_PROG(MAKE, make, /usr/local/bin/make)
else
	AC_PATH_PROG(MAKE, gmake, /usr/local/gnu/bin/make)
fi
makever=`$ac_cv_path_MAKE --version 2>&1 | grep "GNU Make"`
if test -z "$makever"; then
        AC_MSG_ERROR(GNU Make is not installed.  Please download and install it
	from ftp://prep.ai.mit.edu/pub/gnu/make/ before continuing.)
fi

dnl ###########################################################################
dnl #####
dnl #####       2. checks for libraries
dnl #####
dnl ###########################################################################
dnl AC_CANONICAL_HOST
AC_SEARCH_LIBS(pthread_create, [pthread c_r])
if test "x$ac_cv_header_pthread_h" = "xyes"; then
	if test "x$ac_cv_search_pthread_exit" = "x-lc_r"; then
		CXXFLAGS="-pthread $CXXFLAGS"
		CFLAGS="-pthread $CFLAGS"
		LIBS="-lsgthreads $LIBS"
	fi
fi

dnl ##### #############################################################
dnl #####
dnl #####       3. checks for headers
dnl #####
dnl ##### #############################################################
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADER(windows.h, AC_DEFINE(WIN32,[1], [Define to 1 if you have
windows.h]))
AC_CHECK_HEADERS(	\
	stdio.h		\
	stdint.h	\
	byteswap.h	\
	ctype.h		\
	sys/types.h	\
	sys/stat.h	\
	pwd.h		\
	stdlib.h	\
	unistd.h	\
	termios.h	\
	string.h	\
	syslog.h	\
	libgen.h	\
	pthread.h	\
	)

dnl ##################################################################
dnl #####
dnl #####       6. Checks for compiler characteristics
dnl #####
dnl ##################################################################
dnl ##################################################
dnl #####
dnl #####       Ensure that these are defined
dnl #####
dnl ##################################################
AC_C_CONST
AC_MSG_CHECKING(for socklen_t)
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
int accept (int, struct sockaddr *, socklen_t *);
],[],[
AC_MSG_RESULT(yes)],[
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
int accept (int, struct sockaddr *, size_t *);
],[],[
AC_MSG_RESULT(size_t)
AC_DEFINE([socklen_t],size_t,[Define to 1 if you have socketlen_t])], [
AC_MSG_RESULT(int)
AC_DEFINE(socklen_t,int,[Define to 1 if you have socketlen_t])])])

dnl ##################################################
dnl #####
dnl #####       Set Default CXXFLAGS
dnl #####
dnl ##################################################
CXXFLAGS="-Wall \"-DSYSCONFDIR=\"\\\"$sysconfdir\\\" $CXXFLAGS"
if test "x$WITH_DEBUG" = "xyes"; then
        CXXFLAGS="$CXXFLAGS -g -DENABLE_DEBUG"
fi
if test "x$WITH_PROF" = "xyes"; then
        CXXFLAGS="$CXXFLAGS -pg"
fi
if test "x$WITH_OPTIMIZATION" != "xno"; then
	CXXFLAGS="$CXXFLAGS -O$WITH_OPTIMIZATION -fno-strict-aliasing"
fi
case "$host" in
mips-sgi-irix*)
	if test "x$CXX" = "xCC"; then
		CXXFLAGS="$CXXFLAGS -fullwarn";
	fi
	;;
*)
	if test "x$CXX" = "xicpc" || test "x$CXX" = "xicc"; then
		CXXFLAGS="$CXXFLAGS -w1"
	fi
	;;
esac


dnl ##################################################################
dnl #####
dnl #####       And finally, output the results.
dnl #####
dnl ##################################################################
CFLAGS=$CXXFLAGS
AC_SUBST(sysconfdir)
AC_SUBST(CXXFLAGS)
AC_SUBST(CFLAGS)
AC_OUTPUT(
Makefile        \
src/Makefile    \
src/simgear/Makefile	\
src/simgear/debug/Makefile	\
src/simgear/misc/Makefile	\
src/simgear/math/Makefile	\
src/flightgear/Makefile	\
src/flightgear/MultiPlayer/Makefile	\
src/plib/Makefile    	\
src/server/Makefile
)




