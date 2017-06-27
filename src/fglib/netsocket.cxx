//
// netsocket.cxx - Provide a class for TCP/UDP internet connections
//
// Copied and modified version of the PLIB Library:
//
//      PLIB - A Suite of Portable Game Libraries
//      Copyright (C) 1998,2002  Steve Baker
//      http://plib.sourceforge.net
//
// This file is part of fgms.
//
// Copyright (C) 2008 Oliver Schroeder <fgms@postrobot.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see
// <http://www.gnu.org/licenses/>.
//

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
#include "netsocket.hxx"

#if defined(UL_MSVC) && !defined(socklen_t)
#       define socklen_t int
#endif

/* Init/Exit functions */
static bool donenetinit = false;

static void netExit ( void )
{
	if ( donenetinit )
	{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
#else
		/* Clean up windows networking */
		if ( WSACleanup() == SOCKET_ERROR )
		{
			if ( WSAGetLastError() == WSAEINPROGRESS )
			{
				WSACancelBlockingCall();
				WSACleanup();
			}
		}
#endif
	}
	donenetinit = false;
}


int netInit()
{
	if ( donenetinit )
	{
		return 0;        // that's ok
	}
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
#else
	/* Start up the windows networking */
	WORD version_wanted = MAKEWORD ( 2, 2 );
	WSADATA wsaData;
	if ( WSAStartup ( version_wanted, &wsaData ) != 0 )
	{
		fprintf ( stderr, "Couldn't initialize Winsock 2.2\n" );
		return ( -1 );
	}
#endif
	atexit ( netExit );
	donenetinit = true;
	return ( 0 );
}


//////////////////////////////////////////////////////////////////////
/**
 * Construct an 'empty' NetSocket.
 */
//////////////////////////////////////////////////////////////////////
NetSocket::NetSocket
()
{
	m_Handle = -1 ;
	m_IsStream = false;
} // NetSocket::NetSocket ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Close active connections.
 */
//////////////////////////////////////////////////////////////////////
NetSocket::~NetSocket
()
{
	Close ();
} // NetSocket::~NetSocket ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Assign another socket to *this
 */
//////////////////////////////////////////////////////////////////////
void
NetSocket::Assign
( const NetSocket& Socket )
{
	m_Handle = Socket.m_Handle;
	m_IsStream = Socket.m_IsStream;
} // NetSocket::Assign ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the filedescriptor of current connection.
 *
 * @return The filedescriptor.
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::GetHandle
() const
{
	return ( m_Handle );
} // NetSocket::GetHandle ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Set the filedescriptor. Close the current conenction, if any.
 */
//////////////////////////////////////////////////////////////////////
void
NetSocket::SetHandle
(
	int Handle
)
{
	Close ();
	m_Handle = Handle;
} // NetSocket::SetHandle ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Create a socket.
 *
 * @param IsStream      If true: connection is a tcp stream
 *                      If false: connection will be udp based
 * @return true         Socket creation succeeded.
 * @return false        Something went wrong
 */
//////////////////////////////////////////////////////////////////////
bool
NetSocket::Open
( const SOCKET_TYPES Type )
{
	/* start up networking */
	if ( netInit() )
	{
		return false;
	}
	Close ();
	if ( Type == NetSocket::TCP )
	{
		m_Handle = ::socket ( AF_INET, SOCK_STREAM, 0 );
		m_IsStream = true;
	}
	else
	{
		m_Handle = ::socket ( AF_INET, SOCK_DGRAM, 0 );
	}
	return ( m_Handle != -1 );
} // NetSocket::Open ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Set connection into blocking mode.
 *
 * @param IsBlocking    If true: set blocking mode
 *                      If false: set non-blocking mode
 */
