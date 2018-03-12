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
 * @file        fgms.cxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 *              and contributors (see AUTHORS)
 * @date        2005-2017
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
#include <fglib/daemon.hxx>
#else
#include <libmsc/msc_getopt.hxx>
#include <conio.h> // for _kbhit(), _getch
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <fglib/fg_util.hxx>
#include <fglib/fg_log.hxx>
#include <fglib/fg_util.hxx>
#include <fglib/fg_config.hxx>
#include <fglib/fg_version.hxx>
#include <fglib/fg_log.hxx>
#include "fg_cli.hxx"
#include "fgms.hxx"

extern void SigHUPHandler ( int SigType ); // main.cxx

namespace fgmp
{

const version fgms::m_version ( 1, 0, 0, "-dev3" );

using prio = fglog::prio;

//////////////////////////////////////////////////////////////////////

/**
 * @class fgms
 */

//////////////////////////////////////////////////////////////////////

/** Constructor
 */
fgms::fgms
() :    m_cross_list ( "Crossfeed" ),
	m_white_list ( "Whitelist" ),
	m_black_list ( "Blacklist" ),
	m_relay_list ( "Relays" ),
	m_player_list ( "Users" )
{
	int16_t* converter = ( int16_t* ) ( & PROTO_VER );
	m_proto_minor_version   = converter[0]; // ->High;
	m_proto_major_version   = converter[1]; // ->Low;
	m_relay_map             = ip2relay_t ();
	// Be able to enable/disable file interface
	// On start-up if the file already exists, disable
	struct stat buf;
	m_use_exit_file  = ( stat ( m_exit_filename.c_str(), &buf ) ) ? true : false;
	// caution: this has failed in the past
	m_use_reset_file = ( stat ( m_reset_filename.c_str(), &buf ) ) ? true : false;
	m_use_stat_file  = ( stat ( m_stats_filename.c_str(), &buf ) ) ? true : false;
	m_uptime = time ( 0 );
} // fgms::fgms()

//////////////////////////////////////////////////////////////////////

/**
 * destructor
 */
fgms::~fgms
()
{
	shutdown ();
} // fgms::~fgms()

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
	fgms* tmp_server = t->instance;
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
	fgms* tmp_server = t->instance;
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
	tracker* pt = reinterpret_cast<tracker*> ( vp );
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
fgms::init_data_channel
()
{
	if ( ! m_reinit_data )
	{
		return true;
	}
	if ( m_data_channel != nullptr )
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
		LOG ( prio::EMIT, "fgms::init() - "
		      << "failed to bind to " << m_data_port );
		LOG ( prio::EMIT, "already in use?" );
		LOG ( prio::EMIT, e.what() );
		return false;
	}
	return true;
} // fgms::init_data_channel ()

//////////////////////////////////////////////////////////////////////

/** Initialise the query channel
 *
 * @return true OK
 * @return false sonething went wrong
 */
bool
fgms::init_query_channel
()
{
	if ( ! m_reinit_query )
	{
		return true;
	}
	if ( m_query_channel != nullptr )
	{
		delete m_query_channel;
	}
	m_query_channel = nullptr;
	m_reinit_query = false;
	if ( m_query_port == 0 )
	{
		return true;        // query port disabled
	}
	m_query_channel = new fgmp::netsocket;
	try
	{
		m_query_channel->listen_to ( m_bind_addr, m_query_port,
					     fgmp::netsocket::TCP );
	}
	catch ( std::runtime_error& e )
	{
		LOG ( prio::EMIT, "fgms::init() - "
		      << "failed to listen to query port" );
		return false;
	}
	return true;
} // fgms::init_query_channel ()

//////////////////////////////////////////////////////////////////////

/** Initialise the admin channel
 *
 * @return true OK
 * @return false sonething went wrong
 */
bool
fgms::init_admin_channel
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
	m_admin_channel = nullptr;
	m_reinit_admin = false;
	if ( ( m_admin_port == 0 ) || ( m_admin_cli == false ) )
	{
		return true;        // admin channel disabled
	}
	m_admin_channel = new fgmp::netsocket;
	try
	{
		m_admin_channel->listen_to ( m_bind_addr, m_admin_port,
					     fgmp::netsocket::TCP );
	}
	catch ( std::runtime_error& e )
	{
		LOG ( prio::EMIT, "fgms::init() - "
		      << "could not create socket for admin" );
		return false;
	}
	return true;
} // fgms::init_admin_channel ()

//////////////////////////////////////////////////////////////////////

void
fgms::set_bind_addr
(
	const std::string& addr
)
{
	if ( m_bind_addr == addr )
	{
		return;
	}
	m_bind_addr = addr;
	m_reinit_data = true;
	m_reinit_query = true;
	m_reinit_admin = true;
	init_data_channel ();
	init_query_channel ();
	init_admin_channel ();
} // fgms::set_bind_addr ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Basic initialization
 *
 *  If we are already initialized, close
 *  all connections and re-init all variables
 */
