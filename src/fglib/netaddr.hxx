// netaddr.hxx
// class for handling internet addresses
//
// This file is part of fgms
//
// Copyright (C) 2005-2009 Oliver Schroeder
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


/** @file netaddr.hpp
 * provide a class for internet addresses.
 */
#ifndef ADDR_HEADER
#define ADDR_HEADER

#include <string>
#include <cstring>
#ifdef _MSC_VER
#include <WinSock2.h>
#include <Ws2ipdef.h>
typedef int socklen_t;
#else
#include <netinet/in.h>
#endif

using namespace std;

typedef unsigned char AddrNumber;
typedef unsigned int  UINT;

/****
* A class for inet addresses.
*
* @author       Oliver Schroeder <post@o-schroeder.de>
* @date         Dec-2005
****/
class NetAddr
{
public:
	/// some constants for inet addresses
	enum CONSTANTS
	{
		/// number of bits in IPv6
		MaxMask         = 128,
		/// ipv4 only has 32 bit
		MaxMaskIPv4     = 32,
		/// number of bytes in IPv6 addresses
		Size            = 16
	};
	/// formats of ipv6 addresses
	enum ADDRESS_FORMAT
	{
		/// Format 2001:aa0:801:2::2
		Compressed      = 1,
		/// Format 2001:AA0:801:2:0:0:0:2
		Short           = 2,
		/// Format 2001:0AA0:0801:0002:0000:0000:0000:0002
		Full            = 3
	};
	/// base of different ip addresses
	enum ADDRESS_BASE
	{
		/// minimum base is binary
		MinBase         = 2,
		/// maximum base is sedecimal
		MaxBase         = 16,
		/// constant for Base
		BINARY          = 2,
		/// constant for Base
		DECIMAL         = 10,
		/// constant for Base
		SEDECIMAL       = 16,
		/// use the appropriate base for IP4|6
		STD             = 255
	};
	/// type of address
	enum ADDRESS_TYPE
	{
		/// Addr is invalid
		Invalid         = 0,
		/// Addr is version 4
		IPv4            = 1,
		/// Addr is version 6,
		IPv6            = 2,
		/// Ipv4|IPv6
		IPv4inIPv6      = 3,
		/// any
		ANY             = 4
	};
	/// error codes
	enum ERROR_CODE
	{
		/// no error occured
		E_OK            = 0,
		/// address string is empty
		E_AddressIsEmpty,
		/// illegal characters in address string
		E_IllegalChars,
		/// netmask not in decimal notation
		E_NetmaskDecimal,
		/// expected a netmask after a '/' character, but there is none
		E_NetmaskMissing,
		/// netmask is out of range
		E_OutOfRange,
		/// string representation of an IPv4 address has wrong format
		E_WrongIPv4,
		/// string representation of an IPv6 address has wrong format
		E_WrongIPv6,
		/// no value between dots
		E_MissingValue,
		/// ::-expansion is allowed only once
		E_ExpansionOnlyOnce,
		/// not resolvable
		E_CouldNotResolve
	};
	/// default constructor
	NetAddr ();
	/// copy constructor
	NetAddr ( const NetAddr& Addr );
	/// construct from string and port
	NetAddr ( const string& Addr, const int Port = 0 );
	/// assign an int, only works for ipv4
	void Assign ( unsigned int IP, const int Port = 0 );
	/// assign an address as a 4-dotted string
	void Assign ( const string& IP, const int Port = 0 );
	/// assign a NetAddr
	void Assign ( const NetAddr& Addr );
	/// convert a struct sockaddr* to our internal format
	void Assign ( struct sockaddr* SrcSockAddr );
	/// Only assign port
	void SetPort ( const int Port );
	/// resolve hostname
	void Resolve ( const string& Host, const int Port = 0 );
	///@defgroup netaddr-operators
	///@{
	/// assign another Addr
	void operator =  ( const NetAddr& Addr );
	/// assign a string
	void operator =  ( const string& Addr );
	/// increment address by 1
	void operator ++  ( int CppIsStupid );
	/// decrement address by 1
	void operator --  ( int CppIsStupid );
	/// comparison operator
	bool operator ==  ( const NetAddr& Addr ) const;
	/// comparison operator
	bool operator !=  ( const NetAddr& Addr ) const;
	/// comparison operator
	bool operator >= ( const NetAddr& Addr ) const;
	/// comparison operator
	bool operator <= ( const NetAddr& Addr ) const;
	/// comparison operator
	bool operator >  ( const NetAddr& Addr ) const;
	/// comparison operator
	bool operator <  ( const NetAddr& Addr ) const;
	///@}
	/// convert address to string
	string ToString (
		const ADDRESS_BASE nBase = NetAddr::STD,
		const ADDRESS_FORMAT nFormat = NetAddr::Full ) const;
	/// return error code
	inline UINT Error () const
	{
		return m_Error;
	};
	/// return address type
	inline UINT AddrType () const
	{
		return m_Type;
	};
	/// return address mask
	inline UINT AddrMask () const
	{
		return m_Mask;
	};
	/// return number of bits of the host part
	UINT HostBits() const;
	/// return number of hosts in network
	UINT NumberOfHosts() const;
	/// return number of address
	UINT AddrNum() const;
	/// return number of bits of the net part
	UINT NetBits() const;
	/// return the port
	inline UINT Port() const
	{
		return m_Port;
	};
	/// return true if this address is valid
	inline bool IsValid ()
	{
		return ( ( m_Error == E_OK ) && ( m_Type != NetAddr::ANY ) );
	}
	/// return the first address of the net (netaddress)
	NetAddr FirstAddr () const;
	/// return the last address of the net (broadcast)
	NetAddr LastAddr () const;
	/// return the first usable address of the net
	NetAddr FirstUsableAddr () const;
	/// return the last usable address of the net
	NetAddr LastUsableAddr () const;
	/// return the network mask as an address
	NetAddr MaskAddr () const;
	/// return the cisco wildcard of this address
	NetAddr CiscoWildcard () const;
	/// return true if Addr is part of this net
	bool Contains ( const NetAddr& Addr ) const;
	/// return true is this net is part of Addr
	bool IsPartOf ( const NetAddr& Addr ) const;
	/// return true if this is a loopback IP
	bool IsLoopback () const;
	/// return true if this is unset
	bool IsNull () const;
	/// map an ipv4 address to an ipv6 address
	NetAddr MapToV6 () const;
	/// map an IPv4inIPv6 address to an IPv4 address
	NetAddr MapFromV6 () const;
	/// return this address in a sockaddr structure
	struct sockaddr* SockAddr() const;
	/// convert a struct sockaddr[_in[6]] to our internal format
	void CopySockAddr ();
	/// return the size of the sockaddr structure
	socklen_t AddrSize () const;
	/// get error message
	string GetErrorMsg () const;
	/// output operator
	friend std::ostream& operator << ( std::ostream& o, const NetAddr& Addr );
private:
	/// convert a letter into a number
	AddrNumber FromString (
		AddrNumber cLetter,
		const ADDRESS_BASE nBase ) const;
	/// convert a word into a string
	string WordToString (
		UINT nNumber,
		const ADDRESS_BASE nBase,
		const bool bFill=false ) const;
	/// convert a byte into a string
	string ByteToString (
		AddrNumber nNumber,
		const ADDRESS_BASE nBase,
		const bool bFill=false ) const;
	/// convert a string into our internal format
	void FromString ( const string& Addr );
	/// initialise all values
	void Init();
	/// test if string @a Addr represents a valid IP address
	UINT PreScanStringAddress ( const string& Addr );
	/// set internal error code
	void SetError ( const ERROR_CODE nError );
	/// return the last address of current network range
	void BuildFromMask ();
	/// our internal representation of an IP address
	AddrNumber m_Addr[16];
	/// our internal representation of an IP mask
	AddrNumber m_MaskAddr[16];
	/// number of bits of our current network mask
	UINT m_Mask;
	/// our internal representation of IP type
	ADDRESS_TYPE m_Type;
	/// our internal error code
	ERROR_CODE m_Error;
	/// the currently used TCP/UDP port
	uint16_t  m_Port;
	/// internal structure for 'struct sockaddr' conversions
	char m_SockAddr[sizeof ( struct sockaddr_in6 )];
};

#endif

