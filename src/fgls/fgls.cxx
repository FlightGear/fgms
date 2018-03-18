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
 * @file        fgls.cxx
 *              flightgear list server
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        07/2017
 *
 * This is the list server for the flightgear multiplayer
 * network. All servers register at this server. All clients ask fgls
 * which server they should use.
 *
 */

#include <iostream>
#include <thread>
#ifndef _MSC_VER
#include <fglib/daemon.hxx>
#include <unistd.h>     // getopt
#else
#include <libmsc/msc_getopt.hxx>
#endif
#include <fglib/fg_log.hxx>
#include <fglib/fg_util.hxx>
#include "fgls_cli.hxx"
#include "fgls.hxx"

#ifndef _MSC_VER
/** @brief An instance of daemon */
fgmp::daemon Myself;
#endif

namespace fgmp
{

const version fgls::m_version ( 0, 0, 1, "-dev" );

using prio = fglog::prio;

//////////////////////////////////////////////////////////////////////

std::ostream&
operator <<
(
	std::ostream& o,
	const server& s
)
{
	o << s.name
	  << " " << s.location
	  << " " << s.addr
	  << " (ID " << s.id << ")";
	return o;
} // operator << ( server )

//////////////////////////////////////////////////////////////////////

fgls::fgls
()
{
	m_uptime = time ( 0 );
} // fgls::fgls()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Basic initialisation
 * @return true OK
 * @return false on error
 */
bool
fgls::init
()
{
	//
	// enable logging
	//
	if ( m_logfile_name != "" )
		open_logfile ();
	logger.priority ( m_debug_level );
	logger.set_flags ( fglog::flags::WITH_DATE );
	LOG ( prio::EMIT, "# FlightGear List Server v"
		<< m_version.str() << " started" );
	//
	// enable data channel
	//
	if ( ! init_data_channel () )
		return false;
	//
	// enable query channel
	//
	if ( ! init_query_channel () )
		return false;
	//
	// enable admin channel
	//
	if ( ! init_admin_channel () )
		return false;
	if ( m_data_channel == nullptr )
	{
		LOG ( prio::EMIT, "fgls::loop() - "
		      << "not listening on any socket!" );
		return false;
	}
	if ( ! m_run_as_daemon && m_cli_enabled )
	{
		// Run admin cli in foreground reading from stdin
		using namespace std::placeholders;
		std::thread th { std::bind ( &fgls::handle_admin, this, _1 ), 0 };
		th.detach ();
	}
#ifndef _MSC_VER
	if ( m_run_as_daemon )
	{
		m_myself.daemonize ();
		LOG ( prio::URGENT, "# My PID is " << m_myself.get_pid() );
	}
#endif
	return true;
} // fgls::init ()

//////////////////////////////////////////////////////////////////////

void
fgls::loop
()
{
	LOG ( prio::EMIT, "# Main server started!" );
	m_is_parent = true;
	time_t  current_time;
	time_t  last_time { time ( 0 ) };
	int     bytes;
	netsocket* listener [3 + MAX_TELNETS];
	do
	{
		if ( m_data_channel == nullptr )
		{
			LOG ( prio::EMIT, "lost data channel!" );
			return;
		}
		current_time = time ( 0 );
		errno = 0;
		listener[0] = m_data_channel;
		listener[1] = m_query_channel;
		listener[2] = m_admin_channel;
		listener[3] = 0;
		bytes = m_data_channel->select ( listener, 0, m_check_interval );
#if 0
		TODO
		if ( current_time - last_time > m_check_interval )
		{
			std::cout << "check time" << std::endl;
		}
#endif
		if ( bytes < 0 )
			continue;        // some error
		else if ( bytes == 0 )
			continue;
		if ( listener[0] > 0 )
		{
			// something on the data channel
			//bytes = m_data_channel->RecvFrom();
		}
		if ( listener[1] > 0 )
		{
			// something on the query channel
			//bytes = m_data_channel->RecvFrom();
		}
		if ( listener[0] > 0 )
		{
			// something on the admin channel
			netaddr admin_addr ( netaddr::IPv6 );
			int fd = m_admin_channel->accept ( & admin_addr );
			if ( fd < 0 )
			{
				if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
				{
					LOG ( prio::URGENT, "fgls::Loop() - "
					      << strerror ( errno ) );
				}
				continue;
			}
			LOG ( prio::URGENT,
			      "fgls::Loop() - new Admin connection from "
			      << admin_addr.to_string () );
			using namespace std::placeholders;
			std::thread th
			{
				std::bind ( &fgls::handle_admin, this, _1 ), 0
			};
			th.detach ();
		}
	}
	while ( m_want_exit == false );
} // fgls::loop ()

//////////////////////////////////////////////////////////////////////
/** Initialise the data channel
 * @return true OK
 * @return false something went wrong
 */
bool
fgls::init_data_channel
()
{
	if ( ! m_reinit_data )
		return true;
	if ( m_data_channel != nullptr )
		delete m_data_channel;
	m_data_channel = new netsocket ();
	try
	{
		m_data_channel->listen_to ( m_bind_addr, m_data_port,
					    netsocket::UDP );
	}
	catch ( std::runtime_error& e )
	{
		LOG ( prio::URGENT, "fgls::Init() - "
		      << "failed to bind on " << m_bind_addr
		      << ":" << m_data_port );
		LOG ( prio::URGENT, "already in use?" );
		return false;
	}
	m_reinit_data = false;
	return true;
} // fgls::init_data_channel()

//////////////////////////////////////////////////////////////////////
/** Initialise the query channel
 * @return true OK
 * @return false something went wrong
 */
bool
fgls::init_query_channel
()
{
	if ( ! m_reinit_query )
		return true;
	if ( m_query_channel != nullptr )
		delete m_query_channel;
	m_query_channel = nullptr;
	m_reinit_query = false;
	if ( m_query_port == 0 )
		return true;        // query channel disabled
	m_query_channel = new netsocket ();
	try
	{
		m_query_channel->listen_to ( m_bind_addr,
					     m_query_port, netsocket::TCP );
	}
	catch ( std::runtime_error& e )
	{
		LOG ( prio::URGENT, "fgls::Init() - "
		      << "failed to listen to query port" );
		return false;
	}
	return true;
} // fgls::init_query_channel()

//////////////////////////////////////////////////////////////////////
/** Initialise the admin channel
 * @return true OK
 * @return false something went wrong
 */
bool
fgls::init_admin_channel
()
{
	if ( ! m_reinit_cli )
		return true;
	if ( m_admin_channel != nullptr )
		delete m_admin_channel;
	m_admin_channel = nullptr;
	m_reinit_cli = false;
	if  ( ( m_cli_port == 0 ) || ( m_cli_enabled == false ) )
		return true;        // admin channel disabled
	m_admin_channel = new netsocket ();
	try
	{
		m_admin_channel->listen_to ( m_bind_addr,
					     m_cli_port, netsocket::TCP );
	}
	catch ( std::runtime_error& e )
	{
		LOG ( prio::URGENT, "fgls::Init() - "
		      << "failed to listen to query port" );
		return false;
	}
	return true;
} // fgls::init_admin_channel()

//////////////////////////////////////////////////////////////////////

void
fgls::set_bind_addr
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
	m_reinit_cli = true;
	init_data_channel ();
	init_query_channel ();
	init_admin_channel ();
} // fgls::set_bind_addr ()

