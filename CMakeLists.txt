#
# single CMakeLists.txt for fgms-0-x - hand crafted - commenced 2012/07/03
# 20161121 - Remove ability to compile FGTracker which is obsolete
# 20130411 - Change to use external version file
# 20121130 - Add link_directories ( ${PostgreSQL_LIBRARY_DIR} ) at least for WIN32 - AGE 26
# 20121025 - Add a windows 'perror' macro PERROR, and minor changes in crossfeed code - age 25
# 20121018 - Try to get the unix install to /usr/sbin right, and bump age to 24
# 20120815 - In Unix, install to /usr/sbin if CMAKE_INSTALL_PREFIX not given - default is /usr/local/sbin
#            Bump AGE to 20, since a number of other commits done.
# 20120808 - Change the IPCKEY value if building mp_server2 - bumped AGE to 19
# 20120723 - Add ENABLE_DEBUG_OUTPUT, which defines DEF_DEBUG_OUTPUT in fg_tracker.cxx
# and add USE_PTHREAD_IN_TRACKER server, replacing fork()

cmake_minimum_required( VERSION 2.8.8 )

project( fgms )

##################################################
#####       the version of the program
#####
#####       Increment AGE if source code
#####       has changed at all (e.g. bugfix)
#####
#####       If fgms has new features but is still
#####       protocol compatible increment MINOR
#####       and set AGE to 0
#####
#####       If fgms gets incompatible with older
#####       version increment MAJOR and set MINOR
#####       and AGE to 0
#####
#####       See the 'version' file
##################################################
# read 'version' file into a variable (stripping any newlines or spaces)
file(READ version versionFile)
if (NOT versionFile)
    message(FATAL_ERROR "Unable to determine FGMS version. 'version' file is missing.")
endif()
string(STRIP "${versionFile}" FGMS_VERSION)


# change names if build server and tracker #2
set( EXE_NAME fgms )

