// netsocket.hxx - Provide a class for TCP/UDP internet connections
//
// Copied and modified version of the PLIB Library:
//
//      PLIB - A Suite of Portable Game Libraries
//      Copyright (C) 1998,2002  Steve Baker
//      http://plib.sourceforge.net
//
// This file is part of fgms
//

#ifndef NET_SOCKET_HEADER
#define NET_SOCKET_HEADER

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "netaddr.hxx"
#include "netpacket.hxx"

#ifdef _MSC_VER
static int recoverable_wsa_error()
{
	int wsa_errno = WSAGetLastError();
	WSASetLastError ( 0 ); // clear the error
	return ( wsa_errno == WSAEINTR ) ? 1 : 0;
}
#define RECOVERABLE_ERROR recoverable_wsa_error()
#else
#define RECOVERABLE_ERROR (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK )
#define SOCKET_ERROR -1
#endif

/** A class for TCP/UDP internet connections.
 */
class NetSocket
{
public:
	enum SOCKET_TYPES
	{
		UDP,
		TCP
	};
	NetSocket ();
	virtual ~NetSocket ();
	/// Assign another socket
	void Assign ( const NetSocket& Socket );
	/// Return filedescriptor of current connection.
	int GetHandle () const;
	/// Set a filedescriptor for the current connection.
	/// Set CloseOnExit=true if this NetSocket should close() on exit
	void SetHandle ( int Handle );
	/// Open a new connection.
	bool Open ( const SOCKET_TYPES Type=TCP );
	/// Close an existing connection.
	void Close ( void );
	/// Shutdown current connection.
	void Shutdown ( void );
	/// Bind to given address and port.
	bool Bind ( const string& Host, const int Port );
	/// Listen for new connections.
	bool Listen ( const int Backlog );
	/// Accept new connections.
	int  Accept ( NetAddr& Addr );
	/// Connect to the given host and port.
	bool Connect     ( const string& Host, const int Port );
	/// Send data over the current connection.
	int  Send ( const void* Buffer, const int Size, const int Flags=0 );
	/// Send a string over the current connection.
	int  Send ( const string& Msg, const int Flags=0 );
	/// Send a string over the current connection.
	int  Send ( const char& C );
	/// Send data to the given host and port.
	int  SendTo ( const void* Buffer, const int Size,
		      const NetAddr& To, const int Flags=0 );
	/// Send a string to the given host and port.
	int  SendTo ( const string& Msg,
		      const NetAddr& To, const int Flags=0 );
	/// Receive a single character over the current connection
	int RecvChar ( unsigned char& c );
	/// Receive data over the current connection.
	int Recv ( void* Buffer, const int Size, const int Flags = 0 );
	/// Receive data over the current connection.
	int Recv ( NetPacket& Buffer, int Flags = 0 );
	/// Receive data over the current connection, and record senders IP
	int RecvFrom ( void* Buffer, const int Size, NetAddr& From,
		       const int Flags = 0 );
	/// Receive data over the current connection, and record senders IP
	int RecvFrom ( NetPacket& Buffer, NetAddr& From , const int Flags = 0 );
	/// Set current connection into blocking mode
	void SetBlocking ( const bool Blocking );
	/// return true if socket is valid
	bool HasSocket()
	{
		return ( m_Handle > -1 );
	};
	/// set socket option, wrapper for setsockop()
	void SetSocketOption ( const int SocketOption, const bool Set );
	/// deprecated
	void SetBroadcast ( const bool Broadcast );
	/// wtf?
	static bool IsNonBlockingError ();
	/// static select()
	static int Select ( NetSocket** Reads, NetSocket** Writes,
			    const int Timeout );
private:
	/// The current filedescriptor
	int   m_Handle;
	/// true if *this is a TCP connection
	bool  m_IsStream;
}; // class NetSocket
//////////////////////////////////////////////////////////////////////

#endif // NET_SOCKET_HEADER

