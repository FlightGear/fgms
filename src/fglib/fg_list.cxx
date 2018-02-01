/**
 * @file fg_list.cxx
 * @author Oliver Schroeder
 */
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
// Copyright (C) 2005-2012  Oliver Schroeder
//

#ifdef HAVE_CONFIG_H
#include "config.h" // for MSVC, always first
#endif

#include <time.h>
#include "fg_list.hxx"

namespace fgmp
{

const size_t list_item::NONE_EXISTANT = (size_t) -1;

//////////////////////////////////////////////////////////////////////

list_item::list_item
(
	const std::string& name
) : name(name)
{
	id		= NONE_EXISTANT;	// flag a non existant item
	timeout		= 0;
	join_time	= time (0);
	last_seen	= join_time;
	last_sent	= join_time;
	pkts_sent	= 0;
	pkts_rcvd	= 0;
	bytes_rcvd	= 0;
	bytes_sent	= 0;
} // list_item::list_item ( name )

//////////////////////////////////////////////////////////////////////

list_item::list_item
() : name("")
{
	id		= NONE_EXISTANT;	// flag a non existant item
	timeout		= 0;
	join_time	= time (0);
	last_seen	= join_time;
	last_sent	= join_time;
	pkts_sent	= 0;
	pkts_rcvd	= 0;
	bytes_rcvd	= 0;
	bytes_sent	= 0;
} // list_item::list_item ()

//////////////////////////////////////////////////////////////////////

list_item::list_item
(
	const list_item& P
)
{
	this->assign (P);
} // list_item::list_item ( & list_item )

//////////////////////////////////////////////////////////////////////

void
list_item::operator =
(
	const list_item& P
)
{
	this->assign (P);
} // list_item::operator = 

//////////////////////////////////////////////////////////////////////

bool
list_item::operator ==
(
	const list_item& P
)
{
	// FIXME: compare the name, too?
	if (address == P.address)
		return true;
	return false;
} // list_item::operator ==

//////////////////////////////////////////////////////////////////////

void
list_item::assign
(
	const list_item& P
)
{
	id		= P.id;
	timeout		= P.timeout;
        name		= P.name.c_str();
        address 	= P.address;
        join_time 	= P.join_time;
        last_seen	= P.last_seen;
        last_sent	= P.last_sent;
        pkts_sent	= P.pkts_sent;
	bytes_sent	= P.bytes_sent;
        pkts_rcvd	= P.pkts_rcvd;
	bytes_rcvd	= P.bytes_rcvd;
} // list_item::assign ()

//////////////////////////////////////////////////////////////////////

void
list_item::update_sent
(
	size_t bytes
)
{
	pkts_sent++;
	bytes_sent += bytes;
	last_sent = time(0);
} // list_item::update_sent ()

//////////////////////////////////////////////////////////////////////

void
list_item::update_rcvd
(
	size_t bytes
)
{
	pkts_rcvd++;
	bytes_rcvd += bytes;
	last_seen = time(0);
} // list_item::update_rcvd ()

//////////////////////////////////////////////////////////////////////

} // namespace fgmp
