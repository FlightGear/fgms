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
 * @file	fgls.cxx
 *		flightgear list server
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	07/2017
 *
 * This is the list server for the flightgear multiplayer
 * network. All servers register at this server. All clients ask FGLS
 * which server they should use.
 *
 */

#include <iostream>
#include <unistd.h>	// getopt
#include <fglib/daemon.hxx>
#include <fglib/fg_log.hxx>
#include <fglib/fg_util.hxx>
#include <fglib/fg_config.hxx>
#include "fgls_cli.hxx"
#include "fgls.hxx"

const FG_VERSION FGLS::m_version ( 0, 0, 1, "-dev" );

#ifdef _MSC_VER
	#define M_IS_DIR _S_IFDIR
#else // !_MSC_VER
	#define M_IS_DIR S_IFDIR
	/** @brief An instance of ::cDaemon */
	extern  cDaemon Myself;
#endif

//////////////////////////////////////////////////////////////////////

server::server
()
{
	id		= ( size_t ) -1;
	type		= fgms::eCLIENTTYPES::UNSET;
	//name	
	//location
	//admin
	//version
	last_seen	= 0;
	registered_at	= 0;

} // server::server()

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
}; // operator << ( server )

//////////////////////////////////////////////////////////////////////

FGLS::FGLS
()
{
	m_run_as_daemon	= false;
	m_add_cli	= true;
	m_bind_addr	= "";
	// the main data channel
	m_data_port	= 5004;
	m_reinit_data	= true;
	m_data_channel	= 0;
	// the query channel
	m_query_port	= m_data_port + 1;
	m_reinit_query	= true;
	m_query_channel	= 0;
	// the admin channel
	m_admin_port	= m_data_port + 2;
	m_reinit_admin	= true;
	m_admin_channel	= 0;
	m_admin_cli	= true;
	// logging
	m_logfile_name	= "";
	m_reinit_log	= true;
	m_debug_level	= log::MEDIUM;
	// general
	m_is_parent	= false;
	m_check_interval = 5;
	m_have_config	= false;
	m_server_name	= "fgls";
	m_argc		= 0;
	m_want_exit	= false;
	m_uptime	= time ( 0 );
} // FGLS::FGLS()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Basic initialisation
 * @return true OK
 * @return false on error
 */
bool
FGLS::init
()
{
	//
	// enable logging
	//
	if ( m_logfile_name != "" )
	{
		open_logfile ();
	}
	logger.priority ( log::MEDIUM );
	logger.flags ( log::WITH_DATE );
#ifndef _MSC_VER
	if ( m_run_as_daemon )
	{
		Myself.Daemonize ();
	}
#endif
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
	return true;
} // FGLS::init ()

//////////////////////////////////////////////////////////////////////

void
FGLS::loop
()
{
	time_t	current_time;
	int	bytes;
	fgmp::netsocket* listener [3 + MAX_TELNETS];

	if ( m_data_channel == 0 )
	{
		LOG ( log::ERROR, "FGLS::loop() - "
		  << "not listening on any socket!" );
		return;
	}
	if ( ( m_admin_user == "" ) || ( m_admin_pass == "" ) )
	{
		if ( m_admin_channel )
		{
			m_admin_channel->close ();
			delete m_admin_channel;
			m_admin_channel = 0;
			LOG ( log::ERROR,
			  "# Admin port disabled, "
			  "please set user and password"
			);
		}
	}
	if ( ! m_run_as_daemon && m_add_cli )
	{
		// Run admin CLI in foreground reading from stdin
		st_telnet* t = new st_telnet;
		t->instance = this;
		t->fd       = 0;
		pthread_t th;
		pthread_create ( &th, NULL, &detach_admin_cli, t );
	}
	LOG ( log::ERROR, "# Main server started!" );
	m_is_parent = true;
	while ( m_want_exit == false )
	{
		if ( m_data_channel == 0 )
		{
			LOG ( log::ERROR, "lost data channel!" );
			return;
		}
		current_time = time ( 0 );
		errno = 0;
		listener[0] = m_data_channel;
		listener[1] = m_query_channel;
		listener[2] = m_admin_channel;
		listener[3] = 0;
		bytes = m_data_channel->select( listener, 0, m_check_interval );
		if ( bytes < 0 )
			continue;	// some error
		else if ( bytes == 0 )
			continue;
		if ( listener[0] > 0 )
		{	// something on the data channel
			//bytes = m_data_channel->RecvFrom();
		}
		if ( listener[1] > 0 )
		{	// something on the query channel
			//bytes = m_data_channel->RecvFrom();
		}
		if ( listener[0] > 0 )
		{	// something on the admin channel
			fgmp::netaddr admin_addr ( fgmp::netaddr::IPv6 );
			int fd = m_admin_channel->accept ( & admin_addr );
			if ( fd < 0 )
			{
				if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
				{
					LOG ( log::URGENT, "FGLS::Loop() - "
					  << strerror ( errno ) );
				}
				continue;
			}
			LOG ( log::URGENT,
			  "FGLS::Loop() - new Admin connection from "
			  << admin_addr.to_string () );
			st_telnet* t = new st_telnet;
			t->instance = this;
			t->fd       = fd;
			pthread_t th;
			pthread_create ( &th, NULL, &detach_admin_cli, t );
		}
	}
} // FGLS::loop ()

