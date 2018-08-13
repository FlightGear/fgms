//
// This file is part of fgms, the flightgear multiplayer server
// https://sourceforge.net/projects/fgms/
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not see <http://www.gnu.org/licenses/>
//

/**
 * @file	netsocket.cxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	07/2017
 */

#ifdef HAVE_CONFIG_H
	#include "config.h" // for MSVC, always first
#endif

#ifdef _MSC_VER
	#include <sys/timeb.h>
	#include <libmsc/msc_unistd.hxx>
#endif

#if defined(UL_CYGWIN) || !defined (UL_WIN32)
#       if defined(UL_MAC_OSX)
#               include <netinet/in.h>
#       endif
#       include <sys/types.h>
#       include <sys/socket.h>
#       include <netinet/in.h>
#       include <arpa/inet.h>
#       include <time.h>
#       include <sys/time.h>    /* Need both for Mandrake 8.0!! */
#       include <unistd.h>
#       include <netdb.h>
#       include <fcntl.h>
#else
#       include <Winsock2.h>
#       include <stdarg.h>
#endif

#include <iostream>
#include <fglib/fg_util.hxx>
#include <fglib/netsocket.hxx>

#if defined(UL_MSVC) && !defined(socklen_t)
#       define socklen_t int
#endif

#ifdef _MSC_VER
/* 
 * Init/Exit functions
 * only needed on windows
 */
static bool donenetinit = false;

static void netExit ( void )
{
	if ( donenetinit )
	{
		/* Clean up windows networking */
		if ( WSACleanup() == SOCKET_ERROR )
		{
			if ( WSAGetLastError() == WSAEINPROGRESS )
			{
				WSACancelBlockingCall();
				WSACleanup();
			}
		}
	}
	donenetinit = false;
}

int netInit()
{
	if ( donenetinit )
	{
		return 0;        // that's ok
	}
	/* Start up the windows networking */
	WORD version_wanted = MAKEWORD ( 2, 2 );
	WSADATA wsaData;
	if ( WSAStartup ( version_wanted, &wsaData ) != 0 )
	{
		fprintf ( stderr, "Couldn't initialize Winsock 2.2\n" );
		return ( -1 );
	}
	atexit ( netExit );
	donenetinit = true;
	return ( 0 );
}
#endif

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

/** Construct an 'empty' netsocket.
 */
netsocket::netsocket
()
{
	m_handle = -1 ;
	m_is_stream = false;
} // netsocket::netsocket ()

//////////////////////////////////////////////////////////////////////

/** Close active connections.
 */
netsocket::~netsocket
()
{
	close ();
} // netsocket::~netsocket ()

//////////////////////////////////////////////////////////////////////

/** Assign another socket to *this
 */
void
netsocket::assign
(
	const netsocket& socket
)
{
	m_handle = socket.m_handle;
	m_is_stream = socket.m_is_stream;
} // netsocket::assign ()

//////////////////////////////////////////////////////////////////////

/** Return the filedescriptor of current connection.
 *
 * @return The filedescriptor.
 */
int
netsocket::handle
() const
{
	return ( m_handle );
} // netsocket::handle ()

//////////////////////////////////////////////////////////////////////

/** Set the filedescriptor.
 *
 * Close the current conenction, if any.
 */
void
netsocket::handle
(
	int handle
)
{
	close ();
	m_handle = handle;
} // netsocket::handle ()

//////////////////////////////////////////////////////////////////////

/** Create a socket.
 *
 * @param type		netsocket::TCP -> connection is a tcp stream
 *                      netsocket::UDP -> connection will be udp based
 * @return true         Socket creation succeeded.
 * @return false        Something went wrong
 */
bool
netsocket::open
(
	int family,
	const SOCKET_TYPES type
)
{
#ifdef _MSC_VER
	/* start up networking */
	if ( netInit() )
		return false;
#endif
	close ();
	if ( type == netsocket::TCP )
	{
		m_handle = ::socket ( family, SOCK_STREAM, 0 );
		m_is_stream = true;
	}
	else
	{
		m_handle = ::socket ( family, SOCK_DGRAM, 0 );
	}
	return ( m_handle != -1 );
} // netsocket::open ()

//////////////////////////////////////////////////////////////////////

/** Set connection into blocking mode.
 *
 * @param is_blocking   If true: set blocking mode
 *                      If false: set non-blocking mode
 */
