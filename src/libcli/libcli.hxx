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
// along with this program; if not, see http://www.gnu.org/licenses/
//
// derived from libcli by David Parrish (david@dparrish.com)
// Copyright (C) 2011-2021  Oliver Schroeder
//

#ifndef libcli_H
#define libcli_H

#include <string>
#include <map>
#include <vector>
#include <stdarg.h>
#include <functional>
#ifndef _MSC_VER
#include <termios.h>
#endif
#include "cli_client.hxx"
#include "common.hxx"
#include "command.hxx"
#include "editor.hxx"

namespace libcli
{

class cli
{
public:
	/**
	 * @brief States of the cli
	 *
	 * If m_state is > ENABLE_PASSWORD we haved passed authentication.
	 * This is a normal enum, so you can define your own states.
	 */
	enum class STATE
	{
		LOGIN,				///< ask for a username
		PASSWORD,			///< ask for user password
		ENABLE_PASSWORD,	///< ask for the enable password
		NORMAL,				///< normal operation state
		ENABLE				///< user is in enable mode, FIXME: needed?
	};
	using user_map = std::map<std::string, std::string>;
	using cmd_list = command::cmd_list;
	using hist_list = std::vector<std::string>;

	cli ( int fd );
	~cli ();
	void	register_command ( command& parent, const command& cmd );
	void	register_command ( const command& cmd );
	void	unregister_command ( const std::string& cmd );
	void	register_filter ( const command& filter_cmd );
	command& get_command ( const std::string& name, cmd_list& commands ) noexcept(false);
	RESULT	run_command ( const std::string& cmd );
	RESULT	loop ();
	RESULT	read_file ( const std::string& filename, int privilege, int mode );
	void	set_auth_callback ( std::function <int ( const std::string&, const std::string& )> callback );
	void	set_auth_callback ( std::function <int ( cli&, const std::string&, const std::string& )> callback );
	void	set_enable_callback ( std::function <int ( const std::string& )> callback );
	void	set_enable_callback ( std::function <int ( cli&, const std::string& )> callback );
	void	set_regular_callback ( std::function <int ()> callback );
	void	set_regular_callback ( std::function <int ( cli& )> callback );
	void	allow_user ( const std::string& username, const std::string& password );
	void	allow_enable ( const std::string& password );
	void	deny_user ( const std::string& username );
	void	set_banner ( const std::string& banner );
	void	set_hostname ( const std::string& hostname );
	void	set_modestring ( const std::string& modestring );
	int		set_privilege ( int privilege );
	int		set_mode ( int mode, const std::string& desc );
	bool	arg_wants_help ( const std::string& arg );
	bool	wants_help_last_arg ( const std::string& arg );
	RESULT	have_unwanted_args ( const tokens& args );
	void	set_history_size ( size_t size );

protected:
	void		add_history ( const std::string& line );
	std::string	do_history ( const unsigned char& c );
	void		build_shortest ( cmd_list& commands );
	RESULT		split_line ( const std::string& s, tokens& token );
	RESULT		split_filter ( const std::string& s );
	void		result_msg ( RESULT result );
	RESULT		find_filter ( const cmd_list& filters, tokens& words );
	void		clear_active_filters ();
	RESULT		find_command ( const cmd_list& commands, tokens& words );
	std::string	get_completions ( const cmd_list& commands, tokens& words );
	bool		pass_matches ( const std::string& pass, const std::string& tried_pass );
	std::string	make_prompt ();
	int			_print ( int print_mode, const char* format, va_list ap );
	RESULT		internal_enable ( const std::string& name, const tokens& args );
	RESULT		internal_disable ( const std::string& name, const tokens& args );
	RESULT		internal_configure ( const std::string& name, const tokens& args );
	RESULT		internal_help ( const std::string& name, const tokens& args );
	RESULT		internal_whoami ( const std::string& name, const tokens& args );
	RESULT		internal_history ( const std::string& name, const tokens& args );
	RESULT		internal_set_history ( const std::string& name, const tokens& args );
	RESULT		internal_invoke_history ( const std::string& name, const tokens& args );
	RESULT		internal_quit ( const std::string& name, const tokens& args );
	RESULT		internal_exit ( const std::string& name, const tokens& args );
	RESULT 		internal_pager ( const std::string& command, const libcli::tokens& args );
	RESULT 		internal_hostname ( const std::string& command, const libcli::tokens& args );
	RESULT		filter_grab ( const::std::string& name, const tokens& args );
	RESULT		filter_exclude ( const::std::string& name, const tokens& args );
	RESULT		filter_begin ( const::std::string& name, const tokens& args );
	RESULT		filter_between ( const::std::string& name, const tokens& args );
	RESULT		filter_limit ( const::std::string& name, const tokens& args );
	RESULT		filter_last ( const::std::string& name, const tokens& args );
	RESULT		filter_count ( const::std::string& name, const tokens& args );
	RESULT		filter_nomore ( const::std::string& name, const tokens& args );
	bool		pager ();
	void		check_enable ( const std::string& pass );
	void		check_user_auth ( const std::string& username, const std::string& password );
	void		leave_mode ();
	bool		check_pager ();
	std::string		m_banner;
	std::string		m_enable_password;
	std::string		m_hostname;
	std::string		m_modestring;
	int				m_privilege;
	int				m_mode;
	STATE			m_state;
	client			m_client;
	user_map		m_users;
	cmd_list		m_commands;			///< list of commands
	cmd_list		m_filters;			///< list of available filters
	editor			m_edit;
	std::string		m_username;			///< login name of user
	size_t			m_max_history;		///< maximmum size of the history
	size_t			m_in_history;		///< 
	hist_list		m_history;			///< the history
	/**
	 * @brief a callback function taking no arguments and returning an int
	 *
	 * C++ is a pile of bullshit regarding callback functions. I want to store
	 * a pointer to a callback function and I do not care if it is a member of
	 * some class or a static c function.
	 * You can try to find a solution for this. Let me know if you have a
	 * better approach
	 */
	struct auth_func_t
	{
		union
		{
			std::function <int ( cli&, const std::string&, const std::string& )>	member;	// pointer to a class member
			std::function <int ( const std::string&, const std::string& )>			c_func;	// pointer to c- or staic function
		};
		auth_func_t () : member { nullptr } {};
		~auth_func_t () {};
	} m_auth_callback;
	struct enable_func_t
	{
		union
		{
			std::function <int ( cli&, const std::string& )>	member;	// pointer to a class member
			std::function <int ( const std::string& )>			c_func;	// pointer to c- or staic function
		};
		enable_func_t () : member { nullptr } {};
		~enable_func_t () {};
	} m_enable_callback;
#ifndef _MSC_VER
	struct termios  OldModes;
#endif
};

}; // namespace libcli

#endif
