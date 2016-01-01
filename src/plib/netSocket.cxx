/**
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

     $Id: netSocket.cxx,v 1.1.1.1 2007/06/12 10:10:24 oliver Exp $
*/

#include "netSocket.h"

#if defined(UL_CYGWIN) || !defined (UL_WIN32)

#if defined(UL_MAC_OSX)
#  include <netinet/in.h>
#endif

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>    /* Need both for Mandrake 8.0!! */
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

#else

#include <iostream>
#include <winsock2.h>
#include <stdarg.h>

#endif

#if defined(UL_MSVC) && !defined(socklen_t)
#define socklen_t int
#endif

/* Paul Wiltsey says we need this for Solaris 2.8 */
 
#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long)-1)
#endif

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

using namespace std;

netAddress::netAddress ( const char* host, int port )
{
  set ( host, port ) ;
}


void netAddress::set ( const char* host, int port )
{
  memset(this, 0, sizeof(netAddress));

#if __FreeBSD__
  sin_len = sizeof(netAddress);
#endif
  sin_family = AF_INET ;
  sin_port = htons (port);
  sin_addr = INADDR_ANY;

  /* Convert a string specifying a host name or one of a few symbolic
  ** names to a numeric IP address.  This usually calls gethostbyname()
  ** to do the work; the names "" and "<broadcast>" are special.
  */

  if (host[0] == '\0')
    sin_addr = INADDR_ANY;
  else
  if (host[0] == '<' && strcmp(host, "<broadcast>") == 0)
    sin_addr = INADDR_BROADCAST;
  else
  {
    sin_addr = inet_addr ( host ) ;

    if ( sin_addr == INADDR_NONE )
    {
      struct hostent *hp = gethostbyname ( host ) ;
      if ( hp != NULL )
      {
      	memcpy ( (char *) &sin_addr, hp->h_addr, hp->h_length ) ;
      }
      else
      {
        sin_addr = INADDR_ANY ;
      }
    }
  }
}

void netAddress::setPort ( int port )
{
  sin_port = htons (port);
}

/**
 * @brief Create a string object representing an IP address.
 *        This is always a string of the form 'dd.dd.dd.dd' (with variable
 *        size numbers). 
 */
const string netAddress::getHost () const
{
	long x = ntohl(sin_addr);
	string result = NumToStr ((x>>24) & 0xff, 0) + ".";
	result += NumToStr ((x>>16) & 0xff, 0) + ".";
	result += NumToStr ((x>> 8) & 0xff, 0) + ".";
	result += NumToStr ((x>> 0) & 0xff, 0);
	return result;
}

unsigned int
netAddress::getIP () const 
{ 
	return sin_addr; 
}
/**
 * @brief Return the port no as int */
unsigned int netAddress::getPort() const
{
  return ntohs(sin_port);
}

unsigned int
netAddress::getFamily () const 
{ 
	return sin_family; 
}

const char* netAddress::getLocalHost ()
{
  //gethostbyname(gethostname())

  char buf[256];
  memset(buf, 0, sizeof(buf));
  gethostname(buf, sizeof(buf)-1);
  const hostent *hp = gethostbyname(buf);

  if (hp && *hp->h_addr_list)
  {
    in_addr     addr = *((in_addr*)*hp->h_addr_list);
    const char* host = inet_ntoa(addr);

    if ( host )
      return host ;
  }

  return "127.0.0.1" ;
}


bool netAddress::getBroadcast () const
{
  return sin_addr == INADDR_BROADCAST;
}

bool netAddress::operator == (const netAddress& Value) const
{
  if ((this->sin_family == Value.sin_family)
  &&  (this->sin_addr == Value.sin_addr))
    return (true);
  return (false);
}

bool netAddress::operator != (const netAddress& Value) const
{
  if ((this->sin_family == Value.sin_family)
  &&  (this->sin_addr == Value.sin_addr))
    return (false);
  return (true);
}

void netAddress::operator = (const netAddress& Value)
{

  this->sin_family	= Value.sin_family;
  this->sin_port	= Value.sin_port;
  this->sin_addr	= Value.sin_addr;
  for (int i=0; i<8; i++)
  	this->sin_zero[i] = Value.sin_zero[i];
}

netSocket::netSocket ()
{
  handle = -1 ;
  isStream = false;
}


netSocket::~netSocket ()
{
  close () ;
}


void netSocket::setHandle (int _handle)
{
  close () ;
  handle = _handle ;
}


bool netSocket::open ( bool stream )
{
  close () ;
  handle = ::socket ( AF_INET, (stream? SOCK_STREAM: SOCK_DGRAM), 0 ) ;
  isStream = stream;
  return (handle != -1);
}


void netSocket::setBlocking ( bool blocking )
{
  assert ( handle != -1 ) ;

#if defined(UL_CYGWIN) || !defined (UL_WIN32)

  int delay_flag = ::fcntl (handle, F_GETFL, 0);

  if (blocking)
    delay_flag &= (~O_NDELAY);
  else
    delay_flag |= O_NDELAY;

  ::fcntl (handle, F_SETFL, delay_flag);

#else

  u_long nblocking = blocking? 0: 1;
  ::ioctlsocket(handle, FIONBIO, &nblocking);

#endif
}


