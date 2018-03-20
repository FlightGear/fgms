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
 * @file        fgms.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2005-2017
 */

//////////////////////////////////////////////////////////////////////
//
//  server for FlightGear
//
//////////////////////////////////////////////////////////////////////

#ifndef FGMS_HXX
#define FGMS_HXX

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <fglib/netsocket.hxx>
#include <fglib/encoding.hxx>
#include <fglib/mpmessages.hxx>
#include <fglib/fg_version.hxx>
#include <fglib/fg_geometry.hxx>
#include <fglib/fg_list.hxx>
#include "fg_player.hxx"
#include "fg_tracker.hxx"

#ifndef _MSC_VER
#include <fglib/daemon.hxx>
#endif

namespace fgmp
{

//////////////////////////////////////////////////////////////////////
/**
 * @class fgms
 * @brief The Main fgms Class
 */
class fgms
{
public:

	static const version m_version; // ( 1, 0, 0, "-dev" );

	friend class fgcli;

	/** @brief Internal Constants */
	enum
	{
		// other constants
		MAX_PACKET_SIZE		= 1200, // to agree with FG multiplayermgr.cxx (since before  2008)
		UPDATE_INACTIVE_PERIOD	= 1,
		MAX_TELNETS		= 5,
		RELAY_MAGIC		= 0x53464746    // GSGF
	};
	//////////////////////////////////////////////////
	//
	//  constructors
	//
	//////////////////////////////////////////////////
	fgms ();
	~fgms ();

	//////////////////////////////////////////////////
	//
	//  public methods
	//
	//////////////////////////////////////////////////
	bool  init ();
	bool  loop ();
	void  shutdown ();

	void  set_data_port ( int port );
	void  set_query_port ( int port );
	void  set_admin_port ( int port );
	void  open_logfile ();
	void  set_exitfile ( const std::string& name );
	void  set_resetfile ( const std::string& name );
	void  set_statsfile ( const std::string& name );
	void  set_msglogfile ( const std::string& name );
	void  set_updatetracker ( time_t freq );
	void  add_relay ( const std::string& server, int port );
	void  add_crossfeed ( const std::string& server, int port );
	bool  add_tracker ( const std::string& server,
			int port, bool is_tracked );
	void  add_whitelist  ( const std::string& ip );
	void  add_blacklist  ( const std::string& ip, const std::string& reason,
			time_t timeout = 10 );
	void  close_tracker ();
	int   check_files();
	void  show_stats ();
	void* handle_query  ( int fd );
	void* handle_admin  ( int fd );
	void  parse_params ( int argc, char* argv[] );
	bool  read_configs ();
	bool  check_config ();
	bool  process_config ( const std::string& config_name );

	//////////////////////////////////////////////////
	//
	//  public variables
	//
	//////////////////////////////////////////////////

protected:

	#ifndef _MSC_VER
	daemon m_myself;
	#endif
	using ip2relay_t  = std::map<netaddr,std::string>;
	using ip2relay_it = ip2relay_t::iterator;
	//////////////////////////////////////////////////
	//
	//  private variables
	//
	//////////////////////////////////////////////////
	bool		m_reinit_data		= true;
	bool		m_reinit_query		= true;
	bool		m_reinit_admin		= true;
	bool		m_reinit_log		= true;
	bool		m_listening		= false;
	bool		m_is_parent		= false;
	bool		m_have_config		= false;
	bool		m_cli_enabled		= true;
	bool		m_run_as_daemon		= false;
	bool		m_hub_mode		= false;
	bool		m_want_exit		= false;
	int		m_data_port		= 5000;
	int		m_query_port		= m_data_port + 1;
	int		m_player_expires	= 10;
	int		m_out_of_reach		= 100;
	int		m_max_radar_range	= 2000;
	fglog::prio	m_debug_level		= fglog::prio::MEDIUM;
	std::string	m_logfile_name		= "fgms.log";
	std::string	m_exit_filename		= "fgms_exit";
	std::string	m_reset_filename	= "fgms_reset";
	std::string	m_stats_filename	= "fgms_stats";
	std::string	m_bind_addr		= "";
	std::string	m_FQDN			= "local";
	std::string	m_hostname		= "fgms";
	size_t		m_num_max_clients	= 0;
	size_t		m_local_clients		= 0;
	size_t		m_remote_clients	= 0;
	int16_t		m_proto_minor_version	= 1;
	int16_t		m_proto_major_version	= 1;
	ip2relay_t      m_relay_map;
	fglist		m_cross_list;
	fglist		m_white_list;
	fglist		m_black_list;
	fglist		m_relay_list;
	netsocket*	m_data_channel  = nullptr;
	netsocket*	m_query_channel = nullptr;
	netsocket*	m_admin_channel = nullptr;
	pilot_list	m_player_list;