//////////////////////////////////////////////////////////////////////
/** Initialise the data channel
 * @return true OK
 * @return false something went wrong
 */
bool
FGLS::init_data_channel
()
{
	if ( ! m_reinit_data )
		return true;
	if ( m_data_channel )
	{
		delete m_data_channel;
		m_data_channel = 0;
	}
	m_data_channel = new fgmp::netsocket ();
	if ( m_data_channel->open ( fgmp::netaddr::IPv6, fgmp::netsocket::UDP ) == 0 )
	{
		LOG ( log::URGENT, "FGMS::Init() - "
		  << "failed to create data socket" );
		return false;
	}
	m_data_channel->set_blocking ( false );
	m_data_channel->set_sock_opt ( SO_REUSEADDR, true );
	if ( ! m_data_channel->bind ( m_bind_addr, m_data_port ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to bind on " << m_bind_addr
		  << ":" << m_data_port );
		LOG ( log::URGENT, "already in use?" );
		return false;
	}
	m_reinit_data = false;
	return true;
} // FGLS::init_data_channel()

//////////////////////////////////////////////////////////////////////
/** Initialise the query channel
 * @return true OK
 * @return false something went wrong
 */
bool
FGLS::init_query_channel
()
{
	if ( ! m_reinit_query )
		return true;
	if ( m_query_channel )
	{
		delete m_query_channel;
	}
	m_query_channel = 0;
	if ( m_query_port == 0 )
	{
		m_reinit_query = false;
		return true; // query channel disabled
	}
	m_query_channel = new fgmp::netsocket ();
	if ( m_query_channel->open ( fgmp::netaddr::IPv6, fgmp::netsocket::TCP ) == 0 )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to create Query socket" );
		return false;
	}
	m_query_channel->set_blocking ( false );
	m_query_channel->set_sock_opt ( SO_REUSEADDR, true );
	if ( ! m_query_channel->bind ( m_bind_addr, m_query_port ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to bind on " << m_bind_addr
		  << ":" << m_query_port );
		LOG ( log::URGENT, "already in use?" );
		return false;
	}
	if ( ! m_query_channel->listen ( MAX_TELNETS ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to listen to query port" );
		return false;
	}
	m_reinit_query = false;
	return true;
} // FGLS::init_query_channel()

//////////////////////////////////////////////////////////////////////
/** Initialise the admin channel
 * @return true OK
 * @return false something went wrong
 */
bool
FGLS::init_admin_channel
()
{
	if ( ( ! m_reinit_admin ) || ( ! m_add_cli ) )
		return true;
	if ( m_admin_channel )
	{
		delete m_admin_channel;
	}
	m_admin_channel = 0;
	if ( m_admin_cli == false )
		return true;
	if ( m_admin_port == 0 )
	{
		m_reinit_admin = false;
		return true; // query channel disabled
	}
	m_admin_channel = new fgmp::netsocket ();
	if ( m_admin_channel->open ( fgmp::netaddr::IPv6, fgmp::netsocket::TCP ) == 0 )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to create Admin socket" );
		return false;
	}
	m_admin_channel->set_blocking ( false );
	m_admin_channel->set_sock_opt ( SO_REUSEADDR, true );
	if ( ! m_admin_channel->bind ( m_bind_addr, m_admin_port ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to bind on " << m_bind_addr
		  << ":" << m_admin_port );
		LOG ( log::URGENT, "already in use?" );
		return false;
	}
	if ( ! m_admin_channel->listen ( MAX_TELNETS ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to listen to query port" );
		return false;
	}
	m_reinit_admin = false;
	return true;
} // FGLS::init_admin_channel()

//////////////////////////////////////////////////////////////////////

