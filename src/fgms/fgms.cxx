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
 * @file	fgms.cxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 *		and contributors (see AUTHORS)
 * @date	2005-2017
 */

//////////////////////////////////////////////////////////////////////
//
//      Server for FlightGear
//      (c) 2005-2012 Oliver Schroeder
//
//////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
	#include "config.h" // for MSVC, always first
#endif

#ifndef _MSC_VER
	#include <sys/wait.h>
	#ifndef __FreeBSD__
		#include <endian.h>
	#endif
	#include <sys/ipc.h>
	#include <sys/msg.h>
	#include <netinet/in.h>
	#include <signal.h>
	#include <fglib/daemon.hxx>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <simgear/math/SGEuler.hxx>
#include <fglib/fg_util.hxx>
#include <fglib/fg_log.hxx>
#include <fglib/fg_util.hxx>
#include <fglib/fg_config.hxx>
#include <fglib/fg_version.hxx>
#include <fglib/fg_log.hxx>
#include "fg_cli.hxx"
#include "fgms.hxx"    // includes pthread.h


#ifdef _MSC_VER
	#include <conio.h> // for _kbhit(), _getch
#endif

extern void SigHUPHandler ( int SigType ); // main.cxx

const FG_VERSION FGMS::m_version ( 1, 0, 0, "-dev3" );

//////////////////////////////////////////////////////////////////////

/**
 * @class FGMS
 */

//////////////////////////////////////////////////////////////////////

/** Constructor
 */
FGMS::FGMS
() :	m_cross_list ( "Crossfeed" ),
	m_white_list ( "Whitelist" ),
	m_black_list ( "Blacklist" ),
	m_relay_list ( "Relays" ),
	m_player_list ( "Users" )
{
	int16_t* converter = ( int16_t* ) ( & PROTO_VER );

	m_initialized		= false;// init() will do it
	m_reinit_data		= true; // init the data port
	m_reinit_query		= true; // init the telnet port
	m_reinit_admin		= true; // init the telnet port
	m_data_port		= 5000; // port for client connections
	m_player_expires	= 10;	// standard expiration period
	m_listening		= false;
	m_data_channel		= 0;
	m_query_port		= m_data_port+1;
	m_admin_port		= m_data_port+2;
	m_num_max_clients	= 0;
	m_out_of_reach		= 100;	// standard 100 nm
	m_max_radar_range	= 2000;	// standard 2000 nm
	m_is_parent		= false;
	m_server_name		= "fgms";
	m_bind_addr		= "";
	m_FQDN			= "local";
	m_proto_minor_version	= converter[0]; // ->High;
	m_proto_major_version	= converter[1]; // ->Low;
	m_config_name		= "fgms.conf";
	m_logfile_name		= "fgms.log";
	m_exit_filename		= "fgms_exit";
	m_reset_filename	= "fgms_reset";
	m_stats_filename	= "fgms_stats";
	m_msglog_filename	= "message.log";
	m_update_tracker_freq	= 10;
	m_relay_map		= ip2relay_t();
	m_is_tracked		= false; // off until config file read
	m_tracker		= 0;	// no tracker yet
	m_update_tracker_freq	= 10;
	m_have_config		= false;
	m_add_cli		= true;
	// clear stats - should show what type of packet was received
	m_packets_received	= 0;
	m_ping_received		= 0;
	m_pong_received		= 0;
	m_queries_received	= 0;
	m_admin_received	= 0;
	m_black_rejected	= 0;  // in black list
	m_packets_invalid	= 0;  // invalid packet
	m_unknown_relay		= 0;  // unknown relay
	m_relay_magic		= 0;  // relay magic packet
	m_pos_data		= 0;  // position data packet
	m_unknown_data		= 0;
	// clear totals
	m_t_packets_received	= 0;
	m_t_black_rejected	= 0;
	m_t_packets_invalid	= 0;
	m_t_unknown_relay	= 0;
	m_t_pos_data		= 0;
	m_t_telnet_received	= 0;
	m_t_relay_magic		= 0;
	m_t_unknown_data	= 0;
	m_cross_failed		= 0;
	m_cross_sent		= 0;
	m_t_cross_failed	= 0;
	m_t_cross_sent		= 0;
	m_tracker_connect	= 0;
	m_tracker_disconnect	= 0;
	m_tracker_position	= 0; // Tracker messages queued
	m_local_clients		= 0;
	m_remote_clients	= 0;
	m_client_freq		= 5;
	m_client_last		= 0;
	// Be able to enable/disable file interface
	// On start-up if the file already exists, disable
	struct stat buf;
	m_use_exit_file       = ( stat ( m_exit_filename.c_str(), &buf ) ) ? true : false;
	// caution: this has failed in the past
	m_use_reset_file      = ( stat ( m_reset_filename.c_str(), &buf ) ) ? true : false;
	m_use_stat_file       = ( stat ( m_stats_filename.c_str(), &buf ) ) ? true : false;
	m_uptime		= time ( 0 );
	m_want_exit		= false;
	m_config_file		= "";
} // FGMS::FGMS()

//////////////////////////////////////////////////////////////////////

/**
 * destructor
 */
FGMS::~FGMS
()
{
	done();
} // FGMS::~FGMS()

//////////////////////////////////////////////////////////////////////

/** Detach a telnet session
 *
 * Someone queried us via telnet, so start a new thread
 * handling the request.
 */
static void*
detach_telnet
(
	void* context
)
{
	pthread_detach ( pthread_self() );
	st_telnet* t = reinterpret_cast<st_telnet*> ( context );
	FGMS* tmp_server = t->instance;
	tmp_server->handle_query ( t->fd );
	delete t;
	return 0;
} // detach_telnet ()

//////////////////////////////////////////////////////////////////////

/** Detach an admin session
 *
 * Someone started an admin session, so start a new thread
 * handling the request.
 */
void*
detach_admin
(
	void* context
)
{
	st_telnet* t = reinterpret_cast<st_telnet*> ( context );
	FGMS* tmp_server = t->instance;
	pthread_detach ( pthread_self() );
	tmp_server->handle_admin ( t->fd );
	delete t;
	return 0;
} // detach_admin()

//////////////////////////////////////////////////////////////////////

/** Detach the tracker module
 *
 * Only one tracker is allowed.
 */
void*
detach_tracker
(
	void* vp
)
{
	FG_TRACKER* pt = reinterpret_cast<FG_TRACKER*> ( vp );
	pt->loop();
	delete pt;
	return ( ( void* ) 0xdead );
} // detach_tracker ()

//////////////////////////////////////////////////////////////////////

/** Initialise the data channel
 *
 * @return true OK
 * @return false sonething went wrong
 */
bool
FGMS::init_data_channel
()
{

	if ( ! m_reinit_data )
	{
		return true;
	}
	if ( m_data_channel )
	{
		delete m_data_channel;
	}
	m_reinit_data = false;
	m_data_channel = new fgmp::netsocket();
	try
	{
		m_data_channel->listen_to ( m_bind_addr, m_data_port,
		  fgmp::netsocket::UDP );
	}
	catch ( std::runtime_error& e )
	{
		LOG ( log_prio::ERROR, "FGMS::init() - "
		  << "failed to bind to " << m_data_port );
		LOG ( log_prio::ERROR, "already in use?" );
		LOG ( log_prio::ERROR, e.what() );
		return false;
	}
	return true;
} // FGMS::init_data_channel ()

//////////////////////////////////////////////////////////////////////

/** Initialise the query channel
 *
 * @return true OK
 * @return false sonething went wrong
 */
bool
FGMS::init_query_channel
()
{
	if ( ! m_reinit_query )
	{
		return true;
	}
	if ( m_query_channel )
	{
		delete m_query_channel;
	}
	m_query_channel = 0;
	m_reinit_query = false;
	if ( m_query_port == 0 )
	{
		return true;	// query port disabled
	}
	m_query_channel = new fgmp::netsocket;
	try
	{
		m_query_channel->listen_to ( m_bind_addr, m_query_port,
		  fgmp::netsocket::TCP );
	}
	catch ( std::runtime_error& e )
	{
		LOG ( log_prio::ERROR, "FGMS::init() - "
		  << "failed to listen to query port" );
		return false;
	}
	return true;
} // FGMS::init_query_channel ()

//////////////////////////////////////////////////////////////////////

/** Initialise the admin channel
 *
 * @return true OK
 * @return false sonething went wrong
 */
bool
FGMS::init_admin_channel
()
{
	if ( ! m_reinit_admin )
	{
		return true;
	}
	if ( m_admin_channel )
	{
		delete m_admin_channel;
	}
	m_admin_channel = 0;
	m_reinit_admin = false;
	if ( ( m_admin_port == 0 ) || ( ! m_add_cli ) )
	{
		return true; 	// admin channel disabled
	}
	m_admin_channel = new fgmp::netsocket;
	try
	{
		m_admin_channel->listen_to ( m_bind_addr, m_admin_port,
		  fgmp::netsocket::TCP );
	}
	catch ( std::runtime_error& e )
	{
		LOG ( log_prio::ERROR, "FGMS::init() - "
		  << "could not create socket for admin" );
		return false;
	}
	return true;
} // FGMS::init_admin_channel ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Basic initialization
 *
 *  If we are already initialized, close
 *  all connections and re-init all variables
 */