bool
fgms::init
()
{
	if ( m_logfile_name != "" )
	{
		open_logfile ();
	}
	logger.priority ( m_debug_level );
	logger.set_flags ( fglog::flags::WITH_DATE );
	LOG ( prio::EMIT, "# FlightGear Multiplayer Server v"
	      << m_version.str() << " started" );
	if ( m_listening )
	{
		shutdown ();
	}
	m_listening       = false;
	m_data_channel    = 0;
	m_num_max_clients = 0;
	if ( ! init_data_channel () )
	{
		return false;
	}
	if ( ! init_query_channel () )
	{
		return false;
	}
	if ( ! init_admin_channel () )
	{
		return false;
	}
	LOG ( prio::EMIT, "# This is " << m_hostname << " (" << m_FQDN << ")" );
	LOG ( prio::EMIT, "# using protocol version v"
	      << m_proto_major_version << "." << m_proto_minor_version
	      << " (LazyRelay enabled)" );
	LOG ( prio::EMIT, "# listening to port " << m_data_port );
	if ( m_query_channel )
	{
		LOG ( prio::EMIT, "# telnet port " << m_query_port );
	}
	else
	{
		LOG ( prio::EMIT, "# telnet port DISABLED" );
	}
	if ( m_admin_channel )
	{
		LOG ( prio::EMIT, "# admin port " << m_admin_port );
	}
	else
	{
		LOG ( prio::EMIT, "# admin port DISABLED" );
	}
	if ( m_bind_addr != "" )
	{
		LOG ( prio::EMIT, "# listening on " << m_bind_addr );
	}
	if ( m_me_is_hub )
	{
		LOG ( prio::EMIT, "# I am a HUB Server" );
	}
	if ( ( m_is_tracked ) && ( m_tracker != nullptr ) )
	{
		pthread_t th;
		pthread_create ( &th, NULL, &detach_tracker, m_tracker );
		LOG ( prio::EMIT, "# tracked to "
		      << m_tracker->get_server ()
		      << ":" << m_tracker->get_port ()
		      << ", using a thread."
		    );
	}
	else
	{
		LOG ( prio::EMIT, "# tracking is disabled." );
	}
	size_t count;
	list_item entry ( "" );
	//////////////////////////////////////////////////
	// print list of all relays
	//////////////////////////////////////////////////
	count = m_relay_list.size();
	LOG ( prio::EMIT, "# I have " << count << " relays" );
	for ( size_t i = 0; i < count; i++ )
	{
		entry = m_relay_list[i];
		if ( entry.id == list_item::NONE_EXISTANT )
		{
			continue;
		}
		LOG ( prio::EMIT, "# relay " << entry.name
		      << ":" << entry.address.port()
		      << " (" << entry.address << ")" );
	}
	//////////////////////////////////////////////////
	// print list of all crossfeeds
	//////////////////////////////////////////////////
	count = m_cross_list.size();
	LOG ( prio::EMIT, "# I have " << count << " crossfeeds" );
	for ( size_t i = 0; i < count; i++ )
	{
		entry = m_cross_list[i];
		if ( entry.id == list_item::NONE_EXISTANT )
		{
			continue;
		}
		LOG ( prio::EMIT, "# crossfeed " << entry.name
		      << ":" << entry.address.port()
		    );
	}
	LOG ( prio::EMIT, "# I have " << m_black_list.size()
	      << " blacklisted IPs"
	    );
	if ( m_use_exit_file && m_use_stat_file )
	{
		// only show this IFF both are enabled
		LOG ( prio::EMIT, "# Files: exit=[" << m_exit_filename
		      << "] stat=[" << m_stats_filename << "]"
		    );
	}
	m_listening = true;
#ifndef _MSC_VER
	if ( m_run_as_daemon )
	{
		m_myself.daemonize ();
		LOG ( prio::URGENT, "# My PID is " << m_myself.get_pid() );
	}
#endif
	return true;
} // fgms::init()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Handle an admin session.
 *
 * If a telnet connection to the admin port is established, a new fgcli
 * instance is created.
 * @param Fd -- docs todo --
 */
void*
fgms::handle_admin
(
	int fd
)
{
	fgcli*  my_cli;
	errno = 0;
	my_cli = new fgcli ( this, fd );
	my_cli->loop ();
	if ( fd == 0 )
	{
		// reading from stdin
		want_exit();
	}
	delete my_cli;
	return ( 0 );
} // fgms::handle_admin()

//////////////////////////////////////////////////////////////////////

/** Generate the list of currently connected clients
 *
 */
void
fgms::mk_client_list
()
{
	std::string     message;
	pilot   player;
	unsigned int    it;
	m_clients.lock ();
	m_clients.clear ();
	//////////////////////////////////////////////////
	//
	//      create the output message
	//      header
	//
	//////////////////////////////////////////////////
	message  = "# This is " + m_hostname;
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
		message += num_to_str ( player.last_pos.x(), 6 ) +" ";
		message += num_to_str ( player.last_pos.y(), 6 ) +" ";
		message += num_to_str ( player.last_pos.z(), 6 ) +" ";
		message += num_to_str ( player.geod_pos.lat(), 6 ) +" ";
		message += num_to_str ( player.geod_pos.lon(), 6 ) +" ";
		message += num_to_str ( player.geod_pos.alt(), 6 ) +" ";
		message += num_to_str ( player.last_orientation.x(), 6 ) +" ";
		message += num_to_str ( player.last_orientation.y(), 6 ) +" ";
		message += num_to_str ( player.last_orientation.z(), 6 ) +" ";
		message += player.model_name;
		message += "\n";
		m_clients.push_back ( message );
	}
	m_clients.unlock ();
} // fgms::mk_client_list()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle a telnet session. if a telnet connection is opened, this
 *        method outputs a list  of all known clients.
 * @param Fd -- docs todo --
 */