//////////////////////////////////////////////////////////////////////
void
NetSocket::SetBlocking
( const bool IsBlocking )
{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
	int DelayFlag = ::fcntl ( m_Handle, F_GETFL, 0 );
	if ( IsBlocking )
	{
		DelayFlag &= ( ~O_NDELAY );
	}
	else
	{
		DelayFlag |= O_NDELAY;
	}
	::fcntl ( m_Handle, F_SETFL, DelayFlag );
#else
	u_long nBlocking = IsBlocking? 0: 1;
	::ioctlsocket ( m_Handle, FIONBIO, & nBlocking );
#endif
} // NetSocket::SetBlocking ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Set socket option
 *
 * @param SocketOption  the socket option to (un)set, see GETSOCKOPT(2)
 * @param Broadcast     If true: set the option
 *                      if false: unset the option
 */
//////////////////////////////////////////////////////////////////////
void
NetSocket::SetSocketOption
(
	const int SocketOption,
	const bool Set
)
{
	int Result;
	errno = 0;
	if ( Set )
	{
		int One = 1;
		#ifdef UL_WIN32
		  Result = ::setsockopt ( m_Handle,
		    SOL_SOCKET, SocketOption,(char*) & One, sizeof ( One ) );
		#else
		  Result = ::setsockopt ( m_Handle,
		    SOL_SOCKET, SocketOption, & One, sizeof ( One ) );
		#endif
	}
	else
	{
		Result = ::setsockopt ( m_Handle,
		  SOL_SOCKET, SocketOption, NULL, 0 );
	}
	if ( Result < 0 )
	{
		perror ( "setsockopt:" );
		switch ( errno )
		{
		case EBADF:
			std::cout << " EBADF" << std::endl;
			break;
		case EFAULT:
			std::cout << " EFAULT" << std::endl;
			break;
		case EINVAL:
			std::cout << " EINVAL" << std::endl;
			break;
		case ENOPROTOOPT:
			std::cout << " EPROTO" << std::endl;
			break;
		case ENOTSOCK:
			std::cout << " ENOTSOCK" << std::endl;
			break;
		}
	}
} // NetSocket::SetSocketOption ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Receive packets sent to a broadcast address? Only works on
 * non-stream sockets (UDP).
 *
 * @param Broadcast     If true: receive packets to broadcast address
 *                      if false: ignore packets to broadcast address
 */
//////////////////////////////////////////////////////////////////////
void
NetSocket::SetBroadcast
( const bool Broadcast )
{
	int Result;
	if ( Broadcast )
	{
		int One = 1;
		#ifdef UL_WIN32
		  Result = ::setsockopt ( m_Handle,
		    SOL_SOCKET, SO_BROADCAST,(char*) & One, sizeof ( One ) );
		#else
		  Result = ::setsockopt ( m_Handle,
		    SOL_SOCKET, SO_BROADCAST, & One, sizeof ( One ) );
		#endif
	}
	else
	{
		Result = ::setsockopt ( m_Handle,
		  SOL_SOCKET, SO_BROADCAST, NULL, 0 );
	}
	if ( Result < 0 )
	{
		perror ( "set broadcast:" );
	}
} // NetSocket::SetBroadcast ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Bind connection to specified @a Host and @a Port.
 *
 * @param Host          The hostname/IP to bind to.
 * @param Port          The port to bind to.
 *
 * @return true         On success.
 * @return false        If something went wrong.
 */
//////////////////////////////////////////////////////////////////////
bool
NetSocket::Bind
(
	const string& Host,
	const int Port
)
{
	NetAddr Addr ( Host, Port );
	if ( ::bind ( m_Handle, Addr.SockAddr(), sizeof ( sockaddr_in ) ) == 0 )
	{
		return ( true );
	}
	perror ( "bind" );
	return ( false );
} // NetSocket::Bind ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Listen to current connection.
 *
 * @param Backlog       See listen(2)
 *
 * @return true         Success.
 * @return false        Something is wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
