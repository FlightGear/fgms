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
// along with this program; if not, see http://www.gnu.org/licenses/
//
// derived from libcli by David Parrish (david@dparrish.com)
// Copyright (C) 2011-2021  Oliver Schroeder
//
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <exception>
#include <algorithm>
#include <fstream>
#include <exception>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <server/fg_util.hxx>
#include "libcli.hxx"

#if defined(_MSC_VER) || defined(__CYGWIN__)
	// some windows quick fixes
#ifndef __CYGWIN__	
#define CTRL(a)  ( a & 037 )
#endif	
#ifdef __cplusplus
extern "C" {
#endif
	extern char* crypt ( const char* key, const char* salt );
#ifdef __cplusplus
}
#endif
#endif

namespace
{
const size_t NOT_IN_HISTORY { -1UL };
}

namespace libcli
{

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new cli::cli object
 *
 * Initialise the cli, set default values and set up all internal commands.
 *
 * @param fd Either stdout or a socket-fd
 */
cli::cli
(
	int fd
)
	: m_connection { fd }
	, m_edit { &m_connection }
	, m_max_history { 256 }
	, m_in_history { NOT_IN_HISTORY }
{
	register_command ( command (
		this,
		"help",
		&cli::internal_help,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"Description of the interactive help system"
	) );
	register_command ( command (
		this,
		"whoami",
		&cli::internal_whoami,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::STANDARD,
		"Show who you are"
	) );
	register_command ( command (
		this,
		"quit",
		&cli::internal_quit,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::STANDARD,
		"Disconnect"
	) );
	register_command ( command (
		this,
		"configure",
		&cli::internal_configure,
		PRIVLEVEL::PRIVILEGED,
		MODE::STANDARD,
		"Enter configuration mode"
	) );
	register_command ( command (
		this,
		"exit",
		&cli::internal_exit,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"Exit from current mode"
	) );
	register_command ( command (
		this,
		"pager",
		&cli::internal_pager,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::CONFIGURE,
		"Set number of lines printed on a screen without a more prompt"
	) );
	register_command ( command (
		this,
		"hostname",
		&cli::internal_hostname,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::CONFIGURE,
		"Set the name of this host"
	) );
	register_command ( command (
		this,
		"history",
		&cli::internal_history,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"Show a list of previously run commands"
	) );
	register_command ( command (
		this,
		"set-history",	// FIXME: needs better name
		&cli::internal_set_history,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::CONFIGURE,
		"Set number of lines stored in history"
	) );
	register_command ( command (
		this,
		"!",
		&cli::internal_invoke_history,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"invoke previously run commands"
	) );
	register_command ( command (
		this,
		"enable",
		&cli::internal_enable,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::STANDARD,
		"Turn on privileged commands"
	) );
	register_command ( command (
		this,
		"disable",
		&cli::internal_disable,
		PRIVLEVEL::PRIVILEGED,
		MODE::STANDARD,
		"Turn off privileged commands"
	) );

	register_filter ( command (
		this,
		"grab",
		&cli::filter_grab,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"only print lines which match <WORD>"
	) );
	register_filter ( command (
		this,
		"exclude",
		&cli::filter_exclude,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"only print lines which do not match <WORD>"
	) );
	register_filter ( command (
		this,
		"begin",
		&cli::filter_begin,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"start output with a line containing <WORD>"
	) );
	register_filter ( command (
		this,
		"between",
		&cli::filter_between,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"start output with a line containing <START> and stop output with a line containing <END>"
	) );
	register_filter ( command (
		this,
		"limit",
		&cli::filter_limit,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"print only <NUM> lines of output"
	) );
	register_filter ( command (
		this,
		"last",
		&cli::filter_last,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"print only the last <NUM> lines of output"
	) );
	register_filter ( command (
		this,
		"count",
		&cli::filter_count,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"count number of lines"
	) );
	register_filter ( command (
		this,
		"no-more",
		&cli::filter_nomore,
		PRIVLEVEL::UNPRIVILEGED,
		MODE::ANY,
		"temporarily disable the more prompt"
	) );
	build_shortest ( m_filters );
	m_mode = MODE::ANY;
	set_privilege ( PRIVLEVEL::UNPRIVILEGED );
	set_mode ( MODE::STANDARD, "" );
} // cli::CLI ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Destroy the cli::cli object
 *
 * delete internal lists.
 */
cli::~cli
()
{
	clear_active_filters ();
	m_users.clear ();
} // cli::~CLI()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Little helper function
 *
 * @param arg    The argument list to check.
 * @return true  If the last argument ends with a '?'
 * @return false In all other cases.
 */
bool
cli::arg_wants_help
(
	const std::string& arg
)
{
	if ( arg[arg.size () - 1] == '?' )
	{
		return true;
	}
	return false;
} // cli::arg_wants_help ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check if last arg is a '?'
 *
 * @param arg 		The argument to check
 * @return true		If arg matches exactly "?"
 * @return false 	User supplied an additional argument
 */
bool
cli::wants_help_last_arg
(
	const std::string& arg
)
{
	if ( arg == "?" )
	{
		return true;
	}
	return false;
} // cli::wants_help_last_arg ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Convinient function. Handle arguments when none are needed.
 *
 * @param args		The list of arguments.
 *
 * @return libcli::OK there were no arguments
 * @return libcli::ERROR_ANY if the only argument was a '?'
 * @return libcli::ERROR_ARG if there was any other argument
 */
RESULT
cli::have_unwanted_args
(
	const tokens& args
)
{
	int i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0:
			if ( a == "?" )
			{
				m_connection << "<cr>" << crlf;
				m_connection << "|" << crlf;
				return libcli::ERROR_ANY;
			}
		default:
			return libcli::TOO_MANY_ARGS;
		}
	}
	return libcli::OK;
} // cli::have_unwanted_args ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set a callback function.
 *
 * Usefull if you want to implement your own authentication scheme.
 *
 * This is for a C-like authentication function.
 *
 * @param callback The authentication function to call.
 */
void
cli::set_auth_callback
(
	std::function <int ( const std::string&, const std::string& )> callback
)
{
	m_auth_callback.c_func = callback;
} // cli::set_auth_callback ( c_func )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set a callback method.
 *
 * Usefull if you want to implement your own authentication scheme.
 *
 * This is for a class member authentication method.
 *
 * @param callback The authentication method to call.
 */
void
cli::set_auth_callback
(
	std::function <int ( cli&, const std::string&, const std::string& )> callback
)
{
	m_auth_callback.member = callback;
} // cli::set_auth_callback ( cpp_func )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set a callback function to authenticate enable-mode.
 *
 * Usefull to implement your own authentication scheme for the enable
 * mode.
 *
 * This is for a C-like authentication function.
 *
 * @param callback The authentication function to call.
 */
void
cli::set_enable_callback
(
	std::function <int ( const std::string& )> callback
)
{
	m_enable_callback.c_func = callback;
} // cli::set_enable_callback ( c_func )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set a callback method to authenticate enable-mode.
 *
 * Usefull to implement your own authentication scheme for the enable
 * mode.
 *
 * This is for a class member authentication method.
 *
 * @param callback The authentication method to call.
 */
void
cli::set_enable_callback
(
	std::function <int ( cli&, const std::string& )> callback
)
{
	m_enable_callback.member = callback;
} // cli::set_enable_func ( cpp_func )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set a regular callback function.
 *
 * The regular function is called once per second. Usefull if you need
 * to do something on a regular basis.
 *
 * This is for a C-like authentication function.
 *
 * @param callback
 */
void
cli::set_regular_callback
(
	std::function <int ()> callback
)
{
	m_edit.set_regular_callback ( callback );
} // cli::regular ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set a regular callback method.
 *
 * The regular function is called once per second. Usefull if you need
 * to do something on a regular basis.
 *
 * This is for a class member authentication method.
 *
 * @param callback
 */
void
cli::set_regular_callback
(
	std::function <int ( cli& )> callback
)
{
	m_edit.set_regular_callback ( callback );
} // cli::regular ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief	Allow a user to login to the cli
 *
 * @param username	The name of the user
 * @param password	The password of the user
 *
 * @todo	privilege level
 */