void
netsocket::set_blocking
(
	const bool is_blocking
)
{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
	int flag = ::fcntl ( m_handle, F_GETFL, 0 );
	if ( is_blocking )
	{
		flag &= ( ~O_NDELAY );
	}
	else
	{
		flag |= O_NDELAY;
	}
	::fcntl ( m_handle, F_SETFL, flag );
#else
	u_long blocking = is_blocking? 0: 1;
	::ioctlsocket ( m_handle, FIONBIO, & blocking );
#endif
} // netsocket::set_blocking ()

//////////////////////////////////////////////////////////////////////

/** Set socket option
 *
 * @param opt	the socket option to (un)set, see GETSOCKOPT(2)
 * @param set	If true: set the option
 *		if false: unset the option
 */
void
netsocket::set_sock_opt
(
	const int  opt,
	const bool set
)
{
	int result;
	errno = 0;

	if ( set )
	{
		int one = 1;
		#ifdef UL_WIN32
		  result = ::setsockopt ( m_handle,
		    SOL_SOCKET, opt,(char*) & one, sizeof ( one ) );
		#else
		  result = ::setsockopt ( m_handle,
		    SOL_SOCKET, opt, & one, sizeof ( one ) );
		#endif
	}
	else
	{
		result = ::setsockopt ( m_handle, SOL_SOCKET, opt, NULL, 0 );
	}
	if ( result < 0 )
	{
		perror ( "setsockopt:" );
	}
} // netsocket::set_sock_opt ()

//////////////////////////////////////////////////////////////////////

/** Receive packets sent to a broadcast address?
 *
 * Only works on non-stream sockets (UDP).
 *
 * @param broadcast     If true: receive packets to broadcast address
 *                      if false: ignore packets to broadcast address
 */
//////////////////////////////////////////////////////////////////////
void
netsocket::set_broadcast
(
	const bool broadcast
)
{
	int result;

	if ( broadcast )
	{
		int one = 1;
		#ifdef UL_WIN32
		  result = ::setsockopt ( m_handle, SOL_SOCKET,
		    SO_BROADCAST,(char*) & one, sizeof ( one ) );
		#else
		  result = ::setsockopt ( m_handle, SOL_SOCKET,
		    SO_BROADCAST, & one, sizeof ( one ) );
		#endif
	}
	else
	{
		result = ::setsockopt ( m_handle, SOL_SOCKET,
		  SO_BROADCAST, NULL, 0 );
	}
	if ( result < 0 )
	{
		perror ( "set broadcast:" );
	}
} // netsocket::set_broadcast ()

//////////////////////////////////////////////////////////////////////

/** Bind connection to specified @a host and @a port.
 *
 * @param host          The hostname/IP to bind to.
 * @param port          The port to bind to.
 *
 * @return true         On success.
 * @return false        If something went wrong.
 */
bool
netsocket::bind
(
	const std::string& host,
	const uint16_t port
)
{
	netaddr addr ( host, port );
	if ( ::bind ( m_handle, addr.sock_addr(), addr.size() ) == 0 )
	{
		return ( true );
	}
	perror ( "bind" );
	return ( false );
} // netsocket::bind ()

//////////////////////////////////////////////////////////////////////

/** Create a socket which listens on all interfaces
 *
 * @param port          The port to bind to.
 * @param type          specify wether to use TCP or UDP
 *
 * @return true         On success.
 * @return false        If something went wrong.
 */