bool
NetSocket::Listen
(
	const int Backlog
)
{
	if ( ::listen ( m_Handle, Backlog ) == 0 )
	{
		return ( true );
	}
	return ( false );
} // NetSocket::Listen ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Accept new connections.
 *
 * @param Addr          The internet address of the new client.
 *
 * @return >0           Filedescriptor of the new accepted connection
 * @return 0            Something went wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::Accept
(
	NetAddr& Addr
)
{
	int nResult;
	if ( Addr.AddrType() == NetAddr::Invalid )
	{
		nResult = ::accept ( m_Handle, NULL, NULL );
		if ( nResult > 0 )
		{
			m_Handle = nResult;
			return ( true );
		}
		return ( 0 );
	}
	socklen_t AddrSize = Addr.AddrSize();
	nResult = ::accept ( m_Handle, Addr.SockAddr(), &AddrSize );
	if ( nResult < 1 )
	{
		return ( 0 );
	}
	Addr.CopySockAddr ();
	return ( nResult );
} // NetSocket::Accept ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Connect to given @a Host on @a Port.
 *
 * @param Host          Hostname/IP of client to connect to
 * @param Port          Port to connect to.
 *
 * @return true         Connect succeeded.
 * @return false        Something went wrong, check @a errno
 */
//////////////////////////////////////////////////////////////////////
bool
NetSocket::Connect
(
	const string& Host,
	const int Port
)
{
	NetAddr Addr ( Host, Port );
	if ( ::connect ( m_Handle, Addr.SockAddr(), Addr.AddrSize() ) == 0 )
	{
		return ( true );
	}
	return ( false );
} // NetSocket::Connect ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Send data via current connection.
 *
 * @param Buffer        The data to send.
 * @param Size          The number of bytes to send.
 * @param Flags         See send(2) for a description
 *
 * @return >0           Number of bytes actually sent.
 * @return -1           Something went wrong, check @a errno
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::Send
(
	const void* Buffer,
	const int Size,
	const int Flags
)
{
	char* p       = (char*) Buffer;
	int   left    = Size;
	int   written = 0;

	while ( left > 0 )
	{
		written = ::send ( m_Handle, p, left, MSG_NOSIGNAL );
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
	return Size;
} // NetSocket::Send ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Send a string via current connection.
 *
 * @param Msg           The message to send.
 * @param Flags         See socket(2) for a description
 *
 * @return >0           Number of bytes actually sent.
 * @return -1           Something went wrong, check @a errno
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::Send
(
	const string& Msg,
	const int Flags
)
{
	return Send ( ( const void* ) Msg.c_str(), Msg.length(), Flags );

} // NetSocket::Send ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Send a single character via current connection.
 *
 * @param Buffer        The data to send.
 * @param Size          The number of bytes to send.
 *
 * @return >0           Number of bytes actually sent.
 * @return -1           Something went wrong, check @a errno
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::Send
(
	const char& C
)
{
	return ::send ( m_Handle, & C, 1, 0 );
} // NetSocket::Send ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Send data to the specified %NetAddr.
 *
 * @param Buffer        The data to send.
 * @param Size          The number of bytes to send.
 * @param To            The %NetAddr of the destined receiver.
 * @param Flags         See socket(2) for a description
 *
 * @return >0           Number of bytes actually sent.
 * @return -1           Something went wrong, check @a errno
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::SendTo
(
	const void* Buffer,
	const int Size,
	const NetAddr& To,
	const int Flags
)
{
	char* p       = (char*) Buffer;
	int   left    = Size;
	int   written = 0;
	while ( left > 0 )
	{
		written = ::sendto ( m_Handle, p, left, MSG_NOSIGNAL,
		  ( struct sockaddr* ) To.SockAddr(), To.AddrSize()
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
	return Size;
} // NetSocket::SendTo ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Send a string to the specified %NetAddr.
 *
 * @param Msg           The message to send.
 * @param Size          The number of bytes to send.
 * @param To            The %NetAddr of the destined receiver.
 * @param Flags         See socket(2) for a description
 *
 * @return >0           Number of bytes actually sent.
 * @return -1           Something went wrong, check @a errno
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::SendTo
(
	const string& Msg,
	const NetAddr& To,
	const int Flags
)
{
	return SendTo ( ( const void* ) Msg.c_str(), Msg.length(), To, Flags );
} // NetSocket::SendTo ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 */
int
NetSocket::RecvChar
(
	unsigned char& c
)
{
	int n;
	while ( 1 )
	{
		n = ::recv ( m_Handle, ( char* ) &c, 1, 0 );
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
} // NetSocket::RecvChar ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Receive data over the current socket.
 *
 * @param Buffer        The data is stored in this buffer.
 * @param Size          The buffer has a capacity of this size.
 * @param Flags         see recv(2) for a description
 *
 * @return >0           The number of bytes received.
 * @return -1           Something went wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::Recv
(
	void* Buffer,
	const int Size,
	const int Flags
)
{
	return ::recv ( m_Handle, ( char* ) Buffer, Size, Flags );
} // NetSocket::Recv ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Receive data over the current socket.
 *
 * @param Buffer        The data is stored in this buffer.
 * @param Flags         see recv(2) for a description
 *
 * @return >0           The number of bytes received.
 * @return -1           Something went wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::Recv
(
	NetPacket& Buffer,
	const int Flags
)
{
	int read_bytes = ::recv ( m_Handle,
	  ( char* ) Buffer.Buffer(), Buffer.Capacity(), Flags );
	Buffer.SetUsed ( read_bytes );
	return ( read_bytes );
} // NetSocket::Recv ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Receive data over the current socket.
 *
 * @param Buffer        The data is stored in this buffer.
 * @param Size          The buffer has a capacity of this size.
 * @param Flags         See recv(2) for a description.
 * @param From          The senders IP is strored here.
 *
 * @return >0           The number of bytes received.
 * @return -1           Something went wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::RecvFrom
(
	void* Buffer,
	const int Size,
	NetAddr& From,
	const int Flags
)
{
	struct sockaddr_in6 SockAddr;
	socklen_t AddrSize = sizeof ( SockAddr );
	int read_bytes = ::recvfrom ( m_Handle,
	  ( char* ) Buffer, Size, Flags,
	  ( sockaddr* ) &SockAddr, &AddrSize );
	From.Assign ( ( sockaddr* ) &SockAddr );
	return ( read_bytes );
} // NetSocket::RecvFrom ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Receive data over the current socket.
 *
 * @param Buffer        The data is stored in this buffer.
 * @param From          The senders IP is strored here.
 * @param Flags         See recv(2) for a description.
 *
 * @return >0           The number of bytes received.
 * @return -1           Something went wrong, check @a errno.
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::RecvFrom
(
	NetPacket& Buffer,
	NetAddr& From,
	const int Flags
)
{
	struct sockaddr_in6 SockAddr;
	socklen_t AddrSize = sizeof ( SockAddr );
	int read_bytes = ::recvfrom ( m_Handle,
	  ( char* ) Buffer.Buffer(), Buffer.Capacity(), Flags,
	  ( sockaddr* ) &SockAddr, &AddrSize );
	Buffer.SetUsed ( read_bytes );
	From.Assign ( ( sockaddr* ) &SockAddr );
	return ( read_bytes );
} // NetSocket::RecvFrom ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Close the socket, if active.
 */
//////////////////////////////////////////////////////////////////////
void
NetSocket::Close
( void )
{
	if ( m_Handle != -1 )
	{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
		errno = 0;
		if ( ::close ( m_Handle ) != 0 )
		{
			printf ( "NetSocket::close: %s (%u)\n",
				 strerror ( errno ), m_Handle );
		}
#else
		::closesocket ( m_Handle );
#endif
		m_Handle = -1;
	}
} // NetSocket::Close ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Forcibly shut down the socket.
 */
//////////////////////////////////////////////////////////////////////
void
NetSocket::Shutdown
( void )
{
	if ( m_Handle != -1 )
	{
#if defined(UL_CYGWIN) || !defined (UL_WIN32)
		if ( m_IsStream )
		{
			errno = 0;
			if ( ::shutdown ( m_Handle, SHUT_RDWR ) != 0 )
			{
				printf ( "NetSocket::shutdown: %s on fd %u\n",
					 strerror ( errno ), m_Handle );
			}
		}
#endif
		m_Handle = -1 ;
	}
} // NetSocket::Shutdown ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * If an error occured, you can check if it was caused due to
 * non-blocking socket operations.
 *
 * @return true         Error was in cause of non-blocking operation
 * @return false        Error was something else.
 */
//////////////////////////////////////////////////////////////////////
bool
NetSocket::IsNonBlockingError
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
} // NetSocket::IsNonBlockingError ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Wait for a number of @a NetSockets to change state, i.e. there
 * is data to read, or the socket is ready to send.
 *
 * @param Reads         Array of %NetSocket to receive data from
 * @param Writes        Array of %NetSocket to send date
 * @param Timeout       The method returns after @a Timeout seconds
 *                      regardless of state changes.
 *
 * @return >0           The number of filedescriptors which changed
 *                      state
 * @return -1           An error occured.
 * @return -2           Timeout
 */
//////////////////////////////////////////////////////////////////////
int
NetSocket::Select
(
	NetSocket** Reads,
	NetSocket** Writes,
	const int Timeout
)
{
	fd_set  ReadSet;
	fd_set  WriteSet;
	int     Retval;
	FD_ZERO ( & ReadSet );
	FD_ZERO ( & WriteSet );
	int I;
	int Num = 0;
	if ( Reads )
	{
		for ( I=0; Reads[I]; I++ )
		{
			int FD = Reads[I]->GetHandle();
			FD_SET ( FD, & ReadSet );
			Num++;
		}
	}
	if ( Writes )
	{
		for ( I=0; Writes[I]; I++ )
		{
			int FD = Writes[I]->GetHandle();
			FD_SET ( FD, & WriteSet );
			Num++;
		}
	}
	if ( !Num )
	{
		// nothing to do
		return Num ;
	}
	// It bothers me that select()'s first argument does not appear to
	// work as advertised... [it hangs like this if called with
	// anything less than FD_SETSIZE, which seems wasteful?]
	// Note: we ignore the 'exception' fd_set - I have never had a
	// need to use it.  The name is somewhat misleading - the only
	// thing I have ever seen it used for is to detect urgent data -
	// which is an unportable feature anyway.
	if ( Timeout != 0 )
	{
		struct timeval TV ;
		TV.tv_sec  = Timeout;
		TV.tv_usec = 0;
		Retval = ::select ( FD_SETSIZE, & ReadSet, & WriteSet, 0, & TV );
	}
	else
	{
		Retval = ::select ( FD_SETSIZE, & ReadSet, & WriteSet, 0, 0 );
	}
	if ( Retval == 0 ) // timeout
	{
		return ( -2 );
	}
	if ( Retval == -1 ) // error
	{
		return ( -1 );
	}
	//remove sockets that had no activity
	Num = 0 ;
	if ( Reads )
	{
		for ( I=0; Reads[I]; I++ )
		{
			int FD = Reads[I]->GetHandle();
			if ( ! FD_ISSET ( FD, & ReadSet ) )
			{
				Reads[I] = 0;
			}
			else
			{
				Num++;
			}
		}
		Reads[I] = NULL ;
	}
	if ( Writes )
	{
		for ( I=0; Writes[I]; I++ )
		{
			int FD = Writes[I]->GetHandle();
			if ( ! FD_ISSET ( FD, & WriteSet ) )
			{
				Writes[I] = 0;
			}
			else
			{
				Num++;
			}
		}
		Writes[I] = NULL ;
	}
	return Num;
} // NetSocket::Select ()
//////////////////////////////////////////////////////////////////////


