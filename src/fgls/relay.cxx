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
 * @file        relay.cxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        03/2018
 */

#include "relay.hxx"

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

server::server
()
{
	id              = ( size_t ) -1;
	sender_type     = SENDER_TYPE::UNSET;
	config_type     = CONFIG_TYPE::DYNAMIC;
	//name
	//location
	//admin
	//version
	last_seen       = 0;
	registered_at   = time ( 0 );
} // server::server()

//////////////////////////////////////////////////////////////////////

bool
server::operator ==
(
	const server& s
) const
{
	if ( ( addr == s.addr ) && ( name == s.name ) )
	{
		return true;
	}
	return false;
} // server::operator ==

//////////////////////////////////////////////////////////////////////


bool
server::operator !=
(
	const server& s
) const
{
	if ( ( addr == s.addr ) && ( name == s.name ) )
	{
		return false;
	}
	return true;
} // server::operator !=

//////////////////////////////////////////////////////////////////////

} // namespace fgmp