void
cli::allow_user
(
	const std::string& username,
	const std::string& password
)
{
	auto u = m_users.find ( username );
	if ( u != m_users.end () )
	{
		m_connection << unfiltered
			<< "user '" << username << "' already exists!" << crlf;
		return;
	}
	m_users[username] = password;
} // cli::allow_user ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set a password for the 'enable' command
 *
 * @param password The password to use
 *
 * @todo 	privilege level
 */
void
cli::allow_enable
(
	const std::string& password
)
{
	m_enable_password = password;
} // cli::allow_enable ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Delete a user from the allowed user list
 *
 * @param username The user to delete
 */
void
cli::deny_user
(
	const std::string& username
)
{
	if ( m_users.empty () )
	{
		return;
	}
	auto u = m_users.find ( username );
	if ( u == m_users.end () )
	{
		return;
	}
	m_users.erase ( u );
} // cli::deny_user ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set a banner.
 *
 * The banner is printed every time a user connects to the cli.
 *
 * @param banner The banner to be printed.
 */
void
cli::set_banner
(
	const std::string& banner
)
{
	m_banner = banner;
} // cli::set_banner ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set the hostname of this cli.
 *
 * The hostname is part of the prompt of this cli.
 *
 * @param hostname The hostname to be used.
 */
void
cli::set_hostname
(
	const std::string& hostname
)
{
	m_hostname = hostname;
} // cli::set_hostname ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief calculate the minimum unique length of a command
 *
 * @param commands list of commands
 *
 * Calculates the minimum unique length of a command name. Needed for
 * the tab expansion and the completion list.
 * Automatically called once in cli::loop() and every time the privilege
 * level or operation mode changes.
 * Keep in mind to call build_shortest() again if you later register new
 * commands.
 */
void
cli::build_shortest
(
	cmd_list& commands
)
{
	for ( auto& c : commands )
	{
		if ( ( c.m_privilege > m_privilege )
			|| ( ( c.m_mode != MODE::ANY ) && ( c.m_mode != m_mode ) ) )
		{	// user has no access to this command
			c.m_unique_len = c.m_name.length ();
			continue;
		}
		c.m_unique_len = 1;
		for ( auto& p : commands )
		{
			if ( c.m_name == p.m_name )
			{
				continue;
			}
			if ( ( p.m_mode != MODE::ANY ) && ( p.m_mode != m_mode ) )
			{
				continue;
			}
			if ( p.m_privilege > m_privilege )
			{
				continue;
			}
			size_t len = 1;
			auto cp = c.m_name.begin ();
			auto pp = p.m_name.begin ();
			while ( ( cp != c.m_name.end () ) && ( pp != p.m_name.end () ) )
			{
				if ( *cp != *pp )
				{
					break;
				}
				++cp;
				++pp;
				++len;
			}
			if ( len > c.m_unique_len )
			{
				c.m_unique_len = len;
			}
		}
		if ( c.m_children.size () )
		{
			build_shortest ( c.m_children );
		}
	}
} // cli::build_shortest ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Change the privilege level.
 *
 * @param priv The privilege level to change to.
 * @return int The previous privilege level.
 *
 * @see @ref libcli::PRIVLEVEL for predefined privileges.
 */
int
cli::set_privilege
(
	int priv
)
{
	int old = m_privilege;
	m_privilege = priv;
	if ( priv != old )
	{
		build_shortest ( m_commands );
	}
	return old;
} // cli::set_privilege ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set a description for the current mode.
 *
 * @param modestring The description to use.
 *
 * The \a modestring will be part of the prompt for this cli.
 */
void
cli::set_modestring
(
	const std::string& modestring
)
{
	m_modestring = modestring;
} // cli::set_modestring ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Change the operation mode.
 *
 * The \a desc desription will be part of the new prompt.
 *
 * @param mode The mode to change to.
 * @param desc A description of the mode.
 * @return int The old mode,
 *
 * @see @ref libcli::MODE for predefined modes.
 */
int
cli::set_mode
(
	int mode,
	const std::string& desc
)
{
	int old = m_mode;
	m_mode = mode;
	if ( mode != old )
	{
		m_modestring = "";
		if ( desc != "" )
		{
			m_modestring = "(" + desc + ")";
		}
		build_shortest ( m_commands );
	}
	return old;
} // cli::set_mode ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief register a command as a child of a command list
 *
 * @param parent	The parent command list
 * @param cmd		The command to register
 */
void
cli::register_command
(
	command& parent,
	const command& cmd
)
{
	if ( cmd.m_name == "" )
	{
		m_connection << "can not add empty command" << crlf;
		return;
	}
	for ( auto c : parent.m_children )
	{
		if ( ( cmd.m_name == c.m_name ) && ( cmd.m_mode == c.m_mode ) )
		{
			m_connection << "command '" << cmd.m_name << "' already known!" << crlf;
			return;
		}
	}
	parent.m_children.push_back ( cmd );
	return;
} // cli::register_command ( parent, cmd )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief register a command
 *
 * @param cmd The command to register
 */
void
cli::register_command
(
	const command& cmd
)
{
	if ( cmd.m_name == "" )
	{
		m_connection << "can not add empty command" << crlf;
		return;
	}
	for ( auto c : m_commands )
	{	// check if we already have a command with this name
		if ( ( cmd.m_name == c.m_name ) && ( cmd.m_mode == c.m_mode ) )
		{
			m_connection << "command '" << cmd.m_name << "' already known!" << crlf;
			return;
		}
	}
	m_commands.push_back ( cmd );
} // cli::register_command ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Delete a command
 *
 * @param name Name of the command to delete
 */
void
cli::unregister_command
(
	const std::string& name
)
{
	for ( auto c = m_commands.begin (); c != m_commands.end (); ++c )
	{
		if ( name == c->m_name )
		{
			m_commands.erase ( c );
			return;
		}
	}
	m_connection << "could not unregister '" << name << "'" << crlf;
	return;
} // cli::unregsiter_command ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief register a filter command
 *
 * @param filter_cmd The filter to register
 */
void
cli::register_filter
(
	const command& filter_cmd
)
{
	if ( filter_cmd.m_name == "" )
	{
		m_connection << "can not add empty filter command" << crlf;
		return;
	}
	for ( auto c : m_filters )
	{	// check if we already have a command with this name
		if ( filter_cmd.m_name == c.m_name )
		{
			m_connection << "filter '" << filter_cmd.m_name << "' already known!" << crlf;
			return;
		}
	}
	m_filters.push_back ( filter_cmd );
} // cli::register_filter ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Return a reference to the command \a name
 * 
 * @param name      The name of the command
 * @param commands  The list of defined commands
 * @return command& A refenrence to the command
 * 
 * Useful to modify an already registered command
 */
command&
cli::get_command
(
	const std::string& name,
	cmd_list& commands
)
{
	cmd_list::iterator c;
	for ( c = commands.begin(); c != commands.end(); ++c )
	{
		if ( name == c->m_name )
		{
			return *c;
		}		
	}
	std::string s = "'";
	s += name + "': ";
	s += "command not found";
	throw std::runtime_error ( s );
} // cli::get_command ()
//////////////////////////////////////////////////////////////////////

/** @name internal_commands
 *
 * These are the predefined commands, present in all incarnations of the
 * cli.
 */
 ///@{
 //////////////////////////////////////////////////////////////////////
 /**
  * @brief command 'enable', switch into @ref PRIVLEVEL::PRIVILEGED privilege level.
  *
  * @param name UNUSED
  * @param args UNUSED
  *
  * @return libcli::OK when everything went fine
  * @return libcli::ERROR_ARG if there was an argument
  * @return libcli::ERROR_ANY if the only argument was a '?'
  *
  * @fixme parameter 'level'
  * The only accepted argument is '?'
  */
