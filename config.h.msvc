// config.h
// SPECIAL HAND CRAFTED CONFIG.H FOR MSVC
// 20121016 - After removal of 'build' folder, and all automake pieces, add back this as config.h.msvc
// 20120623 - Bumped verions, to agree with configure.ac
// 20120322 - update using pthreads for telnet
// 20110812 - updated with latest source - updfgms-stable.bat
// 20110324 - hand crafted for MSVC
#ifndef _FGMS_CONFIG_H_
#define _FGMS_CONFIG_H_

#pragma warning ( disable : 4996 )  // remove warning "The POSIX name for this item is depreciated" ...
#pragma warning ( disable : 4244 )  // conversion from 'a' to 'b', possible loss of data

#include <sys/types.h>
#include <sys/stat.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

#ifndef VERSION
#define VERSION "0.10.23-WIN32" // UPD20121026 - Should agree with AGE etc in CMakeLists.txt
#endif

#define sleep(a) Sleep(a * 1000)
#define strcasecmp strcmp
#define strncasecmp _strnicmp
#define snprintf _snprintf
#define unlink _unlink

/* Now REQUIRED <pthread.h>, and the pthreadVC2.lib and dll */
#define HAVE_PTHREAD_H 1

#endif // #ifndef _FGMS_CONFIG_H_
// eof - config.h
