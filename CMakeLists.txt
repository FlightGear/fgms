#
# single CMakeLists.txt for fgms-0-x - hand crafted - commenced 2012/07/03
#
# 20170627 - reformmated this file
#          - reordered entries
#          - added status sumary at the end
# 20161121 - Remove ability to compile FGTracker which is obsolete
# 20130411 - Change to use external version file
# 20121130 - Add link_directories ( ${PostgreSQL_LIBRARY_DIR} )
#            at least for WIN32 - AGE 26
# 20121025 - Add a windows 'perror' macro PERROR, and minor changes in
#            crossfeed code - age 25
# 20121018 - Try to get the unix install to /usr/sbin right, and bump age to 24
# 20120815 - In Unix, install to /usr/sbin if CMAKE_INSTALL_PREFIX not given.
#            Default is /usr/local/sbin
#            Bump AGE to 20, since a number of other commits done.
# 20120808 - Change the IPCKEY value if building mp_server2 - bumped AGE to 19
# 20120723 - Add ENABLE_DEBUG_OUTPUT, which defines DEF_DEBUG_OUTPUT in
#            fg_tracker.cxx
# and add USE_PTHREAD_IN_TRACKER server, replacing fork()

cmake_minimum_required ( VERSION 2.8.8 )

project ( fgms )

# change names if build server and tracker #2
set ( EXE_NAME fgms )

##################################################
#
# setup directories
#
##################################################
if ( NOT DEFINED CMAKE_INSTALL_PREFIX )
	set ( CMAKE_INSTALL_PREFIX "/usr" )
endif ( NOT DEFINED CMAKE_INSTALL_PREFIX )
if ( NOT DEFINED SBINDIR )
	if ( MSVC )
		set( SBINDIR bin ) 
	else ()
		set( SBINDIR "${CMAKE_INSTALL_PREFIX}/sbin" )
	endif()
endif()
if ( NOT DEFINED SYSCONFDIR )
	set ( SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc" )
endif ( NOT DEFINED SYSCONFDIR )
add_definitions( -DSYSCONFDIR="${SYSCONFDIR}" )

##################################################
#
# setup optional build settings
#
##################################################
option ( BUILD_SHARED_LIB "Build Shared Library" OFF )
option ( BUILD_SERVER2 "Build a server with different defaults" OFF )
option ( BUILD_FGLS "Build 'fgls' server" ON ) # FIXME: set to OFF
option ( BUILD_TESTS "Build test applications" ON ) # FIXME: set to OFF

set ( LIB_TYPE STATIC )  # set default static
if ( BUILD_SHARED_LIB )
	set ( LIB_TYPE SHARED )
endif ( BUILD_SHARED_LIB )

# set build type so it's not empty
if ( NOT DEFINED CMAKE_BUILD_TYPE )
	set ( CMAKE_BUILD_TYPE Release )
endif()

##################################################
#
# setup compile flags
#
##################################################
if ( UNIX )
	option ( ENABLE_DEBUG_OUTPUT "Add additional debug output." OFF )
	option ( ENABLE_DEBUG_TRACE "Add trace info." OFF )
endif ( UNIX )

if ( CMAKE_COMPILER_IS_GNUCXX )
	set ( WARNING_FLAGS "-Wall --pedantic" )
endif ( CMAKE_COMPILER_IS_GNUCXX )

if ( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
	set ( WARNING_FLAGS "-Wall -Wno-overloaded-virtual" )
endif() 

if ( WIN32 )
	if ( MSVC )
		# turn off various warnings
		set ( WARNING_FLAGS "${WARNING_FLAGS} /wd4996" )
		# C4146: unary minus operator applied to unsigned type,
		#        result still unsigned
		# C4267: '=' : conversion from 'size_t' to 'int',
		#        possible loss of data
		# foreach(warning 4244 4251 4267 4275 4290 4786 4305)
		foreach ( warning 4146 4267 )
			set ( WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}" )
		endforeach ()

		set( EXTRA_FLAGS "-DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS" )
		# if (${MSVC_VERSION} EQUAL 1600)
		#    list(APPEND EXTRA_LD_FLAGS "/FORCE:MULTIPLE" )
		# endif (${MSVC_VERSION} EQUAL 1600)
        set ( CMAKE_DEBUG_POSTFIX "d" )
	endif ( MSVC )
	set ( NOMINMAX 1 )
	add_definitions (
	  -DUL_MSVC -DOPENALSDK -DHAVE_CONFIG_H -DFREEGLUT_LIB_PRAGMAS
	  -DUL_WIN32 -D_CRT_SECURE_NO_DEPRECATE -DFREEGLUT_STATIC
	)
	configure_file (
	  "${CMAKE_SOURCE_DIR}/config.h.msvc"
	  "${CMAKE_BINARY_DIR}/config.h" COPYONLY
	)
	file ( WRITE
	  "${CMAKE_BINARY_DIR}/unistd.h"
	  "/* dummy unistd.h for windows */"
	) 
else ( WIN32 )
	set ( EXTRA_FLAGS "-std=c++11" )
	list ( APPEND EXTRA_LD_FLAGS -L. )
	set ( WARNING_FLAGS "${WARNING_FLAGS} -Wno-unused-local-typedefs -Wno-unused-but-set-variable" )
endif ( WIN32 )

# do we use CMAKE_C_FLAGS ???
set ( CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} ${WARNING_FLAGS} ${EXTRA_FLAGS} -D_REENTRANT"
)
set ( CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${EXTRA_FLAGS} -D_REENTRANT"
)
set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EXTRA_LD_FLAGS}" )