void
FGLS::open_logfile
()
{
	if ( m_reinit_log == false )
		return;	// nothing to do
	if ( m_logfile_name == "" )
	{
		return;
	}
	LOG ( log::CONSOLE, "# using logfile " << m_logfile_name );
	if ( ! logger.open ( m_logfile_name ) )
	{
		LOG ( log::URGENT,
		  "FGLS::open_logfile(): "
		  << "Failed to open log file " << m_logfile_name );
	}
	m_reinit_log = false;
} // FGLS::open_logfile ()

//////////////////////////////////////////////////////////////////////

void
FGLS::shutdown
()
{
	if ( ! m_is_parent )
		return;
	LOG ( log::URGENT, "FGLS::Shutdown() - exiting" );
	logger.close ();
	if ( m_data_channel )
	{
		m_data_channel->close ();
		delete m_data_channel;
		m_data_channel = 0;
	}
	if ( m_query_channel )
	{
		m_query_channel->close ();
		delete m_query_channel;
		m_query_channel = 0;
	}
	if ( m_admin_channel )
	{
		m_admin_channel->close ();
		delete m_admin_channel;
		m_admin_channel = 0;
	}
} // FGLS::shutdown ()

//////////////////////////////////////////////////////////////////////

/** Read a config file
 *
 * Reads in @c config_name and sets internal variables accordingly
 * @param config_name	Path to config file to read
 * @return true		OK
 * @return false	An error occurred
 */
bool
FGLS::process_config
(
	const std::string & config_name
)
{
	FG_CONFIG	Config;
	std::string	Val;
	int		E;

	if ( m_have_config )	// we already have a config, so ignore
	{
		return true;
	}
	if ( Config.Read ( config_name ) )
	{
	LOG ( log::ERROR, "failed to read " << config_name );
		return false;
	}
	LOG ( log::ERROR, "processing " << config_name );
	Val = Config.Get ( "fgls.name" );
	if ( Val != "" )
	{
		m_server_name = Val;
	}
	Val = Config.Get ( "fgls.bind_address" );
	if ( Val != "" )
	{
		m_bind_addr = Val;
	}
	Val = Config.Get ( "fgls.port" );
	if ( Val != "" )
	{
		m_data_port = StrToNum<uint16_t> ( Val, E );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for fgls.port: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "fgls.telnet" );
	if ( Val != "" )
	{
		m_query_port = StrToNum<uint16_t> ( Val, E );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for fgls.telnet: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "fgls.admin_cli" );
	if ( Val != "" )
	{
		if ( ( Val == "on" ) || ( Val == "true" ) )
		{
			m_admin_cli = true;
		}
		else if ( ( Val == "off" ) || ( Val == "false" ) )
		{
			m_admin_cli = false;
		}
		else
		{
			LOG ( log::URGENT,
			  "unknown value for fgls.admin_cli '"
			  << Val << "'"
			);
		}
	}
	Val = Config.Get ( "fgls.admin_port" );
	if ( Val != "" )
	{
		m_admin_port = StrToNum<uint16_t> ( Val, E );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for fgls.admin_port: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "fgls.admin_user" );
	if ( Val != "" )
	{
		m_admin_user = Val;
	}
	Val = Config.Get ( "fgls.admin_pass" );
	if ( Val != "" )
	{
		m_admin_pass = Val;
	}
	Val = Config.Get ( "fgls.admin_enable" );
	if ( Val != "" )
	{
		m_admin_enable = Val;
	}
	Val = Config.Get ( "fgls.daemon" );
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
			  "unknown value for fgls.daemon '"
			  << Val << "'"
			);
		}
	}
	Val = Config.Get ( "fgls.logfile" );
	if ( Val != "" )
	{
		m_logfile_name = Val;
	}
	Val = Config.Get ( "fgls.checkinterval" );
	if ( Val != "" )
	{
		m_check_interval = StrToNum<uint16_t> ( Val, E );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for fgls.checkinterval: '"
			  << Val << "'"
			);
			exit ( 1 );
		}
	}
	m_have_config = true;
	return true;
} // FGLS::process_config ()

//////////////////////////////////////////////////////////////////////

void*
FGLS::detach_admin_cli
(
	void* ctx
)
{
	st_telnet* t = reinterpret_cast<st_telnet*> ( ctx );
	FGLS* tmp_fgls = t->instance;
	pthread_detach ( pthread_self() );
	tmp_fgls->handle_admin ( t->fd );
	delete t;
	return 0;
}

//////////////////////////////////////////////////////////////////////

