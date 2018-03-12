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
 * @file	ipaddr.hxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	07/2017
 */

#ifndef IPADDR_HEADER
#define IPADDR_HEADER

#include <string>
#include <cstring>
#include <stdint.h>
#ifdef _MSC_VER
	#include <WinSock2.h>
	#include <Ws2ipdef.h>
	using socklen_t = int;
#else
	#include <netinet/in.h>
#endif

namespace fgmp
{

/**@ingroup fglib
* A class for inet addresses.
*
* @author       Oliver Schroeder <post@o-schroeder.de>
* @date         Dec-2005
****/
class ipaddr
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
	ipaddr ();
	ipaddr ( const ipaddr& Addr );
	ipaddr ( const std::string& Addr );
	void assign ( unsigned int ip );
	void assign ( const std::string& ip );
	void assign ( const ipaddr& addr );
	void assign ( struct sockaddr* src_sockaddr );
	void resolve ( const std::string& Host );
	void operator =  ( const ipaddr& Addr );
	void operator =  ( const std::string& Addr );
	void operator ++  ( int CppIsStupid );
	void operator --  ( int CppIsStupid );
	bool operator ==  ( const ipaddr& Addr ) const;
	bool operator !=  ( const ipaddr& Addr ) const;
	bool operator >= ( const ipaddr& Addr ) const;
	bool operator <= ( const ipaddr& Addr ) const;
	bool operator >  ( const ipaddr& Addr ) const;
	bool operator <  ( const ipaddr& Addr ) const;
	std::string to_string (
		const ADDRESS_BASE nBase = ipaddr::STD,
		const ADDRESS_FORMAT nFormat = ipaddr::Full ) const;
	inline uint32_t error () const
	{
		return m_error;
	};
	inline uint32_t type () const
	{
		return m_type;
	};
	inline uint32_t mask () const
	{
		return m_mask;
	};
	uint32_t host_bits() const;
	uint32_t number_of_hosts() const;
	uint32_t addr_num() const;
	uint32_t net_bits() const;
	inline bool is_valid ()
	{
		return ( ( m_error == E_OK ) && ( m_type != ipaddr::ANY ) );
	}
	ipaddr first_addr () const;
	ipaddr last_addr () const;
	ipaddr first_usable_addr () const;
	ipaddr last_usable_addr () const;
	ipaddr mask_adde () const;
	ipaddr cisco_wildcard () const;
	bool contains ( const ipaddr& Addr ) const;
	bool is_part_of ( const ipaddr& Addr ) const;
	bool is_loopback () const;
	bool is_mapped_v4 () const;
	bool is_unspecified () const;
	bool is_multicast () const;
	bool is_link_local () const;
	bool is_site_local () const;
	ipaddr map_to_v6 () const;
	ipaddr map_from_v6 () const;
	struct sockaddr* to_sockaddr( const uint16_t port ) const;
	void copy_sockaddr ();
	socklen_t size () const;
	std::string get_error_msg () const;
	friend std::ostream& operator << ( std::ostream& o, const ipaddr& Addr );
private:
	void from_string ( const std::string& addr );
	void init();
	void set_error ( const ERROR_CODE error );
	void build_from_mask ();
	/// our internal representation of an IP address
	uint8_t m_addr[16];
	/// our internal representation of an IP mask
	uint8_t m_mask_addr[16];
	/// number of bits of our current network mask
	uint32_t m_mask;
	/// our internal representation of IP type
	ADDRESS_TYPE m_type;
	/// our internal error code
	ERROR_CODE m_error;
	/// the currently used TCP/UDP port
	/// internal structure for 'struct sockaddr' conversions
	char m_sockaddr[sizeof ( struct sockaddr_in6 )];
};

} // namespace fgmp

#endif

