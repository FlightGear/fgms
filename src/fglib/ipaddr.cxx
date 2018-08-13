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
 * @file	ipaddr.cxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	07/2017
 */

#if !defined(NDEBUG) && defined(_MSC_VER)
	// If MSVC, turn on ALL debug if Debug build
	#define SG_DEBUG_ALL
#endif // MSVC Debug config

#include <list>
#include <ctype.h>      // toupper()
#ifdef _MSC_VER
	#include <WinSock2.h>
#else
	#include <netdb.h>      // gethostbyname()
	#include <arpa/inet.h>  // htons()
#endif
#include <fglib/encoding.hxx>
#include <fglib/ipaddr.hxx>
#include <fglib/fg_util.hxx>
#include <fglib/debug.hxx>

namespace fgmp
{

//////////////////////////////////////////////////////////////////////
/**
 * standard constructor. Calls @a init
 * @see ipaddr::init
 */
//////////////////////////////////////////////////////////////////////
ipaddr::ipaddr
()
{
	m_error       = E_OK;
	init ();
} // ipaddr::ipaddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * A ipaddr can be constructed as a copy of another ipaddr.
 * @param Addr The %ipaddr to copy from
 */
//////////////////////////////////////////////////////////////////////
ipaddr::ipaddr
(
	const ipaddr& Addr
)
{
	m_error       = E_OK;
	init ();
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		m_addr[i]     = Addr.m_addr[i];
		m_mask_addr[i] = Addr.m_mask_addr[i];
	}
	m_error       = Addr.m_error;
	m_type        = Addr.m_type;
	m_mask        = Addr.m_mask;
} // ipaddr::ipaddr ( const ipaddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * A ipaddr can be constructed from a hostname and a port number.
 * @param Addr a hostname or string representation of an IP address
 */
//////////////////////////////////////////////////////////////////////
ipaddr::ipaddr
(
	const std::string& Addr
)
{
	m_error = E_OK;
	from_string ( Addr.c_str() );
} // ipaddr::ipaddr ( const string& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
ipaddr::assign
(
	unsigned int ip
)
{
	init();
	int x = 1;
	if ( * ( ( char* ) &x ) != 0 )
	{
		// little endian, swap bytes
		ip = ( ( ip>>8 ) & 0x00FF00FFL ) | ( ( ip<< 8 ) & 0xFF00FF00L );
		ip = ( ip >> 16 ) | ( ip << 16 );
	}
	m_type = ipaddr::IPv4;
	m_mask = 32;
	uint32_t* left  = (uint32_t*) & m_addr;
	uint32_t* right = (uint32_t*) & ip;
	left = right;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
ipaddr::assign
(
	const std::string& IP
)
{
	m_error = E_OK;
	from_string ( IP );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
ipaddr::assign
(
	const ipaddr& Addr
)
{
	m_error = E_OK;
	init ();
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		m_addr[i]     = Addr.m_addr[i];
		m_mask_addr[i] = Addr.m_mask_addr[i];
	}
	m_error       = Addr.m_error;
	m_type        = Addr.m_type;
	m_mask        = Addr.m_mask;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Convert a struct sockaddr* to our internal
 * Format.
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::assign
(
	struct sockaddr* src_addr
)
{
	struct sockaddr* sock = ( struct sockaddr* ) src_addr;
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		m_addr[i] = 0;
	}
	if ( sock->sa_family == AF_INET )
	{
		// IPv4
		m_mask  = MaxMaskIPv4;  // always host address
		m_type  = IPv4;
		m_error = E_OK;
		struct sockaddr_in* sock_v4 = ( struct sockaddr_in* ) src_addr;
		memcpy ( & m_addr, & ( sock_v4->sin_addr ), 4 );
	}
	else if ( sock->sa_family == AF_INET6 )
	{
		// IPv6
		m_mask  = MaxMask;  // always host address
		m_type  = IPv6;
		m_error = E_OK;
		struct sockaddr_in6* sock_v6 = ( struct sockaddr_in6* ) src_addr;
		memcpy ( & m_addr, & ( sock_v6->sin6_addr ), 16 );
	}
} // ipaddr::assign (sockaddr)

//////////////////////////////////////////////////////////////////////

void
ipaddr::resolve
(
	const std::string& Host
)
{
	m_error = E_OK;
	// FIXME: replace by getaddrinfo
	struct hostent* hp = gethostbyname ( Host.c_str() );
	if ( hp != NULL )
	{
		memcpy ( ( char* ) &m_addr, hp->h_addr, hp->h_length );
		if ( hp->h_addrtype == AF_INET )
		{
			m_mask = 32;
			m_type = ipaddr::IPv4;
		}
		if ( hp->h_addrtype == AF_INET6 )
		{
			m_mask = 128;
			m_type = ipaddr::IPv6;
		}
	}
	else
	{
		init ();
		m_error = E_CouldNotResolve;
	}
} // ipaddr::resolve ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Inititiales member variables. Creates an 'empty' address.
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::init
()
{
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		m_addr[i] = 0;
	}
	m_type        = ipaddr::ANY;
	m_mask        = ipaddr::MaxMask;
	m_mask_addr[0] = 0;
} // ipaddr::init ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Set internal error number.
 * @param m_error internal %ERROR_CODE
 * @see ERROR_CODE
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::set_error
(
	const ERROR_CODE error
)
{
	m_error = error;
	m_type  = ipaddr::Invalid;
	return;
} // ipaddr::set_error ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return a message describing an error
 * @see set_error
 */