void*
FGLS::handle_admin
(
	int fd
)
{
	FGLS_CLI* cli;

	errno = 0;
	cli = new FGLS_CLI ( this, fd );
	cli->loop ();
	if ( fd == 0 )
	{	// reading from stdin
		m_want_exit = true;
	}
	delete cli;
	return 0;
} // FGLS::handle_admin()

//////////////////////////////////////////////////////////////////////

/** Try to read the config file from several well known places
 *
 * @param reinit	true when reinitialising (restart)
 * @return true		OK
 * @return false	on error
 */
bool
FGLS::read_configs
(
	bool reinit
)
{
	std::string Path;

#ifndef _MSC_VER
	// Unix version
	Path  = SYSCONFDIR;
	Path += "/fgls.conf";
	if ( process_config ( Path ) == true )
		return true;
	// failed, try to find config in home dir
	Path = getenv ( "HOME" );
#else
	// windows version
	char* cp = getenv ( "HOME" );
	if ( cp )
	{
		Path = cp;
	}
	else
	{
		// XP=C:\Documents and Settings\<name>,
		// Win7=C:\Users\<user>
		cp = getenv ( "USERPROFILE" );
		if ( cp )
		{
			Path = cp;
		}
	}
#endif
	if ( Path != "" )
	{
		Path += "/fgls.conf";
		if ( process_config ( Path ) == true )
			return true;
	}
	// failed, try current directory
	if ( process_config ( "fgls.conf" ) == true )
		return true;
	LOG ( log::ERROR,
	  "Could not find a config file => using defaults");
	return true;
} // FGLS::read_configs ()

//////////////////////////////////////////////////////////////////////


/**
 * @brief Print a help screen for command line parameters
 */
void
FGLS::print_version
()
{
	std::cout << "\n";
	std::cout << "fgls version " << m_version
		  << ", compiled on " << __DATE__
		  << " at " << __TIME__ << std::endl;
	std::cout << "\n";
} // FGLS::print_version()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Print a help screen for command line parameters
 */
void
FGLS::print_help
()
{
	print_version ();
	std::cout <<
		"syntax: " << m_argv[0] << " [options]\n"
		"\n"
		"options are:\n"
		"-h          print this help screen\n"
		"-p PORT     listen port for data\n"
		"-a PORT     listen port for admin CLI\n"
		"-t PORT     listen port for telnet queries\n"
		"-l LOGFILE  log to LOGFILE\n"
		"-L LEVEL    verbosity of log in range 0 (few) to 3 (much)\n"
		"            5 to disable\n"
		"-d          do not run in backgound (overwrite config file)\n"
		"-D          do run in backgound (overwrite config file)\n"
		"            default is: run in background\n"
		"-v          print version and exit\n"
		"\n";
} // FGLS::print_help()

//////////////////////////////////////////////////////////////////////

/**
 * @bried Parse commandline parameters
 * @param argc	number of commandline arguments
 * @param argv	list of parameters
 */
int
FGLS::parse_params
(
	int argc,
	char* argv[]
)
{
	int	m;
	int	e;

	m_argc = argc;
	m_argv = argv;
	while ( ( m=getopt ( argc, argv, "hp:a:t:l:L:dDv" ) ) != -1 )
	{
		switch (m)
		{
		case 'h':
			print_help ();
			exit ( 0);
		case 'p':
			m_data_port = StrToNum<uint16_t> ( optarg, e );
			if ( e != 0 )
			{
				std::cerr << "invalid value for query port "
				  "'" << optarg << "' " << e  << std::endl;
				exit (1);
			}
			break;
		case 'a':
			m_admin_port = StrToNum<uint16_t> ( optarg, e );
			if ( e )
			{
				std::cerr << "invalid value for admin port "
				  "'" << optarg << "'" << std::endl;
				exit (1);
			}
			break;
		case 't':
			m_query_port = StrToNum<uint16_t> ( optarg, e );
			if ( e )
			{
				std::cerr << "invalid value for query port "
				  "'" << optarg << "'" << std::endl;
				exit (1);
			}
			break;
		case 'l':
			m_logfile_name = optarg;
			break;
		case 'L':
			m_debug_level = StrToNum<uint16_t> ( optarg, e );
			if ( e )
			{
				std::cerr << "invalid value for LEVEL "
				  "'" << optarg << "'" << std::endl;
				exit (1);
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
			exit ( 0);
		}
	} // while()
	return 0;
} // FGLS::parse_params()

//////////////////////////////////////////////////////////////////////

int
main
(
	int   argc,
	char* argv[]
)
{
	FGLS fgls;

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
