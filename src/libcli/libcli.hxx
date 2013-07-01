#ifndef __LIBCLI_H__
#define __LIBCLI_H__

#include <stdio.h>
#include <stdarg.h>
#ifndef _MSC_VER
	#include <termios.h>
#endif
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
	void    clear_line ( int sockfd, char* cmd, int l, int cursor );
	int     pass_matches ( char* pass, char* tried_pass );
	int     show_prompt ( int sockfd );
	void    setup_terminal ();
	void    _print ( int print_mode, const char* format, va_list ap );
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
	FILE*   client;
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
	void	destroy ();
	int     done();
	void    register_command ( Command<CLI>* command, Command<CLI>* parent = 0 );
	int     unregister_command ( char* command );
	int     run_command ( char* command );
	int     loop ( int sockfd );
	int     file ( FILE* fh, int privilege, int mode );
	void    set_auth_callback ( c_auth_func callback );
	void    set_auth_callback ( cpp_auth_func callback );
	void    set_enable_callback ( c_enable_func callback );
	void    set_enable_callback ( cpp_enable_func callback );
	void    allow_user ( char* username, char* password );
	void    allow_enable ( char* password );
	void    deny_user ( char* username );
	void    set_banner ( const char* banner );
	void    set_hostname ( const char* hostname );
	void    set_promptchar ( const char* prompt );
	void    set_modestring ( const char* modestring );
	int     set_privilege ( int privilege );
	int     set_configmode ( int mode, const char* config_desc );
	void    reprompt();
	void    regular ( int ( *callback ) () );
	int     print ( const char* format, ... ) __attribute__ ( ( format ( printf, 2, 3 ) ) );
	void    bufprint ( const char* format, ... ) __attribute__ ( ( format ( printf, 2, 3 ) ) );
	void    vabufprint ( const char* format, va_list ap );
	void    error ( const char* format, ... ) __attribute__ ( ( format ( printf, 2, 3 ) ) );
	void    free_history();
};

}; // namespace LIBCLI

#endif