//////////////////////////////////////////////////////////////////////
std::string
ipaddr::get_error_msg
() const
{
	switch ( m_error )
	{
	case E_OK:
		return ( "no error occured!" );
	case E_AddressIsEmpty:
		return ( "address string is empty!" );
	case E_IllegalChars:
		return ( "illegal characters in address string!" );
	case E_NetmaskDecimal:
		return ( "netmask must be in decimal notation!" );
	case E_NetmaskMissing:
		return ( "expected netmask, but there is none!" );
	case E_OutOfRange:
		return ( "netmask is out of range!" );
	case E_WrongIPv4:
		return ( "wrong format in IPv4 address!" );
	case E_WrongIPv6:
		return ( "wrong format in IPv6 address!" );
	case E_MissingValue:
		return ( "no value between dots" );
	case E_ExpansionOnlyOnce:
		return ( "::-expansion is allowed only once!" );
	case E_CouldNotResolve:
		return ( "could not resolve hostname!" );
	default:
		return ( "ipaddr::get_error_msg: unexpected error " );
	} // switch ()
	// never reached
	return ( "" );
} // ipaddr::set_error ()
//////////////////////////////////////////////////////////////////////

/** @name ipaddr operators */
/** @{ */

//////////////////////////////////////////////////////////////////////
/** ipaddr increment operator
 * Increment address by 1
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::operator ++
(
	int w
)
{
	int     i;
	int     nResult;
	int     nOverflow;
	i = ipaddr::Size-1;
	nOverflow = 1;
	while ( ( nOverflow != 0 ) && ( i>=0 ) )
	{
		nResult = m_addr[i] + nOverflow;
		nOverflow = 0;
		if ( nResult > 0xff )
		{
			nOverflow = nResult - 0xff;
			nResult   = 0x00;
		}
		m_addr[i] = nResult;
		i--;
	}
} // ipaddr::operator ++ int
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** ipaddr increment operator
 * Decrement address by 1
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::operator --
(
	int w
)
{
	int     i;
	int     nResult;
	int     nOverflow;
	i = ipaddr::Size-1;
	nOverflow = 1;
	while ( ( nOverflow != 0 ) && ( i>=0 ) )
	{
		nResult = m_addr[i] - nOverflow;
		nOverflow = 0;
		if ( nResult > 0xff )
		{
			nOverflow = nResult - 0xff;
			nResult   = 0x00;
		}
		m_addr[i] = nResult;
		i--;
	}
} // ipaddr::operator -- int
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** ipaddr assignment operator
 * assign another %ipaddr.
 *
 * @param Addr the address to copy values from
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::operator =
(
	const ipaddr& Addr
)
{
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		m_addr[i] = Addr.m_addr[i];
	}
	m_type = Addr.m_type;
	m_mask = Addr.m_mask;
} // ipaddr::operator = ipaddr
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** ipaddr comparison OPERATOR
 * compare *this with Addr. Two addresses are equal, if all bits of
 * their address, all bits of their mask and the port are equal.
 *
 * @param Addr the ipaddr to compare to
 * @return 1 if *this is equal to @a Addr
 * @return 0 if *this is not equal to @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::operator ==
(
	const ipaddr& Addr
) const
{
	for ( int i = 0; i < ipaddr::Size; i++ )
	{
		if ( m_addr[i] != Addr.m_addr[i] )
		{
			return ( false );
		}
	}
	if ( m_mask != Addr.m_mask )
	{
		return ( false );
	}
	return ( true );
} // ipaddr::operator == ( const ipaddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** ipaddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the ipaddr to compare to
 * @return 1 if *this is not equal to @a Addr
 * @return 0 if *this is equal to @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::operator !=
(
	const ipaddr& Addr
) const
{
	for ( int i = 0; i < ipaddr::Size; i++ )
	{
		if ( m_addr[i] != Addr.m_addr[i] )
		{
			return ( true );
		}
	}
	if ( m_mask != Addr.m_mask )
	{
		return ( true );
	}
	return ( false );
} // ipaddr::operator != ( const ipaddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** ipaddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the ipaddr to compare to
 * @return 1 if *this is less than @a Addr
 * @return 0 if *this is not less than @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::operator <
(
	const ipaddr& Addr
) const
{
	for ( int i = 0; i < ipaddr::Size; i++ )
	{
		if ( m_addr[i] < Addr.m_addr[i] )
		{
			return ( true );
		}
	}
	return ( false );
} // ipaddr::operator < ( const ipaddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** ipaddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the ipaddr to compare to
 * @return 1 if *this is less or equal than @a Addr
 * @return 0 if *this is not less or equal than @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::operator <=
(
	const ipaddr& Addr
) const
{
	for ( int i = 0; i < ipaddr::Size; i++ )
	{
		if ( m_addr[i] > Addr.m_addr[i] )
		{
			return ( false );
		}
	}
	return ( true );
} // ipaddr::operator <= ( const ipaddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** ipaddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the ipaddr to compare to
 * @return 1 if *this is greater than @a Addr
 * @return 0 if *this is not greater than @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::operator >
(
	const ipaddr& Addr
) const
{
	for ( int i = 0; i < ipaddr::Size; i++ )
	{
		if ( m_addr[i] > Addr.m_addr[i] )
		{
			return ( true );
		}
	}
	return ( false );
} // ipaddr::operator > ( const ipaddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** ipaddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the ipaddr to compare to
 * @return 1 if *this is greater or equal than @a Addr
 * @return 0 if *this is not greater or equal than @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::operator >=
(
	const ipaddr& Addr
) const
{
	for ( int i = 0; i < ipaddr::Size; i++ )
	{
		if ( m_addr[i] < Addr.m_addr[i] )
		{
			return ( false );
		}
	}
	return ( true );
} // ipaddr::operator >= ( const ipaddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** ipaddr assignment operator
 * assign a string representation of an IP address.
 *
 * @param Addr a string representation of an IP address.
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::operator =
(
	const std::string& Addr
)
{
	from_string ( Addr );
} // ipaddr::operator = string
//////////////////////////////////////////////////////////////////////

/** @} */