RESULT
cli::internal_enable
(
	UNUSED ( const std::string& name ),
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	if ( PRIVLEVEL::PRIVILEGED == m_privilege )
	{
		return libcli::OK;
	}
	if ( ( "" == m_enable_password ) && ( nullptr == m_enable_callback.member ) )
	{
		/* no password required, set privilege immediately */
		set_privilege ( PRIVLEVEL::PRIVILEGED );
		set_mode ( MODE::STANDARD, "" );
	}
	else
	{
		/* require password entry */
		m_state = STATE::ENABLE_PASSWORD;
	}
	return libcli::OK;
} // cli::internal_enable ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief command 'disable', switch back to normal state.
 *
 * @param name UNUSED
 * @param args UNUSED
 *
 * @return libcli::OK when everything went fine
 * @return libcli::ERROR_ARG when there was an argument
 *
 * The only accepted argument is '?'
 */
RESULT
cli::internal_disable
(
	UNUSED ( const std::string& name ),
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return libcli::ERROR_ARG;
	}
	set_privilege ( PRIVLEVEL::UNPRIVILEGED );
	set_mode ( MODE::STANDARD, "" );
	return libcli::OK;
} // cli::internal_disable ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Switch to configure mode
 * 
 * @param name UNUSED
 * @param args UNSUSED
 *
 * @return libcli::OK when everything went fine
 * @return libcli::ERROR_ARG when there was an argument
 * @return libcli::ERROR_ANY if the only argument was a '?'
 */
RESULT
cli::internal_configure
(
	const std::string& name,
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	set_mode ( MODE::CONFIGURE, "configure" );
	return libcli::OK;
} // cli::internal_configure ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief command 'help', print a general help message.
 *
 * @param name UNUSED
 * @param args UNUSED
 *
 * @return libcli::OK when everything went fine
 * @return libcli::ERROR_ARG when there was an argument
 * @return libcli::ERROR_ANY if the only argument was a '?'
 *
 * The only accepted argument is '?'
 */
RESULT
cli::internal_help
(
	UNUSED ( const std::string& name ),
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	m_connection << crlf;
	m_connection <<
		"Help may be requested at any point in a command by entering\r\n"
		"a question mark '?'.  If nothing matches, the help list will\r\n"
		"be empty and you must backup until entering a '?' shows the\r\n"
		"available options.\r\n"
		"Two styles of help are provided:\r\n"
		"1. Full help is available when you are ready to enter a\r\n"
		"   command argument (e.g. 'show ?') and describes each possible\r\n"
		"   argument.\r\n"
		"2. Partial help is provided when an abbreviated argument is entered\r\n"
		"   and you want to know what arguments match the input\r\n"
		"   (e.g. 'show pr?'.)\r\n"
		<< crlf;
	return libcli::OK;
} // cli::internal_help
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief command 'whoami', print the username of the currently connected user.
 *
 * @param name UNUSED
 * @param args UNUSED
 *
 * @return libcli::OK when everything went fine
 * @return libcli::ERROR_ARG when there was an argument
 * @return libcli::ERROR_ANY if the only argument was a '?'
 *
 * The only valid argument is a '?', asking for help
 */
RESULT
cli::internal_whoami
(
	UNUSED ( const std::string& name ),
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	m_connection << "You are '" << m_username << "'" << crlf;
	return libcli::OK;
} // cli::internal_whoami ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief command 'history', print the command history.
 *
 * @param name UNUSED
 * @param args UNUSED
 *
 * @return libcli::OK when everything went fine
 * @return libcli::ERROR_ARG when there was an argument
 * @return libcli::ERROR_ANY if the only argument was a '?'
 *
 * The only valid argument is a '?', asking for help
 */
RESULT
cli::internal_history
(
	UNUSED ( const std::string& name ),
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	m_connection << "Command history:" << crlf;
	size_t i = 0;
	for ( auto a : m_history )
	{
		m_connection << std::setfill ( ' ' ) << std::setw ( 3 ) << i << ": " << a << crlf;
		++i;
	}
	return libcli::OK;
} // cli::internal_history ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set the size of the history
 * 
 * @param command UNUSED
 * @param args    The number of lines stored in history
 * 
 * @return libcli::OK when everything went fine
 * @return libcli::ERROR_ARG if there was an argument
 * @return libcli::ERROR_ANY if the only argument was a '?'
 */
RESULT
cli::internal_set_history
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t lines { 0 };
	int	   invalid { -1 };

	int i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // argument 'number of lines'
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<NUM>" << "Number of lines to hold in the history" << crlf;
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show current number of lines" << crlf;
				return ( libcli::OK );
			}
			lines = StrToNum<size_t> ( a, invalid );
			if ( invalid )
			{
				return libcli::INVALID_ARG;
			}
			break;
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	if ( lines > 0 )
	{
		set_history_size ( lines );
	}
	m_connection << "history size is " << m_max_history << " lines." << crlf;
	return libcli::OK;
} // cli::internal_set_history ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief command '!', invoke a command from the history
 *
 * @param name UNUSED
 * @param args The history number of the command to be executed.
 *
 * @return int libcli::OK      when everything went well
 * @return libcli::INVALID_ARG if the argument was not a number.
 * @return libcli::END_OF_ARGS if the last argument was asking for help.
 * @return libcli::ERROR_ARG   if the number was out of range
 */
RESULT
cli::internal_invoke_history
(
	UNUSED ( const std::string& name ),
	const tokens& args
)
{
	size_t h { NOT_IN_HISTORY };
	size_t i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // argument 'number of history entry'
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<NUM>" << "Number of history entry to invoke" << crlf;
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<CR>" << "repeat last command" << crlf;
				return ( libcli::OK );
			}
			int invalid;
			h = StrToNum<size_t> ( a, invalid );
			if ( invalid )
			{
				return libcli::INVALID_ARG;
			}
			break;
		default:
			if ( wants_help_last_arg ( a ) )
			{	// user wanted help
				return libcli::END_OF_ARGS;
			}
			return libcli::INVALID_ARG;
		}
		++i;
	}
	if ( m_history.size () == 0 )
	{
		m_connection << unfiltered << "% no history yet!" << crlf;
		return libcli::OK;
	}
	if ( h == NOT_IN_HISTORY )
	{
		h = m_history.size () - 1;
	}
	if ( h > m_history.size () - 1 )
	{
		m_connection << unfiltered << "% number out f range!" << crlf;
		return libcli::ERROR_ARG;
	}
	auto hist = m_history.begin () + h;
	std::string line = *hist;
	if ( run_command ( line ) == libcli::OK )
	{
		add_history ( line );
	}
	return libcli::OK;
} // cli::internal_invoke_history ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief command 'quit', exit the cli.
 *
 * @param name UNUSED
 * @param args the list of arguments
 *
 * @return libcli::OK when everything went fine
 * @return libcli::ERROR_ARG when there was an argument
 * @return libcli::ERROR_ANY if the only argument was a '?'
 *
 */
RESULT
cli::internal_quit
(
	UNUSED ( const std::string& name ),
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	set_privilege ( PRIVLEVEL::UNPRIVILEGED );
	set_mode ( MODE::STANDARD, "" );
	std::vector<std::string> quit_message =
	{
		"Roger and out.",
		"Roger, wilco!",
		"Time is on my side...",
		"This is the worst kind of discrimination. The kind against me.",
		"Good news, everyone! I've taught the toaster to feel love.",
		"Valentine's Day is coming? Oh crap - I forgot to get a girlfriend again.",
		"There will be plenty of time to discuss your objections when and if you return.",
		"I'm so embarrassed. I wish everybody else was dead.",
		"You win again gravity.",
		"Oh wait, you're serious. Let me laugh even harder.",
		"We will meet again!"
	};
	srandom ( time ( 0 ) );
	long i = random () % quit_message.size ();
	m_connection << unfiltered << quit_message[i] << crlf;
	return libcli::QUIT;
} // cli::internal_quit ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief command 'exit'
 *
 * Exit the current mode. If we are in @ref libcli::MODE::STANDARD
 * exit the cli.
 *
 * @param name UNUSED
 * @param args UNUSED
 *
 * @return libcli::OK when everything went fine
 * @return libcli::ERROR_ARG if there was an argument
 * @return libcli::ERROR_ANY if the only argument was a '?'
 *
 */
