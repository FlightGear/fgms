# single CMakeLists.txt for fgms-0-x - hand crafted 2012/07/03

cmake_minimum_required( VERSION 2.6 )

project( fgms )

# The version number.
set( fgms_MAJOR 0 )
set( fgms_MINOR 10 )
set( fgms_AGE 18 )

set( EXE_NAME fgms )

# Allow developer to select is Dynamic or static library built
set( LIB_TYPE STATIC )  # set default static
option( BUILD_SHARED_LIB "Build Shared Library" OFF )
option( BUILD_TRACKER "Build 'tracker' server and client" OFF )
option( BUILD_SERVER2 "Build a server with different defaults" OFF )
option( USE_TRACKER_PORT "Use pthreads to replace fork, and a vector for message exchange." OFF )
option( ENABLE_TRACKER_LOG "Enable a tracker message log." OFF )
option( USE_POSTGRESQL "Use a PostgreSQL database in the tracking server." ON )
if(UNIX)
    option( ENABLED_DEBUG_SYMBOLS "Add debug symbols into the binary." OFF )
endif(UNIX)

if(CMAKE_COMPILER_IS_GNUCXX)
    set( WARNING_FLAGS -Wall )
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
   set( WARNING_FLAGS "-Wall -Wno-overloaded-virtual" )
endif() 

if(WIN32)
    if(MSVC)
        # turn off various warnings
        set(WARNING_FLAGS "${WARNING_FLAGS} /wd4996")
        # foreach(warning 4244 4251 4267 4275 4290 4786 4305)
        #     set(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
        # endforeach(warning)

        set( EXTRA_FLAGS "-DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS" )
        # if (${MSVC_VERSION} EQUAL 1600)
        #    list(APPEND EXTRA_LD_FLAGS "/FORCE:MULTIPLE" )
        # endif (${MSVC_VERSION} EQUAL 1600)
    endif(MSVC)
    set( NOMINMAX 1 )
    add_definitions( -DUL_MSVC -DOPENALSDK -DHAVE_CONFIG_H -DFREEGLUT_LIB_PRAGMAS -DUL_WIN32 -D_CRT_SECURE_NO_DEPRECATE -DFREEGLUT_STATIC )
	if(NOT USE_TRACKER_PORT)
		add_definitions( -DNO_TRACKER_PORT )
	endif(NOT USE_TRACKER_PORT)
else(WIN32)
    if(ENABLE_DEBUG_SYMBOLS)
        list(APPEND EXTRA_FLAGS "-g")
    endif(ENABLE_DEBUG_SYMBOLS)
    list(APPEND EXTRA_LD_FLAGS -L.)
endif(WIN32)

if(USE_TRACKER_PORT)
	add_definitions( -DUSE_TRACKER_PORT )
endif(USE_TRACKER_PORT)

if(ENABLE_TRACKER_LOG)
    add_definitions( -DENABLE_TRACKER_LOG )
endif(ENABLE_TRACKER_LOG)

if(BUILD_SERVER2)
    set( EXE_NAME mp_server2 )
    add_definitions( -DDEF_SERVER_LOG="fg_server2.log" -DDEF_EXIT_FILE="fgms_exit2"
        -DDEF_RESET_FILE="fgms_reset2" -DDEF_STAT_FILE="fgms_stat2" -DDEF_CONF_FILE="fgms2.conf" )
    message(STATUS "*** Building server 2. File fg_server2.log fgms_exit2 fgms_reset2 fgms_stat2 fgms2.conf")
endif(BUILD_SERVER2)

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS} ${EXTRA_FLAGS} -D_REENTRANT" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${EXTRA_FLAGS} -D_REENTRANT" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EXTRA_LD_FLAGS}" )
message(STATUS "*** set CXX FLAGS [${CMAKE_CXX_FLAGS}] C FLAGS [${CMAKE_C_FLAGS}]")

add_definitions( -DVERSION="${fgms_MAJOR}.${fgms_MINOR}.${fgms_AGE}" )

# to distinguish between debug and release lib
set( CMAKE_DEBUG_POSTFIX "d" )

if(BUILD_SHARED_LIB)
   set(LIB_TYPE SHARED)
   message(STATUS "*** Building DLL library ${LIB_TYPE}")
else(BUILD_SHARED_LIB)
   message(STATUS "*** Building static library ${LIB_TYPE}")
endif(BUILD_SHARED_LIB)

include_directories( SYSTEM "${PROJECT_BINARY_DIR}" src src/flightgear/MultiPlayer src/plib src/simgear/debug src/server )

### add_subdirectory( src/server )
# Project [fg_server] [Static Library] [noinst_LIBRARIES], with 6 sources. 12 hdrs.
set( fg_server_SRCS 
    src/server/fg_server.cxx 
    src/server/fg_tracker.cxx 
    src/server/fg_config.cxx 
    src/server/fg_player.cxx 
    src/server/daemon.cxx 
    src/server/fg_geometry.cxx )
set( fg_server_HDRS  src/server/fg_server.hxx src/server/fg_tracker.hxx 
    src/server/fg_config.hxx src/server/fg_player.hxx 
    src/server/daemon.hxx src/server/typcnvt.hxx 
    src/server/common.h )
