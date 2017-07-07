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
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include <simgear/math/SGEuler.hxx>
#include <fglib/fg_util.hxx>
#include <fglib/fg_log.hxx>
#include <fglib/fg_util.hxx>
#include <fglib/fg_config.hxx>
#include <fglib/daemon.hxx>
#include <fglib/fg_version.hxx>
#include <fglib/fg_log.hxx>
#include "fg_cli.hxx"
#include "fgms.hxx"    // includes pthread.h

const FG_VERSION FGMS::m_version ( 1, 0, 0, "-dev3" );

#ifdef _MSC_VER
	#include <conio.h> // for _kbhit(), _getch
#endif

/* FIXME: put in config file */
#ifndef DEF_SERVER_LOG
	#define DEF_SERVER_LOG "fgms.log"
#endif

#ifndef DEF_EXIT_FILE
	#define DEF_EXIT_FILE "fgms_exit"
#endif

#ifndef DEF_RESET_FILE
	#define DEF_RESET_FILE "fgms_reset"
#endif

#ifndef DEF_STAT_FILE
	#define DEF_STAT_FILE "fgms_stat"
#endif

#ifndef DEF_UPDATE_SECS
	#define DEF_UPDATE_SECS 10
#endif

extern void SigHUPHandler ( int SigType );

#ifdef ADD_TRACKER_LOG
	#ifndef DEF_MESSAGE_LOG
		#define DEF_MESSAGE_LOG "fg_message.log"
	#endif
	static char* msg_log = ( char* ) DEF_MESSAGE_LOG;
	static FILE* msg_file = NULL;
#endif // #ifdef ADD_TRACKER_LOG

#if _MSC_VER
	static char* exit_file   = ( char* ) DEF_EXIT_FILE; // "fgms_exit"
	static char* reset_file  = ( char* ) DEF_RESET_FILE; // "fgms_reset"
	static char* stat_file   = ( char* ) DEF_STAT_FILE; // "fgms_stat"
#else // !_MSC_VER
	static char* exit_file   = ( char* ) "/tmp/" DEF_EXIT_FILE;
	static char* reset_file  = ( char* ) "/tmp/" DEF_RESET_FILE;
	static char* stat_file   = ( char* ) "/tmp/" DEF_STAT_FILE;
#endif // _MSC_VER y/n

#ifdef ADD_TRACKER_LOG
static void
write_time_string
(
	FILE* file
)
{
	time_t Timestamp = time ( 0 );
	char TimeStr[100];
	tm*  tm;
	// Creates the UTC time string
	tm = gmtime ( & Timestamp );
	int len = sprintf (
			  TimeStr,
			  "%04d-%02d-%02d %02d:%02d:%02d: ",
			  tm->tm_year+1900,
			  tm->tm_mon+1,
			  tm->tm_mday,
			  tm->tm_hour,
			  tm->tm_min,
			  tm->tm_sec );
	fwrite ( TimeStr,1,len,file );
} // write_time_string()

