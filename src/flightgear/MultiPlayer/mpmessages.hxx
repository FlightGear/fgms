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
#include "tiny_xdr.hxx"

/** @brief  ID  of a "chat" message */
#define CHAT_MSG_ID             1

/** @brief The `Magic` value for message (currently FGFS). The magic is at
 * the start of every packet and is used for packet validation.
 */
const uint32_t MSG_MAGIC = 0x46474653;  // "FGFS"

/** @brief  The MP protocol version that is send with each packet
 * (currently 1.1).
 */
const uint32_t PROTO_VER = 0x00010001;  // 1.1

/**
 * @brief  IDs of message types seen by fgms
 */
namespace FGFS
{

/**
 * Message identifiers
 */
typedef enum
{
	//@{ deprecated message ids
	U1 = 1,	// old CHAT_MSG_ID
	U2,	// old pos data
	U3,	// old pos data,
	U4,	// old pos data,
	U5,	// old pos data,
	U6,	// RESET_DATA_ID
	//@}
	/// a "position" message, and the most trafficked
	POS_DATA,
	MP_2017_DATA_ID,
	/// a ping packet is send verbatim back to sender
	PING,	// request
	PONG,	// answer
	/// reply with a server info packet
	RQ_SERVER_INFO,	// request
	SERVER_INFO,	// answer
} MSG_ID;

} // namspace FGFS

/** @brief  Maximum length of a callsign */
#define MAX_CALLSIGN_LEN        8

/** @brief  Maximum length of a chat message 
 */
#define MAX_CHAT_MSG_LEN        256

/** @brief  Maximum length of a model name, eg /model/x17/aero-123.xml */
#define MAX_MODEL_NAME_LEN      96

/** @brief  Maximum length of property */
#define MAX_PROPERTY_LEN        52


/** 
 * @struct T_MsgHdr
 * @brief The header sent as the first part of all mp message packets.
 * 
 * The header is expected to have the correct ::MSG_MAGIC and ::PROTO_VER
 * and is checked upon in FG_SERVER::PacketIsValid
 */
struct T_MsgHdr
{
	/** @brief Magic Value */
	xdr_data_t  Magic;   
	/** @brief Protocol version */
	xdr_data_t  Version;           
	/** @brief Message identifier  */
	xdr_data_t  MsgId;     
	/** @brief Absolute length of message */
	xdr_data_t  MsgLen;    
	/** @brief Player's radar range */
	xdr_data_t  RadarRange;   
	/** @brief Player's receiver port
	 * @deprecated Not used in current implementation
	 * set to zero
	 */
	xdr_data_t  ReplyPort;   
	/** @brief Callsign used by the player */
	char Name[MAX_CALLSIGN_LEN]; 
};

/** 
 * @struct T_PositionMsg
 * @brief A Position Message
 */
struct T_PositionMsg
{
	/** @brief  Name of the aircraft model */
	char Model[MAX_MODEL_NAME_LEN]; 
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
	xdr_data_t linearVel[3];
	/** @brief Angular velocity wrt the earth centered frame measured in
	 *          the earth centered frame
	 */
	xdr_data_t angularVel[3];
	/** @brief Linear acceleration wrt the earth centered frame measured
	 *         in the earth centered frame
	 */
	xdr_data_t linearAccel[3];
	/** @brief Angular acceleration wrt the earth centered frame measured
	 *         in the earth centered frame
	 */
	xdr_data_t angularAccel[3];
};

/** 
 * @struct T_PropertyMsg 
 * @brief Property Message 
 */
struct T_PropertyMsg
{
	xdr_data_t id;
	xdr_data_t value;
};

/**
 * @struct FGFloatPropertyData  
 * @brief Property Data 
 */
struct FGFloatPropertyData
{
	unsigned id;
	float value;
};

/** @brief Motion Message */
struct FGExternalMotionData
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
	* @brief Angular velocity wrt the earth centered frame measured in the earth centered frame
	*/
	SGVec3f angularVel;
	/** @brief Linear acceleration wrt the earth centered frame measured in the earth centered frame */
	SGVec3f linearAccel;
	/** @brief Angular acceleration wrt the earth centered frame measured in the earth centered frame */
	SGVec3f angularAccel;
	/** @brief The set of properties recieved for this timeslot */
	std::vector<FGFloatPropertyData> properties;
};

#endif
