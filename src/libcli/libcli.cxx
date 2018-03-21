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
 * @file        libcli.cxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2011/2018
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <exception>
#include <vector>
#include <random>
#include <fglib/fg_log.hxx>
#include <fglib/fg_util.hxx>
#include "libcli.hxx"

#if defined(_MSC_VER) || defined(__CYGWIN__)
	// some windows quick fixes
	#ifdef __cplusplus
	extern "C"
	{
	#endif
		extern char* crypt ( const char* key, const char* salt );
	#ifdef __cplusplus
	}
	#endif
#endif

namespace libcli
{

/** some predefined filters
 */
struct filter_cmds_t
{
	const std::string name;
	const size_t num_args;
	const size_t unique_len;
	const std::string help;
};

// define some useful help texts for internal filters.
std::vector<filter_cmds_t> filter_cmds
{
	{ "begin",   1, 3, "begin WORD - Begin with lines that match WORD" },
	{ "between", 2, 3, "Between lines that match" },
	{ "count",   0, 1, "Count of lines"   },
	{ "exclude", 1, 1, "Exclude lines that match" },
	{ "include", 1, 1, "Include lines that match" },
	{ "pager",   1, 1, "activate pager" },
	{ "file",    2, 1, "write output to a file" },
};

namespace
{
	/// little helper for parse_line()
	inline bool
	is_quote
	(
		int c
	)
	{
		return ( ( c == '\"' ) || ( c == '\'' ) );
	} // is_quote()

	//////////////////////////////////////////////////