if ( ENABLE_DEBUG_OUTPUT )
	add_definitions( -DDEF_ENABLE_DEBUG )
endif ( ENABLE_DEBUG_OUTPUT )

if ( ENABLE_DEBUG_TRACE )
	add_definitions( -DDEF_ENABLE_TRACE )
endif ( ENABLE_DEBUG_TRACE )

##################################################
#
# find the threads library
#
##################################################
find_package ( Threads REQUIRED )
if ( Threads_FOUND )
	if ( WIN32 )
		list ( APPEND add_LIBS winmm.lib ws2_32.lib )
		###message( STATUS "*** ENV INCLUDE $ENV{INCLUDE}")
		message ( STATUS
		  "*** Finding pthread.h HINTS [${CMAKE_PREFIX_PATH}] PATHS [${CMAKE_INCLUDE_PATH}] PATH_SUFFIXES include"
		)
		###find_file( thread_INC pthread.h
		find_path ( thread_INC pthread.h
			HINTS ${CMAKE_PREFIX_PATH} $ENV{INCLUDE}
			PATHS ${CMAKE_INCLUDE_PATH} $ENV{INCLUDE}
			PATH_SUFFIXES include
		)
		if ( thread_INC )
			include_directories ( SYSTEM ${thread_INC} )
			message (STATUS "*** thread_INC: ${thread_INC}" )
		else ( thread_INC )
			message ( FATAL_ERROR
			  "*** thread_INC: ${thread_INC} pthread.h NOT FOUND!"
			)
		endif ( thread_INC )
		find_library ( thread_LIB pthreadVC2.lib
			HINTS ${CMAKE_PREFIX_PATH} $ENV{LIB}
			PATHS ${CMAKE_INCLUDE_PATH} $ENV{LIB}
			PATH_SUFFIXES lib
		)
		if ( thread_LIB )
			list ( APPEND add_LIBS ${thread_LIB} )
			message ( STATUS "*** Found ${thread_LIB})" )
		else ( thread_LIB )
			message ( FATAL_ERROR
			  "*** NOT FOUND target_link_libraries(pthread1 ${thread_LIB})"
			)
		endif ( thread_LIB )
	else ( WIN32 )
		list ( APPEND add_LIBS ${CMAKE_THREAD_LIBS_INIT} )
	endif ( WIN32 )
else ( Threads_FOUND )
	message ( FATAL_ERROR "*** THREADS NOT FOUND!" )
endif ( Threads_FOUND )

##################################################
#
# build all modules
#
##################################################
include_directories ( SYSTEM "${PROJECT_BINARY_DIR}"
	src
	src/fgms
	src/fglib
	src/libcli
)

