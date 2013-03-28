/**
 * @file fg_player.cxx
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

//////////////////////////////////////////////////////////////////////
/** 
 * @class FG_Player
 * @brief A Player Object
 * @author Oliver Schroeder
 */

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
	
	/** @brief Packets recieved from client */
	unsigned int  PktsReceivedFrom;  
	
	/** @brief Packets sent to client */
	unsigned int  PktsSentTo;        
	
	/** @brief Packets from client sent to other players/relays */
	unsigned int  PktsForwarded;    

	FG_Player ();
	FG_Player ( const FG_Player& P);
	~FG_Player ();
	void operator =  ( const FG_Player& P );
private:
	void assign ( const FG_Player& P );
}; // FG_Player


#endif