void netSocket::setSockOpt ( int SocketOption, bool Set )
{
  assert ( handle != -1 ) ;
  int result;
  if ( Set == true ) {
      int one = 1;
#ifdef UL_WIN32
      result = ::setsockopt( handle, SOL_SOCKET, SocketOption, (char*)&one, sizeof(one) );
#else
      result = ::setsockopt( handle, SOL_SOCKET, SocketOption, &one, sizeof(one) );
#endif
  } else {
      result = ::setsockopt( handle, SOL_SOCKET, SocketOption, NULL, 0 );
  }
  if ( result < 0 ) {
      perror("set sockopt:");
  }
  assert ( result != -1 );
}

void netSocket::setBroadcast ( bool broadcast )
{
  assert ( handle != -1 ) ;
  int result;
  if ( broadcast ) {
      int one = 1;
#ifdef UL_WIN32
      result = ::setsockopt( handle, SOL_SOCKET, SO_BROADCAST, (char*)&one, sizeof(one) );
#else
      result = ::setsockopt( handle, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one) );
#endif
  } else {
      result = ::setsockopt( handle, SOL_SOCKET, SO_BROADCAST, NULL, 0 );
  }
  if ( result < 0 ) {
      perror("set broadcast:");
  }
  assert ( result != -1 );
}


int netSocket::bind ( const char* host, int port )
{
  assert ( handle != -1 ) ;
  netAddress addr ( host, port ) ;
  return ::bind(handle,(const sockaddr*)&addr,sizeof(netAddress));
}


int netSocket::listen ( int backlog )
{
  assert ( handle != -1 ) ;
  return ::listen(handle,backlog);
}


int netSocket::accept ( netAddress* addr )
{
  assert ( handle != -1 ) ;

  if ( addr == NULL )
  {
    return ::accept(handle,NULL,NULL);
  }
  else
  {
    socklen_t addr_len = (socklen_t) sizeof(netAddress) ;
    return ::accept(handle,(sockaddr*)addr,&addr_len);
  }
}


int netSocket::connect ( const char* host, int port )
{
  assert ( handle != -1 ) ;
  netAddress addr ( host, port ) ;
  if ( addr.getBroadcast() ) {
      setBroadcast( true );
  }
  return ::connect(handle,(const sockaddr*)&addr,sizeof(netAddress));
}

int netSocket::write_str ( const char* str, int len )
{
	char* p		= (char*) str;
	int left	= len;
	int written	= 0;

	while (left > 0)
	{
		written = ::send (handle, p, left, MSG_NOSIGNAL);
		if (written == SOCKET_ERROR)
		{
			if (RECOVERABLE_ERROR)
			{
				written = 0;
				usleep(5000);
			}
				
			else
				return -1;
		}
		left -= written;
		p += written;
	}
	return len;
}

int netSocket::write_str ( const string&  str )
{
	return write_str (str.c_str(), str.length());
}

int netSocket::write_char ( const char&  c )
{
	return ::send (handle, &c, 1, 0);
}

int netSocket::send (const void * buffer, int size, int flags)
{
  assert ( handle != -1 ) ;
  return ::send (handle, (const char*)buffer, size, flags);
}


int netSocket::sendto ( const void * buffer, int size,
                        int flags, const netAddress* to )
{
  assert ( handle != -1 ) ;
  return ::sendto(handle,(const char*)buffer,size,flags,
                         (const sockaddr*)to,sizeof(netAddress));
}

int netSocket::read_char ( unsigned char& c )
{
	int n;
	while (1)
	{
		n = ::recv (handle, (char *)&c, 1, 0 );
		if (n == SOCKET_ERROR)
		{
			if (RECOVERABLE_ERROR)
				continue;
			return -1;
		}
		return n;
	}
	return n;
}

int netSocket::recv (void * buffer, int size, int flags)
{
  assert ( handle != -1 ) ;
  return ::recv (handle, (char*)buffer, size, flags);
}


int netSocket::recvfrom ( void * buffer, int size,
                          int flags, netAddress* from )
{
  assert ( handle != -1 ) ;
  socklen_t fromlen = (socklen_t) sizeof(netAddress) ;
  return ::recvfrom(handle,(char*)buffer,size,flags,(sockaddr*)from,&fromlen);
}


void netSocket::close (void)
{
  if ( handle != -1 )
  {
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
    errno = 0;
    if (::close( handle ) != 0)
    {
        printf ("netSocket::close: %s\n", strerror (errno));
    }
#else
    ::closesocket( handle );
#endif
    handle = -1 ;
  }
}

void netSocket::shutdown (void)
{
  if ( handle != -1 )
  {
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
    if (isStream)
    {
        errno = 0;
        if (::shutdown (handle, SHUT_RDWR) != 0)
        {
            printf ("netSocket::shutdown: %s on fd %u\n", strerror (errno),
                    handle);
        }
    }
#endif
    handle = -1 ;
  }
}