	/// internally known users
	struct cli_users
	{
		std::string password;
		cmd_priv    privlevel;
	}; // struct cli_users
	std::map < std::string, cli_users > m_users;
} // anonymous namespace

//////////////////////////////////////////////////////////////////////

/**
 * Add a user to the internal list of known users.
 * @param username Login name of the user.
 * @param password The password for this user.
 */
void
cli::allow_user
(
	const std::string& username,
	const std::string& password,
	const cmd_priv     privlevel
)
{
	auto u = m_users.find ( username );
	if ( u != m_users.end() )
	{
		LOG ( fgmp::fglog::prio::EMIT,
			"user '" << username << "' already exists!" );
		return;
	}
	m_users[username] = { password, privlevel };
} // cli::allow_user ()

//////////////////////////////////////////////////////////////////////

/**
 * Remove a user from the internal list of known users.
 * @param username The name of the user to remove.
 */
RESULT
cli::deny_user
(
	const std::string& username
)
{
	auto u = m_users.find ( username );
	if ( u == m_users.end() )
	{
		m_client << "user '" << username << "' not found!"
			<< libcli::cli_client::endl;
		return RESULT::ERROR_ANY;
	}
	m_users.erase ( u );
	return RESULT::OK;
} // cli::deny_user ()

//////////////////////////////////////////////////////////////////////

/**
 * Set a function which gets regularly called (once per second).
 * @see line_editor::set_callback
 */
void
cli::regular
(
	line_editor::std_callback callback
)
{
	m_editor.set_callback ( callback );
} // cli::regular ()

//////////////////////////////////////////////////////////////////////

/**
 * Every command can be abbreviated. For this to work, cli needs to
 * know the minimum number of characters which makes an abbreviation
 * unique. The minimum number is stored in unique_len() of the command.
 */
void
cli::build_shortest
(
	command::cmdlist& commands
)
{
	for ( const auto& c : commands )
	{
		for ( const auto& p : commands )
		{
			if ( c == p )
				continue;
			if ( c->m_privilege < p->m_privilege )
				continue;
			if ( ( c->m_mode != p->m_mode )
			&&   ( c->m_mode != CLI_MODE::ANY ) )
				continue;
			size_t len = c->compare_len (
			  p->name(), m_compare_case );
			if ( len > c->unique_len() )
			{
				if ( len == p->name().size() )
					++len;
				c->unique_len ( len );
			}
		}
		if ( c->has_children () )
			build_shortest ( c->m_children );
	}
} // cli::build_shortest ()

//////////////////////////////////////////////////////////////////////

/**
 * Compose the string which is used as the prompt for the user.
 * The prompt is composed of:<br>
 * The \ref m_hostname<br>
 * The \ref m_modestr<br>
 * The \ref m_prompt<br>
 * All strings are simply concatenated.
 */
void
cli::build_prompt
()
{
	std::string prompt;
	if ( m_hostname != "" )
		prompt += m_hostname;
	if ( m_modestr != "" )
		prompt += m_modestr;
	if ( m_prompt != "" )
		prompt += m_prompt;
	m_editor.set_prompt ( prompt );
} // cli::build_prompt ()

//////////////////////////////////////////////////////////////////////

/**
 * Set the privilege level of the current user.
 */
cmd_priv
cli::set_privilege
(
	const cmd_priv priv
)
{
	if ( priv == m_privilege )
		return priv;
	cmd_priv old = m_privilege;
	m_privilege  = priv;
	m_prompt = ( priv == PRIVLEVEL::PRIVILEGED? "# ":"> " );
	build_prompt ();
	return old;
} // cli::set_privilege ()

//////////////////////////////////////////////////////////////////////

cmd_mode
cli::set_configmode
(
	const cmd_mode mode,
	const std::string& config_desc
)
{
	if ( mode == m_mode )
		return m_mode;
	cmd_mode old = m_mode;
	m_mode = mode;
	if ( m_mode == CLI_MODE::EXEC ) // Not config mode
		m_modestr = "";
	else if ( config_desc != "" )
		m_modestr = "(config-" + config_desc + ")";
	else
		m_modestr = "(config)";
	build_prompt ();
	return old;
} // cli::set_configmode ()

//////////////////////////////////////////////////////////////////////

/**
 * Register a new \ref command to this cli.
 * @param cmd The \ref command to register.
 * @param parent A parent \ref command (can be 0)
 */
void
cli::register_command
(
	command*   cmd,
	command*   parent
)
{
	if ( ! cmd )
		return;
	if ( parent )
	{
		for ( const auto c : parent->m_children )
		{
			if ( c->name() == cmd->name() )
			{
				if ( ( cmd->m_privilege == c->m_privilege )
				&&   ( cmd->m_mode == c->m_mode )
				&&   ( cmd->m_mode != CLI_MODE::ANY ) )
					throw arg_error (
						cmd->name() +
						" : command already defined" );
			}
		}
		parent->m_children.push_back ( command::command_p ( cmd ) );
		return;
	}
	for ( const auto c : m_commands )
	{
		if ( c->name() == cmd->name() )
		{
			if ( ( cmd->m_privilege == c->m_privilege )
			&&   ( cmd->m_mode == c->m_mode )
			&&   ( cmd->m_mode != CLI_MODE::ANY ) )
				throw arg_error (
					cmd->name() +
					" : command already defined" );
		}
	}
	m_commands.push_back ( command::command_p ( cmd ) );
} // cli::register_command ()

//////////////////////////////////////////////////////////////////////

/**
 * Delete a known command
 */
void
cli::unregister_command
(
	const std::string& name
)
{
	command::cmdlist::iterator c = m_commands.begin();
	while ( c != m_commands.end() )
	{
		if ( ( *c )->name() == name )
		{
			m_commands.erase ( c );
			break;
		}
		++c;
	}
} // cli::unregister_command ()

//////////////////////////////////////////////////////////////////////

/**
 * Write the help text of a command to the user
 */
void
cli::show_help
(
	const std::string& arg,
	const std::string& desc
)
{
	m_client << align_left ( 22 ) << arg << desc << cli_client::endl;
} // show_help()

//////////////////////////////////////////////////////////////////////

/**
 * Convenience function. Handle additional arguments of a command.
 */
RESULT
cli::no_more_args
(
	const strvec& args,
	const size_t first_arg
)
{
	size_t num_args = args.size() - first_arg;
	if ( num_args > 0 )
	{
		if ( ( num_args == 1 ) && ( args[ first_arg ] == "?" ) )
		{
			show_help ( "|" , "Output modifiers" );
			return RESULT::SHOW_HELP;
		}
		return RESULT::TOO_MANY_ARGS;
	}
	return RESULT::OK;
} // cli::no_more_args ()

//////////////////////////////////////////////////////////////////////

/**
 * Convenience function. Handle additional arguments of a command.
 */
RESULT
cli::need_n_args
(
	const size_t needed_args,
	const strvec& args,
	const size_t first_arg
) const
{
	size_t num_args = args.size() - first_arg;
	if ( ( num_args == 0 )
	||   ( num_args < needed_args )   // too few
	||   ( num_args > args.size() ) ) // overflow
	{
		return RESULT::MISSING_ARG;
	}
	if ( num_args > needed_args )
		return RESULT::TOO_MANY_ARGS;
	/* this breaks get_bool() !
	if ( wants_help ( args[num_args] ) )
		return RESULT::SHOW_HELP;
	*/
	return RESULT::OK;
} // cli::need_n_args ()

//////////////////////////////////////////////////////////////////////

/** @name Prefefined Commands
 * @{
 */

//////////////////////////////////////////////////////////////////////

RESULT
cli::internal_help
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << cli_client::endl;
	m_client <<
	 "Help may be requested at any point in a command by entering\r\n"
	 "a question mark '?'.  If nothing matches, the help list will\r\n"
	 "be empty and you must backup until entering a '?' shows the\r\n"
	 "available options.\r\n"
	 "Two styles of help are provided:\r\n"
	 "1. Full help is available when you are ready to enter a\r\n"
	 "   command argument and press the <TAB> key, or enter a\r\n"
	 "   question mark.\r\n"
	 "   argument.\r\n"
	 "2. Partial help is provided when an abbreviated argument is entered"
	 "\r\n"
	 "   and you want to know what arguments match the input\r\n"
	 "   (e.g. 'show pr?'.)\r\n"
	 << cli_client::endl;
	return RESULT::OK;
} // cli::internal_help ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::internal_whoami
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << cli_client::endl;
	m_client << "You are '" << m_username << "'" << cli_client::endl;
	return RESULT::OK;
} // cli::internal_whoami ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::internal_history
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	m_client << cli_client::endl;
	m_editor.show_history ();
	return RESULT::OK;
} // cli::internal_history ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::internal_quit
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	set_privilege ( PRIVLEVEL::UNPRIVILEGED );
	set_configmode ( CLI_MODE::EXEC, "" );
	m_state = CLI_STATE::QUIT;
	return RESULT::OK;
} // cli::internal_quit ()

//////////////////////////////////////////////////////////////////////

/**
 * Return to EXEC mode.
 *
 */
RESULT
cli::internal_exit
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	set_configmode ( CLI_MODE::EXEC, "" );
	return RESULT::OK;
} // cli::internal_exit ()

//////////////////////////////////////////////////////////////////////

/**
 * Exit the current mode
 *
 */
