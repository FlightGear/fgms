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

#ifndef PILOT_HXX
#define PILOT_HXX

#include <fglib/fg_list.hxx>

namespace fgmp
{

enum class ATC_TYPE
{
	NONE,		// not an ATC
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
 * @class pilot
 * @brief Represent a pilot
 *
 * pilot objects are stored in the fgms::m_pilot_list
 * They are created and added in FGMS::AddClient
 * They are dropped with FGMS::DropClient after expiry time
 * Clients are added even if they have bad data, see FGMS::AddBadClient
 */
class pilot : public list_item
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
	point3d	last_pos;
	/** @brief The last recorded position in geodectic coordinates (lat/lon/alt) */
	point3d	geod_pos;
	/** @brief The last recorded orientation */
	point3d	last_orientation;
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
	pilot ();
	pilot ( const std::string& name );
	pilot ( const pilot& p );
	void operator =  ( const pilot& p );
	virtual bool operator ==  ( const pilot& p );
private:
	void assign ( const pilot& p );
}; // pilot

using pilot_list = list<pilot>;
using pilot_it   = std::vector<pilot>::iterator;

} // namepsace fgmp

#endif
