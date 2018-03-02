// Copyright (C) Oliver Schroeder <fgms@postrobot.de>
//
// This file is part of fgms, the flightgear multiplayer server
//
// fgms is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// fgms is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fgms.  If not, see <http://www.gnu.org/licenses/>
//

/// @file fg_proto.hxx
/// A class for handling the multiplayer protocol
///
/// @author     Oliver Schroeder <fgms@o-schroeder.de>
/// @date       2017
/// @copyright  GPLv2
///

#ifndef FG_PROTO_HEADER
#define FG_PROTO_HEADER

#include <fglib/netsocket.hxx>
#include <fglib/encoding.hxx>

namespace fgmp
{

/// client types
enum class sender_type
{
        UNSET,       // unset
        FGFS    = 0x46474653,           // a normal client
        FGMS    = 0x53464746,           // FlightGear Multiplayer Server
        FGAS,           // FlightGear Authentication Service
        FGLS,           // FlightGear List Server
        OBSERVER        // an observer
};

enum fgmp_commands
{
        REGISTER_SERVER,///< server requests to be part of our network
        REGISTER_FGFS,  ///< fgfs requests to be part of our network
        SERVER_QUIT,    ///< tell clients that I (a server) quit the network
        FGFS_QUIT,      ///< tell clients that I (a fgfs) quit the network
        REGISTER_OK,    ///< tell server that it is registered
        GENERAL_ERROR,  ///< inform client of an error
        SET_CLIENT_ID,  ///< tell client its ID
        AUTH_CLIENT,    ///< fgms sends auth-request to fgas
        AUTH_OK,        ///< fgas tells fgms authentication is OK
        AUTH_FAILED,    ///< tell client about authentication failure
        AUTHENTICATED,  ///< fgms tells fgfs authentication is OK
        PING,           ///< ping a client
        PONG,           ///< client responds with pong
        USE_FGAS,       ///< tell fgms which fgas to use
        NO_FGAS,        ///< tell fgms that there is no fgas currently online
        YOU_ARE_HUB,    ///< tell fgms that is the HUB server
        YOU_ARE_LEAVE,  ///< tell fgms that is a LEAVE server
        NEW_SERVER,     ///< tell fgls about a new server
        REQUEST_SERVER, ///< fgfs searching for a server
        USE_SERVER,     ///< tell fgfs which server to use
        REGISTER_AGAIN, ///< tell a client to register again
        INTERNAL_COMMAND ///< mark the end of the list. Used to check if it is a command
};


} // namespace fgmp

#endif
