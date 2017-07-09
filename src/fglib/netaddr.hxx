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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, U$
//

/**
 * @file	sockaddr.hxx
 * @author	Oliver Schroeder
 * @date	07/2017
 */

#ifndef NETADDR_HEADER
#define NETADDR_HEADER

#include <string>
#include <stdexcept>
#ifdef _MSC_VER
	#include <WinSock2.h>
	#include <Ws2ipdef.h>
	typedef int socklen_t;
#else
	#include <netinet/in.h>
#endif


namespace fgmp
{

/*
 * int		sa_family
 * []		sa_data
 */
typedef struct ::sockaddr	sys_sock;

/**
 * @class netaddr
 * @brief Socket address, internet style.
 *
 * This class handles systems socket addresses
 * in a fast and usable way, we do not need to know
 * the internal structure of socket addresses.
 *
 * This class handles IPv4, IPv6 and Unix domain sockets
 */
	const int MAXSOCKADDR = 128; // unix domain socket
class netaddr
{
	sys_sock* m_addr;
public:
	enum FAMILY
	{
		IPv4 = AF_INET,
		IPv6 = AF_INET6,
		UNIX = AF_UNIX,
		ANY  = AF_UNSPEC
	};
	netaddr ();
	netaddr ( FAMILY family );
	netaddr ( const std::string& host, uint16_t port ) ;
	netaddr ( const sys_sock& addr );
	netaddr ( const netaddr& addr );
	~netaddr ();

	bool is_valid() const;
	void assign ( const std::string& host, uint16_t port );
	void assign ( const sys_sock& addr );
	void resolv ( const std::string& host, const std::string& port );
	void port ( uint16_t port );
	uint16_t port() const ;
	uint32_t family () const ;
	uint32_t size () const;
	void clear ();
	const std::string to_string () const ;
	bool is_mapped_v4 () const;
	bool cmp ( const netaddr& addr ) const;
	void operator =  ( const netaddr& addr );
	void operator =  ( const sys_sock& addr );
	bool operator == ( const netaddr& addr ) const;
	bool operator != ( const netaddr& addr ) const;
	bool operator <  ( const netaddr& Addr ) const;
	bool operator >  ( const netaddr& Addr ) const;
	inline sockaddr* sock_addr () { return &(*m_addr); };
	inline const sockaddr* sock_addr () const { return &(*m_addr); };
	//inline sockaddr* operator & () { return &(*m_addr); };
	//inline const sockaddr* operator & () const { return &(*m_addr); };
	friend std::ostream& operator <<
	  ( std::ostream& o, const netaddr& addr );
private:
	void allocate ( int family );
};

} // namespace fgmp

#endif

