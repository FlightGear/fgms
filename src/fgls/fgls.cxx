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
#include <fglib/fg_config.hxx>
#include "fgls_cli.hxx"
#include "fgls.hxx"


#ifdef _MSC_VER
        #define M_IS_DIR _S_IFDIR
#else // !_MSC_VER
        #define M_IS_DIR S_IFDIR
        /** @brief An instance of daemon */
        fgmp::daemon Myself;
#endif

const fgmp::version fgls::m_version ( 0, 0, 1, "-dev" );

using prio = fgmp::fglog::prio;

//////////////////////////////////////////////////////////////////////

server::server
()
{
        id              = ( size_t ) -1;
        type            = fgms::eCLIENTTYPES::UNSET;
        //name  
        //location
        //admin
        //version
        last_seen       = 0;
        registered_at   = 0;

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
} // operator << ( server )

//////////////////////////////////////////////////////////////////////

fgls::fgls
()
{
        m_run_as_daemon = false;
        m_add_cli       = true;
        m_bind_addr     = "";
        // the main data channel
        m_data_port     = 5004;
        m_reinit_data   = true;
        m_data_channel  = 0;
        // the query channel
        m_query_port    = m_data_port + 1;
        m_reinit_query  = true;
        m_query_channel = 0;
        // the admin channel
        m_admin_port    = m_data_port + 2;
        m_reinit_admin  = true;
        m_admin_channel = 0;
        m_admin_cli     = true;
        // logging
        m_logfile_name  = "";
        m_reinit_log    = true;
        m_debug_level   = prio::MEDIUM;
        // general
        m_is_parent     = false;
        m_check_interval = 5;
        m_have_config   = false;
        m_server_name   = "fgls";
        m_argc          = 0;
        m_want_exit     = false;
        m_uptime        = time ( 0 );
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
        {
                open_logfile ();
        }
        logger.priority ( prio::MEDIUM );
        logger.set_flags ( fgmp::fglog::flags::WITH_DATE );
#ifndef _MSC_VER
        if ( m_run_as_daemon )
        {
                Myself.daemonize ();
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
} // fgls::init ()

//////////////////////////////////////////////////////////////////////

void
fgls::loop
()
{
        time_t  current_time;
        int     bytes;
        fgmp::netsocket* listener [3 + MAX_TELNETS];

        if ( m_data_channel == 0 )
        {
                LOG ( prio::EMIT, "fgls::loop() - "
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
                        LOG ( prio::EMIT,
                          "# Admin port disabled, "
                          "please set user and password"
                        );
                }
        }
        if ( ! m_run_as_daemon && m_add_cli )
        {
                // Run admin cli in foreground reading from stdin
                using namespace std::placeholders;
                std::thread th { std::bind (&fgls::handle_admin, this, _1), 0 };
                th.detach ();
        }
        LOG ( prio::EMIT, "# Main server started!" );
        m_is_parent = true;
        while ( m_want_exit == false )
        {
                if ( m_data_channel == 0 )
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
                bytes = m_data_channel->select( listener, 0, m_check_interval );
                if ( bytes < 0 )
                        continue;       // some error
                else if ( bytes == 0 )
                        continue;
                if ( listener[0] > 0 )
                {       // something on the data channel
                        //bytes = m_data_channel->RecvFrom();
                }
                if ( listener[1] > 0 )
                {       // something on the query channel
                        //bytes = m_data_channel->RecvFrom();
                }
                if ( listener[0] > 0 )
                {       // something on the admin channel
                        fgmp::netaddr admin_addr ( fgmp::netaddr::IPv6 );
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
                        std::thread th { std::bind (&fgls::handle_admin, this, _1), 0 };
                        th.detach ();
                }
        }
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
        {
                return true;
        }
        if ( m_data_channel )
        {
                delete m_data_channel;
                m_data_channel = 0;
        }
        m_data_channel = new fgmp::netsocket ();
        try
        {
                m_data_channel->listen_to ( m_bind_addr, m_data_port,
                  fgmp::netsocket::UDP );
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
        if ( m_query_channel )
        {
                delete m_query_channel;
        }
        m_query_channel = 0;
        m_reinit_query = false;
        if ( m_query_port == 0 )
        {
                return true; // query channel disabled
        }
        m_query_channel = new fgmp::netsocket ();
        try
        {
                m_query_channel->listen_to ( m_bind_addr,
                  m_query_port, fgmp::netsocket::TCP );
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
        if ( ! m_reinit_admin )
                return true;
        if ( m_admin_channel )
        {
                delete m_admin_channel;
        }
        m_admin_channel = 0;
        m_reinit_admin = false;
        if  ( ( m_admin_port == 0 ) || ( m_admin_cli == false ) )
        {
                return true; // admin channel disabled
        }
        m_admin_channel = new fgmp::netsocket ();
        try
        {
                m_admin_channel->listen_to ( m_bind_addr,
                  m_admin_port, fgmp::netsocket::TCP );
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
fgls::open_logfile
()
{
        if ( m_reinit_log == false )
                return; // nothing to do
        if ( m_logfile_name == "" )
        {
                return;
        }
        LOG ( prio::CONSOLE, "# using logfile " << m_logfile_name );
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
        const std::string & config_name
)
{
        fgmp::config  config;
        std::string     val;
        int             e;

        if ( m_have_config )    // we already have a config, so ignore
        {
                return true;
        }
        if ( config.read ( config_name ) )
        {
        LOG ( prio::EMIT, "failed to read " << config_name );
                return false;
        }
        LOG ( prio::EMIT, "processing " << config_name );
        val = config.get ( "fgls.name" );
        if ( val != "" )
        {
                m_server_name = val;
        }
        val = config.get ( "fgls.bind_address" );
        if ( val != "" )
        {
                m_bind_addr = val;
        }
        val = config.get ( "fgls.port" );
        if ( val != "" )
        {
                m_data_port = fgmp::str_to_num<uint16_t> ( val, e );
                if ( e )
                {
                        LOG ( prio::URGENT,
                          "invalid value for fgls.port: '" << val << "'"
                        );
                        exit ( 1 );
                }
        }
        val = config.get ( "fgls.telnet" );
        if ( val != "" )
        {
                m_query_port = fgmp::str_to_num<uint16_t> ( val, e );
                if ( e )
                {
                        LOG ( prio::URGENT,
                          "invalid value for fgls.telnet: '" << val << "'"
                        );
                        exit ( 1 );
                }
        }
        val = config.get ( "fgls.admin_cli" );
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
                          "unknown value for fgls.admin_cli '"
                          << val << "'"
                        );
                }
        }
        val = config.get ( "fgls.admin_port" );
        if ( val != "" )
        {
                m_admin_port = fgmp::str_to_num<uint16_t> ( val, e );
                if ( e )
                {
                        LOG ( prio::URGENT,
                          "invalid value for fgls.admin_port: '" << val << "'"
                        );
                        exit ( 1 );
                }
        }
        val = config.get ( "fgls.admin_user" );
        if ( val != "" )
        {
                m_admin_user = val;
        }
        val = config.get ( "fgls.admin_pass" );
        if ( val != "" )
        {
                m_admin_pass = val;
        }
        val = config.get ( "fgls.admin_enable" );
        if ( val != "" )
        {
                m_admin_enable = val;
        }
        val = config.get ( "fgls.daemon" );
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
                          "unknown value for fgls.daemon '"
                          << val << "'"
                        );
                }
        }
        val = config.get ( "fgls.logfile" );
        if ( val != "" )
        {
                m_logfile_name = val;
        }
        val = config.get ( "fgls.checkinterval" );
        if ( val != "" )
        {
                m_check_interval = fgmp::str_to_num<uint16_t> ( val, e );
                if ( e )
                {
                        LOG ( prio::URGENT,
                          "invalid value for fgls.checkinterval: '"
                          << val << "'"
                        );
                        exit ( 1 );
                }
        }
        m_have_config = true;
        return true;
} // fgls::process_config ()

