/**
 * @file fg_player.hxx
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

//////////////////////////////////////////////////////////////////////
/** 
 * @class FG_Player
 * @brief Represent a Player
 * 
 * * Player objects are stored in the FG_SERVER::mT_PlayerList
 * * They are created and added in FG_SERVER::AddClient
 * * They are dropped with FG_SERVER::DropClient after expiry time
 * * Clients are added even if they have bad data, see FG_SERVER::AddBadClient
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
	
	/** @brief The network address of this player */
	netAddress    Address;
	
	/** @brief The callsign */
	string        Callsign;
	
	/** @brief The password 
		@warning This is not currently used
	 */
	string        Passwd;
	
	/** @brief The model name */
	string        ModelName;
	
	/** @brief The time this player joined the sessin in utc */
	time_t        JoinTime;
	
	/** @brief ? */
	time_t        Timestamp;
	
	/** @brief The last recorded position */
	Point3D       LastPos;
	
	/** @brief The last recorded orientation */
	Point3D       LastOrientation;
	
	/** @brief \b true is this client is directly connected to this \ref fgms instance */
	bool          IsLocal;
	
	/** @brief The last error message is any 
	 * @see FG_SERVER::AddBadClient
	 */
	string        Error;    // in case of errors
	
	/** @brief \b true if this client has errors
	 * @see FG_SERVER::AddBadClient
	 */
	bool          HasErrors;
	
	/** @brief This client id
	 * @see FG_SERVER::m_MaxClientID
	 */
	int           ClientID;
	time_t        LastRelayedToInactive;
	
	/** @brief Count of packets recieved from client */
	unsigned int  PktsReceivedFrom;  
	
	/** @brief Count of packets sent to client */
	unsigned int  PktsSentTo;        
	
	/** @brief Count of packets from client sent to other players/relays */
	unsigned int  PktsForwarded;    

	FG_Player ();
	FG_Player ( const FG_Player& P);
	~FG_Player ();
	void operator =  ( const FG_Player& P );
private:
	void assign ( const FG_Player& P );
}; // FG_Player


#endif