void
write_msg_log
(
	const char* msg,
	int len,
	char* src
)
{
	if ( msg_file == NULL )
	{
		msg_file = fopen ( msg_log, "ab" );
		if ( !msg_file )
		{
			LOG ( log::ERROR,
			  "ERROR: Failed to OPEN/append "
			  << log file << "!" )
			msg_file = ( FILE* )-1;
		}
	}
	if ( len && msg_file && ( msg_file != ( FILE* )-1 ) )
	{
		write_time_string ( msg_file );
		if ( src && strlen ( src ) )
		{
			fwrite ( src,1,strlen ( src ),msg_file );
		}
		int wtn = ( int ) fwrite ( msg,1,len,msg_file );
		if ( wtn != len )
		{
			fclose ( msg_file );
			msg_file = ( FILE* )-1;
			LOG ( log::ERROR,
			  "ERROR: Failed to WRITE "
			  << wtn << " != " << len
			  << " to " << msg_log );
		}
		else
		{
			if ( msg[len-1] != '\n' )
			{
				fwrite ( ( char* ) "\n",1,1,msg_file );
			}
			fflush ( msg_file ); // push to disk now
		}
	}
} // write_msg_log ()
#endif // #ifdef ADD_TRACKER_LOG

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
	m_player_expires	= 10; // standard expiration period
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
	m_FQDN			= "";
	m_proto_minor_version	= converter[0]; // ->High;
	m_proto_major_version	= converter[1]; // ->Low;
	m_logfile_name		= DEF_SERVER_LOG; // "fgms.log";
	m_relay_map		= ip2relay_t();
	m_is_tracked		= false; // off until config file read
	m_tracker		= 0; // no tracker yet
	m_update_tracker_freq	= DEF_UPDATE_SECS;
	m_have_config		= false;
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
	m_use_exit_file       = ( stat ( exit_file,&buf ) ) ? true : false;
	// caution: this has failed in the past
	m_use_reset_file      = ( stat ( reset_file,&buf ) ) ? true : false;
	m_use_stat_file       = ( stat ( stat_file,&buf ) ) ? true : false;
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
	FGMS* tmp_server = t->Instance;
	tmp_server->handle_query ( t->Fd );
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
	FGMS* tmp_server = t->Instance;
	pthread_detach ( pthread_self() );
	tmp_server->handle_admin ( t->Fd );
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
		LOG ( log::ERROR, "FGMS::init() - "
		  << "failed to bind to " << m_data_port );
		LOG ( log::ERROR, "already in use?" );
		LOG ( log::ERROR, e.what() );
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
		LOG ( log::ERROR, "FGMS::init() - "
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
		LOG ( log::ERROR, "FGMS::init() - "
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
	LOG ( log::ERROR, "# FlightGear Multiplayer Server v"
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
	LOG ( log::ERROR, "# This is " << m_server_name << " (" << m_FQDN << ")" );
	LOG ( log::ERROR, "# using protocol version v"
	  << m_proto_major_version << "." << m_proto_minor_version
	  << " (LazyRelay enabled)" );
	LOG ( log::ERROR, "# listening to port " << m_data_port );
	if ( m_query_channel )
	{
		LOG ( log::ERROR, "# telnet port " << m_query_port );
	}
	else
	{
		LOG ( log::ERROR, "# telnet port DISABLED" );
	}
	if ( m_admin_channel )
	{
		LOG ( log::ERROR, "# admin port " << m_admin_port );
	}
	else
	{
		LOG ( log::ERROR, "# admin port DISABLED" );
	}
	LOG ( log::ERROR, "# using logfile " << m_logfile_name );
	if ( m_bind_addr != "" )
	{
		LOG ( log::ERROR, "# listening on " << m_bind_addr );
	}
	if ( m_me_is_hub )
	{
		LOG ( log::ERROR, "# I am a HUB Server" );
	}
	if ( ( m_is_tracked ) && ( m_tracker != 0 ) )
	{
		pthread_t th;
		pthread_create ( &th, NULL, &detach_tracker, m_tracker );
		LOG ( log::ERROR, "# tracked to "
		  << m_tracker->GetTrackerServer ()
		  << ":" << m_tracker->GetTrackerPort ()
		  << ", using a thread."
		);
	}
	else
	{
		LOG ( log::ERROR, "# tracking is disabled." );
	}
	size_t Count;
	ListElement Entry ( "" );
	//////////////////////////////////////////////////
	// print list of all relays
	//////////////////////////////////////////////////
	Count = m_relay_list.Size();
	LOG ( log::ERROR, "# I have " << Count << " relays" );
	for ( size_t i = 0; i < Count; i++ )
	{
		Entry = m_relay_list[i];
		if ( Entry.ID == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		LOG ( log::ERROR, "# relay " << Entry.Name
			     << ":" << Entry.Address.port()
			     << " (" << Entry.Address << ")" );
	}
	//////////////////////////////////////////////////
	// print list of all crossfeeds
	//////////////////////////////////////////////////
	Count = m_cross_list.Size();
	LOG ( log::ERROR, "# I have " << Count << " crossfeeds" );
	for ( size_t i = 0; i < Count; i++ )
	{
		Entry = m_cross_list[i];
		if ( Entry.ID == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		LOG ( log::ERROR, "# crossfeed " << Entry.Name
		  << ":" << Entry.Address.port()
		);
	}
	LOG ( log::ERROR, "# I have " << m_black_list.Size()
	  << " blacklisted IPs"
	);
	if ( m_use_exit_file && m_use_stat_file )
	{	// only show this IFF both are enabled
		LOG ( log::ERROR, "# Files: exit=[" << exit_file
		  << "] stat=[" << stat_file << "]"
		);
	}
	m_listening = true;
	if ( m_run_as_daemon )
	{
		daemonize ();
	}
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
	LOG ( log::URGENT, "# caught SIGHUP, doing reinit!" );
	// release all locks
	m_player_list.Unlock ();
	m_relay_list.Unlock ();
	m_cross_list.Unlock ();
	m_white_list.Unlock ();
	m_black_list.Unlock ();
	// and clear all but the player list
	m_relay_list.Clear ();
	m_white_list.Clear ();
	m_black_list.Clear ();
	m_cross_list.Clear ();
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
	int Fd
)
{
	FG_CLI*	MyCLI;
	errno = 0;
	MyCLI = new FG_CLI ( this, Fd );
	MyCLI->loop ();
	if ( Fd == 0 )
	{
		// reading from stdin
		want_exit();
	}
	delete MyCLI;
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
	std::string	Message;
	FG_Player	CurrentPlayer;
	unsigned int	it;

	m_clients.Lock ();
	m_clients.clear ();
	//////////////////////////////////////////////////
	//
	//      create the output message
	//      header
	//
	//////////////////////////////////////////////////
	Message  = "# This is " + m_server_name;
	Message += "\n";
	m_clients.push_back ( Message );
	Message  = "# FlightGear Multiplayer Server v";
	Message += m_version.str();
	Message += "\n";
	m_clients.push_back ( Message );
	Message  = "# using protocol version v";
	Message += NumToStr ( m_proto_major_version, 0 );
	Message += "." + NumToStr ( m_proto_minor_version, 0 );
	Message += " (LazyRelay enabled)";
	Message += "\n";
	m_clients.push_back ( Message );
	if ( m_is_tracked )
	{
		Message  = "# This server is tracked: ";
		Message += m_tracker->GetTrackerServer();
		Message += "\n";
		m_clients.push_back ( Message );
	}
	Message  = "# "+ NumToStr ( m_player_list.Size(), 0 );
	Message += " pilot(s) online\n";
	m_clients.push_back ( Message );
	//////////////////////////////////////////////////
	//
	//      create list of players
	//
	//////////////////////////////////////////////////
	it = 0;
	for ( ;; )
	{
		if ( it < m_player_list.Size() )
		{
			CurrentPlayer = m_player_list[it];
			it++;
		}
		else
		{
			break;
		}
		if ( CurrentPlayer.Name.compare ( 0, 3, "obs", 3 ) == 0 )
		{
			continue;
		}
		Message = CurrentPlayer.Name + "@";
		if ( CurrentPlayer.IsLocal )
		{
			Message += "LOCAL: ";
		}
		else
		{
			ip2relay_it Relay = m_relay_map.find
			  ( CurrentPlayer.Address );
			if ( Relay != m_relay_map.end() )
			{
				Message += Relay->second + ": ";
			}
			else
			{
				Message += CurrentPlayer.Origin + ": ";
			}
		}
		if ( CurrentPlayer.Error != "" )
		{
			Message += CurrentPlayer.Error + " ";
		}
		Message += NumToStr ( CurrentPlayer.LastPos[X], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.LastPos[Y], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.LastPos[Z], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.GeodPos[Lat], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.GeodPos[Lon], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.GeodPos[Alt], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.LastOrientation[X], 6 )+" ";
		Message += NumToStr ( CurrentPlayer.LastOrientation[Y], 6 )+" ";
		Message += NumToStr ( CurrentPlayer.LastOrientation[Z], 6 )+" ";
		Message += CurrentPlayer.ModelName;
		Message += "\n";
		m_clients.push_back ( Message );
	}
	m_clients.Unlock ();
} // FGMS::mk_client_list()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle a telnet session. if a telnet connection is opened, this
 *        method outputs a list  of all known clients.
 * @param Fd -- docs todo --
 */
void*
FGMS::handle_query
(
	int Fd
)
{
	StrIt		Line;
	fgmp::netsocket	NewTelnet;
	NewTelnet.handle ( Fd );
	errno = 0;
	m_clients.Lock ();
	for ( Line = m_clients.begin(); Line != m_clients.end(); Line++ )
	{
		if ( NewTelnet.send ( *Line ) < 0 )
		{
			if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
			{
				LOG ( log::URGENT, "FGMS::handle_query() - "
				  << strerror ( errno ) );
			}
			NewTelnet.close ();
			m_clients.Unlock ();
			return ( 0 );
		}
	}
	NewTelnet.close ();
	m_clients.Unlock ();
	return ( 0 );
} // FGMS::handle_query ()

//////////////////////////////////////////////////////////////////////
/**
 * @brief  If we receive bad data from a client, we add the client to
 *         the internal list anyway, but mark them as bad. But first
 *         we look if it isn't already there.
 *         Send an error message to the bad client.
 * @param Sender
 * @param ErrorMsg
 * @param IsLocal
 */
void
FGMS::add_bad_client
(
	const fgmp::netaddr& Sender,
	string&	ErrorMsg,
	bool	IsLocal,
	int	Bytes
)
{
	string		Message;
	FG_Player	NewPlayer;
	PlayerIt	CurrentPlayer;
	//////////////////////////////////////////////////
	//      see, if we already know the client
	//////////////////////////////////////////////////
	m_player_list.Lock ();
	CurrentPlayer = m_player_list.Find ( Sender, true );
	if ( CurrentPlayer != m_player_list.End () )
	{
		CurrentPlayer->UpdateRcvd ( Bytes );
		m_player_list.UpdateRcvd ( Bytes );
		m_player_list.Unlock();
		return;
	}
	//////////////////////////////////////////////////
	//      new client, add to the list
	//////////////////////////////////////////////////
	if ( IsLocal )
	{
		m_local_clients++;
	}
	else
	{
		m_remote_clients++;
	}
	NewPlayer.Name      = "* Bad Client *";
	NewPlayer.ModelName     = "* unknown *";
	NewPlayer.Origin        = Sender.to_string ();
	NewPlayer.Address       = Sender;
	NewPlayer.IsLocal       = IsLocal;
	NewPlayer.HasErrors     = true;
	NewPlayer.Error         = ErrorMsg;
	NewPlayer.UpdateRcvd ( Bytes );
	LOG ( log::MEDIUM, "FGMS::add_bad_client() - " << ErrorMsg );
	m_player_list.Add ( NewPlayer, m_player_expires );
	m_player_list.UpdateRcvd ( Bytes );
	m_player_list.Unlock();
} // FGMS::add_bad_client ()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new client to internal list
 * @param Sender
 * @param Msg
 */
void
FGMS::add_client
(
	const fgmp::netaddr& Sender,
	char* Msg
)
{
	uint32_t	MsgMagic;
	uint32_t	ProtoVersion;
	string		Message;
	string		Origin;
	T_MsgHdr*	MsgHdr;
	T_PositionMsg*	PosMsg;
	FG_Player	NewPlayer;
	bool		IsLocal;
	int16_t*	converter;

	MsgHdr		= ( T_MsgHdr* ) Msg;
	PosMsg		= ( T_PositionMsg* ) ( Msg + sizeof ( T_MsgHdr ) );
	MsgMagic	= XDR_decode_uint32 ( MsgHdr->Magic );
	IsLocal		= true;
	if ( MsgMagic == RELAY_MAGIC ) // not a local client
	{
		IsLocal = false;
	}
	ProtoVersion	= XDR_decode_uint32 ( MsgHdr->Version );
	converter = ( int16_t* ) & ProtoVersion;
	NewPlayer.Name	    = MsgHdr->Name;
	NewPlayer.Passwd    = "test"; //MsgHdr->Passwd;
	NewPlayer.ModelName = "* unknown *";
	NewPlayer.Origin    = Sender.to_string ();
	NewPlayer.Address   = Sender;
	NewPlayer.IsLocal   = IsLocal;
	NewPlayer.ProtoMajor	= converter[0];
	NewPlayer.ProtoMinor	= converter[1];
	NewPlayer.LastPos.Set (
		XDR_decode_double ( PosMsg->position[X] ),
		XDR_decode_double ( PosMsg->position[Y] ),
		XDR_decode_double ( PosMsg->position[Z] )
	);
	NewPlayer.LastOrientation.Set (
		XDR_decode_float ( PosMsg->orientation[X] ),
		XDR_decode_float ( PosMsg->orientation[Y] ),
		XDR_decode_float ( PosMsg->orientation[Z] )
	);
	sgCartToGeod ( NewPlayer.LastPos, NewPlayer.GeodPos );
	NewPlayer.ModelName = PosMsg->Model;
	if ( ( NewPlayer.ModelName == "OpenRadar" )
	||   ( NewPlayer.ModelName.find ( "ATC" ) != std::string::npos ) )
	{
		// client is an ATC
		if ( str_ends_with ( NewPlayer.Name, "_DL" ) )
		{
			NewPlayer.IsATC = FG_Player::ATC_DL;
		}
		else if ( str_ends_with ( NewPlayer.Name, "_GN" ) )
		{
			NewPlayer.IsATC = FG_Player::ATC_GN;
		}
		else if ( str_ends_with ( NewPlayer.Name, "_TW" ) )
		{
			NewPlayer.IsATC = FG_Player::ATC_TW;
		}
		else if ( str_ends_with ( NewPlayer.Name, "_AP" ) )
		{
			NewPlayer.IsATC = FG_Player::ATC_AP;
		}
		else if ( str_ends_with ( NewPlayer.Name, "_DE" ) )
		{
			NewPlayer.IsATC = FG_Player::ATC_DE;
		}
		else if ( str_ends_with ( NewPlayer.Name, "_CT" ) )
		{
			NewPlayer.IsATC = FG_Player::ATC_CT;
		}
		else
		{
			NewPlayer.IsATC = FG_Player::ATC;
		}
	}
	MsgHdr->RadarRange = XDR_decode_uint32 ( MsgHdr->RadarRange );
	converter = (int16_t *) & MsgHdr->RadarRange;
	if ( ( converter[0] != 0 ) || ( converter[1] == 0 ) )
	{
		// client comes from an old server which overwrites
		// the radar range or the client does not set
		// RadarRange
		NewPlayer.RadarRange = m_out_of_reach;
	}
	else
	{
		if ( converter[0] <= m_max_radar_range )
		{
			NewPlayer.RadarRange = converter[0];
		}
	}
	if ( NewPlayer.RadarRange == 0 )
	{
		NewPlayer.RadarRange = m_out_of_reach;
	}
	m_player_list.Add ( NewPlayer, m_player_expires );
	size_t NumClients = m_player_list.Size ();
	if ( NumClients > m_num_max_clients )
	{
		m_num_max_clients = NumClients;
	}
	Origin  = NewPlayer.Origin;
	if ( IsLocal )
	{
		Message = "New LOCAL Client: ";
		m_local_clients++;
		update_tracker ( NewPlayer.Name, NewPlayer.Passwd,
		  NewPlayer.ModelName, NewPlayer.LastSeen, CONNECT );
	}
	else
	{
		m_remote_clients++;
		Message = "New REMOTE Client: ";
		ip2relay_it Relay = m_relay_map.find ( NewPlayer.Address );
		if ( Relay != m_relay_map.end() )
		{
			Origin = Relay->second;
		}
#ifdef TRACK_ALL
		update_tracker ( NewPlayer.Name, NewPlayer.Passwd,
		  NewPlayer.ModelName, NewPlayer.LastSeen, CONNECT );
#endif
	}
	LOG ( log::MEDIUM, Message
	  << NewPlayer.Name << "@"
	  << Origin << ":" << Sender.port()
	  << " (" << NewPlayer.ModelName << ")"
	  << " current clients: "
	  << NumClients << " max: " << m_num_max_clients
	);
} // FGMS::add_client()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new relay server into internal list
 * @param Server
 * @param Port
 */
void
FGMS::add_relay
(
	const string& Relay,
	int Port
)
{
	ListElement  B ( Relay );
	B.Address.assign ( Relay, Port );
	if ( ! B.Address.is_valid () )
	{
		LOG ( log::URGENT,
		  "could not resolve '" << Relay << "'" );
		return;
	}
	m_relay_list.Lock ();
	ItList CurrentEntry = m_relay_list.Find ( B.Address, true );
	m_relay_list.Unlock ();
	if ( CurrentEntry == m_relay_list.End() )
	{
		m_relay_list.Add ( B, 0 );
		string S;
		if ( B.Address.to_string () == Relay )
		{
			S = Relay;
		}
		else
		{
			unsigned I;
			I = Relay.find ( "." );
			if ( I != string::npos )
			{
				S = Relay.substr ( 0, I );
			}
			else
			{
				S = Relay;
			}
		}
		m_relay_map[B.Address] = S;
	}
} // FGMS::add_relay()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new crossfeed server into internal list
 * @param Server char with server
 * @param Port int with port number
 */
void
FGMS::add_crossfeed
(
	const string& Server,
	int Port
)
{
	string s = Server;
#ifdef _MSC_VER
	if ( s == "localhost" )
	{
		s = "127.0.0.1";
	}
#endif // _MSC_VER
	ListElement B ( s );
	B.Address.assign ( ( char* ) s.c_str(), Port );
	m_cross_list.Lock ();
	ItList CurrentEntry = m_cross_list.Find ( B.Address, true );
	m_cross_list.Unlock ();
	if ( CurrentEntry == m_cross_list.End() )
	{
		m_cross_list.Add ( B, 0 );
	}
} // FGMS::add_crossfeed()

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
	const string& Server,
	int Port,
	bool IsTracked
)
{
	close_tracker();
	m_is_tracked = IsTracked;
	m_tracker = new FG_TRACKER ( Port, Server, m_server_name,
	  m_FQDN, m_version.str() );
	return true;
} // FGMS::add_tracker()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Add an IP to the whitelist
 * @param FourDottedIP IP to add to whitelist
 */
void
FGMS::add_whitelist
(
	const string& DottedIP
)
{
	ListElement B ( DottedIP );
	B.Address.assign ( DottedIP.c_str(), 0 );
	m_white_list.Lock ();
	ItList CurrentEntry = m_white_list.Find ( B.Address );
	m_white_list.Unlock ();
	if ( CurrentEntry == m_white_list.End() )
	{
		m_white_list.Add ( B, 0 );
	}
} // FGMS::add_whitelist()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Add an IP to the blacklist
 * @param FourDottedIP IP to add to blacklist
 */
void
FGMS::add_blacklist
(
	const string& DottedIP,
	const string& Reason,
	time_t Timeout
)
{
	ListElement B ( Reason );
	B.Address.assign ( DottedIP.c_str(), 0 );
	m_black_list.Lock ();
	ItList CurrentEntry = m_black_list.Find ( B.Address );
	m_black_list.Unlock ();
	if ( CurrentEntry == m_black_list.End() )
	{
		// FIXME: every list has its own standard TTL
		m_black_list.Add ( B, Timeout );
	}
} // FGMS::add_blacklist()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check if the sender is a known relay
 * @param SenderAddress
 * @retval bool true if known relay
 */
bool
FGMS::is_known_relay
(
	const fgmp::netaddr& SenderAddress,
	size_t Bytes
)
{
	ItList CurrentEntry;
	m_white_list.Lock ();
	CurrentEntry = m_white_list.Find ( SenderAddress );
	if ( CurrentEntry != m_white_list.End() )
	{
		m_white_list.UpdateRcvd ( CurrentEntry, Bytes );
		m_white_list.Unlock ();
		return true;
	}
	m_white_list.Unlock ();
	m_relay_list.Lock ();
	CurrentEntry = m_relay_list.Find ( SenderAddress );
	if ( CurrentEntry != m_relay_list.End() )
	{
		m_relay_list.UpdateRcvd ( CurrentEntry, Bytes );
		m_relay_list.Unlock ();
		return true;
	}
	m_relay_list.Unlock ();
	string ErrorMsg;
	ErrorMsg  = SenderAddress.to_string ();
	ErrorMsg += " is not a valid relay!";
	add_blacklist ( SenderAddress.to_string (), "not a valid relay", 0 );
	LOG ( log::URGENT, "UNKNOWN RELAY: " << ErrorMsg );
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
	int		Bytes,
	T_MsgHdr*	MsgHdr,
	const fgmp::netaddr& SenderAddress
)
{
	uint32_t        MsgMagic;
	uint32_t        MsgLen;
	uint32_t        MsgId;
	uint32_t        ProtoVer;
	string          ErrorMsg;
	string          Origin;
	typedef struct
	{
		int16_t		High;
		int16_t		Low;
	} converter;
	converter*    tmp;
	Origin   = SenderAddress.to_string ();
	MsgMagic = XDR_decode_uint32 ( MsgHdr->Magic );
	MsgId    = XDR_decode_uint32 ( MsgHdr->MsgId );
	MsgLen   = XDR_decode_uint32 ( MsgHdr->MsgLen );
	if ( Bytes < ( int ) sizeof ( MsgHdr ) )
	{
		ErrorMsg  = SenderAddress.to_string ();
		ErrorMsg += " packet size is too small!";
		add_bad_client ( SenderAddress, ErrorMsg, true, Bytes );
		return ( false );
	}
	if ( ( MsgMagic != MSG_MAGIC ) && ( MsgMagic != RELAY_MAGIC ) )
	{
		char m[5];
		memcpy ( m, ( char* ) &MsgMagic, 4 );
		m[4] = 0;
		ErrorMsg  = Origin;
		ErrorMsg += " BAD magic number: ";
		ErrorMsg += m;
		add_bad_client ( SenderAddress, ErrorMsg, true, Bytes );
		return ( false );
	}
	ProtoVer = XDR_decode_uint32 ( MsgHdr->Version );
	tmp = ( converter* ) & ProtoVer;
	if ( tmp->High != m_proto_major_version )
	{
		MsgHdr->Version = XDR_decode_uint32 ( MsgHdr->Version );
		ErrorMsg  = Origin;
		ErrorMsg += " BAD protocol version! Should be ";
		tmp = ( converter* ) ( & PROTO_VER );
		ErrorMsg += NumToStr ( tmp->High, 0 );
		ErrorMsg += "." + NumToStr ( tmp->Low, 0 );
		ErrorMsg += " but is ";
		tmp = ( converter* ) ( & MsgHdr->Version );
		ErrorMsg += NumToStr ( tmp->Low, 0 );
		ErrorMsg += "." + NumToStr ( tmp->High, 0 );
		add_bad_client ( SenderAddress, ErrorMsg, true, Bytes );
		return ( false );
	}
	if ( MsgId == FGFS::POS_DATA )
	{
		if ( MsgLen < sizeof ( T_MsgHdr ) + sizeof ( T_PositionMsg ) )
		{
			ErrorMsg  = Origin;
			ErrorMsg += " Client sends insufficient position ";
			ErrorMsg += "data, should be ";
			ErrorMsg += NumToStr (
			  sizeof ( T_MsgHdr ) +sizeof ( T_PositionMsg ) );
			ErrorMsg += " is: " + NumToStr ( MsgHdr->MsgLen );
			add_bad_client ( SenderAddress, ErrorMsg, true, Bytes );
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
	char* Msg,
	int Bytes,
	const fgmp::netaddr& SenderAddress 
)
{
	T_MsgHdr*       MsgHdr;
	uint32_t        MsgMagic;
	int             sent;
	ItList		Entry;
	MsgHdr		= ( T_MsgHdr* ) Msg;
	MsgMagic	= MsgHdr->Magic;
	MsgHdr->Magic	= XDR_encode_uint32 ( RELAY_MAGIC );
	m_cross_list.Lock();
	for ( Entry = m_cross_list.Begin(); Entry != m_cross_list.End(); Entry++ )
	{
		sent = m_data_channel->send_to ( Msg, Bytes, Entry->Address );
		m_cross_list.UpdateSent ( Entry, sent );
	}
	m_cross_list.Unlock();
	MsgHdr->Magic = MsgMagic;  // restore the magic value
} // FGMS::send_toCrossfeed ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Send message to all relay servers
 */
void
FGMS::send_to_relays
(
	char* Msg,
	int Bytes,
	PlayerIt& SendingPlayer
)
{
	T_MsgHdr*       MsgHdr;
	uint32_t        MsgMagic;
	uint32_t        MsgId;
	unsigned int    PktsForwarded = 0;
	ItList		CurrentRelay;
	if ( ( ! SendingPlayer->IsLocal ) && ( ! m_me_is_hub ) )
	{
		return;
	}
	MsgHdr    = ( T_MsgHdr* ) Msg;
	MsgMagic  = XDR_decode_uint32 ( MsgHdr->Magic );
	MsgId     = XDR_decode_uint32 ( MsgHdr->MsgId );
	MsgHdr->Magic = XDR_encode_uint32 ( RELAY_MAGIC );
	m_relay_list.Lock ();
	CurrentRelay = m_relay_list.Begin();
	while ( CurrentRelay != m_relay_list.End() )
	{
		if ( CurrentRelay->Address != SendingPlayer->Address )
		{
			if ( SendingPlayer->DoUpdate || is_in_range ( *CurrentRelay, SendingPlayer, MsgId ) )
			{
				m_data_channel->send_to ( Msg, Bytes, CurrentRelay->Address );
				m_relay_list.UpdateSent ( CurrentRelay, Bytes );
				PktsForwarded++;
			}
		}
		CurrentRelay++;
	}
	m_relay_list.Unlock ();
	MsgHdr->Magic = XDR_encode_uint32 ( MsgMagic ); // restore the magic value
} // FGMS::send_toRelays ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//	Remove Player from list
void
FGMS::drop_client
(
	PlayerIt& CurrentPlayer
)
{
	string Origin;
#ifdef TRACK_ALL
	update_tracker (
		CurrentPlayer->Name,
		CurrentPlayer->Passwd,
		CurrentPlayer->ModelName,
		CurrentPlayer->LastSeen,
		DISCONNECT );
#else
	if ( ( CurrentPlayer->IsLocal ) && ( CurrentPlayer->HasErrors == false ) )
	{
		update_tracker (
			CurrentPlayer->Name,
			CurrentPlayer->Passwd,
			CurrentPlayer->ModelName,
			CurrentPlayer->LastSeen,
			DISCONNECT );
	}
#endif
	if ( CurrentPlayer->IsLocal )
	{
		m_local_clients--;
	}
	else
	{
		m_remote_clients--;
	}
	ip2relay_it Relay = m_relay_map.find ( CurrentPlayer->Address );
	if ( Relay != m_relay_map.end() )
	{
		Origin = Relay->second;
	}
	else
	{
		Origin = "LOCAL";
	}
	LOG ( log::MEDIUM, "Dropping pilot "
	  << CurrentPlayer->Name << "@" << Origin
	  << " after " << time ( 0 )-CurrentPlayer->JoinTime << " seconds. "
	  << "Current clients: "
	  << m_player_list.Size()-1 << " max: " << m_num_max_clients
	);
	CurrentPlayer = m_player_list.Delete ( CurrentPlayer );
} // FGMS::drop_client()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle client connections
 * @param Msg
 * @param Bytes
 * @param SenderAddress
 */
void
FGMS::handle_data
(
	char* Msg,
	int Bytes,
	const fgmp::netaddr& SenderAddress
)
{
	T_MsgHdr*       MsgHdr;
	T_PositionMsg*  PosMsg;
	uint32_t        MsgId;
	uint32_t        MsgMagic;
	PlayerIt	SendingPlayer;
	PlayerIt	CurrentPlayer;
	ItList		CurrentEntry;
	time_t          Now;
	unsigned int    PktsForwarded = 0;
	typedef struct
	{
		int16_t         High;
		int16_t         Low;
	} converter;
	converter* tmp;
	MsgHdr    = ( T_MsgHdr* ) Msg;
	MsgMagic  = XDR_decode_uint32 ( MsgHdr->Magic );
	MsgId     = XDR_decode_uint32 ( MsgHdr->MsgId );
	Now       = time ( 0 );
	//////////////////////////////////////////////////
	//
	//  First of all, send packet to all
	//  crossfeed servers.
	//
	//////////////////////////////////////////////////
	send_to_cross ( Msg, Bytes, SenderAddress );
	//////////////////////////////////////////////////
	//
	//  Now do the local processing
	//
	//////////////////////////////////////////////////
	m_black_list.Lock ();
	CurrentEntry = m_black_list.Find ( SenderAddress );
	if ( CurrentEntry != m_black_list.End() )
	{
		m_black_list.UpdateRcvd ( CurrentEntry, Bytes );
		m_black_rejected++;
		m_black_list.Unlock ();
		return;
	}
	m_black_list.Unlock ();
	if ( ! packet_is_valid ( Bytes, MsgHdr, SenderAddress ) )
	{
		m_packets_invalid++;
		return;
	}
	if ( MsgMagic == RELAY_MAGIC ) // not a local client
	{
		if ( ! is_known_relay ( SenderAddress, Bytes ) )
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
	if ( MsgId == FGFS::POS_DATA )
	{
		m_pos_data++;
	}
	else
	{
		//////////////////////////////////////////////////
		// handle special packets
		//////////////////////////////////////////////////
		if ( MsgId == FGFS::PING )
		{
			// send packet verbatim back to sender
			m_ping_received++;
			MsgHdr->MsgId = XDR_encode_uint32 ( FGFS::PONG );
			m_data_channel->send_to ( Msg, Bytes, SenderAddress );
			return;
		}
		else if ( MsgId == FGFS::PONG )
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
	//    Add Client to list if its not known
	//
	//////////////////////////////////////////////////
	m_player_list.Lock();
	SendingPlayer = m_player_list.FindByName ( MsgHdr->Name );
	if ( SendingPlayer == m_player_list.End () )
	{
		// unknown, add to the list
		if ( MsgId != FGFS::POS_DATA )
		{
			// ignore clients until we have a valid position
			m_player_list.Unlock();
			return;
		}
		add_client ( SenderAddress, Msg );
		SendingPlayer = m_player_list.Last();
	}
	else
	{
		//////////////////////////////////////////////////
		//
		// found the client, update internal values
		//
		//////////////////////////////////////////////////
		if ( SendingPlayer->Address != SenderAddress )
		{
			m_player_list.Unlock();
			return;
		}
		m_player_list.UpdateRcvd ( SendingPlayer, Bytes );
		if ( MsgId == FGFS::POS_DATA )
		{
			PosMsg = ( T_PositionMsg* ) ( Msg + sizeof ( T_MsgHdr ) );
			double x = XDR_decode_double ( PosMsg->position[X] );
			double y = XDR_decode_double ( PosMsg->position[Y] );
			double z = XDR_decode_double ( PosMsg->position[Z] );
			if ( ( x == 0.0 ) || ( y == 0.0 ) || ( z == 0.0 ) )
			{
				// ignore while position is not settled
				m_player_list.Unlock();
				return;
			}
			SendingPlayer->LastPos.Set ( x, y, z );
			SendingPlayer->LastOrientation.Set (
				XDR_decode_float ( PosMsg->orientation[X] ),
				XDR_decode_float ( PosMsg->orientation[Y] ),
				XDR_decode_float ( PosMsg->orientation[Z] )
			);
			sgCartToGeod ( SendingPlayer->LastPos, SendingPlayer->GeodPos );
		}
		MsgHdr->RadarRange = XDR_decode_uint32 ( MsgHdr->RadarRange );
		tmp =  ( converter* ) & MsgHdr->RadarRange;
		if ( ( tmp->High != 0 ) && ( tmp->Low == 0 ) )
		{
			// client is 'new' and transmit radar range
			if ( tmp->High != SendingPlayer->RadarRange )
			{
				LOG ( log::MEDIUM, SendingPlayer->Name
				  << " changes radar range from "
				  << SendingPlayer->RadarRange
				  << " to "
				  << tmp->High
				);
				if ( tmp->High <= m_max_radar_range )
				{
					SendingPlayer->RadarRange = tmp->High;
				}
				else
				{
					LOG ( log::MEDIUM, SendingPlayer->Name
					  << " radar range to high, ignoring"
					);
				}
			}
		}
	}
	m_player_list.Unlock();
	//////////////////////////////////////////
	//
	//      send the packet to all clients.
	//      since we are walking through the list,
	//      we look for the sending client, too. if it
	//      is not already there, add it to the list
	//
	//////////////////////////////////////////////////
	MsgHdr->Magic = XDR_encode_uint32 ( MSG_MAGIC );
	CurrentPlayer = m_player_list.Begin();
	while ( CurrentPlayer != m_player_list.End() )
	{
		//////////////////////////////////////////////////
		//
		//      ignore clients with errors
		//
		//////////////////////////////////////////////////
		if ( CurrentPlayer->HasErrors )
		{
			if ( ( Now - CurrentPlayer->LastSeen ) > CurrentPlayer->Timeout )
			{
				drop_client ( CurrentPlayer );
			}
			else
			{
				CurrentPlayer++;
			}
			continue;
		}
		//////////////////////////////////////////////////
		//        Sender == CurrentPlayer?
		//////////////////////////////////////////////////
		//  FIXME: if Sender is a Relay,
		//         CurrentPlayer->Address will be
		//         address of Relay and not the client's!
		//         so use a clientID instead
		if ( CurrentPlayer->Name == MsgHdr->Name )
		{
			//////////////////////////////////////////////////
			//	send update to inactive relays?
			//////////////////////////////////////////////////
			CurrentPlayer->DoUpdate = ( ( Now - CurrentPlayer->LastRelayedToInactive ) > UPDATE_INACTIVE_PERIOD );
			if ( CurrentPlayer->DoUpdate )
			{
				CurrentPlayer->LastRelayedToInactive = Now;
			}
			CurrentPlayer++;
			continue; // don't send packet back to sender
		}
		//////////////////////////////////////////////////
		// 'hidden' feature of fgms. If a callsign starts
		// with 'obs', do not send the packet to other
		// clients. Useful for test connections.
		//////////////////////////////////////////////////
		if ( CurrentPlayer->Name.compare ( 0, 3, "obs", 3 ) == 0 )
		{
			CurrentPlayer++;
			continue;
		}
		//////////////////////////////////////////////////
		//      do not send packet to clients which
		//      are out of reach.
		//////////////////////////////////////////////////
		if ( MsgId == CHAT_MSG_ID )
		{
			// apply 'radio' rules
			// if ( not receiver_wants_chat( SendingPlayer, *CurrentPlayer ) )
			if ( ! receiver_wants_data ( SendingPlayer, *CurrentPlayer ) )
			{
				CurrentPlayer++;
				continue;
			}
		}
		else
		{
			// apply 'visibility' rules, for now we apply 'radio' rules
			if ( ! receiver_wants_data ( SendingPlayer, *CurrentPlayer ) )
			{
				CurrentPlayer++;
				continue;
			}
		}
		//////////////////////////////////////////////////
		//
		//  only send packet to local clients
		//
		//////////////////////////////////////////////////
		if ( CurrentPlayer->IsLocal )
		{
			m_data_channel->send_to ( Msg, Bytes, CurrentPlayer->Address );
			m_player_list.UpdateSent ( CurrentPlayer, Bytes );
			PktsForwarded++;
		}
		CurrentPlayer++;
	}
	if ( SendingPlayer->ID ==  ListElement::NONE_EXISTANT )
	{
		// player not yet in our list
		// should not happen, but test just in case
		LOG ( log::URGENT, "## BAD => "
		  << MsgHdr->Name << ":" << SenderAddress.to_string ()
		);
		return;
	}
	send_to_relays ( Msg, Bytes, SendingPlayer );
} // FGMS::handle_data ();
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
	m_t_pos_data    += m_pos_data;
	m_t_unknown_data     += m_unknown_data;
	m_t_telnet_received  += m_queries_received;
	m_t_cross_failed += m_cross_failed;
	m_t_cross_sent   += m_cross_sent;
	// output to LOG and cerr channels
	pilot_cnt = local_cnt = 0;
	FG_Player CurrentPlayer; // get LOCAL pilot count
	pilot_cnt = m_player_list.Size ();
	for ( int i = 0; i < pilot_cnt; i++ )
	{
		CurrentPlayer = m_player_list[i];
		if ( CurrentPlayer.ID == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		if ( CurrentPlayer.IsLocal )
		{
			local_cnt++;
		}
	}
	LOG ( log::URGENT, "## Pilots: total "
	  << pilot_cnt << ", local " << local_cnt );
	LOG ( log::URGENT, "## Since: Packets " <<
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
	LOG ( log::URGENT, "## Total: Packets " <<
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
}

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
	if ( m_use_exit_file && ( stat ( exit_file,&buf ) == 0 ) )
	{
		LOG ( log::URGENT, "## Got EXIT file : " << exit_file );
		unlink ( exit_file );
		if ( stat ( exit_file,&buf ) == 0 )
		{
			LOG ( log::URGENT,
				 "WARNING: Unable to delete exit file "
				 << exit_file << "! Disabled interface..." );
			m_use_exit_file = false;
		}
		return 1;
	}
	else if ( m_use_reset_file && ( stat ( reset_file,&buf ) == 0 ) )
	{
		LOG ( log::URGENT, "## Got RESET file "
			 << reset_file );
		unlink ( reset_file );
		if ( stat ( reset_file,&buf ) == 0 )
		{
			LOG ( log::URGENT,
				 "WARNING: Unable to delete reset file "
				 << reset_file << "! Disabled interface..." );
			m_use_reset_file = false;
		}
		m_reinit_data	= true; // init the data port
		m_reinit_query	= true; // init the telnet port
		m_reinit_admin	= true; // init the admin port
		SigHUPHandler ( 0 );
	}
	else if ( m_use_stat_file && ( stat ( stat_file,&buf ) == 0 ) )
	{
		LOG ( log::URGENT, "## Got STAT file " << stat_file );
		unlink ( stat_file );
		if ( stat ( stat_file,&buf ) == 0 )
		{
			LOG ( log::URGENT,
				 "WARNING: Unable to delete stat file "
				 << stat_file << "! Disabled interface..." );
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
}

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
	int         Bytes;
	char        Msg[MAX_PACKET_SIZE];
	fgmp::netaddr     SenderAddress ( fgmp::netaddr::IPv6 );
	fgmp::netsocket*  ListenSockets[3 + MAX_TELNETS];
	time_t      LastTrackerUpdate;
	time_t      CurrentTime;
	PlayerIt    CurrentPlayer;
	LastTrackerUpdate = time ( 0 );
	if ( m_listening == false )
	{
		LOG ( log::ERROR, "FGMS::loop() - "
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
			LOG ( log::ERROR,
			  "# Admin port disabled, "
			  "please set user and password" );
		}
	}
	LOG ( log::ERROR, "# Main server started!" );
#ifdef _MSC_VER
	LOG ( log::URGENT,
	  "ESC key to EXIT (after select "
	  << m_player_expires << " sec timeout)." );
#endif
	if ( ! m_run_as_daemon && m_add_cli )
	{
		// Run admin CLI in foreground reading from stdin
		st_telnet* t = new st_telnet;
		t->Instance = this;
		t->Fd       = 0;
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
		CurrentTime = time ( 0 );
		// check timeout
		CurrentPlayer = m_player_list.Begin();
		for ( size_t i = 0; i < m_player_list.Size(); i++ )
		{
			if ( !m_player_list.CheckTTL ( i )
			|| ( ( ( CurrentTime - CurrentPlayer->LastSeen ) > CurrentPlayer->Timeout )
			   &&  ( ( CurrentTime - CurrentPlayer->JoinTime ) > 30 ) ) )
			{
				drop_client ( CurrentPlayer );
			}
			CurrentPlayer++;
		}
		for ( size_t i = 0; i < m_black_list.Size(); i++ )
		{
			if ( !m_black_list.CheckTTL ( i ) )
			{
				m_black_list.DeleteByPosition ( i );
			}
		}
		// Update some things every (default) 10 secondes
		if ( ( ( CurrentTime - LastTrackerUpdate ) >= m_update_tracker_freq )
		|| ( ( CurrentTime - LastTrackerUpdate ) < 0 ) )
		{
			LastTrackerUpdate = time ( 0 );
			if ( m_player_list.Size() >0 )
			{
				// updates the position of the users
				// regularly (tracker)
				update_tracker ( "" , "", "", LastTrackerUpdate, UPDATE );
			}
			if ( check_files() )
			{
				break;
			}
		} // position (tracker)
		errno = 0;
		ListenSockets[0] = m_data_channel;
		ListenSockets[1] = m_query_channel;
		ListenSockets[2] = m_admin_channel;
		ListenSockets[3] = 0;
		Bytes = m_data_channel->select ( ListenSockets, 0,
		  m_player_expires );
		if ( Bytes < 0 )
		{
			// error
			continue;
		}
		else if ( Bytes == 0 )
		{
			continue;
		}
		if ( ListenSockets[0] > 0 )
		{
			// something on the wire (clients)
			Bytes = m_data_channel->recv_from ( Msg,
			  MAX_PACKET_SIZE, SenderAddress );
			if ( Bytes <= 0 )
			{
				continue;
			}
			m_packets_received++;
			handle_data ( ( char* ) &Msg, Bytes, SenderAddress );
		} // DataSocket
		else if ( ListenSockets[1] > 0 )
		{
			// something on the wire (telnet)
			m_queries_received++;
			int Fd = m_query_channel->accept ( 0 );
			if ( Fd < 0 )
			{
				if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
				{
					LOG ( log::URGENT, "FGMS::loop() - "
					  << strerror ( errno ) );
				}
				continue;
			}
			st_telnet* t = new st_telnet;
			t->Instance = this;
			t->Fd       = Fd;
			pthread_t th;
			pthread_create ( &th, NULL, & detach_telnet, t );
		} // TelnetSocket
		else if ( ListenSockets[2] > 0 )
		{
			// something on the wire (admin port)
			fgmp::netaddr AdminAddress;
			m_admin_received++;
			int Fd = m_admin_channel->accept ( & AdminAddress );
			if ( Fd < 0 )
			{
				if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
				{
					LOG ( log::URGENT, "FGMS::loop() - "
					  << strerror ( errno ) );
				}
				continue;
			}
			LOG ( log::URGENT,
			  "FGMS::loop() - new Admin connection from "
			  << AdminAddress.to_string () );
			st_telnet* t = new st_telnet;
			t->Instance = this;
			t->Fd       = Fd;
			pthread_t th;
			pthread_create ( &th, NULL, &detach_admin, t );
		} // AdminSocket
		//
		// regenrate the client list?
		if ( ( CurrentTime - m_client_last ) > m_client_freq )
		{
			mk_client_list ();
			m_client_last = CurrentTime;
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
	int Port
)
{
	if ( Port != m_data_port )
	{
		m_data_port = Port;
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
	int Port
)
{
	if ( m_query_port != Port )
	{
		m_query_port = Port;
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
	int Port
)
{
	if ( m_admin_port != Port )
	{
		m_admin_port = Port;
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
	LOG ( log::ERROR, "# using logfile " << m_logfile_name );
	if ( ! logger.open ( m_logfile_name ) )
	{
		LOG ( log::ERROR, "FGMS::Init() - "
		  << "Failed to open log file " << m_logfile_name );
	}
	logger.priority ( log::MEDIUM );
	logger.flags ( log::WITH_DATE );
} // FGMS::set_logfile ( const std::string &logfile_name )
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
	LOG ( log::URGENT, "FGMS::done() - exiting" );
	show_stats ();   // 20150619:0.11.9: Add stats to the LOG on exit
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
	m_player_list.Unlock ();
	m_player_list.Clear ();
	m_relay_list.Unlock ();
	m_relay_list.Clear ();
	m_cross_list.Unlock ();
	m_cross_list.Clear ();
	m_black_list.Unlock ();
	m_black_list.Clear ();
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
	const string& Name,
	const string& Passwd,
	const string& Modelname,
	const time_t Timestamp,
	const int type
)
{
	char            TimeStr[100];
	FG_Player	CurrentPlayer;
	Point3D         PlayerPosGeod;
	string          Aircraft;
	string          Message;
	tm*              tm;
	if ( ! m_is_tracked || ( Name == "mpdummy" ) )
	{
		return ( 1 );
	}
	// Creates the UTC time string
	tm = gmtime ( & Timestamp );
	sprintf (
		TimeStr,
		"%04d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year+1900,
		tm->tm_mon+1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec
	);
	// Edits the aircraft name string
	size_t Index = Modelname.rfind ( "/" );
	if ( Index != string::npos )
	{
		Aircraft = Modelname.substr ( Index + 1 );
	}
	else
	{
		Aircraft = Modelname;
	}
	Index = Aircraft.find ( ".xml" );
	if ( Index != string::npos )
	{
		Aircraft.erase ( Index );
	}
	// Creates the message
	if ( type == CONNECT )
	{
		Message  = "CONNECT ";
		Message += Name;
		Message += " ";
		Message += Passwd;
		Message += " ";
		Message += Aircraft;
		Message += " ";
		Message += TimeStr;
		// queue the message
		m_tracker->AddMessage ( Message );
#ifdef ADD_TRACKER_LOG
		write_msg_log ( Message.c_str(), Message.size(), ( char* ) "IN: " ); // write message log
#endif // #ifdef ADD_TRACKER_LOG
		m_tracker_connect++; // count a CONNECT message queued
		return ( 0 );
	}
	else if ( type == DISCONNECT )
	{
		Message  = "DISCONNECT ";
		Message += Name;
		Message += " ";
		Message += Passwd;
		Message += " ";
		Message += Aircraft;
		Message += " ";
		Message += TimeStr;
		// queue the message
		m_tracker->AddMessage ( Message );
#ifdef ADD_TRACKER_LOG
		write_msg_log ( Message.c_str(), Message.size(), ( char* ) "IN: " ); // write message log
#endif // #ifdef ADD_TRACKER_LOG
		m_tracker_disconnect++; // count a DISCONNECT message queued
		return ( 0 );
	}
	// we only arrive here if type!=CONNECT and !=DISCONNECT
	Message = "";
	float heading, pitch, roll;
	size_t j=0; /*message count*/
	for ( size_t i = 0; i < m_player_list.Size(); i++ )
	{
		CurrentPlayer = m_player_list[i];
		if ( CurrentPlayer.ID == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		euler_get ( PlayerPosGeod[Lat], PlayerPosGeod[Lon],
			    CurrentPlayer.LastOrientation[X], CurrentPlayer.LastOrientation[Y], CurrentPlayer.LastOrientation[Z],
			    &heading, &pitch, &roll );
		if ( ( CurrentPlayer.IsLocal ) && ( CurrentPlayer.HasErrors == false ) )
		{
			if ( j!=0 )
			{
				Message += "\n";
			}
			sgCartToGeod ( CurrentPlayer.LastPos, PlayerPosGeod );
			Message +=  "POSITION ";
			Message += CurrentPlayer.Name +" ";
			Message += CurrentPlayer.Passwd +" ";
			Message += NumToStr ( PlayerPosGeod[Lat], 6 ) +" "; //lat
			Message += NumToStr ( PlayerPosGeod[Lon], 6 ) +" "; //lon
			Message += NumToStr ( PlayerPosGeod[Alt], 6 ) +" "; //alt
			Message += NumToStr ( heading, 6 ) +" ";
			Message += NumToStr ( pitch,   6 ) +" ";
			Message += NumToStr ( roll,    6 ) +" ";
			Message += TimeStr;
			// queue the message
			j++;
		}
#ifdef TRACK_ALL
		if ( !CurrentPlayer.IsLocal )
		{
			if ( j!=0 )
			{
				Message += "\n";
			}
			sgCartToGeod ( CurrentPlayer.LastPos, PlayerPosGeod );
			Message +=  "POSITION ";
			Message += CurrentPlayer.Name +" ";
			Message += CurrentPlayer.Passwd +" ";
			Message += NumToStr ( PlayerPosGeod[Lat], 6 ) +" "; //lat
			Message += NumToStr ( PlayerPosGeod[Lon], 6 ) +" "; //lon
			Message += NumToStr ( PlayerPosGeod[Alt], 6 ) +" "; //alt
			Message += NumToStr ( heading, 6 ) +" ";
			Message += NumToStr ( pitch,   6 ) +" ";
			Message += NumToStr ( roll,    6 ) +" ";
			Message += TimeStr;
			// queue the message
			j++;
		}
#endif
	} // while
	if ( Message!= "" )
	{
		m_tracker->AddMessage ( Message );
		m_tracker_position++; // count a POSITION messge queued
	}
	Message.erase ( 0 );
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
			m_tracker->want_exit = true;
			pthread_cond_signal ( &m_tracker->condition_var );  // wake up the worker
			pthread_join ( m_tracker->GetThreadID(), 0 );
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
	const PlayerIt& Sender,
	const FG_Player& Receiver
)
{
	if ( Distance ( Sender->LastPos, Receiver.LastPos ) < Receiver.RadarRange )
	{
		return true;
	}
	return false;
	// TODO:
	float	out_of_reach;
	if ( ( Sender->IsATC == FG_Player::ATC_NONE )
			&&   ( Receiver.IsATC == FG_Player::ATC_NONE ) )
	{
		// Sender and Receiver are normal pilots, so m_out_of_reach applies
		if ( Distance ( Sender->LastPos, Receiver.LastPos ) < Receiver.RadarRange )
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
	if ( Receiver.IsATC != FG_Player::ATC_NONE )
	{
		switch ( Receiver.IsATC )
		{
		case FG_Player::ATC_DL:
		case FG_Player::ATC_GN:
			out_of_reach = 5;
			break;
		case FG_Player::ATC_TW:
			out_of_reach = 30;
			break;
		case FG_Player::ATC_AP:
		case FG_Player::ATC_DE:
			out_of_reach = 100;
			break;
		case FG_Player::ATC_CT:
			out_of_reach = 400;
			break;
		default:
			out_of_reach = m_out_of_reach;
		}
	}
	else if ( Sender->IsATC != FG_Player::ATC_NONE )
	{
		// FIXME: if sender is the ATC, the pos-data does not need to be send.
		//        but we can not implement it before chat- and pos-messages are
		//        sent seperatly. For now we leave it to be
		//        m_out_of_reach
		out_of_reach = m_out_of_reach;
		// return false;
	}
	if ( Distance ( Sender->LastPos, Receiver.LastPos ) < out_of_reach )
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
	const PlayerIt& Sender,
	const FG_Player& Receiver
)
{
	float	out_of_reach;
	float	altitude;
	//////////////////////////////////////////////////
	// If the sender is an ATC use a predefined
	// range for radio transmission. For now we
	// use m_out_of_reach
	//////////////////////////////////////////////////
	if ( Sender->IsATC != FG_Player::ATC_NONE )
	{
		if ( Distance ( Sender->LastPos, Receiver.LastPos ) <
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
	altitude = Sender->GeodPos[Alt];
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
	if ( Distance ( Sender->LastPos, Receiver.LastPos ) < out_of_reach )
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
 * @param SendingPlayer
 * @retval true is within range
 */
bool
FGMS::is_in_range
(
	const ListElement& Relay,
	const PlayerIt& SendingPlayer,
	uint32_t MsgId
)
{
	FG_Player CurrentPlayer;
	size_t    Cnt;
	Cnt = m_player_list.Size ();
	for ( size_t i = 0; i < Cnt; i++ )
	{
		CurrentPlayer = m_player_list[i];
		if ( CurrentPlayer.ID == ListElement::NONE_EXISTANT )
		{
			continue;
		}
		if ( CurrentPlayer.Address == Relay.Address )
		{
			if ( MsgId == CHAT_MSG_ID )
			{
				// apply 'radio' rules
				// if ( receiver_wants_chat( SendingPlayer, CurrentPlayer ) )
				if ( receiver_wants_data ( SendingPlayer, CurrentPlayer ) )
				{
					return true;
				}
			}
			else
			{
				// apply 'visibility' rules, for now we apply 'radio' rules
				if ( receiver_wants_data ( SendingPlayer, CurrentPlayer ) )
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

using namespace std;

/** @brief The running  ::FGMS server process */
FGMS       fgms;

/** @def DEF_CONF_FILE
 *  @brief The default config file to load unless overriden on \ref command_line
 */
#ifndef DEF_CONF_FILE
#define DEF_CONF_FILE "fgms.conf"
#endif

#ifdef _MSC_VER
// kludge for getopt() for WIN32
// FIXME: put in libmsc
static char* optarg;
static int curr_arg = 0;
int getopt ( int argc, char* argv[], char* args )
{
	size_t len = strlen ( args );
	size_t i;
	int c = 0;
	if ( curr_arg == 0 )
	{
		curr_arg = 1;
	}
	if ( curr_arg < argc )
	{
		char* arg = argv[curr_arg];
		if ( *arg == '-' )
		{
			arg++;
			c = *arg; // get first char
			for ( i = 0; i < len; i++ )
			{
				if ( c == args[i] )
				{
					// found
					if ( args[i+1] == ':' )
					{
						// fill in following
						curr_arg++;
						optarg = argv[curr_arg];
					}
					break;
				}
			}
			curr_arg++;
			return c;
		}
		else
		{
			return '-';
		}
	}
	return -1;
}
#endif // _MSC_VER

//////////////////////////////////////////////////////////////////////

/** Read a config file and set internal variables accordingly
 *
 * @param ConfigName Path of config file to load
 * @retval int  -- todo--
 */
bool
FGMS::process_config
(
	const string& ConfigName
)
{
	FG_CONFIG   Config;
	string      Val;
	int         E;
	if ( m_have_config )	// we already have a config, so ignore
	{
		return ( true );
	}
	if ( Config.Read ( ConfigName ) )
	{
		LOG ( log::URGENT,
		  "Could not read config file '" << ConfigName
		  << "' => using defaults");
		return ( false );
	}
	LOG ( log::ERROR, "processing " << ConfigName );
	fgms.m_config_file =  ConfigName;
	Val = Config.Get ( "server.name" );
	if ( Val != "" )
	{
		m_server_name = Val;
	}
	Val = Config.Get ( "server.address" );
	if ( Val != "" )
	{
		m_bind_addr = Val;
	}
	Val = Config.Get ( "server.FQDN" );
	if ( Val != "" )
	{
		m_FQDN = Val;
	}
	Val = Config.Get ( "server.port" );
	if ( Val != "" )
	{
		set_data_port ( StrToNum<int> ( Val, E ) );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for DataPort: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.telnet_port" );
	if ( Val != "" )
	{
		set_query_port ( StrToNum<int> ( Val, E ) );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for TelnetPort: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.admin_cli" );
	if ( Val != "" )
	{
		if ( ( Val == "on" ) || ( Val == "true" ) )
		{
			m_add_cli = true;
		}
		else if ( ( Val == "off" ) || ( Val == "false" ) )
		{
			m_add_cli = false;
		}
		else
		{
			LOG ( log::URGENT,
			  "unknown value for 'server.admin_cli'!"
			  << " in file " << ConfigName
			);
		}
	}
	Val = Config.Get ( "server.admin_port" );
	if ( Val != "" )
	{
		set_admin_port ( StrToNum<int> ( Val, E ) );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for AdminPort: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.admin_user" );
	if ( Val != "" )
	{
		m_admin_user = Val;
	}
	Val = Config.Get ( "server.admin_pass" );
	if ( Val != "" )
	{
		m_admin_pass = Val;
	}
	Val = Config.Get ( "server.admin_enable" );
	if ( Val != "" )
	{
		m_admin_enable = Val;
	}
	Val = Config.Get ( "server.out_of_reach" );
	if ( Val != "" )
	{
		m_out_of_reach = StrToNum<int> ( Val, E );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for out_of_reach: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.max_radar_range" );
	if ( Val != "" )
	{
		m_max_radar_range = StrToNum<int> ( Val, E );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for max_radar_range: '" << Val
			  << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.playerexpires" );
	if ( Val != "" )
	{
		m_player_expires = StrToNum<int> ( Val, E );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for Expire: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.logfile" );
	if ( Val != "" )
	{
		fgms.set_logfile ( Val );
	}
	Val = Config.Get ( "server.daemon" );
	if ( Val != "" )
	{
		if ( ( Val == "on" ) || ( Val == "true" ) )
		{
			m_run_as_daemon = true;
		}
		else if ( ( Val == "off" ) || ( Val == "false" ) )
		{
			m_run_as_daemon = false;
		}
		else
		{
			LOG ( log::URGENT,
			  "unknown value for 'server.daemon'!"
			  << " in file " << ConfigName
			);
		}
	}
	Val = Config.Get ( "server.tracked" );
	if ( Val != "" )
	{
		string  Server;
		int     Port;
		bool    tracked = false;

		if ( Val == "true" )
		{
			tracked = true;
			Server = Config.Get ( "server.tracking_server" );
			Val = Config.Get ( "server.tracking_port" );
			Port = StrToNum<int> ( Val, E );
			if ( E )
			{
				LOG ( log::URGENT,
				  "invalid value for tracking_port: '"
				  << Val << "'"
				);
				exit ( 1 );
			}
			if ( tracked
			&& ( ! add_tracker ( Server, Port, tracked ) ) ) // set master m_is_tracked
			{
				LOG ( log::URGENT,
				  "Failed to get IPC msg queue ID! error "
				  << errno );
				exit ( 1 ); // do NOT continue if a requested 'tracker' FAILED
			}
		}
	}
	Val = Config.Get ( "server.is_hub" );
	if ( Val != "" )
	{
		if ( Val == "true" )
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
	bool    MoreToRead  = true;
	string  Section = "relay";
	string  Var;
	string  Server = "";
	int     Port   = 0;
	if ( ! Config.SetSection ( Section ) )
	{
		MoreToRead = false;
	}
	while ( MoreToRead )
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if ( Var == "relay.host" )
		{
			Server = Val;
		}
		if ( Var == "relay.port" )
		{
			Port = StrToNum<int> ( Val, E );
			if ( E )
			{
				LOG ( log::URGENT,
				  "invalid value for RelayPort: '"
				  << Val << "'"
				);
				exit ( 1 );
			}
		}
		if ( ( Server != "" ) && ( Port != 0 ) )
		{
			fgms.add_relay ( Server, Port );
			Server = "";
			Port   = 0;
		}
		if ( Config.SecNext () == 0 )
		{
			MoreToRead = false;
		}
	}
	//////////////////////////////////////////////////
	//      read the list of crossfeeds
	//////////////////////////////////////////////////
	MoreToRead  = true;
	Section = "crossfeed";
	Var    = "";
	Server = "";
	Port   = 0;
	if ( ! Config.SetSection ( Section ) )
	{
		MoreToRead = false;
	}
	while ( MoreToRead )
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if ( Var == "crossfeed.host" )
		{
			Server = Val;
		}
		if ( Var == "crossfeed.port" )
		{
			Port = StrToNum<int> ( Val, E );
			if ( E )
			{
				LOG ( log::URGENT,
				  "invalid value for crossfeed.port: '"
				  << Val << "'"
				);
				exit ( 1 );
			}
		}
		if ( ( Server != "" ) && ( Port != 0 ) )
		{
			fgms.add_crossfeed ( Server, Port );
			Server = "";
			Port   = 0;
		}
		if ( Config.SecNext () == 0 )
		{
			MoreToRead = false;
		}
	}

	//////////////////////////////////////////////////
	//      read the list of whitelisted IPs
	//      (a crossfeed might list the sender here
	//      to avoid blacklisting without defining the
	//      sender as a relay)
	//////////////////////////////////////////////////
	MoreToRead  = true;
	Section = "whitelist";
	Var    = "";
	Val    = "";
	if ( ! Config.SetSection ( Section ) )
	{
		MoreToRead = false;
	}
	while ( MoreToRead )
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if ( Var == "whitelist" )
		{
			fgms.add_whitelist ( Val.c_str() );
		}
		if ( Config.SecNext () == 0 )
		{
			MoreToRead = false;
		}
	}

	//////////////////////////////////////////////////
	//      read the list of blacklisted IPs
	//////////////////////////////////////////////////
	MoreToRead  = true;
	Section = "blacklist";
	Var    = "";
	Val    = "";
	if ( ! Config.SetSection ( Section ) )
	{
		MoreToRead = false;
	}
	while ( MoreToRead )
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if ( Var == "blacklist" )
		{
			fgms.add_blacklist ( Val.c_str(),
			  "static config entry", 0 );
		}
		if ( Config.SecNext () == 0 )
		{
			MoreToRead = false;
		}
	}

	//////////////////////////////////////////////////
	return ( true );
} // FGMS::process_config ( const string& ConfigName )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print a help screen for command line parameters, see \ref command_line
 */
void
FGMS::print_version
()
{
	std::cout << std::endl;
	std::cout << "fgms version " << fgms.m_version
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
	int     E;
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
			set_query_port ( StrToNum<int> ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for TelnetPort: '" << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'b':
			set_admin_port ( StrToNum<int> ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for AdminPort: '" << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'c':
			process_config ( optarg );
			break;
		case 'p':
			set_data_port ( StrToNum<int>  ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for DataPort: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'o':
			m_out_of_reach = StrToNum<int> ( optarg, E );
			if ( E )
			{
				cerr << "invalid value for OutOfReach: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'v':
			logger.priority ( StrToNum<int> ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for Loglevel: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 't':
			m_player_expires = StrToNum<int> ( optarg, E );
			if ( E )
			{
				cerr << "invalid value for expire: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'l':
			fgms.set_logfile ( optarg );
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
	bool ReInit
)
{
	string Path;
#ifndef _MSC_VER
	Path = SYSCONFDIR;
	Path += "/" DEF_CONF_FILE; // fgms.conf
	if ( process_config ( Path ) == true )
	{
		return 1;
	}
	Path = getenv ( "HOME" );
#else
	char* cp = getenv ( "HOME" );
	if ( cp )
	{
		Path = cp;
	}
	else
	{
		cp = getenv ( "USERPROFILE" ); // XP=C:\Documents and Settings\<name>, Win7=C:\Users\<user>
		if ( cp )
		{
			Path = cp;
		}
	}
#endif
	if ( Path != "" )
	{
		Path += "/" DEF_CONF_FILE;
		if ( process_config ( Path ) )
		{
			return 1;
		}
	}
	if ( process_config ( DEF_CONF_FILE ) )
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
	if ( ( m_server_name == "fgms" ) && ( m_relay_list.Size () > 0) )
	{
		LOG ( log::ERROR, "If you want to provide a public "
		  << "server, please provide a unique server name!"
			
		);
		return false;
	}
	return true;
} // FGMS::check_config ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief If we receive a SIGHUP, reinit application
 * @param SigType int with signal type
 */
void
SigHUPHandler
(
	int SigType
)
{
	fgms.prepare_init ();
	if (fgms.m_config_file == "")
	{
		if ( ! fgms.read_configs ( true ) )
		{
			LOG ( log::HIGH,
			  "received HUP signal, but read config file failed!" );
			exit ( 1 );
		}
	}
	else
	{
		if ( fgms.process_config ( fgms.m_config_file ) == false )
		{
			LOG ( log::HIGH,
			  "received HUP signal, but read config file failed!" );
			exit ( 1 );
		}
	}
	if ( fgms.init () != 0 )
	{
		LOG ( log::HIGH,
		  "received HUP signal, but reinit failed!" );
		exit ( 1 );
	}
#ifndef _MSC_VER
	signal ( SigType, SigHUPHandler );
#endif
} // SigHUPHandler ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief MAIN routine
 * @param argc
 * @param argv*[]
 */
int
main
(
	int argc,
	char* argv[]
)
{
	fgms.parse_params ( argc, argv );
	fgms.read_configs ();
	if ( ! fgms.check_config() )
	{
		exit ( 1 );
	}
#ifndef _MSC_VER
	signal ( SIGHUP, SigHUPHandler );
#endif
	if ( ! fgms.init () )
	{
		return 1;
	}
	fgms.loop();
	fgms.done();
	return ( 0 );
} // main()
//////////////////////////////////////////////////////////////////////