bool netSocket::isNonBlockingError ()
{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
  switch (errno) {
  case EWOULDBLOCK: // always == NET_EAGAIN?
  case EALREADY:
  case EINPROGRESS:
    return true;
  }
  return false;
#else
  int wsa_errno = WSAGetLastError();
  if ( wsa_errno != 0 )
  {
    WSASetLastError(0);
    switch (wsa_errno) {
    case WSAEWOULDBLOCK: // always == NET_EAGAIN?
    case WSAEALREADY:
    case WSAEINPROGRESS:
      return true;
    }
    printf("WARNING: WSAGetLastError() = %d",wsa_errno);
  }
  return false;
#endif
}


//////////////////////////////////////////////////////////////////////
//
//	
//
//////////////////////////////////////////////////////////////////////
int netSocket::select ( netSocket** reads, netSocket** writes, int timeout )
{
  fd_set r,w;
  int	retval;
  FD_ZERO (&r);
  FD_ZERO (&w);
  int i;
  int num = 0 ;
  if ((reads == 0) || (reads[0] == 0))
    return (0);
  if (reads)
  {
    for ( i=0; reads[i]; i++ )
    {
      int fd = reads[i]->getHandle();
      FD_SET (fd, &r);
      num++;
    }
  }
  if (writes)
  {
    for ( i=0; writes[i]; i++ )
    {
      int fd = writes[i]->getHandle();
      FD_SET (fd, &w);
      num++;
    }
  }
  if (!num)
    return num ;
  /* Set up the timeout */
  struct timeval tv ;
  /*
  tv.tv_sec = timeout/1000;
  tv.tv_usec = (timeout%1000)*1000;
  */
  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  // It bothers me that select()'s first argument does not appear to
  // work as advertised... [it hangs like this if called with
  // anything less than FD_SETSIZE, which seems wasteful?]
  // Note: we ignore the 'exception' fd_set - I have never had a
  // need to use it.  The name is somewhat misleading - the only
  // thing I have ever seen it used for is to detect urgent data -
  // which is an unportable feature anyway.
  retval = ::select (FD_SETSIZE, &r, &w, 0, &tv);
  if (retval <= 0) // timeout or error
    return retval;
  //remove sockets that had no activity
  num = 0 ;
  if (reads)
  {
    for ( i=0; reads[i]; i++ )
    {
      int fd = reads[i]->getHandle();
      if (! FD_ISSET (fd, &r)) {
        reads[i] = 0;
      }
      else
        num++;
    }
    reads[i] = NULL ;
  }
  if (writes)
  {
    for ( i=0; writes[i]; i++ )
    {
      int fd = writes[i]->getHandle();
      if (! FD_ISSET (fd, &w)) {
        writes[i] = 0;
      }
      else
        num++;
    }
    writes[i] = NULL ;
  }
  return num ;
}

/* Init/Exit functions */

static void netExit ( void )
{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
#else
	/* Clean up windows networking */
	if ( WSACleanup() == SOCKET_ERROR ) {
		if ( WSAGetLastError() == WSAEINPROGRESS ) {
			WSACancelBlockingCall();
			WSACleanup();
		}
	}
#endif
}


int netInit ()
{
  assert ( sizeof(sockaddr_in) == sizeof(netAddress) ) ;

#if defined(UL_CYGWIN) || !defined (UL_WIN32)
#else
	/* Start up the windows networking */
	WORD version_wanted = MAKEWORD(1,1);
	WSADATA wsaData;

	if ( WSAStartup(version_wanted, &wsaData) != 0 ) {
		printf("Couldn't initialize Winsock 1.1\n");
		return(-1);
	}
#endif

  atexit( netExit ) ;
	return(0);
}


const char* netFormat ( const char* format, ... )
{
  static char buffer[ 256 ];
  va_list argptr;
  va_start(argptr, format);
  vsprintf( buffer, format, argptr );
  va_end(argptr);
  return( buffer );
}

#if defined(UL_WIN32)
#ifndef SPRTF
#define SPRTF printf
#endif
// get a message from the system for this error value
char *get_errmsg_text( int err )
{
    LPSTR ptr = 0;
    DWORD fm = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&ptr, 0, NULL );
    if (ptr) {
        size_t len = strlen(ptr);
        while(len--) {
            if (ptr[len] > ' ') break;
            ptr[len] = 0;
        }
        if (len) return ptr;
        LocalFree(ptr);
    }
    return NULL;
}

void win_wsa_perror( char * msg )
{
    int err = WSAGetLastError();
    LPSTR ptr = get_errmsg_text(err);
    if (ptr) {
        SPRTF("%s = %s (%d)\n", msg, ptr, err);
        LocalFree(ptr);
    } else {
        SPRTF("%s %d\n", msg, err);
    }
}

#endif /* UL_WIN32 */
