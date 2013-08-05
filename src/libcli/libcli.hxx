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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, U$
//
// derived from libcli by David Parrish (david@dparrish.com)
// Copyright (C) 2011  Oliver Schroeder
//

#ifndef __LIBCLI_H__
#define __LIBCLI_H__

#include <string>
#include <map>
#include <stdarg.h>
#ifndef _MSC_VER
	#include <termios.h>
#endif
#include "cli_client.hxx"
#include "common.hxx"
#include "command.hxx"

namespace LIBCLI
{

class CLI
{
public:
	typedef std::map<string,string> unp;
	typedef std::map<string,string>::iterator unp_it;
	typedef int (*c_auth_func) ( const string& , const string& );
	typedef int (CLI::*cpp_auth_func) ( const string&, const string& );
	typedef int (*c_enable_func) ( const string& );
	typedef int (CLI::*cpp_enable_func) ( const string& );

	int	completion_callback;
	string	banner;
	string	enable_password;
	char*   history[MAX_HISTORY];
	bool	showprompt;
	string	prompt;
	string	hostname;
	string	modestring;
	int     privilege;
	int     mode;
	int     state;
	Client  client;
	unp	users;
	Command<CLI>*   commands;
	// filter_t*   filters;

	int		(*regular_callback)();
	c_auth_func	auth_callback;
	cpp_auth_func	cpp_auth_callback;
	c_enable_func	enable_callback;
	cpp_enable_func	cpp_enable_callback;
	
	CLI ( int fd );
	~CLI ();
	void	destroy ();
	void    register_command ( Command<CLI>* command, Command<CLI>* parent = 0 );
	int     unregister_command ( char* command );
	int     run_command ( char* command );
	int     loop ();
	int     file ( FILE* fh, int privilege, int mode );
	void    set_auth_callback ( c_auth_func callback );
	void    set_auth_callback ( cpp_auth_func callback );
	void    set_enable_callback ( c_enable_func callback );
	void    set_enable_callback ( cpp_enable_func callback );
	void    allow_user ( const string& username, const string& password );
	void    allow_enable ( const string& password );
	void    deny_user ( const string& username );
	void    set_banner ( const string& banner );
	void    set_hostname ( const string& hostname );
	void    set_prompt ( const string& prompt );
	void    set_modestring ( const string& modestring );
	int     set_privilege ( int privilege );
	int     set_configmode ( int mode, const string& config_desc );
	void    regular ( int ( *callback ) () );
	int     print ( const char* format, ... );
	void    free_history();

protected:
	void    free_command ( Command<CLI> *cmd );
	int     build_shortest ( Command<CLI>* commands );
	int     add_history ( char* cmd );
	int     parse_line ( char* line, char* words[], int max_words );
	int     find_command ( Command<CLI> *commands, int num_words, char* words[], int start_word, int filters[] );
	int     get_completions ( char* command, char** completions, int max_completions );
	void    clear_line ( char* cmd, int l, int cursor );
	int     pass_matches ( string pass, string tried_pass );
	void	show_prompt ();
	int     _print ( int print_mode, const char* format, va_list ap );
	int     internal_enable ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_disable ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_help ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_whoami ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_history ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_quit ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_exit ( char* command, char* argv[], int argc );
	int     internal_dump ( char* command, char* argv[], int argc );
	int     int_configure_terminal ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	void    unregister_all ( Command<CLI> *command );
	int	pager ();
	// line editing and handling of special chars
	void	handle_telnet_option ();
	int	get_input ( unsigned char& c );
	void	check_enable ( const char* pass );
	void	check_user_auth ( const string& username, const string& password );
	void	delete_backwards ( const unsigned char c );
	void	prompt_user ();
	void	redraw_line ();
	void	clear_line ();
	void	clear_to_eol ();
	bool	try_logout ();
	void	leave_config_mode ();
	void	list_completions ();
	void	do_history ( const unsigned char& c );
	void	cursor_left ();
	void	cursor_right ();
	void	jump_start_of_line ();
	void	jump_end_of_line ();
	bool	append ( const unsigned char& c );
	void	insert ( const unsigned char& c );
	bool	check_pager ();
	unsigned char	map_esc ();
	// Variables
	bool	from_socket;
	int	my_sock;
	int	length;		// length of current input line
	int	cursor;		// cursor position within input line
	char*	cmd;		// content of current input line
	string	username;	// login name of user
	int	in_history;
#ifndef _MSC_VER
	struct termios  OldModes;
#endif
};

}; // namespace LIBCLI

#endif
