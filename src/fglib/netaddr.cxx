/// @file netaddr.cxx
/// implement a class for IP handling
///
/// @author	Oliver Schroeder <fgms@o-schroeder.de>
/// @date	2005-2015
/// @copyright	GPLv3
///
/// long description here.
///

// Copyright (C) Oliver Schroeder <fgms@postrobot.de>
//
// This file is part of fgms, the flightgear multiplayer server
//
// fgms is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// fgms is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fgms.  If not, see <http://www.gnu.org/licenses/>
//

#if !defined(NDEBUG) && defined(_MSC_VER) // If MSVC, turn on ALL debug if Debug build
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
#include "encoding.hxx"
#include "netaddr.hxx"
#include "fg_util.hxx"

//////////////////////////////////////////////////////////////////////
/**
 * standard constructor. Calls @a Init
 * @see NetAddr::Init
 */
//////////////////////////////////////////////////////////////////////
NetAddr::NetAddr
()
{
	m_Error       = E_OK;
	m_Port        = 0;
	Init ();
} // NetAddr::NetAddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * A NetAddr can be constructed as a copy of another NetAddr.
 * @param Addr The %NetAddr to copy from
 */
//////////////////////////////////////////////////////////////////////
NetAddr::NetAddr
(
	const NetAddr& Addr
)
{
	m_Error       = E_OK;
	Init ();
	for ( int i=0; i<NetAddr::Size; i++ )
	{
		m_Addr[i]     = Addr.m_Addr[i];
		m_MaskAddr[i] = Addr.m_MaskAddr[i];
	}
	m_Error       = Addr.m_Error;
	m_Type        = Addr.m_Type;
	m_Mask        = Addr.m_Mask;
	m_Port        = Addr.m_Port;
} // NetAddr::NetAddr ( const NetAddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * A NetAddr can be constructed from a hostname and a port number.
 * @param Addr a hostname or string representation of an IP address
 * @param Port a port number
 */
