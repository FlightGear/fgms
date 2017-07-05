/**
 * @file fg_list.hxx
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
// Copyright (C) 2005-2013  Oliver Schroeder
//

/**
 * @file fg_list.hxx
 * @author Oliver Schroeder
 *
 * Implementation of a thread-safe list.
 */

#ifndef FG_PLAYER_HXX
#define FG_PLAYER_HXX

#include <fglib/fg_list.hxx>

namespace fgmp
{

//////////////////////////////////////////////////////////////////////
/** 
 * @class FG_Player
 * @brief Represent a Player
 * 
 * Player objects are stored in the FGMS::m_PlayerList
 * They are created and added in FGMS::AddClient
 * They are dropped with FGMS::DropClient after expiry time
 * Clients are added even if they have bad data, see FGMS::AddBadClient
 */
class FG_Player : public ListElement
{
public:
	typedef enum
	{
		ATC_NONE,	// not an ATC
		ATC,		// undefined ATC
		ATC_DL,		// Clearance Delivery
		ATC_GN,		// Ground
		ATC_TW,		// Tower
		ATC_AP,		// Approach
		ATC_DE,		// Departure
		ATC_CT		// Center
	} ATC_TYPE;
	std::string	Origin;
	/** @brief The password 
	 *  @warning This is not currently used
	 */
	std::string	Passwd;
	/** @brief The model name */
	std::string	ModelName;
	/** @brief The last recorded position */
	Point3D	LastPos;
	/** @brief The last recorded position in geodectic coordinates (lat/lon/alt) */
	Point3D	GeodPos;
	/** @brief The last recorded orientation */
	Point3D	LastOrientation;
	/** @brief \b true if this client is directly connected to this \ref fgms instance */
	bool	IsLocal;
	/** @brief \b true if this client is an ATC */
	ATC_TYPE IsATC;
	/** @brief client provided radar range */
	uint16_t RadarRange;
	/** @brief client major protocol version */
	uint16_t ProtoMajor;
	/** @brief client minor protocol version */
	uint16_t ProtoMinor;
	/** @brief in case of errors the reason is stored here 
	 * @see FGMS::AddBadClient
	 */
	std::string	Error;    // in case of errors
	/** @brief \b true if this client has errors
	 * @see FGMS::AddBadClient
	 */
	bool	HasErrors;
	/** when did we sent updates of this player to inactive relays */
	time_t	LastRelayedToInactive;
	/** \b true if we need to send updates to inactive relays */
	bool	DoUpdate;
	FG_Player ();
	FG_Player ( const std::string& Name );
	FG_Player ( const FG_Player& P);
	~FG_Player ();
	void operator =  ( const FG_Player& P );
	virtual bool operator ==  ( const FG_Player& P );
private:
	void assign ( const FG_Player& P );
}; // FG_Player

typedef List<FG_Player>			PlayerList;
typedef std::vector<FG_Player>::iterator	PlayerIt;

} // namepsace fgmp

#endif