RESULT
cli::internal_exit
(
	UNUSED ( const std::string& name ),
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	if ( m_mode == MODE::STANDARD )
	{
		return internal_quit ( name, args );
	}
	if ( m_mode > MODE::STANDARD )
	{
		set_mode ( MODE::STANDARD, "" );
	}
	return libcli::OK;
} // cli::internal_exit ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set the number of lines printed in a row without a more prompt
 * 
 * @param command UNUSED
 * @param args    The number of line to be printed
 * 
 * @return libcli::OK when everything went fine
 * @return libcli::ERROR_ARG if there was an argument
 * @return libcli::ERROR_ANY if the only argument was a '?'
 */
RESULT
cli::internal_pager
(
	const std::string& command,
	const libcli::tokens& args
)
{
	int	lines { -1 };
	int	invalid { -1 };

	int i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // argument 'number of lines'
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<0-512>" << "Number of lines on screen (0 for no pausing)" << crlf;
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show current number of lines" << crlf;
				return ( libcli::OK );
			}
			lines = StrToNum<size_t> ( a, invalid );
			if ( invalid )
			{
				return libcli::INVALID_ARG;
			}
			break;
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	if ( lines > 0 )
		m_connection.max_screen_lines = lines;
	if ( lines == 0 )
		m_connection << "disabled pausing" << crlf;
	else
		m_connection << "show " << m_connection.max_screen_lines << " lines without pausing" << crlf;
	return libcli::OK;
} // cli::internal_pager ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
RESULT
cli::internal_hostname
(
	const std::string& command,
	const libcli::tokens& args
)
{
	int i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // argument 'number of lines'
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<string>" << "a hostname" << crlf;
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show current hostname" << crlf;
				return ( libcli::OK );
			}
			set_hostname ( a );
			return libcli::OK;
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	m_connection << "hostname is '" << m_hostname << "'" << crlf;
	return libcli::OK;
} // cli::internal_hostname ()
//////////////////////////////////////////////////////////////////////

///@}

/** @name filters
 */
///@{
//////////////////////////////////////////////////////////////////////
/**
 * @brief filter 'grab'
 *
 * Instantiates a @ref match_filter in match_filter::MATCH_MODE::NORM mode.
 * This filter only prints lines which contain a match string.
 *
 * @see @ref match_filter::match_filter
 *
 * @param name UNUSED
 * @param args The string to be matched
 *
 * @return libcli::END_OF_ARGS if the arguments conaints a '?' (asking for help)
 * @return libcli::OTHER       if the only argument was a '?'
 * @return libcli::ERROR_ARG   if there was no argument
 * @return libcli::OK          when everything went fine
 */
RESULT
cli::filter_grab
(
	const std::string& name,
	const tokens& args
)
{
	std::string match_me;
	size_t i { 0 };
	for ( auto a : args )
	{	// argument '<WORD>'
		switch ( i )
		{
		case 0:
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<WORD>" << "only display lines that match <WORD>" << crlf;
				return ( libcli::OTHER );
			}
			match_me = a;
			break;
		default:
			if ( wants_help_last_arg ( a ) )
			{
				return libcli::END_OF_ARGS;
			}
			match_me += " ";
			match_me += a;
		}
		++i;
	}
	if ( match_me.length () == 0 )
	{
		m_connection << unfiltered << "% missing argument!" << crlf;
		return libcli::ERROR_ARG;
	}
	m_connection.m_active_filters.push_back (
		new match_filter {
			match_filter::MATCH_MODE::NORM,
			match_me
		}
	);
	return libcli::OK;
} // cli::filter_grap ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief filter 'exclude'
 *
 * Instantiates a @ref match_filter in match_filter::MATCH_MODE::INVERT mode.
 * This filter only prints lines which do not contain a match string.
 *
 * @see @ref match_filter::match_filter
 *
 * @param name UNUSED
 * @param args The string not to be matched
 *
 * @return libcli::END_OF_ARGS if the arguments conaints a '?' (asking for help)
 * @return libcli::OTHER       if the only argument was a '?'
 * @return libcli::ERROR_ARG   if there was no argument
 * @return libcli::OK          when everything went fine
 */
RESULT
cli::filter_exclude
(
	const std::string& name,
	const tokens& args
)
{
	std::string match_me;
	size_t i { 0 };
	for ( auto a : args )
	{	// argument '<WORD>'
		switch ( i )
		{
		case 0:
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<WORD>" << "only display lines that do not match <WORD>" << crlf;
				return ( libcli::OTHER );
			}
			match_me = a;
			break;
		default:
			if ( wants_help_last_arg ( a ) )
			{
				return libcli::END_OF_ARGS;
			}
			match_me += " ";
			match_me += a;
		}
		++i;
	}
	if ( match_me.length () == 0 )
	{
		m_connection << unfiltered << "% missing argument!" << crlf;
		return libcli::ERROR_ARG;
	}
	m_connection.m_active_filters.push_back (
		new match_filter {
			match_filter::MATCH_MODE::INVERT,
			match_me
		}
	);
	return libcli::OK;
} // cli::filter_exclude ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief filter 'begin'
 *
 * Instantiates a @ref begin_filter.
 * This filter starts to print lines when a match string is found.
 *
 * @param name UNUSED
 * @param args The string to match
 *
 * @return libcli::END_OF_ARGS if the arguments conaints a '?' (asking for help)
 * @return libcli::OTHER       if the only argument was a '?'
 * @return libcli::ERROR_ARG   if there was no argument
 * @return libcli::OK          when everything went fine
 *
 * @see begin_filter::begin_filter
 */
RESULT
cli::filter_begin
(
	const std::string& name,
	const tokens& args
)
{
	std::string match_me;
	size_t i { 0 };
	for ( auto a : args )
	{	// argument '<WORD>'
		switch ( i )
		{
		case 0:
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<WORD>" << "start printing lines when <WORD> matches" << crlf;
				return ( libcli::OTHER );
			}
			match_me = a;
			break;
		default:
			if ( wants_help_last_arg ( a ) )
			{
				return libcli::END_OF_ARGS;
			}
			return libcli::ERROR_ARG;
		}
		++i;
	}
	if ( match_me.length () == 0 )
	{
		m_connection << unfiltered << "% missing argument!" << crlf;
		return libcli::ERROR_ARG;
	}
	m_connection.m_active_filters.push_back (
		new begin_filter { match_me }
	);
	return libcli::OK;
} // cli::filter_begin ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief filter 'between'
 *
 * Instantiates a @ref between_filter.
 * This filter starts to print lines when a match string is found and stops
 * when another string matches.
 *
 * @param name UNUSED
 * @param args A string 'match start' and a string "match end"
 *
 * @return libcli::END_OF_ARGS if the arguments conaints a '?' (asking for help)
 * @return libcli::OTHER       if the only argument was a '?'
 * @return libcli::ERROR_ARG   if there is an argument missing
 * @return libcli::OK          when everything went fine
 *
 * @see begin_filter::begin_filter
 */
RESULT
cli::filter_between
(
	const std::string& name,
	const tokens& args
)
{
	std::string start_with;
	std::string end_with;
	size_t i { 0 };
	for ( auto a : args )
	{	// argument '<WORD>'
		switch ( i )
		{
		case 0:
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<WORD>" << "start printing lines when <WORD> matches" << crlf;
				return ( libcli::OTHER );
			}
			start_with = a;
			break;
		case 1:
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<WORD>" << "stop printing lines when <WORD> matches" << crlf;
				return ( libcli::OTHER );
			}
			end_with = a;
			break;
		default:
			if ( wants_help_last_arg ( a ) )
			{
				return libcli::END_OF_ARGS;
			}
			return libcli::ERROR_ARG;
		}
		++i;
	}
	if ( start_with.length () == 0 )
	{
		m_connection << unfiltered << "% missing argument!" << crlf;
		return libcli::ERROR_ARG;
	}
	if ( end_with.length () == 0 )
	{
		m_connection << unfiltered << "% missing argument!" << crlf;
		return libcli::ERROR_ARG;
	}
	m_connection.m_active_filters.push_back (
		new between_filter { start_with, end_with }
	);
	return libcli::OK;
} // cli::filter_between ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief filter 'count'
 *
 * Instantiates a @ref count_filter.
 * This filter only counts the lines passed to it. Only none-empty lines
 * are counted.
 *
 * @param name UNUSED
 * @param args UNUSED
 *
 * @return libcli::ERROR_ANY   if the only argument was a '?'
 * @return libcli::ERROR_ARG   if there was an argument
 * @return libcli::OK          when everything went fine
 *
 * @see begin_filter::begin_filter
 */