RESULT
cli::internal_end
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	if ( m_mode == CLI_MODE::EXEC )
		return internal_quit ( command, args, first_arg );
	if ( m_mode == CLI_MODE::CONFIG )
		set_configmode ( CLI_MODE::EXEC, "" );
	else	// mode > CLI_MODE::CONFIG
		set_configmode ( CLI_MODE::CONFIG, "" );
	return RESULT::OK;
} // cli::internal_end ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::internal_pager
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	size_t num_args { args.size() - first_arg };
	size_t lines { 0 };
	if ( num_args == 0 )
	{
		if ( m_client.max_screen_lines() == 0 )
			m_client  << "pager is disabled!" << cli_client::endl;
		m_client  << "show " << m_client.max_screen_lines()
			  << " lines without pausing" << cli_client::endl;
		return RESULT::OK;
	}
	else if ( num_args == 1 )
	{
		if ( wants_help ( args[first_arg] ) )
		{
			show_help ( "<0-512>",
				    "Number of lines on screen "
				    "(0 for no pausing)" );
			show_help ( "<cr>",
				    "show current number of lines" );
			return RESULT::OK;
		}
		int invalid { -1 };
		lines = fgmp::str_to_num<size_t> ( args[first_arg], invalid );
		if ( invalid )
			return RESULT::INVALID_ARG;
	}
	else
		return no_more_args ( args, first_arg + 1 );
	m_client.max_screen_lines ( lines );
	if ( lines == 0 )
		m_client  << "pager disabled!" << cli_client::endl;
	else
		m_client  << "show " << m_client.max_screen_lines()
			  << " lines without pausing" << cli_client::endl;
	return RESULT::OK;
} // cli::internal_pager ()

//////////////////////////////////////////////////////////////////////

/**
 * Enter configuration mode
 *
 * This command is predefined by libcli but not automatically
 * registered. You have to register this command if you want to
 * use it.
 */
RESULT
cli::internal_configure
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	RESULT r = no_more_args ( args, first_arg );
	if ( r != RESULT::OK )
		return r;
	set_configmode ( CLI_MODE::CONFIG, "" );
	return RESULT::OK;
} // cli::internal_configure ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::internal_add_user
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	std::string username;
	std::string password;
	cmd_priv    privlevel { PRIVLEVEL::UNPRIVILEGED };
	size_t      arg_num { 0 };
	int e;
	for ( size_t i=first_arg; i < args.size(); i++ )
	{
		switch ( arg_num )
		{
		case 0:	// username
			if ( wants_help ( args[i] ) )
			{
				show_help ( "USERNAME",
				  "loginname of the user" );
				return RESULT::OK;
			}
			username = args[i];
			break;
		case 1: // password
			if ( wants_help ( args[i] ) )
			{
				show_help ( "PASSWORD",
				  "password of the user" );
				return RESULT::OK;
			}
			password = args[i];
			break;
		case 2: // privlevel
			if ( wants_help ( args[i] ) )
			{
				show_help ( "PRIVLEVEL",
				  "privilege level of the user" );
				return RESULT::OK;
			}
			privlevel = fgmp::str_to_num<cmd_priv> ( args[i], e );
			if ( e )
				return RESULT::INVALID_ARG;
			break;
		default:
			return no_more_args ( args, i );
		}
		arg_num++;
	}
	if ( arg_num < 2 )
		return RESULT::MISSING_ARG;
	allow_user ( username, password, privlevel );
	return RESULT::OK;
} // cli::internal_add_user ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::internal_del_user
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
		show_help ( "NAME", "delete user NAME" );
		return RESULT::ERROR_ANY;
	}
	deny_user ( args[first_arg] );
	return RESULT::OK;
} // cli::internal_del_user ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::internal_show_users
(
	const std::string& command,
	const strvec& args,
	size_t first_arg
)
{
	std::string username;
	size_t      arg_num { 0 };
	for ( size_t i=first_arg; i < args.size(); i++ )
	{
		switch ( arg_num )
		{
		case 0:	// username
			if ( wants_help ( args[i] ) )
			{
				show_help ( "USERNAME",
				  "show users which match USERNAME" );
				return RESULT::OK;
			}
			username = args[i];
			break;
		default:
			return no_more_args ( args, i );
		}
		arg_num++;
	}
	for ( auto u : m_users )
	{
		if ( ! fgmp::compare ( username, u.first, m_compare_case ) )
			continue;
		m_client << u.first << " "
			<< u.second.password << " "
			<< u.second.privlevel << cli_client::endl;
	}
	return RESULT::OK;
} // cli::internal_show_users ()

//////////////////////////////////////////////////////////////////////

/**
 * @}
 */

//////////////////////////////////////////////////////////////////////

