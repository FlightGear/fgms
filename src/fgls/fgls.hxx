//
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

///
/// @file FGLS.hxx
/// flightgear list server
///
/// @author     Oliver Schroeder <fgms@o-schroeder.de>
/// @date       2017
/// @copyright  GPLv2
///
/// This is the list server for the flightgear multiplayer
/// network. All servers register at this server. All clients ask FGLS
/// which server they should use.
///

#ifndef FGLS_HEADER
#define FGLS_HEADER

#include <pthread.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <fstream>
#include <fglib/daemon.hxx>
#include <fglib/fg_thread.hxx>
#include <fglib/fg_list.hxx>
#include <fglib/fg_version.hxx>
#include <fglib/fg_proto.hxx>

//////////////////////////////////////////////////////////////////////
/** An fgms server as seen from FGLS
 */
class server
{
public:
	uint64_t		id;		///< internal ID
	fgms::eCLIENTTYPES	type;		///< type of server
	NetAddr			addr;		///< sockaddr of server
	std::string		name;		///< eg. mpserver01.flightgear.org
	std::string		location;	///< "city/province/country"
	std::string		admin;		///< email address of admin
	FG_VERSION		version;	///< version of server
	time_t			last_seen;
	time_t			registered_at;

	server ();
	friend std::ostream& operator << ( std::ostream& o, const server& s );
}; // class server

typedef fgmp::lock_list_t<server>		ServerList;
typedef fgmp::lock_list_t<server>::iterator	ServerIt;

//////////////////////////////////////////////////////////////////////
/** The List server
 */
class FGLS
{
public:
	static const FG_VERSION m_version;
	FGLS ();
	bool	Init ();
	void	Loop ();
	int	ParseParams ( int argc, char* argv[] );
	void	ShutDown ();
	friend class FGLS_cli;
private:
	const int MAX_TELNETS = 5;
	ServerList	m_ServerList;
	ServerIt	m_CurrentHUB;
	ServerIt	m_CurrentFGMS;
	std::string	m_BindAddress;
	// general
	bool		m_IsParent;	// true in main thread
	// the main data channel
	uint16_t	m_DataPort;
	bool		m_ReinitData;	// data channel needs to be reopened
	NetSocket*	m_DataChannel;
	// the telnet port
	uint16_t	m_QueryPort;
	bool		m_ReinitQuery;	// query channel needs to be reopened
	NetSocket*	m_QueryChannel;
	// the admin channel
	uint16_t	m_AdminPort;
	bool		m_ReinitAdmin;	// admin channel needs to be reopened
	NetSocket*	m_AdminChannel;
	bool		m_AdminCLI;	// open a CLI on terminal
	// logging
	std::string	m_LogFileName;
	int		m_DebugLevel;
	bool		m_ReinitLog;	// logfile needs to be reopened
	bool InitDataChannel ();
	bool InitQueryChannel ();
	bool InitAdminChannel ();
	void OpenLogfile ();
}; // class FGLS

#endif