RESULT
cli::filter_count
(
	const std::string& name,
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	m_connection.m_active_filters.push_back (
		new count_filter {}
	);
	return libcli::OK;
} // cli::filter_count ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief filter 'limit'
 *
 * Instantiates a @ref limit_filter.
 * This filter only prints <NUM> lines of output. Only
 * none-empty lines are counted.
 *
 * @param name UNUSED
 * @param args <NUM> The number of lines to print
 *
 * @return libcli::END_OF_ARGS if the arguments conaints a '?' (asking for help)
 * @return libcli::OTHER       if the only argument was a '?'
 * @return libcli::ERROR_ARG   if there is an argument missing
 * @return libcli::OK          when everything went fine
 *
 * @see limit_filter::limit_filter
 */
RESULT
cli::filter_limit
(
	const std::string& name,
	const tokens& args
)
{
	size_t limit { 10 };
	size_t i { 0 };
	for ( auto a : args )
	{	// argument '<WORD>'
		switch ( i )
		{
		case 0:
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<NUM>" << "print only <NUM> lines of output" << crlf;
				return ( libcli::OTHER );
			}
			int invalid;
			limit = StrToNum<size_t> ( a, invalid );
			if ( invalid )
			{
				return libcli::INVALID_ARG;
			}
			break;
		default:
			if ( wants_help_last_arg ( a ) )
			{
				return libcli::END_OF_ARGS;
			}
			return libcli::ERROR_ARG;
		}
		++i;
	}
	m_connection.m_active_filters.push_back (
		new limit_filter { limit }
	);
	return libcli::OK;
} // cli::filter_limit ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief filter 'no-more'
 *
 * Temporarily disable the more prompt.
 * Instantiates a @ref nomore_filter.
 *
 * @param name UNUSED
 * @param args UNUSED
 *
 * @return libcli::OTHER       if the only argument was a '?'
 * @return libcli::ERROR_ARG   if there was an argument
 * @return libcli::OK          when everything went fine
 *
 * @see nomore_filter::nomore_filter
 */
RESULT
cli::filter_nomore
(
	const std::string& name,
	const tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	m_connection.m_active_filters.push_back (
		new nomore_filter { m_connection }
	);
	return libcli::OK;
} // cli::filter_nomore ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief filter 'last'
 *
 * Instantiates a @ref last
 * _filter.
 * This filter only prints the last <NUM> lines of output. Only
 * none-empty lines are counted.
 *
 * @param name UNUSED
 * @param args <NUM> The number of lines to print
 *
 * @return libcli::END_OF_ARGS if the arguments conaints a '?' (asking for help)
 * @return libcli::OTHER       if the only argument was a '?'
 * @return libcli::ERROR_ARG   if there is an argument missing
 * @return libcli::OK          when everything went fine
 *
 * @see last_filter::last_filter
 */
RESULT
cli::filter_last
(
	const std::string& name,
	const tokens& args
)
{
	size_t limit { 10 };
	size_t i { 0 };
	for ( auto a : args )
	{	// argument '<WORD>'
		switch ( i )
		{
		case 0:
			if ( arg_wants_help ( a ) )
			{
				m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<NUM>" << "print the last <NUM> lines of output" << crlf;
				return ( libcli::OTHER );
			}
			int invalid;
			limit = StrToNum<size_t> ( a, invalid );
			if ( invalid )
			{
				return libcli::INVALID_ARG;
			}
			break;
		default:
			if ( wants_help_last_arg ( a ) )
			{
				return libcli::END_OF_ARGS;
			}
			return libcli::ERROR_ARG;
		}
		++i;
	}
	m_connection.m_active_filters.push_back (
		new last_filter { limit }
	);
	return libcli::OK;
} // cli::filter_last ()
//////////////////////////////////////////////////////////////////////

///@} filters

//////////////////////////////////////////////////////////////////////
/**
 * @brief split a string into tokens
 *
 * Tokens are words seperated by whitespaces or character sequences
 * enclosed in quotes. Quotes are either enclosed by ' or by "
 *
 * @param s     The string to split
 * @param words A list of strings containing the tokens
 *
 * @return libcli::OK          Everything went fine
 * @return libcli::ERROR_QUOTE If there is a missing quote
 *
 * @see libcli::WHITESPACE
 */
RESULT
cli::split_line
(
	const std::string& s,
	tokens& words
)
{
	size_t		pos_start { 0 };
	size_t		pos_end { 0 };
	char		in_quotes { '\0' };
	std::string	token;

	while ( ( pos_start != std::string::npos ) && ( pos_end != std::string::npos ) )
	{
		pos_start = s.find_first_not_of ( WHITESPACE, pos_start );
		if ( pos_start == std::string::npos )
		{
			return libcli::OK;
		}
		if ( ( s[pos_start] == '"' ) || ( s[pos_start] == '\'' ) )
		{
			in_quotes = s[pos_start];
			++pos_start;
			pos_end = s.find_first_of ( in_quotes, pos_start );
			if ( pos_end == std::string::npos )
			{
				m_connection << unfiltered << "missing closing quote" << crlf;
				return libcli::MISSING_QUOTE;
			}
		}
		else
		{
			pos_end = s.find_first_of ( WHITESPACE, pos_start );
		}
		token = s.substr ( pos_start, pos_end - pos_start );
		pos_start = pos_end + 1;
		if ( in_quotes != '\0' )
		{
			in_quotes = '\0';
		}
		words.push_back ( token );
	}
	return libcli::OK;
} // cli::split_lines ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print a standard message according to \a result
 *
 * @param result FIXME: enum class RESULT
 */
void
cli::result_msg
(
	RESULT result
)
{
	switch ( result )
	{
	case libcli::TOO_MANY_ARGS:
		m_connection << "% too many arguments" << crlf;
		return;
	case libcli::END_OF_ARGS:
		m_connection << "<cr>" << crlf;
		m_connection << "|" << crlf;
		return;
	case libcli::MISSING_ARG:
		m_connection << "% missing argument" << crlf;
		return;
	case ERROR_ARG:
		m_connection << "% errornous argument" << crlf;
		return;
	case MISSING_QUOTE:
		m_connection << "% missing quote" << crlf;
		return;
	case UNKNOWN_COMMAND:
		m_connection << "% unknown command" << crlf;
		return;
	case UNKNOWN_FILTER:
		m_connection << "% unknown filter" << crlf;
		return;
	case INVALID_ARG:
		m_connection << "% invalid argument" << crlf;
		return;
	case OK:
	case ERROR_ANY:
	case OTHER:
	case QUIT:
	case DEFAULT_MAX:
		// keep the compiler happy but do nothing
		break;
	}
} // cli::result_msg ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Find and execute a filter
 *
 * Executing a filter means to instantiate the corresponding filter
 * and adding that to @ref connection::m_active_filters.
 *
 * @param filters A @ref cmd_list of all defined filters
 * @param words   A list of words. The first word is the name of the
 *                filter to execute, all other words are arguments to
 *                the filter
 *
 * @return libcli::OK        Everything went fine
 * @return libcli::ERROR_ARG Found no matching filter
 * @return libcli::OTHER     User wanted and got help
 * @return libcli::ERROR_ANY The filter was found, bit it has no callback
 */