void
netsocket::listen_all
(
	const uint16_t port,
	const SOCKET_TYPES type
)
{
	int socktype;
	int family = AF_INET6; // try v6 first
	sys_sock* sa;
	socklen_t len;

	if ( type == TCP )
	{
		socktype = SOCK_STREAM;
	}
	else
	{
		socktype = SOCK_DGRAM;
	}
	m_handle = ::socket ( family, socktype, 0 );
	if ( m_handle < 0 )
	{
		family = AF_INET;
		m_handle = ::socket ( family, socktype, 0 );
		if ( m_handle < 0 )
		{
			throw std::runtime_error (
			  "netsocket::listen_all(): could not create socket"
			);
		}
	}
	switch ( family )
	{
	case AF_INET6:
		struct sockaddr_in6 sin6;
		sa  = (sys_sock*) & sin6;
		len = sizeof ( sin6 );
		memset ( &sin6, 0, sizeof(sin6) );
		sin6.sin6_family = AF_INET6;
		sin6.sin6_addr = in6addr_any;
		sin6.sin6_port = net_encode_uint16 ( port );
		break;
	case AF_INET:
		struct sockaddr_in sin4;
		sa  = (sys_sock*) & sin4;
		len = sizeof ( sin4 );
		memset ( &sin4, 0, sizeof(sin4) );
		sin4.sin_family = AF_INET;
		sin4.sin_addr.s_addr = 0;
		sin4.sin_port = net_encode_uint16 ( port );
		break;
	}
	if ( ::bind ( m_handle, sa, len ) < 0 )
	{
		throw std::runtime_error (
		  "netsocket::listen_all(): could not bind"
		);
	}
	if ( ::getsockname ( m_handle, sa, &len ) < 0 )
	{
		throw std::runtime_error (
		  "netsocket::listen_all(): getsockname"
		);
	}
	if ( type == TCP )
	{
		if ( ! listen ( 5 ) )
		{
			throw std::runtime_error (
			  "netsocket::listen_all(): could not listen"
			);
		}
		m_is_stream = true;
	}
} // netsocket::listen_all ()

//////////////////////////////////////////////////////////////////////

/** Make *this netsocket listen to the address/hostname specified
 * by @c host on the specified @c port
 *
 * @param host          The hostname/IP to bind to.
 * 			Can be 'any' or emtpy, in which case the socket
 * 			will listen on all interfaces and all ip
 * 			addresses.
 * @param port          The port to bind to.
 * @param type          specify wether to use TCP or UDP
 *
 * @return true         On success.
 * @return false        If something went wrong.
 */