#### add_subdirectory( src/libcli )
# Project [libcli] [Static Library] [noinst_LIBRARIES]
set ( libcli_SRCS
	src/libcli/common.hxx
	src/libcli/command.hxx
	src/libcli/cli_client.hxx
	src/libcli/cli_client.cxx
	src/libcli/libcli.hxx
	src/libcli/libcli.cxx
	src/libcli/filter.hxx
	src/libcli/filter.cxx
)
if ( WIN32 )
	list ( APPEND libcli_SRCS src/libcli/crypt-win.c )
else ( WIN32 )
	list ( APPEND add_LIBS crypt )
endif ( WIN32 )
add_library ( LibCLI ${LIB_TYPE} ${libcli_SRCS} )
list ( APPEND add_LIBS LibCLI )

#### add_subdirectory( src/libfgmp )
set ( Libfgmp_SRCS
	src/fglib/mpmessages.hxx
	src/fglib/debug.hxx
	src/fglib/debug.cxx
	src/fglib/fg_util.cxx 
	src/fglib/fg_util.hxx 
	src/fglib/encoding.hxx
	src/fglib/encoding.cxx
	src/fglib/netaddr.hxx
	src/fglib/netaddr.cxx
	src/fglib/ipaddr.hxx
	src/fglib/ipaddr.cxx
	src/fglib/netpacket.hxx
	src/fglib/netpacket.cxx
	src/fglib/netsocket.hxx
	src/fglib/netsocket.cxx
	src/fglib/fg_config.hxx 
	src/fglib/fg_config.cxx 
	src/fglib/fg_geometry.hxx
	src/fglib/fg_geometry.cxx
	src/fglib/fg_version.hxx
	src/fglib/fg_version.cxx
	src/fglib/fg_thread.hxx 
	src/fglib/fg_thread.cxx 
	src/fglib/fg_list.hxx 
	src/fglib/fg_list.cxx 
	src/fglib/fg_log.hxx 
	src/fglib/fg_log.cxx 
    )
if (NOT MSVC)
    list(APPEND Libfgmp_SRCS
		src/fglib/daemon.hxx 
		src/fglib/daemon.cxx 
	)
endif (NOT MSVC)        
add_library ( Libfgmp ${LIB_TYPE} ${Libfgmp_SRCS} )
# NO INSTALL [noinst_LIBRARIES]
list ( APPEND add_LIBS Libfgmp )

#### add_subdirectory( src/libmsc ) Windows Only
# Project [libmsc] [Static Library] [noinst_LIBRARIES]
if ( MSVC )
	add_library (
		libmsc ${LIB_TYPE}
		src/libmsc/msc_unistd.cxx
		src/libmsc/msc_unistd.hxx
	)
	list ( APPEND add_LIBS libmsc )
endif ( MSVC )

# Project [sgutils] [Static Library] [noinst_LIBRARIES]
add_library (
	sgutils ${LIB_TYPE}
	src/simgear/math/SGEuler.hxx 
	src/simgear/math/SGEuler.cxx 
)
# NO INSTALL [noinst_LIBRARIES]
list ( APPEND add_LIBS sgutils )

##################################################
#
# build the list server
#
##################################################
if ( BUILD_FGLS )
	add_executable ( fgls
		src/fgls/fgls.hxx
		src/fgls/fgls.cxx
		src/fgls/fgls_cli.hxx
		src/fgls/fgls_cli.cxx
	)
	target_link_libraries ( fgls ${add_LIBS} )
endif ( BUILD_FGLS )

##################################################
#
# build test apps
#
##################################################
if ( BUILD_TESTS )
	# test_log
	add_executable ( test_log src/tests/test_log.cxx )
	target_link_libraries ( test_log  ${add_LIBS} )
	# test_ipaddr
	add_executable ( test_ipaddr src/tests/test_ipaddr.cxx )
	target_link_libraries ( test_ipaddr  ${add_LIBS} )
	# test_netaddr
	add_executable ( test_netaddr src/tests/test_netaddr.cxx )
	target_link_libraries ( test_netaddr  ${add_LIBS} )
endif ( BUILD_TESTS )

##################################################
#
# build the fgms
#
##################################################
if ( BUILD_SERVER2 )
	set ( EXE_NAME mp_server2 )
	add_definitions (
	  -DDEF_SERVER_LOG="fgms2.log" -DDEF_EXIT_FILE="fgms_exit2"
	  -DDEF_RESET_FILE="fgms_reset2" -DDEF_STAT_FILE="fgms_stat2"
	  -DDEF_CONF_FILE="fgms2.conf" -DIPCKEY=0xf9f6 )