void*
fgms::handle_query
(
	int fd
)
{
	str_it          line;
	fgmp::netsocket telnet;
	telnet.handle ( fd );
	errno = 0;
	m_clients.lock ();
	for ( line = m_clients.begin(); line != m_clients.end(); line++ )
	{
		if ( telnet.send ( *line ) < 0 )
		{
			if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
			{
				LOG ( prio::URGENT, "fgms::handle_query() - "
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
} // fgms::handle_query ()
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
fgms::add_bad_client
(
	const fgmp::netaddr& sender,
	std::string&    error_msg,
	bool    is_local,
	int     bytes
)
{
	std::string     message;
	pilot   new_player;
	pilot_it        player;
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
	LOG ( prio::MEDIUM, "fgms::add_bad_client() - " << error_msg );
	m_player_list.add ( new_player, m_player_expires );
	m_player_list.update_rcvd ( bytes );
	m_player_list.unlock();
} // fgms::add_bad_client ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new client to internal list
 * @param sender
 * @param msg
 */
void
fgms::add_client
(
	const fgmp::netaddr& sender,
	char* msg
)
{
	uint32_t        msg_magic;
	uint32_t        proto_ver;
	std::string     message;
	std::string     origin;
	msg_hdr_t*      msg_hdr;
	pos_msg_t*      pos_msg;
	pilot           new_player;
	bool            is_local;
	int16_t*        converter;
	msg_hdr         = ( msg_hdr_t* ) msg;
	pos_msg         = ( pos_msg_t* ) ( msg + sizeof ( msg_hdr_t ) );
	msg_magic       = XDR_decode_uint32 ( msg_hdr->magic );
	is_local        = true;
	if ( msg_magic == RELAY_MAGIC ) // not a local client
	{
		is_local = false;
	}
	proto_ver = XDR_decode_uint32 ( msg_hdr->version );
	converter = ( int16_t* ) & proto_ver;
	new_player.name     = msg_hdr->name;
	new_player.passwd    = "test"; //msg_hdr->passwd;
	new_player.model_name = "* unknown *";
	new_player.origin    = sender.to_string ();
	new_player.address   = sender;
	new_player.is_local   = is_local;
	new_player.proto_major  = converter[0];
	new_player.proto_minor  = converter[1];
	new_player.last_pos.set (
		XDR_decode_double ( pos_msg->position[point3d::X] ),
		XDR_decode_double ( pos_msg->position[point3d::Y] ),
		XDR_decode_double ( pos_msg->position[point3d::Z] )
	);
	new_player.last_orientation.set (
		XDR_decode_float ( pos_msg->orientation[point3d::X] ),
		XDR_decode_float ( pos_msg->orientation[point3d::Y] ),
		XDR_decode_float ( pos_msg->orientation[point3d::Z] )
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
	converter = ( int16_t* ) & msg_hdr->radar_range;
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
				 new_player.model_name, new_player.last_seen, tracker::CONNECT );
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
				 new_player.model_name, new_player.last_seen, tracker::CONNECT );
#endif
	}
	LOG ( prio::MEDIUM, message
	      << new_player.name << "@"
	      << origin << ":" << sender.port()
	      << " (" << new_player.model_name << ")"
	      << " current clients: "
	      << NumClients << " max: " << m_num_max_clients
	    );
} // fgms::add_client()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new relay server into internal list
 * @param Server
 * @param Port
 */
void
fgms::add_relay
(
	const std::string& relay,
	int port
)
{
	list_item b ( relay );
	b.address.assign ( relay, port );
	if ( ! b.address.is_valid () )
	{
		LOG ( prio::URGENT,
		      "could not resolve '" << relay << "'" );
		return;
	}
	m_relay_list.lock ();
	fglistit current_entry = m_relay_list.find ( b.address, true );
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
} // fgms::add_relay()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new crossfeed server into internal list
 * @param Server char with server
 * @param Port int with port number
 */
void
fgms::add_crossfeed
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
	list_item B ( s );
	B.address.assign ( ( char* ) s.c_str(), port );
	m_cross_list.lock ();
	fglistit current_entry = m_cross_list.find ( B.address, true );
	m_cross_list.unlock ();
	if ( current_entry == m_cross_list.end() )
	{
		m_cross_list.add ( B, 0 );
	}
} // fgms::add_crossfeed()
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
fgms::add_tracker
(
	const std::string& server,
	int  port,
	bool is_tracked
)
{
	close_tracker();
	m_is_tracked = is_tracked;
	m_tracker = new tracker ( port, server, m_hostname,
				  m_FQDN, m_version.str() );
	return true;
} // fgms::add_tracker()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Add an IP to the whitelist
 * @param FourDottedIP IP to add to whitelist
 */
void
fgms::add_whitelist
(
	const std::string& dotted_ip
)
{
	list_item B ( dotted_ip );
	B.address.assign ( dotted_ip.c_str() );
	m_white_list.lock ();
	fglistit current_entry = m_white_list.find ( B.address );
	m_white_list.unlock ();
	if ( current_entry == m_white_list.end() )
	{
		m_white_list.add ( B, 0 );
	}
} // fgms::add_whitelist()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief add an IP to the blacklist
 * @param FourDottedIP IP to add to blacklist
 */
void
fgms::add_blacklist
(
	const std::string& dotted_ip,
	const std::string& reason,
	time_t timeout
)
{
	list_item B ( reason );
	B.address.assign ( dotted_ip.c_str() );
	m_black_list.lock ();
	fglistit current_entry = m_black_list.find ( B.address );
	m_black_list.unlock ();
	if ( current_entry == m_black_list.end() )
	{
		// FIXME: every list has its own standard TTL
		m_black_list.add ( B, timeout );
	}
} // fgms::add_blacklist()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check if the sender is a known relay
 * @param sender_address
 * @retval bool true if known relay
 */
bool
fgms::is_known_relay
(
	const fgmp::netaddr& sender_address,
	size_t bytes
)
{
	fglistit current_entry;
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
	LOG ( prio::URGENT, "UNKNOWN RELAY: " << error_msg );
	return ( false );
} // fgms::is_known_relay ()
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
fgms::packet_is_valid
(
	int             bytes,
	msg_hdr_t*      msg_hdr,
	const fgmp::netaddr& sender_address
)
{
	uint32_t        msg_magic;
	uint32_t        msg_len;
	MSG_ID          msg_id;
	uint32_t        proto_ver;
	std::string     error_msg;
	std::string     origin;
	struct converter
	{
		int16_t         High;
		int16_t         Low;
	};
	converter*    tmp;
	origin    = sender_address.to_string ();
	msg_magic = XDR_decode_uint32 ( msg_hdr->magic );
	msg_id    = static_cast<MSG_ID> ( XDR_decode_uint32 ( msg_hdr->msg_id ) );
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
	if ( msg_id == MSG_ID::POS_DATA )
	{
		if ( msg_len < sizeof ( msg_hdr_t ) + sizeof ( pos_msg_t ) )
		{
			error_msg  = origin;
			error_msg += " Client sends insufficient position ";
			error_msg += "data, should be ";
			error_msg += num_to_str (
					     sizeof ( msg_hdr_t ) +sizeof ( pos_msg_t ) );
			error_msg += " is: " + num_to_str ( msg_hdr->msg_len );
			add_bad_client ( sender_address, error_msg, true, bytes );
			return ( false );
		}
	}
	return ( true );
} // fgms::packet_is_valid ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Send message to all crossfeed servers.
 *         Crossfeed servers receive all traffic without condition,
 *         mainly used for testing and debugging
 */
void
fgms::send_to_cross
(
	char* msg,
	int bytes,
	const fgmp::netaddr& sender_address
)
{
	msg_hdr_t*       msg_hdr;
	uint32_t        msg_magic;
	int             sent;
	fglistit                entry;
	msg_hdr         = ( msg_hdr_t* ) msg;
	msg_magic       = msg_hdr->magic;
	msg_hdr->magic  = XDR_encode_uint32 ( RELAY_MAGIC );
	m_cross_list.lock();
	for ( entry = m_cross_list.begin(); entry != m_cross_list.end(); entry++ )
	{
		sent = m_data_channel->send_to ( msg, bytes, entry->address );
		m_cross_list.update_sent ( entry, sent );
	}
	m_cross_list.unlock();
	msg_hdr->magic = msg_magic;  // restore the magic value
} // fgms::send_toCrossfeed ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Send message to all relay servers
 */
void
fgms::send_to_relays
(
	char* msg,
	int bytes,
	pilot_it& sender
)
{
	msg_hdr_t*       msg_hdr;
	uint32_t        msg_magic;
	MSG_ID          msg_id;
	unsigned int    pkts_forwarded = 0;
	fglistit                current_relay;
	if ( ( ! sender->is_local ) && ( ! m_me_is_hub ) )
	{
		return;
	}
	msg_hdr    = ( msg_hdr_t* ) msg;
	msg_magic  = XDR_decode_uint32 ( msg_hdr->magic );
	msg_id     = static_cast<MSG_ID> ( XDR_decode_uint32 ( msg_hdr->msg_id ) );
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
} // fgms::send_toRelays ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//      Remove Player from list
void
fgms::drop_client
(
	pilot_it& player
)
{
	std::string origin;
#ifdef TRACK_ALL
	update_tracker (
		player->name,
		player->passwd,
		player->model_name,
		player->last_seen,
		tracker::DISCONNECT );
#else
	if ( ( player->is_local ) && ( player->has_errors == false ) )
	{
		update_tracker (
			player->name,
			player->passwd,
			player->model_name,
			player->last_seen,
			tracker::DISCONNECT );
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
	LOG ( prio::MEDIUM, "TTL exceeded for "
	      << player->name << "@" << origin
	      << ", dropping after " << time ( 0 )-player->join_time
	      << " seconds. " << "Current clients: "
	      << m_player_list.size()-1 << " max: " << m_num_max_clients
	    );
	player = m_player_list.erase ( player );
} // fgms::drop_client()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle client connections
 * @param msg
 * @param bytes
 * @param sender_address
 */
void
fgms::handle_data
(
	char* msg,
	int   bytes,
	const fgmp::netaddr& sender_address
)
{
	msg_hdr_t*       msg_hdr;
	pos_msg_t*  pos_msg;
	MSG_ID          msg_id;
	uint32_t        msg_magic;
	pilot_it        sender;
	pilot_it        player;
	fglistit                current_entry;
	time_t          now;
	unsigned int    pkts_forwarded = 0;
	struct converter
	{
		int16_t         High;
		int16_t         Low;
	};
	converter* tmp;
	msg_hdr   = ( msg_hdr_t* ) msg;
	msg_magic = XDR_decode_uint32 ( msg_hdr->magic );
	msg_id    = static_cast<MSG_ID> ( XDR_decode_uint32 ( msg_hdr->msg_id ) );
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
	if ( msg_id == MSG_ID::POS_DATA )
	{
		m_pos_data++;
	}
	else
	{
		//////////////////////////////////////////////////
		// handle special packets
		//////////////////////////////////////////////////
		if ( msg_id == MSG_ID::PING )
		{
			// send packet verbatim back to sender
			m_ping_received++;
			msg_hdr->msg_id = XDR_encode_uint32 (
						  static_cast<int> ( MSG_ID::PONG ) );
			m_data_channel->send_to ( msg, bytes, sender_address );
			return;
		}
		else if ( msg_id == MSG_ID::PONG )
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
		if ( msg_id != MSG_ID::POS_DATA )
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
		if ( msg_id == MSG_ID::POS_DATA )
		{
			pos_msg = ( pos_msg_t* ) ( msg + sizeof ( msg_hdr_t ) );
			double x = XDR_decode_double ( pos_msg->position[point3d::X] );
			double y = XDR_decode_double ( pos_msg->position[point3d::Y] );
			double z = XDR_decode_double ( pos_msg->position[point3d::Z] );
			if ( ( x == 0.0 ) || ( y == 0.0 ) || ( z == 0.0 ) )
			{
				// ignore while position is not settled
				m_player_list.unlock();
				return;
			}
			sender->last_pos.set ( x, y, z );
			sender->last_orientation.set (
				XDR_decode_float ( pos_msg->orientation[point3d::X] ),
				XDR_decode_float ( pos_msg->orientation[point3d::Y] ),
				XDR_decode_float ( pos_msg->orientation[point3d::Z] )
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
				LOG ( prio::MEDIUM, sender->name
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
					LOG ( prio::MEDIUM, sender->name
					      << " " << tmp->High
					      << "/" << m_max_radar_range
					      << ": radar range to high, ignoring"
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
			//      send update to inactive relays?
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
		if ( msg_id == MSG_ID::CHAT_MSG )
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
	if ( sender->id ==  list_item::NONE_EXISTANT )
	{
		// player not yet in our list
		// should not happen, but test just in case
		LOG ( prio::URGENT, "## BAD => "
		      << msg_hdr->name << ":" << sender_address.to_string ()
		    );
		return;
	}
	send_to_relays ( msg, bytes, sender );
} // fgms::handle_data ();
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Show Stats
 */
void fgms::show_stats
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
	// output to LOG and cerr channels
	pilot_cnt = local_cnt = 0;
	pilot player; // get LOCAL pilot count
	pilot_cnt = m_player_list.size ();
	for ( int i = 0; i < pilot_cnt; i++ )
	{
		player = m_player_list[i];
		if ( player.id == list_item::NONE_EXISTANT )
		{
			continue;
		}
		if ( player.is_local )
		{
			local_cnt++;
		}
	}
	LOG ( prio::URGENT, "## Pilots: total "
	      << pilot_cnt << ", local " << local_cnt );
	LOG ( prio::URGENT, "## Since: Packets " <<
	      m_packets_received << " BL=" <<
	      m_black_rejected << " INV=" <<
	      m_packets_invalid << " UR=" <<
	      m_unknown_relay << " RD=" <<
	      m_relay_magic << " PD=" <<
	      m_pos_data << " NP=" <<
	      m_unknown_data << " tn=" <<
	      m_queries_received
	    );
	LOG ( prio::URGENT, "## Total: Packets " <<
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
} // fgms::show_stats ()
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
fgms::check_files
()
{
	struct stat buf;
	if ( m_use_exit_file && ( stat ( m_exit_filename.c_str(), &buf ) == 0 ) )
	{
		LOG ( prio::URGENT, "## Got EXIT file : " << m_exit_filename );
		unlink ( m_exit_filename.c_str() );
		if ( stat ( m_exit_filename.c_str(), &buf ) == 0 )
		{
			LOG ( prio::URGENT,
			      "WARNING: Unable to delete exit file "
			      << m_exit_filename << "! Disabled interface..." );
			m_use_exit_file = false;
		}
		return 1;
	}
	else if ( m_use_reset_file && ( stat ( m_reset_filename.c_str(), &buf ) == 0 ) )
	{
		LOG ( prio::URGENT, "## Got RESET file "
		      << m_reset_filename );
		unlink ( m_reset_filename.c_str() );
		if ( stat ( m_reset_filename.c_str(), &buf ) == 0 )
		{
			LOG ( prio::URGENT,
			      "WARNING: Unable to delete reset file "
			      << m_reset_filename << "! Disabled interface..." );
			m_use_reset_file = false;
		}
		m_reinit_data   = true; // init the data port
		m_reinit_query  = true; // init the telnet port
		m_reinit_admin  = true; // init the admin port
		//FIXME: SigHUPHandler ( 0 );
	}
	else if ( m_use_stat_file && ( stat ( m_stats_filename.c_str(), &buf ) == 0 ) )
	{
		LOG ( prio::URGENT, "## Got STAT file " << m_stats_filename );
		unlink ( m_stats_filename.c_str() );
		if ( stat ( m_stats_filename.c_str(), &buf ) == 0 )
		{
			LOG ( prio::URGENT,
			      "WARNING: Unable to delete stat file "
			      << m_stats_filename << "! Disabled interface..." );
			m_use_stat_file = false;
		}
		show_stats();
	}
#ifdef _MSC_VER
	if ( !m_admin_cli && _kbhit() )
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
} // fgms::check_files ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief allow the Admin cli to shut down fgms
 */
void
fgms::want_exit
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
fgms::loop
()
{
	int         bytes;
	char        msg[MAX_PACKET_SIZE];
	time_t      last_tracker_update;
	time_t      current_time;
	pilot_it    player;
	fgmp::netaddr     sender_address ( fgmp::netaddr::IPv6 );
	fgmp::netsocket*  listen_sockets[3 + MAX_TELNETS];
	last_tracker_update = time ( 0 );
	if ( m_listening == false )
	{
		LOG ( prio::EMIT, "fgms::loop() - "
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
			LOG ( prio::EMIT,
			      "# Admin port disabled, "
			      "please set user and password" );
		}
	}
	LOG ( prio::EMIT, "# Main server started!" );
#ifdef _MSC_VER
	LOG ( prio::URGENT,
	      "ESC key to EXIT (after select "
	      << m_player_expires << " sec timeout)." );
#endif
	if ( ! m_run_as_daemon && m_admin_cli )
	{
		// Run admin cli in foreground reading from stdin
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
			LOG ( prio::EMIT, "bummer 1!" );
			return false;;
		}
		if ( m_data_channel == 0 )
		{
			LOG ( prio::EMIT, "bummer 2!" );
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
				update_tracker ( "" , "", "", last_tracker_update, tracker::UPDATE );
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
					LOG ( prio::URGENT, "fgms::loop() - "
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
					LOG ( prio::URGENT, "fgms::loop() - "
					      << strerror ( errno ) );
				}
				continue;
			}
			LOG ( prio::URGENT,
			      "fgms::loop() - new Admin connection from "
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
} // fgms::loop()

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Set listening port for incoming clients
 */
void
fgms::set_data_port
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
} // fgms::SetPort ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set listening port for telnets
 */
void
fgms::set_query_port
(
	int port
)
{
	if ( m_query_port != port )
	{
		m_query_port = port;
		m_reinit_query = true;
	}
} // fgms::set_query_port ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set listening port for admin connections
 */
void
fgms::set_admin_port
(
	int port
)
{
	if ( m_admin_port != port )
	{
		m_admin_port = port;
		m_reinit_admin = true;
	}
} // fgms::set_admin_port ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////

/**
 * @brief  Set the logfile
 */
void
fgms::open_logfile
()
{
	if ( m_reinit_log == false )
	{
		return;
	}
	if ( m_logfile_name == "" )
	{
		return;
	}
	LOG ( prio::EMIT, "# using logfile '" << m_logfile_name << "'" );
	if ( ! logger.open ( m_logfile_name ) )
	{
		LOG ( prio::EMIT, "fgms::Init() - "
		      << "Failed to open log file " << m_logfile_name );
	}
	m_reinit_log = false;
} // fgms::open_logfile ( const std::string &logfile_name )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
fgms::set_exitfile
(
	const std::string& name
)
{
	m_exit_filename = name;
} // fgms::set_exitfile ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
fgms::set_resetfile
(
	const std::string& name
)
{
	m_reset_filename = name;
} // fgms::set_resetfile ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
fgms::set_statsfile
(
	const std::string& name
)
{
	m_stats_filename = name;
} // fgms::set_statsfile ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
fgms::set_msglogfile
(
	const std::string& name
)
{
	m_tracker_logname = name;
} // fgms::set_msglogfile ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
fgms::set_updatetracker
(
	time_t freq
)
{
	m_update_tracker_freq = freq;
} // fgms::set_updatetracker ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
fgms::tracker_log
(
	const std::string& msg,
	const char* src
)
{
	if ( msg.size () == 0 )
	{
		return;
	}
	if ( ! m_tracker_log.is_open() )
	{
		m_tracker_log.open ( m_tracker_logname, std::ios::out|std::ios::app );;
		if ( ! m_tracker_log.is_open() )
		{
			LOG ( prio::EMIT,
			      "ERROR: Failed to OPEN/append "
			      << m_tracker_logname << " file !" )
			return;
		}
	}
	// write timestamp
	time_t timestamp = time ( 0 );
	tm*  tm;
	tm = gmtime ( & timestamp ); // Creates the UTC time string
	m_tracker_log << std::setfill ( '0' )
		      << std::setw ( 4 ) << tm->tm_year+1900 << "-"
		      << std::setw ( 2 ) << tm->tm_mon+1 << "-"
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
fgms::shutdown
()
{
	if ( ! m_is_parent )
	{
		return;
	}
	LOG ( prio::URGENT, "fgms::shutdown () - exiting" );
	show_stats ();   // 20150619:0.11.9: add stats to the LOG on exit
	if ( m_listening == false )
	{
		return;
	}
	if ( m_query_channel != nullptr )
	{
		m_query_channel->close();
		delete m_query_channel;
		m_query_channel = nullptr;
	}
	if ( m_admin_channel != nullptr )
	{
		m_admin_channel->close();
		delete m_admin_channel;
		m_admin_channel = nullptr;
	}
	if ( m_data_channel != nullptr )
	{
		m_data_channel->close();
		delete m_data_channel;
		m_data_channel = nullptr;
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
	m_relay_map.clear ();   // clear(): is a std::map (NOT a fglist)
	m_listening = false;
} // fgms::shutdown ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Updates the remote tracker web server
 */
int
fgms::update_tracker
(
	const std::string& name,
	const std::string& passwd,
	const std::string& model_name,
	const time_t timestamp,
	const int type
)
{
	char            time_str[100];
	pilot   player;
	point3d         playerpos_geod;
	std::string     aircraft;
	std::string     message;
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
	if ( index != std::string::npos )
	{
		aircraft = model_name.substr ( index + 1 );
	}
	else
	{
		aircraft = model_name;
	}
	index = aircraft.find ( ".xml" );
	if ( index != std::string::npos )
	{
		aircraft.erase ( index );
	}
	// Creates the message
	if ( type == tracker::CONNECT )
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
	else if ( type == tracker::DISCONNECT )
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
	size_t j=0; /*message count*/
	for ( size_t i = 0; i < m_player_list.size(); i++ )
	{
		player = m_player_list[i];
		if ( player.id == list_item::NONE_EXISTANT )
		{
			continue;
		}
		point3d hpr = euler_get ( playerpos_geod, player.last_orientation );
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
			message += num_to_str ( playerpos_geod.lat(), 6 ) + " ";
			message += num_to_str ( playerpos_geod.lon(), 6 ) + " ";
			message += num_to_str ( playerpos_geod.alt(), 6 ) + " ";
			message += num_to_str ( hpr.x(), 6 ) + " ";
			message += num_to_str ( hpr.y(), 6 ) + " ";
			message += num_to_str ( hpr.z(), 6 ) + " ";
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
			message += num_to_str ( playerpos_geod.lat(), 6 ) + " ";
			message += num_to_str ( playerpos_geod.lon(), 6 ) + " ";
			message += num_to_str ( playerpos_geod.alt(), 6 ) + " ";
			message += num_to_str ( hpr.x(), 6 ) + " ";
			message += num_to_str ( hpr.y(), 6 ) + " ";
			message += num_to_str ( hpr.z(), 6 ) + " ";
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
fgms::close_tracker
()
{
	if ( m_is_tracked )
	{
		if ( m_tracker != nullptr )
		{
			m_tracker->set_want_exit ();
			pthread_cond_signal ( m_tracker->get_cond_var() );  // wake up the worker
			pthread_join ( m_tracker->get_thread_id(), 0 );
		}
		m_tracker = nullptr;
		m_is_tracked = false;
	}
} // close_tracker ( )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 */
bool
fgms::receiver_wants_data
(
	const pilot_it& sender,
	const pilot& receiver
)
{
	if ( distance ( sender->last_pos, receiver.last_pos ) < receiver.radar_range )
	{
		return true;
	}
	return false;
	// TODO:
	float   out_of_reach;
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
} // fgms::receiver_wants_data ( player, player )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 */
bool
fgms::receiver_wants_chat
(
	const pilot_it&  sender,
	const pilot& receiver
)
{
	float   out_of_reach;
	float   altitude;
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
	altitude = sender->geod_pos.alt();
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
} // fgms::receiver_wants_chat ( player, player )
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
fgms::is_in_range
(
	const list_item& relay,
	const pilot_it& sender,
	MSG_ID msg_id
)
{
	pilot player;
	size_t    cnt;
	cnt = m_player_list.size ();
	for ( size_t i = 0; i < cnt; i++ )
	{
		player = m_player_list[i];
		if ( player.id == list_item::NONE_EXISTANT )
		{
			continue;
		}
		if ( player.address == relay.address )
		{
			if ( msg_id == MSG_ID::CHAT_MSG )
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
} // fgms::is_in_range( relay, player )

//////////////////////////////////////////////////////////////////////

/** Read a config file and set internal variables accordingly
 *
 * @param config_name Path of config file to load
 * @retval int  -- todo--
 */
bool
fgms::process_config
(
	const std::string& config_name
)
{
	fgmp::config    config;
	std::string     val;
	int         e;
	if ( m_have_config )    // we already have a config, so ignore
	{
		return ( true );
	}
	if ( config.read ( config_name ) )
	{
		LOG ( prio::URGENT,
		      "Could not read config file '" << config_name
		      << "' => using defaults" );
		return ( false );
	}
	LOG ( prio::EMIT, "processing " << config_name );
	val = config.get ( "server.name" );
	if ( val != "" )
	{
		m_hostname = val;
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
			LOG ( prio::URGENT,
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
			LOG ( prio::URGENT,
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
			m_admin_cli = true;
		}
		else if ( ( val == "off" ) || ( val == "false" ) )
		{
			m_admin_cli = false;
		}
		else
		{
			LOG ( prio::URGENT,
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
			LOG ( prio::URGENT,
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
			LOG ( prio::URGENT,
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
			LOG ( prio::URGENT,
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
			LOG ( prio::URGENT,
			      "invalid value for Expire: '" << val << "'"
			    );
			exit ( 1 );
		}
	}
	val = config.get ( "server.logfile" );
	if ( val != "" )
	{
		m_logfile_name = val;
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
			LOG ( prio::URGENT,
			      "unknown value for 'server.daemon'!"
			      << " in file " << config_name
			    );
		}
	}
	val = config.get ( "server.tracked" );
	if ( val != "" )
	{
		std::string  server;
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
				LOG ( prio::URGENT,
				      "invalid value for tracking_port: '"
				      << val << "'"
				    );
				exit ( 1 );
			}
			if ( tracked
					&& ( ! add_tracker ( server, port, tracked ) ) ) // set master m_is_tracked
			{
				LOG ( prio::URGENT,
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
	std::string  var;
	std::string  server = "";
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
				LOG ( prio::URGENT,
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
				LOG ( prio::URGENT,
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
} // fgms::process_config ( const std::string& config_name )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print a help screen for command line parameters, see \ref command_line
 */
void
fgms::print_version
()
{
	std::cout << std::endl;
	std::cout << "fgms version " << m_version
		  << ", compiled on " << __DATE__
		  << " at " << __TIME__ << std::endl;
	std::cout << std::endl;
} // fgms::print_version()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print a help screen for command line parameters, see \ref command_line
 */
void
fgms::print_help
()
{
	print_version ();
	std::cout << "syntax: " << m_argv[0] << " options" << std::endl;
	std::cout << "\n"
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
} // fgms::print_help ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** Parse commandline parameters
 *
 * @param  argc
 * @param  argv
 * @retval int 1 on success
 */
void
fgms::parse_params
(
	int   argc,
	char* argv[]
)
{
	int     m;
	int     e;
	m_argc = argc;
	m_argv = argv;
	while ( ( m=getopt ( argc, argv, "a:b:c:dDhl:L:o:p:t:v:" ) ) != -1 )
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
				std::cerr << "invalid value for query port: '"
					  << optarg << "'" << std::endl;
				exit ( 1 );
			}
			break;
		case 'b':
			set_admin_port ( str_to_num<int> ( optarg, e ) );
			if ( e )
			{
				std::cerr << "invalid value for admin port: '"
					  << optarg << "'" << std::endl;
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
				std::cerr << "invalid value for DataPort: '"
					  << optarg << "'" << std::endl;
				exit ( 1 );
			}
			break;
		case 'o':
			m_out_of_reach = str_to_num<int> ( optarg, e );
			if ( e )
			{
				std::cerr << "invalid value for OutOfReach: '"
					  << optarg << "'" << std::endl;
				exit ( 1 );
			}
			break;
		case 'v':
			logger.priority (
				static_cast<prio> ( str_to_num<int> ( optarg, e ) ) );
			if ( e )
			{
				std::cerr << "invalid value for Loglevel: '"
					  << optarg << "'" << std::endl;
				exit ( 1 );
			}
			break;
		case 't':
			m_player_expires = str_to_num<int> ( optarg, e );
			if ( e )
			{
				std::cerr << "invalid value for expire: '"
					  << optarg << "'" << std::endl;
				exit ( 1 );
			}
			break;
		case 'l':
			m_logfile_name = optarg;
			break;
		case 'L':
			m_debug_level = make_prio ( str_to_num<int> ( optarg,e ) );
			if ( e )
			{
				std::cerr << "invalid value for LEVEL "
					  "'" << optarg << "'" << std::endl;
				exit ( 1 );
			}
			break;
		case 'd':
			m_run_as_daemon = false;
			break;
		case 'D':
			m_run_as_daemon = true;
			break;
		default:
			std::cerr << std::endl << std::endl;
			print_help ();
			exit ( 1 );
		} // switch ()
	} // while ()
} // parse_params()

//////////////////////////////////////////////////////////////////////

/**
 * @brief  (re)Read config files
 */
bool
fgms::read_configs
()
{
	std::string path;
#ifndef _MSC_VER
	// try /etc/fgms.conf (or whatever SYSCONFDIR is)
	path = SYSCONFDIR;
	path += "/";
	path += m_config_name;
	if ( process_config ( path ) == true )
	{
		return true;
	}
	// try users home directory
	path = getenv ( "HOME" );
#else
	// windows version
	char* cp = getenv ( "HOME" );
	if ( cp )
	{
		path = cp;
	}
	else
	{
		cp = getenv ( "USERPROFILE" );
		if ( cp )
		{
			path = cp;
		}
	}
#endif
	if ( path != "" )
	{
		path += "/";
		path += m_config_name;
		if ( process_config ( path ) )
		{
			return true;
		}
	}
	// failed, try current directory
	if ( process_config ( m_config_name ) )
	{
		return true;
	}
	LOG ( prio::EMIT,
	      "Could not find a config file => using defaults" );
	return false;
} // fgms::read_configs ()

//////////////////////////////////////////////////////////////////////

/** Check configuration
 *
 * Check some configuration values for witted values
 *
 * @return true         everything OK
 * @return false        some value should be tweaked
 */
bool
fgms::check_config
()
{
	if ( ( m_hostname == "fgms" ) && ( m_relay_list.size () > 0 ) )
	{
		LOG ( prio::EMIT, "If you want to provide a public "
		      << "server, please provide a unique server name!"
		    );
		return false;
	}
	return true;
} // fgms::check_config ()

} // namespace fgmp

//////////////////////////////////////////////////////////////////////

/**
 * @brief MAIN routine
 * @param argc
 * @param argv*[]
 */
int
main
(
	int   argc,
	char* argv[]
)
{
	fgmp::fgms fgms;
	fgms.parse_params ( argc, argv );
	fgms.read_configs ();
	if ( ! fgms.check_config () )
	{
		exit ( 1 );
	}
	if ( ! fgms.init () )
	{
		return 1;
	}
	fgms.loop ();
	fgms.shutdown ();
	return ( 0 );
} // main()
//////////////////////////////////////////////////////////////////////