//////////////////////////////////////////////////////////////////////
/**
 * @return The number of bits for hosts of current netmask.
 */
//////////////////////////////////////////////////////////////////////
uint32_t
ipaddr::host_bits
() const
{
	if ( ( m_type & IPv6 ) == 0 )
	{
		// must be IPv4 only
		return ( MaxMaskIPv4 - m_mask );
	}
	return ( MaxMask - m_mask );
} // host_bits()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @return The number of hosts of current netmask.
 * only works for IPv4!
 */
//////////////////////////////////////////////////////////////////////
uint32_t
ipaddr::number_of_hosts
() const
{
	uint32_t nI = net_bits();
	uint32_t nM = 0;
	for ( uint32_t i=0; i<nI; i++ )
	{
		nM |= ( 0x80000000 >> i );
	}
	return ( ( 0xffffffff ^ nM ) + 1 );
} // host_bits()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @return The address as umeric
 * only works for IPv4!
 */
//////////////////////////////////////////////////////////////////////
uint32_t
ipaddr::addr_num
() const
{
	uint32_t nM;
	nM  = m_addr[0] << 24;
	nM |= m_addr[1] << 16;
	nM |= m_addr[2] << 8;
	nM |= m_addr[3];
	return ( nM );
} // host_bits()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @return The number of bits for net of current netmask.
 */
//////////////////////////////////////////////////////////////////////
uint32_t
ipaddr::net_bits
() const
{
	return ( m_mask );
} // net_bits()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Check if @a Addr is part of *this address
 *
 * @param Addr the ipaddr to check
 * @return true if @a Addr is part of *this address
 * @return false if @a Addr is not part of *this address
 * @see ipaddr::is_part_of
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::contains
(
	const ipaddr& Addr
) const
{
	int      i;
	ipaddr First, Last;
	if ( Addr.m_mask < m_mask )
	{
		return ( false );
	}
	First = first_addr();
	Last  = last_addr();
	for ( i = 0; i < ipaddr::Size; i++ )
	{
		if ( ( Addr.m_addr[i] < First.m_addr[i] )
				||  ( Addr.m_addr[i] > Last.m_addr[i] ) )
		{
			return ( false );
		}
	}
	return ( true );
} // ipaddr::contains ( const ipaddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Check if *this address is part of @a Addr
 *
 * @param Addr the ipaddr to check
 * @return true if @a Addr is part of *this address
 * @return false if @a Addr is not part of *this address
 * @see ipaddr::contains
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::is_part_of
(
	const ipaddr& Addr
) const
{
	int      i;
	ipaddr First, Last;
	if ( m_mask < Addr.m_mask )
	{
		return ( false );
	}
	First = Addr.first_addr();
	Last  = Addr.last_addr();
	for ( i = 0; i < ipaddr::Size; i++ )
	{
		if ( ( m_addr[i] < First.m_addr[i] )
		||   ( m_addr[i] > Last.m_addr[i] ) )
		{
			return ( false );
		}
	}
	return ( true );
} // ipaddr::is_part_of ( const ipaddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Check if *this address is a Loopback IP
 *
 * @return true if *this is a Loopback IP
 * @return false if *this is not a Loopback IP
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::is_loopback
() const
{
	if ( m_type == ipaddr::IPv4 )
	{
		if ( m_addr[0] == 0x7F )
			return true;
	}
	else if ( m_type == ipaddr::IPv6 )
	{
		uint64_t* ip = (uint64_t*) & m_addr[0];
		if ((  ip[0] == 0 )
		&& ( ip[1] == net_encode_uint64 ( 1 ) ) )
			return true;
	}
	return false;
} // ipaddr::is_loopback ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** Return true if *this address is an ipv4 address mapped in ipv6
 */
