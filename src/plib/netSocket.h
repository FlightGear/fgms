/**
 * @file netSocket.h
 * @author  Dave McClurg <dpm@efn.org>
 * @brief netSocket is a thin C++ wrapper over bsd sockets to
 *        facilitate porting to other platforms.
 *        Part of <a href="../../plib/html">PLIB</a> - A Suite of Portable Game Libraries
 */


/*
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 1998,2002  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net

     $Id: netSocket.h,v 1.1.1.1 2007/06/12 10:10:24 oliver Exp $
*/

/****
* NAME
*   netSocket - network sockets
*
* DESCRIPTION
*   netSocket is a thin C++ wrapper over bsd sockets to
*   facilitate porting to other platforms
*
* AUTHOR
*   Dave McClurg <dpm@efn.org>
*
* CREATION DATE
*   Dec-2000
*
****/

#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#if __FreeBSD__
  #include <sys/types.h>
  #include <sys/socket.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <string>
#include <fg_util.hxx>

/**
 * @class netAddress
 * @brief Socket address, internet style.
 */
class netAddress
{
  /* DANGER!!!  This MUST match 'struct sockaddr_in' exactly! */
#if __FreeBSD__
  int8_t         sin_len        ;
  sa_family_t    sin_family     ;
  in_port_t      sin_port       ;
  in_addr_t      sin_addr       ;
#else 
  short          sin_family     ;
  unsigned short sin_port       ;
  unsigned int   sin_addr       ;
#endif
  char           sin_zero [ 8 ] ;

public:
  netAddress () {}
  netAddress ( const char* host, int port ) ;

  void set ( const char* host, int port ) ;
  const std::string getHost () const ;
  unsigned int getPort() const ;
  unsigned int getIP () const ;
  unsigned int getFamily () const ;
  static const char* getLocalHost () ;

  bool getBroadcast () const ;
  bool operator == ( const netAddress& Value ) const;
  bool operator != ( const netAddress& Value ) const;
  void operator =  ( const netAddress& Value );
};


/**
 * @class netSocket
 * @brief Socket type
 */
class netSocket
{
  int   handle ;
  bool  isStream;

public:

  netSocket () ;
  virtual ~netSocket () ;

  int getHandle () const { return handle; }
  void setHandle (int handle) ;

  bool  open        ( bool stream=true ) ;
  void  close       ( void ) ;
  void  shutdown    ( void ) ;
  int   bind        ( const char* host, int port ) ;
  int   listen      ( int backlog ) ;
  int   accept      ( netAddress* addr ) ;
  int   connect     ( const char* host, int port ) ;
  int   write_str   ( const char* str, int len );
  int   write_str   ( const string&  str );
  int   write_char  ( const char&  c );
  int   send        ( const void * buffer, int size, int flags = 0 ) ;
  int   sendto      ( const void * buffer, int size, int flags, const netAddress* to ) ;
  int	read_char   ( unsigned char& c);
  int   recv        ( void * buffer, int size, int flags = 0 ) ;
  int   recvfrom    ( void * buffer, int size, int flags, netAddress* from ) ;

  void setSockOpt ( int SocketOption, bool Set );
  void setBlocking ( bool blocking ) ;
  void setBroadcast ( bool broadcast ) ;

  static bool isNonBlockingError () ;
  static int select ( netSocket** reads, netSocket** writes, int timeout ) ;
} ;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int netInit () ;

extern const char* netFormat ( const char* fmt, ... ) ;
#if defined(UL_WIN32)
extern void win_wsa_perror( char * msg );
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // NET_SOCKET_H

