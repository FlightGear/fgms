
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
        Origin          = P.Origin;
        Address         = P.Address;
        Callsign        = P.Callsign;
        Passwd          = P.Passwd;
        ModelName       = P.ModelName;
        JoinTime        = P.JoinTime;
        Timestamp       = P.Timestamp;
        LastPos         = P.LastPos;
        IsLocal         = P.IsLocal;
        Error           = P.Error;
        HasErrors       = P.HasErrors;
        ClientID        = P.ClientID;
        PktsSentTo      = P.PktsSentTo;
        PktsForwarded   = P.PktsForwarded;
        LastOrientation         = P.LastOrientation;
        PktsReceivedFrom        = P.PktsReceivedFrom;
        LastRelayedToInactive   = P.LastRelayedToInactive;
}
//////////////////////////////////////////////////////////////////////