//////////////////////////////////////////////////////////////////////
bool
ipaddr::is_mapped_v4
() const
{
	uint32_t* ip = (uint32_t*) & m_addr[0];
	if ( ( ip[0] == 0 )
	&&   ( ip[1] == 0 )
	&&   ( ip[2] == net_encode_uint32 ( 0xffff ) ) )
		return true;
	return false;
} // ipaddr::is_mapped_v4 ()

//////////////////////////////////////////////////////////////////////

/** Return true if *this address is unspecified ("any")
 */
bool
ipaddr::is_unspecified
() const
{
	uint32_t* ip = (uint32_t*) & m_addr[0];
	if ( ( ip[0] == 0 )
	&&   ( ip[1] == 0 )
	&&   ( ip[2] == 0 )
	&&   ( ip[3] == 0 ) )
		return true;
	return false;
} // ipaddr::is_unspecified()

//////////////////////////////////////////////////////////////////////

/** Return true if *this is a multicast address
 */
bool 
ipaddr::is_multicast
() const
{
	if ( m_type == ipaddr::IPv4 )
		return ( m_addr[0] == 0xe0 );
	return ( m_addr[0] == 0xff );
} // ipaddr::is_multicast()

//////////////////////////////////////////////////////////////////////

/** Return true if *this is a link local address
 */
bool 
ipaddr::is_link_local
() const
{
	if ( m_type == ipaddr::IPv4 )
		return false;
	uint16_t* ip = (uint16_t*) & m_addr[0];
	return ( ip[0] == net_encode_uint16 (0xfe80) );
} // ipaddr::is_link_local()

//////////////////////////////////////////////////////////////////////

/** Return true if *this is a site local address
 *
 * 
 */
bool 
ipaddr::is_site_local
() const
{
	if ( m_type == ipaddr::IPv4 )
		return false;
	uint16_t* ip = (uint16_t*) & m_addr[0];
	return ( ip[0] == net_encode_uint16 (0xfec0) );
} // ipaddr::is_site_local ()

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Convert internal representation to a Posix address structure
 *
 * @return a pointer to a posix address structure (struct sockaddr)
 *         as needed by all standard C functions.
 */
//////////////////////////////////////////////////////////////////////
struct sockaddr*
ipaddr::to_sockaddr
(
	const uint16_t port
) const
{
	struct sockaddr*        sock;
	struct sockaddr_in*     sock_v4;
	struct sockaddr_in6*    sock_v6;

	sock   = ( struct sockaddr* )&         m_sockaddr;
	sock_v4 = ( struct sockaddr_in* )&      m_sockaddr;
	sock_v6 = ( struct sockaddr_in6* )&     m_sockaddr;
	memset ( sock, 0, sizeof ( sockaddr_in6 ) );
	sock_v4->sin_port = htons ( port );
	if ( m_type == ipaddr::ANY )
	{
		sock_v4->sin_family = AF_INET;
		sock_v4->sin_addr.s_addr = INADDR_ANY;
	}
	else if ( m_type == ipaddr::IPv4 )
	{
		sock_v4->sin_family = AF_INET;
		memcpy ( & ( sock_v4->sin_addr ), & m_addr, 4 );
	}
	else
	{
		sock_v6->sin6_family = AF_INET6;
		memcpy ( & ( sock_v6->sin6_addr ), & m_addr, 16 );
	}
	return ( sock );
} // ipaddr::to_sockaddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Convert internal struct sockaddr m_sockaddr to our internal
 * Format.
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::copy_sockaddr
()
{
	struct sockaddr*        sock;
	sock   = ( struct sockaddr* )         &m_sockaddr;
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		m_addr[i] = 0;
	}
	if ( sock->sa_family == AF_INET )
	{
		// IPv4
		m_mask  = MaxMaskIPv4;  // always host address
		m_type  = IPv4;
		m_error = E_OK;
		struct sockaddr_in* sock_v4 = ( struct sockaddr_in* ) &m_sockaddr;
		memcpy ( & m_addr, & ( sock_v4->sin_addr ), 4 );
	}
	else if ( sock->sa_family == AF_INET6 )
	{
		// IPv6
		m_mask  = MaxMask;  // always host address
		m_type  = IPv6;
		m_error = E_OK;
		struct sockaddr_in6* sock_v6 = ( struct sockaddr_in6* ) &m_sockaddr;
		memcpy ( & m_addr, & ( sock_v6->sin6_addr ), 16 );
	}
} // ipaddr::pm_copy_sockaddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the size of our internal struct sockaddr
 *
 * @return The size of our address structure
 */