endif ( BUILD_SERVER2 )
# Project [fgms] [Console Application] [sbin_PROGRAMS]
add_executable ( ${EXE_NAME}
	src/fgms/fgms.hxx 
	src/fgms/fgms.cxx 
	src/fgms/fg_tracker.hxx 
	src/fgms/fg_tracker.cxx 
	src/fgms/fg_player.hxx 
	src/fgms/fg_player.cxx 
	src/fgms/fg_cli.hxx
	src/fgms/fg_cli.cxx 
	src/fgms/main.cxx 
)
target_link_libraries ( ${EXE_NAME} ${add_LIBS} )
if ( MSVC )
	set_target_properties ( ${EXE_NAME} PROPERTIES DEBUG_POSTFIX "d" )
endif ( MSVC )
install ( TARGETS ${EXE_NAME} DESTINATION ${SBINDIR} )

##################################################
#
# print summary of build settings
#
##################################################
message ( STATUS )
message ( STATUS "SUMMARY" )
message ( STATUS )
message ( STATUS "    Will install binary to ${SBINDIR}" )
message ( STATUS "    Will read config from ${SYSCONFDIR}" )
message ( STATUS "    Build type is: ${CMAKE_BUILD_TYPE}" )
if ( CMAKE_BUILD_TYPE STREQUAL Debug )
	message ( STATUS "    use -DCMAKE_BUILD_TYPE=Release to change" )
else()
	message ( STATUS "    use -DCMAKE_BUILD_TYPE=Debug to change" )
endif()
if ( BUILD_SHARED_LIB )
	message( STATUS "    Building DLL library ${LIB_TYPE}" )
else (BUILD_SHARED_LIB )
	message(STATUS "    Building static library ${LIB_TYPE}")
endif ( BUILD_SHARED_LIB )
message ( STATUS "    set CXX FLAGS [${CMAKE_CXX_FLAGS}]" )
message ( STATUS "    set C FLAGS [${CMAKE_C_FLAGS}]" )
message ( STATUS "    target_link_libraries( ${EXE_NAME} ${add_LIBS} )" )


if ( ENABLE_DEBUG_OUTPUT )
	message ( STATUS "    building with additinal debug output" )
else ( ENABLE_DEBUG_OUTPUT )
	message ( STATUS
	  "    building without additional debug info.. Use -DENABLE_DEBUG_OUTPUT:BOOL=TRUE to enable it."
	)
endif ( ENABLE_DEBUG_OUTPUT )

if ( ENABLE_DEBUG_TRACE )
	message ( STATUS "    building with trace info" )
else ( ENABLE_DEBUG_TRACE )
	message ( STATUS
	  "    building without trace info.. Use -DENABLE_DEBUG_TRACE:BOOL=TRUE to enable it."
	)
endif ( ENABLE_DEBUG_TRACE )

if ( BUILD_TESTS )
	message ( STATUS "    building test applications" )
else ( BUILD_TESTS )
	message ( STATUS
	  "    test apps NOT being built. Use -DBUILD_TESTS:BOOL=TRUE to enable it."
	)
endif ( BUILD_TESTS )

if ( BUILD_FGLS )
	message ( STATUS "    building fgls" )
else ( BUILD_FGLS )
	message ( STATUS
	  "    fgls NOT being built. Use -DBUILD_FGLS:BOOL=TRUE to enable it."
	)
endif ( BUILD_FGLS )

if ( BUILD_SERVER2 )
	message ( STATUS "    Building server 2. File fgms2.log fgms_exit2 fgms_reset2 fgms_stat2 fgms2.conf")
else ( BUILD_SERVER2 )
	message ( STATUS
	  "    server2 NOT being built. Use -DBUILD_SERVER2:BOOL=TRUE to enable it."
	)
	message ( STATUS "    Building FGMS")
endif ( BUILD_SERVER2 )

message ( STATUS )
message ( STATUS "type 'make' to build all parts" )
message ( STATUS "type 'make VERBOSE=1' for verbose build" )
message ( STATUS )

# eof - CMakeLists.txt
