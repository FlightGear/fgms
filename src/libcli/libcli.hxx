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

#include <stdio.h>
#include <stdarg.h>
#ifndef _MSC_VER
	#include <termios.h>
#endif
#include <plib/netSocket.h>
#include "common.hxx"
#include "command.hxx"
#include "filter.hxx"

namespace LIBCLI
{

class CLI
{
public:

	class unp
	{
	public:
		char* username;
		char* password;
		unp* next;
	};

protected:
	int     match_filter_init ( int argc, char** argv, filter_t* filt );
	int     range_filter_init ( int argc, char** argv, filter_t* filt );
	int     count_filter_init ( int argc, char** argv, filter_t* filt );
	int     match_filter ( char* cmd, void* data );
	int     range_filter ( char* cmd, void* data );
	int     count_filter ( char* cmd, void* data );
	void    free_command ( Command<CLI> *cmd );
	int     build_shortest ( Command<CLI>* commands );
	int     add_history ( char* cmd );
	int     parse_line ( char* line, char* words[], int max_words );
	char*   join_words ( int argc, char** argv );
	int     find_command ( Command<CLI> *commands, int num_words, char* words[], int start_word, int filters[] );
	int     get_completions ( char* command, char** completions, int max_completions );
	void    clear_line ( char* cmd, int l, int cursor );
	int     pass_matches ( char* pass, char* tried_pass );
	int     show_prompt ();
	void    setup_terminal ();
	int     _print ( int print_mode, const char* format, va_list ap );
	int     show_help ( Command<CLI> *c );
	int     internal_enable ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_disable ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_help ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_history ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_quit ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	int     internal_exit ( char* command, char* argv[], int argc );
	int     internal_dump ( char* command, char* argv[], int argc );
	int     int_configure_terminal ( UNUSED ( char* command ), UNUSED ( char* argv[] ), UNUSED ( int argc ) );
	void    unregister_all ( Command<CLI> *command );
	int	pager ();
	bool	from_socket;
	int	my_sock;
	size_t	lines_out;
	size_t	max_screen_lines;
#ifndef _MSC_VER
	struct termios  OldModes;                                                                                            
#endif

public:

	typedef int (*c_auth_func) ( const string& , const string& );
	typedef int (CLI::*cpp_auth_func) ( const string&, const string& );
	typedef int (*c_enable_func) ( const string& );
	typedef int (CLI::*cpp_enable_func) ( const string& );

	int     completion_callback;
	char*   banner;
	char*   enable_password;
	char*   history[MAX_HISTORY];
	char    showprompt;
	char*   promptchar;
	char*   hostname;
	char*   modestring;
	int     privilege;
	int     mode;
	int     state;
	netSocket*   client;
	/* internal buffers */
	void*   conn;
	void*   service;
	char*   buffer;
	unsigned buf_size;
	unp*    users;
	Command<CLI>*   commands;
	filter_t*   filters;

	int     (*regular_callback)();
	c_auth_func   auth_callback;
	cpp_auth_func cpp_auth_callback;
	c_enable_func enable_callback;
	cpp_enable_func cpp_enable_callback;
	
	void    ( *print_callback ) ( char* cmd );
	void    set_print_callback ( void ( *callback ) ( char* ) );

	CLI ();
	~CLI ();
	void	destroy ();
	void    register_command ( Command<CLI>* command, Command<CLI>* parent = 0 );
	int     unregister_command ( char* command );
	int     run_command ( char* command );
	int     loop ( int sockfd );
	int     file ( FILE* fh, int privilege, int mode );
	void    set_auth_callback ( c_auth_func callback );
	void    set_auth_callback ( cpp_auth_func callback );
	void    set_enable_callback ( c_enable_func callback );
	void    set_enable_callback ( cpp_enable_func callback );
	void    allow_user ( const char* username, const char* password );
	void    allow_enable ( const char* password );
	void    deny_user ( char* username );
	void    set_banner ( const char* banner );
	void    set_hostname ( const char* hostname );
	void    set_promptchar ( const char* prompt );
	void    set_modestring ( const char* modestring );
	int     set_privilege ( int privilege );
	int     set_configmode ( int mode, const char* config_desc );
	void    reprompt();
	void    regular ( int ( *callback ) () );
// #ifdef _MSC_VER
	int     print ( const char* format, ... );
	void    bufprint ( const char* format, ... );
	void    error ( const char* format, ... );
#if 0
// #else // !_MSC_VER
	int     print ( const char* format, ... ) __attribute__ ( ( format ( printf, 2, 3 ) ) );
	void    bufprint ( const char* format, ... ) __attribute__ ( ( format ( printf, 2, 3 ) ) );
	void    error ( const char* format, ... ) __attribute__ ( ( format ( printf, 2, 3 ) ) );
#endif // _MSC_VER y/n
	void    vabufprint ( const char* format, va_list ap );
	void    free_history();
};

}; // namespace LIBCLI

#endif