RESULT
cli::find_filter
(
	const cmd_list& filters,
	tokens& words
)
{
	if ( ( filters.empty () ) || ( words.empty () ) )
	{
		return libcli::ERROR_ARG;
	}
	std::string word { words.front () };
	words.erase ( words.begin () );
	// deal with '?'
	bool want_help { false };
	size_t len = word.length () - 1;
	if ( '?' == word[len] )
	{
		/////////////////////////////////////////////////
		//
		// Instead of a filtername we just got a '?' or 
		// the beginning of a name ending with a '?'.
		// So list all possible matching filters.
		//
		/////////////////////////////////////////////////
		bool have_help { false };
		want_help = true;
		word.pop_back ();	// delete '?'
		for ( auto c : filters )
		{	// now find the filter command
			if ( ( c.m_name.rfind ( word, 0 ) != std::string::npos )
				&& ( ( nullptr != c.m_callback.member ) || ( !c.m_children.empty () ) )
				)
			{	// and print help
				m_connection << "  "
					<< std::left << std::setfill ( ' ' ) << std::setw ( 20 )
					<< c.m_name
					<< c.m_help
					<< crlf;
				have_help = true;
			}
		}
		if ( !have_help )
		{
			m_connection << "'" << word << "' not found!" << crlf;
			return libcli::ERROR_ARG;
		}
		return libcli::OTHER;	// user just wanted help
	}
	/////////////////////////////////////////////////
	//
	// Find and execute the filter
	//
	/////////////////////////////////////////////////
	for ( auto f : filters )
	{
		if ( std::string::npos == f.m_name.rfind ( word, 0 ) )
		{
			continue;
		}
		if ( word.length () < f.m_unique_len )
		{
			m_connection << unfiltered
				<< "Ambiguous filter!" << crlf;
			continue;
		}
		// found the filter
		if ( nullptr == f.m_callback.member )
		{
			m_connection << unfiltered
				<< "% filter with no callback!" << crlf;
			return libcli::ERROR_ANY;
		}
		if ( want_help )
		{
			m_connection << "  "
				<< std::left << std::setfill ( ' ' ) << std::setw ( 20 )
				<< f.m_name
				<< f.m_help
				<< crlf;
			return libcli::OTHER;
		}
		return f ( f.m_name, words );
	}
	return libcli::UNKNOWN_FILTER;
} // cli::find_filter()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Split a string defining filters into tokens
 *
 * A string defining filters probably contains a list of filters
 * seperated by a '|' character.
 * For each filter cli::find_filter is executed.
 *
 * @param s     The string to split
 *
 * @return libcli::OK Everything went fine
 * @return libcli::ERROR_ARG Found no matching filter
 * @return libcli::OTHER User wanted and got help
 * @return libcli::ERROR_ANY The filter was found, bit it has no callback
 * @return libcli::MISSING_QUOTE Missing ' or "
 */
RESULT
cli::split_filter
(
	const std::string& s
)
{
	size_t		pos_start { 0 };
	size_t		pos_end { 0 };
	std::string	token;

	while ( ( pos_start != std::string::npos ) && ( pos_end != std::string::npos ) )
	{
		pos_start = s.find ( '|', pos_start );
		if ( std::string::npos == pos_start )
		{
			return libcli::OK;
		}
		++pos_start;
		pos_end = s.find ( "|", pos_start );
		token = s.substr ( pos_start, pos_end - pos_start );
		tokens filter;
		RESULT r = split_line ( token, filter );
		if ( libcli::OK != r )
		{
			return r;
		}
		r = find_filter ( m_filters, filter );
		if ( libcli::OK != r )
		{
			result_msg ( r );
			return r;
		}
	}
	return libcli::OK;
} // cli::split_filter ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

/**
 * @brief Find and execute a command
 *
 * @param commands A @ref cmd_list of all defined commands.
 * @param words    A list of words. The first word is the name of the
 *                 command to execute, all other words are arguments to
 *                 the command
 *
 * @return libcli::OK        Everything went fine
 * @return libcli::ERROR_ARG Found no matching filter
 * @return libcli::OTHER     User wanted and got help
 * @return libcli::ERROR_ANY The filter was found, bit it has no callback
 */
RESULT
cli::find_command
(
	const cmd_list& commands,
	tokens& words
)
{
	if ( words.empty () )
	{
		return libcli::ERROR_ANY;
	}
	std::string word { words.front () };
	words.erase ( words.begin () );
	// deal with '?'
	size_t len = word.length () - 1;
	if ( word[len] == '?' )
	{
		/////////////////////////////////////////////////
		//
		// Instead of a commandname we just got a '?' or 
		// the beginning of a name ending with a '?'.
		// So list all possible matching commands.
		//
		/////////////////////////////////////////////////
		bool have_help { false };
		word.pop_back ();	// delete '?'
		for ( auto c : commands )
		{	// now find the command
			if ( ( c.m_name.rfind ( word, 0 ) != std::string::npos )
				&& ( ( c.m_callback.member != nullptr ) || ( !c.m_children.empty () ) )
				&& ( m_privilege >= c.m_privilege )
				&& ( ( c.m_mode == m_mode ) || ( c.m_mode == MODE::ANY ) )
				)
			{	// and print help
				m_connection << "  "
					<< std::left << std::setfill ( ' ' ) << std::setw ( 20 )
					<< c.m_name
					<< c.m_help
					<< crlf;
				have_help = true;
			}
		}
		if ( !have_help )
		{
			m_connection << "'" << word << "' not found!" << crlf;
			return libcli::ERROR_ARG;
		}
		return libcli::OTHER;	// user just wanted help
	}
	/////////////////////////////////////////////////
	//
	// Find and execute the command
	//
	/////////////////////////////////////////////////
	for ( auto c : commands )
	{
		if ( c.m_privilege > m_privilege )
		{
			continue;
		}
		if ( ( c.m_mode != MODE::ANY ) && ( c.m_mode != m_mode ) )
		{	// user has no access to this command
			continue;
		}
		if ( c.m_name.rfind ( word, 0 ) )
		{
			continue;
		}
		if ( word.length () < c.m_unique_len )
		{
			m_connection << unfiltered
				<< "Ambiguous command!" << crlf;
			return libcli::ERROR_ANY;
		}
		if ( !c.m_children.empty () )
		{	// command has children
			if ( !words.empty () )
			{
				std::string word { words.front () };
				RESULT r = find_command ( c.m_children, words );
				if ( r == libcli::OTHER )
				{	// user just wanted help
					return libcli::OK;
				}
				return r;
			}
		}
		// found the command
		if ( c.m_callback.member == nullptr )
		{
			m_connection << unfiltered
				<< "% incomplete command!" << crlf;
			return libcli::ERROR_ANY;
		}
		RESULT i = c ( c.m_name, words );
		// call 'final' of all filters
		if ( m_connection.m_active_filters.size () > 0 )
		{
			for ( auto& f : m_connection.m_active_filters )
			{
				f->final ( m_connection );
			}
		}
		return i;
	}
	return libcli::UNKNOWN_COMMAND;
} // cli::find_command ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Clear the list of active filters
 */
void
cli::clear_active_filters
()
{
	while ( m_connection.m_active_filters.size () > 0 )
	{
		auto f = m_connection.m_active_filters.back ();
		delete f;
		m_connection.m_active_filters.pop_back ();
	}
} // cli::clear_active_filters ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Do everything necessary to execute the command line
 *
 * @param cmd_line The command line to execute
 *
 * @return libcli::OK Everything went fine
 * @return libcli::ERROR_ARG Found no matching filter
 * @return libcli::OTHER User wanted and got help
 * @return libcli::ERROR_ANY The filter was found, bit it has no callback
 * @return libcli::MISSING_QUOTE Missing ' or "
 */
