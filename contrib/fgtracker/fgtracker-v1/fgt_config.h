/*
 * fgt_config.h - configuration options
 *
 *   Author: Gabor Toth <tgbp@freemail.hu>
 *   License: GPL
 *
 *   $Log: config.h,v $
 *   Revision 1.2  2006/05/10 21:22:34  koversrac
 *   Comment with author and license has been added.
 *   Revision 1.3 2012/07/02 geoff
 *   Allow external setting of the defaults
 *
 */

#ifndef _fgt_config_h
#define _fgt_config_h

#ifdef _MSC_VER
#include <Winsock2.h> /* NOTE: This ALSO includes <windows.h> */
#include <sys/types.h>
#include <sys/stat.h>
#include <Ws2tcpip.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#define inline __inline
#ifndef bool
#define bool int
#endif
#endif // _MSC_VER

#ifndef SERVER_ADDRESS
#define SERVER_ADDRESS		"127.0.0.1"
#endif

#ifndef SERVER_PORT
#define SERVER_PORT		8000
#endif

#ifndef SERVER_LISTENQ
#define SERVER_LISTENQ		10
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL		3 // 0 = none, max is 3, to syslog("fgtracker-server") if daemon, else printf()
#endif

#ifndef RUN_AS_DAEMON
#define RUN_AS_DAEMON   1
#endif

#endif // #ifndef _fgt_config_h
/* eof - fgt_config.h */