//////////////////////////////////////////////////////////////////////

void
fgls::open_logfile
()
{
	if ( m_reinit_log == false )
		return;        // nothing to do
	if ( m_logfile_name == "" )
		return;
	LOG ( prio::EMIT, "# using logfile '" << m_logfile_name << "'" );
	if ( ! logger.open ( m_logfile_name ) )
	{
		LOG ( prio::URGENT,
		      "fgls::open_logfile(): "
		      << "Failed to open log file " << m_logfile_name );
	}
	m_reinit_log = false;
} // fgls::open_logfile ()

//////////////////////////////////////////////////////////////////////

void
fgls::shutdown
()
{
	if ( ! m_is_parent )
		return;
	LOG ( prio::URGENT, "fgls::Shutdown() - exiting" );
	logger.close ();
	if ( m_data_channel != nullptr )
	{
		m_data_channel->close ();
		delete m_data_channel;
		m_data_channel = nullptr;
	}
	if ( m_query_channel != nullptr )
	{
		m_query_channel->close ();
		delete m_query_channel;
		m_query_channel = nullptr;
	}
	if ( m_admin_channel != nullptr )
	{
		m_admin_channel->close ();
		delete m_admin_channel;
		m_admin_channel = nullptr;
	}
} // fgls::shutdown ()

//////////////////////////////////////////////////////////////////////

/** Read a config file
 *
 * Reads in @c config_name and sets internal variables accordingly
 * @param config_name   Path to config file to read
 * @return true         OK
 * @return false        An error occurred
 */
bool
fgls::process_config
(
	const std::string& config_name
)
{
	if ( m_have_config )    // we already have a config, so ignore
		return true;
	LOG ( prio::EMIT, "processing " << config_name );
	using namespace libcli;
	RESULT r;
	fgls_cli cli ( this, 0 );
	r = cli.file ( config_name, PRIVLEVEL::PRIVILEGED,CLI_MODE::CONFIG );
	if ( RESULT::INVALID_ARG == r )
	{
		LOG ( prio::EMIT, "failed to read " << config_name );
		return false;
	}
	m_have_config = true;
	return true;
} // fgls::process_config ()