//////////////////////////////////////////////////////////////////////
socklen_t
ipaddr::size
() const
{
	return ( sizeof ( m_sockaddr ) );
} // ipaddr::size ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Map an IPv4 address to an IPv4inIPv6 address.
 *
 * @return A mapped %ipaddr of *this
 */
//////////////////////////////////////////////////////////////////////
ipaddr
ipaddr::map_to_v6
() const
{
	ipaddr Result;
	if ( ( ( m_type & IPv6 ) > 0 )
	||     ( m_type & IPv4 ) == 0 )
	{
		// no IPv4 address
		Result.set_error ( ipaddr::E_WrongIPv4 );
		return ( Result );
	}
	Result.m_addr[0] = 0x20;
	Result.m_addr[1] = 0x02;
	Result.m_addr[2] = m_addr[0];
	Result.m_addr[3] = m_addr[1];
	Result.m_addr[4] = m_addr[2];
	Result.m_addr[5] = m_addr[3];
	Result.m_mask    = m_mask+16;
	Result.m_type    = IPv6;
	return ( Result );
} // ipaddr::map_to_v6 ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Map an IPv4inIPv6 address to an IPv4 address.
 *
 * @return A mapped %ipaddr of *this
 */
//////////////////////////////////////////////////////////////////////
ipaddr
ipaddr::map_from_v6
() const
{
	ipaddr Result;
	if ( ( ( m_type & IPv6 ) == 0 )
	||  ( ( m_type & IPv4 ) > 0 )
	||  ( m_addr[0] != 0x20 )
	||  ( m_addr[1] != 0x02 ) )
	{
		// no IPv4 address
		Result.set_error ( ipaddr::E_WrongIPv6 );
		return ( Result );
	}
	Result.m_addr[0] = m_addr[2];
	Result.m_addr[1] = m_addr[3];
	Result.m_addr[2] = m_addr[4];
	Result.m_addr[3] = m_addr[5];
	Result.m_mask     = m_mask-16;
	Result.m_type     = IPv4;
	return ( Result );
} // ipaddr::map_from_v6 ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Represent the netmask as an IP address, eg. '255.255.255.0'
 *
 * @return A %ipaddr representation of the netmask
 */
//////////////////////////////////////////////////////////////////////
ipaddr
ipaddr::mask_adde
() const
{
	ipaddr Result ( *this );
	Result.build_from_mask ();
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		Result.m_addr[i] = Result.m_mask_addr[i];
	}
	return ( Result );
} // ipaddr::mask_adde()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Represent the netmask as an IP address, eg. '255.255.255.0'
 *
 * @return A %ipaddr representation of the netmask
 */
//////////////////////////////////////////////////////////////////////
ipaddr
ipaddr::cisco_wildcard
() const
{
	ipaddr Result ( *this );
	Result.build_from_mask ();
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		Result.m_addr[i] = ~Result.m_mask_addr[i];
	}
	return ( Result );
} // ipaddr::cisco_wildcard()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the first address from network. If *this ipaddr is an
 * IPv4 address, the result represents the network address.
 *
 * @return The first address of *this network.
 */
//////////////////////////////////////////////////////////////////////
ipaddr
ipaddr::first_addr
() const
{
	ipaddr Result ( *this );

	if ( m_mask_addr[0] == 0 )
	{
		Result.build_from_mask();
	}
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		Result.m_addr[i] = m_addr[i] & Result.m_mask_addr[i];
	}
	return ( Result );
} // ipaddr::first_addr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the first usable address from network. If *this address
 * is an IPv6 address, the first usable address is the first address
 * of *this network. For IPv4 Addresses it is the successor of the
 * netaddress.
 *
 * @return The first usable address of *this network
 * @see ipaddr::first_addr
 */