bool
FGMS::init
()
{
	if ( ! logger.is_open() )
	{
		set_logfile ( m_logfile_name );
	}
	LOG ( log_prio::ERROR, "# FlightGear Multiplayer Server v"
	  << m_version.str() << " started" );
	if ( m_initialized == false )
	{
		if ( m_listening )
		{
			done();
		}
		m_initialized     = true;
		m_listening       = false;
		m_data_channel    = 0;
		m_num_max_clients = 0;
	}
	if ( ! init_data_channel () )
		return false;
	if ( ! init_query_channel () )
		return false;
	if ( ! init_admin_channel () )
		return false;
	LOG ( log_prio::ERROR, "# This is " << m_server_name << " (" << m_FQDN << ")" );
	LOG ( log_prio::ERROR, "# using protocol version v"
	  << m_proto_major_version << "." << m_proto_minor_version
	  << " (LazyRelay enabled)" );
	LOG ( log_prio::ERROR, "# listening to port " << m_data_port );
	if ( m_query_channel )
	{
		LOG ( log_prio::ERROR, "# telnet port " << m_query_port );
	}
	else
	{
		LOG ( log_prio::ERROR, "# telnet port DISABLED" );
	}
	if ( m_admin_channel )
	{
		LOG ( log_prio::ERROR, "# admin port " << m_admin_port );
	}
	else
	{
		LOG ( log_prio::ERROR, "# admin port DISABLED" );
	}
	LOG ( log_prio::ERROR, "# using logfile '" << m_logfile_name << "'" );
	if ( m_bind_addr != "" )
	{
		LOG ( log_prio::ERROR, "# listening on " << m_bind_addr );
	}
	if ( m_me_is_hub )
	{
		LOG ( log_prio::ERROR, "# I am a HUB Server" );
	}
	if ( ( m_is_tracked ) && ( m_tracker != 0 ) )
	{
		pthread_t th;
		pthread_create ( &th, NULL, &detach_tracker, m_tracker );
		LOG ( log_prio::ERROR, "# tracked to "
		  << m_tracker->get_server ()
		  << ":" << m_tracker->get_port ()
		  << ", using a thread."
		);
	}
	else
	{
		LOG ( log_prio::ERROR, "# tracking is disabled." );
	}
	size_t count;
	ListElement entry ( "" );
	//////////////////////////////////////////////////
	// print list of all relays
	//////////////////////////////////////////////////
	count = m_relay_list.size();
	LOG ( log_prio::ERROR, "# I have " << count << " relays" );
	for ( size_t i = 0; i < count; i++ )
	{
		entry = m_relay_list[i];
		if ( entry.id == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		LOG ( log_prio::ERROR, "# relay " << entry.name
			     << ":" << entry.address.port()
			     << " (" << entry.address << ")" );
	}
	//////////////////////////////////////////////////
	// print list of all crossfeeds
	//////////////////////////////////////////////////
	count = m_cross_list.size();
	LOG ( log_prio::ERROR, "# I have " << count << " crossfeeds" );
	for ( size_t i = 0; i < count; i++ )
	{
		entry = m_cross_list[i];
		if ( entry.id == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		LOG ( log_prio::ERROR, "# crossfeed " << entry.name
		  << ":" << entry.address.port()
		);
	}
	LOG ( log_prio::ERROR, "# I have " << m_black_list.size()
	  << " blacklisted IPs"
	);
	if ( m_use_exit_file && m_use_stat_file )
	{	// only show this IFF both are enabled
		LOG ( log_prio::ERROR, "# Files: exit=[" << m_exit_filename
		  << "] stat=[" << m_stats_filename << "]"
		);
	}
	m_listening = true;
#ifndef _MSC_VER
	if ( m_run_as_daemon )
	{
		m_myself.daemonize ();
		LOG ( log_prio::URGENT, "# My PID is " << m_myself.get_pid() );
	}
#endif
	return true;
} // FGMS::init()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Do anything necessary to (re-) init the server
 *
 * used to handle kill -HUP
 *
 * @TODO	This is awfully wrong. We need to block
 * 		all threads, until reinit is done
 */
void
FGMS::prepare_init
()
{
	if ( ! m_is_parent )
	{
		return;
	}
	m_have_config = false;
	LOG ( log_prio::URGENT, "# caught SIGHUP, doing reinit!" );
	// release all locks
	m_player_list.unlock ();
	m_relay_list.unlock ();
	m_cross_list.unlock ();
	m_white_list.unlock ();
	m_black_list.unlock ();
	// and clear all but the player list
	m_relay_list.clear ();
	m_white_list.clear ();
	m_black_list.clear ();
	m_cross_list.clear ();
	m_relay_map.clear ();	// clear(): is a std::map (NOT a FG_List)
	close_tracker ();
} // FGMS::prepare_init ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle an admin session.
 *
 * If a telnet connection to the admin port is established, a new FG_CLI
 * instance is created.
 * @param Fd -- docs todo --
 */
void*
FGMS::handle_admin
(
	int fd
)
{
	FG_CLI*	my_cli;
	errno = 0;
	my_cli = new FG_CLI ( this, fd );
	my_cli->loop ();
	if ( fd == 0 )
	{
		// reading from stdin
		want_exit();
	}
	delete my_cli;
	return ( 0 );
} // FGMS::handle_admin()

//////////////////////////////////////////////////////////////////////

/** Generate the list of currently connected clients
 *
 */
void
FGMS::mk_client_list
()
{
	std::string	message;
	FG_Player	player;
	unsigned int	it;

	m_clients.lock ();
	m_clients.clear ();
	//////////////////////////////////////////////////
	//
	//      create the output message
	//      header
	//
	//////////////////////////////////////////////////
	message  = "# This is " + m_server_name;
	message += "\n";
	m_clients.push_back ( message );
	message  = "# FlightGear Multiplayer Server v";
	message += m_version.str();
	message += "\n";
	m_clients.push_back ( message );
	message  = "# using protocol version v";
	message += num_to_str ( m_proto_major_version );
	message += "." + num_to_str ( m_proto_minor_version );
	message += " (LazyRelay enabled)";
	message += "\n";
	m_clients.push_back ( message );
	if ( m_is_tracked )
	{
		message  = "# This server is tracked: ";
		message += m_tracker->get_server();
		message += "\n";
		m_clients.push_back ( message );
	}
	message  = "# "+ num_to_str ( m_player_list.size() );
	message += " pilot(s) online\n";
	m_clients.push_back ( message );
	//////////////////////////////////////////////////
	//
	//      create list of players
	//
	//////////////////////////////////////////////////
	it = 0;
	for ( ;; )
	{
		if ( it < m_player_list.size() )
		{
			player = m_player_list[it];
			it++;
		}
		else
		{
			break;
		}
		if ( player.name.compare ( 0, 3, "obs", 3 ) == 0 )
		{
			continue;
		}
		message = player.name + "@";
		if ( player.is_local )
		{
			message += "LOCAL: ";
		}
		else
		{
			ip2relay_it Relay = m_relay_map.find
			  ( player.address );
			if ( Relay != m_relay_map.end() )
			{
				message += Relay->second + ": ";
			}
			else
			{
				message += player.origin + ": ";
			}
		}
		if ( player.error != "" )
		{
			message += player.error + " ";
		}
		message += num_to_str ( player.last_pos[Point3D::X], 6 ) +" ";
		message += num_to_str ( player.last_pos[Point3D::Y], 6 ) +" ";
		message += num_to_str ( player.last_pos[Point3D::Z], 6 ) +" ";
		message += num_to_str ( player.geod_pos[Point3D::LAT], 6 ) +" ";
		message += num_to_str ( player.geod_pos[Point3D::LON], 6 ) +" ";
		message += num_to_str ( player.geod_pos[Point3D::ALT], 6 ) +" ";
		message += num_to_str ( player.last_orientation[Point3D::X], 6 )+" ";
		message += num_to_str ( player.last_orientation[Point3D::Y], 6 )+" ";
		message += num_to_str ( player.last_orientation[Point3D::Z], 6 )+" ";
		message += player.model_name;
		message += "\n";
		m_clients.push_back ( message );
	}
	m_clients.unlock ();
} // FGMS::mk_client_list()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle a telnet session. if a telnet connection is opened, this
 *        method outputs a list  of all known clients.
 * @param Fd -- docs todo --
 */
void*
FGMS::handle_query
(
	int fd
)
{
	StrIt		line;
	fgmp::netsocket	telnet;
	telnet.handle ( fd );
	errno = 0;
	m_clients.lock ();
	for ( line = m_clients.begin(); line != m_clients.end(); line++ )
	{
		if ( telnet.send ( *line ) < 0 )
		{
			if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
			{
				LOG ( log_prio::URGENT, "FGMS::handle_query() - "
				  << strerror ( errno ) );
			}
			telnet.close ();
			m_clients.unlock ();
			return ( 0 );
		}
	}
	telnet.close ();
	m_clients.unlock ();
	return ( 0 );
} // FGMS::handle_query ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  If we receive bad data from a client, we add the client to
 *         the internal list anyway, but mark them as bad. But first
 *         we look if it isn't already there.
 *         Send an error message to the bad client.
 * @param sender
 * @param error_msg
 * @param is_local
 */
void
FGMS::add_bad_client
(
	const fgmp::netaddr& sender,
	std::string&	error_msg,
	bool	is_local,
	int	bytes
)
{
	std::string	message;
	FG_Player	new_player;
	PlayerIt	player;
	//////////////////////////////////////////////////
	//      see, if we already know the client
	//////////////////////////////////////////////////
	m_player_list.lock ();
	player = m_player_list.find ( sender, true );
	if ( player != m_player_list.end () )
	{
		player->update_rcvd ( bytes );
		m_player_list.update_rcvd ( bytes );
		m_player_list.unlock();
		return;
	}
	//////////////////////////////////////////////////
	//      new client, add to the list
	//////////////////////////////////////////////////
	if ( is_local )
	{
		m_local_clients++;
	}
	else
	{
		m_remote_clients++;
	}
	new_player.name       = "* Bad Client *";
	new_player.model_name = "* unknown *";
	new_player.origin     = sender.to_string ();
	new_player.address    = sender;
	new_player.is_local   = is_local;
	new_player.has_errors = true;
	new_player.error      = error_msg;
	new_player.update_rcvd ( bytes );
	LOG ( log_prio::MEDIUM, "FGMS::add_bad_client() - " << error_msg );
	m_player_list.add ( new_player, m_player_expires );
	m_player_list.update_rcvd ( bytes );
	m_player_list.unlock();
} // FGMS::add_bad_client ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new client to internal list
 * @param sender
 * @param msg
 */
void
FGMS::add_client
(
	const fgmp::netaddr& sender,
	char* msg
)
{
	uint32_t	msg_magic;
	uint32_t	proto_ver;
	std::string	message;
	std::string	origin;
	T_MsgHdr*	msg_hdr;
	T_PositionMsg*	pos_msg;
	FG_Player	new_player;
	bool		is_local;
	int16_t*	converter;

	msg_hdr		= ( T_MsgHdr* ) msg;
	pos_msg		= ( T_PositionMsg* ) ( msg + sizeof ( T_MsgHdr ) );
	msg_magic	= XDR_decode_uint32 ( msg_hdr->magic );
	is_local		= true;
	if ( msg_magic == RELAY_MAGIC ) // not a local client
	{
		is_local = false;
	}
	proto_ver = XDR_decode_uint32 ( msg_hdr->version );
	converter = ( int16_t* ) & proto_ver;
	new_player.name	    = msg_hdr->name;
	new_player.passwd    = "test"; //msg_hdr->passwd;
	new_player.model_name = "* unknown *";
	new_player.origin    = sender.to_string ();
	new_player.address   = sender;
	new_player.is_local   = is_local;
	new_player.proto_major	= converter[0];
	new_player.proto_minor	= converter[1];
	new_player.last_pos.set (
		XDR_decode_double ( pos_msg->position[Point3D::X] ),
		XDR_decode_double ( pos_msg->position[Point3D::Y] ),
		XDR_decode_double ( pos_msg->position[Point3D::Z] )
	);
	new_player.last_orientation.set (
		XDR_decode_float ( pos_msg->orientation[Point3D::X] ),
		XDR_decode_float ( pos_msg->orientation[Point3D::Y] ),
		XDR_decode_float ( pos_msg->orientation[Point3D::Z] )
	);
	cart_to_geod ( new_player.last_pos, new_player.geod_pos );
	new_player.model_name = pos_msg->model;
	using fgmp::ATC_TYPE;
	if ( ( new_player.model_name == "OpenRadar" )
	||   ( new_player.model_name.find ( "ATC" ) != std::string::npos ) )
	{
		// client is an ATC
		if ( str_ends_with ( new_player.name, "_DL" ) )
		{
			new_player.is_ATC = ATC_TYPE::ATC_DL;
		}
		else if ( str_ends_with ( new_player.name, "_GN" ) )
		{
			new_player.is_ATC = ATC_TYPE::ATC_GN;
		}
		else if ( str_ends_with ( new_player.name, "_TW" ) )
		{
			new_player.is_ATC = ATC_TYPE::ATC_TW;
		}
		else if ( str_ends_with ( new_player.name, "_AP" ) )
		{
			new_player.is_ATC = ATC_TYPE::ATC_AP;
		}
		else if ( str_ends_with ( new_player.name, "_DE" ) )
		{
			new_player.is_ATC = ATC_TYPE::ATC_DE;
		}
		else if ( str_ends_with ( new_player.name, "_CT" ) )
		{
			new_player.is_ATC = ATC_TYPE::ATC_CT;
		}
		else
		{
			new_player.is_ATC = ATC_TYPE::ATC;
		}
	}
	msg_hdr->radar_range = XDR_decode_uint32 ( msg_hdr->radar_range );
	converter = (int16_t *) & msg_hdr->radar_range;
	if ( ( converter[0] != 0 ) || ( converter[1] == 0 ) )
	{
		// client comes from an old server which overwrites
		// the radar range or the client does not set
		// radar_range
		new_player.radar_range = m_out_of_reach;
	}
	else
	{
		if ( converter[0] <= m_max_radar_range )
		{
			new_player.radar_range = converter[0];
		}
	}
	if ( new_player.radar_range == 0 )
	{
		new_player.radar_range = m_out_of_reach;
	}
	m_player_list.add ( new_player, m_player_expires );
	size_t NumClients = m_player_list.size ();
	if ( NumClients > m_num_max_clients )
	{
		m_num_max_clients = NumClients;
	}
	origin  = new_player.origin;
	if ( is_local )
	{
		message = "New LOCAL Client: ";
		m_local_clients++;
		update_tracker ( new_player.name, new_player.passwd,
		  new_player.model_name, new_player.last_seen, CONNECT );
	}
	else
	{
		m_remote_clients++;
		message = "New REMOTE Client: ";
		ip2relay_it Relay = m_relay_map.find ( new_player.address );
		if ( Relay != m_relay_map.end() )
		{
			origin = Relay->second;
		}
#ifdef TRACK_ALL
		update_tracker ( new_player.name, new_player.passwd,
		  new_player.model_name, new_player.last_seen, CONNECT );
#endif
	}
	LOG ( log_prio::MEDIUM, message
	  << new_player.name << "@"
	  << origin << ":" << sender.port()
	  << " (" << new_player.model_name << ")"
	  << " current clients: "
	  << NumClients << " max: " << m_num_max_clients
	);
} // FGMS::add_client()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new relay server into internal list
 * @param Server
 * @param Port
 */
void
FGMS::add_relay
(
	const std::string& relay,
	int port
)
{
	ListElement b ( relay );
	b.address.assign ( relay, port );
	if ( ! b.address.is_valid () )
	{
		LOG ( log_prio::URGENT,
		  "could not resolve '" << relay << "'" );
		return;
	}
	m_relay_list.lock ();
	ItList current_entry = m_relay_list.find ( b.address, true );
	m_relay_list.unlock ();
	if ( current_entry == m_relay_list.end() )
	{
		m_relay_list.add ( b, 0 );
		std::string s;
		if ( b.address.to_string () == relay )
		{
			s = relay;
		}
		else
		{
			unsigned i;
			i = relay.find ( "." );
			if ( i != std::string::npos )
			{
				s = relay.substr ( 0, i );
			}
			else
			{
				s = relay;
			}
		}
		m_relay_map[b.address] = s;
	}
} // FGMS::add_relay()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new crossfeed server into internal list
 * @param Server char with server
 * @param Port int with port number
 */
void
FGMS::add_crossfeed
(
	const std::string& server,
	int port
)
{
	std::string s = server;
#ifdef _MSC_VER
	if ( s == "localhost" )
	{
		s = "127.0.0.1";
	}
#endif // _MSC_VER
	ListElement B ( s );
	B.address.assign ( ( char* ) s.c_str(), port );
	m_cross_list.lock ();
	ItList current_entry = m_cross_list.find ( B.address, true );
	m_cross_list.unlock ();
	if ( current_entry == m_cross_list.end() )
	{
		m_cross_list.add ( B, 0 );
	}
} // FGMS::add_crossfeed()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Add a tracking server
 * @param Server String with server
 * @param Port The port number
 * @param IsTracked Is Stracked
 * @retval int -1 for fail or SUCCESS
 */
bool
FGMS::add_tracker
(
	const std::string& server,
	int  port,
	bool is_tracked
)
{
	close_tracker();
	m_is_tracked = is_tracked;
	m_tracker = new FG_TRACKER ( port, server, m_server_name,
	  m_FQDN, m_version.str() );
	return true;
} // FGMS::add_tracker()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Add an IP to the whitelist
 * @param FourDottedIP IP to add to whitelist
 */
void
FGMS::add_whitelist
(
	const std::string& dotted_ip
)
{
	ListElement B ( dotted_ip );
	B.address.assign ( dotted_ip.c_str(), 0 );
	m_white_list.lock ();
	ItList current_entry = m_white_list.find ( B.address );
	m_white_list.unlock ();
	if ( current_entry == m_white_list.end() )
	{
		m_white_list.add ( B, 0 );
	}
} // FGMS::add_whitelist()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief add an IP to the blacklist
 * @param FourDottedIP IP to add to blacklist
 */
void
FGMS::add_blacklist
(
	const std::string& dotted_ip,
	const std::string& reason,
	time_t timeout
)
{
	ListElement B ( reason );
	B.address.assign ( dotted_ip.c_str(), 0 );
	m_black_list.lock ();
	ItList current_entry = m_black_list.find ( B.address );
	m_black_list.unlock ();
	if ( current_entry == m_black_list.end() )
	{
		// FIXME: every list has its own standard TTL
		m_black_list.add ( B, timeout );
	}
} // FGMS::add_blacklist()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check if the sender is a known relay
 * @param sender_address
 * @retval bool true if known relay
 */
bool
FGMS::is_known_relay
(
	const fgmp::netaddr& sender_address,
	size_t bytes
)
{
	ItList current_entry;
	m_white_list.lock ();
	current_entry = m_white_list.find ( sender_address );
	if ( current_entry != m_white_list.end() )
	{
		m_white_list.update_rcvd ( current_entry, bytes );
		m_white_list.unlock ();
		return true;
	}
	m_white_list.unlock ();
	m_relay_list.lock ();
	current_entry = m_relay_list.find ( sender_address );
	if ( current_entry != m_relay_list.end() )
	{
		m_relay_list.update_rcvd ( current_entry, bytes );
		m_relay_list.unlock ();
		return true;
	}
	m_relay_list.unlock ();
	std::string error_msg;
	error_msg  = sender_address.to_string ();
	error_msg += " is not a valid relay!";
	add_blacklist ( sender_address.to_string (), "not a valid relay", 0 );
	LOG ( log_prio::URGENT, "UNKNOWN RELAY: " << error_msg );
	return ( false );
} // FGMS::is_known_relay ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      check if the packet is valid
//
//////////////////////////////////////////////////////////////////////
/**
 * @brief
 */
bool
FGMS::packet_is_valid
(
	int		bytes,
	T_MsgHdr*	msg_hdr,
	const fgmp::netaddr& sender_address
)
{
	uint32_t        msg_magic;
	uint32_t        msg_len;
	uint32_t        msg_id;
	uint32_t        proto_ver;
	std::string     error_msg;
	std::string     origin;
	struct converter
	{
		int16_t		High;
		int16_t		Low;
	};
	converter*    tmp;
	origin    = sender_address.to_string ();
	msg_magic = XDR_decode_uint32 ( msg_hdr->magic );
	msg_id    = XDR_decode_uint32 ( msg_hdr->msg_id );
	msg_len   = XDR_decode_uint32 ( msg_hdr->msg_len );
	if ( bytes < ( int ) sizeof ( msg_hdr ) )
	{
		error_msg  = sender_address.to_string ();
		error_msg += " packet size is too small!";
		add_bad_client ( sender_address, error_msg, true, bytes );
		return ( false );
	}
	if ( ( msg_magic != MSG_MAGIC ) && ( msg_magic != RELAY_MAGIC ) )
	{
		char m[5];
		memcpy ( m, ( char* ) &msg_magic, 4 );
		m[4] = 0;
		error_msg  = origin;
		error_msg += " BAD magic number: ";
		error_msg += m;
		add_bad_client ( sender_address, error_msg, true, bytes );
		return ( false );
	}
	proto_ver = XDR_decode_uint32 ( msg_hdr->version );
	tmp = ( converter* ) & proto_ver;
	if ( tmp->High != m_proto_major_version )
	{
		msg_hdr->version = XDR_decode_uint32 ( msg_hdr->version );
		error_msg  = origin;
		error_msg += " BAD protocol version! Should be ";
		tmp = ( converter* ) ( & PROTO_VER );
		error_msg += num_to_str ( tmp->High );
		error_msg += "." + num_to_str ( tmp->Low );
		error_msg += " but is ";
		tmp = ( converter* ) ( & msg_hdr->version );
		error_msg += num_to_str ( tmp->Low );
		error_msg += "." + num_to_str ( tmp->High );
		add_bad_client ( sender_address, error_msg, true, bytes );
		return ( false );
	}
	if ( msg_id == FGFS::POS_DATA )
	{
		if ( msg_len < sizeof ( T_MsgHdr ) + sizeof ( T_PositionMsg ) )
		{
			error_msg  = origin;
			error_msg += " Client sends insufficient position ";
			error_msg += "data, should be ";
			error_msg += num_to_str (
			  sizeof ( T_MsgHdr ) +sizeof ( T_PositionMsg ) );
			error_msg += " is: " + num_to_str ( msg_hdr->msg_len );
			add_bad_client ( sender_address, error_msg, true, bytes );
			return ( false );
		}
	}
	return ( true );
} // FGMS::packet_is_valid ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Send message to all crossfeed servers.
 *         Crossfeed servers receive all traffic without condition,
 *         mainly used for testing and debugging
 */
void
FGMS::send_to_cross
(
	char* msg,
	int bytes,
	const fgmp::netaddr& sender_address 
)
{
	T_MsgHdr*       msg_hdr;
	uint32_t        msg_magic;
	int             sent;
	ItList		entry;
	msg_hdr		= ( T_MsgHdr* ) msg;
	msg_magic	= msg_hdr->magic;
	msg_hdr->magic	= XDR_encode_uint32 ( RELAY_MAGIC );
	m_cross_list.lock();
	for ( entry = m_cross_list.begin(); entry != m_cross_list.end(); entry++ )
	{
		sent = m_data_channel->send_to ( msg, bytes, entry->address );
		m_cross_list.update_sent ( entry, sent );
	}
	m_cross_list.unlock();
	msg_hdr->magic = msg_magic;  // restore the magic value
} // FGMS::send_toCrossfeed ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Send message to all relay servers
 */
void
FGMS::send_to_relays
(
	char* msg,
	int bytes,
	PlayerIt& sender
)
{
	T_MsgHdr*       msg_hdr;
	uint32_t        msg_magic;
	uint32_t        msg_id;
	unsigned int    pkts_forwarded = 0;
	ItList		current_relay;
	if ( ( ! sender->is_local ) && ( ! m_me_is_hub ) )
	{
		return;
	}
	msg_hdr    = ( T_MsgHdr* ) msg;
	msg_magic  = XDR_decode_uint32 ( msg_hdr->magic );
	msg_id     = XDR_decode_uint32 ( msg_hdr->msg_id );
	msg_hdr->magic = XDR_encode_uint32 ( RELAY_MAGIC );
	m_relay_list.lock ();
	current_relay = m_relay_list.begin();
	while ( current_relay != m_relay_list.end() )
	{
		if ( current_relay->address != sender->address )
		{
			if ( sender->do_update || is_in_range ( *current_relay, sender, msg_id ) )
			{
				m_data_channel->send_to ( msg, bytes, current_relay->address );
				m_relay_list.update_sent ( current_relay, bytes );
				pkts_forwarded++;
			}
		}
		current_relay++;
	}
	m_relay_list.unlock ();
	msg_hdr->magic = XDR_encode_uint32 ( msg_magic ); // restore the magic value
} // FGMS::send_toRelays ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//	Remove Player from list
void
FGMS::drop_client
(
	PlayerIt& player
)
{
	std::string origin;
#ifdef TRACK_ALL
	update_tracker (
		player->name,
		player->passwd,
		player->model_name,
		player->last_seen,
		DISCONNECT );
#else
	if ( ( player->is_local ) && ( player->has_errors == false ) )
	{
		update_tracker (
			player->name,
			player->passwd,
			player->model_name,
			player->last_seen,
			DISCONNECT );
	}
#endif
	if ( player->is_local )
	{
		m_local_clients--;
	}
	else
	{
		m_remote_clients--;
	}
	ip2relay_it Relay = m_relay_map.find ( player->address );
	if ( Relay != m_relay_map.end() )
	{
		origin = Relay->second;
	}
	else
	{
		origin = "LOCAL";
	}
	LOG ( log_prio::MEDIUM, "TTL exceeded for "
	  << player->name << "@" << origin
	  << ", dropping after " << time ( 0 )-player->join_time
	  << " seconds. " << "Current clients: "
	  << m_player_list.size()-1 << " max: " << m_num_max_clients
	);
	player = m_player_list.erase ( player );
} // FGMS::drop_client()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle client connections
 * @param msg
 * @param bytes
 * @param sender_address
 */
void
FGMS::handle_data
(
	char* msg,
	int   bytes,
	const fgmp::netaddr& sender_address
)
{
	T_MsgHdr*       msg_hdr;
	T_PositionMsg*  pos_msg;
	uint32_t        msg_id;
	uint32_t        msg_magic;
	PlayerIt	sender;
	PlayerIt	player;
	ItList		current_entry;
	time_t          now;
	unsigned int    pkts_forwarded = 0;
	struct converter
	{
		int16_t         High;
		int16_t         Low;
	};
	converter* tmp;
	msg_hdr   = ( T_MsgHdr* ) msg;
	msg_magic = XDR_decode_uint32 ( msg_hdr->magic );
	msg_id    = XDR_decode_uint32 ( msg_hdr->msg_id );
	now       = time ( 0 );
	//////////////////////////////////////////////////
	//
	//  First of all, send packet to all
	//  crossfeed servers.
	//
	//////////////////////////////////////////////////
	send_to_cross ( msg, bytes, sender_address );
	//////////////////////////////////////////////////
	//
	//  now do the local processing
	//
	//////////////////////////////////////////////////
	m_black_list.lock ();
	current_entry = m_black_list.find ( sender_address );
	if ( current_entry != m_black_list.end() )
	{
		m_black_list.update_rcvd ( current_entry, bytes );
		m_black_rejected++;
		m_black_list.unlock ();
		return;
	}
	m_black_list.unlock ();
	if ( ! packet_is_valid ( bytes, msg_hdr, sender_address ) )
	{
		m_packets_invalid++;
		return;
	}
	if ( msg_magic == RELAY_MAGIC ) // not a local client
	{
		if ( ! is_known_relay ( sender_address, bytes ) )
		{
			m_unknown_relay++;
			return;
		}
		else
		{
			m_relay_magic++; // bump relay magic packet
		}
	}
	//////////////////////////////////////////////////
	//
	//    Statistics
	//
	//////////////////////////////////////////////////
	if ( msg_id == FGFS::POS_DATA )
	{
		m_pos_data++;
	}
	else
	{
		//////////////////////////////////////////////////
		// handle special packets
		//////////////////////////////////////////////////
		if ( msg_id == FGFS::PING )
		{
			// send packet verbatim back to sender
			m_ping_received++;
			msg_hdr->msg_id = XDR_encode_uint32 ( FGFS::PONG );
			m_data_channel->send_to ( msg, bytes, sender_address );
			return;
		}
		else if ( msg_id == FGFS::PONG )
		{
			// we should never receive PONGs, but silently
			// discard them if someone tries to play tricks
			m_pong_received++;
			return;
		}
		m_unknown_data++;
	}
	//////////////////////////////////////////////////
	//
	//    add Client to list if its not known
	//
	//////////////////////////////////////////////////
	m_player_list.lock();
	sender = m_player_list.find_by_name ( msg_hdr->name );
	if ( sender == m_player_list.end () )
	{
		// unknown, add to the list
		if ( msg_id != FGFS::POS_DATA )
		{
			// ignore clients until we have a valid position
			m_player_list.unlock();
			return;
		}
		add_client ( sender_address, msg );
		sender = m_player_list.last();
	}
	else
	{
		//////////////////////////////////////////////////
		//
		// found the client, update internal values
		//
		//////////////////////////////////////////////////
		if ( sender->address != sender_address )
		{
			m_player_list.unlock();
			return;
		}
		m_player_list.update_rcvd ( sender, bytes );
		if ( msg_id == FGFS::POS_DATA )
		{
			pos_msg = ( T_PositionMsg* ) ( msg + sizeof ( T_MsgHdr ) );
			double x = XDR_decode_double ( pos_msg->position[Point3D::X] );
			double y = XDR_decode_double ( pos_msg->position[Point3D::Y] );
			double z = XDR_decode_double ( pos_msg->position[Point3D::Z] );
			if ( ( x == 0.0 ) || ( y == 0.0 ) || ( z == 0.0 ) )
			{
				// ignore while position is not settled
				m_player_list.unlock();
				return;
			}
			sender->last_pos.set ( x, y, z );
			sender->last_orientation.set (
				XDR_decode_float ( pos_msg->orientation[Point3D::X] ),
				XDR_decode_float ( pos_msg->orientation[Point3D::Y] ),
				XDR_decode_float ( pos_msg->orientation[Point3D::Z] )
			);
			cart_to_geod ( sender->last_pos, sender->geod_pos );
		}
		msg_hdr->radar_range = XDR_decode_uint32 ( msg_hdr->radar_range );
		tmp =  ( converter* ) & msg_hdr->radar_range;
		if ( ( tmp->High != 0 ) && ( tmp->Low == 0 ) )
		{
			// client is 'new' and transmit radar range
			if ( tmp->High != sender->radar_range )
			{
				LOG ( log_prio::MEDIUM, sender->name
				  << " changes radar range from "
				  << sender->radar_range
				  << " to "
				  << tmp->High
				);
				if ( tmp->High <= m_max_radar_range )
				{
					sender->radar_range = tmp->High;
				}
				else
				{
					LOG ( log_prio::MEDIUM, sender->name
					  << " radar range to high, ignoring"
					);
				}
			}
		}
	}
	m_player_list.unlock();
	//////////////////////////////////////////
	//
	//      send the packet to all clients.
	//      since we are walking through the list,
	//      we look for the sending client, too. if it
	//      is not already there, add it to the list
	//
	//////////////////////////////////////////////////
	msg_hdr->magic = XDR_encode_uint32 ( MSG_MAGIC );
	player = m_player_list.begin();
	while ( player != m_player_list.end() )
	{
		//////////////////////////////////////////////////
		//
		//      ignore clients with errors
		//
		//////////////////////////////////////////////////
		if ( player->has_errors )
		{
			if ( ( now - player->last_seen ) > player->timeout )
			{
				drop_client ( player );
			}
			else
			{
				player++;
			}
			continue;
		}
		//////////////////////////////////////////////////
		//        sender == player?
		//////////////////////////////////////////////////
		//  FIXME: if sender is a Relay,
		//         player->address will be
		//         address of Relay and not the client's!
		//         so use a clientID instead
		if ( player->name == msg_hdr->name )
		{
			//////////////////////////////////////////////////
			//	send update to inactive relays?
			//////////////////////////////////////////////////
			player->do_update = ( ( now - player->last_relayed_to_inactive ) > UPDATE_INACTIVE_PERIOD );
			if ( player->do_update )
			{
				player->last_relayed_to_inactive = now;
			}
			player++;
			continue; // don't send packet back to sender
		}
		//////////////////////////////////////////////////
		// 'hidden' feature of fgms. If a callsign starts
		// with 'obs', do not send the packet to other
		// clients. Useful for test connections.
		//////////////////////////////////////////////////
		if ( player->name.compare ( 0, 3, "obs", 3 ) == 0 )
		{
			player++;
			continue;
		}
		//////////////////////////////////////////////////
		//      do not send packet to clients which
		//      are out of reach.
		//////////////////////////////////////////////////
		if ( msg_id == FGFS::CHAT_MSG )
		{
			// apply 'radio' rules
			// if ( not receiver_wants_chat( sender, *player ) )
			if ( ! receiver_wants_data ( sender, *player ) )
			{
				player++;
				continue;
			}
		}
		else
		{
			// apply 'visibility' rules, for now we apply 'radio' rules
			if ( ! receiver_wants_data ( sender, *player ) )
			{
				player++;
				continue;
			}
		}
		//////////////////////////////////////////////////
		//
		//  only send packet to local clients
		//
		//////////////////////////////////////////////////
		if ( player->is_local )
		{
			m_data_channel->send_to ( msg, bytes, player->address );
			m_player_list.update_sent ( player, bytes );
			pkts_forwarded++;
		}
		player++;
	}
	if ( sender->id ==  ListElement::NONE_EXISTANT )
	{
		// player not yet in our list
		// should not happen, but test just in case
		LOG ( log_prio::URGENT, "## BAD => "
		  << msg_hdr->name << ":" << sender_address.to_string ()
		);
		return;
	}
	send_to_relays ( msg, bytes, sender );
} // FGMS::handle_data ();
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Show Stats
 */
void FGMS::show_stats
()
{
	int pilot_cnt, local_cnt;
	// update totals since start
	m_t_packets_received += m_packets_received;
	m_t_black_rejected   += m_black_rejected;
	m_t_packets_invalid  += m_packets_invalid;
	m_t_unknown_relay    += m_unknown_relay;
	m_t_relay_magic      += m_relay_magic;
	m_t_pos_data         += m_pos_data;
	m_t_unknown_data     += m_unknown_data;
	m_t_telnet_received  += m_queries_received;
	m_t_cross_failed     += m_cross_failed;
	m_t_cross_sent       += m_cross_sent;
	// output to LOG and cerr channels
	pilot_cnt = local_cnt = 0;
	FG_Player player; // get LOCAL pilot count
	pilot_cnt = m_player_list.size ();
	for ( int i = 0; i < pilot_cnt; i++ )
	{
		player = m_player_list[i];
		if ( player.id == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		if ( player.is_local )
		{
			local_cnt++;
		}
	}
	LOG ( log_prio::URGENT, "## Pilots: total "
	  << pilot_cnt << ", local " << local_cnt );
	LOG ( log_prio::URGENT, "## Since: Packets " <<
		 m_packets_received << " BL=" <<
		 m_black_rejected << " INV=" <<
		 m_packets_invalid << " UR=" <<
		 m_unknown_relay << " RD=" <<
		 m_relay_magic << " PD=" <<
		 m_pos_data << " NP=" <<
		 m_unknown_data << " CF=" <<
		 m_cross_sent << "/" << m_cross_failed << " TN=" <<
		 m_queries_received
	       );
	LOG ( log_prio::URGENT, "## Total: Packets " <<
		 m_t_packets_received << " BL=" <<
		 m_t_black_rejected << " INV=" <<
		 m_t_packets_invalid << " UR=" <<
		 m_t_unknown_relay << " RD=" <<
		 m_t_relay_magic << " PD=" <<
		 m_t_pos_data << " NP=" <<
		 m_t_unknown_data <<  " CF=" <<
		 m_t_cross_sent << "/" << m_t_cross_failed << " TN=" <<
		 m_t_telnet_received << " TC/D/P=" <<
		 m_tracker_connect << "/" << m_tracker_disconnect << "/" << m_tracker_position
	       );
	// restart 'since' last stat counter
	m_packets_received = m_black_rejected = m_packets_invalid = 0;
	m_unknown_relay = m_pos_data = m_queries_received = 0; // reset
	m_relay_magic = m_unknown_data = 0; // reset
	m_cross_failed = m_cross_sent = 0;
} // FGMS::show_stats ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check exit and stat files
 *
 * Do not think this is used by many, but is a convenient way to output
 * some stats to the LOG, or request an exit. In the past the reset action
 * has failed, and although some fixes have been put in place, a caution about
 * using this reset.
 *
 * 20150619:0.11.9: If running instance can NOT delete a detected file,
 * usually due to wrong permissions, that particular file interface will
 * be disabled. Also if any of these files exist at start-up, again
 * that file interface will be disable. This also gives a way to disable this
 * file interface actions.
 *
 */
int
FGMS::check_files
()
{
	struct stat buf;
	if ( m_use_exit_file && ( stat ( m_exit_filename.c_str(), &buf ) == 0 ) )
	{
		LOG ( log_prio::URGENT, "## Got EXIT file : " << m_exit_filename );
		unlink ( m_exit_filename.c_str() );
		if ( stat ( m_exit_filename.c_str(), &buf ) == 0 )
		{
			LOG ( log_prio::URGENT,
				 "WARNING: Unable to delete exit file "
				 << m_exit_filename << "! Disabled interface..." );
			m_use_exit_file = false;
		}
		return 1;
	}
	else if ( m_use_reset_file && ( stat ( m_reset_filename.c_str(), &buf ) == 0 ) )
	{
		LOG ( log_prio::URGENT, "## Got RESET file "
			 << m_reset_filename );
		unlink ( m_reset_filename.c_str() );
		if ( stat ( m_reset_filename.c_str(), &buf ) == 0 )
		{
			LOG ( log_prio::URGENT,
				 "WARNING: Unable to delete reset file "
				 << m_reset_filename << "! Disabled interface..." );
			m_use_reset_file = false;
		}
		m_reinit_data	= true; // init the data port
		m_reinit_query	= true; // init the telnet port
		m_reinit_admin	= true; // init the admin port
		//FIXME: SigHUPHandler ( 0 );
	}
	else if ( m_use_stat_file && ( stat ( m_stats_filename.c_str(), &buf ) == 0 ) )
	{
		LOG ( log_prio::URGENT, "## Got STAT file " << m_stats_filename );
		unlink ( m_stats_filename.c_str() );
		if ( stat ( m_stats_filename.c_str(), &buf ) == 0 )
		{
			LOG ( log_prio::URGENT,
				 "WARNING: Unable to delete stat file "
				 << m_stats_filename << "! Disabled interface..." );
			m_use_stat_file = false;
		}
		show_stats();
	}
#ifdef _MSC_VER
	if ( !m_add_cli && _kbhit() )
	{
		int ch = _getch ();
		if ( ch == 0x1b )
		{
			printf ( "Got ESC key to exit...\n" );
			return 1;
		}
		else
		{
			printf ( "Got UNKNOWN keyboard! %#X - Only ESC, to exit\n", ch );
		}
	}
#endif // _MSC_VER
	return 0;
} // FGMS::check_files ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief allow the Admin CLI to shut down fgms
 */
void
FGMS::want_exit
()
{
	m_want_exit = true;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Main loop of the server
 */
bool
FGMS::loop
()
{
	int         bytes;
	char        msg[MAX_PACKET_SIZE];
	time_t      last_tracker_update;
	time_t      current_time;
	PlayerIt    player;
	fgmp::netaddr     sender_address ( fgmp::netaddr::IPv6 );
	fgmp::netsocket*  listen_sockets[3 + MAX_TELNETS];
	last_tracker_update = time ( 0 );
	if ( m_listening == false )
	{
		LOG ( log_prio::ERROR, "FGMS::loop() - "
			 << "not listening on any socket!" );
		return false;
	}
	if ( ( m_admin_user == "" ) || ( m_admin_pass == "" ) )
	{
		if ( m_admin_channel )
		{
			m_admin_channel->close();
			delete m_admin_channel;
			m_admin_channel = 0;
			LOG ( log_prio::ERROR,
			  "# Admin port disabled, "
			  "please set user and password" );
		}
	}
	LOG ( log_prio::ERROR, "# Main server started!" );
#ifdef _MSC_VER
	LOG ( log_prio::URGENT,
	  "ESC key to EXIT (after select "
	  << m_player_expires << " sec timeout)." );
#endif
	if ( ! m_run_as_daemon && m_add_cli )
	{
		// Run admin CLI in foreground reading from stdin
		st_telnet* t = new st_telnet;
		t->instance = this;
		t->fd       = 0;
		pthread_t th;
		pthread_create ( &th, NULL, &detach_admin, t );
	}
	m_is_parent = true;
	//////////////////////////////////////////////////
	//
	//      infinite listening loop
	//
	//////////////////////////////////////////////////
	while ( m_want_exit == false )
	{
		if ( ! m_listening )
		{
			cout << "bummer 1!" << endl;
			return false;;
		}
		if ( m_data_channel == 0 )
		{
			cout << "bummer 2!" << endl;
			return false;
		}
		current_time = time ( 0 );
		// check timeout
		player = m_player_list.begin();
		for ( size_t i = 0; i < m_player_list.size(); i++ )
		{
			if ( !m_player_list.check_ttl ( i )
			|| ( ( ( current_time - player->last_seen ) > player->timeout )
			&&   ( ( current_time - player->join_time ) > 30 ) ) )
			{
				drop_client ( player );
			}
			player++;
		}
		for ( size_t i = 0; i < m_black_list.size(); i++ )
		{
			if ( !m_black_list.check_ttl ( i ) )
			{
				m_black_list.delete_by_pos ( i );
			}
		}
		// Update some things every (default) 10 secondes
		if ( ( ( current_time - last_tracker_update ) >= m_update_tracker_freq )
		|| ( ( current_time - last_tracker_update ) < 0 ) )
		{
			last_tracker_update = time ( 0 );
			if ( m_player_list.size() >0 )
			{
				// updates the position of the users
				// regularly (tracker)
				update_tracker ( "" , "", "", last_tracker_update, UPDATE );
			}
			if ( check_files() )
			{
				break;
			}
		} // position (tracker)
		errno = 0;
		listen_sockets[0] = m_data_channel;
		listen_sockets[1] = m_query_channel;
		listen_sockets[2] = m_admin_channel;
		listen_sockets[3] = 0;
		bytes = m_data_channel->select ( listen_sockets, 0,
		  m_player_expires );
		if ( bytes < 0 )
		{
			// error
			continue;
		}
		else if ( bytes == 0 )
		{
			continue;
		}
		if ( listen_sockets[0] > 0 )
		{
			// something on the wire (clients)
			bytes = m_data_channel->recv_from ( msg,
			  MAX_PACKET_SIZE, sender_address );
			if ( bytes <= 0 )
			{
				continue;
			}
			m_packets_received++;
			handle_data ( ( char* ) &msg, bytes, sender_address );
		} // DataSocket
		else if ( listen_sockets[1] > 0 )
		{
			// something on the wire (telnet)
			m_queries_received++;
			int Fd = m_query_channel->accept ( 0 );
			if ( Fd < 0 )
			{
				if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
				{
					LOG ( log_prio::URGENT, "FGMS::loop() - "
					  << strerror ( errno ) );
				}
				continue;
			}
			st_telnet* t = new st_telnet;
			t->instance = this;
			t->fd       = Fd;
			pthread_t th;
			pthread_create ( &th, NULL, & detach_telnet, t );
		} // TelnetSocket
		else if ( listen_sockets[2] > 0 )
		{
			// something on the wire (admin port)
			fgmp::netaddr admin_address;
			m_admin_received++;
			int Fd = m_admin_channel->accept ( & admin_address );
			if ( Fd < 0 )
			{
				if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
				{
					LOG ( log_prio::URGENT, "FGMS::loop() - "
					  << strerror ( errno ) );
				}
				continue;
			}
			LOG ( log_prio::URGENT,
			  "FGMS::loop() - new Admin connection from "
			  << admin_address.to_string () );
			st_telnet* t = new st_telnet;
			t->instance = this;
			t->fd       = Fd;
			pthread_t th;
			pthread_create ( &th, NULL, &detach_admin, t );
		} // AdminSocket
		//
		// regenrate the client list?
		if ( ( current_time - m_client_last ) > m_client_freq )
		{
			mk_client_list ();
			m_client_last = current_time;
		}
	}
	return true;
} // FGMS::loop()

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Set listening port for incoming clients
 */
void
FGMS::set_data_port
(
	int port
)
{
	if ( port != m_data_port )
	{
		m_data_port = port;
		m_reinit_data   = true;
		m_query_port = m_data_port+1;
		m_reinit_query = true;
		m_admin_port  = m_data_port+2;
		m_reinit_admin  = true;
	}
} // FGMS::SetPort ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set listening port for telnets
 */
void
FGMS::set_query_port
(
	int port
)
{
	if ( m_query_port != port )
	{
		m_query_port = port;
		m_reinit_query = true;
	}
} // FGMS::set_query_port ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set listening port for admin connections
 */
void
FGMS::set_admin_port
(
	int port
)
{
	if ( m_admin_port != port )
	{
		m_admin_port = port;
		m_reinit_admin = true;
	}
} // FGMS::set_admin_port ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////

/**
 * @brief  Set the logfile
 */
void
FGMS::set_logfile
(
	const std::string& logfile_name
)
{
	m_logfile_name = logfile_name;
	if ( ! logger.open ( m_logfile_name ) )
	{
		LOG ( log_prio::ERROR, "FGMS::Init() - "
		  << "Failed to open log file " << m_logfile_name );
	}
	logger.priority ( log_prio::MEDIUM );
	logger.flags ( fgmp::fglog::WITH_DATE );
} // FGMS::set_logfile ( const std::string &logfile_name )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FGMS::set_exitfile
(
	const std::string& name
)
{
	m_exit_filename = name;
} // FGMS::set_exitfile ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FGMS::set_resetfile 
(
	const std::string& name
)
{
	m_reset_filename = name;
} // FGMS::set_resetfile ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FGMS::set_statsfile
(
	const std::string& name
)
{
	m_stats_filename = name;
} // FGMS::set_statsfile ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FGMS::set_msglogfile
(
	const std::string& name
)
{
	m_msglog_filename = name;
} // FGMS::set_msglogfile ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FGMS::set_updatetracker
(
	time_t freq
)
{
	m_update_tracker_freq = freq;
} // FGMS::set_updatetracker ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FGMS::tracker_log
(
	const std::string& msg,
	const char* src
)
{
	if ( msg.size () == 0 )
		return;
	if ( ! m_tracker_log.is_open() )
	{
		m_tracker_log.open ( m_msglog_filename, std::ios::out|std::ios::app );;
		if ( ! m_tracker_log.is_open() )
		{
			LOG ( log_prio::ERROR,
			  "ERROR: Failed to OPEN/append "
			  << m_msglog_filename << " file !" )
			return;
		}
	}
	// write timestamp
	time_t timestamp = time ( 0 );
	tm*  tm;
	tm = gmtime ( & timestamp ); // Creates the UTC time string
	m_tracker_log << std::setfill ('0')
		<< std::setw (4) << tm->tm_year+1900 << "-"
		<< std::setw (2) << tm->tm_mon+1 << "-"
		<< tm->tm_mday << " "
		<< tm->tm_hour << ":"
		<< tm->tm_min << ":"
		<< tm->tm_sec << " ";
	if ( src && strlen ( src ) )
	{
		m_tracker_log << src;
	}
	m_tracker_log << msg << std::endl;
	m_tracker_log.flush ();
} // tracker_log ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Close sockets, logfile etc.
 */
void
FGMS::done()
{
	if ( m_initialized == false )
		return;
	if ( ! m_is_parent )
	{
		return;
	}
	LOG ( log_prio::URGENT, "FGMS::done() - exiting" );
	show_stats ();   // 20150619:0.11.9: add stats to the LOG on exit
	if ( m_listening == false )
	{
		return;
	}
	if ( m_query_channel )
	{
		m_query_channel->close();
		delete m_query_channel;
		m_query_channel = 0;
	}
	if ( m_admin_channel )
	{
		m_admin_channel->close();
		delete m_admin_channel;
		m_admin_channel = 0;
	}
	if ( m_data_channel )
	{
		m_data_channel->close();
		delete m_data_channel;
		m_data_channel = 0;
	}
	close_tracker ();
	m_player_list.unlock ();
	m_player_list.clear ();
	m_relay_list.unlock ();
	m_relay_list.clear ();
	m_cross_list.unlock ();
	m_cross_list.clear ();
	m_black_list.unlock ();
	m_black_list.clear ();
	m_relay_map.clear ();	// clear(): is a std::map (NOT a FG_List)
	m_listening = false;
	m_initialized = false;
} // FGMS::done()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Updates the remote tracker web server
 */
int
FGMS::update_tracker
(
	const string& name,
	const string& passwd,
	const string& model_name,
	const time_t timestamp,
	const int type
)
{
	char            time_str[100];
	FG_Player	player;
	Point3D         playerpos_geod;
	string          aircraft;
	string          message;
	tm*             tm;
	if ( ! m_is_tracked || ( name == "mpdummy" ) )
	{
		return ( 1 );
	}
	// Creates the UTC time string
	tm = gmtime ( & timestamp );
	sprintf (
		time_str,
		"%04d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year+1900,
		tm->tm_mon+1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec
	);
	// Edits the aircraft name string
	size_t index = model_name.rfind ( "/" );
	if ( index != string::npos )
	{
		aircraft = model_name.substr ( index + 1 );
	}
	else
	{
		aircraft = model_name;
	}
	index = aircraft.find ( ".xml" );
	if ( index != string::npos )
	{
		aircraft.erase ( index );
	}
	// Creates the message
	if ( type == CONNECT )
	{
		message  = "CONNECT ";
		message += name;
		message += " ";
		message += passwd;
		message += " ";
		message += aircraft;
		message += " ";
		message += time_str;
		// queue the message
		m_tracker->add_message ( message );
#ifdef ADD_TRACKER_LOG
		tracker_log ( message, "IN: " ); // write message log
#endif // #ifdef ADD_TRACKER_LOG
		m_tracker_connect++; // count a CONNECT message queued
		return ( 0 );
	}
	else if ( type == DISCONNECT )
	{
		message  = "DISCONNECT ";
		message += name;
		message += " ";
		message += passwd;
		message += " ";
		message += aircraft;
		message += " ";
		message += time_str;
		// queue the message
		m_tracker->add_message ( message );
#ifdef ADD_TRACKER_LOG
		tracker_log ( message, "IN: " ); // write message log
#endif // #ifdef ADD_TRACKER_LOG
		m_tracker_disconnect++; // count a DISCONNECT message queued
		return ( 0 );
	}
	// we only arrive here if type!=CONNECT and !=DISCONNECT
	message = "";
	float heading, pitch, roll;
	size_t j=0; /*message count*/
	for ( size_t i = 0; i < m_player_list.size(); i++ )
	{
		player = m_player_list[i];
		if ( player.id == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		euler_get ( playerpos_geod[Point3D::LAT], playerpos_geod[Point3D::LON],
			    player.last_orientation[Point3D::X], player.last_orientation[Point3D::Y], player.last_orientation[Point3D::Z],
			    &heading, &pitch, &roll );
		if ( ( player.is_local ) && ( player.has_errors == false ) )
		{
			if ( j!=0 )
			{
				message += "\n";
			}
			cart_to_geod ( player.last_pos, playerpos_geod );
			message +=  "POSITION ";
			message += player.name +" ";
			message += player.passwd +" ";
			message += num_to_str ( playerpos_geod[Point3D::LAT], 6 ) +" ";
			message += num_to_str ( playerpos_geod[Point3D::LON], 6 ) +" ";
			message += num_to_str ( playerpos_geod[Point3D::ALT], 6 ) +" ";
			message += num_to_str ( heading, 6 ) +" ";
			message += num_to_str ( pitch,   6 ) +" ";
			message += num_to_str ( roll,    6 ) +" ";
			message += time_str;
			// queue the message
			j++;
		}
#ifdef TRACK_ALL
		if ( !player.is_local )
		{
			if ( j!=0 )
			{
				message += "\n";
			}
			cart_to_geod ( player.last_pos, playerpos_geod );
			message +=  "POSITION ";
			message += player.name +" ";
			message += player.passwd +" ";
			message += num_to_str ( playerpos_geod[Point3D::LAT], 6 ) +" ";
			message += num_to_str ( playerpos_geod[Point3D::LON], 6 ) +" ";
			message += num_to_str ( playerpos_geod[Point3D::ALT], 6 ) +" ";
			message += num_to_str ( heading, 6 ) +" ";
			message += num_to_str ( pitch,   6 ) +" ";
			message += num_to_str ( roll,    6 ) +" ";
			message += time_str;
			// queue the message
			j++;
		}
#endif
	} // while
	if ( message!= "" )
	{
		m_tracker->add_message ( message );
		m_tracker_position++; // count a POSITION messge queued
	}
	message.erase ( 0 );
	return ( 0 );
} // update_tracker (...)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Cleanly closes the tracker
 */
void
FGMS::close_tracker
()
{
	if ( m_is_tracked )
	{
		if ( m_tracker )
		{
			m_tracker->set_want_exit ();
			pthread_cond_signal ( m_tracker->get_cond_var() );  // wake up the worker
			pthread_join ( m_tracker->get_thread_id(), 0 );
		}
		m_tracker = 0;
		m_is_tracked = false;
	}
} // close_tracker ( )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 */
bool
FGMS::receiver_wants_data
(
	const PlayerIt& sender,
	const FG_Player& receiver
)
{
	if ( distance ( sender->last_pos, receiver.last_pos ) < receiver.radar_range )
	{
		return true;
	}
	return false;
	// TODO:
	float	out_of_reach;
	using fgmp::ATC_TYPE;
	if ( ( sender->is_ATC == ATC_TYPE::NONE )
	&&   ( receiver.is_ATC == ATC_TYPE::NONE ) )
	{
		// sender and Receiver are normal pilots, so m_out_of_reach applies
		if ( distance ( sender->last_pos, receiver.last_pos ) < receiver.radar_range )
		{
			return true;
		}
		return false;
	}
	//////////////////////////////////////////////////
	//
	// either sender or receiver is an ATC
	//
	//////////////////////////////////////////////////
	/* range of ATC, see https://forums.vatsim.net/viewtopic.php?f=7&t=56924 */
	if ( receiver.is_ATC != ATC_TYPE::NONE )
	{
		switch ( receiver.is_ATC )
		{
		case ATC_TYPE::ATC_DL:
		case ATC_TYPE::ATC_GN:
			out_of_reach = 5;
			break;
		case ATC_TYPE::ATC_TW:
			out_of_reach = 30;
			break;
		case ATC_TYPE::ATC_AP:
		case ATC_TYPE::ATC_DE:
			out_of_reach = 100;
			break;
		case ATC_TYPE::ATC_CT:
			out_of_reach = 400;
			break;
		default:
			out_of_reach = m_out_of_reach;
		}
	}
	else if ( sender->is_ATC != ATC_TYPE::NONE )
	{
		// FIXME: if sender is the ATC, the pos-data does not need to be send.
		//        but we can not implement it before chat- and pos-messages are
		//        sent seperatly. For now we leave it to be
		//        m_out_of_reach
		out_of_reach = m_out_of_reach;
		// return false;
	}
	if ( distance ( sender->last_pos, receiver.last_pos ) < out_of_reach )
	{
		return true;
	}
	return false;
} // FGMS::receiver_wants_data ( player, player )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 */
bool
FGMS::receiver_wants_chat
(
	const PlayerIt&  sender,
	const FG_Player& receiver
)
{
	float	out_of_reach;
	float	altitude;
	//////////////////////////////////////////////////
	// If the sender is an ATC use a predefined
	// range for radio transmission. For now we
	// use m_out_of_reach
	//////////////////////////////////////////////////
	if ( sender->is_ATC != fgmp::ATC_TYPE::NONE )
	{
		if ( distance ( sender->last_pos, receiver.last_pos ) <
		m_out_of_reach )
		{
			return true;
		}
		return false;
	}
	//////////////////////////////////////////////////
	// If the sender is NOT an ATC use a variable
	// value for range, depending on the senders
	// altitude
	//////////////////////////////////////////////////
	/* drr0ckso:
	Here is a table of values, based on the assumption of an airplane flying
	over water, talking from the airplane (altitude above mean sea level
	[AMSL]) to the ground or a boat (sea level [SL]), or vice versa. Yes, I
	was a little curious myself, too. :) Of course, this does not consider
	consider path loss over the distance, or AM versus FM or SSB. Generally
	speaking, the closer to line-of-sight the communication is, the more
	likely it is to work.
	*/
	altitude = sender->geod_pos[Point3D::ALT];
	if ( altitude < 1000.0 )
	{
		out_of_reach = 39.1;
	}
	else if ( altitude < 2.000 )
	{
		out_of_reach = 54.75;
	}
	else if ( altitude < 3.000 )
	{
		out_of_reach = 66.91;
	}
	else if ( altitude < 4.000 )
	{
		out_of_reach = 77.34;
	}
	else if ( altitude < 5.000 )
	{
		out_of_reach = 86.90;
	}
	else if ( altitude < 6.000 )
	{
		out_of_reach = 95.59;
	}
	else if ( altitude < 7.000 )
	{
		out_of_reach = 102.54;
	}
	else if ( altitude < 8.000 )
	{
		out_of_reach = 109.50;
	}
	else if ( altitude < 9.000 )
	{
		out_of_reach = 116.44;
	}
	else if ( altitude < 10.000 )
	{
		out_of_reach = 122.53;
	}
	else if ( altitude < 15.000 )
	{
		out_of_reach = 150.33;
	}
	else if ( altitude < 20.000 )
	{
		out_of_reach = 173.80;
	}
	else if ( altitude < 25.000 )
	{
		out_of_reach = 194.65;
	}
	else if ( altitude < 30.000 )
	{
		out_of_reach = 212.90;
	}
	else if ( altitude < 35.000 )
	{
		out_of_reach = 230.29;
	}
	else if ( altitude < 40.000 )
	{
		out_of_reach = 283;
	}
	else if ( altitude < 45.000 )
	{
		out_of_reach = 300;
	}
	else if ( altitude < 50.000 )
	{
		out_of_reach = 316;
	}
	else if ( altitude < 55.000 )
	{
		out_of_reach = 332;
	}
	else if ( altitude < 60.000 )
	{
		out_of_reach = 346;
	}
	else if ( altitude < 65.000 )
	{
		out_of_reach = 361;
	}
	else if ( altitude < 70.000 )
	{
		out_of_reach = 374;
	}
	else if ( altitude < 75.000 )
	{
		out_of_reach = 387;
	}
	else if ( altitude < 80.000 )
	{
		out_of_reach = 400;
	}
	else if ( altitude < 85.000 )
	{
		out_of_reach = 412;
	}
	else if ( altitude < 90.000 )
	{
		out_of_reach = 424;
	}
	else if ( altitude < 100.000 )
	{
		out_of_reach = 447;
	}
	else if ( altitude < 125.000 )
	{
		out_of_reach = 500;
	}
	else if ( altitude < 250.000 )
	{
		out_of_reach = 707;
	}
	else if ( altitude < 500.000 )
	{
		out_of_reach = 1000;
	}
	else
	{
		out_of_reach = 1500;
	}
	if ( distance ( sender->last_pos, receiver.last_pos ) < out_of_reach )
	{
		return true;
	}
	return false;
} // FGMS::receiver_wants_chat ( player, player )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Decide whether the relay is interested in full rate updates.
 * @see \ref server_out_of_reach config.
 * @param Relay
 * @param sender
 * @retval true is within range
 */
bool
FGMS::is_in_range
(
	const ListElement& relay,
	const PlayerIt& sender,
	uint32_t msg_id
)
{
	FG_Player player;
	size_t    cnt;
	cnt = m_player_list.size ();
	for ( size_t i = 0; i < cnt; i++ )
	{
		player = m_player_list[i];
		if ( player.id == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		if ( player.address == relay.address )
		{
			if ( msg_id == FGFS::CHAT_MSG )
			{
				// apply 'radio' rules
				// if ( receiver_wants_chat( sender, player ) )
				if ( receiver_wants_data ( sender, player ) )
				{
					return true;
				}
			}
			else
			{
				// apply 'visibility' rules, for now we apply 'radio' rules
				if ( receiver_wants_data ( sender, player ) )
				{
					return true;
				}
			}
			return false;
		}
	}
	return false;
} // FGMS::is_in_range( relay, player )

//////////////////////////////////////////////////////////////////////

/** Read a config file and set internal variables accordingly
 *
 * @param config_name Path of config file to load
 * @retval int  -- todo--
 */
bool
FGMS::process_config
(
	const string& config_name
)
{
	FG_CONFIG   config;
	string      val;
	int         e;
	if ( m_have_config )	// we already have a config, so ignore
	{
		return ( true );
	}
	if ( config.read ( config_name ) )
	{
		LOG ( log_prio::URGENT,
		  "Could not read config file '" << config_name
		  << "' => using defaults");
		return ( false );
	}
	LOG ( log_prio::ERROR, "processing " << config_name );
	m_config_file =  config_name;
	val = config.get ( "server.name" );
	if ( val != "" )
	{
		m_server_name = val;
	}
	val = config.get ( "server.address" );
	if ( val != "" )
	{
		m_bind_addr = val;
	}
	val = config.get ( "server.FQDN" );
	if ( val != "" )
	{
		m_FQDN = val;
	}
	val = config.get ( "server.port" );
	if ( val != "" )
	{
		set_data_port ( str_to_num<int> ( val, e ) );
		if ( e )
		{
			LOG ( log_prio::URGENT,
			  "invalid value for DataPort: '" << val << "'"
			);
			exit ( 1 );
		}
	}
	val = config.get ( "server.telnet_port" );
	if ( val != "" )
	{
		set_query_port ( str_to_num<int> ( val, e ) );
		if ( e )
		{
			LOG ( log_prio::URGENT,
			  "invalid value for TelnetPort: '" << val << "'"
			);
			exit ( 1 );
		}
	}
	val = config.get ( "server.admin_cli" );
	if ( val != "" )
	{
		if ( ( val == "on" ) || ( val == "true" ) )
		{
			m_add_cli = true;
		}
		else if ( ( val == "off" ) || ( val == "false" ) )
		{
			m_add_cli = false;
		}
		else
		{
			LOG ( log_prio::URGENT,
			  "unknown value for 'server.admin_cli'!"
			  << " in file " << config_name
			);
		}
	}
	val = config.get ( "server.admin_port" );
	if ( val != "" )
	{
		set_admin_port ( str_to_num<int> ( val, e ) );
		if ( e )
		{
			LOG ( log_prio::URGENT,
			  "invalid value for AdminPort: '" << val << "'"
			);
			exit ( 1 );
		}
	}
	val = config.get ( "server.admin_user" );
	if ( val != "" )
	{
		m_admin_user = val;
	}
	val = config.get ( "server.admin_pass" );
	if ( val != "" )
	{
		m_admin_pass = val;
	}
	val = config.get ( "server.admin_enable" );
	if ( val != "" )
	{
		m_admin_enable = val;
	}
	val = config.get ( "server.out_of_reach" );
	if ( val != "" )
	{
		m_out_of_reach = str_to_num<int> ( val, e );
		if ( e )
		{
			LOG ( log_prio::URGENT,
			  "invalid value for out_of_reach: '" << val << "'"
			);
			exit ( 1 );
		}
	}
	val = config.get ( "server.max_radar_range" );
	if ( val != "" )
	{
		m_max_radar_range = str_to_num<int> ( val, e );
		if ( e )
		{
			LOG ( log_prio::URGENT,
			  "invalid value for max_radar_range: '" << val
			  << "'"
			);
			exit ( 1 );
		}
	}
	val = config.get ( "server.playerexpires" );
	if ( val != "" )
	{
		m_player_expires = str_to_num<int> ( val, e );
		if ( e )
		{
			LOG ( log_prio::URGENT,
			  "invalid value for Expire: '" << val << "'"
			);
			exit ( 1 );
		}
	}
	val = config.get ( "server.logfile" );
	if ( val != "" )
	{
		set_logfile ( val );
	}
	val = config.get ( "server.daemon" );
	if ( val != "" )
	{
		if ( ( val == "on" ) || ( val == "true" ) )
		{
			m_run_as_daemon = true;
		}
		else if ( ( val == "off" ) || ( val == "false" ) )
		{
			m_run_as_daemon = false;
		}
		else
		{
			LOG ( log_prio::URGENT,
			  "unknown value for 'server.daemon'!"
			  << " in file " << config_name
			);
		}
	}
	val = config.get ( "server.tracked" );
	if ( val != "" )
	{
		string  server;
		int     port;
		bool    tracked = false;

		if ( val == "true" )
		{
			tracked = true;
			server = config.get ( "server.tracking_server" );
			val = config.get ( "server.tracking_port" );
			port = str_to_num<int> ( val, e );
			if ( e )
			{
				LOG ( log_prio::URGENT,
				  "invalid value for tracking_port: '"
				  << val << "'"
				);
				exit ( 1 );
			}
			if ( tracked
			&& ( ! add_tracker ( server, port, tracked ) ) ) // set master m_is_tracked
			{
				LOG ( log_prio::URGENT,
				  "Failed to get IPC msg queue ID! error "
				  << errno );
				exit ( 1 ); // do NOT continue if a requested 'tracker' FAILED
			}
		}
	}
	val = config.get ( "server.is_hub" );
	if ( val != "" )
	{
		if ( val == "true" )
		{
			m_me_is_hub = true;
		}
		else
		{
			m_me_is_hub = false;
		}
	}
	//////////////////////////////////////////////////
	//      read the list of relays
	//////////////////////////////////////////////////
	bool    more_to_read  = true;
	string  var;
	string  server = "";
	int     port   = 0;
	if ( ! config.set_section ( "relay" ) )
	{
		more_to_read = false;
	}
	while ( more_to_read )
	{
		var = config.get_name ();
		val = config.get_value();
		if ( var == "relay.host" )
		{
			server = val;
		}
		if ( var == "relay.port" )
		{
			port = str_to_num<int> ( val, e );
			if ( e )
			{
				LOG ( log_prio::URGENT,
				  "invalid value for RelayPort: '"
				  << val << "'"
				);
				exit ( 1 );
			}
		}
		if ( ( server != "" ) && ( port != 0 ) )
		{
			add_relay ( server, port );
			server = "";
			port   = 0;
		}
		if ( config.sec_next () == 0 )
		{
			more_to_read = false;
		}
	}
	//////////////////////////////////////////////////
	//      read the list of crossfeeds
	//////////////////////////////////////////////////
	more_to_read  = true;
	var    = "";
	server = "";
	port   = 0;
	if ( ! config.set_section ( "crossfeed" ) )
	{
		more_to_read = false;
	}
	while ( more_to_read )
	{
		var = config.get_name ();
		val = config.get_value();
		if ( var == "crossfeed.host" )
		{
			server = val;
		}
		if ( var == "crossfeed.port" )
		{
			port = str_to_num<int> ( val, e );
			if ( e )
			{
				LOG ( log_prio::URGENT,
				  "invalid value for crossfeed.port: '"
				  << val << "'"
				);
				exit ( 1 );
			}
		}
		if ( ( server != "" ) && ( port != 0 ) )
		{
			add_crossfeed ( server, port );
			server = "";
			port   = 0;
		}
		if ( config.sec_next () == 0 )
		{
			more_to_read = false;
		}
	}

	//////////////////////////////////////////////////
	//      read the list of whitelisted IPs
	//      (a crossfeed might list the sender here
	//      to avoid blacklisting without defining the
	//      sender as a relay)
	//////////////////////////////////////////////////
	more_to_read  = true;
	var    = "";
	val    = "";
	if ( ! config.set_section ( "whitelist" ) )
	{
		more_to_read = false;
	}
	while ( more_to_read )
	{
		var = config.get_name ();
		val = config.get_value();
		if ( var == "whitelist" )
		{
			add_whitelist ( val.c_str() );
		}
		if ( config.sec_next () == 0 )
		{
			more_to_read = false;
		}
	}

	//////////////////////////////////////////////////
	//      read the list of blacklisted IPs
	//////////////////////////////////////////////////
	more_to_read  = true;
	var    = "";
	val    = "";
	if ( ! config.set_section ( "blacklist" ) )
	{
		more_to_read = false;
	}
	while ( more_to_read )
	{
		var = config.get_name ();
		val = config.get_value();
		if ( var == "blacklist" )
		{
			add_blacklist ( val.c_str(),
			  "static config entry", 0 );
		}
		if ( config.sec_next () == 0 )
		{
			more_to_read = false;
		}
	}

	//////////////////////////////////////////////////
	return ( true );
} // FGMS::process_config ( const string& config_name )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print a help screen for command line parameters, see \ref command_line
 */
void
FGMS::print_version
()
{
	std::cout << std::endl;
	std::cout << "fgms version " << m_version
	     << ", compiled on " << __DATE__
	     << " at " << __TIME__ << std::endl;
	std::cout << std::endl;
} // FGMS::print_version()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print a help screen for command line parameters, see \ref command_line
 */
void
FGMS::print_help
()
{
	print_version ();
	cout << "syntax: " << m_argv[0] << " options" << endl;
	cout << "\n"
	     "options are:\n"
	     "-h            print this help screen\n"
	     "-a PORT       listen to PORT for telnet\n"
	     "-c config     read 'config' as configuration file\n"
	     "-p PORT       listen to PORT\n"
	     "-t TTL        Time a client is active while not sending packets\n"
	     "-o OOR        nautical miles two players must be apart to be out of reach\n"
	     "-l LOGFILE    Log to LOGFILE\n"
	     "-v LEVEL      verbosity (loglevel) in range 0 (few) and 3 (much). 5 to disable. (def=" << logger.priority() << ")\n"
	     "-d            do _not_ run as a daemon (stay in foreground)\n"
	     "-D            do run as a daemon (default)\n"
	     "\n";
	exit ( 0 );
} // FGMS::print_help ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** Parse commandline parameters
 *
 * @param  argc
 * @param  argv
 * @retval int 1 on success
 */
int
FGMS::parse_params
(
	int   argc,
	char* argv[]
)
{
	int     m;
	int     e;
	m_argc = argc;
	m_argv = argv;
	while ( ( m=getopt ( argc, argv, "a:b:c:dDhl:o:p:t:v:" ) ) != -1 )
	{
		switch ( m )
		{
		case 'h':
			print_help ();
			break; // never reached
		case 'a':
			set_query_port ( str_to_num<int> ( optarg, e ) );
			if ( e )
			{
				cerr << "invalid value for TelnetPort: '" << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'b':
			set_admin_port ( str_to_num<int> ( optarg, e ) );
			if ( e )
			{
				cerr << "invalid value for AdminPort: '" << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'c':
			process_config ( optarg );
			break;
		case 'p':
			set_data_port ( str_to_num<int>  ( optarg, e ) );
			if ( e )
			{
				cerr << "invalid value for DataPort: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'o':
			m_out_of_reach = str_to_num<int> ( optarg, e );
			if ( e )
			{
				cerr << "invalid value for OutOfReach: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'v':
			logger.priority ( static_cast<log_prio> (str_to_num<int> ( optarg, e ) ) );
			if ( e )
			{
				cerr << "invalid value for Loglevel: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 't':
			m_player_expires = str_to_num<int> ( optarg, e );
			if ( e )
			{
				cerr << "invalid value for expire: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'l':
			set_logfile ( optarg );
			break;
		case 'd':
			m_run_as_daemon = false;
			break;
		case 'D':
			m_run_as_daemon = true;
			break;
		default:
			cerr << endl << endl;
			print_help ();
			exit ( 1 );
		} // switch ()
	} // while ()
	return ( 1 ); // success
} // parse_params()

//////////////////////////////////////////////////////////////////////

/**
 * @brief  (re)Read config files
 * @param ReInit True to reinitialize
 */
int
FGMS::read_configs
(
	bool ReInit	// FIXME: not used?
)
{
	string path;
#ifndef _MSC_VER
	path = SYSCONFDIR;
	path += "/";
	path += m_config_name;
	if ( process_config ( path ) == true )
	{
		return 1;
	}
	path = getenv ( "HOME" );
#else
	char* cp = getenv ( "HOME" );
	if ( cp )
	{
		path = cp;
	}
	else
	{
		cp = getenv ( "USERPROFILE" ); // XP=C:\Documents and Settings\<name>, Win7=C:\Users\<user>
		if ( cp )
		{
			path = cp;
		}
	}
#endif
	if ( path != "" )
	{
		path += "/"; // DEF_CONF_FILE;
		path += m_config_name;
		if ( process_config ( path ) )
		{
			return 1;
		}
	}
	if ( process_config ( m_config_name ) )
	{
		return 1;
	}
	return 0;
} // FGMS::read_configs ()

//////////////////////////////////////////////////////////////////////

/** Check configuration
 *
 * Check some configuration values for witted values
 *
 * @return true		everything OK
 * @return false	some value should be tweaked
 */
bool
FGMS::check_config
()
{
	if ( ( m_server_name == "fgms" ) && ( m_relay_list.size () > 0) )
	{
		LOG ( log_prio::ERROR, "If you want to provide a public "
		  << "server, please provide a unique server name!"
			
		);
		return false;
	}
	return true;
} // FGMS::check_config ()

//////////////////////////////////////////////////////////////////////