RESULT
cli::run_command
(
	const std::string& cmd_line
)
{
	if ( cmd_line == "" )
	{
		return libcli::OK;
	}
	RESULT r;
	tokens words;
	size_t pos = cmd_line.find ( '|' );
	if ( pos != std::string::npos )
	{	// if there are filters, initialise them first
		r = split_filter ( cmd_line );
		if ( r != libcli::OK )
		{
			return r;
		}
		// now split of the filters and execute the command
		std::string c = cmd_line.substr ( 0, pos );
		r = split_line ( cmd_line.substr ( 0, pos ), words );
	}
	else
	{
		r = split_line ( cmd_line, words );
	}
	if ( r != libcli::OK )
	{
		return r;
	}
	if ( words.size () == 0 )
	{
		return libcli::ERROR_ARG;
	}
	r = find_command ( m_commands, words );
	if ( libcli::OK != r )
	{
		result_msg ( r );
	}
	return r;
} // cli::run_command ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief get possible completions
 *
 * If there are more than one completion, list them all and return
 * an the original command line.
 * 
 * If there is only one completion return the complete command name.
 *
 * @param command The command line entered
 * @return string If there is only one completion
 */
std::string
cli::get_completions
(
	const cmd_list& commands,
	tokens& words
)
{
	std::string word;
	if ( !words.empty () )
	{
		word = words.front ();
		words.erase ( words.begin () );
	}
	bool is_ambiguous { false };
	for ( auto c : commands )
	{
		if ( c.m_privilege > m_privilege )
		{
			continue;
		}
		if ( ( c.m_mode != MODE::ANY ) && ( c.m_mode != m_mode ) )
		{	// user has no access to this command
			continue;
		}
		if ( c.m_name.rfind ( word, 0 ) )
		{
			continue;
		}
		if ( word.length () < c.m_unique_len )
		{
			if ( !is_ambiguous )	// first command, not yet ambiguous
			{
				m_connection << unfiltered << crlf;
			}
			// ambiguous command. print help for all possible completions
			m_connection << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 20 )
				<< c.m_name
				<< c.m_help
				<< crlf;
			is_ambiguous = true;
			continue;
		}
		if ( is_ambiguous )
		{	// cmd is ambiguous
			return "";
		}
		// single completion
		std::string new_line = c.m_name + " ";
		if ( !c.m_children.empty () )
		{	// command has children
			if ( words.size () )
			{
				new_line += get_completions ( c.m_children, words );
			}
		}
		return new_line;
	}
	// command not found, return the original command line provided
	m_connection << unfiltered << crlf;
	std::string new_line { word };
	for ( auto w : words )
	{
		new_line += w + " ";
	}
	return new_line; // should never happen
} // cli::get_completions ()
//////////////////////////////////////////////////////////////////////

const std::string DES_PREFIX = "{crypt}";	/* to distinguish clear text from DES crypted */
const std::string MD5_PREFIX = "$1$";

//////////////////////////////////////////////////////////////////////
/**
 * @brief Compare two passwords
 *
 * @param pass       The password to match
 * @param tried_pass The password entered by the user
 * 
 * @return true      The passwords do match
 * @return false     The passwords do not match
 * 
 * @todo             other encryptions like SHA
 */
bool
cli::pass_matches
(
	const std::string& pass,
	const std::string& tried_pass
)
{
	int des;
	int idx = 0;
	des = !pass.compare ( 0, DES_PREFIX.size (), DES_PREFIX );
	if ( des )
	{
		idx = sizeof ( DES_PREFIX ) - 1;
	}
	if ( des || ( !pass.compare ( 0, MD5_PREFIX.size (), MD5_PREFIX ) ) )
	{
		return pass.compare ( idx, pass.size (),
			crypt ( tried_pass.c_str (), pass.c_str () )
		);
	}
	return !pass.compare ( idx, pass.size (), tried_pass );
} // cli::pass_matches ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check the enable password
 * 
 * @param pass The password entered by the user
 * 
 * If the password is correct, switch to enable state and set users
 * privilege level to PRIVLEVEL::PRIVILEGED
 */
void
cli::check_enable
(
	const std::string& pass
)
{
	int allowed = 0;
	if ( m_enable_password != "" )
	{	// check stored static enable password 
		if ( pass_matches ( m_enable_password, pass ) )
		{
			allowed++;
		}
	}
	if ( !allowed )
	{
		/* check callback */
		if ( m_enable_callback.member != nullptr )
		{
			if ( m_enable_callback.member ( *this, pass ) )
			{
				allowed++;
			}
		}
	}
	if ( allowed )
	{
		m_connection << "-ok-" << crlf;
		m_state = STATE::ENABLE;
		set_privilege ( PRIVLEVEL::PRIVILEGED );
	}
	else
	{
		m_connection << crlf << crlf << "Access denied" << crlf;
		m_state = STATE::NORMAL;
	}
} // cli::check_enable ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check authentication of username and password
 * 
 * @param username The username entered by the user
 * @param password The password entered by the user
 * 
 * If authentication succeeds set m_state to STATE::NORMAL
 * 
 * The privilege level is already set to PRIVILEGE::UNPRIVILEGED
 * by cli::cli ().
 */
void
cli::check_user_auth
(
	const std::string& username,
	const std::string& password
)
{
	/* require password */
	int allowed = 0;
	if ( m_auth_callback.member != nullptr )
	{
		if ( m_auth_callback.member ( *this, username, password ) == libcli::OK )
		{
			allowed++;
		}
	}
	if ( !allowed )
	{
		auto u = m_users.find ( username );
		if ( u != m_users.end () )
		{
			if ( pass_matches ( u->second, password ) )
				allowed++;
		}
	}
	if ( allowed )
	{
		m_connection << "-ok-" << crlf;
		m_connection << "type '?' or 'help' for help." << crlf << crlf;
		m_state = STATE::NORMAL;
	}
	else
	{
		m_connection << crlf << crlf << "Access denied" << crlf;
		m_state = STATE::LOGIN;
	}
} // cli::check_user_auth ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief return to standard mode
 */
void
cli::leave_mode
()
{
	if ( m_mode != MODE::STANDARD )
	{
		// m_connection << crlf;
		set_mode ( MODE::STANDARD, "" );
	}
} // cli::leave_mode ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set the size of the history
 * 
 * @param size Number of lines to keep in history.
 * 
 * If the new size is larger then the old size, shrink the current
 * history accordingly.
 */
void
cli::set_history_size
(
	size_t size
)
{
	m_max_history = size;
	while ( m_max_history > m_history.size () )
	{
		m_history.erase ( m_history.begin () );
	}
} // cli::echo_chars ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Add a command line to the history
 * 
 * @param line The command line to add
 * 
 * If the history grows above the history size delete the oldest
 * entry.
 */
void
cli::add_history
(
	const std::string& line
)
{
	if ( m_history.size () > m_max_history )
	{
		m_history.erase ( m_history.begin () );
	}
	m_history.push_back ( line );
} // cli::add_history ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Scroll through the hostory
 * 
 * @param c The arrow key pressed by the user (up or down)
 * @return  The corresponding history entry
 * 
 * Scroll backwards through the history if the user pressed the 'UP' key.
 * Scroll not beyond the first entry.
 * 
 * Scoll forward through the history if the user pressed the 'DOWN' key. If
 * we are beyond the last entry return an empty line.
 */
std::string
cli::do_history
(
	const unsigned char& c
)
{
	if ( m_history.empty () )
	{
		return "";
	}
	if ( c == CTRL ( 'P' ) ) // Up
	{
		if ( m_in_history == NOT_IN_HISTORY )
		{
			m_in_history = m_history.size ();
		}
		if ( m_in_history > 0 )
			--m_in_history;
		auto h = m_history.begin () + m_in_history;
		return *h;
	}
	// Down
	if ( ( m_in_history == NOT_IN_HISTORY ) || ( m_in_history > m_history.size () - 1 ) )
	{
		m_in_history = m_history.size () - 1;
	}
	++m_in_history;
	if ( m_in_history < m_history.size () )
	{
		auto h = m_history.begin () + m_in_history;
		return *h;
	}
	return "";
} // cli::do_history ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Generate a prompt string
 * 
 * @return The generated prompt string 
 */