//////////////////////////////////////////////////////////////////////
ipaddr
ipaddr::first_usable_addr
() const
{
	ipaddr Result ( *this );
	if ( ( ( m_type & IPv4 ) > 0 ) && ( m_mask == 32 ) )
	{
		return ( Result );
	}
	if ( ( m_type & IPv6 ) > 0 )
	{
		return ( first_addr () );
	}
	Result = first_addr ();
	Result++;
	return ( Result );
} // ipaddr::first_usable_addr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the last address of *this network. For an IPv4 address, the
 * result represents the broadcast address of *this network.
 *
 * @return The last address of *this network
 */
//////////////////////////////////////////////////////////////////////
ipaddr
ipaddr::last_addr
() const
{
	ipaddr Result ( *this );

	if ( m_mask_addr[0] == 0 )
	{
		Result.build_from_mask();
	}
	for ( int i=0; i<ipaddr::Size; i++ )
	{
		Result.m_addr[i] = m_addr[i] | ~Result.m_mask_addr[i];
	}
	Result.m_type     = m_type;
	Result.m_mask     = m_mask;
	Result.m_error    = m_error;
	return ( Result );
}  // ipaddr::last_addr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the last usable address from network. If *this is an IPv6
 * address, the result is the last address of *this network.
 *
 * @return The last usable address of *this network.
 * @see ipaddr::last_addr
 */
//////////////////////////////////////////////////////////////////////
ipaddr
ipaddr::last_usable_addr
() const
{
	ipaddr Result ( *this );
	if ( ( ( m_type & IPv4 ) > 0 ) && ( m_mask == 32 ) )
	{
		return ( Result );
	}
	if ( ( m_type & IPv6 ) > 0 )
	{
		return ( last_addr () );
	}
	Result = last_addr ();
	Result--;
	return ( Result );
} // ipaddr::last_usable_addr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Create m_mask_addr from m_mask
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::build_from_mask
()
{
	uint32_t i;
	uint32_t nBytes;
	uint32_t nBits;
	uint32_t nBitMask;

	for ( i=0; i < ipaddr::Size; i++ )
	{
		m_mask_addr[i] = 0;
	}
	nBits = m_mask;
	nBytes = ( m_mask / 8 );
	for ( i=0; i < nBytes; i++ )
	{
		m_mask_addr[i] = 0xff;
		nBits -= 8;
	}
	nBytes = i;
	nBitMask = 0x80;
	for ( i=0; i < nBits; i++ )
	{
		m_mask_addr[nBytes] |= ( nBitMask >> i );
	}
} // ipaddr::build_from_mask ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Convert *this address into a string representation.
 *
 * @param nFormat the format of the string representation
 * @param nBase   the base of the string representation.
 *                (Eg. Base=SEDECIMAL will result in ff.ff.ff.0)
 * @return        a string representation of *this address
 * @see ADDRESS_FORMAT
 * @see ADDRESS_BASE
 */
