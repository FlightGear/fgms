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

	enum class ATC_TYPE
	{
		NONE,	// not an ATC
		ATC,		// undefined ATC
		ATC_DL,		// Clearance Delivery
		ATC_GN,		// Ground
		ATC_TW,		// Tower
		ATC_AP,		// Approach
		ATC_DE,		// Departure
		ATC_CT		// Center
	};

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

	std::string	origin;
	/** @brief The password 
	 *  @warning This is not currently used
	 */
	std::string	passwd;
	/** @brief The model name */
	std::string	model_name;
	/** @brief The last recorded position */
	Point3D	last_pos;
	/** @brief The last recorded position in geodectic coordinates (lat/lon/alt) */
	Point3D	geod_pos;
	/** @brief The last recorded orientation */
	Point3D	last_orientation;
	/** @brief \b true if this client is directly connected to this \ref fgms instance */
	bool	is_local;
	/** @brief \b true if this client is an ATC */
	ATC_TYPE is_ATC;
	/** @brief client provided radar range */
	uint16_t radar_range;
	/** @brief client major protocol version */
	uint16_t proto_major;
	/** @brief client minor protocol version */
	uint16_t proto_minor;
	/** @brief in case of errors the reason is stored here 
	 * @see FGMS::AddBadClient
	 */
	std::string	error;    // in case of errors
	/** @brief \b true if this client has errors
	 * @see FGMS::AddBadClient
	 */
	bool	has_errors;
	/** when did we sent updates of this player to inactive relays */
	time_t	last_relayed_to_inactive;
	/** \b true if we need to send updates to inactive relays */
	bool	do_update;
	FG_Player ();
	FG_Player ( const std::string& name );
	FG_Player ( const FG_Player& p);
	virtual ~FG_Player ();
	void operator =  ( const FG_Player& p );
	virtual bool operator ==  ( const FG_Player& p );
private:
	void assign ( const FG_Player& p );
}; // FG_Player

using PlayerList = List<FG_Player>;
using PlayerIt   = std::vector<FG_Player>::iterator;

} // namepsace fgmp

#endif
