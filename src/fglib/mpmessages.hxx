/**
 * @file mpmessages.hxx 
 * @author  Duncan McCreanor
 * 
 * @brief Message definitions for multiplayer communications
 *        within multiplayer Flightgear
 * 
 * - Each message used for multiplayer communications consists of a header
 *   and optionally a block of data. 
 * - The combined header and data is sent as one IP packet, \ref xdr encoded.
 *  @note 
 * 		XDR demands 4 byte alignment, but some compilers use 8 byte
 * 		alignment, so it's safe to let the overall size of a network
 * 		message be a multiple of 8! 
 */

//
//  Written by Duncan McCreanor, started February 2003.
// duncan.mccreanor@airservicesaustralia.com
//
// Copyright (C) 2003  Airservices Australia
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef MPMESSAGES_H
#define MPMESSAGES_H

#include <vector>
#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>
#include "encoding.hxx"

/**
 * @brief  IDs of message types seen by fgms
 */
namespace fgmp
{

/**
 * Message identifiers
 */
enum class MSG_ID
{
	//@{ deprecated message ids
	CHAT_MSG = 1,	// old CHAT_MSG_ID
	/// a ping packet is send verbatim back to sender
	PING,	// request
	PONG,	// answer
	U4,	// old pos data,
	U5,	// old pos data,
	U6,	// RESET_DATA_ID
	//@}
	/// a "position" message, and the most trafficked
	POS_DATA,
	MP_2017_DATA_ID
};

/** @brief The `Magic` value for message (currently FGFS). The magic is at
 * the start of every packet and is used for packet validation.
 */
constexpr uint32_t MSG_MAGIC { 0x46474653 };  // "FGFS"
/** @brief  The MP protocol version that is send with each packet
 * (currently 1.1).
 */
constexpr uint32_t PROTO_VER { 0x00010001 };  // 1.1

/** @brief  Maximum length of a callsign */
constexpr uint32_t MAX_CALLSIGN_LEN { 8 };

/** @brief  Maximum length of a chat message 
 */
constexpr uint32_t MAX_CHAT_MSG_LEN { 256 };

/** @brief  Maximum length of a model name, eg /model/x17/aero-123.xml */
constexpr uint32_t MAX_MODEL_NAME_LEN { 96 };

/** @brief  Maximum length of property */
constexpr uint32_t MAX_PROPERTY_LEN { 52 };


/** 
 * @struct msg_hdr
 * @brief The header sent as the first part of all mp message packets.
 * 
 * The header is expected to have the correct ::MSG_MAGIC and ::PROTO_VER
 * and is checked upon in FG_SERVER::PacketIsValid
 */
struct msg_hdr_t
{
	/** @brief Magic Value */
	xdr_data_t  magic;   
	/** @brief Protocol version */
	xdr_data_t  version;           
	/** @brief Message identifier  */
	xdr_data_t  msg_id;     
	/** @brief Absolute length of message */
	xdr_data_t  msg_len;    
	/** @brief Player's radar range */
	xdr_data_t  radar_range;   
	/** @brief Player's receiver port
	 * @deprecated Not used in current implementation
	 * set to zero
	 */
	xdr_data_t  reply_port;   
	/** @brief Callsign used by the player */
	char name[MAX_CALLSIGN_LEN]; 
};

/** 
 * @struct pos_msg_t
 * @brief A Position Message
 */
struct pos_msg_t
{
	/** @brief  Name of the aircraft model */
	char model[MAX_MODEL_NAME_LEN]; 
	/** @brief Time when this packet was generated */
	xdr_data2_t time;
	/** @brief Time when this packet was generated */
	xdr_data2_t lag;
	/** @brief Position wrt the earth centered frame */
	xdr_data2_t position[3];
	/** @brief Orientation wrt the earth centered frame, stored in
	 * the angle axis representation where the angle is coded into
	 * the axis length
	 */
	xdr_data_t orientation[3];
	/** @brief Linear velocity wrt the earth centered frame measured in
	 *         the earth centered frame
	 */
	xdr_data_t linear_vel[3];
	/** @brief Angular velocity wrt the earth centered frame measured in
	 *          the earth centered frame
	 */
	xdr_data_t angular_vel[3];
	/** @brief Linear acceleration wrt the earth centered frame measured
	 *         in the earth centered frame
	 */
	xdr_data_t linear_accel[3];
	/** @brief Angular acceleration wrt the earth centered frame measured
	 *         in the earth centered frame
	 */
	xdr_data_t angular_accel[3];
};

/** 
 * @struct prop_msg_t 
 * @brief Property Message 
 */
struct prop_msg_t
{
	xdr_data_t id;
	xdr_data_t value;
};

/**
 * @struct float_prop_data_t  
 * @brief Property Data 
 */
struct float_prop_data_t
{
	unsigned id;
	float value;
};

/** @brief Motion Message */
struct motion_data_t
{
	/** 
	 * @brief Simulation time when this packet was generated 
	 */
	double time;
	// the artificial lag the client should stay behind the average
	// simulation time to arrival time difference
	// FIXME: should be some 'per model' instead of 'per packet' property
	double lag;
	/** 
	*  Position wrt the earth centered frame  
	*/
	SGVec3d position;
	/** @brief Orientation wrt the earth centered frame */
	SGQuatf orientation;
	/**
	* @brief Linear velocity wrt the earth centered frame measured in
	*        the earth centered frame
	*/
	SGVec3f linearVel;
	/** 
	* @brief Angular velocity wrt the earth centered frame measured
	*        in the earth centered frame
	*/
	SGVec3f angularVel;
	/** @brief Linear acceleration wrt the earth centered frame
	 *         measured in the earth centered frame
	 */
	SGVec3f linearAccel;
	/** @brief Angular acceleration wrt the earth centered frame
	 *         measured in the earth centered frame
	 */
	SGVec3f angularAccel;
	/** @brief The set of properties recieved for this timeslot */
	std::vector<float_prop_data_t> properties;
};

} // namespace fgmp

#endif