//////////////////////////////////////////////////////////////////////
std::string
ipaddr::to_string
(
	const ADDRESS_BASE nBase,
	const ADDRESS_FORMAT nFormat
) const
{
	char buffer[50];
	std::string ret;

	if ( m_type == IPv4 )
	{
		inet_ntop ( AF_INET, &m_addr, buffer, 50 );
	}
	else if ( m_type == IPv6 )
	{
		inet_ntop ( AF_INET6, &m_addr, buffer, 50 );
	}
	else if ( m_type == IPv4inIPv6 )
	{
		inet_ntop ( AF_INET, &m_addr[12], buffer, 50 );
	}
	return buffer;

#if 0
	int             nNumBytes;
	int             nRealBytes;
	int             nOutputBytes;
	ADDRESS_BASE    nWantBase;
	uint32_t            nOctet;
	bool            bWantFill;
	bool            bDidCompression;
	bool            bDoOutput;
	string          strReturn;
	if ( nBase == ipaddr::STD )
	{
		nWantBase = ipaddr::SEDECIMAL;
	}
	else
	{
		nWantBase = nBase;
	}
	nRealBytes      = 0;
	nOutputBytes    = ipaddr::Size;
	bDidCompression = false;
	if ( nFormat != ipaddr::Compressed )
	{
		bDoOutput = true;
	}
	else
	{
		bDoOutput = false;
	}
	if ( ( m_type & IPv4 ) > 0 )
	{
		nOutputBytes -= 4;
	}
	if ( ( m_type & IPv6 ) == 0 )
	{
		nOutputBytes = 0;
	}
	nNumBytes = 0;
	while ( nNumBytes < nOutputBytes )
	{
		// addr is v6
		nOctet = ( m_addr[nNumBytes] << 8 );
		nNumBytes++;
		nOctet += m_addr[nNumBytes];
		nNumBytes++;
		//////////////////////////////////////////////////
		//
		//      insert leading zeros if necessary
		//
		//////////////////////////////////////////////////
		bWantFill = false;
		if ( ( nRealBytes > 0 ) || ( nFormat == ipaddr::Full ) )
		{
			bWantFill = true;
		}
		if ( ( nFormat == ipaddr::Short ) && ( nOctet == 0 ) )
		{
			bWantFill = false;
		}
		if ( ( nOctet != 0 ) || ( bWantFill == true )
		||  ( bDoOutput == true ) )
		{
			strReturn += word_to_string ( nOctet, nWantBase, bWantFill );
			nRealBytes += 2;
		}
		if ( ( ( nNumBytes ) % 2 ) == 0 )
		{
			if ( ( nRealBytes == 0 ) && ( bDidCompression == false ) )
			{
				strReturn += ":";
				bDidCompression = true;
			}
			if ( ( nNumBytes < ipaddr::Size ) && ( nRealBytes > 0 ) )
			{
				strReturn += ":";
				nRealBytes = 0;
			}
		}
	} // while()
	if ( ( m_type & IPv4 ) > 0 )
	{
		//////////////////////////////////////////////////
		//
		//      output of last 4 bytes
		//
		//////////////////////////////////////////////////
		if ( nBase == ipaddr::STD )
		{
			nWantBase = ipaddr::DECIMAL;
			bWantFill = false;
		}
		else
		{
			nWantBase = nBase;
			bWantFill = true;
		}
		for ( int i=12; i<ipaddr::Size; i++ )
		{
			strReturn += byte_to_string ( m_addr[i], nWantBase, bWantFill );
			if ( i<15 ) // last byte
			{
				strReturn += '.';
			}
		}
	}
	return ( strReturn );
#endif
} // ipaddr::to_string ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Scan a given string if it is a valid IPv[4|6] address.\n
 * Calling this function we know:\n
 *      - if the address is syntactically correct\n
 *      - how many octets are in the string\n
 *      - how many octets we have to insert in an expansion (::)\n
 *      - type of address (m_type gets set)\n
 *
 * @param Addr  A string representation of an IP address.
 * @return      The number of octest in the string.
 */
