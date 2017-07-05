//
// This file is part of fgms, the flightgear multiplayer server
// https://sourceforge.net/projects/fgms/
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
// along with this program; if not see <http://www.gnu.org/licenses/>
//

/**
 * @file	fgls.hxx
 *		flightgear list server
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	07/2017
 *
 * This is the list server for the flightgear multiplayer
 * network. All servers register at this server. All clients ask FGLS
 * which server they should use.
 *
 */

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
	fgmp::netaddr		addr;		///< sockaddr of server
	std::string		name;		///< eg. mpserver01
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
	bool	init ();
	void	loop ();
	int	parse_params ( int argc, char* argv[] );
	bool	read_configs ( bool reinit = false );
	void	shutdown ();
	friend class FGLS_CLI;
protected:
	/// Maximum number of concurrent telnets.
	const int MAX_TELNETS = 5;
	/// List of known servers.
	ServerList	m_server_list;
	/// Current selected HUB.
	ServerIt	m_cur_hub;
	/// Next server presented to clients.
	ServerIt	m_cur_fgms;
	/// True in main thread.
	bool		m_is_parent;
	/// Check if server is still allive in this interval.
	/// Can be set in config file.
	int		m_check_interval;
	/// Run in the background?
	/// Can be set in config file.
	bool		m_run_as_daemon;
	/// Add an administrative CLI?
	/// Can be set in config file.
	bool		m_add_cli;
	/// Name of this server.
	/// Must be set in config file.
	std::string	m_server_name;
	/// If set, only listen on this IP.
	/// Can be set in config file.
	std::string	m_bind_addr;
	//////////////////////////////////////////////////
	// The main data channel.
	// Over this channel go all requests
	/// Port to listen for data queries.
	/// Can be set in config file.
	uint16_t	m_data_port;
	/// True if the data channel needs to be (re-) initialised.
	bool		m_reinit_data;	// data channel needs to be reopened
	fgmp::netsocket*	m_data_channel;
	//////////////////////////////////////////////////
	// The telnet port.
	// Over this port fgls can be queried via telnet.
	// fgls will answer with a list of all known servers
	/// Port to listen for queries.
	/// Can be set in config file.
	uint16_t	m_query_port;
	/// True if the query channel needs to be (re-) initialised.
	bool		m_reinit_query;	// query channel needs to be reopened
	fgmp::netsocket*	m_query_channel;
	//////////////////////////////////////////////////
	// The admin channel.
	// Provides an administrative CLI to the server
	/// Port to listen for admin connections.
	/// Can be set in config file.
	uint16_t	m_admin_port;
	/// True if the admin channel needs to be (re-) initialised.
	bool		m_reinit_admin;
	fgmp::netsocket*	m_admin_channel;
	std::string	m_admin_user;
	std::string	m_admin_pass;
	std::string	m_admin_enable;
	/// If true, open a CLI on the terminal.
	/// If running in backgound, this switch is ignored
	bool		m_admin_cli;
	/// log into this file
	std::string	m_logfile_name;
	/// we want to see only logs of this log priority
	int		m_debug_level;
	/// true if logfile needs to be reopened.
	bool		m_reinit_log;
	/// true if have read a config file
	bool		m_have_config;
	bool		m_want_exit;
	time_t		m_uptime;

	bool	init_data_channel ();
	bool	init_query_channel ();
	bool	init_admin_channel ();
	void	open_logfile ();
	void	print_version ();
	void	print_help ();
	bool	process_config ( const std::string & config_name );
	static void* detach_admin_cli ( void* ctx );
	void*	handle_admin ( int fd );

	int	m_argc;	// number of commandline arguments
	char**	m_argv; // pointer to commandline arguments (copy)
}; // class FGLS

typedef struct st_telnet
{
	FGLS* instance;
	int   fd;
} st_telnet;

#endif
