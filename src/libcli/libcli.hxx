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
 * @file        libcli.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2011/2018
 */

#ifndef _libcli_header
#define _libcli_header

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <utility>
#include <stdarg.h>
#ifndef _MSC_VER
#include <termios.h>
#endif
#include "common.hxx"
#include "command.hxx"
#include "cli_client.hxx"
#include "cli_line.hxx"

namespace libcli
{

struct cli_users
{
	std::string password;
	cmd_priv    privlevel;
}; // struct cli_users

/** @defgroup libcli The CLI Library */
/** @{ */

/**
 *
 * @brief The main cli class, which draws together all parts.
 */
class cli
{
public:
	using string = std::string;
	using userlist = std::map < string, cli_users >;
	using auth_func = std::function <cmd_priv ( const string&, const string& ) >;
	using enable_func = std::function< int ( const string& ) >;
	using regular_callback = std::function< int () >;
	using filterlist  = std::vector<filter>;

	cli ( int fd );
	~cli ();
	void register_command ( command* cmd, command* parent = 0 );
	void unregister_command ( const string& name );
	RESULT run_command ( const string& line );
	void loop ();
	RESULT file (
		const string& filename,
		const cmd_priv privilege,
		const cmd_mode mode
	);
	void allow_user (
		const string& username,
		const string& password,
		const cmd_priv privlevel
	);
	void deny_user ( const string& username );
	void regular ( line_editor::std_callback callback );
	void show_help ( const string& arg, const string& desc );
	cmd_priv set_privilege ( const cmd_priv privilege );
	cmd_mode set_configmode (
		const cmd_mode mode,
		const string& config_desc
	);
	inline void set_prompt   ( const string& prompt );
	inline void set_hostname ( const string& hostname );
	inline void set_modestr  ( const string& modestring );
	inline void set_auth_callback ( auth_func callback );
	inline void set_enable_callback ( enable_func callback );
	inline void set_enable_password ( const string& password );
	inline void set_banner ( const string& banner );
	inline void set_compare_case ( bool cmpcase );
	RESULT internal_enable (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_disable (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_help (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_whoami (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_history (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_quit (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_pager (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_configure (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_exit (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_end (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_add_users (
		const string& command,
		const strvec& args,
		size_t first_arg
		);
	RESULT internal_show_users (
		const string& command,
		const strvec& args,
		size_t first_arg
		);

protected:
	std::pair <libcli::RESULT,bool> get_bool ( const std::string& arg );
	/// The different states of the cli
	enum class CLI_STATE
	{
		LOGIN,                  ///< ask for login name
		PASSWORD,               ///< ask for password
		NORMAL,                 ///< in normal operation mode
		QUIT                    ///< user requested to quit the cli
	};
	cli_client m_client;
	void build_prompt ();
	void build_shortest ( command::cmdlist& commands );
	RESULT parse_line (
		const string& line,
		strvec& commands,
		strvec& filters
		);
	bool command_available ( const command::command_p cmd ) const;
	std::pair <libcli::RESULT, std::string> install_filter (
		const strvec& filter_cmds
		);
	std::pair <libcli::RESULT, std::string> exec_command (
		const command::cmdlist& cmds,
		const strvec& words,
		size_t& start_word
		);
	void get_completions (
		const command::cmdlist& cmds,
		const strvec& words,
		const size_t start_word,
		char lastchar
		);
	void get_filter_completions (
		const strvec& words,
		const size_t start_word,
		const char lastchar
		);
	bool pass_matches (
		const string& pass,
		const string& tried_pass
		) const;
	void show_prompt ();

	RESULT no_more_args (
		const strvec& args,
		const size_t first_arg
		);
	RESULT need_n_args (
		const size_t needed_args,
		const strvec& args,
		const size_t first_arg
		) const;

	int  check_enable ( const string& pass ) const;
	int  check_user_auth (
		const string& username,
		const string& password
		) const;
	void leave_config_mode ();
	void list_completions ();
	bool wants_help ( const string& arg );
	bool authenticate_user ();
	RESULT install_begin_filter (
		const strvec& filters,
		const size_t start );
	RESULT install_between_filter (
		const strvec& filters,
		const size_t start );
	RESULT install_count_filter (
		const strvec& filters,
		const size_t start );
	RESULT install_exclude_filter (
		const strvec& filters,
		const size_t start );
	RESULT install_include_filter (
		const strvec& filters,
		const size_t start );
	RESULT install_pager_filter (
		const strvec& filters,
		const size_t start );
	RESULT install_file_filter (
		const strvec& filters,
		const size_t start );
	line_editor m_editor;		///< internal line editor
	std::string m_username;		///< login name of user
	bool m_compare_case  = false;
	std::string m_prompt;		///< current prompt
	std::string m_hostname;		///< part of the prompt
	std::string m_modestr;		///< part of the prompt
	std::string m_banner;	///< presented when connecting to the cli
	std::string m_enable_password;
	int m_privilege = -1;		///< current privilege level of the user
	int m_mode  = -1;		///< current 'mode' of the cli
	CLI_STATE   m_state;		///< current state of the cli
	userlist    m_users;		///< internally known users
	auth_func   m_auth_callback   = nullptr;
	enable_func m_enable_callback = nullptr;
	command::cmdlist m_commands;	///< list of known commands
};

//////////////////////////////////////////////////////////////////////

/** @} */

//////////////////////////////////////////////////////////////////////
//
// inline methods
//
//////////////////////////////////////////////////////////////////////

/**
 * Set the base prompt.
 * @see build_prompt
 */
void
cli::set_prompt
(
	const std::string& prompt
)
{
	m_prompt = prompt;
	build_prompt ();
}

//////////////////////////////////////////////////////////////////////

/**
 * Set the hostname.
 * @see build_prompt
 */
void
cli::set_hostname
(
	const std::string& hostname
)
{
	m_hostname = hostname;
	build_prompt ();
}

//////////////////////////////////////////////////////////////////////

/**
 * Set the modestring (eg. '-config'.
 * @see build_prompt
 */
void
cli::set_modestr
(
	const std::string& modestr
)
{
	m_modestr = modestr;
	build_prompt ();
}

//////////////////////////////////////////////////////////////////////

/**
 * Set an authentication callback. Can be used to authenticate via
 * RADIUS or TACACS.
 */
void
cli::set_auth_callback
(
	auth_func callback
)
{
	m_auth_callback = callback;
} // cli::set_auth_callback ()

//////////////////////////////////////////////////////////////////////

/**
 * Set an authentication callback for the enable password.
 * Can be used to authenticate via RADIUS or TACACS.
 */
void
cli::set_enable_callback
(
	enable_func callback
)
{
	m_enable_callback = callback;
} // cli::set_enable_callback ()


//////////////////////////////////////////////////////////////////////

void
cli::set_enable_password
(
	const std::string& password
)
{
	m_enable_password = password;
} // cli::set_enable_password ()

//////////////////////////////////////////////////////////////////////

/**
 * Set a banner. The banner is presented to the user when connecting
 * to the cli.
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

/**
 * When set to 'false' commands are case insensitive.<br>
 * When set to 'true' commands are case sensitive.
 */
void
cli::set_compare_case
(
	bool cmpcase
)
{
	m_compare_case = cmpcase;
} // cli::set_compare_case ()

//////////////////////////////////////////////////////////////////////

} // namespace libcli

#endif
