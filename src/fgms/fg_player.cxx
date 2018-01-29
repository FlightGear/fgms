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

#include "fg_player.hxx"

namespace fgmp
{

//////////////////////////////////////////////////////////////////////
FG_Player::FG_Player
()
{
	name		= "";
	join_time	= time (0);
	last_seen	= join_time;
	passwd		= "";
	model_name	= "";
	error		= "";
	has_errors	= false;
	do_update	= false;
	is_ATC		= ATC_TYPE::NONE;
	radar_range	= 0;
	proto_major	= 0;
	proto_minor	= 0;
	last_relayed_to_inactive = 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::FG_Player
(
	const std::string& name
)
{
	this->name	= name;
	join_time	= time (0);
	last_seen	= 0;
	passwd		= "";
	model_name 	= "";
	error 		= "";
	has_errors 	= false;
	do_update	= false;
	is_ATC		= ATC_TYPE::NONE;
	radar_range	= 0;
	proto_major	= 0;
	proto_minor	= 0;
	last_relayed_to_inactive = 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// FIXME: apply to the "rule of five" or even better "the rule of zero"
FG_Player::FG_Player
(
	const FG_Player& p
)
{
	this->assign (p);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::~FG_Player
()
{
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_Player::operator =
(
	const FG_Player& p
)
{
	this->assign (p);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
bool
FG_Player::operator ==
(
	const FG_Player& p
)
{
	if ((address == p.address) && (name == p.name))
		return true;
	return false;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_Player::assign
(
	const FG_Player& p
)
{
	//
	// using str.c_str() here to prevent copy-on-write in std::string!
	//
	ListElement::assign (p);
	origin		= p.origin.c_str();
	passwd		= p.passwd.c_str();
	model_name	= p.model_name.c_str();
	join_time	= p.join_time;
	last_seen	= p.last_seen ;
	last_seen	= p.last_seen ;
	last_pos	= p.last_pos;
	geod_pos	= p.geod_pos;
	is_local	= p.is_local;
	is_ATC		= p.is_ATC;
	radar_range	= p.radar_range;
	proto_major	= p.proto_major;
	proto_minor	= p.proto_minor;
	error		= p.error.c_str();
	has_errors	= p.has_errors;
	last_orientation= p.last_orientation;
	do_update	= p.do_update;
	last_relayed_to_inactive = p.last_relayed_to_inactive;
}
//////////////////////////////////////////////////////////////////////

} // namespace fgmp

