/*
 * config.h - configuration options
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

#ifndef __config_h
#define __config_h

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
#define DEBUG_LEVEL		1
#endif

#ifndef RUN_AS_DAEMON
#define RUN_AS_DAEMON   1
#endif

#endif
/* eof - config.h */