# directories
IF(NOT DEFINED USER_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr")
  message(STATUS "*** Set CMAKE_INSTALL_PREFIX to ${CMAKE_INSTALL_PREFIX}")
ELSE(NOT DEFINED USER_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX ${USER_INSTALL_PREFIX})
  message(STATUS "*** Set CMAKE_INSTALL_PREFIX to user ${CMAKE_INSTALL_PREFIX}")
ENDIF(NOT DEFINED USER_INSTALL_PREFIX)
if (NOT DEFINED SYSCONFDIR)
    set ( SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc" )
endif (NOT DEFINED SYSCONFDIR)
if (NOT DEFINED SBINDIR)
    set ( SBINDIR "${CMAKE_INSTALL_PREFIX}/sbin" )
endif (NOT DEFINED SBINDIR)
add_definitions( -DSYSCONFDIR="${SYSCONFDIR}" )

# Allow developer to select Dynamic or static library build
set( LIB_TYPE STATIC )  # set default static
option( BUILD_SHARED_LIB "Build Shared Library" OFF )
option( BUILD_SERVER2 "Build a server with different defaults" OFF )

if(UNIX)
    option( ENABLE_DEBUG_SYMBOLS "Add debug symbols into the binary." OFF )
endif(UNIX)

if(CMAKE_COMPILER_IS_GNUCXX)
    set( WARNING_FLAGS -Wall )
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
   set( WARNING_FLAGS "-Wall -Wno-overloaded-virtual" )
else()
   set( WARNING_FLAGS "-Wno-unused-but-set-variable" )
endif() 

if(WIN32)
    if(MSVC)
        # turn off various warnings
        set(WARNING_FLAGS "${WARNING_FLAGS} /wd4996")
        # C4146: unary minus operator applied to unsigned type, result still unsigned
        # C4267: '=' : conversion from 'size_t' to 'int', possible loss of data
        # foreach(warning 4244 4251 4267 4275 4290 4786 4305)
        foreach(warning 4146 4267)
            set(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
        endforeach()

        set( EXTRA_FLAGS "-DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS" )
        # if (${MSVC_VERSION} EQUAL 1600)
        #    list(APPEND EXTRA_LD_FLAGS "/FORCE:MULTIPLE" )
        # endif (${MSVC_VERSION} EQUAL 1600)
    endif(MSVC)
    set( NOMINMAX 1 )
    add_definitions( -DUL_MSVC -DOPENALSDK -DHAVE_CONFIG_H -DFREEGLUT_LIB_PRAGMAS -DUL_WIN32 -D_CRT_SECURE_NO_DEPRECATE -DFREEGLUT_STATIC )
    configure_file( "${CMAKE_SOURCE_DIR}/config.h.msvc" "${CMAKE_BINARY_DIR}/config.h" COPYONLY)
    file(WRITE "${CMAKE_BINARY_DIR}/unistd.h" "/* dummy unistd.h for windows */") 
else(WIN32)
    set ( EXTRA_FLAGS "-std=c++11" )
    if(ENABLE_DEBUG_SYMBOLS)
        list(APPEND EXTRA_FLAGS "-g")
    endif(ENABLE_DEBUG_SYMBOLS)
    set( WARNING_FLAGS "${WARNING_FLAGS} -Wno-unused-local-typedefs" )
endif(WIN32)

if(BUILD_SERVER2)
    set( EXE_NAME mp_server2 )
    add_definitions( -DDEF_SERVER_LOG="fg_server2.log" -DDEF_EXIT_FILE="fgms_exit2"
        -DDEF_RESET_FILE="fgms_reset2" -DDEF_STAT_FILE="fgms_stat2" -DDEF_CONF_FILE="fgms2.conf"
        -DIPCKEY=0xf9f6 )
    message(STATUS "*** Building server 2. File fg_server2.log fgms_exit2 fgms_reset2 fgms_stat2 fgms2.conf")
endif(BUILD_SERVER2)

add_definitions(${WARNING_FLAGS} ${EXTRA_FLAGS} -D_REENTRANT)
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EXTRA_LD_FLAGS}" )
message(STATUS "*** set CXX FLAGS [${CMAKE_CXX_FLAGS}] C FLAGS [${CMAKE_C_FLAGS}]")

add_definitions( -DVERSION="${FGMS_VERSION}" )

if(ENABLE_DEBUG_OUTPUT)
   add_definitions( -DDEF_DEBUG_OUTPUT=true )
endif(ENABLE_DEBUG_OUTPUT)

# to distinguish between debug and release lib
if (WIN32)
set( CMAKE_DEBUG_POSTFIX "d" )
endif (WIN32)

if(BUILD_SHARED_LIB)
   set(LIB_TYPE SHARED)
   message(STATUS "*** Building DLL library ${LIB_TYPE}")
else(BUILD_SHARED_LIB)
   message(STATUS "*** Building static library ${LIB_TYPE}")
endif(BUILD_SHARED_LIB)

include_directories( SYSTEM "${PROJECT_BINARY_DIR}" src src/flightgear/MultiPlayer src/plib src/simgear/debug src/server
src/libcli )

### add_subdirectory( src/server )
# Project [fg_server] [Static Library] [noinst_LIBRARIES], with 6 sources. 12 hdrs.
set( fg_server_SRCS 
    src/server/fg_server.cxx 
    src/server/fg_tracker.cxx 
    src/server/fg_config.cxx 
    src/server/fg_list.cxx 
    src/server/fg_cli.cxx 
    src/server/fg_util.cxx 
    src/server/daemon.cxx 
    src/server/fg_geometry.cxx )
set( fg_server_HDRS  
	src/server/fg_server.hxx 
	src/server/fg_tracker.hxx 
    src/server/fg_config.hxx 
	src/server/fg_list.hxx 
    src/server/fg_cli.hxx
    src/server/daemon.hxx 
	src/server/fg_util.hxx 
    src/server/fg_common.hxx )
add_library( fg_server ${LIB_TYPE} ${fg_server_SRCS} ${fg_server_HDRS} )
# NO INSTALL [noinst_LIBRARIES]
list( APPEND add_LIBS fg_server )

#### add_subdirectory( src/flightgear/MultiPlayer )
# Project [MultiPlayer] [Static Library] [noinst_LIBRARIES], with 1 sources. 7 hdrs.
set( MultiPlayer_SRCS src/flightgear/MultiPlayer/tiny_xdr.cxx )
set( MultiPlayer_HDRS src/flightgear/MultiPlayer/tiny_xdr.hxx 
    src/flightgear/MultiPlayer/mpmessages.hxx )
add_library( Multiplayer ${LIB_TYPE} ${MultiPlayer_SRCS} ${MultiPlayer_HDRS} )
# NO INSTALL [noinst_LIBRARIES]
list(APPEND add_LIBS Multiplayer)

#### add_subdirectory( src/plib )
# Project [plib] [Static Library] [noinst_LIBRARIES], with 1 sources. 1 hdrs.
set( plib_SRCS src/plib/netSocket.cxx )
set( plib_HDRS src/plib/netSocket.h )
add_library( Plib ${LIB_TYPE} ${plib_SRCS} ${plib_HDRS} )
# NO INSTALL [noinst_LIBRARIES]
list(APPEND add_LIBS Plib)

#### add_subdirectory( src/libcli )
# Project [libcli] [Static Library] [noinst_LIBRARIES], with 1 sources. 1 hdrs.
set( libcli_SRCS
    src/libcli/debug.cxx
    src/libcli/filter.cxx
    src/libcli/cli_client.cxx
    src/libcli/libcli.cxx
    )
set( libcli_HDRS
    src/libcli/common.hxx
    src/libcli/command.hxx
    src/libcli/debug.hxx
    src/libcli/filter.hxx
    src/libcli/cli_client.hxx
    src/libcli/libcli.hxx
    )
if (WIN32)
    list(APPEND libcli_SRCS src/libcli/crypt-win.c)
endif ()
add_library( LibCLI ${LIB_TYPE} ${libcli_SRCS} ${libcli_HDRS} )
if(WIN32)
    list(APPEND add_LIBS LibCLI)
else(WIN32)
    list(APPEND add_LIBS LibCLI -lcrypt)
endif(WIN32)

#### add_subdirectory( src/libmsc ) Windows Only
# Project [libmsc] [Static Library] [noinst_LIBRARIES], with 1 source. 1 hdr.
set( libmsc_SRCS
    src/libmsc/msc_unistd.cxx
    )
set( libmsc_HDRS
	src/libmsc/msc_unistd.hxx
    )
if(MSVC)
	add_library( LibMSC ${LIB_TYPE} ${libmsc_SRCS} ${libmsc_HDRS} )
endif(MSVC)
if(WIN32)
    list(APPEND add_LIBS LibMSC)
else(WIN32)
	if(MSVC)
		list(APPEND add_LIBS -lLibMSC)
	endif(MSVC)
endif(WIN32)

# Project [SGUtils] [Static Library] [noinst_LIBRARIES], with 2 sources. 3 hdrs.
set( name SGUtils )
set( ${name}_SRCS
    src/simgear/debug/logstream.cxx 
    src/simgear/math/SGEuler.cxx 
    )
set( ${name}_HDRS
    src/simgear/debug/debug_types.h 
    src/simgear/debug/logstream.hxx 
    src/simgear/math/SGEuler.hxx 
    )
add_library( ${name} ${LIB_TYPE} ${${name}_SRCS} ${${name}_HDRS} )
# NO INSTALL [noinst_LIBRARIES]
list(APPEND add_LIBS ${name})

# find the threads library
find_package(Threads REQUIRED)
if(Threads_FOUND)
  if(WIN32)
    list(APPEND add_LIBS winmm.lib ws2_32.lib)
	###message( STATUS "*** ENV INCLUDE $ENV{INCLUDE}")
	message( STATUS "*** Finding pthread.h HINTS [${CMAKE_PREFIX_PATH}] PATHS [${CMAKE_INCLUDE_PATH}] PATH_SUFFIXES include" )
	###find_file( thread_INC pthread.h
	find_path( thread_INC pthread.h
		HINTS ${CMAKE_PREFIX_PATH} $ENV{INCLUDE}
		PATHS ${CMAKE_INCLUDE_PATH} $ENV{INCLUDE}
		PATH_SUFFIXES include )
	if(thread_INC)
		include_directories( SYSTEM ${thread_INC} )
		message(STATUS "*** thread_INC: ${thread_INC}")
	else(thread_INC)
		message(FATAL_ERROR "*** thread_INC: ${thread_INC} pthread.h NOT FOUND!")
	endif(thread_INC)
	find_library( thread_LIB pthreadVC2.lib
		HINTS ${CMAKE_PREFIX_PATH} $ENV{LIB}
		PATHS ${CMAKE_INCLUDE_PATH} $ENV{LIB}
		PATH_SUFFIXES lib )
	if(thread_LIB)
		list(APPEND add_LIBS ${thread_LIB})
		message(STATUS "*** Found ${thread_LIB})" )
	else(thread_LIB)
		message( FATAL_ERROR "*** NOT FOUND target_link_libraries(pthread1 ${thread_LIB})" )
	endif(thread_LIB)
  else(WIN32)
     list(APPEND add_LIBS ${CMAKE_THREAD_LIBS_INIT})
  endif(WIN32)
else(Threads_FOUND)
  message( FATAL_ERROR "*** THREADS NOT FOUND!")
endif(Threads_FOUND)

# Project [fgms] [Console Application] [sbin_PROGRAMS], with 1 sources. deps [sgutils MultiPlayer plib fg_server]4
set( fgms_SRCS src/server/main.cxx )
add_executable( ${EXE_NAME} ${fgms_SRCS} )
target_link_libraries( ${EXE_NAME} ${add_LIBS} )
message(STATUS "*** target_link_libraries( ${EXE_NAME} ${add_LIBS} )" )
if(WIN32)
    set_target_properties( ${EXE_NAME} PROPERTIES DEBUG_POSTFIX "d" )
    install(TARGETS ${EXE_NAME} DESTINATION bin)
else(WIN32)
    message(STATUS "*** Will install binary to ${SBINDIR}")
    message(STATUS "*** Will read config from ${SYSCONFDIR}")
    install(TARGETS ${EXE_NAME} DESTINATION ${SBINDIR})
endif(WIN32)
# eof - CMakeLists.txt
