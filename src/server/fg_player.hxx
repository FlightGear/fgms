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

//////////////////////////////////////////////////////////////////////
//
//  a player object
//
//////////////////////////////////////////////////////////////////////

#if !defined FG_PLAYER_HXX
#define FG_PLAYER_HXX

#include <string>
#include <time.h>
#include <plib/netSocket.h>
#include <simgear/debug/logstream.hxx>
#include "fg_geometry.hxx"

using namespace std;

class FG_Player
{
public:
	string        Origin;
	netAddress    Address;
	string        Callsign;
	string        Passwd;
	string        ModelName;
	time_t        JoinTime;
	time_t        Timestamp;
	Point3D       LastPos;
	Point3D       LastOrientation;
	bool          IsLocal;
	string        Error;    // in case of errors
	bool          HasErrors;
	int           ClientID;
	time_t        LastRelayedToInactive;
	unsigned int  PktsReceivedFrom;  // From client
	unsigned int  PktsSentTo;        // Sent to client
	unsigned int  PktsForwarded;     // From client sent to other players/relays

	FG_Player ();
	FG_Player ( const FG_Player& P);
	~FG_Player ();
	void operator =  ( const FG_Player& P );
private:
	void assign ( const FG_Player& P );
}; // FG_Player


#endif
