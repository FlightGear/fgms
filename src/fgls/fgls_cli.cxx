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
 * @file fgls_cli.cxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        07/2017
 */

#include <sstream>
#include <fglib/fg_util.hxx>
#include "fgls_cli.hxx"

namespace libcli
{

/// new configure modes
namespace CLI_MODE
{
	enum
	{
		CONFIG_FGLS = CLI_MODE::EXTENSION,
		CONFIG_CLI
	};
}

}

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

fgls_cli::fgls_cli
(
	fgmp::fgls*   fgls,
	int     fd
) : cli { fd }, m_fgls { fgls }
{
	setup ();
} // fgls_cli::fgls_cli ()

//////////////////////////////////////////////////////////////////////

/** Setup all commands
 */
void
fgls_cli::setup
()
{
	using namespace libcli;
	command* c;
	command* c2;
	set_hostname ( m_fgls->m_hostname );
	std::stringstream banner;
	banner << "\r\n"
	       << "------------------------------------------------\r\n"
	       << "FlightGear List Server cli\r\n"
	       << "This is " << m_fgls->m_hostname << "\r\n"
	       << "------------------------------------------------\r\n";
	set_banner ( banner.str() );
	using namespace std::placeholders;
	#define _ptr(X) (std::bind (& X, this, _1, _2, _3))
	//////////////////////////////////////////////////
	//
	// show commands
	//
	//////////////////////////////////////////////////
	c = new command (
		"show",
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::EXEC,
		"show system information"
	);
	register_command ( c );
	//////////////////////////////////////////////////
	// show subcommands
	//////////////////////////////////////////////////
	// 'show log'
	register_command ( new command (
		"log",
		_ptr ( fgls_cli::show_log ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show log buffer"
	), c );
	// 'show settings'
	register_command ( new command (
		"settings",
		_ptr ( fgls_cli::show_settings ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show general settings"
	), c );
	// 'show relay'
	register_command ( new command (
		"relays",
		_ptr ( fgls_cli::show_relay ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show lsit of known relays"
	), c );

	//////////////////////////////////////////////////
	// 'show fgls' subcommands
	//////////////////////////////////////////////////
	c2 = new command (
		"fgls",
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::EXEC,
		"show fgls information"
	);
	register_command ( c2, c );
	// 'show daemon'
	register_command ( new command (
		"daemon",
		_ptr ( fgls_cli::show_daemon ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show if running as a daemon"
	), c2 );
	// 'show fgls bind_addr'
	register_command ( new command (
		"bind_addr",
		_ptr ( fgls_cli::show_bind_addr ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show bind address"
	), c2 );
	// 'show fgls data_port'
	register_command ( new command (
		"data_port",
		_ptr ( fgls_cli::show_data_port ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show data port"
	), c2 );
	// 'show fgls query_port'
	register_command ( new command (
		"query_port",
		_ptr ( fgls_cli::show_query_port ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show query port"
	), c2 );
	// 'show fgls logfile'
	register_command ( new command (
		"logfile",
		_ptr ( fgls_cli::show_logfile_name ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show logfile name"
	), c2 );
	// 'show fgls debug'
	register_command ( new command (
		"debug",
		_ptr ( fgls_cli::show_debug_level ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show debug level"
	), c2 );
	// 'show fgls check_interval'
	register_command ( new command (
		"check_interval",
		_ptr ( fgls_cli::show_check_interval ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show check interval"
	), c2 );
	// 'show fgls hostname'
	register_command ( new command (
		"hostname",
		_ptr ( fgls_cli::show_hostname ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show server name"
	), c2 );
	// 'show fgls version'
	register_command ( new command (
		"version",
		_ptr ( fgls_cli::show_version ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show running version information"
	), c2 );
	// 'show fgls uptime'
	register_command ( new command (
		"uptime",
		_ptr ( fgls_cli::show_uptime ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show uptime information"
	), c2 );

	//////////////////////////////////////////////////
	// 'show cli' subcommands
	//////////////////////////////////////////////////
	// 'show cli'
	c2 = new command (
		"cli",
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::EXEC,
		"show cli information"
	);
	register_command ( c2, c );
	register_command ( new command (
		"port",
		_ptr ( fgls_cli::show_cli_port ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show admin port"
	), c2 );
	// 'show cli users'
	register_command ( new command (
		"users",
		_ptr ( cli::internal_show_users ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Show list of allowed cli users"
	), c2 );

	//////////////////////////////////////////////////
	//
	// configure commands
	//
	//////////////////////////////////////////////////
	// 'configure'
	c = new command (
		"configure",
		_ptr ( cli::internal_configure ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::EXEC,
		"Enter configuration mode"
	);
	register_command ( c );
	//////////////////////////////////////////////////
	// configure subcommands
	//////////////////////////////////////////////////
	// 'configure fgls' in EXEC mode
	register_command ( new command (
		"fgls",
		_ptr ( fgls_cli::cfg_fgls ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::EXEC,
		"configure fgls properties"
	), c );
	// 'configure cli' in EXEC mode
	register_command ( new command (
		"cli",
		_ptr ( fgls_cli::cfg_cli ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::EXEC,
		"configure cli properties"
	), c );

	//////////////////////////////////////////////////
	// config commands in CONFIG mode
	//////////////////////////////////////////////////
	// 'fgls'
	register_command ( new command (
		"fgls",
		_ptr ( fgls_cli::cfg_fgls ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG,
		"configure fgls properties"
	) );
	// 'cli'
	register_command ( new command (
		"cli",
		_ptr ( fgls_cli::cfg_cli ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG,
		"configure cli properties"
	) );
	// enable 'exit' command
	register_command ( new command (
		"exit",
		_ptr ( cli::internal_exit ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG,
		"return to EXEC mode"
	) );
	// enable 'end' command
	register_command ( new command (
		"end",
		_ptr ( cli::internal_end ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG,
		"return to previous mode"
	) );

	//////////////////////////////////////////////////
	// config commands in CONFIG_FGLS mode
	//////////////////////////////////////////////////
	// 'fgls daemon'
	register_command ( new command (
		"daemon",
		_ptr ( fgls_cli::cfg_daemon ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"en-/disable daemon mode (windows only)"
	) );
	// 'fgls bind_addr'
	register_command ( new command (
		"bind_address",
		_ptr ( fgls_cli::cfg_bind_addr ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"Set bind address (only listen to this address)"
	) );
	// 'fgls data_port'
	register_command ( new command (
		"data_port",
		_ptr ( fgls_cli::cfg_data_port ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"Set data port"
	) );
	// 'fgls query_port'
	register_command ( new command (
		"query_port",
		_ptr ( fgls_cli::cfg_query_port ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"Set query port"
	) );
	// 'fgls logfile'
	register_command ( new command (
		"logfile",
		_ptr ( fgls_cli::cfg_logfile_name ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"Set logfile name"
	) );
	// 'fgls debug_level'
	register_command ( new command (
		"debug_level",
		_ptr ( fgls_cli::cfg_debug_level ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"Set debug level"
	) );
	// 'fgls check_interval'
	register_command ( new command (
		"check_interval",
		_ptr ( fgls_cli::cfg_check_interval ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"Set check interval "
	) );
	// 'fgls hostname'
	register_command ( new command (
		"hostname",
		_ptr ( fgls_cli::cfg_hostname ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"Set server name"
	) );
	// enable 'exit' command in configure fgls mode
	register_command ( new command (
		"exit",
		_ptr ( cli::internal_exit ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"return to EXEC mode"
	) );
	// enable 'end' command in configure fgls mode
	register_command ( new command (
		"end",
		_ptr ( cli::internal_end ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::CONFIG_FGLS,
		"return to previous mode"
	) );

	//////////////////////////////////////////////////
	// config commands in CONFIG_CLI mode
	//////////////////////////////////////////////////
	// 'enable true|false'
	register_command ( new command (
		"enable",
		_ptr ( fgls_cli::cfg_cli_enable ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_CLI,
		"enable/disable admin cli"
	) );
	// 'port PORT'
	register_command ( new command (
		"port",
		_ptr ( fgls_cli::cfg_cli_port ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_CLI,
		"set port of admin cli"
	) );
	// 'user NAME PASSWORD LEVEL'
	register_command ( new command (
		"user",
		_ptr ( fgls_cli::internal_add_user ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_CLI,
		"add cli users"
	) );
	// 'delete user NAME'
	c = new command (
		"delete",
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_CLI,
		"add cli users"
	);
	register_command ( c );
	register_command ( new command (
		"user",
		_ptr ( fgls_cli::internal_del_user ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::CONFIG_CLI,
		"delete cli users"
	), c );
	// enable 'exit' command in configure cli mode
	register_command ( new command (
		"exit",
		_ptr ( cli::internal_exit ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::CONFIG_CLI,
		"return to EXEC mode"
	) );
	// enable 'end' command in configure cli mode
	register_command ( new command (
		"end",
		_ptr ( cli::internal_end ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::CONFIG_CLI,
		"return to previous mode"
	) );

	//////////////////////////////////////////////////
	// general commands
	//////////////////////////////////////////////////
	register_command ( new command (
		"die",
		_ptr ( fgls_cli::cmd_die ),
		PRIVLEVEL::PRIVILEGED,
		CLI_MODE::EXEC,
		"force fgls to exit"
	) );
	#undef _ptr
} // fgls_cli::setup()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_fgls
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r { no_more_args ( args, first_arg ) };
	if ( RESULT::OK != r )
		return r;
	set_configmode ( libcli::CLI_MODE::CONFIG_FGLS, "fgls" );
	return RESULT::OK;
} // fgls_cli::cfg_fgls ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_daemon
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	std::pair <libcli::RESULT, bool> r { get_bool ( args[first_arg] ) };
	if ( r.first == RESULT::OK )
	{
		// if set from false to true a restart is needed
		m_fgls->m_run_as_daemon = r.second;
	}
	return r.first;
} // fgls_cli::cfg_daemon ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_bind_addr
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( wants_help ( args[first_arg] ) )
	{
		show_help ( "IP", "only listen on IP" );
		return RESULT::ERROR_ANY;
	}
	fgmp::netaddr address;
	address.assign ( args[first_arg]  );
	if ( ! address.is_valid () )
	{
		if ( compare ( "any", args[first_arg], m_compare_case ) )
		{
			m_fgls->set_bind_addr ( "" );
			return RESULT::OK;
		}
		m_client << command << " : ";
		return RESULT::INVALID_ARG;
	}
	m_fgls->set_bind_addr ( args[first_arg] );
	return RESULT::OK;
} // fgls_cli::cfg_bind_addr ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_data_port
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( wants_help ( args[first_arg] ) )
	{
		show_help ( "PORT", "set data port to PORT" );
		return RESULT::ERROR_ANY;
	}
	int e;
	int p { str_to_num<uint16_t> ( args[first_arg], e ) };
	if ( e )
		return RESULT::INVALID_ARG;
	if ( m_fgls->m_data_port == p )
		return RESULT::OK;
	m_fgls->m_data_port = p;
	m_fgls->m_reinit_data = true;
	m_fgls->init_data_channel ();
	return RESULT::OK;
}  // fgls_cli::cfg_data_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_query_port
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( wants_help ( args[first_arg] ) )
	{
		show_help ( "PORT", "set query port to PORT" );
		return RESULT::ERROR_ANY;
	}
	int e;
	int p { str_to_num<uint16_t> ( args[first_arg], e ) };
	if ( e )
		return RESULT::INVALID_ARG;
	if ( m_fgls->m_query_port == p )
		return RESULT::OK;
	m_fgls->m_query_port = p;
	m_fgls->m_reinit_query = true;
	m_fgls->init_query_channel ();
	return RESULT::OK;
} // fgls_cli::cfg_query_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_logfile_name
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( wants_help ( args[first_arg] ) )
	{
		show_help ( "NAME", "set logfile to NAME" );
		return RESULT::ERROR_ANY;
	}
	m_fgls->m_reinit_log = true;
	m_fgls->m_logfile_name = args[first_arg];
	m_fgls->open_logfile ();
	return RESULT::OK;
} // fgls_cli::cfg_logfile_name ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_debug_level
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( wants_help ( args[first_arg] ) )
	{
		show_help ( "LEVEL", "set debug level to LEVEL seconds" );
		return RESULT::ERROR_ANY;
	}
	int e;
	int p { str_to_num<uint16_t> ( args[first_arg], e ) };
	if ( e )
		return RESULT::INVALID_ARG;
	m_fgls->m_debug_level = fgmp::make_prio ( p );
	logger.priority ( m_fgls->m_debug_level );
	return RESULT::OK;
} // fgls_cli::cfg_debug_level ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_check_interval
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( wants_help ( args[first_arg] ) )
	{
		show_help ( "INTERVAL",
			    "set check interval to INTERVAL seconds" );
		return RESULT::ERROR_ANY;
	}
	int e;
	int p { str_to_num<uint16_t> ( args[first_arg], e ) };
	if ( e )
		return RESULT::INVALID_ARG;
	m_fgls->m_check_interval = p;
	return RESULT::OK;
} // fgls_cli::cfg_check_interval ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_hostname
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( wants_help ( args[first_arg] ) )
	{
		show_help ( "NAME", "set NAME as hostname" );
		return RESULT::ERROR_ANY;
	}
	m_fgls->m_hostname = args[first_arg];
	set_hostname ( m_fgls->m_hostname );
	return RESULT::OK;
} // fgls_cli::cfg_hostname

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_cli
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r { no_more_args ( args, first_arg ) };
	if ( RESULT::OK != r )
		return r;
	set_configmode ( libcli::CLI_MODE::CONFIG_CLI, "cli" );
	return RESULT::OK;
} // fgls_cli::cfg_cli ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_cli_enable
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	std::pair <libcli::RESULT, bool> r { get_bool ( args[first_arg] ) };
	if ( r.first == RESULT::OK )
		m_fgls->m_cli_enabled = r.second;
	return r.first;
} // fgls_cli::cfg_cli_enable ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::cfg_cli_port
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( wants_help ( args[first_arg] ) )
	{
		show_help ( "PORT", "set cli port to PORT" );
		return RESULT::ERROR_ANY;
	}
	int e;
	int p { str_to_num<uint16_t> ( args[first_arg], e ) };
	if ( e )
		return RESULT::INVALID_ARG;
	if ( m_fgls->m_cli_port == p )
		return RESULT::OK;
	m_fgls->m_query_port = p;
	m_fgls->m_reinit_query = true;
	m_fgls->init_query_channel ();
	return RESULT::OK;
} // fgls_cli::cfg_cli_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_daemon
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << libcli::align_left ( 22 )
		 << "I am daemon" << ": " << m_fgls->m_run_as_daemon
		 << libcli::cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_daemon ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_bind_addr
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	std::string bind_addr;
	if ( m_fgls->m_bind_addr == "" )
		bind_addr = "*";
	else
		bind_addr = m_fgls->m_bind_addr;
	m_client << libcli::align_left ( 22 )
		 << "bind address"<< ": " << bind_addr
		 << libcli::cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_bind_addr ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_data_port
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << libcli::align_left ( 22 )
		 << "data port" << ": " << m_fgls->m_data_port
		 << libcli::cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_data_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_query_port
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << libcli::align_left ( 22 )
		 << "query port" << ": " << m_fgls->m_query_port
		 << libcli::cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_query_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_cli_port
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << libcli::align_left ( 22 )
		 << "cli port" << ": " << m_fgls->m_cli_port
		 << libcli::cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_cli_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_logfile_name
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << libcli::align_left ( 22 )
		 << "logfile name" << ": " << m_fgls->m_logfile_name
		 << libcli::cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_logfile_name ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_debug_level
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << libcli::align_left ( 22 )
		 << "debug level" << ": " << ( int ) m_fgls->m_debug_level
		 << libcli::cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_debug_level

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_check_interval
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << libcli::align_left ( 22 )
		 << "check interval" << ": "  << m_fgls->m_check_interval
		 << libcli::cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_check_interval ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_hostname
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << libcli::align_left ( 22 )
		 << "server name" << ": " << m_fgls->m_hostname
		 << libcli::cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_hostname ()

//////////////////////////////////////////////////////////////////////

namespace
{

//
// a little helper to print a fgmp::SENDER_TYPE to a cli_client
//
libcli::cli_client&
operator <<
(
	libcli::cli_client& out,
	const fgmp::SENDER_TYPE t
)
{
	switch ( t )
	{
	case fgmp::SENDER_TYPE::UNSET:
		out << "unset";
		break;
	case fgmp::SENDER_TYPE::FGFS:
		out << "FGFS";
		break;
	case fgmp::SENDER_TYPE::FGMS:
		out << "FGMS";
		break;
	case fgmp::SENDER_TYPE::FGAS:
		out << "FGAS";
		break;
	case fgmp::SENDER_TYPE::FGLS:
		out << "FGLS";
		break;
	case fgmp::SENDER_TYPE::OBSERVER:
		out << "OBSERVER";
		break;
	}
	return out;
}

//
// a little helper to print a fgmp::CONFIG_TYPE to a cli_client
//
libcli::cli_client&
operator <<
(
	libcli::cli_client& out,
	const fgmp::CONFIG_TYPE t
)
{
	switch ( t )
	{
	case fgmp::CONFIG_TYPE::DYNAMIC:
		out << "dynamic";
		break;
	case fgmp::CONFIG_TYPE::STATIC:
		out << "static";
		break;
	}
	return out;
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgls_cli::show_relay
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT n { need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( wants_help ( args[first_arg] ) )
	{
		show_help ( "brief", "show brief listing" );
		show_help ( "id", "show entry with id" );
		show_help ( "IP", "show entry with IP-address" );
		show_help ( "NAME", "show user with NAME" );
		show_help ( "<cr>", "show log listing" );
		show_help ( "|", "output modifier" );
		return RESULT::OK;
	}
	bool brief { false };
	std::string name;
	fgmp::netaddr address;
	size_t id { 0 };
	if ( compare ( args[first_arg], "brief", m_compare_case ) )
		brief = true;
	else
	{
		int e;
		id = str_to_num<size_t> ( args[first_arg], e );
		if ( e )
		{
			id = 0;
			address.assign ( args[first_arg] );
			if ( ! address.is_valid() )
				name = args[first_arg];
		}
	}
	using namespace libcli;
	size_t entries_matched { 0 };
	m_fgls->m_server_list.lock ();
	for ( auto s : m_fgls->m_server_list )
	{
		m_fgls->m_server_list.unlock ();
		if ( ( 0 == id ) && ( address.is_valid () ) )
		{
			// show only entries that match the ip
			if ( s->addr != address )
				continue;
		}
		else if ( 0 != id )
		{
			// show only the entry with id
			if ( id != s->id )
				continue;
		}
		else if ( "" != name )
		{
			// show only entries which match name
			if ( ! compare ( s->name, name, m_compare_case ) )
				continue;
		}
		++entries_matched;
		m_client
			<< "id " << s->id
			<< ": "  << s->addr.to_string ()
			<< ": "  << s->addr.port ()
			<< " : " << s->name
			<< cli_client::endl;
		if ( brief )
			continue;
		m_client << "  sender    : "
			 << s->sender_type
			 << cli_client::endl;
		m_client << "  config    : "
			 << s->config_type
			 << cli_client::endl;
		m_client << "  location  : "
			 << s->location
			 << cli_client::endl;
		m_client << "  admin     : "
			 << s->admin_email
			 << cli_client::endl;
		m_client << "  version   : "
			 << s->version.str()
			 << cli_client::endl;
		m_client << "  entered   : "
			 << timestamp_to_datestr ( s->registered_at )
			 << cli_client::endl;
		m_client << "  last_seen : "
			 << timestamp_to_datestr ( s->last_seen )
			 << cli_client::endl;
		m_fgls->m_server_list.lock ();
	}
	m_fgls->m_server_list.unlock ();
	m_client << cli_client::endl;
	m_client << entries_matched << " entries found" << cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_rela ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show log buffer of the the server
 */
libcli::RESULT
fgls_cli::show_log
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	fgmp::str_list* buf = logger.logbuf();
	fgmp::str_it it;
	buf->lock ();
	m_client.enable_filters ();
	try
	{
		using namespace libcli;
		for ( it = buf->begin(); it != buf->end(); it++ )
			m_client << *it << cli_client::endl;
	}
	catch ( libcli::pager_wants_quit )
	{
		/* do nothing */
	}
	buf->unlock ();
	m_client.disable_filters ();
	return RESULT::OK;
} // fgls_cli::show_log()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show general settings
 */
libcli::RESULT
fgls_cli::show_settings
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	strvec noargs;
	m_client << libcli::cli_client::endl;
	show_version ( command, args, first_arg );
	m_client << libcli::cli_client::endl;
	m_client << "current settings:" << libcli::cli_client::endl;
	show_daemon         ( "", noargs, 0 );
	show_bind_addr      ( "", noargs, 0 );
	show_data_port      ( "", noargs, 0 );
	show_query_port     ( "", noargs, 0 );
	show_logfile_name   ( "", noargs, 0 );
	show_debug_level    ( "", noargs, 0 );
	show_check_interval ( "", noargs, 0 );
	show_hostname       ( "", noargs, 0 );
	return RESULT::OK;
} // fgls_cli::show_settings ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Shutdown the server
 */
libcli::RESULT
fgls_cli::cmd_die
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_fgls->m_want_exit = true;
	return libcli::RESULT::OK;
} // fgls_cli::cmd_die

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show the uptime of the the server
 *         in a human readable form.
 */
libcli::RESULT
fgls_cli::show_uptime
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	using namespace libcli;
	m_client << "UP since "
		<< fgmp::timestamp_to_datestr ( m_fgls->m_uptime )
		<< "(" << fgmp::timestamp_to_days ( m_fgls->m_uptime ) << ")"
		<< cli_client::endl;
	return RESULT::OK;
} // fgls_cli::show_uptime

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show the version number of the the server
 */
libcli::RESULT
fgls_cli::show_version
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	libcli::RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	using namespace libcli;
	m_client << "This is " << m_fgls->m_hostname << cli_client::endl;
	m_client << "FlightGear List Server version "
		 << m_fgls->m_version.str() << cli_client::endl;
	m_client << "compiled on " << __DATE__ << " at "
		 << __TIME__  << cli_client::endl;
	m_client << "using protocol version v ! TODO !" << cli_client::endl;
	show_uptime ( command, args, first_arg );
	return RESULT::OK;
} // fgls_cli::show_version

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