cli::cli
(
	int fd
) : m_client ( fd ), m_editor ( m_client )
{
	set_privilege ( PRIVLEVEL::UNPRIVILEGED );
	set_configmode ( CLI_MODE::EXEC, "" );
	using namespace std::placeholders;
	#define _ptr(X) (std::bind (& X, this, _1, _2, _3))
	register_command ( new command (
		"help",
		_ptr ( cli::internal_help ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Description of the interactive help system"
	) );
	register_command ( new command (
		"whoami",
		_ptr ( cli::internal_whoami ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::EXEC,
		"Show who you are"
	) );
	register_command ( new command (
		"quit",
		_ptr ( cli::internal_quit ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::EXEC,
		"Disconnect"
	) );
	register_command ( new command (
		"history",
		_ptr ( cli::internal_history ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::EXEC,
		"Show a list of previously run commands"
	) );
	register_command ( new command (
		"pager",
		_ptr ( cli::internal_pager ),
		PRIVLEVEL::UNPRIVILEGED,
		CLI_MODE::ANY,
		"Set number of lines on a screen"
	) );
	#undef _ptr
} // cli::cli ()

//////////////////////////////////////////////////////////////////////

cli::~cli
()
{
	m_commands.clear();
} // cli::~cli ()

//////////////////////////////////////////////////////////////////////

/**
 * Parse the command line into tokens delimited by spaces.
 * @return a vector of strings containing the tokens.
 */
RESULT
cli::parse_line
(
	const std::string& line,
	strvec& commands,
	strvec& filters
)
{
	size_t pos { 0 };
	bool   in_filter { false };
	while ( pos < line.size() )
	{
		size_t start = line.find_first_not_of ( " \t\f\n\r\v", pos );
		size_t end;
		if ( start == std::string::npos )
			return RESULT::OK;
		if ( line[start] == '|' )
		{
			if ( in_filter )
				filters.push_back ( "|" );
			else
				in_filter = true;
			pos = start + 1;
			continue;
		}
		if ( '#' ==  line[start] )
			return RESULT::OK;
		if ( is_quote ( line[start] ) )
		{
			char q = line[start];
			++start;
			end = line.find ( q, start );
			if ( end == std::string::npos )
			{
				m_client << "unterminated quote"
					 << cli_client::endl;
				commands.clear ();
				return RESULT::INVALID_ARG;
			}
			if ( in_filter )
				filters.push_back (
					line.substr ( start, end - start ) );
			else
				commands.push_back (
					line.substr ( start, end - start ) );
			pos = end + 1;
			continue;
		}
		end = line.find_first_of ( " \t\f\n\r\v|#\"'", start );
		if ( in_filter )
			filters.push_back (
				line.substr ( start, end - start ) );
		else
			commands.push_back (
				line.substr ( start, end - start ) );
		if ( start == end )
			return RESULT::OK;
		pos = end;
	}
	return RESULT::OK;
} // cli::parse_line ()

//////////////////////////////////////////////////////////////////////

/**
 * @return true if the command is available with current
 * mode and privilege level of the user
 */
bool
cli::command_available
(
	const command::command_p cmd
) const
{
	if ( m_privilege < cmd->m_privilege )
		return false;
	if ( ( m_mode != cmd->m_mode ) && ( cmd->m_mode != CLI_MODE::ANY ) )
		return false;
	return true;
} // cli::command_available ()

//////////////////////////////////////////////////////////////////////

/**
 * Get all possible completions of a filter. If there is only one
 * completion, replace the current word of the command line with it.
 */
void
cli::get_filter_completions
(
	const strvec& filters,
	const size_t start_word,
	const char last_char
)
{
	if ( start_word > filters.size() )
		return;
	size_t current  { start_word };
	size_t word_num;
	bool   complete; // filter has all needed arguments?
	bool found { false };
	while ( current < filters.size() )
	{
		word_num = current;
		std::string word { filters [ current ] };
		if ( word  == "|" )
		{
			++current;
			continue;
		}
		for ( auto f : filter_cmds )
		{
			complete = false;
			if ( ! fgmp::compare ( word, f.name, m_compare_case ) )
				continue;
			found = true;
			if ( word.size () < f.unique_len )
			{
				show_help ( f.name, f.help );
				continue;
			}
			// found a unique filter name
			if ( current != filters.size() - 1 )
				// only expand at end of line
				continue;
			if ( ( f.name != word ) || ( last_char != ' ' ) )
			{
				// needs expansion
				m_editor.replace_word ( f.name + ' ' );
				return;
			}
			// eat up arguments
			size_t num_args { 0 };
			while ( current + 1 < filters.size() )
			{
				++current;
				if ( filters [ current ] == "|" )
					break;
				++num_args;
				if ( num_args == f.num_args )
					break;
			}
			if ( num_args < f.num_args )
			{
				m_client << "'" << f.name << "' needs "
					 << f.num_args << " arguments"
					 << cli_client::endl;
				return;
			}
			// we have the filter and all needed arguments
			complete = true;
			break;
		}
		if ( ! found )
			break;
		++current;
	}
	if ( ! found )
	{
		m_client << "invalid filter: '" << filters[word_num]
			 << "'" << cli_client::endl;
	}
} // cli::get_filter_completions ()

//////////////////////////////////////////////////////////////////////

/**
 * Get all possible completions of the current word (command).
 * If there is only one completion, replace the current word of
 * the command line with it.
 */
void
cli::get_completions
(
	const command::cmdlist& cmds,
	const strvec& words,
	const size_t start_word,
	char last_char
)
{
	if ( start_word >= words.size() )
	{
		for ( auto c : cmds )
		{
			if ( ! command_available ( c ) )
				continue;
			show_help ( c->name(), c->help() );
		}
		return;
	}
	std::string word { words [ start_word ] };
	for ( auto c : cmds )
	{
		if ( ! command_available ( c ) )
			continue;
		if ( ! c->compare ( word, m_compare_case ) )
			continue;
		// found a valid completion
		if ( word.size () < c->unique_len() )
		{
			// not unique
			show_help ( c->name(), c->help() );
			continue;
		}
		if ( ( last_char != ' ' )       // at the end of a word
		&&   ( start_word + 1 < words.size() ) ) // more words to go
		{
			if ( c->has_children () )
			{
				// found the command, now list possible
				// sub commands
				get_completions ( c->m_children, words,
						  start_word + 1, last_char );
			}
			return;
		}
		if ( last_char == ' ' )
		{
			// word completed, list subcommands
			if ( c->has_children () )
			{
				// found the command, now list possible
				// sub commands
				get_completions ( c->m_children, words,
						  start_word + 1, last_char );
			}
			return;
		}
		// at the end of the word
		m_editor.replace_word ( c->name() + ' ' );
		return;
	}
} // cli::get_completions ()

//////////////////////////////////////////////////////////////////////

/** @name Parsing and activating filters
 *
 * @{
 */

//////////////////////////////////////////////////////////////////////

RESULT
cli::install_begin_filter
(
	const strvec& filters,
	const size_t start
)
{
	RESULT r { need_n_args ( 1, filters, start ) };
	if ( RESULT::MISSING_ARG == r )
	{
		show_help ( "WORD", "start output with line including WORD" );
		return RESULT::ERROR_ANY;;
	}
	if ( RESULT::TOO_MANY_ARGS == r )
	{
		if ( "|" != filters[start + 1] )
			return r;
	}
	if ( wants_help ( filters[start] ) )
	{
		show_help ( "WORD", "show only lines including WORD" );
		return RESULT::ERROR_ANY;;
	}
	m_client.register_filter ( new begin_filter ( filters[start] ) );
	return RESULT::OK;
} // cli::install_exclude_filter ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::install_between_filter
(
	const strvec& filters,
	const size_t start
)
{
	std::string start_word;
	std::string end_word;
	size_t arg_num { 0 };
	for ( size_t i=start; i < filters.size(); i++ )
	{
		switch ( arg_num )
		{
		case 0: // start_word
			if ( wants_help ( filters[i] ) )
			{
				show_help ( "WORD",
				  "start output with line containing WORD" );
				return RESULT::ERROR_ANY;;
			}
			start_word = filters[i];
			break;
		case 1: // endword
			if ( wants_help ( filters[i] ) )
			{
				show_help ( "WORD",
				  "end output with line containing WORD" );
				return RESULT::ERROR_ANY;;
			}
			break;
		default:
			if ( filters[i] == "|" )        // next filter
			{
				i = filters.size() + 1;
				break;
			}
			return no_more_args ( filters, i );
		}
		++arg_num;
	}
	if ( arg_num < 2 )
	{
		return RESULT::MISSING_ARG;
	}
	m_client.register_filter (
		new between_filter ( start_word, end_word ) );
	return RESULT::OK;
} // cli::install_between_filter ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::install_count_filter
(
	const strvec& filters,
	const size_t start
)
{
	RESULT r = no_more_args ( filters, start );
	if ( RESULT::TOO_MANY_ARGS == r )
	{
		if ( "|" != filters[start] )
			return r;
	}
	if ( RESULT::SHOW_HELP == r )
		return r;
	m_client.register_filter ( new count_filter ( & m_client ) );
	return RESULT::OK;
} // cli::install_count_filter ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::install_exclude_filter
(
	const strvec& filters,
	const size_t start
)
{
	RESULT r { need_n_args ( 1, filters, start ) };
	if ( RESULT::MISSING_ARG == r )
	{
		show_help ( "WORD", "show only lines including WORD" );
		return RESULT::ERROR_ANY;;
	}
	if ( RESULT::TOO_MANY_ARGS == r )
	{
		if ( "|" != filters[start + 1] )
			return r;
	}
	if ( wants_help ( filters[start] ) )
	{
		show_help ( "WORD", "show only lines including WORD" );
		return RESULT::ERROR_ANY;;
	}
	m_client.register_filter ( new match_filter ( filters[start], true ) );
	return RESULT::OK;
} // cli::install_exclude_filter ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::install_include_filter
(
	const strvec& filters,
	const size_t start
)
{
	RESULT r { need_n_args ( 1, filters, start ) };
	if ( RESULT::MISSING_ARG == r )
	{
		show_help ( "WORD", "show only lines including WORD" );
		return RESULT::ERROR_ANY;;
	}
	if ( RESULT::TOO_MANY_ARGS == r )
	{
		if ( "|" != filters[start + 1] )
			return r;
	}
	if ( wants_help ( filters[start] ) )
	{
		show_help ( "WORD", "show only lines including WORD" );
		return RESULT::ERROR_ANY;;
	}
	m_client.register_filter ( new match_filter ( filters[start], false ) );
	return RESULT::OK;
} // cli::install_include_filter ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::install_pager_filter
(
	const strvec& filters,
	const size_t start
)
{
	RESULT r { need_n_args ( 1, filters, start ) };
	if ( RESULT::MISSING_ARG == r )
	{
		show_help ( "LINE", "prompt every LINE lines" );
		return RESULT::ERROR_ANY;;
	}
	if ( RESULT::TOO_MANY_ARGS == r )
	{
		if ( "|" != filters[start + 1] )
			return r;
	}
	if ( wants_help ( filters[start] ) )
	{
		show_help ( "LINE", "prompt every LINE lines" );
		return RESULT::ERROR_ANY;;
	}
	int e;
	size_t v { fgmp::str_to_num<size_t> ( filters[start], e ) };
	if ( e )
		return RESULT::INVALID_ARG;
	m_client.register_filter ( new pager_filter (  & m_client, v ) );
	return RESULT::OK;
} // cli::install_pager_filter ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::install_file_filter
(
	const strvec& filters,
	const size_t start
)
{
	bool append;
	std::string filename;
	size_t arg_num { 0 };
	for ( size_t i=start; i < filters.size(); i++ )
	{
		switch ( arg_num )
		{
		case 0: // append|replace
			if ( wants_help ( filters[i] ) )
			{
				show_help ( "append", "append to file" );
				show_help ( "replace", "relpace file" );
				return RESULT::ERROR_ANY;;
			}
			if ( fgmp::compare ( filters[i],
				"append", m_compare_case ) )
				append = true;
			else if ( fgmp::compare ( filters[i],
				"replace", m_compare_case ) )
				append = false;
			else
				return RESULT::INVALID_ARG;
			break;
		case 1: // filename
			if ( wants_help ( filters[i] ) )
			{
				show_help ( "filename",
					    "write output to 'filename'" );
				return RESULT::ERROR_ANY;;
			}
			filename = filters[i];
			break;
		default:
			if ( filters[i] == "|" )        // next filter
			{
				i = filters.size() + 1;
				break;
			}
			return no_more_args ( filters, i );
		}
		++arg_num;
	}
	if ( arg_num < 2 )
		return RESULT::MISSING_ARG;
	try
	{
		m_client.register_filter (
			new file_filter ( filename, append ) );
	}
	catch ( std::runtime_error& e )
	{
		m_client << "% " << e.what() << cli_client::endl;
		return RESULT::ERROR_ANY;
	}
	return RESULT::OK;
} // cli::install_file_filter ()

//////////////////////////////////////////////////////////////////////

/**
 * Parse through 'filters' and register the filters with
 * m_client.
 */
std::pair<libcli::RESULT,std::string>
cli::install_filter
(
	const strvec& filters
)
{
	using std::make_pair;
	using std::string;
	strvec::const_iterator f { filters.begin() };
	if ( *f == "?" )
	{
		// nothing but a '?' => list all possible filters
		for ( auto f : filter_cmds )
			show_help ( f.name, f.help );
		return make_pair ( RESULT::ERROR_ANY, "" );
	}
	size_t num { 1 };
	while ( f != filters.end() )
	{
		if ( *f == "|" ) // nect filter
		{
			++f;
			++num;
			continue;
		}
		if ( wants_help ( *f ) ) // handle '?'
		{
			std::string word { *f, 0, ( *f ).size() - 1 };
			for ( auto t : filter_cmds )
			{
				if ( ! fgmp::compare ( word, t.name,
					m_compare_case ) )
					continue;
				show_help ( t.name, t.help );
			}
			return make_pair ( RESULT::ERROR_ANY, "" );
		}
		if ( fgmp::compare ( *f, "begin", m_compare_case ) )
		{
			RESULT r = install_begin_filter ( filters, num );
			if ( RESULT::OK != r )
				return make_pair ( r, "begin" );
			f   += 2; // 'begin'  + 'arg1'
			num += 2;
			continue;
		}
		if ( fgmp::compare ( *f, "between", m_compare_case ) )
		{
			RESULT r = install_between_filter ( filters, num );
			if ( RESULT::OK != r )
				return make_pair ( r, "between" );
			f   += 3; // 'between'  + 'arg1' + 'arg2'
			num += 3;
			continue;
		}
		if ( fgmp::compare ( *f, "count", m_compare_case ) )
		{
			RESULT r = install_count_filter ( filters, num );
			if ( RESULT::OK != r )
				return make_pair ( r, "count" );
			f   += 1; // 'count'
			num += 1;
			continue;
		}
		if ( fgmp::compare ( *f, "include", m_compare_case ) )
		{
			RESULT r = install_include_filter ( filters, num );
			if ( RESULT::OK != r )
				return make_pair ( r, "include" );
			f   += 2; // 'include' + 'arg1'
			num += 2;
			continue;
		}
		if ( fgmp::compare ( *f, "exclude", m_compare_case ) )
		{
			RESULT r = install_exclude_filter ( filters, num );
			if ( RESULT::OK != r )
				return make_pair ( r, "exclude" );
			f   += 2; // 'exclude' + 'arg1'
			num += 2;
			continue;
		}
		if ( fgmp::compare ( *f, "pager", m_compare_case ) )
		{
			RESULT r = install_pager_filter ( filters, num );
			if ( RESULT::OK != r )
				return make_pair ( r, "pager" );
			f   += 2; // 'pager' + 'arg1'
			num += 2;
			continue;
		}
		if ( fgmp::compare ( *f, "file", m_compare_case ) )
		{
			RESULT r = install_file_filter ( filters, num );
			if ( RESULT::OK != r )
				return make_pair ( r, "file" );
			f   += 3; // 'file' + 'arg1' + 'arg2'
			num += 3;
			continue;
		}
		m_client << "unkown filter '" << *f << "'" << cli_client::endl;
		return make_pair ( RESULT::INVALID_COMMAND, *f );
	}
	return make_pair ( RESULT::OK, "" );
} // cli::install_filter ()

//////////////////////////////////////////////////////////////////////

/**
 * @}
 */

//////////////////////////////////////////////////////////////////////

/**
 * Execute the command line.
 */
std::pair <libcli::RESULT, std::string>
cli::exec_command
(
	const command::cmdlist& cmds,
	const strvec& words,
	size_t& start_word
)
{
	using std::make_pair;
	using std::string;
	if ( start_word >= words.size() )
		return make_pair ( RESULT::ERROR_ANY, "" );
	std::string word { words [ start_word ] };
	size_t num_words = words.size() - 1;
	// deal with ? for help
	if ( word == "?" )
	{
		for ( auto c : cmds )
		{
			if ( ! command_available ( c ) )
				continue;
			show_help ( c->name(), c->help() );
		}
		return make_pair ( RESULT::OK, "" );
	}
	int num_commands = 0;
	for ( auto c : cmds )
	{
		if ( ! command_available ( c ) )
			continue;
		if ( wants_help ( word ) )
		{
			if ( ! c->compare( word,m_compare_case,word.size()-1 ) )
				continue;
			show_help ( c->name(), c->help() );
			if ( word.size () - 1 < c->unique_len() )
			{
				++num_commands;
				continue;
			}
			return make_pair ( RESULT::OK, "" );
		}
		if ( ! c->compare ( word, m_compare_case ) )
			continue;
		if ( word.size () < c->unique_len() )
		{
			++num_commands;
			continue;
		}
		if ( ! c->compare ( word, m_compare_case, c->unique_len() ) )
			continue;
		// found the command
		if ( start_word < num_words )
		{
			++start_word;
			if ( c->has_children () )
				return exec_command (
				  c->m_children, words, start_word );
			if ( ! c->has_callback () )
			{
				m_client << "No callback for '"
				  << c->name() << "'" << cli_client::endl;
				return make_pair( RESULT::ERROR_ANY,c->name() );
			}
			// found the command and there are arguments left
			// on the command line
			RESULT r = (*c) ( c->name(), words, start_word );
			if ( r == RESULT::SHOW_HELP )
				show_help ( "<cr>", c->help() );
			return make_pair ( r, c->name() );
		}
		if ( ( c->has_children() ) && ( start_word == num_words ) )
		{
			if ( c->has_callback () )
			{
				++start_word;
				return make_pair (
				  (*c) ( c->name(), words, start_word ),
				  c->name() );
			}
			m_client << "# Incomplete command" << cli_client::endl;
			return make_pair ( RESULT::ERROR_ANY, c->name() );
		}
		// found the command and there are no arguments left on
		// the command line
		++start_word; // words.size() - start_word == number of commands
		return make_pair (
		  (*c) ( c->name(), words, start_word ), c->name() );
	}
	if ( num_commands )
	{
		m_client << "'" << word << "' is ambigous" << cli_client::endl;
		return make_pair ( RESULT::ERROR_ANY, "" );
	}
	return make_pair ( RESULT::INVALID_COMMAND, word );
} // cli::exec_command ()

//////////////////////////////////////////////////////////////////////

void
cli::list_completions
()
{
	std::string s { m_editor.get_line() };
	strvec commands;
	strvec filters;
	RESULT r { parse_line ( s, commands, filters ) };
	if ( RESULT::OK != r )
		return;
	if ( commands.size() == 0 ) // command line is empty
		return;
	m_client << cli_client::endl;
	if ( m_editor.cursor_pos() != m_editor.size() )
	{
		// We are not at the end of the line
		// completion currently not possible.
		// This test fails if there are more than
		// one space at the end.
		return;
	}
	if ( filters.size() > 0 )
	{
		get_filter_completions ( filters, 0,
		  m_editor.char_before_cursor() );
		return;
	}
	get_completions ( m_commands,commands,0,m_editor.char_before_cursor() );
} // cli::list_completions ()

//////////////////////////////////////////////////////////////////////

RESULT
cli::run_command
(
	const std::string& line
)
{
	std::string s { line };
	strvec commands;
	strvec filters;
	RESULT r { parse_line ( s, commands, filters ) };
	if ( RESULT::OK != r )
		return r;
	if ( commands.size() == 0 ) // command line is empty
		return r;
	std::pair <RESULT, std::string> res;
	res = std::make_pair ( RESULT::OK, "" );
	if ( filters.size() > 0 )
		res = install_filter ( filters );
	if ( RESULT::OK == res.first )
	{
		size_t arg_ptr { 0 };
		r = RESULT::OK;
		m_client.enable_filters ();
		try
		{
			res = exec_command ( m_commands, commands, arg_ptr );
		}
		catch ( pager_wants_quit )
		{
			/* nothing to do */
		}
		m_client.disable_filters();
		r = res.first;
	}
	switch ( res.first )
	{
	case RESULT::INVALID_ARG:
		m_client << "% " << res.second
			 << " : Invalid argument" << cli_client::endl;
		break;
	case RESULT::TOO_MANY_ARGS:
		m_client << "% " << res.second <<
			 " : too many arguments" << cli_client::endl;
		break;
	case RESULT::MISSING_ARG:
		m_client << "% " << res.second
			 << " : missing argument" << cli_client::endl;
		break;
	case RESULT::INVALID_COMMAND:
		m_client << "% " << res.second
			 << " : invalid command" << cli_client::endl;
		break;
	default:
		break;
	}
	return res.first;
} // cli::run_command ()

//////////////////////////////////////////////////////////////////////

/**
 * Can be used as a configuration parser
 */
RESULT
cli::file
(
	const std::string& filename,
	const cmd_priv privilege,
	const cmd_mode mode
)
{
	std::ifstream infile { filename };
	if ( ! infile )
		return RESULT::INVALID_ARG;
	cmd_priv oldpriv = set_privilege ( privilege );
	cmd_mode oldmode = set_configmode ( mode, "" );
	std::string line;
	size_t line_number { 1 };
	for ( std::string line; getline ( infile, line ); )
	{
		RESULT r { run_command ( line ) };
		if ( RESULT::OK != r )
		{
			m_client << "  ... in line number "
				 << line_number
				 << cli_client::endl;
		}
		++line_number;
	}
	set_privilege ( oldpriv );
	set_configmode ( oldmode, m_modestr );
	return RESULT::OK;
} // cli::file

//////////////////////////////////////////////////////////////////////

// a little helper function
bool
cli::wants_help
(
	const std::string& arg
)
{
	size_t l { arg.size() };
	if ( l == 0 )
		return false;
	if ( arg[l-1] == '?' )
		return true;
	return false;
} // cli::wants_help ()

//////////////////////////////////////////////////////////////////////

// to distinguish clear text from DES crypted
const std::string DES_PREFIX = "{crypt}";
// to distinguish clear text from MD5 crypted
const std::string MD5_PREFIX = "$1$";

/**
 * Compare two passwords.
 *
 * Can handle md5 hashes and DES encrypted passwords.
 * @return true if the passwords match.
 * @return false if they don't match
 */
bool
cli::pass_matches
(
	const std::string& pass,
	const std::string& tried_pass
) const
{
	int des;
	int idx = 0;
	des = ! pass.compare ( 0, DES_PREFIX.size(), DES_PREFIX );
	if ( des )
		idx = sizeof ( DES_PREFIX )-1;
	std::string trial;
	if ( des || ( ! pass.compare ( 0, MD5_PREFIX.size(), MD5_PREFIX ) ) )
	{
		trial = crypt (
		  ( char* ) tried_pass.c_str(), ( char* ) pass.c_str() );
	}
	else
		trial = tried_pass;
	return ( pass.compare ( idx, pass.size(), trial ) == 0 );
} // cli::pass_matches ()

//////////////////////////////////////////////////////////////////////

/**
 * Check username/password against internal userlist or an
 * authentication callback.
 *
 * @return -1 if the username/password was wrong.
 * @return >= 0 the privilege level granted to the user.
 */
int
cli::check_user_auth
(
	const std::string& username,
	const std::string& password
) const
{
	// check callback
	if ( m_auth_callback )
		return m_auth_callback ( username, password );
	// check internal userlist
	auto u = m_users.find ( username );
	if ( u != m_users.end() )
	{
		if ( pass_matches ( u->second.password, password ) )
			return u->second.privlevel;
	}
	return PRIVLEVEL::DENIED;
} // cli::check_user_auth ()

//////////////////////////////////////////////////////////////////////

void
cli::leave_config_mode
()
{
	if ( m_mode != CLI_MODE::EXEC )
	{
		m_client << cli_client::endl;
		set_configmode ( CLI_MODE::EXEC, "" );
	}
} // cli::leave_config_mode ()

//////////////////////////////////////////////////////////////////////

std::pair <libcli::RESULT, bool>
cli::get_bool
(
	const std::string& arg
)
{
	if ( wants_help ( arg ) )
	{
		show_help ( "enable|on|true", "enable this feature");
		show_help ( "disable|off|false", "disable this feature");
		return std::make_pair<RESULT,bool> ( RESULT::ERROR_ANY, false );
	}
	if ( arg == "" )
		return std::make_pair<RESULT,bool>(RESULT::MISSING_ARG,false);
	if ( fgmp::compare ( arg, "enable", m_compare_case )
	||   fgmp::compare ( arg, "true", m_compare_case )
	||   fgmp::compare ( arg, "on", m_compare_case ) )
		return std::make_pair<RESULT, bool> ( RESULT::OK, true );
	if ( fgmp::compare ( arg, "disable", m_compare_case )
	||   fgmp::compare ( arg, "false", m_compare_case )
	||   fgmp::compare ( arg, "off", m_compare_case ) )
		return std::make_pair<RESULT, bool> ( RESULT::OK, false );
	return std::make_pair<RESULT, bool> ( RESULT::INVALID_ARG, false );
} // cli::get_bool ()

//////////////////////////////////////////////////////////////////////

/**
 * Ask the user for username and password.
 * @return true if the authentication succeeded.
 * @return false if the credentials were wrong.
 */
bool
cli::authenticate_user
()
{
	std::string password;
	int c;
	int counter { 0 };
	while ( m_state != CLI_STATE::NORMAL )
	{
		m_client << "Username: " << cli_client::flush;
		c = m_editor.read_line ();
		m_username = m_editor.get_line ();
		m_editor.do_echo ( false );
		m_client << "Password: " << cli_client::flush;
		c = m_editor.read_line ();
		password = m_editor.get_line ();
		m_editor.do_echo ( true );
		int priv = check_user_auth ( m_username, password );
		if ( priv != PRIVLEVEL::DENIED )
		{
			m_client << "-ok-" << cli_client::endl;
			set_privilege ( priv );
			return true;
		}
		else
		{
			m_client << cli_client::endl
				 << "Access denied."
				 << cli_client::endl << cli_client::endl;
			using fgmp::fglog;
			LOG ( fglog::prio::URGENT,
			      "Bad login '" << m_username << "' / '"
			      << password << "'" );
			m_state = CLI_STATE::LOGIN;
			++counter;
			if ( counter == 3 )
				return false;
		}
	}
	return true;
} // cli::authenticate_user ()

//////////////////////////////////////////////////////////////////////

/**
 * The main loop of the cli
 */
void
cli::loop
()
{
	build_shortest ( m_commands );
	if ( m_banner.size() > 0 )
		m_client << m_banner << cli_client::endl;
	// authentication required ?
	if ( ( ! m_users.empty() ) || ( m_auth_callback != nullptr ) )
	{
		m_editor.do_prompt ( false );
		if ( ! authenticate_user () )
			return;
		m_editor.do_prompt ( true );
	}
	else	// start off in privileged mode
		set_privilege  ( PRIVLEVEL::PRIVILEGED );
	set_configmode ( CLI_MODE::EXEC, "" );
	m_state = CLI_STATE::NORMAL;
	m_client << "type '?' or 'help' for help."
		 << cli_client::endl << cli_client::endl;
	bool clear_buffer = true;
	do
	{
		int c = m_editor.read_line ( clear_buffer );
		switch ( c )
		{
		case KEY::LF:
			// find and execute command
			run_command ( m_editor.get_line () );
			clear_buffer = true;
			break;
		case KEY::TAB:
			// find completions
			list_completions ();
			clear_buffer = false;
			break;
		case '?':
			// show help
			run_command ( m_editor.get_line() );
			clear_buffer = false;
			break;
		}
	} while ( m_state != CLI_STATE::QUIT );
} // cli::loop ()

//////////////////////////////////////////////////////////////////////

} // namespace libcli