add_library( fg_server ${LIB_TYPE} ${fg_server_SRCS} ${fg_server_HDRS} )
# NO INSTALL [noinst_LIBRARIES]
if(WIN32)
    list( APPEND add_LIBS fg_server )
else(WIN32)
    list(APPEND add_LIBS -lfg_server)
endif(WIN32)

#### add_subdirectory( src/flightgear/MultiPlayer )
# Project [MultiPlayer] [Static Library] [noinst_LIBRARIES], with 1 sources. 7 hdrs.
set( MultiPlayer_SRCS src/flightgear/MultiPlayer/tiny_xdr.cxx )
set( MultiPlayer_HDRS src/flightgear/MultiPlayer/tiny_xdr.hxx 
    src/flightgear/MultiPlayer/mpmessages.hxx )
add_library( MultiPlayer ${LIB_TYPE} ${MultiPlayer_SRCS} ${MultiPlayer_HDRS} )
# NO INSTALL [noinst_LIBRARIES]
if(WIN32)
    list(APPEND add_LIBS MultiPlayer)
else(WIN32)
    list(APPEND add_LIBS -lMultiPlayer)
endif(WIN32)

#### add_subdirectory( src/plib )
# Project [plib] [Static Library] [noinst_LIBRARIES], with 1 sources. 1 hdrs.
set( plib_SRCS src/plib/netSocket.cxx )
set( plib_HDRS src/plib/netSocket.h )
add_library( Plib ${LIB_TYPE} ${plib_SRCS} ${plib_HDRS} )
# NO INSTALL [noinst_LIBRARIES]
if(WIN32)
    list(APPEND add_LIBS Plib)
else(WIN32)
    list(APPEND add_LIBS -lPlib)
endif(WIN32)

### add_subdirectory( src/simgear/debug )
# Project [sgdebug] [Static Library] [noinst_LIBRARIES], with 1 sources. 2 hdrs.
set( sgdebug_SRCS src/simgear/debug/logstream.cxx )
set( sgdebug_HDRS src/simgear/debug/debug_types.h 
   src/simgear/debug/logstream.hxx )
add_library( SGdebug ${LIB_TYPE} ${sgdebug_SRCS} ${sgdebug_HDRS} )
# NO INSTALL [noinst_LIBRARIES]
if(WIN32)
    list(APPEND add_LIBS SGdebug)
else(WIN32)
    list(APPEND add_LIBS -lSGdebug)
endif(WIN32)

# Project [fgms] [Console Application] [sbin_PROGRAMS], with 1 sources. deps [sgdebug MultiPlayer plib fg_server]4
set( fgms_SRCS src/server/main.cxx )
add_executable( ${EXE_NAME} ${fgms_SRCS} )

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

target_link_libraries( ${EXE_NAME} ${add_LIBS} )
message(STATUS "*** target_link_libraries( ${EXE_NAME} ${add_LIBS} )" )
if(WIN32)
    set_target_properties( ${EXE_NAME} PROPERTIES DEBUG_POSTFIX "d" )
    install(TARGETS ${EXE_NAME} DESTINATION bin)
else(WIN32)
    install(TARGETS ${EXE_NAME} DESTINATION sbin)
endif(WIN32)

if(BUILD_TRACKER)
    if(USE_POSTGRESQL)
        # find postgresql include and libraries
        # =====================================
        find_package(PostgreSQL REQUIRED)
        if(PostgreSQL_FOUND)
            set( add_LIBS ${PostgreSQL_LIBRARIES} )
            include_directories( SYSTEM "${PostgreSQL_INCLUDE_DIRS}" )
            message(STATUS "*** Found PosgreSQL ${PostgreSQL_INCLUDE_DIRS} ${PostgreSQL_LIBRARIES}")
        else(PostgreSQL_FOUND)
            message(FATAL_ERROR "*** PosgreSQL NOT FOUND! Run cmake with -DBUILD_TRACKER:BOOL=FALSE")
        endif(PostgreSQL_FOUND)
    else(USE_POSTGRESQL)
        add_definitions( -DNO_POSTGRESQL )
    endif(USE_POSTGRESQL)

    set( tracker_SRCS contrib/fgtracker/fgt_error.c contrib/fgtracker/wrappers.c )
    set( tracker_HDRS contrib/fgtracker/fgt_error.h contrib/fgtracker/wrappers.h contrib/fgtracker/fgt_config.h contrib/fgtracker/fgt_common.h )
    add_library( tracker ${LIB_TYPE} ${tracker_SRCS} ${tracker_HDRS} )
    list(APPEND add_LIBS tracker)

    set( server_SRCS contrib/fgtracker/server.c )
    add_executable( fgt_server ${server_SRCS} )
    target_link_libraries( fgt_server ${add_LIBS} )
    set_target_properties( fgt_server PROPERTIES DEBUG_POSTFIX "d" )

    set( client_SRCS contrib/fgtracker/client.c )
    add_executable( fgt_client ${client_SRCS} )
    target_link_libraries( fgt_client ${add_LIBS} )
    set_target_properties( fgt_client PROPERTIES DEBUG_POSTFIX "d" )

    # deal with INSTALL
    install(TARGETS fgt_server fgt_client DESTINATION bin)
endif(BUILD_TRACKER)

# eof - CMakeLists.txt
