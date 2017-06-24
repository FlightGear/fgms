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
FG_Player::FG_Player()
{
	Name		= "";
	JoinTime	= time (0);
	LastSeen	= JoinTime;
	LastSent	= 0;
	Passwd		= "";
	ModelName	= "";
	Error		= "";
	HasErrors	= false;
	DoUpdate	= false;
	IsATC		= ATC_NONE;
	RadarRange	= 0;
	ProtoMajor	= 0;
	ProtoMinor	= 0;
	LastRelayedToInactive = 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::FG_Player( const string& Name )
{
	this->Name	= Name;
	JoinTime	= time (0);
	LastSeen	= JoinTime;
	LastSent	= 0;
	Passwd		= "";
	ModelName 	= "";
	Error 		= "";
	HasErrors 	= false;
	DoUpdate	= false;
	IsATC		= ATC_NONE;
	RadarRange	= 0;
	ProtoMajor	= 0;
	ProtoMinor	= 0;
	LastRelayedToInactive = 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::FG_Player( const FG_Player& P )
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::~FG_Player()
{
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_Player::operator =( const FG_Player& P )
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
bool
FG_Player::operator ==( const FG_Player& P )
{
	if ((Address == P.Address) && (Name == P.Name))
		return true;
	return false;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_Player::assign( const FG_Player& P )
{
	//
	// using str.c_str() here to prevent copy-on-write in std::string!
	//
	ListElement::assign (P);
	Origin = P.Origin.c_str();
	Passwd = P.Passwd.c_str();
	ModelName = P.ModelName.c_str();
	JoinTime = P.JoinTime;
	LastSeen = P.LastSeen ;
	LastSent = P.LastSent ;
	LastPos = P.LastPos;
	GeodPos = P.GeodPos;
	IsLocal = P.IsLocal;
	IsATC = P.IsATC;
	RadarRange = P.RadarRange;
	ProtoMajor = P.ProtoMajor;
	ProtoMinor = P.ProtoMinor;
	Error = P.Error.c_str();
	HasErrors = P.HasErrors;
	LastOrientation	= P.LastOrientation;
	DoUpdate	= P.DoUpdate;
	LastRelayedToInactive = P.LastRelayedToInactive;
}
//////////////////////////////////////////////////////////////////////

} // namespace fgmp

