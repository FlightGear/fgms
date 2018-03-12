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
 * @file	netaddr.cxx
 * @author	Oliver Schroeder
 * @date	07/2017
 */

#ifdef _MSC_VER
	#include <WinSock2.h>
#else
	#include <netdb.h>      // getaddrinfo()
	#include <arpa/inet.h>  // inet_ntop()
	#include <sys/un.h>	// unix domain sockets
#endif
#include <assert.h>	// memset() et al.
#include <string.h>	// memset() et al.
#include <iostream>
#include <fglib/encoding.hxx>
#include <fglib/netaddr.hxx>
#include <fglib/debug.hxx>

namespace fgmp
{

/*
 * int		sin_family
 * uint16	sin_port
 * [4]		sin_addr
 * size 16 bytes
 */
using sys_sock4 = ::sockaddr_in;

/*
 * int		sin6_family
 * uint16	sin6_port
 * int32	sin6_flowinfo
 * [16]		sin6_addr
 * int32	sin6_scope_id
 * size 28 bytes
 */
using sys_sock6 = ::sockaddr_in6;

#ifndef _MSC_VER
/*
 * int		sun_family
 * [108]	sun_path
 * size 128 bytes
 */
using sys_unix = ::sockaddr_un;
#endif

//////////////////////////////////////////////////////////////////////

/** Construct an empty \c netaddr
 *
 * The allocated buffer is large enough to hold any type
 * of sockets addresses, but gets reallocated when
 * netaddr::assign() is called.
 */
netaddr::netaddr
()
{
	DEBUG_TRACE
	m_addr = 0;
	allocate ( netaddr::ANY );
} // netaddr::netaddr()

//////////////////////////////////////////////////////////////////////

/** Construct an empty \c netaddr of the provided family
 */
netaddr::netaddr
(
	FAMILY family
)
{
	DEBUG_TRACE
	m_addr = 0;
	allocate ( family );
} // netaddr::netaddr()

//////////////////////////////////////////////////////////////////////

/** Construct from a given hostname and port
 *
 * Convenient method to construct a netaddr without the need of
 * a complex resolver.
 * 
 * @param host	Can be either a hostname or IP-Address
 * @param port	A port to be used, eg. 443 (HTTPS)
 * @see netaddr::assign
 */
netaddr::netaddr
(
	const std::string& host,
	uint16_t port
)
{
	DEBUG_TRACE
	m_addr = 0;
	assign ( host, port ) ;
} // netaddr::netaddr(host,port)

//////////////////////////////////////////////////////////////////////

/** Construct from a system provided struct sockaddr
 */
netaddr::netaddr
(
	const sys_sock& addr
)
{
	DEBUG_TRACE
	m_addr = 0;
	assign ( addr );
} // netaddr::netaddr(netaddr)

//////////////////////////////////////////////////////////////////////

/** Copy constructor
 *
 * @note using a std::shared_ptr for \c m_addr might be faster
 *       than using a copy constructor
 */
netaddr::netaddr
(
	const netaddr& addr
)
{
	DEBUG_TRACE
	m_addr = 0;
	allocate ( addr.m_addr->sa_family );
	DEBUG_OUT ("Cpy from: " << addr.m_addr << " to " << m_addr );
	memcpy ( m_addr, addr.m_addr,addr.size () );
} // netaddr::netaddr(netaddr)

//////////////////////////////////////////////////////////////////////

/** Free the memory block used to store *this address
 */
netaddr::~netaddr
()
{
	DEBUG_TRACE
	if ( m_addr != 0 )
	{
		DEBUG_OUT("deleting ptr " << m_addr);
		delete m_addr;
	}
	m_addr = 0;
} // netaddr::~netaddr(netaddr)

//////////////////////////////////////////////////////////////////////

/** Check if *this is a valid socket address
 *
 * @return true
 * @return false
 */
bool
netaddr::is_valid
() const
{
	if ( m_addr == 0 )
		return false;
	if ( m_addr->sa_family == AF_UNSPEC )
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////

/** Allocate a memory block to store the address
 */
void
netaddr::allocate
(
	int family
)
{
	DEBUG_TRACE
	if ( m_addr > 0 )
	{
		DEBUG_OUT("deleting ptr " << m_addr);
		delete m_addr;
	}
	switch (family)
	{
	case AF_INET:
		DEBUG_OUT("allocating " << sizeof ( sys_sock4 ) << " bytes");
		m_addr = reinterpret_cast<sys_sock*> (new sys_sock4);
		break;
	case AF_INET6:
		DEBUG_OUT("allocating " << sizeof ( sys_sock6 ) << " bytes");
		m_addr = reinterpret_cast<sys_sock*> (new sys_sock6);
		break;
	case AF_UNIX:
	case AF_UNSPEC:
	default:
		DEBUG_OUT("allocating " << MAXSOCKADDR << " bytes");
		m_addr = reinterpret_cast<sys_sock*> (new char[MAXSOCKADDR]);
	}
	DEBUG_OUT("allocated ptr " << m_addr);
	m_addr->sa_family = family;
	clear ();
} // netaddr::netaddr()

//////////////////////////////////////////////////////////////////////

/** Clear the memory block
 */
void
netaddr::clear
()
{
	DEBUG_TRACE
	if ( m_addr == 0 )
		return;
	memset ( m_addr, 0, size () );
} // netaddr::netaddr()

//////////////////////////////////////////////////////////////////////

/** Initialise by a given hostname and port.
 *
 * Convenient method to construct a netaddr without the need of
 * a complex resolver.
 * 
 * @param host	Can be either a hostname or IP-Address
 * @param port	A port to be used, eg. 443 (HTTPS)
 * @attention	This does not work for unix domain sockets
 */
void
netaddr::assign
(
	const std::string& host,
	uint16_t port
)
{
	DEBUG_TRACE
	struct addrinfo  hints;
	struct addrinfo* rp;

	memset ( &hints, 0, sizeof(struct addrinfo) );
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	int s;
	if ( ( host == "any" ) || ( host == "" ) )
	{
		hints.ai_flags = AI_PASSIVE;
		s = getaddrinfo ( "::0", "500", &hints, &rp );
	}
	else
	{
		s = getaddrinfo ( host.c_str(), "500", &hints, &rp );
	}
	if (s != 0)
	{
		//throw std::runtime_error ( gai_strerror(s) );
                m_addr = 0;
                return;
	}
	if ( rp->ai_family == AF_INET )
	{
		assign ( *rp->ai_addr );
		sys_sock4* s = (sys_sock4*) m_addr;
		s->sin_port  = NET_encode_uint16 ( port );
	}
	else if ( rp->ai_family == AF_INET6 )
	{
		assign ( *rp->ai_addr );
		sys_sock6* s = (sys_sock6*) m_addr;
		s->sin6_port = NET_encode_uint16 ( port );
	}
	else
	{
		std::cout << "family: " << rp->ai_family << std::endl;
		//throw std::runtime_error (
		//  "netaddr::assign(): unsupported addr family"
		//);
                m_addr = 0;
                return;
	}
	freeaddrinfo ( rp );
} // netaddr::assign(host,port)

//////////////////////////////////////////////////////////////////////

/** Initialise by a system provided struct sockaddr.
 *
 * Convenient for initialising a netaddr by values returned
 * from other system functions.
 */
void
netaddr::assign
(
	const sys_sock& addr
)
{
	DEBUG_TRACE
	if ( m_addr == 0 )
	{
		allocate ( addr.sa_family );
	}
	else if ( m_addr > 0 )
	{
		if ( addr.sa_family != m_addr->sa_family )
			allocate ( addr.sa_family );
	}
	DEBUG_OUT ("assign to " << m_addr);
	if ( addr.sa_family == AF_INET )
	{
		sys_sock4* src = (sys_sock4*) & addr;
		sys_sock4* dst = (sys_sock4*) m_addr;
		dst->sin_family = src->sin_family;
		dst->sin_port   = src->sin_port;
		dst->sin_addr.s_addr = src->sin_addr.s_addr;
	}
	else if ( addr.sa_family == AF_INET6 )
	{
		sys_sock6* src = (sys_sock6*) & addr;
		sys_sock6* dst = (sys_sock6*) m_addr;
		uint64_t*  s = (uint64_t* ) &src->sin6_addr;
		uint64_t*  d = (uint64_t* ) &dst->sin6_addr;
		dst->sin6_family      = src->sin6_family;
		dst->sin6_port        = src->sin6_port;
		dst->sin6_flowinfo    = src->sin6_flowinfo;
		dst->sin6_scope_id    = src->sin6_scope_id;
		d[0] = s[0];
		d[1] = s[1];
	}
#ifndef _MSC_VER
	else if ( addr.sa_family == AF_UNIX )
	{
		sys_unix* src = (sys_unix*) & addr;
		sys_unix* dst = (sys_unix*) m_addr;
		dst->sun_family = src->sun_family;
		strcpy ( dst->sun_path, src->sun_path );
	}
#endif
	else
	{
		std::cout << "family: " << addr.sa_family << std::endl;
		//throw std::runtime_error (
		//  "netaddr::assign(): unsupported addr family"
		//);
                m_addr = 0;
	}
} // netaddr::assign(sock_addr)

//////////////////////////////////////////////////////////////////////

/** Set the port
 */
void
netaddr::port
(
	uint16_t port
)
{
	DEBUG_TRACE
	assert ( m_addr != 0 );
	sys_sock6* s = (sys_sock6*) m_addr;
	s->sin6_port = NET_encode_uint16 ( port );
} // netaddr::port(port)

//////////////////////////////////////////////////////////////////////

/** Return the port 
 */
uint16_t
netaddr::port
() const
{
	DEBUG_TRACE
	assert ( m_addr != 0 );
	sys_sock6* s = (sys_sock6*) m_addr;
	return NET_decode_uint16 ( s->sin6_port );
} // netaddr::port()

//////////////////////////////////////////////////////////////////////

/** Create a string object representing the socket address
 *
 * @attention	the returned string does not contain the port.
 * @todo	include the port in the string?
 */
const std::string
netaddr::to_string
() const
{
	DEBUG_TRACE
	char	buffer[50];

	if ( m_addr == 0 )
	{
		return std::string ("unset");
	}
	if ( m_addr->sa_family == AF_INET )
	{
		sys_sock4* s = (sys_sock4*) m_addr;
		inet_ntop ( m_addr->sa_family, & s->sin_addr, buffer, 50 );
	}
	else if ( m_addr->sa_family == AF_INET6 )
	{
		sys_sock6* s = (sys_sock6*) m_addr;
		if ( is_mapped_v4() )
		{
			char* d = (char*) & s->sin6_addr;
			uint32_t* ip4 = (uint32_t*) & d[12];
			inet_ntop ( AF_INET, ip4, buffer, 50 );
			return std::string (buffer);
		}
		inet_ntop ( m_addr->sa_family, & s->sin6_addr, buffer, 50 );
	}
#ifndef _MSC_VER
	else if ( m_addr->sa_family == AF_UNIX )
	{
		sys_unix* s = (sys_unix*) m_addr;
		return std::string ( s->sun_path );
	}
#endif
	return std::string (buffer);
} // netaddr::to_string()

//////////////////////////////////////////////////////////////////////

/** Return the address family of the netaddr
 *
 * @return AF_UNSPEC	for empty addresses
 * @return AF_INET	for IPv4 addresses
 * @return AF_INET6	for IPv6 addresses
 * @return AF_UNIX	for unix domain sockets (unix only)
 */
uint32_t
netaddr::family
() const
{
	DEBUG_TRACE
	assert ( m_addr != 0 );
	return m_addr->sa_family;
} // netaddr::family()

//////////////////////////////////////////////////////////////////////

/** Return the number of bytes used to store *this address
 */
uint32_t
netaddr::size
() const
{
	DEBUG_TRACE
	assert ( m_addr != 0 );
	switch (m_addr->sa_family)
	{
	case AF_INET:	return sizeof (sys_sock4);
	case AF_INET6:	return sizeof (sys_sock6);
	case AF_UNIX:
	case AF_UNSPEC:	return MAXSOCKADDR;
	}
	return 0; // throw error?
} // netaddr::family()

//////////////////////////////////////////////////////////////////////

/** Return true if *this address is an ipv4 address mapped in ipv6
 */
bool
netaddr::is_mapped_v4
() const
{
	DEBUG_TRACE
	assert ( m_addr != 0 );
	if ( m_addr->sa_family != AF_INET6 )
		return false;
	sys_sock6* s = (sys_sock6*) m_addr;
	uint32_t* ip = (uint32_t*) & s->sin6_addr;
	if ( ( ip[0] == 0 )
	&&   ( ip[1] == 0 )
	&&   ( ip[2] == NET_encode_uint32 ( 0xffff ) ) )
		return true;
	return false;
} // netaddr::is_mapped_v4()

//////////////////////////////////////////////////////////////////////

/**
 * @name netaddr operators
 * @{
 */

/** compare *this with \c addr
 *
 * @return true		*this and addr are equal
 * @return false	*this and addr are not equal
 */
bool
netaddr::cmp
(
	const netaddr& addr
) const
{
	DEBUG_TRACE

	if ( ( m_addr == 0 ) || ( addr.m_addr == 0 ) )
	{
		return true;
	}
	sys_sock6* sl = (sys_sock6*) m_addr;
	sys_sock6* sr = (sys_sock6*) addr.m_addr;
	if ( m_addr->sa_family == AF_INET )
	{
		if ( addr.is_mapped_v4() )
		{
			char* d = (char*) & sr->sin6_addr;
			uint32_t* ip4 = (uint32_t*) & d[12];
			if ( sl->sin6_flowinfo == *ip4 )
				return true;
			return false;
		}
		// sin6_flowinfo == sin_addr
		if ( sl->sin6_flowinfo == sr->sin6_flowinfo )
			return true;
		return false;
	}
	else if (  m_addr->sa_family == AF_INET6 )
	{
		if ( ( is_mapped_v4() )
		&&   ( addr.m_addr->sa_family == AF_INET ) )
		{
			char* d = (char*) & sl->sin6_addr;
			uint32_t* ip4 = (uint32_t*) & d[12];
			if ( *ip4 == sr->sin6_flowinfo )
				return true;
			return false;
		}
		uint64_t* left  = (uint64_t*) &sl->sin6_addr;
		uint64_t* right = (uint64_t*) &sr->sin6_addr;
		if (( left[0] == right[0] ) 
		&&  ( left[1] == right[1] ))
		{
			return true;
		}
	}
#ifndef _MSC_VER
	else if ( m_addr->sa_family == AF_UNIX )
	{
		sys_unix* left  = (sys_unix*) sl;
		sys_unix* right = (sys_unix*) sr;
		return (strcmp (left->sun_path, right->sun_path) != 0 );
	}
#endif
	return false;
} // netaddr::cmp()

//////////////////////////////////////////////////////////////////////

/** compare two \c netaddrs, return true if equal
 */
bool
netaddr::operator ==
(
	const netaddr& addr
) const
{
	DEBUG_TRACE

	return ( cmp ( addr ) == true );

	if ( m_addr == 0 )
	{
		return false;
	}
	sys_sock6* sl = (sys_sock6*) m_addr;
	sys_sock6* sr = (sys_sock6*) addr.m_addr;
	if ( sl->sin6_family != sr->sin6_family )
	{
		return false;
	}
	if ( sl->sin6_family == AF_INET )
	{
		// sin6_flowinfo == sin_addr
		if ( sl->sin6_flowinfo == sr->sin6_flowinfo )
		{
			return true;
		}
		return false;
	}
	else if ( m_addr->sa_family == AF_INET6 )
	{
		uint64_t* left  = (uint64_t*) &sl->sin6_addr;
		uint64_t* right = (uint64_t*) &sr->sin6_addr;
		if (( left[0] == right[0] ) 
		&&  ( left[1] == right[1] ))
		{
			return true;
		}
	}
#ifndef _MSC_VER
	else if ( m_addr->sa_family == AF_UNIX )
	{
		sys_unix* left  = (sys_unix*) sl;
		sys_unix* right = (sys_unix*) sr;
		return (strcmp (left->sun_path, right->sun_path) == 0 );
	}
#endif
	return false;
} // netaddr::operator netaddr == netaddr

//////////////////////////////////////////////////////////////////////

/** compare two \c netaddrs, return false if equal
 */
bool
netaddr::operator !=
(
	const netaddr& addr 
) const
{
	DEBUG_TRACE

	return ( cmp ( addr ) == false );

	if ( m_addr == 0 )
	{
		return false;
	}
	sys_sock6* sl = (sys_sock6*) m_addr;
	sys_sock6* sr = (sys_sock6*) addr.m_addr;
	if ( sl->sin6_family != sr->sin6_family )
	{
		return true;
	}
	if ( sl->sin6_family == AF_INET )
	{
		// sin6_flowinfo == sin_addr
		if ( sl->sin6_flowinfo != sr->sin6_flowinfo )
		{
			return true;
		}
		return false;
	}
	else if ( sl->sin6_family == AF_INET6 )
	{
		uint64_t* left  = (uint64_t*) &sl->sin6_addr;
		uint64_t* right = (uint64_t*) &sr->sin6_addr;
		if (( left[0] != right[0] ) 
		||  ( left[1] != right[1] ))
		{
			return true;
		}
	}
#ifndef _MSC_VER
	else if ( m_addr->sa_family == AF_UNIX )
	{
		sys_unix* left  = (sys_unix*) sl;
		sys_unix* right = (sys_unix*) sr;
		return (strcmp (left->sun_path, right->sun_path) != 0 );
	}
#endif
	return false;
} // netaddr::operator netaddr != netaddr

//////////////////////////////////////////////////////////////////////

/** compare two \c netaddrs, return true if
 * left address is less than right address
 */
bool
netaddr::operator <
(
	const netaddr& addr 
) const
{
	DEBUG_TRACE

	if ( m_addr == 0 )
	{
		return false;
	}
	sys_sock6* sl = (sys_sock6*) m_addr;
	sys_sock6* sr = (sys_sock6*) addr.m_addr;
	if ( sl->sin6_family != sr->sin6_family )
	{
		return false;
	}
	if ( sl->sin6_family == AF_INET )
	{
		// sin_addr = sin6_flowinfo
		uint32_t l = XDR_decode_uint32 (sl->sin6_flowinfo);
		uint32_t r = XDR_decode_uint32 (sr->sin6_flowinfo);
		if ( l < r )
		{
			return true;
		}
	}
	else if ( sl->sin6_family == AF_INET6 )
	{
		uint64_t* l = (uint64_t*) &sl->sin6_addr;
		uint64_t* r = (uint64_t*) &sr->sin6_addr;
		uint64_t left  = XDR_decode_uint64 (l[0]);
		uint64_t right = XDR_decode_uint64 (r[0]);
		if ( left < right ) 
		{
			return true;
		}
		if ( left == right ) // the upper part is equal
		{	// so compare the lower part too
			if ( XDR_decode_uint64(l[1]) < XDR_decode_uint64(r[1]) )
				return true;
		}
	}
#ifndef _MSC_VER
	else if ( m_addr->sa_family == AF_UNIX )
	{
		sys_unix* left  = (sys_unix*) sl;
		sys_unix* right = (sys_unix*) sr;
		return (strcmp (left->sun_path, right->sun_path) < 0 );
	}
#endif
	return false;
} // netaddr::operator netaddr < netaddr

//////////////////////////////////////////////////////////////////////

/** compare two \c netaddrs, return true if
 * left address is greater than right address
 */
bool
netaddr::operator >
(
	const netaddr& addr 
) const
{
	DEBUG_TRACE

	if ( m_addr == 0 )
	{
		return false;
	}
	sys_sock6* sl = (sys_sock6*) m_addr;
	sys_sock6* sr = (sys_sock6*) addr.m_addr;
	if ( sl->sin6_family != sr->sin6_family )
	{
		return false;
	}
	if ( sl->sin6_family == AF_INET )
	{
		// sin_addr = sin6_flowinfo
		uint32_t l = XDR_decode_uint32 (sl->sin6_flowinfo);
		uint32_t r = XDR_decode_uint32 (sr->sin6_flowinfo);
		if ( l > r )
		{
			return true;
		}
	}
	else if ( sl->sin6_family == AF_INET6 )
	{
		uint64_t* l = (uint64_t*) &sl->sin6_addr;
		uint64_t* r = (uint64_t*) &sr->sin6_addr;
		uint64_t left  = XDR_decode_uint64 (l[0]);
		uint64_t right = XDR_decode_uint64 (r[0]);
		if ( left > right ) 
		{
			return true;
		}
		if ( left == right ) // the upper part is equal
		{	// so compare the lower part too
			if ( XDR_decode_uint64(l[1]) > XDR_decode_uint64(r[1]) )
				return true;
		}
	}
#ifndef _MSC_VER
	else if ( m_addr->sa_family == AF_UNIX )
	{
		sys_unix* left  = (sys_unix*) sl;
		sys_unix* right = (sys_unix*) sr;
		return (strcmp (left->sun_path, right->sun_path) > 0 );
	}
#endif
	return false;
} // netaddr::operator netaddr > netaddr

//////////////////////////////////////////////////////////////////////

/** Assign another netaddr
 */
void
netaddr::operator =
(
	const netaddr& addr
)
{
	DEBUG_TRACE
	if ( m_addr == 0 )
	{
		allocate ( addr.family() );
	}
	memcpy ( m_addr, addr.m_addr,addr.size () );
} // netaddr::operator = netaddr

//////////////////////////////////////////////////////////////////////

/** Assign a system provided  netaddr
 */
void
netaddr::operator =
(
	const sys_sock& addr
)
{
	DEBUG_TRACE
	assign ( addr );
} // netaddr::operator = sys_sock

//////////////////////////////////////////////////////////////////////

/** Put the netaddr on a stream, this includes the port
 */
std::ostream&
operator <<
(
	std::ostream& o,
	const netaddr& addr
)
{
	DEBUG_TRACE
	o << addr.to_string();
	if ( addr.m_addr == 0 )
	{
		return o;
	}
	if ( ( addr.m_addr->sa_family == AF_INET )
	||   ( addr.m_addr->sa_family == AF_INET6 ) )
	{
		sys_sock6* a = (sys_sock6*) addr.m_addr;
		o << ":" << NET_decode_uint16 ( a->sin6_port );
	}
	return o;
} // operator ostream << netaddr

//////////////////////////////////////////////////////////////////////

/** @} */

} // namespace fgmp

