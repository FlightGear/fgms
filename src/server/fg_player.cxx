/**
 * @file fg_player.cxx
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

#include "fg_player.hxx"

//////////////////////////////////////////////////////////////////////
FG_Player::FG_Player
()
{
        Callsign        = "";
        Passwd          = "";
        ModelName       = "";
        JoinTime        = 0;
        Timestamp       = 0;
        Error           = "";
        HasErrors       = false;
        ClientID        = 0;
        PktsSentTo      = 0;
        PktsForwarded   = 0;
        PktsReceivedFrom        = 0;
        LastRelayedToInactive   = 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::FG_Player
(
	const FG_Player& P
)
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::~FG_Player
()
{
	//SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_Player::~FG_Player(" << pthread_self() << ") - " << this->Callsign);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_Player::operator =
(
	const FG_Player& P
)
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_Player::assign
(
	const FG_Player& P
)
{
	//SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_Player::assign(" << pthread_self() << ") - " << P.Callsign);
	//
	// using str.c_str() here to prevent copy-on-write in std::string!
	//
        Origin          = P.Origin.c_str();
        Address         = P.Address;
        Callsign        = P.Callsign.c_str();
        Passwd          = P.Passwd.c_str();
        ModelName       = P.ModelName.c_str();
        JoinTime        = P.JoinTime;
        Timestamp       = P.Timestamp;
        LastPos         = P.LastPos;
        IsLocal         = P.IsLocal;
        Error           = P.Error.c_str();
        HasErrors       = P.HasErrors;
        ClientID        = P.ClientID;
        PktsSentTo      = P.PktsSentTo;
        PktsForwarded   = P.PktsForwarded;
        LastOrientation         = P.LastOrientation;
        PktsReceivedFrom        = P.PktsReceivedFrom;
        LastRelayedToInactive   = P.LastRelayedToInactive;
	#if 0
	void* p;
	void* q;
	p = (void*)   Callsign.c_str();
	q = (void*) P.Callsign.c_str();
	cout << " assign p: " << p << " q: " << q << endl;
	#endif
}
//////////////////////////////////////////////////////////////////////