std::string
cli::make_prompt
()
{
	std::string prompt;
	switch ( m_state )
	{
	case STATE::NORMAL:
	case STATE::ENABLE:
		prompt = m_hostname;
		if ( m_modestring != "" )
			prompt += m_modestring;
		if ( m_privilege == PRIVLEVEL::PRIVILEGED )
			prompt += "# ";
		else
			prompt += "> ";
		break;
	case STATE::LOGIN:
		prompt = "Username: ";
		break;
	case STATE::PASSWORD:
	case STATE::ENABLE_PASSWORD:
		prompt = "Password: ";
		break;
	}
	return prompt;
} // cli::make_prompt ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief The main loop of the cli
 * 
 * @return libcli::OK Always
 */
RESULT
cli::loop
()
{
	build_shortest ( m_commands );
	sort ( m_commands.begin (), m_commands.end (),
		[]( const command& lhs, const command& rhs ) { return lhs.m_name < rhs.m_name; }
	);

	if ( m_banner != "" )
	{
		m_connection << m_banner << crlf;
	}
	m_state = STATE::LOGIN;
	if ( m_users.empty () && ( m_auth_callback.member == nullptr ) )
	{	// no auth required?
		m_state = STATE::NORMAL;
	}
	/* start off in unprivileged mode */
	set_privilege ( PRIVLEVEL::UNPRIVILEGED );
	set_mode ( MODE::STANDARD, "" );
	bool finished { false };
	std::string input;
	int curr_key { 0 };
	int last_key;
	while ( !finished )
	{
		m_connection.lines_out = 0;
		m_edit.set_prompt ( make_prompt () );
		last_key = curr_key;	// detect double TAB
		curr_key = m_edit.get_line ( input );
		if ( m_state == STATE::LOGIN )
		{
			if ( curr_key == CTRL ( 'D' ) )
			{
				m_connection << "quit" << crlf;
				internal_quit ( "quit", tokens {} );
				finished = true;
				break;
			}
			m_username = input;
			input.clear ();
			m_state = STATE::PASSWORD;
			m_edit.password_mode ( true );
			continue;
		}
		else if ( m_state == STATE::PASSWORD )
		{
			check_user_auth ( m_username, input );
			m_edit.password_mode ( false );
			input.clear ();
			continue;
		}
		if ( m_state == STATE::ENABLE_PASSWORD )
		{
			check_enable ( input );
			m_edit.password_mode ( false );
			input.clear ();
			continue;
		}
		switch ( curr_key )
		{
		case '\n':
		case '\r':
			//if ( m_state != STATE::PASSWORD && m_state != STATE::ENABLE_PASSWORD )
			//	m_connection << crlf;
			break;
		case CTRL ( 'Z' ):	// leave the current mode, FIXME CTRL-Z does not work
			leave_mode ();
			continue;
		case CTRL ( 'I' ):	// TAB completion
		{
			if ( last_key == curr_key )	// double TAB
			{
				m_connection << unfiltered << crlf;
				// input += '?';	// ask for help
				// curr_key = '?';
				// break;
			}
			tokens words;
			size_t pos = input.rfind ( '|' );
			if ( pos != std::string::npos )
			{	// find completion for a filter
				pos = input.find_first_not_of ( WHITESPACE, pos + 1 );
				if ( pos == std::string::npos )
				{	// found '|' but nothing else
					m_connection << unfiltered << crlf << "incomplete command!" << crlf;
				}
				else
				{
					std::string f = input.substr ( pos );
					split_line ( f, words );
					std::string replace = get_completions ( m_filters, words );
					input.replace ( pos, f.length (), replace );
				}
			}
			else
			{
				split_line ( input, words );
				input = get_completions ( m_commands, words );
				clear_active_filters ();
			}
			continue;
		}
		case CTRL ( 'D' ):
			m_connection << "quit" << crlf;
			internal_quit ( "quit", tokens {} );
			finished = true;
			break;
		case CTRL ( 'P' ):	// m_cursor Up
		case CTRL ( 'N' ):	// m_cursor Down
			if ( ( m_state > STATE::ENABLE_PASSWORD ) )
			{
				// delete the current input line
				for ( size_t i = 0; i < input.length (); ++i )
				{
					m_connection.put_char ( '\b' );
					m_connection.put_char ( ' ' );
					m_connection.put_char ( '\b' );
				}
				m_connection.put_char ( '\r' );
				input = do_history ( curr_key );
				continue;
			}
			break;
		default:		// normal character typed
			if ( m_state < STATE::NORMAL )
				continue;
			break;
		} // switch
		if ( input == "" )
		{
			continue;
		}
		RESULT r = run_command ( input );
		clear_active_filters ();
		if ( r == libcli::QUIT )
		{
			break;
		}
		if ( curr_key == '?' )
		{
			// delete the trailing '?' and re-edit the line
			input.erase ( input.length () - 1 );
		}
		else
		{
			auto t { input.find_first_not_of ( " \t" ) };
			if ( ( t != std::string::npos ) && ( input[t] != '!' ) )
			{
				add_history ( input );
			}
			input.clear ();
		}
		m_in_history = NOT_IN_HISTORY;
	}
	return libcli::OK;
} // cli::loop ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Read commands from a file
 * 
 * @param filename  The file to read from
 * @param privilege The privilege level to use while reading the file
 * @param mode      The mode to use while reading the file
 *
 * @return libcli::OK Everything went fine
 * @return libcli::ERROR_ARG Found no matching filter
 * @return libcli::OTHER User wanted and got help
 * @return libcli::ERROR_ANY The filter was found, bit it has no callback
 * @return libcli::MISSING_QUOTE Missing ' or "
 */
RESULT
cli::read_file
(
	const std::string& filename,
	int privilege,
	int mode
)
{
	int oldpriv = set_privilege ( privilege );
	int oldmode = set_mode ( mode, "" );
	std::ifstream in_file ( filename );
	if ( !in_file.is_open () )
	{
		m_connection << unfiltered << "unable to open '" << filename << "' for reading." << crlf;
	}
	std::string line;
	while ( getline ( in_file, line ) )
	{
		if ( run_command ( line ) == libcli::QUIT )
		{
			clear_active_filters ();
			break;
		}
		clear_active_filters ();
	}
	set_privilege ( oldpriv );
	set_mode ( oldmode, "" /* didn't save desc */ );
	return libcli::OK;
} // cli::file ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Present a more-prompt to the user
 * 
 * @return false The user just pressed a key, continue with output
 * @return true  The user pressed 'q', stop the output
 */
bool
cli::pager
()
{
	unsigned char c;
	bool done = false;
	const std::string more_prompt { "--- more ---" };
	m_connection << unfiltered << more_prompt << commit;
	c = ' ';
	while ( done == false )
	{
		m_connection.get_input ( c );
		if ( c == 0x00 )
		{	// timeout
			continue;
		}
		if ( c == 'q' )
		{
			m_connection.lines_out = 0;
			m_connection.put_char ( '\r' );
			for ( size_t i = 0; i < more_prompt.length (); ++i )
				m_connection.put_char ( ' ' );
			m_connection.put_char ( '\r' );
			return true;
		}
		if ( c == ' ' )
		{
			m_connection.lines_out = 0;
			done = true;
		}
		if ( ( c == '\r' ) || ( c == '\n' ) )
		{	// show next line and reprompt for more
			m_connection.lines_out--;
			done = true;
			m_connection.put_char ( '\r' );
			for ( size_t i = 0; i < more_prompt.length (); ++i )
				m_connection.put_char ( ' ' );
		}
	}
	m_connection.put_char ( '\r' );
	for ( size_t i = 0; i < more_prompt.length (); ++i )
		m_connection.put_char ( ' ' );
	m_connection.put_char ( '\r' );
	return false;
} // cli::pager ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check if we need to present the more prompt
 * 
 * @return true  The user pressed 'q', stop output
 * @return false Continue with output
 */
bool
cli::check_pager
()
{
	if ( m_connection.max_screen_lines == 0 )
		return false;
	if ( m_connection.lines_out > m_connection.max_screen_lines )
	{
		if ( pager () )
		{
			return true;
		}
	}
	return false;
} // cli::check_pager ()
//////////////////////////////////////////////////////////////////////

}; // namespace libcli