	// for the cli
	uint16_t	m_cli_port;

	// for the tracker module
	tracker*	m_tracker		= nullptr;
	std::ofstream	m_tracker_log;
	bool		m_tracker_enabled	= false;
	std::string	m_tracker_logname	= "tracker.log";
	time_t		m_tracker_freq		= 10;
	std::string	m_tracker_server	= "localhost";
	int		m_tracker_port		= 8000;
	bool		m_tracker_reinit	= false;


	//////////////////////////////////////////////////
	// 20150619:0.11.9: be able to disable these functions
	bool    m_use_exit_file;
	bool    m_use_reset_file;
	bool    m_use_stat_file;
	//////////////////////////////////////////////////
	//
	//  statistics
	//
	//////////////////////////////////////////////////
	size_t  m_packets_received   = 0; ///< # data packets received
	size_t  m_ping_received      = 0; ///< # ping packets received
	size_t  m_pong_received      = 0; ///< # pong packets received
	size_t  m_black_rejected     = 0; ///< # entries in black list
	size_t  m_packets_invalid    = 0; ///< # invalid packets received
	size_t  m_unknown_relay      = 0; ///< # packets from unknown relays
	size_t  m_relay_magic        = 0; ///< # packets from known relays
	size_t  m_pos_data           = 0; ///< # position data packets
	size_t  m_unknown_data       = 0; ///< # packets with unknown data
	size_t  m_queries_received   = 0; ///< # queries via telnet
	size_t  m_admin_received     = 0; ///< # admin connections
	size_t  m_tracker_connect    = 0; ///< # of connections to tracker
	size_t  m_tracker_disconnect = 0; ///< # of disconnects for tracker
	size_t  m_tracker_position   = 0; ///< # of pos packets sent to tracker
	time_t  m_uptime             = 0; ///< unix timestamp of server start

	// totals of the above
	size_t  m_t_packets_received = 0;
	size_t  m_t_black_rejected   = 0;
	size_t  m_t_packets_invalid  = 0;
	size_t  m_t_unknown_relay    = 0;
	size_t  m_t_pos_data         = 0;
	size_t  m_t_telnet_received  = 0;
	size_t  m_t_relay_magic      = 0;
	size_t  m_t_unknown_data     = 0;
	size_t  m_t_cross_failed     = 0;
	size_t  m_t_cross_sent       = 0;

	//////////////////////////////////////////////////
	// list of currently connected clients
	// gets updated periodically
	//////////////////////////////////////////////////
	// time in seconds to regenerate the client list
	time_t  m_client_freq = 5;
	time_t  m_client_last = 0;
	void mk_client_list ();
	str_list m_clients;

	//////////////////////////////////////////////////
	//
	//  private methods
	//
	//////////////////////////////////////////////////
	bool  init_data_channel ();
	bool  init_query_channel ();
	bool  init_admin_channel ();
	void  set_bind_addr ( const std::string& addr );
	void  add_client ( const netaddr& sender, char* msg );
	void  add_bad_client ( const netaddr& sender, std::string& err_msg,
			bool is_local, int bytes );
	bool  is_known_relay ( const netaddr& sender, size_t bytes );
	bool  packet_is_valid ( int bytes, msg_hdr_t* msg_hdr,
			const netaddr& sender );
	void  handle_data ( char* msg, int bytes,
			const netaddr& sender );
	int   update_tracker ( const std::string& callsign,
			const std::string& passwd,
			const std::string& modelname, const time_t time,
			const int type );
	void  drop_client ( pilot_it& player );
	bool  receiver_wants_data ( const pilot_it& sender,
			const pilot& receiver );
	bool  receiver_wants_chat ( const pilot_it& sender,
			const pilot& receiver );
	bool  is_in_range ( const list_item& relay,
			const pilot_it& sender, MSG_ID msg_id );
	void  send_to_cross ( char* msg, int bytes,
			const netaddr& sender );
	void  send_to_relays ( char* msg, int bytes, pilot_it& sender );
	void  want_exit ();
	void  print_version ();
	void  print_help ();
	void  tracker_log ( const std::string& msg, const char* src );

	int     m_argc = 0;
	char**  m_argv;
}; // fgms

} // namespace fgmp

#endif