//////////////////////////////////////////////////////////////////////
#if 0
uint32_t
ipaddr::prescan_str
(
	const string& Addr
)
{
	uint32_t    nNumOctets;             // number of octets in string
	uint32_t    nNumColons;             // number of colons in string
	uint32_t    nNumDots;               // number of dots in string
	uint32_t    nLastColon;             // position of last colon
	uint32_t    nLastDot;               // position of last dot
	bool    bIsCompressed;          // "::" in string
	bool    bHaveMask;              // "/" in string
	uint32_t    i;
	m_type          = ipaddr::Invalid;// assume address is not valid
	nNumColons      = 0;
	nNumDots        = 0;
	nNumOctets      = 0;
	nLastColon      = 0;
	nLastDot        = 0;
	bIsCompressed   = false;
	bHaveMask       = false;
	if ( Addr.size() == 0 )
	{
		set_error ( ipaddr::E_AddressIsEmpty );
		return ( 0 );
	}
	for ( i=0; i < Addr.size(); i++ )
	{
		if ( Addr[i] == ':' )
		{
			if ( ( m_type & IPv4 ) > 0 )
			{
				// already seen dots, no more : allowed
				set_error ( ipaddr::E_WrongIPv6 );
				return ( 0 );
			}
			m_type = ipaddr::IPv6;
			if ( ( i-nLastColon ) == 1 )
			{
				if ( bIsCompressed )
				{
					// already found ::
					set_error ( ipaddr::E_ExpansionOnlyOnce );
					return ( 0 );
				}
				bIsCompressed = true;
			}
			else if ( i>0 )
			{
				nNumOctets++;
			}
			nLastColon = i;
			nLastDot = i;
			nNumColons++;
		}
		else if ( Addr[i] == '.' )
		{
			m_type = ipaddr::IPv4;
			if ( ( i-nLastDot ) == 1 )
			{
				set_error ( ipaddr::E_MissingValue );
				return ( 0 );
			}
			else if ( ( i-nLastDot ) > 4 )
			{
				set_error ( ipaddr::E_WrongIPv4 );
				return ( 0 );
			}
			nLastDot = i;
			nNumDots++;
		}
		else if ( Addr[i] == '/' )
		{
			if ( ( ( m_type & IPv4 ) != 0 ) && ( ( i - nLastDot ) > 4 ) )
			{
				set_error ( ipaddr::E_WrongIPv4 );
				return ( 0 );
			}
			else if ( ( ( m_type & IPv6 ) !=0 ) && ( ( i-nLastDot ) > 5 ) )
			{
				set_error ( ipaddr::E_WrongIPv6 );
				return ( 0 );
			}
			if ( i+1 == Addr.size() )
			{
				set_error ( ipaddr::E_NetmaskMissing );
				return ( 0 );
			}
			if ( ( Addr[i-1] != ':' ) && ( ( m_type & IPv4 ) == 0 ) )
			{
				nNumOctets++;
			}
			bHaveMask = true;
		}
		else if ( ( bHaveMask == true )
				&& ( ( Addr[i] < '0' ) || ( Addr[i] > '9' ) ) )
		{
			set_error ( ipaddr::E_NetmaskDecimal );
			return ( 0 );
		}
		else if ( ( ( Addr[i] < '0' ) || ( Addr[i] > '9' ) )
		&&   ( ( Addr[i] < 'a' ) || ( Addr[i] > 'f' ) )
		&&   ( ( Addr[i] < 'A' ) || ( Addr[i] > 'F' ) ) )
		{
			set_error ( ipaddr::E_IllegalChars );
			return ( 0 );
		}
	} // for ()
	if ( m_type == ipaddr::Invalid )
	{
		set_error ( ipaddr::E_WrongIPv6 );
		return ( 0 );
	}
	if ( ( ( m_type & IPv4 ) != 0 ) && ( bHaveMask == false )
			&&  ( ( i - nLastDot ) > 3 ) )
	{
		set_error ( ipaddr::E_WrongIPv4 );
		return ( 0 );
	}
	else if ( ( ( m_type & IPv6 ) != 0 ) && ( bHaveMask == false )
			&&  ( ( i - nLastDot ) > 5 ) )
	{
		set_error ( ipaddr::E_WrongIPv6 );
		return ( 0 );
	}
	//////////////////////////////////////////////////
	//      if the string contained no mask
	//      the last octet was not counted
	//////////////////////////////////////////////////
	if ( ( bHaveMask == false ) && ( Addr[Addr.size() -1] != ':' ) )
	{
		if ( ( m_type & IPv4 ) == 0 )
		{
			nNumOctets++;
		}
	}
	if ( ( nNumDots != 0 ) && ( nNumDots != 3 ) )
	{
		set_error ( ipaddr::E_WrongIPv4 );
		return ( 0 );
	}
	if ( nNumColons > 7 )
	{
		set_error ( ipaddr::E_WrongIPv6 );
		return ( 0 );
	}
	if ( ( nNumColons == 6 ) && ( nNumDots != 3 ) )
	{
		set_error ( ipaddr::E_WrongIPv6 );
		return ( 0 );
	}
	if ( ( nNumOctets == 0 ) && ( nNumDots == 0 ) )
	{
		set_error ( ipaddr::E_AddressIsEmpty );
		return ( 0 );
	}
	return ( nNumOctets );
} // ipaddr::prescan_str ( const string& Addr, uint32_t& nNumOctets )
//////////////////////////////////////////////////////////////////////
#endif

//////////////////////////////////////////////////////////////////////
/**
 * Convert a string representation of an IP address to our internal
 * Format.
 *
 * @param Addr  A string representation of an IP address
 */
//////////////////////////////////////////////////////////////////////
void
ipaddr::from_string
(
	const std::string& addr
)
{
	bool is_ipv6  = false;
	int have_mask = 0;
	
	char* p = (char*) addr.c_str();
	for ( unsigned i = 0; i < addr.size(); i++ )
	{
		if ( p[i] == ':' )
			is_ipv6 = true;
		if ( p[i] == '/' )
			have_mask = i;
	}
	if ( have_mask > 0 )
	{
		p[have_mask] = 0;
		m_mask = atoi ( & p[have_mask+1] );
	}
	if ( is_ipv6 )
	{
		m_type = ipaddr::IPv6;
		if ( inet_pton ( AF_INET6, p, &m_addr ) < 1 )
		{
			set_error ( ipaddr::E_WrongIPv6 );
			return;
		}
		if ( is_mapped_v4 () )
		{
			m_type = ipaddr::IPv4inIPv6;
		}
	}
	else
	{
		m_type = ipaddr::IPv4;
		if ( inet_pton ( AF_INET, p, &m_addr ) < 1 )
		{
			set_error ( ipaddr::E_WrongIPv4 );
			return;
		}
	}
} // ipaddr::from_string ( const char* Addr )
//////////////////////////////////////////////////////////////////////

/** ipaddr output operator
 */
std::ostream&
operator <<
(
	std::ostream& o,
	const ipaddr& Addr
)
{
	o << Addr.to_string();
	return ( o );
} // ostream& operator << ( ipaddr )

} // namespace fgmp