//////////////////////////////////////////////////////////////////////

void*
fgls::detach_admin_cli
(
        void* ctx
)
{
        st_telnet* t = reinterpret_cast<st_telnet*> ( ctx );
        fgls* tmp_fgls = t->instance;
        pthread_detach ( pthread_self() );
        tmp_fgls->handle_admin ( t->fd );
        delete t;
        return 0;
}

//////////////////////////////////////////////////////////////////////

void*
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
        {       // reading from stdin
                m_want_exit = true;
        }
        delete cli;
        return 0;
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
        LOG ( prio::EMIT,
          "Could not find a config file => using defaults");
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
int
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
        while ( ( m=getopt ( argc, argv, "hp:a:t:l:L:dDv" ) ) != -1 )
        {
                switch (m)
                {
                case 'h':
                        print_help ();
                        exit ( 0);
                case 'p':
                        m_data_port = fgmp::str_to_num<uint16_t> ( optarg, e );
                        if ( e != 0 )
                        {
                                std::cerr << "invalid value for query port "
                                  "'" << optarg << "' " << e  << std::endl;
                                exit (1);
                        }
                        break;
                case 'a':
                        m_admin_port = fgmp::str_to_num<uint16_t> ( optarg, e );
                        if ( e )
                        {
                                std::cerr << "invalid value for admin port "
                                  "'" << optarg << "'" << std::endl;
                                exit (1);
                        }
                        break;
                case 't':
                        m_query_port = fgmp::str_to_num<uint16_t> ( optarg, e );
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
                        m_debug_level = static_cast<prio> (fgmp::str_to_num<uint16_t> ( optarg, e ));
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
} // fgls::parse_params()

//////////////////////////////////////////////////////////////////////

int
main
(
        int   argc,
        char* argv[]
)
{
        fgls fgls;

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