//////////////////////////////////////////////////////////////////////
NetAddr::NetAddr
(
	const string& Addr,
	const int Port
)
{
	m_Error       = E_OK;
	m_Port        = Port;
	FromString ( Addr.c_str() );
} // NetAddr::NetAddr ( const string& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
NetAddr::Assign
(
	unsigned int IP,
	const int Port
)
{
	Init();
	int x = 1;
	if ( * ( ( char* ) &x ) != 0 )
	{
		// little endian, swap bytes
		IP = ( ( IP>>8 ) & 0x00FF00FFL ) | ( ( IP<< 8 ) & 0xFF00FF00L );
		IP = ( IP >> 16 ) | ( IP << 16 );
	}
	m_Type = NetAddr::IPv4;
	m_Mask = 32;
	char* C = ( char* ) &IP;
	m_Addr[12] = C[0];
	m_Addr[13] = C[1];
	m_Addr[14] = C[2];
	m_Addr[15] = C[3];
	if ( Port != 0 )
	{
		m_Port     = Port;
	}
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
NetAddr::Assign
(
	const string& IP,
	const int Port
)
{
	m_Error = E_OK;
	FromString ( IP );
	if ( Port != 0 )
	{
		m_Port     = Port;
	}
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
NetAddr::Assign
(
	const NetAddr& Addr
)
{
	m_Error = E_OK;
	Init ();
	for ( int i=0; i<NetAddr::Size; i++ )
	{
		m_Addr[i]     = Addr.m_Addr[i];
		m_MaskAddr[i] = Addr.m_MaskAddr[i];
	}
	m_Error       = Addr.m_Error;
	m_Type        = Addr.m_Type;
	m_Mask        = Addr.m_Mask;
	m_Port        = Addr.m_Port;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Convert a struct sockaddr* to our internal
 * Format.
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::Assign
(
	struct sockaddr* SrcSockAddr
)
{
	struct sockaddr* SockAddr = ( struct sockaddr* ) SrcSockAddr;
	for ( int i=0; i<NetAddr::Size; i++ )
	{
		m_Addr[i] = 0;
	}
	if ( SockAddr->sa_family == AF_INET )
	{
		// IPv4
		m_Mask  = MaxMaskIPv4;  // always host address
		m_Type  = IPv4;
		m_Error = E_OK;
		struct sockaddr_in* SockAddrV4 = ( struct sockaddr_in* ) SrcSockAddr;
		m_Port  = htons ( SockAddrV4->sin_port );
		memcpy ( & m_Addr[12], & ( SockAddrV4->sin_addr ), 4 );
	}
	else if ( SockAddr->sa_family == AF_INET6 )
	{
		// IPv6
		m_Mask  = MaxMask;  // always host address
		m_Type  = IPv6;
		m_Error = E_OK;
		struct sockaddr_in6* SockAddrV6 = ( struct sockaddr_in6* ) SrcSockAddr;
		m_Port  = htons ( SockAddrV6->sin6_port );
		memcpy ( & m_Addr, & ( SockAddrV6->sin6_addr ), 16 );
	}
} // NetAddr::Assign (sockaddr)

//////////////////////////////////////////////////////////////////////

void
NetAddr::SetPort
(
	int Port
)
{
	m_Port  = Port;
} // NetAddr::SetPort ()

//////////////////////////////////////////////////////////////////////

void
NetAddr::Resolve
(
	const string& Host,
	const int Port
)
{
	m_Error = E_OK;
	m_Port  = Port;
	struct hostent* hp = gethostbyname ( Host.c_str() );
	if ( hp != NULL )
	{
		memcpy ( ( char* ) &m_Addr[12], hp->h_addr, hp->h_length );
		if ( hp->h_addrtype == AF_INET )
		{
			m_Mask = 32;
			m_Type = NetAddr::IPv4;
		}
		if ( hp->h_addrtype == AF_INET6 )
		{
			m_Mask = 128;
			m_Type = NetAddr::IPv6;
		}
	}
	else
	{
		Init ();
		m_Error = E_CouldNotResolve;
	}
} // NetAddr::Resolve ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Inititiales member variables. Creates an 'empty' address.
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::Init
()
{
	for ( int i=0; i<NetAddr::Size; i++ )
	{
		m_Addr[i] = 0;
	}
	m_Type        = NetAddr::ANY;
	m_Mask        = NetAddr::MaxMask;
	m_MaskAddr[0] = 0;
} // NetAddr::Init ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Set internal error number.
 * @param nError internal %ERROR_CODE
 * @see ERROR_CODE
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::SetError
(
	const ERROR_CODE nError
)
{
	m_Error = nError;
	m_Type  = NetAddr::Invalid;
	return;
} // NetAddr::SetError ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return a message describing an error
 * @see SetError
 */
//////////////////////////////////////////////////////////////////////
string
NetAddr::GetErrorMsg
() const
{
	switch ( m_Error )
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
		return ( "NetAddr::GetErrorMsg: unexpected error " );
	} // switch ()
	// never reached
	return ( "" );
} // NetAddr::SetError ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr increment operator
 * Increment address by 1
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::operator ++
(
	int w
)
{
	int     i;
	int     nResult;
	int     nOverflow;
	i = NetAddr::Size-1;
	nOverflow = 1;
	while ( ( nOverflow != 0 ) && ( i>=0 ) )
	{
		nResult = m_Addr[i] + nOverflow;
		nOverflow = 0;
		if ( nResult > 0xff )
		{
			nOverflow = nResult - 0xff;
			nResult   = 0x00;
		}
		m_Addr[i] = nResult;
		i--;
	}
} // NetAddr::operator ++ int
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr increment operator
 * Decrement address by 1
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::operator --
(
	int w
)
{
	int     i;
	int     nResult;
	int     nOverflow;
	i = NetAddr::Size-1;
	nOverflow = 1;
	while ( ( nOverflow != 0 ) && ( i>=0 ) )
	{
		nResult = m_Addr[i] - nOverflow;
		nOverflow = 0;
		if ( nResult > 0xff )
		{
			nOverflow = nResult - 0xff;
			nResult   = 0x00;
		}
		m_Addr[i] = nResult;
		i--;
	}
} // NetAddr::operator -- int
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr assignment operator
 * Assign another %NetAddr.
 *
 * @param Addr the address to copy values from
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::operator =
(
	const NetAddr& Addr
)
{
	for ( int i=0; i<NetAddr::Size; i++ )
	{
		m_Addr[i] = Addr.m_Addr[i];
	}
	m_Type = Addr.m_Type;
	m_Mask = Addr.m_Mask;
	m_Port = Addr.m_Port;
} // NetAddr::operator = NetAddr
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr comparison OPERATOR
 * compare *this with Addr. Two addresses are equal, if all bits of
 * their address, all bits of their mask and the port are equal.
 *
 * @param Addr the NetAddr to compare to
 * @return 1 if *this is equal to @a Addr
 * @return 0 if *this is not equal to @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
NetAddr::operator ==
(
	const NetAddr& Addr
) const
{
	for ( int i = 0; i < NetAddr::Size; i++ )
	{
		if ( m_Addr[i] != Addr.m_Addr[i] )
		{
			return ( false );
		}
	}
	if ( m_Mask != Addr.m_Mask )
	{
		return ( false );
	}
	return ( true );
} // NetAddr::operator == ( const NetAddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the NetAddr to compare to
 * @return 1 if *this is not equal to @a Addr
 * @return 0 if *this is equal to @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
NetAddr::operator !=
(
	const NetAddr& Addr
) const
{
	for ( int i = 0; i < NetAddr::Size; i++ )
	{
		if ( m_Addr[i] != Addr.m_Addr[i] )
		{
			return ( true );
		}
	}
	if ( m_Mask != Addr.m_Mask )
	{
		return ( true );
	}
	return ( false );
} // NetAddr::operator != ( const NetAddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the NetAddr to compare to
 * @return 1 if *this is less than @a Addr
 * @return 0 if *this is not less than @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
NetAddr::operator <
(
	const NetAddr& Addr
) const
{
	for ( int i = 0; i < NetAddr::Size; i++ )
	{
		if ( m_Addr[i] >= Addr.m_Addr[i] )
		{
			return ( false );
		}
	}
	return ( true );
} // NetAddr::operator < ( const NetAddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the NetAddr to compare to
 * @return 1 if *this is less or equal than @a Addr
 * @return 0 if *this is not less or equal than @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
NetAddr::operator <=
(
	const NetAddr& Addr
) const
{
	for ( int i = 0; i < NetAddr::Size; i++ )
	{
		if ( m_Addr[i] > Addr.m_Addr[i] )
		{
			return ( false );
		}
	}
	return ( true );
} // NetAddr::operator <= ( const NetAddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the NetAddr to compare to
 * @return 1 if *this is greater than @a Addr
 * @return 0 if *this is not greater than @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
NetAddr::operator >
(
	const NetAddr& Addr
) const
{
	for ( int i = 0; i < NetAddr::Size; i++ )
	{
		if ( m_Addr[i] <= Addr.m_Addr[i] )
		{
			return ( false );
		}
	}
	return ( true );
} // NetAddr::operator > ( const NetAddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr comparison operator.
 * Compare *this with Addr.
 *
 * @param Addr the NetAddr to compare to
 * @return 1 if *this is greater or equal than @a Addr
 * @return 0 if *this is not greater or equal than @a Addr
 */
//////////////////////////////////////////////////////////////////////
bool
NetAddr::operator >=
(
	const NetAddr& Addr
) const
{
	for ( int i = 0; i < NetAddr::Size; i++ )
	{
		if ( m_Addr[i] < Addr.m_Addr[i] )
		{
			return ( false );
		}
	}
	return ( true );
} // NetAddr::operator >= ( const NetAddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NetAddr assignment operator
 * Assign a string representation of an IP address.
 *
 * @param Addr a string representation of an IP address.
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::operator =
(
	const string& Addr
)
{
	FromString ( Addr );
} // NetAddr::operator = string
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @return The number of bits for hosts of current netmask.
 */
//////////////////////////////////////////////////////////////////////
UINT
NetAddr::HostBits
() const
{
	if ( ( m_Type & IPv6 ) == 0 )
	{
		// must be IPv4 only
		return ( MaxMaskIPv4 - m_Mask );
	}
	return ( MaxMask - m_Mask );
} // HostBits()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @return The number of hosts of current netmask.
 * only works for IPv4!
 */
//////////////////////////////////////////////////////////////////////
UINT
NetAddr::NumberOfHosts
() const
{
	UINT nI = NetBits();
	UINT nM = 0;
	for ( UINT i=0; i<nI; i++ )
	{
		nM |= ( 0x80000000 >> i );
	}
	return ( ( 0xffffffff ^ nM ) + 1 );
} // HostBits()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @return The address as umeric
 * only works for IPv4!
 */
//////////////////////////////////////////////////////////////////////
UINT
NetAddr::AddrNum
() const
{
	UINT nM;
	nM  = m_Addr[12] << 24;
	nM |= m_Addr[13] << 16;
	nM |= m_Addr[14] << 8;
	nM |= m_Addr[15];
	return ( nM );
} // HostBits()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @return The number of bits for net of current netmask.
 */
//////////////////////////////////////////////////////////////////////
UINT
NetAddr::NetBits
() const
{
	return ( m_Mask );
} // NetBits()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Check if @a Addr is part of *this address
 *
 * @param Addr the NetAddr to check
 * @return true if @a Addr is part of *this address
 * @return false if @a Addr is not part of *this address
 * @see NetAddr::IsPartOf
 */
//////////////////////////////////////////////////////////////////////
bool
NetAddr::Contains
(
	const NetAddr& Addr
) const
{
	int      i;
	NetAddr First, Last;
	if ( Addr.m_Mask < m_Mask )
	{
		return ( false );
	}
	First = FirstAddr();
	Last  = LastAddr();
	for ( i = 0; i < NetAddr::Size; i++ )
	{
		if ( ( Addr.m_Addr[i] < First.m_Addr[i] )
				||  ( Addr.m_Addr[i] > Last.m_Addr[i] ) )
		{
			return ( false );
		}
	}
	return ( true );
} // NetAddr::Contains ( const NetAddr& Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Check if *this address is part of @a Addr
 *
 * @param Addr the NetAddr to check
 * @return true if @a Addr is part of *this address
 * @return false if @a Addr is not part of *this address
 * @see NetAddr::Contains
 */
//////////////////////////////////////////////////////////////////////
bool
NetAddr::IsPartOf
(
	const NetAddr& Addr
) const
{
	int      i;
	NetAddr First, Last;
	if ( m_Mask < Addr.m_Mask )
	{
		return ( false );
	}
	First = Addr.FirstAddr();
	Last  = Addr.LastAddr();
	for ( i = 0; i < NetAddr::Size; i++ )
	{
		if ( ( m_Addr[i] < First.m_Addr[i] )
		||   ( m_Addr[i] > Last.m_Addr[i] ) )
		{
			return ( false );
		}
	}
	return ( true );
} // NetAddr::IsPartOf ( const NetAddr& Addr )
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
NetAddr::IsLoopback
() const
{
	if ( m_Type == NetAddr::IPv4 )
	{
		if ( m_Addr[12] == 0x7F )
			return true;
	}
	else if ( m_Type == NetAddr::IPv6 )
	{
		uint64_t* ip = (uint64_t*) & m_Addr[0];
		if ((  ip[0] == 0 )
		&& ( XDR_decode_uint64 (ip[1]) == 1 ) )
			return true;
	}
	return false;
} // NetAddr::IsLoopback ()
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
NetAddr::IsNull
() const
{
	if ( m_Type == NetAddr::IPv4 )
	{
		uint32_t* ip = (uint32_t*) & m_Addr[12];
		if ( ip == 0 )
			return true;
	}
	else if ( m_Type == NetAddr::IPv6 )
	{
		uint64_t* ip = (uint64_t*) & m_Addr;
		if ((  ip[0] == 0 ) && ( ip[1] == 0 ) )
			return true;
	}
	return false;
} // NetAddr::IsNull ()
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
NetAddr::SockAddr
() const
{
	struct sockaddr*        SockAddr;
	struct sockaddr_in*     SockAddrV4;
	struct sockaddr_in6*    SockAddrV6;

	SockAddr   = ( struct sockaddr* )&         m_SockAddr;
	SockAddrV4 = ( struct sockaddr_in* )&      m_SockAddr;
	SockAddrV6 = ( struct sockaddr_in6* )&     m_SockAddr;
	memset ( SockAddr, 0, sizeof ( sockaddr_in6 ) );
	SockAddrV4->sin_port = htons ( m_Port );
	if ( m_Type == NetAddr::ANY )
	{
		SockAddrV4->sin_family = AF_INET;
		SockAddrV4->sin_addr.s_addr = INADDR_ANY;
	}
	else if ( m_Type == NetAddr::IPv4 )
	{
		SockAddrV4->sin_family = AF_INET;
		memcpy ( & ( SockAddrV4->sin_addr ), & m_Addr[12], 4 );
	}
	else
	{
		SockAddrV6->sin6_family = AF_INET6;
		memcpy ( & ( SockAddrV6->sin6_addr ), & m_Addr, 16 );
	}
	return ( SockAddr );
} // NetAddr::SockAddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Convert internal struct sockaddr m_SockAddr to our internal
 * Format.
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::CopySockAddr
()
{
	struct sockaddr*        SockAddr;
	SockAddr   = ( struct sockaddr* )         &m_SockAddr;
	for ( int i=0; i<NetAddr::Size; i++ )
	{
		m_Addr[i] = 0;
	}
	if ( SockAddr->sa_family == AF_INET )
	{
		// IPv4
		m_Mask  = MaxMaskIPv4;  // always host address
		m_Type  = IPv4;
		m_Error = E_OK;
		struct sockaddr_in* SockAddrV4 = ( struct sockaddr_in* ) &m_SockAddr;
		m_Port  = SockAddrV4->sin_port;
		memcpy ( & m_Addr[12], & ( SockAddrV4->sin_addr ), 4 );
	}
	else if ( SockAddr->sa_family == AF_INET6 )
	{
		// IPv6
		m_Mask  = MaxMask;  // always host address
		m_Type  = IPv6;
		m_Error = E_OK;
		struct sockaddr_in6* SockAddrV6 = ( struct sockaddr_in6* ) &m_SockAddr;
		m_Port  = SockAddrV6->sin6_port;
		memcpy ( & m_Addr, & ( SockAddrV6->sin6_addr ), 16 );
	}
} // NetAddr::pm_CopySockAddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the size of our internal struct sockaddr
 *
 * @return The size of our address structure
 */
//////////////////////////////////////////////////////////////////////
socklen_t
NetAddr::AddrSize
() const
{
	return ( sizeof ( m_SockAddr ) );
} // NetAddr::AddrSize ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Map an IPv4 address to an IPv4inIPv6 address.
 *
 * @return A mapped %NetAddr of *this
 */
//////////////////////////////////////////////////////////////////////
NetAddr
NetAddr::MapToV6
() const
{
	NetAddr Result;
	if ( ( ( m_Type & IPv6 ) > 0 )
	||     ( m_Type & IPv4 ) == 0 )
	{
		// no IPv4 address
		Result.SetError ( NetAddr::E_WrongIPv4 );
		return ( Result );
	}
	Result.m_Addr[0] = 0x20;
	Result.m_Addr[1] = 0x02;
	Result.m_Addr[2] = m_Addr[12];
	Result.m_Addr[3] = m_Addr[13];
	Result.m_Addr[4] = m_Addr[14];
	Result.m_Addr[5] = m_Addr[15];
	Result.m_Mask    = m_Mask+16;
	Result.m_Type    = IPv6;
	return ( Result );
} // NetAddr::MapToV6 ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Map an IPv4inIPv6 address to an IPv4 address.
 *
 * @return A mapped %NetAddr of *this
 */
//////////////////////////////////////////////////////////////////////
NetAddr
NetAddr::MapFromV6
() const
{
	NetAddr Result;
	if ( ( ( m_Type & IPv6 ) == 0 )
	||  ( ( m_Type & IPv4 ) > 0 )
	||  ( m_Addr[0] != 0x20 )
	||  ( m_Addr[1] != 0x02 ) )
	{
		// no IPv4 address
		Result.SetError ( NetAddr::E_WrongIPv6 );
		return ( Result );
	}
	Result.m_Addr[12] = m_Addr[2];
	Result.m_Addr[13] = m_Addr[3];
	Result.m_Addr[14] = m_Addr[4];
	Result.m_Addr[15] = m_Addr[5];
	Result.m_Mask     = m_Mask-16;
	Result.m_Type     = IPv4;
	return ( Result );
} // NetAddr::MapFromV6 ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Represent the netmask as an IP address, eg. '255.255.255.0'
 *
 * @return A %NetAddr representation of the netmask
 */
//////////////////////////////////////////////////////////////////////
NetAddr
NetAddr::MaskAddr
() const
{
	NetAddr Result ( *this );
	Result.BuildFromMask ();
	for ( int i=0; i<NetAddr::Size; i++ )
	{
		Result.m_Addr[i] = Result.m_MaskAddr[i];
	}
	return ( Result );
} // NetAddr::MaskAddr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Represent the netmask as an IP address, eg. '255.255.255.0'
 *
 * @return A %NetAddr representation of the netmask
 */
//////////////////////////////////////////////////////////////////////
NetAddr
NetAddr::CiscoWildcard
() const
{
	NetAddr Result ( *this );
	Result.BuildFromMask ();
	for ( int i=0; i<NetAddr::Size; i++ )
	{
		Result.m_Addr[i] = ~Result.m_MaskAddr[i];
	}
	return ( Result );
} // NetAddr::CiscoWildcard()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the first address from network. If *this NetAddr is an
 * IPv4 address, the result represents the network address.
 *
 * @return The first address of *this network.
 */
//////////////////////////////////////////////////////////////////////
NetAddr
NetAddr::FirstAddr
() const
{
	NetAddr Result ( *this );
	UINT    nStart;
	if ( m_MaskAddr[0] == 0 )
	{
		Result.BuildFromMask();
	}
	if ( ( m_Type & IPv6 ) == 0 )
	{
		// must be IPv4 only
		nStart = 12;
	}
	else
	{
		nStart = 0;
	}
	for ( int i=nStart; i<NetAddr::Size; i++ )
	{
		Result.m_Addr[i] = m_Addr[i] & Result.m_MaskAddr[i];
	}
	return ( Result );
} // NetAddr::FirstAddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the first usable address from network. If *this address
 * is an IPv6 address, the first usable address is the first address
 * of *this network. For IPv4 Addresses it is the successor of the
 * netaddress.
 *
 * @return The first usable address of *this network
 * @see NetAddr::FirstAddr
 */
//////////////////////////////////////////////////////////////////////
NetAddr
NetAddr::FirstUsableAddr
() const
{
	NetAddr Result ( *this );
	if ( ( ( m_Type & IPv4 ) > 0 ) && ( m_Mask == 32 ) )
	{
		return ( Result );
	}
	if ( ( m_Type & IPv6 ) > 0 )
	{
		return ( FirstAddr () );
	}
	Result = FirstAddr ();
	Result++;
	return ( Result );
} // NetAddr::FirstUsableAddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the last address of *this network. For an IPv4 address, the
 * result represents the broadcast address of *this network.
 *
 * @return The last address of *this network
 */
//////////////////////////////////////////////////////////////////////
NetAddr
NetAddr::LastAddr
() const
{
	NetAddr Result ( *this );
	UINT    nStart;
	if ( m_MaskAddr[0] == 0 )
	{
		Result.BuildFromMask();
	}
	if ( ( m_Type & IPv6 ) == 0 )
	{
		// must be IPv4 only
		nStart = 12;
	}
	else
	{
		nStart = 0;
	}
	for ( int i=nStart; i<NetAddr::Size; i++ )
	{
		Result.m_Addr[i] = m_Addr[i] | ~Result.m_MaskAddr[i];
	}
	Result.m_Type     = m_Type;
	Result.m_Mask     = m_Mask;
	Result.m_Error    = m_Error;
	return ( Result );
}  // NetAddr::LastAddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Return the last usable address from network. If *this is an IPv6
 * address, the result is the last address of *this network.
 *
 * @return The last usable address of *this network.
 * @see NetAddr::LastAddr
 */
//////////////////////////////////////////////////////////////////////
NetAddr
NetAddr::LastUsableAddr
() const
{
	NetAddr Result ( *this );
	if ( ( ( m_Type & IPv4 ) > 0 ) && ( m_Mask == 32 ) )
	{
		return ( Result );
	}
	if ( ( m_Type & IPv6 ) > 0 )
	{
		return ( LastAddr () );
	}
	Result = LastAddr ();
	Result--;
	return ( Result );
} // NetAddr::LastUsableAddr ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Create m_MaskAddr from m_Mask
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::BuildFromMask
()
{
	UINT i;
	UINT nBytes;
	UINT nBits;
	UINT nBitMask;
	UINT nStart;
	for ( i=0; i < NetAddr::Size; i++ )
	{
		m_MaskAddr[i] = 0;
	}
	nBits = m_Mask;
	nBytes = ( m_Mask / 8 );
	if ( ( m_Type & IPv6 ) == 0 ) // must be IPv4 only
	{
		nStart = 12;
	}
	else
	{
		nStart = 0;
	}
	for ( i=0; i < nBytes; i++ )
	{
		m_MaskAddr[i+nStart] = 0xff;
		nBits -= 8;
	}
	nBytes = i;
	nBitMask = 0x80;
	for ( i=0; i < nBits; i++ )
	{
		m_MaskAddr[nBytes+nStart] |= ( nBitMask >> i );
	}
} // NetAddr::BuildFromMask ()
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
string
NetAddr::ToString
(
	const ADDRESS_BASE nBase,
	const ADDRESS_FORMAT nFormat
) const
{
	int             nNumBytes;
	int             nRealBytes;
	int             nOutputBytes;
	ADDRESS_BASE    nWantBase;
	UINT            nOctet;
	bool            bWantFill;
	bool            bDidCompression;
	bool            bDoOutput;
	string          strReturn;
	if ( nBase == NetAddr::STD )
	{
		nWantBase = NetAddr::SEDECIMAL;
	}
	else
	{
		nWantBase = nBase;
	}
	nRealBytes      = 0;
	nOutputBytes    = NetAddr::Size;
	bDidCompression = false;
	if ( nFormat != NetAddr::Compressed )
	{
		bDoOutput = true;
	}
	else
	{
		bDoOutput = false;
	}
	if ( ( m_Type & IPv4 ) > 0 )
	{
		nOutputBytes -= 4;
	}
	if ( ( m_Type & IPv6 ) == 0 )
	{
		nOutputBytes = 0;
	}
	nNumBytes = 0;
	while ( nNumBytes < nOutputBytes )
	{
		// addr is v6
		nOctet = ( m_Addr[nNumBytes] << 8 );
		nNumBytes++;
		nOctet += m_Addr[nNumBytes];
		nNumBytes++;
		//////////////////////////////////////////////////
		//
		//      insert leading zeros if necessary
		//
		//////////////////////////////////////////////////
		bWantFill = false;
		if ( ( nRealBytes > 0 ) || ( nFormat == NetAddr::Full ) )
		{
			bWantFill = true;
		}
		if ( ( nFormat == NetAddr::Short ) && ( nOctet == 0 ) )
		{
			bWantFill = false;
		}
		if ( ( nOctet != 0 ) || ( bWantFill == true )
		||  ( bDoOutput == true ) )
		{
			strReturn += WordToString ( nOctet, nWantBase, bWantFill );
			nRealBytes += 2;
		}
		if ( ( ( nNumBytes ) % 2 ) == 0 )
		{
			if ( ( nRealBytes == 0 ) && ( bDidCompression == false ) )
			{
				strReturn += ":";
				bDidCompression = true;
			}
			if ( ( nNumBytes < NetAddr::Size ) && ( nRealBytes > 0 ) )
			{
				strReturn += ":";
				nRealBytes = 0;
			}
		}
	} // while()
	if ( ( m_Type & IPv4 ) > 0 )
	{
		//////////////////////////////////////////////////
		//
		//      output of last 4 bytes
		//
		//////////////////////////////////////////////////
		if ( nBase == NetAddr::STD )
		{
			nWantBase = NetAddr::DECIMAL;
			bWantFill = false;
		}
		else
		{
			nWantBase = nBase;
			bWantFill = true;
		}
		for ( int i=12; i<NetAddr::Size; i++ )
		{
			strReturn += ByteToString ( m_Addr[i], nWantBase, bWantFill );
			if ( i<15 ) // last byte
			{
				strReturn += '.';
			}
		}
	}
	return ( strReturn );
} // NetAddr::pm_ToString ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Scan a given string if it is a valid IPv[4|6] address.\n
 * Calling this function we know:\n
 *      - if the address is syntactically correct\n
 *      - how many octets are in the string\n
 *      - how many octets we have to insert in an expansion (::)\n
 *      - type of address (m_Type gets set)\n
 *
 * @param Addr  A string representation of an IP address.
 * @return      The number of octest in the string.
 */
//////////////////////////////////////////////////////////////////////
UINT
NetAddr::PreScanStringAddress
(
	const string& Addr
)
{
	UINT    nNumOctets;             // number of octets in string
	UINT    nNumColons;             // number of colons in string
	UINT    nNumDots;               // number of dots in string
	UINT    nLastColon;             // position of last colon
	UINT    nLastDot;               // position of last dot
	bool    bIsCompressed;          // "::" in string
	bool    bHaveMask;              // "/" in string
	UINT    i;
	m_Type          = NetAddr::Invalid;// assume address is not valid
	nNumColons      = 0;
	nNumDots        = 0;
	nNumOctets      = 0;
	nLastColon      = 0;
	nLastDot        = 0;
	bIsCompressed   = false;
	bHaveMask       = false;
	if ( Addr.size() == 0 )
	{
		SetError ( NetAddr::E_AddressIsEmpty );
		return ( 0 );
	}
	for ( i=0; i < Addr.size(); i++ )
	{
		if ( Addr[i] == ':' )
		{
			if ( ( m_Type & IPv4 ) > 0 )
			{
				// already seen dots, no more : allowed
				SetError ( NetAddr::E_WrongIPv6 );
				return ( 0 );
			}
			m_Type = NetAddr::IPv6;
			if ( ( i-nLastColon ) == 1 )
			{
				if ( bIsCompressed )
				{
					// already found ::
					SetError ( NetAddr::E_ExpansionOnlyOnce );
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
			m_Type = NetAddr::IPv4;
			if ( ( i-nLastDot ) == 1 )
			{
				SetError ( NetAddr::E_MissingValue );
				return ( 0 );
			}
			else if ( ( i-nLastDot ) > 4 )
			{
				SetError ( NetAddr::E_WrongIPv4 );
				return ( 0 );
			}
			nLastDot = i;
			nNumDots++;
		}
		else if ( Addr[i] == '/' )
		{
			if ( ( ( m_Type & IPv4 ) != 0 ) && ( ( i - nLastDot ) > 4 ) )
			{
				SetError ( NetAddr::E_WrongIPv4 );
				return ( 0 );
			}
			else if ( ( ( m_Type & IPv6 ) !=0 ) && ( ( i-nLastDot ) > 5 ) )
			{
				SetError ( NetAddr::E_WrongIPv6 );
				return ( 0 );
			}
			if ( i+1 == Addr.size() )
			{
				SetError ( NetAddr::E_NetmaskMissing );
				return ( 0 );
			}
			if ( ( Addr[i-1] != ':' ) && ( ( m_Type & IPv4 ) == 0 ) )
			{
				nNumOctets++;
			}
			bHaveMask = true;
		}
		else if ( ( bHaveMask == true )
				&& ( ( Addr[i] < '0' ) || ( Addr[i] > '9' ) ) )
		{
			SetError ( NetAddr::E_NetmaskDecimal );
			return ( 0 );
		}
		else if ( ( ( Addr[i] < '0' ) || ( Addr[i] > '9' ) )
		&&   ( ( Addr[i] < 'a' ) || ( Addr[i] > 'f' ) )
		&&   ( ( Addr[i] < 'A' ) || ( Addr[i] > 'F' ) ) )
		{
			SetError ( NetAddr::E_IllegalChars );
			return ( 0 );
		}
	} // for ()
	if ( m_Type == NetAddr::Invalid )
	{
		SetError ( NetAddr::E_WrongIPv6 );
		return ( 0 );
	}
	if ( ( ( m_Type & IPv4 ) != 0 ) && ( bHaveMask == false )
			&&  ( ( i - nLastDot ) > 3 ) )
	{
		SetError ( NetAddr::E_WrongIPv4 );
		return ( 0 );
	}
	else if ( ( ( m_Type & IPv6 ) != 0 ) && ( bHaveMask == false )
			&&  ( ( i - nLastDot ) > 5 ) )
	{
		SetError ( NetAddr::E_WrongIPv6 );
		return ( 0 );
	}
	//////////////////////////////////////////////////
	//      if the string contained no mask
	//      the last octet was not counted
	//////////////////////////////////////////////////
	if ( ( bHaveMask == false ) && ( Addr[Addr.size() -1] != ':' ) )
	{
		if ( ( m_Type & IPv4 ) == 0 )
		{
			nNumOctets++;
		}
	}
	if ( ( nNumDots != 0 ) && ( nNumDots != 3 ) )
	{
		SetError ( NetAddr::E_WrongIPv4 );
		return ( 0 );
	}
	if ( nNumColons > 7 )
	{
		SetError ( NetAddr::E_WrongIPv6 );
		return ( 0 );
	}
	if ( ( nNumColons == 6 ) && ( nNumDots != 3 ) )
	{
		SetError ( NetAddr::E_WrongIPv6 );
		return ( 0 );
	}
	if ( ( nNumOctets == 0 ) && ( nNumDots == 0 ) )
	{
		SetError ( NetAddr::E_AddressIsEmpty );
		return ( 0 );
	}
	return ( nNumOctets );
} // NetAddr::PreScanStringAddress ( const string& Addr, UINT& nNumOctets )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Convert a string representation of an IP address to our internal
 * Format.
 *
 * @param Addr  A string representation of an IP address
 */
//////////////////////////////////////////////////////////////////////
void
NetAddr::FromString
(
	const string& Addr
)
{
	AddrNumber      Val;            // value of letter
	AddrNumber      CurrentByte;    // value of current byte
	unsigned int    Current;        // pointer in Addr to current letter
	unsigned int    NumLetters;     // number of already collected letters
	unsigned int    NextColon;      // number of letters to the next colon
	unsigned int    LettersNeeded;  // number of letters to build a byte
	unsigned int    i;
	UINT            nNumOctets;
	bool            bFoundDot;
	bool            bExpanded;
	ADDRESS_BASE    nBase;
	Init ();

	if ( Addr == "" )
	{
		return;
	}
	//////////////////////////////////////////////////
	//
	//      check string
	//
	//////////////////////////////////////////////////
	nNumOctets = PreScanStringAddress ( Addr );
	if ( m_Type == NetAddr::Invalid )
	{
		// given string is not an IP Address
		Resolve ( Addr, 0 );
		if ( m_Error != E_OK )
		{
			// not a hostname either so
			// make sure address is empty and invalid
			Init();
			return;
		}
		return;
	}
	//////////////////////////////////////////////////
	//
	//      evaluate string
	//
	//////////////////////////////////////////////////
	Val             = 0;
	NumLetters      = 0;
	CurrentByte     = 0;
	nBase           = NetAddr::SEDECIMAL;
	bExpanded       = false;
	bFoundDot       = false;
	if ( ( ( m_Type & IPv6 ) == false )
	&&  ( ( m_Type & IPv4 ) == true ) )
	{
		// IPv4 only, fill only last 4 bytes
		CurrentByte = 12;
	}
	Current = 0;
	LettersNeeded = 0;
	while ( Current < Addr.size () )
	{
		//////////////////////////////////////////////////
		//
		//      check how many letters there are to
		//      the next colon (or dot)
		//
		//////////////////////////////////////////////////
		if ( NumLetters == 0 )
		{
			LettersNeeded = 0;
			NextColon = Current;
			while ( ( NextColon < Addr.size() )
			&& ( Addr[NextColon] != ':' )
			&& ( Addr[NextColon] != '/' )
			&& ( Addr[NextColon] != '.' ) )
			{
				NextColon++;
			}
			if ( Addr[NextColon] == '.' )
			{
				bFoundDot = true;
				nBase = NetAddr::DECIMAL;
			}
			NextColon -= Current;
			if ( bFoundDot )
			{
				// eat up to dot
				LettersNeeded = NextColon;
			}
			else
			{
				switch ( NextColon )
				{
				case 0:
					LettersNeeded = ( UINT )-1;
					break;
				case 1:
					LettersNeeded = 1;
					break;
				case 2:
					LettersNeeded = 2;
					break;
				case 3:
					LettersNeeded = 1;
					break;
				case 4:
					LettersNeeded = 2;
					break;
				default:
					SetError ( NetAddr::E_WrongIPv6 );
					return;
				}
			}
		}
		if ( NumLetters == LettersNeeded )
		{
			if ( NextColon == 3 )
			{
				LettersNeeded = 2;
			}
			if ( ( !bFoundDot )
			&& ( ( NextColon == 2 ) || ( NextColon == 1 ) ) )
			{
				// one byte missing
				m_Addr[CurrentByte] = 0;
				CurrentByte++;
			}
			m_Addr[CurrentByte] = Val;
			CurrentByte++;
			NumLetters = 0;
			Val = 0;
		}
		if ( Addr[Current] == ':' )
		{
			if ( ( NextColon == 0 ) && ( bExpanded == false ) )
			{
				int NeedFill;
				bExpanded = true;
				NeedFill = ( 8 - nNumOctets ) *2;
				if ( ( m_Type & IPv4 ) > 0 )
				{
					NeedFill -= 4;
				}
				if ( ( m_Type & IPv6 ) == 0 )
				{
					NeedFill -= 4;
				}
				if ( NeedFill < 0 )
				{
					NeedFill = 0;        // FIXME: reached?
				}
				if ( CurrentByte+NeedFill > NetAddr::Size )
				{
					NeedFill = 0;
				}
				for ( i = 0; i< ( UINT ) NeedFill; i++ )
				{
					m_Addr[CurrentByte] = 0;
					CurrentByte++;
				}
			}
		}
		else if ( Addr[Current] == '.' )
		{}
		else if ( Addr[Current] == '/' )
		{
			break;        // found netmask
		}
		else
		{
			Val *= nBase;
			Val += FromString ( Addr[Current], nBase );
			NumLetters++;
		}
		Current++;
	} // while ()
	if ( NumLetters == LettersNeeded )
	{
		if ( ( NumLetters == 1 ) && ( ( m_Type & IPv4 ) == 0 ) )
		{
			m_Addr[CurrentByte] = 0;
			CurrentByte++;
		}
		m_Addr[CurrentByte] = Val;
		CurrentByte++;
		NumLetters = 0;
		Val = 0;
	}
	//////////////////////////////////////////////////
	//
	//      evaluate netmask
	//
	//////////////////////////////////////////////////
	if ( Addr[Current] == '/' )
	{
		m_Mask = 0;
		Current++;
		while ( Current < Addr.size() )
		{
			m_Mask *= NetAddr::DECIMAL;
			m_Mask += FromString ( Addr[Current], NetAddr::DECIMAL );
			Current++;
		}
		if ( ( ( m_Type & NetAddr::IPv6 ) == 0 )
				&&   ( m_Mask > NetAddr::MaxMaskIPv4 ) )
		{
			SetError ( NetAddr::E_OutOfRange );
			return;
		}
		if ( ( m_Mask > NetAddr::MaxMask ) || ( m_Mask < 0 ) )
		{
			m_Error = NetAddr::E_OutOfRange;
			return;
		}
		if ( m_Mask == 0 )
		{
			m_Error = NetAddr::E_NetmaskMissing;
			return;
		}
	}
	if ( ( m_Type == NetAddr::IPv4 )
			&&  ( m_Mask > NetAddr::MaxMaskIPv4 ) )
	{
		m_Mask = MaxMaskIPv4;
	}
} // NetAddr::FromString ( const char* Addr )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Internal converter. Convert @a cLetter into a number, honours
 * @a nBase. (Eg: 'c' -> 12)
 *
 * @param cLetter       the letter (character) to convert.
 * @param nBase         the letter is in this base (eg. SEDECIMAL)
 * @return              A number representation of @a cLetter
 */
//////////////////////////////////////////////////////////////////////
AddrNumber
NetAddr::FromString
(
	AddrNumber cLetter,
	const ADDRESS_BASE nBase
) const
{
	if ( ( nBase < NetAddr::MinBase )
	||  ( nBase > NetAddr::MaxBase ) )
	{
		// SetError (E_OutOfRange);
		return ( '0' );
	}
	if ( ( cLetter < '0' ) || ( cLetter > '9' ) )
	{
		cLetter = toupper ( cLetter );
		if ( ( cLetter < 'A' ) || ( cLetter > 'F' ) )
		{
			// SetError (E_IllegalChars);
			return ( '0' );
		}
		cLetter -= ( 'A' - 10 );
	}
	else
	{
		cLetter -= '0';
	}
	if ( cLetter > nBase )
	{
		// SetError (E_OutOfRange);
		return ( '0' );
	}
	return ( cLetter );
} // NetAddr::FromString ( char cLetter )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Internal converter. Convert a word (integer) into a string
 * (Eg: 1212 -> 'cc'
 *
 * @param nNumber       The Number to convert.
 * @param nBase         The base to convert to (eg: SEDECIMAL).
 * @param bFill         If true, fill missing leading characters
 *                      with '0's.
 * @return              A string representation of @a nNumber
 */
//////////////////////////////////////////////////////////////////////
string
NetAddr::WordToString
(
	UINT nNumber,
	const ADDRESS_BASE nBase,
	const bool bFill
) const
{
	const char*     Sedecimals = "0123456789ABCDEF";
	const unsigned char NumberOfDigits[] = {16,11,8,7,7,6,6,6,5,5,5,5,5,5,4};
	string          strReturn;
	if ( ( nBase < NetAddr::MinBase )
	||  ( nBase > NetAddr::MaxBase ) )
	{
		// SetError (E_OutOfRange);
		return ( "0" );
	}
	if ( nNumber == 0 )
	{
		if ( ! bFill )
		{
			return ( "0" );
		}
		for ( nNumber=0; nNumber<NumberOfDigits[nBase-2]; nNumber++ )
		{
			strReturn = "0" + strReturn;
		}
		return ( strReturn );
	}
	while ( nNumber > 0 )
	{
		strReturn = Sedecimals [nNumber % nBase] + strReturn;
		nNumber /= nBase;
	}
	if ( bFill )
	{
		nNumber = NumberOfDigits[nBase-2] - strReturn.size();
		while ( nNumber > 0 )
		{
			strReturn = "0" + strReturn;
			nNumber--;
		}
	}
	return ( strReturn );
} // NetAddr::ByteToString ( char cNumber )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Internal converter. Convert a byte into a string representation.
 * (Eg: 12 -> "12")
 *
 * @param nNumber       The byte to convert.
 * @param nBase         The base the number will be represented
 * @param bFill         Fill missing leading characters with '0's.
 * @return              A string representation of @a nNumber
 */
//////////////////////////////////////////////////////////////////////
string
NetAddr::ByteToString
(
	AddrNumber nNumber,
	const ADDRESS_BASE nBase,
	const bool bFill
) const
{
	const char*     Sedecimals = "0123456789ABCDEF";
	const char      NumberOfDigits[] = {8,6,4,4,4,3,3,3,3,3,3,3,3,3,2};
	string          strReturn;
	if ( ( nBase < NetAddr::MinBase ) ||  ( nBase > NetAddr::MaxBase ) )
	{
		// SetError (E_OutOfRange);
		return ( "0" );
	}
	if ( nNumber == 0 )
	{
		if ( ! bFill )
		{
			return ( "0" );
		}
		for ( nNumber=0; nNumber<NumberOfDigits[nBase-2]; nNumber++ )
		{
			strReturn = "0" + strReturn;
		}
		return ( strReturn );
	}
	while ( nNumber > 0 )
	{
		strReturn = Sedecimals [nNumber % nBase] + strReturn;
		nNumber /= nBase;
	}
	if ( bFill )
	{
		nNumber = NumberOfDigits[nBase-2] - strReturn.size();
		while ( nNumber > 0 )
		{
			strReturn = "0" + strReturn;
			nNumber--;
		}
	}
	return ( strReturn );
} // NetAddr::ByteToString ( char cNumber )
//////////////////////////////////////////////////////////////////////

/** NetAddr output operator
 */
std::ostream&
operator <<
(
	std::ostream& o,
	const NetAddr& Addr
)
{
	o << Addr.ToString();
	return ( o );
} // ostream& operator << ( NetAddr )

