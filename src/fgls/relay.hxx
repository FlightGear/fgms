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
 * @file        relay.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        03/2018
 */

#ifndef _fgls_relay_header
#define _fgls_relay_header

#include <stdint.h>
#include <string>
#include <memory>
#include <fglib/fg_thread.hxx>  // fgmp::lock_list_t
#include <fglib/fg_version.hxx>
#include <fglib/fg_proto.hxx>

namespace fgmp
{

/// Was this relay statically configured (via cli) or
/// dynamically (via network)?
/// Dynamic relays can not be edited/deleted via cli
enum class CONFIG_TYPE
{
	DYNAMIC,
	STATIC
};

//////////////////////////////////////////////////////////////////////

/** An fgms server as seen from fgls
 */
class server
{
public:
	uint64_t        id;             ///< internal ID
	SENDER_TYPE     sender_type;    ///< type of server
	CONFIG_TYPE     config_type;    ///< dynamic/static
	netaddr         addr;           ///< sockaddr of server
	std::string     name;           ///< eg. mpserver01
	std::string     location;       ///< "city/province/country"
	std::string     admin_email;    ///< email address of admin
	fgmp::version   version;        ///< version of server
	time_t          last_seen;
	time_t          registered_at;

	server ();
	bool operator ==  ( const server& s ) const;
	bool operator !=  ( const server& s ) const;
	friend std::ostream& operator << ( std::ostream& o, const server& s );
}; // class server

using server_p   = std::shared_ptr<server>;
using serverlist = fgmp::lock_list_t<server_p>;
using server_it  = serverlist::iterator;

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

#endif
