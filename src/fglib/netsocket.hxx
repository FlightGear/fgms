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
 * @file	netsocket.hxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	07/2017
 */

#ifndef NET_SOCKET_HEADER
#define NET_SOCKET_HEADER

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <fglib/netaddr.hxx>
#include <fglib/netpacket.hxx>

#ifdef _MSC_VER
	static int recoverable_wsa_error()
	{
		int wsa_errno = WSAGetLastError();
		WSASetLastError ( 0 ); // clear the error
		return ( wsa_errno == WSAEINTR ) ? 1 : 0;
	}
	#define RECOVERABLE_ERROR recoverable_wsa_error()
#else
	#define RECOVERABLE_ERROR \
	  (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK )
	#define SOCKET_ERROR -1
#endif

namespace fgmp
{

/** A class for TCP/UDP internet connections.
 */
class netsocket
{
public:
	/** Types of socket connections we support.
	 *
	 * @todo unix domain sockets
	 */
	enum SOCKET_TYPES
	{
		UDP,
		TCP
	};
	netsocket ();
	virtual ~netsocket ();
	void assign ( const netsocket& socket );
	int  handle () const;
	void handle ( int handle );
	bool open ( int family, const SOCKET_TYPES type=TCP );
	void close ();
	void shutdown ();
	bool listen ( const int backlog );
	int  accept ( netaddr* a ) const;
	bool connect ( const std::string& host, const uint16_t port,
			const SOCKET_TYPES type=TCP );
	bool bind ( const std::string& host, const uint16_t port );
	bool listen_all ( const uint16_t port, const SOCKET_TYPES type=TCP )
			throw ( std::runtime_error );
	bool listen_to ( const std::string& host, const uint16_t port,
			 const SOCKET_TYPES type=TCP )
			 throw ( std::runtime_error );
	int  send ( const void* buffer, const int size,
			const int flags = 0 );
	int  send ( const std::string& msg, const int flags = MSG_NOSIGNAL );
	int  send ( const char& c );
	int  send_to ( const void* buffer, const int size,
			const netaddr& to, const int flags = MSG_NOSIGNAL );
	int  send_to ( const std::string& msg,
			const netaddr& to, const int flags = MSG_NOSIGNAL );
	int  recv_char ( unsigned char& c );
	int  recv ( void* buffer, const int size,
			const int flags = MSG_NOSIGNAL );
	int  recv ( NetPacket& buffer, int flags = MSG_NOSIGNAL );
	int  recv_from ( void* buffer, const int size, netaddr& from,
			const int flags = MSG_NOSIGNAL );
	int  recv_from ( NetPacket& buffer, netaddr& from,
			const int flags = MSG_NOSIGNAL );
	void set_blocking ( const bool is_blocking );
	inline bool has_socket()
	{
		return ( m_handle > -1 );
	};
	void set_sock_opt ( const int opt, const bool set );
	/// deprecated
	void set_broadcast ( const bool broadcast );
	static int select ( netsocket** reads, netsocket** writes,
			    const int timeout );
	static bool is_non_blocking_error ();
private:
	int   m_handle;
	bool  m_is_stream;
}; // class netsocket
//////////////////////////////////////////////////////////////////////

} // namespace fgmp

#endif // NET_SOCKET_HEADER