void
netsocket::listen_to
(
	const std::string& host,
	const uint16_t port,
	const SOCKET_TYPES type
)
{
	if ( ( host == "" ) || ( host == "any" ) )
	{
		try
		{
			listen_all ( port, type );
		}
		catch ( std::runtime_error& e )
		{
			throw e;
		}
		return;
	}
	struct addrinfo  hints;
	struct addrinfo* res;
	struct addrinfo* rp;
	int n;

	memset ( &hints, 0, sizeof(struct addrinfo) );
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	if ( type == TCP )
	{
		hints.ai_socktype = SOCK_STREAM;
	}
	else
	{
		hints.ai_socktype = SOCK_DGRAM;
	}
	n = getaddrinfo ( host.c_str(),
	  num_to_str( port ).c_str(), &hints, &res );
	if ( n != 0 )
	{
		throw std::runtime_error ( gai_strerror(n) );
	}
	rp = res;
	do
	{
		m_handle = ::socket ( rp->ai_family, rp->ai_socktype, 0 );
		if ( m_handle < 0 )
		{
			continue;
		}
		set_blocking ( false );
		set_sock_opt ( SO_REUSEADDR, true );
		if ( ::bind ( m_handle, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break; // success
		}
		close ();
		rp = rp->ai_next;
	} while ( rp != 0 );
	if ( rp == 0 )
	{
		throw std::runtime_error (
		  "netsocket::listen_to: could not find a suitable socket"
		);
	}
	if ( type == TCP )
	{
		if ( ! listen ( 5 ) )
		{
			throw std::runtime_error (
			  "netsocket::listen_to: could not listen"
			);
		}
		m_is_stream = true;
	}
	freeaddrinfo ( rp );
} // netsocket::listen_to ()

//////////////////////////////////////////////////////////////////////

/** Listen to current connection.
 *
 * @param backlog       See listen(2)
 *
 * @return true         Success.
 * @return false        Something is wrong, check @a errno.
 */
bool
netsocket::listen
(
	const int backlog
)
{
	if ( ::listen ( m_handle, backlog ) == 0 )
	{
		return ( true );
	}
	return ( false );
} // netsocket::listen ()

//////////////////////////////////////////////////////////////////////

/** Accept new connections.
 *
 * @param Addr          The internet address of the new client.
 *
 * @return >0           Filedescriptor of the new accepted connection
 * @return <=0          Something went wrong, check @a errno.
 */
int
netsocket::accept
(
	netaddr* addr
) const
{
	if ( addr == 0 )
	{
		return ::accept ( m_handle, NULL, NULL );
	}
	socklen_t size = addr->size();
	return ::accept ( m_handle, addr->sock_addr(), &size );
} // netsocket::accept ()

//////////////////////////////////////////////////////////////////////

/** Connect to given @a host on @a port.
 *
 * @param host          Hostname/IP of client to connect to
 * @param port          Port to connect to.
 *
 * @return true         Connect succeeded.
 * @return false        Something went wrong, check @a errno
 */
bool
netsocket::connect
(
	const std::string& host,
	const uint16_t port,
	const SOCKET_TYPES type
)
{
	struct addrinfo  hints;
	struct addrinfo* res;
	struct addrinfo* rp;
	int n;

	memset ( &hints, 0, sizeof(struct addrinfo) );
	hints.ai_family = AF_UNSPEC;
	if ( type == TCP )
		hints.ai_socktype = SOCK_STREAM;
	else
		hints.ai_socktype = SOCK_DGRAM;
	n = getaddrinfo ( host.c_str(), num_to_str( port ).c_str(),
	  &hints, &res );
	if ( n != 0 )
	{
		throw std::runtime_error ( gai_strerror(n) );
	}
	rp = res;
	do
	{
		m_handle = ::socket ( rp->ai_family, rp->ai_socktype, 0 );
		if ( m_handle < 0 )
			continue;
		if ( ::connect ( m_handle, rp->ai_addr, rp->ai_addrlen) == 0)
			break; // success
		close ();
		rp = rp->ai_next;
	} while ( rp != 0 );
	if ( rp == 0 )
		return false; // no suitable socket found
	return ( true );
} // netsocket::connect ()

//////////////////////////////////////////////////////////////////////

/** Send data via current connection.
 *
 * @param buffer        The data to send.
 * @param size          The number of bytes to send.
 * @param flags         See send(2) for a description
 *
 * @return >0           Number of bytes actually sent.
 * @return -1           Something went wrong, check @a errno
 */
//////////////////////////////////////////////////////////////////////
int
netsocket::send
(
	const void* buffer,
	const int size,
	const int flags
)
{
	char* p       = (char*) buffer;
	int   left    = size;
	int   written = 0;

	while ( left > 0 )
	{
		written = ::send ( m_handle, p, left, flags );
		if ( written == SOCKET_ERROR )
		{
			if ( RECOVERABLE_ERROR )
			{
				written = 0;
				usleep ( 5000 );
			}
			else
			{
				return -1;
			}
		}
		left -= written;
		p    += written;
	}
	return size;
} // netsocket::send ()

//////////////////////////////////////////////////////////////////////

/** Send a string via current connection.
 *
 * @param msg           The message to send.
 * @param flags         See socket(2) for a description
 *
 * @return >0           Number of bytes actually sent.
 * @return -1           Something went wrong, check @a errno
 */
int
netsocket::send
(
	const std::string& msg,
	const int flags
)
{
	return send ( ( const void* ) msg.c_str(), msg.length(), flags );

} // netsocket::send ()

//////////////////////////////////////////////////////////////////////

/** Send a single character via current connection.
 *
 * @param c	The character to send.
 *
 * @return >0   Number of bytes actually sent.
 * @return -1   Something went wrong, check @a errno
 */
int
netsocket::send
(
	const char& C
)
{
	return ::send ( m_handle, & C, 1, 0 );
} // netsocket::send ()

//////////////////////////////////////////////////////////////////////

/** send data to the specified %netaddr.
 *
 * @param buffer        The data to send.
 * @param size          The number of bytes to send.
 * @param to            The %netaddr of the destined receiver.
 * @param flags         See socket(2) for a description
 *
 * @return >0           Number of bytes actually sent.
 * @return -1           Something went wrong, check @a errno
 */
//////////////////////////////////////////////////////////////////////
int
netsocket::send_to
(
	const void*	buffer,
	const int	size,
	const netaddr&	to,
	const int	flags
)
{
	char* p       = (char*) buffer;
	int   left    = size;
	int   written = 0;
	while ( left > 0 )
	{
		written = ::sendto ( m_handle, p, left, flags,
		  to.sock_addr(), to.size()
		);
		if ( written == SOCKET_ERROR )
		{
			if ( RECOVERABLE_ERROR )
			{
				written = 0;
				usleep ( 5000 );
			}
			else
			{
				return -1;
			}
		}
		left -= written;
		p    += written;
	}
	return size;
} // netsocket::send_to ()

//////////////////////////////////////////////////////////////////////

/** Send a string to the specified %netaddr.
 *
 * @param msg           The message to send.
 * @param to            The %netaddr of the destined receiver.
 * @param flags         See socket(2) for a description
 *
 * @return >0           Number of bytes actually sent.
 * @return -1           Something went wrong, check @a errno
 */
//////////////////////////////////////////////////////////////////////
int
netsocket::send_to
(
	const std::string&	msg,
	const netaddr&	to,
	const int	flags
)
{
	return send_to ( ( const void* ) msg.c_str(), msg.length(), to, flags );
} // netsocket::send_to ()

//////////////////////////////////////////////////////////////////////

/** Receive a single character over the current connection.
 *
 * @return the character read
 */
int
netsocket::recv_char
(
	unsigned char& c
)
{
	int n;
	while ( 1 )
	{
		n = ::recv ( m_handle, ( char* ) &c, 1, 0 );
		if ( n == SOCKET_ERROR )
		{
			if ( RECOVERABLE_ERROR )
			{
				continue;
			}
			return -1;
		}
		return n;
	}
	return n;
} // netsocket::recv_char ()

//////////////////////////////////////////////////////////////////////

/** Receive data over the current socket.
 *
 * @param buffer        The data is stored in this buffer.
 * @param size          The buffer has a capacity of this size.
 * @param flags         see recv(2) for a description
 *
 * @return >0           The number of bytes received.
 * @return -1           Something went wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
int
netsocket::recv
(
	void*     buffer,
	const int size,
	const int flags
)
{
	return ::recv ( m_handle, ( char* ) buffer, size, flags );
} // netsocket::recv ()

//////////////////////////////////////////////////////////////////////

/** Receive data over the current socket.
 *
 * @param buffer        The data is stored in this buffer.
 * @param flags         see recv(2) for a description
 *
 * @return >0           The number of bytes received.
 * @return -1           Something went wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
int
netsocket::recv
(
	netpacket& buffer,
	const int  flags
)
{
	int read_bytes = ::recv ( m_handle,
	  ( char* ) buffer.buffer(), buffer.capacity(), flags );
	buffer.set_used ( read_bytes );
	return ( read_bytes );
} // netsocket::recv ()

//////////////////////////////////////////////////////////////////////

/** Receive data over the current socket.
 *
 * @param buffer        The data is stored in this buffer.
 * @param size          The buffer has a capacity of this size.
 * @param flags         See recv(2) for a description.
 * @param from          The senders IP is strored here.
 *
 * @return >0           The number of bytes received.
 * @return -1           Something went wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
int
netsocket::recv_from
(
	void*     buffer,
	const int size,
	netaddr&  from,
	const int flags
)
{
	socklen_t addr_size = from.size();

	int read_bytes = ::recvfrom ( m_handle, ( char* ) buffer,
	  size, flags, from.sock_addr(), & addr_size );
	return ( read_bytes );
} // netsocket::recv_from ()

//////////////////////////////////////////////////////////////////////

/** Receive data over the current socket.
 *
 * @param buffer        The data is stored in this buffer.
 * @param from          The senders IP is strored here.
 * @param flags         See recv(2) for a description.
 *
 * @return >0           The number of bytes received.
 * @return -1           Something went wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
int
netsocket::recv_from
(
	netpacket& buffer,
	netaddr&   from,
	const int  flags
)
{
	socklen_t addr_size = from.size();

	int read_bytes = ::recvfrom ( m_handle, ( char* ) buffer.buffer(),
	  buffer.capacity(), flags, from.sock_addr(), & addr_size );
	buffer.set_used ( read_bytes );
	return ( read_bytes );
} // netsocket::recv_from ()

//////////////////////////////////////////////////////////////////////

/** Close the socket, if active.
 */
void
netsocket::close
()
{
	if ( m_handle != -1 )
	{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
		errno = 0;
		if ( ::close ( m_handle ) != 0 )
		{
			printf ( "netsocket::close: %s (%u)\n",
				 strerror ( errno ), m_handle );
		}
#else
                if ( m_handle > 0 )
                        ::closesocket ( m_handle );
#endif
		m_handle = -1;
	}
} // netsocket::close ()

//////////////////////////////////////////////////////////////////////

/** Forcibly shut down the socket.
 */
//////////////////////////////////////////////////////////////////////
void
netsocket::shutdown
()
{
	if ( m_handle != -1 )
	{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
		if ( m_is_stream )
		{
			errno = 0;
			if ( ::shutdown ( m_handle, SHUT_RDWR ) != 0 )
			{
				printf ( "netsocket::shutdown: %s on fd %u\n",
					 strerror ( errno ), m_handle );
			}
		}
#endif
		m_handle = -1 ;
	}
} // netsocket::shutdown ()

//////////////////////////////////////////////////////////////////////

/**
 * If an error occured, you can check if it was caused due to
 * non-blocking socket operations.
 *
 * @return true         Error was in cause of non-blocking operation
 * @return false        Error was something else.
 */
bool
netsocket::is_non_blocking_error
()
{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
	switch ( errno )
	{
	case EWOULDBLOCK: // always == NET_EAGAIN?
	case EALREADY:
	case EINPROGRESS:
	{
		return true;
	}
	}
	return false;
#else
	int wsa_errno = WSAGetLastError();
	if ( wsa_errno != 0 )
	{
		WSASetLastError ( 0 );
		switch ( wsa_errno )
		{
		case WSAEWOULDBLOCK: // always == NET_EAGAIN?
		case WSAEALREADY:
		case WSAEINPROGRESS:
		{
			return true;
		}
		}
	}
	return false;
#endif
} // netsocket::is_non_blocking_error ()

//////////////////////////////////////////////////////////////////////

/**
 * Wait for a number of @a netsockets to change state, i.e. there
 * is data to read, or the socket is ready to send.
 *
 * @param reads         Array of %netsocket to receive data from
 * @param writes        Array of %netsocket to send date
 * @param timeout       The method returns after @a timeout seconds
 *                      regardless of state changes.
 *
 * @return >0           The number of filedescriptors which changed
 *                      state
 * @return -1           An error occured.
 * @return -2           timeout
 */
int
netsocket::select
(
	netsocket** reads,
	netsocket** writes,
	const int   timeout
)
{
	fd_set  read_set;
	fd_set  write_set;
	int     retval;
	FD_ZERO ( & read_set );
	FD_ZERO ( & write_set );
	int i;
	int num = 0;

	if ( reads )
	{
		for ( i=0; reads[i]; i++ )
		{
			int FD = reads[i]->handle();
			FD_SET ( FD, & read_set );
			num++;
		}
	}
	if ( writes )
	{
		for ( i=0; writes[i]; i++ )
		{
			int FD = writes[i]->handle();
			FD_SET ( FD, & write_set );
			num++;
		}
	}
	if ( !num )
	{
		return num ; // nothing to do
	}
	// It bothers me that select()'s first argument does not appear to
	// work as advertised... [it hangs like this if called with
	// anything less than FD_SETSIZE, which seems wasteful?]
	// Note: we ignore the 'exception' fd_set - I have never had a
	// need to use it.  The name is somewhat misleading - the only
	// thing I have ever seen it used for is to detect urgent data -
	// which is an unportable feature anyway.
	if ( timeout != 0 )
	{
		struct timeval TV ;
		TV.tv_sec  = timeout;
		TV.tv_usec = 0;
		retval = ::select ( FD_SETSIZE,
		  & read_set, & write_set, 0, & TV );
	}
	else
	{
		retval = ::select ( FD_SETSIZE,
		  & read_set, & write_set, 0, 0 );
	}
	if ( retval == 0 )
	{
		return ( -2 ); // timeout
	}
	if ( retval == -1 )
	{
		return ( -1 ); // error
	}
	//remove sockets that had no activity
	num = 0 ;
	if ( reads )
	{
		for ( i=0; reads[i]; i++ )
		{
			int FD = reads[i]->handle();
			if ( ! FD_ISSET ( FD, & read_set ) )
			{
				reads[i] = 0;
			}
			else
			{
				num++;
			}
		}
		reads[i] = NULL ;
	}
	if ( writes )
	{
		for ( i=0; writes[i]; i++ )
		{
			int FD = writes[i]->handle();
			if ( ! FD_ISSET ( FD, & write_set ) )
			{
				writes[i] = 0;
			}
			else
			{
				num++;
			}
		}
		writes[i] = NULL ;
	}
	return num;
} // netsocket::select ()

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