//////////////////////////////////////////////////////////////////////

void
fgls::handle_admin
(
	int fd
)
{
	fgls_cli* cli;
	errno = 0;
	cli = new fgls_cli ( this, fd );
	cli->loop ();
	if ( fd == 0 )
	{
		// reading from stdin
		m_want_exit = true;
		m_check_interval = 1;
	}
	delete cli;
} // fgls::handle_admin()

//////////////////////////////////////////////////////////////////////

/** Try to read the config file from several well known places
 *
 * @param reinit        true when reinitialising (restart)
 * @return true         OK
 * @return false        on error
 */
bool
fgls::read_configs
()
{
	std::string Path;
	const char* config_name = "fgls.conf";
	#ifndef _MSC_VER
	// try /etc/fgms.conf (or whatever SYSCONFDIR is)
	Path  = SYSCONFDIR;
	Path += "/";
	Path += config_name;
	if ( process_config ( Path ) == true )
		return true;
	// failed, try to find config in home dir
	Path = getenv ( "HOME" );
	#else
	// windows version
	char* cp = getenv ( "HOME" );
	if ( cp )
		Path = cp;
	else
	{
		cp = getenv ( "USERPROFILE" );
		if ( cp )
			Path = cp;
	}
	#endif
	if ( Path != "" )
	{
		Path += "/";
		Path += config_name;
		if ( process_config ( Path ) == true )
			return true;
	}
	// failed, try current directory
	if ( process_config ( config_name ) == true )
		return true;
	LOG ( prio::EMIT,
	      "Could not find a config file => using defaults" );
	return true;
} // fgls::read_configs ()

//////////////////////////////////////////////////////////////////////


/**
 * @brief Print a help screen for command line parameters
 */
void
fgls::print_version
()
{
	std::cout << std::endl;
	std::cout << "fgls version " << m_version
		  << ", compiled on " << __DATE__
		  << " at " << __TIME__ << std::endl;
	std::cout << std::endl;
} // fgls::print_version()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Print a help screen for command line parameters
 */
void
fgls::print_help
()
{
	print_version ();
	std::cout <<
	  "syntax: " << m_argv[0] << " [options]\n"
	  "\n"
	  "options are:\n"
	  "-h          print this help screen\n"
	  "-p PORT     listen port for data\n"
	  "-a PORT     listen port for admin cli\n"
	  "-t PORT     listen port for telnet queries\n"
	  "-l LOGFILE  log to LOGFILE\n"
	  "-L LEVEL    verbosity of log in range 0 (few) to 3 (much)\n"
	  "            5 to disable\n"
	  "-d          do not run in backgound (overwrite config file)\n"
	  "-D          do run in backgound (overwrite config file)\n"
	  "            default is: run in background\n"
	  "-v          print version and exit\n"
	  "\n";
} // fgls::print_help()

//////////////////////////////////////////////////////////////////////

/**
 * @bried Parse commandline parameters
 * @param argc  number of commandline arguments
 * @param argv  list of parameters
 */
void
fgls::parse_params
(
	int argc,
	char* argv[]
)
{
	int     m;
	int     e;
	m_argc = argc;
	m_argv = argv;
	while ( ( m=getopt ( argc, argv, "hp:a:c:t:l:L:dDv" ) ) != -1 )
	{
		switch ( m )
		{
		case 'h':
			print_help ();
			exit ( 0 );
		case 'p':
			m_data_port = str_to_num<uint16_t> ( optarg, e );
			if ( e != 0 )
			{
				std::cerr << "invalid value for query port "
				  "'" << optarg << "' " << e  << std::endl;
				exit ( 1 );
			}
			break;
		case 'a':
			m_cli_port = str_to_num<uint16_t> ( optarg, e );
			if ( e )
			{
				std::cerr << "invalid value for admin port "
				  "'" << optarg << "'" << std::endl;
				exit ( 1 );
			}
			break;
		case 'c':
			process_config ( optarg );
			break;
		case 't':
			m_query_port = str_to_num<uint16_t> ( optarg, e );
			if ( e )
			{
				std::cerr << "invalid value for query port "
				  "'" << optarg << "'" << std::endl;
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
		case 'v':
			print_version ();
			exit ( 0 );
		}
	} // while()
} // fgls::parse_params()

} // namespace fgmp

//////////////////////////////////////////////////////////////////////

int
main
(
	int   argc,
	char* argv[]
)
{
	fgmp::fgls fgls;
	fgls.parse_params ( argc, argv );
	fgls.read_configs ();
	if ( ! fgls.init() )
	{
		return 1;
	}
	fgls.loop ();
	fgls.shutdown ();
	return 0;
}

