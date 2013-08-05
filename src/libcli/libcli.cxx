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
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

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

#ifdef _MSC_VER
	// some windows quick fixes
	#define CTRL(a)  ( a & 037 )
	#ifdef __cplusplus
		extern "C" {
	#endif
		extern char *crypt(const char *key, const char *salt);
	#ifdef __cplusplus
		}
	#endif
#endif

namespace LIBCLI
{

using namespace std;

#ifndef CALL_MEMBER_FN
	CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))
#endif

filter_cmds_t filter_cmds[] =
{
	{ "begin",   "Begin with lines that match" },
	{ "between", "Between lines that match" },
	{ "count",   "Count of lines"   },
	{ "exclude", "Exclude lines that match" },
	{ "include", "Include lines that match" },
	{ NULL, NULL}
};

void
CLI::set_auth_callback
(
        c_auth_func callback
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->auth_callback = callback;
	this->cpp_auth_callback = 0;
}

void
CLI::set_auth_callback
(
        cpp_auth_func callback
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->auth_callback = 0;
	this->cpp_auth_callback = callback;
}

void
CLI::set_enable_callback
(
        c_enable_func callback
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->enable_callback = callback;
	this->cpp_enable_callback = 0;
}

void
CLI::set_enable_callback
(
        cpp_enable_func callback
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->enable_callback = 0;
	this->cpp_enable_callback = callback;
}

void
CLI::allow_user
(
        const string& username,
        const string& password
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	unp_it u = users.find (username);
	if (u != users.end())
	{
		cerr << "user '" << username << "' already exists!" << endl;
		return;
	}
	users[username] = password;
}

void
CLI::allow_enable
(
	const string& password
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	enable_password = password;
}

void
CLI::deny_user
(
        const string& username
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	if (users.empty())
		return;
	unp_it u = users.find (username);
	if (u == users.end())
		return;
	users.erase (u);
}

void
CLI::set_banner
(
        const string& banner
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->banner = banner;
}

void
CLI::set_hostname
(
        const string& hostname
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->hostname = hostname;
}

void
CLI::set_prompt
(
        const string& prompt
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->prompt = prompt;
}

int
CLI::build_shortest
(
        Command<CLI> *commands
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c, *p;
	char* cp, *pp;
	unsigned int len;
	for ( c = commands; c; c = c->next )
	{
		c->unique_len = strlen ( c->command );
		if ( ( c->mode != MODE_ANY && c->mode != this->mode )
		                ||  ( c->privilege > this->privilege ) )
		{
			continue;
		}
		c->unique_len = 1;
		for ( p = commands; p; p = p->next )
		{
			if ( c == p )
			{
				continue;
			}
			if ( ( p->mode != MODE_ANY && p->mode != this->mode )
			                ||  ( p->privilege > this->privilege ) )
			{
				continue;
			}
			cp = c->command;
			pp = p->command;
			len = 0;
			while ( *cp && *pp && *cp++ == *pp++ )
			{
				len++;
			}
			if ( len > c->unique_len )
			{
				c->unique_len = len;
			}
		}
		if ( c->children )
		{
			build_shortest ( c->children );
		}
	}
	return LIBCLI::OK;
}

int
CLI::set_privilege
(
        int priv
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int old = this->privilege;
	this->privilege = priv;
	if ( priv != old )
	{
		set_prompt ( priv == LIBCLI::PRIVILEGED ? "# " : "> " );
		build_shortest ( this->commands );
	}
	return old;
}

void
CLI::set_modestring
(
        const string& modestring
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->modestring = modestring;
}

int
CLI::set_configmode
(
        int mode,
        const string& config_desc
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int old = this->mode;
	this->mode = mode;
	if ( mode != old )
	{
		if ( !this->mode )
		{
			// Not config mode
			set_modestring ("");
		}
		else if ( config_desc != "" )
		{
			string s = "(config-" + config_desc + ")";
			set_modestring (s);
		}
		else
		{
			set_modestring ( "(config)" );
		}
		build_shortest ( this->commands );
	}
	return old;
}

void
CLI::register_command
(
        Command<CLI>*   command,
        Command<CLI>*   parent
)
{
	DEBUG d ( CLI_TRACE );
	if ( ! command )
	{
		return;
	}
	if ( parent )
	{
		command->parent = parent;
		if ( ! parent->children )
		{
			parent->children = command;
			build_shortest ( command->parent );
			return;
		}
		Command<CLI>* c;
		for ( c = parent->children; c && c->next; c = c->next )
			/* intentionally empty */
		{
			;
		}
		if ( c )
		{
			c->next = command;
			build_shortest ( parent );
			return;
		}
		cout << "bummer!" << std::endl;
	}
	if ( ! this->commands )
	{
		this->commands = command;
	}
	else
	{
		Command<CLI>* c;
		for ( c = this->commands; c && c->next; c = c->next )
			/* intentionally empty */
		{
			;
		}
		if ( c )
		{
			c->next = command;
		}
		else
		{
			cout << "bummer 2" << std::endl;
		}
	}
	build_shortest ( ( command->parent ) ? command->parent : this->commands );
}

void
CLI::free_command
(
        Command<CLI> *cmd
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c,*p;
	for ( c = cmd->children; c; )
	{
		p = c->next;
		free_command ( c );
		c = p;
	}
	free_z ( cmd->command );
	if ( cmd->help )
	{
		free_z ( cmd->help );
	}
	free_z ( cmd );
}

int
CLI::unregister_command
(
        char* command
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c, *p = NULL;
	if ( ! command )
	{
		return -1;
	}
	if ( !this->commands )
	{
		return LIBCLI::OK;
	}
	for ( c = this->commands; c; c = c->next )
	{
		if ( strcmp ( c->command, command ) == 0 )
		{
			if ( p )
			{
				p->next = c->next;
			}
			else
			{
				this->commands = c->next;
			}
			free_command ( c );
			return LIBCLI::OK;
		}
		p = c;
	}
	return LIBCLI::OK;
}

int
CLI::internal_enable
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	if ( this->privilege == LIBCLI::PRIVILEGED )
	{
		return LIBCLI::OK;
	}
	if ( (enable_password == "") && !this->enable_callback && !this->cpp_enable_callback)
	{
		/* no password required, set privilege immediately */
		set_privilege ( LIBCLI::PRIVILEGED );
		set_configmode ( LIBCLI::MODE_EXEC, "" );
	}
	else
	{
		/* require password entry */
		this->state = LIBCLI::STATE_ENABLE_PASSWORD;
	}
	return LIBCLI::OK;
}

int
CLI::internal_disable
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	set_privilege ( LIBCLI::UNPRIVILEGED );
	set_configmode ( LIBCLI::MODE_EXEC, "" );
	return LIBCLI::OK;
}

int
CLI::internal_help
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	client << CRLF;
	client <<
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
		<< CRLF;
	return LIBCLI::OK;
}

int
CLI::internal_whoami
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	client << "You are '" << username << "'" << CRLF;
	return LIBCLI::OK;
}

int
CLI::internal_history
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int i;
	client << CRLF << "Command history:" << CRLF;
	for ( i = 0; i < LIBCLI::MAX_HISTORY; i++ )
	{
		if ( this->history[i] )
		{
			client << setw(3) << i << " " << this->history[i] << CRLF;
		}
	}
	return LIBCLI::OK;
}

int
CLI::internal_quit
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	set_privilege ( LIBCLI::UNPRIVILEGED );
	set_configmode ( LIBCLI::MODE_EXEC, "" );
	return LIBCLI::QUIT;
}

int
CLI::internal_exit
(
        char* command,
        char* argv[],
        int argc
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	if ( this->mode == MODE_EXEC )
	{
		return internal_quit ( command, argv, argc );
	}
	if ( this->mode > MODE_CONFIG )
	{
		set_configmode ( MODE_CONFIG, "" );
	}
	else
	{
		set_configmode ( MODE_EXEC, "" );
	}
	return LIBCLI::OK;
}

int
CLI::int_configure_terminal
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	set_configmode ( MODE_CONFIG, "" );
	return LIBCLI::OK;
}

CLI::CLI
(
	int fd
): client (fd)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c;
	length	= 0;
	cursor	= 0;
	cmd	= 0;
	in_history = 0;
	this->commands		= 0;
	this->auth_callback	= 0;
	this->cpp_auth_callback	= 0;
	this->regular_callback	= 0;
	this->enable_callback	= 0;
	this->cpp_enable_callback	= 0;
	this->from_socket	= false;
	int i;
	for ( i = 0; i < MAX_HISTORY; i++ )
	{
		this->history[i] = 0;
	}
	register_command ( new Command<CLI> (
	                           this,
	                           "help",
	                           & CLI::internal_help,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_ANY,
	                           "Description of the interactive help system"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "whoami",
	                           & CLI::internal_whoami,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_EXEC,
	                           "Show who you are"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "quit",
	                           & CLI::internal_quit,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_EXEC,
	                           "Disconnect"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "exit",
	                           & CLI::internal_exit,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_ANY,
	                           "Exit from current mode"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "history",
	                           & CLI::internal_history,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_EXEC,
	                           "Show a list of previously run commands"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "enable",
	                           & CLI::internal_enable,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_EXEC,
	                           "Turn on privileged commands"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "disable",
	                           & CLI::internal_disable,
	                           LIBCLI::PRIVILEGED,
	                           LIBCLI::MODE_EXEC,
	                           "Turn off privileged commands"
	                   ) );
	c = new Command<CLI> (
	        this,
	        "configure",
	        LIBCLI::PRIVILEGED,
	        LIBCLI::MODE_EXEC,
	        "Enter configuration mode"
	);
	register_command ( c );
	register_command ( new Command<CLI> (
	                           this,
	                           "terminal",
	                           & CLI::int_configure_terminal,
	                           LIBCLI::PRIVILEGED,
	                           LIBCLI::MODE_EXEC,
	                           "Configure from the terminal"
	                   ), c );
	this->privilege = this->mode = -1;
	set_privilege ( LIBCLI::UNPRIVILEGED );
	set_configmode ( LIBCLI::MODE_EXEC, "" );
}

CLI::~CLI
()
{
	users.clear();
	free_history();
	unregister_all ( 0 );
}

void
CLI::destroy
()
{
}

void
CLI::unregister_all
(
        Command<CLI> *command
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c, *p = NULL;
	if ( ! command )
	{
		command = this->commands;
	}
	if ( ! command )
	{
		return;
	}
	for ( c = command; c; )
	{
		p = c->next;
		// Unregister all child commands
		if ( c->children )
		{
			unregister_all ( c->children );
		}
		if ( c->command )
		{
			free_z ( c->command );
		}
		if ( c->help )
		{
			free_z ( c->help );
		}
		free_z ( c );
		c = p;
	}
}

int
CLI::add_history
(
        char* cmd
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int i;
	for ( i = 0; i < MAX_HISTORY; i++ )
	{
		if ( !this->history[i] )
		{
			if ( i == 0 || strcasecmp ( this->history[i-1], cmd ) )
			{
				if ( ! ( this->history[i] = strdup ( cmd ) ) )
				{
					in_history = i + 1;
					return LIBCLI::ERROR_ANY;
				}
			}
			in_history = i + 1;
			return LIBCLI::OK;
		}
	}
	// No space found, drop one off the beginning of the list
	free_z ( this->history[0] );
	for ( i = 0; i < MAX_HISTORY-1; i++ )
	{
		this->history[i] = this->history[i+1];
	}
	if ( ! ( this->history[MAX_HISTORY - 1] = strdup ( cmd ) ) )
	{
		in_history = MAX_HISTORY;
		return LIBCLI::ERROR_ANY;
	}
	return LIBCLI::OK;
}

void
CLI::free_history
()
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int i;
	for ( i = 0; i < MAX_HISTORY; i++ )
	{
		if ( this->history[i] )
		{
			free_z ( this->history[i] );
		}
	}
}

int
CLI::parse_line
(
        char* line,
        char* words[],
        int max_words
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int nwords = 0;
	char* p = line;
	char* word_start = 0;
	int inquote = 0;
	while ( *p )
	{
		if ( ! isspace ( *p ) )
		{
			word_start = p;
			break;
		}
		p++;
	}
	while ( nwords < max_words - 1 )
	{
		if ( !*p || *p == inquote
		                || ( word_start && !inquote && ( isspace ( *p ) || *p == '|' ) ) )
		{
			if ( word_start )
			{
				int len = p - word_start;
				memcpy ( words[nwords] = ( char* ) malloc ( len + 1 ), word_start, len );
				words[nwords++][len] = 0;
			}
			if ( !*p )
			{
				break;
			}
			if ( inquote )
			{
				p++;        /* skip over trailing quote */
			}
			inquote = 0;
			word_start = 0;
		}
		else if ( *p == '"' || *p == '\'' )
		{
			inquote = *p++;
			word_start = p;
		}
		else
		{
			if ( !word_start )
			{
				if ( *p == '|' )
				{
					if ( ! ( words[nwords++] = strdup ( "|" ) ) )
					{
						return 0;
					}
				}
				else if ( !isspace ( *p ) )
				{
					word_start = p;
				}
			}
			p++;
		}
	}
	words[nwords] = 0;
	return nwords;
}

int
CLI::find_command
(
        Command<CLI> *commands,
        int num_words,
        char* words[],
        int start_word,
        int filters[]
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c, *again = NULL;
	int c_words = num_words;
	if ( filters[0] )
	{
		c_words = filters[0];
	}
	// Deal with ? for help
	if ( ! words[start_word] )
	{
		return LIBCLI::ERROR_ANY;
	}
	if ( words[start_word][strlen ( words[start_word] ) - 1] == '?' )
	{
		int l = strlen ( words[start_word] )-1;
		for ( c = commands; c; c = c->next )
		{
			if ( strncasecmp ( c->command, words[start_word], l ) == 0
			&& ( c->have_callback || c->children )
			&& this->privilege >= c->privilege
			&& ( c->mode == this->mode || c->mode == MODE_ANY ) )
			{
				
				client << "  "
				  << left << setfill(' ') << setw(20)
				  << c->command
				  << (c->help ? c->help : "") << CRLF;
			}
		}
		if ( commands->parent && commands->parent->have_callback )
		{
			client << "  "
			  << left << setfill(' ') << setw(20) << "<br>"
			  << (commands->parent->help ? commands->parent->help : "")
			  << CRLF;
		}
		return LIBCLI::OK;
	}
	for ( c = commands; c; c = c->next )
	{
		if ( this->privilege < c->privilege )
		{
			continue;
		}
		if ( strncasecmp ( c->command, words[start_word], c->unique_len ) )
		{
			continue;
		}
		if ( strncasecmp ( c->command, words[start_word], strlen ( words[start_word] ) ) )
		{
			continue;
		}
AGAIN:
		if ( c->mode == this->mode || c->mode == MODE_ANY )
		{
			int rc = LIBCLI::OK;
			int f;
			filter_t** filt = &client.filters;
			// Found a word!
			if ( ! c->children )
			{
				// Last word
				if ( ! c->have_callback )
				{
					client << UNFILTERED << "No callback for '" << c->command << "'" << CRLF;
					return LIBCLI::ERROR_ANY;
				}
			}
			else
			{
				if ( start_word == c_words - 1 )
				{
					if ( c->have_callback )
					{
						goto CORRECT_CHECKS;
					}
					client << UNFILTERED << "Incomplete command" << CRLF;
					return LIBCLI::ERROR_ANY;
				}
				rc = find_command ( c->children, num_words, words, start_word + 1, filters );
				if ( rc == LIBCLI::ERROR_ARG )
				{
					if ( c->have_callback )
					{
						rc = LIBCLI::OK;
						goto CORRECT_CHECKS;
					}
					else
					{
						client << UNFILTERED << "Invalid argument '" << words[start_word + 1] << "'"
						  << CRLF;
					}
				}
				return rc;
			}
			if ( ! c->have_callback )
			{
				client << UNFILTERED << "Internal server error processing '" << c->command << "'"
				  << CRLF;
				return LIBCLI::ERROR_ANY;
			}
CORRECT_CHECKS:
			for ( f = 0; rc == LIBCLI::OK && filters[f]; f++ )
			{
				int n = num_words;
				char** argv;
				int argc;
				int len;
				if ( filters[f+1] )
				{
					n = filters[f+1];
				}
				if ( filters[f] == n - 1 )
				{
					client << UNFILTERED << "Missing filter" << CRLF;
					return LIBCLI::ERROR_ANY;
				}
				argv = words + filters[f] + 1;
				argc = n - ( filters[f] + 1 );
				len = strlen ( argv[0] );
				if ( argv[argc - 1][strlen ( argv[argc - 1] ) - 1] == '?' )
				{
					if ( argc == 1 )
					{
						int i;
						for ( i = 0; filter_cmds[i].cmd; i++ )
						{
							client << "  "
							  << left << setfill(' ') << setw(20)
							  << filter_cmds[i].cmd
							  << filter_cmds[i].help << CRLF;
						}
					}
					else
					{
						if ( argv[0][0] != 'c' ) // count
						{
							client << "  WORD" << CRLF;
						}
						if ( argc > 2 || argv[0][0] == 'c' ) // count
						{
							client << "  <cr>" << CRLF;
						}
					}
					return LIBCLI::OK;
				}
				if ( argv[0][0] == 'b' && len < 3 ) // [beg]in, [bet]ween
				{
					client << UNFILTERED << "Ambiguous filter '" << argv[0] << "' (begin, between)" << CRLF;
					return LIBCLI::ERROR_ANY;
				}
				*filt = ( filter_t* ) calloc ( sizeof ( filter_t ), 1 );
				if ( !strncmp ( "include", argv[0], len )
				||  !strncmp ( "exclude", argv[0], len ) )
				{
					rc = client.match_filter_init ( argc, argv, *filt );
				}
				else if ( !strncmp ( "begin", argv[0], len )
				|| !strncmp ( "between", argv[0], len ) )
				{
					rc = client.range_filter_init ( argc, argv, *filt );
				}
				else if ( !strncmp ( "count", argv[0], len ) )
				{
					rc = client.count_filter_init ( argc, argv, *filt );
				}
				else
				{
					client << UNFILTERED << "Invalid filter '" << argv[0] << "'" << CRLF;
					rc = LIBCLI::ERROR_ANY;
				}
				if ( rc == LIBCLI::OK )
				{
					filt = & ( *filt )->next;
				}
				else
				{
					free_z ( *filt );
					*filt = 0;
				}
			}
			if ( rc == LIBCLI::OK )
			{
				rc = c->exec ( c->command, words + start_word + 1, c_words - start_word - 1 );
			}
			while ( client.filters )
			{
				filter_t* filt = client.filters;
				// call one last time to clean up
				filt->exec ( client, NULL );
				client.filters = filt->next;
				free_z ( filt );
			}
			return rc;
		}
		else if ( this->mode > MODE_CONFIG && c->mode == MODE_CONFIG )
		{
			// command matched but from another mode,
			// remember it if we fail to find correct command
			again = c;
		}
	}
	// drop out of config submode if we have matched command on MODE_CONFIG
	if ( again )
	{
		c = again;
		set_configmode ( MODE_CONFIG, "" );
		goto AGAIN;
	}
	if ( start_word == 0 )
	{
		client << UNFILTERED << "Invalid " << (commands->parent ? "argument" : "command")
		  << " '" << words[start_word] << "'" << CRLF;
	}
	return LIBCLI::ERROR_ARG;
}

int
CLI::run_command
(
        char* command
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int r;
	unsigned int num_words, i, f;
	char* words[128] = {0};
	int filters[128] = {0};
	if ( ! command )
	{
		return LIBCLI::ERROR_ANY;
	}
	while ( isspace ( *command ) )
	{
		command++;
	}
	if ( ! *command )
	{
		return LIBCLI::OK;
	}
	num_words = parse_line ( command, words, sizeof ( words ) / sizeof ( words[0] ) );
	for ( i = f = 0; i < num_words && f < sizeof ( filters ) / sizeof ( filters[0] ) - 1; i++ )
	{
		if ( words[i][0] == '|' )
		{
			filters[f++] = i;
		}
	}
	filters[f] = 0;
	if ( num_words )
	{
		r = find_command ( this->commands, num_words, words, 0, filters );
	}
	else
	{
		r = LIBCLI::ERROR_ANY;
	}
	for ( i = 0; i < num_words; i++ )
	{
		free ( words[i] );
	}
	if ( r == LIBCLI::QUIT )
	{
		return r;
	}
	return LIBCLI::OK;
}

int
CLI::get_completions
(
        char* command,
        char** completions,
        int max_completions
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c;
	Command<CLI> *n;
	Command<CLI> *p = 0;
	int num_words, i, k=0, j;
	int num_words_line;
	char* words[128] = {0};
	int filter = 0;
	if ( ! command )
	{
		return 0;
	}
	while ( isspace ( *command ) )
	{
		command++;
	}
	num_words_line = parse_line ( command, words, sizeof ( words ) /sizeof ( words[0] ) );
	num_words = num_words_line;
	if ( !command[0] || command[strlen ( command )-1] == ' ' )
	{
		num_words++;
	}
	if ( ! num_words )
	{
		return 0;
	}
	for ( i = 0; i < num_words; i++ )
	{
		if ( words[i] && words[i][0] == '|' )
		{
			filter = i;
		}
	}
	if ( filter ) // complete filters
	{
		unsigned len = 0;
		if ( filter < num_words - 1 ) // filter already completed
		{
			return 0;
		}
		if ( filter == num_words - 1 )
		{
			len = strlen ( words[num_words-1] );
		}
		for ( i = 0; filter_cmds[i].cmd && k < max_completions; i++ )
		{
			if ( !len || ( len < strlen ( filter_cmds[i].cmd )
			                && !strncmp ( filter_cmds[i].cmd, words[num_words - 1], len ) ) )
			{
				completions[k++] = ( char* ) filter_cmds[i].cmd;
			}
		}
		completions[k] = NULL;
		return k;
	}
	j = 0;
	for ( c = this->commands, i = 0; c && i < num_words && k < max_completions; c = n )
	{
		n = c->next;
		if ( this->privilege < c->privilege )
		{
			continue;
		}
		if ( c->mode != this->mode && c->mode != MODE_ANY )
		{
			continue;
		}
		if ( words[i] && strncasecmp ( c->command, words[i], strlen ( words[i] ) ) )
		{
			continue;
		}
		if ( i < num_words - 1 )
		{
			if ( strlen ( words[i] ) < c->unique_len )
			{
				continue;
			}
			j = 0;
			p = c;
			n = c->children;
			i++;
			continue;
		}
		if ( ( (j > 0)  || c->next ) || ( ( p != 0 ) && ( p->have_callback ) ) )
		{	// more than one completion
			if ( j == 0 )
			{
				client << CRLF;
				j++;
			}
			client << "  " << left << setfill(' ') << setw(20)
			  << c->command
			  << (c->help ? c->help : "") << CRLF;
		}
		if (strncmp (command, c->command, strlen (c->command)) != 0)
			completions[k++] = c->command;
	}
	if (completions[k-1])
	if ( p != 0 )
	if ( k != 0 )
	if (strncmp (words[num_words_line - 1], completions[k-1], strlen (words[num_words_line - 1])) != 0)
	{
		if ( p->have_callback )
		{
			if ( j == 0 )
			{
				client << CRLF;
			}
			client << "  "
			  << left << setfill(' ') << setw(20)
			  << "<br>"
			  << (p->help ? p->help : "") << CRLF;
			k++;
		}
	}
	return k;
}

void
CLI::clear_line
(
        char* cmd,
        int l,
        int cursor
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int i;
	if ( cursor < l )
	{
		for ( i = 0; i < ( l - cursor ); i++ )
			client.put_char (' ');
	}
	for ( i = 0; i < l; i++ )
	{
		cmd[i] = '\b';
	}
	for ( ; i < l * 2; i++ )
	{
		cmd[i] = ' ';
	}
	for ( ; i < l * 3; i++ )
	{
		cmd[i] = '\b';
	}
	client << cmd << commit;
	memset ( cmd, 0, i );
	l = cursor = 0;
}

void
CLI::regular
(
        int ( *callback ) ()
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->regular_callback = callback;
}

const string DES_PREFIX = "{crypt}";    /* to distinguish clear text from DES crypted */
const string MD5_PREFIX = "$1$";

int
CLI::pass_matches
(
	string pass,
	string tried_pass
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int des;
	int idx = 0;
	des = ! pass.compare (0, DES_PREFIX.size(), DES_PREFIX);
	if (des)
	{
		idx = sizeof ( DES_PREFIX )-1;
	}
	if ( des || (! pass.compare (0, MD5_PREFIX.size(), MD5_PREFIX)))
	{
		tried_pass = crypt ( (char*) tried_pass.c_str(), (char*) pass.c_str() );
	}
	return ! pass.compare (idx, pass.size(), tried_pass);
}

void
CLI::show_prompt
()
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );



	if ( hostname != "" )
	{
		client << hostname << commit;
	}
	if ( modestring != "" )
	{
		client << modestring << commit;
	}
	client.lines_out = 0;
	client << prompt << commit;
}

unsigned char
CLI::map_esc
()
{
	unsigned char c;
	client.read_char (c);
	client.read_char (c);
	/* remap to readline control codes */
	switch ( c )
	{
	case 'A': /* Up */
		c = CTRL ( 'P' );
		break;
	case 'B': /* Down */
		c = CTRL ( 'N' );
		break;
	case 'C': /* Right */
		c = CTRL ( 'F' );
		break;
	case 'D': /* Left */
		c = CTRL ( 'B' );
		break;
	default:
		c = 0;
	}
	return c;
}

void
CLI::handle_telnet_option ()
{
	unsigned char c;
	client.read_char (c);
	switch (c)
	{
		case 0xfb:	// WILL
		case 0xfc:	// WON'T
		case 0xfd:	// DO
		case 0xfe:	// DON'T
			client.read_char (c);
			break;
	}
}

int
CLI::get_input
(
	unsigned char& c
)
{
	int ret = 0;
	while (ret == 0)
	{
		ret = client.wait_for_input(1);
		if (ret == SOCKET_ERROR)
		{	// error
			if ( RECOVERABLE_ERROR )
				continue;
			perror ("read");
			return ret;
		}
		if ( ret == 0 )
		{	/* timeout every second */
			if ( this->regular_callback && this->regular_callback() != LIBCLI::OK )
				break;
			continue;
		}
		ret = client.read_char (c);
		if (ret == SOCKET_ERROR)
		{
			if ( errno == EINTR )
				continue;
			return ret;
		}
		return ret;
	}
	return ret;
}

void
CLI::check_enable ( const char* pass )
{
	int allowed = 0;
	if ( enable_password != "" )
	{	// check stored static enable password 
		if ( pass_matches ( enable_password, pass ) )
		{
			allowed++;
		}
	}
	if ( !allowed )
	{
		/* check callback */
		if (this->enable_callback)
		{
			if ( enable_callback ( pass ) )
			{
				allowed++;
			}
		}
		else if (cpp_enable_callback)
		{
			if ( CALL_MEMBER_FN ((*this), cpp_enable_callback) ( pass ) )
			{
				allowed++;
			}
		}
	}
	if ( allowed )
	{
		client << "-ok-" << CRLF;
		state = LIBCLI::STATE_ENABLE;
		set_privilege ( LIBCLI::PRIVILEGED );
	}
	else
	{
		client << CRLF << CRLF << "Access denied" << CRLF;
		state = LIBCLI::STATE_NORMAL;
	}
}

void
CLI::check_user_auth
(
	const string& username,
	const string& password
)
{
	/* require password */
	int allowed = 0;
	if ( this->auth_callback )
	{
		if ( this->auth_callback ( username, password ) == LIBCLI::OK )
		{
			allowed++;
		}
	}
	else if ( this->cpp_auth_callback )
	{
		if ( CALL_MEMBER_FN ((*this), cpp_auth_callback)  ( username, password ) == LIBCLI::OK )
		{
			allowed++;
		}
	}
	if ( ! allowed )
	{
		unp_it u = users.find (username);
		if (u != users.end())
		{
			if (pass_matches (u->second, password))
				allowed++;
		}
	}
	if ( allowed )
	{
		client << "-ok-" << CRLF;
		this->state = STATE_NORMAL;
		client << "type '?' or 'help' for help." << CRLF << CRLF;
	}
	else
	{
		client << CRLF << CRLF << "Access denied" << CRLF;
		this->state = LIBCLI::STATE_LOGIN;
	}
	showprompt = true;
}

void
CLI::delete_backwards
(
	const unsigned char c
)
{
	int back = 0;

	if ( length == 0 || cursor == 0 )
	{
		client.put_char ('\a');
		return;
	}
	if ( c == CTRL ( 'W' ) ) /* word */
	{
		int nc = cursor;
		while ( nc && cmd[nc - 1] == ' ' )
		{
			nc--;
			back++;
		}
		while ( nc && cmd[nc - 1] != ' ' )
		{
			nc--;
			back++;
		}
	}
	else /* char */
	{
		back = 1;
	}
	while ( back-- )
	{
		if ( length == cursor )
		{
			cmd[--cursor] = 0;
			if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
			{
				client << "\b \b" << commit;
			}
		}
		else
		{
			int i;
			cursor--;
			if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
			{
				for ( i = cursor; i <= length; i++ )
				{
					cmd[i] = cmd[i+1];
				}
				client.put_char ('\b');
				client << (cmd + cursor) << commit;
				client.put_char (' ');
				for ( i = 0; i <= ( int ) strlen ( cmd + cursor ); i++ )
				{
					client.put_char ('\b');
				}
			}
		}
		length--;
	}
}

void
CLI::prompt_user ()
{
	switch ( this->state )
	{
	case STATE_LOGIN:
		client << "Username: " << commit;
		break;
	case STATE_PASSWORD:
		client << "Password: " << commit;
		break;
	case STATE_NORMAL:
	case STATE_ENABLE:
		show_prompt ();
		client << cmd << commit;
		if ( cursor < length )
		{
			int n = length - cursor;
			while ( n-- )
			{
				client.put_char ('\b');
			}
		}
		break;
	case STATE_ENABLE_PASSWORD:
		client << "Password: " << commit;
		break;
	}
	showprompt = false;
}

void
CLI::redraw_line
()
{
	if ( this->state == STATE_PASSWORD || this->state == STATE_ENABLE_PASSWORD )
	{
		return;
	}
	client << CRLF;
	show_prompt ();
	client << cmd << commit;
	for ( int i = 0; i < (length - cursor); i++ )
	{
		client.put_char ('\b');
	}
}

void
CLI::clear_line
()
{
	if ( this->state == STATE_PASSWORD || this->state == STATE_ENABLE_PASSWORD )
	{
		memset ( cmd, 0, length );
	}
	else
	{
		clear_line ( cmd, length, cursor );
	}
	length = cursor = 0;
}

void
CLI::clear_to_eol
()
{
	if ( cursor == length )
	{
		return;
	}
	if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
	{
		int c;
		for ( c = cursor; c < length; c++ )
		{
			client.put_char (' ');
		}
		for ( c = cursor; c < length; c++ )
		{
			client.put_char ('\b');
		}
	}
	memset ( cmd + cursor, 0, length - cursor );
	length = cursor;
}

bool
CLI::try_logout
()
{
	if ( length )
	{
		return false;
	}
	strcpy ( cmd, "quit" );
	length = cursor = strlen ( cmd );
	client << "quit" << CRLF;
	return true;
}

void
CLI::leave_config_mode
()
{
	if ( this->mode != MODE_EXEC )
	{
		client << CRLF;
		clear_line ( cmd, length, cursor );
		set_configmode ( MODE_EXEC, "" );
		showprompt = true;
	}
	else
	{
		client.put_char ('\a');
	}
}

void
CLI::list_completions
()
{
	char* completions[128];
	int num_completions = 0;
	if ( this->state == STATE_PASSWORD || this->state == STATE_ENABLE_PASSWORD )
	{
		return;
	}
	if ( cursor != length )
	{
		return;
	}
	num_completions = get_completions ( cmd, completions, 128 );
	showprompt = true;
	if ( num_completions == 1 )
	{	// Single completion
		for ( ; length > 0; length--, cursor-- )
		{
			if ( cmd[length-1] == ' ' || cmd[length-1] == '|' )
			{
				break;
			}
			client.put_char ('\b');
		}
		strcpy ( ( cmd + length ), completions[0] );
		length += strlen ( completions[0] );
		cmd[length++] = ' ';
		cursor = length;
		client << CRLF;
	}
	if ( num_completions == 0 )
	{
		client << CRLF;
		client << "  <br>" << CRLF;
	}
}

void
CLI::do_history
(
	const unsigned char& c
)
{
	int history_found = 0;
	if ( this->state == STATE_PASSWORD || this->state == STATE_ENABLE_PASSWORD )
	{
		return;
	}
	if ( c == CTRL ( 'P' ) ) // Up
	{
		in_history--;
		if ( in_history < 0 )
		{
			for ( in_history = MAX_HISTORY-1; in_history >= 0; in_history-- )
			{
				if ( this->history[in_history] )
				{
					history_found = 1;
					break;
				}
			}
		}
		else
		{
			if ( this->history[in_history] )
			{
				history_found = 1;
			}
		}
	}
	else // Down
	{
		in_history++;
		if ( in_history >= MAX_HISTORY || !this->history[in_history] )
		{
			int i = 0;
			for ( i = 0; i < MAX_HISTORY; i++ )
			{
				if ( this->history[i] )
				{
					in_history = i;
					history_found = 1;
					break;
				}
			}
		}
		else
		{
			if ( this->history[in_history] )
			{
				history_found = 1;
			}
		}
	}
	if ( history_found && this->history[in_history] )
	{
		// Show history item
		clear_line ( cmd, length, cursor );
		memset ( cmd, 0, 4096 );
		strncpy ( cmd, this->history[in_history], 4095 );
		length = cursor = strlen ( cmd );
		client << cmd << commit;
	}
}

void
CLI::cursor_left
()
{
	if ( cursor == 0 )
	{
		return;
	}
	if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
	{
		client.put_char ('\b');
	}
	cursor--;
}

void
CLI::cursor_right
()
{
	if ( cursor >= length )
	{
		return;
	}
	if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
	{
		client.put_char (cmd[cursor]);
	}
	cursor++;
}

void
CLI::jump_start_of_line
()
{
	if ( cursor )
	{
		if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
		{
			client.put_char ('\r');
			show_prompt ();
		}
		cursor = 0;
	}
}

void
CLI::jump_end_of_line
()
{
	if ( cursor < length )
	{
		if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
		{
			client << (cmd + cursor) << commit;
		}
		cursor = length;
	}
}

bool
CLI::append
(
	const unsigned char& c
)
{
	cmd[cursor] = c;
	if ( length < 4095 )
	{
		length++;
		cursor++;
		cmd[cursor] = 0;
		return true;
	}
	client.put_char ('\a');
	return false;
}

void
CLI::insert
(
	const unsigned char& c
)
{
	int  insertmode = 1;	// what keypress could change this?
	// Middle of text
	if ( insertmode )
	{
		int i;
		// Move everything one character to the right
		if ( length >= 4094 )
		{
			length--;
		}
		for ( i = length; i >= cursor; i-- )
		{
			cmd[i + 1] = cmd[i];
		}
		// Write what we've just added
		cmd[cursor] = c;
		client << (cmd + cursor) << commit;
		for ( i = 0; i < ( length - cursor + 1 ); i++ )
		{
			client.put_char ('\b');
		}
		length++;
		cmd[length] = 0;
	}
	else
	{
		cmd[cursor] = c;
	}
	cursor++;
}

int
CLI::loop
()
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	unsigned char c;
	bool remember_command;

	this->state = STATE_LOGIN;
	free_history ();
	if ( ( cmd = ( char* ) malloc ( 4096 ) ) == NULL )
	{
		return LIBCLI::ERROR_ANY;
	}
	if ( banner != "" )
	{
		client << banner << CRLF;
	}
	/* start off in unprivileged mode */
	set_privilege ( LIBCLI::UNPRIVILEGED );
	set_configmode ( LIBCLI::MODE_EXEC, "" );
	/* no auth required? */
	if ( users.empty() && !this->auth_callback && !this->cpp_auth_callback )
	{
		this->state = STATE_NORMAL;
	}
	showprompt = true;
	while ( 1 )
	{
		remember_command = false;
		if (showprompt == true)
		{
			prompt_user ();
		}
		if (get_input (c) == SOCKET_ERROR)
		{
			length = -1;
			break;
		}
		switch (c)
		{
		case 255:
			handle_telnet_option();
			continue;
		case 27:		// handle ANSI arrows
			c = map_esc ();
			switch (c)
			{
			case CTRL ( 'B' ):	// cursor left
				cursor_left ();
				continue;
			case CTRL ( 'F' ):	// cursor right
				cursor_right ();
				continue;
			case CTRL ( 'P' ):	// Cursor Up
			case CTRL ( 'N' ):	// Cursor Down
				do_history (c);
				continue;
			}
			continue;
		case '\n':
		case '\r':
			showprompt = true;
			if ( state != STATE_PASSWORD && state != STATE_ENABLE_PASSWORD )
				client << CRLF;
			break;
		case 0:
			continue;
		case CTRL ( 'C' ):
			client.put_char ('\a');
			continue;
		case CTRL ( 'W' ):	// back word
		case CTRL ( 'H' ):	// backspace
		case 0x7f:		// delete
			delete_backwards (c);
			continue;
		case CTRL ( 'L' ):	// redraw
			redraw_line ();
			continue;
		case CTRL ( 'U' ):	// clear line
			clear_line ();
			continue;
		case CTRL ( 'K' ):
			clear_to_eol ();
			continue;
		case CTRL ( 'D' ):	// EOT
			if (try_logout () == true)
				break;
			continue;
		case CTRL ( 'Z' ):	// leave config mode
			leave_config_mode ();
			continue;
		case CTRL ( 'I' ):	// TAB completion
			list_completions ();
			continue;
		case CTRL ( 'A' ):	// start of line
			jump_start_of_line ();
			continue;
		case CTRL ( 'E' ):	// end of line
			jump_end_of_line ();
			continue;
		default:		// normal character typed
			if ( cursor == length )
			{
				if (! append (c))
				{
					continue;
				}
			}
			else
			{	// Middle of text
				insert (c);
			}
			if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
			{
				if ( (c == '?') && (cursor == length) )
				{
					client << CRLF;
					remember_command = true;
					showprompt = true;
					break;
				}
				client.put_char (c);
			}
			continue;
		} // switch
		if ( !strcasecmp ( cmd, "quit" ) )
		{
			break;
		}
		if ( this->state == STATE_LOGIN )
		{
			if ( length == 0 )
			{
				continue;
			}
			/* require login */
			username = cmd;
			this->state = STATE_PASSWORD;
			showprompt = true;
		}
		else if ( this->state == STATE_PASSWORD )
		{
			check_user_auth (username, cmd);
		}
		else if ( this->state == LIBCLI::STATE_ENABLE_PASSWORD )
		{
			check_enable (cmd);
		}
		else
		{
			if ( length == 0 )
			{
				continue;
			}
			if ( cmd[length - 1] != '?' && strcasecmp ( cmd, "history" ) != 0 )
			{
				add_history ( cmd );
			}
			if ( run_command ( cmd ) == LIBCLI::QUIT )
			{
				break;
			}
			if ( (c == '?') && (cursor == length) )
			{
				cursor = length -1;
				length = cursor;
				cmd[length] = 0;
			}
			showprompt = true;
		}
		if (remember_command == false)
		{
			memset ( cmd, 0, 4096 );
			length = 0;
			cursor = 0;
		}
	}
	free_z ( cmd );
	return LIBCLI::OK;
}

int
CLI::file
(
        FILE* fh,
        int privilege,
        int mode
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int oldpriv = set_privilege ( privilege );
	int oldmode = set_configmode ( mode, "" );
	char buf[4096];
	while ( 1 )
	{
		char* p;
		char* cmd;
		char* end;
		if ( fgets ( buf, sizeof ( buf ), fh ) == NULL )
		{
			break;        /* end of file */
		}
		if ( ( p = strpbrk ( buf, "#\r\n" ) ) )
		{
			*p = 0;
		}
		cmd = buf;
		while ( isspace ( *cmd ) )
		{
			cmd++;
		}
		if ( !*cmd )
		{
			continue;
		}
		for ( p = end = cmd; *p; p++ )
		{
			if ( !isspace ( *p ) )
			{
				end = p;
			}
		}
		*++end = 0;
		if ( strcasecmp ( cmd, "quit" ) == 0 )
		{
			break;
		}
		if ( run_command ( cmd ) == LIBCLI::QUIT )
		{
			break;
		}
	}
	set_privilege ( oldpriv );
	set_configmode ( oldmode, "" /* didn't save desc */ );
	return LIBCLI::OK;
}

int
CLI::pager
()
{
	unsigned char c;
	bool done = false;
	client << UNFILTERED << "--- more ---" << commit;
	c = ' ';
	while ( done == false )
	{
		client.read_char (c);
		if (c == 'q')
		{
			client.lines_out = 0;
			client.put_char ('\r');
			return 1;
		}
		if (c == ' ')
		{
			client.lines_out = 0;
			done = true;
		}
		if ((c == '\r') || (c == '\n'))
		{	// show next line and reprompt for more
			client.lines_out--;
			done = true;
		}
	}
	#if 0
	client.put_char ('\r');
	client << UNFILTERED << "            " << commit;
	#endif
	client.put_char ('\r');
	return 0;
}

bool
CLI::check_pager
()
{
	// ask for a key after 20 lines of output
	// FIXME: make this configurable
	if (client.lines_out > client.max_screen_lines)
	{
		if (pager ())
		{
			return 1;
		}
	}
	return 0;
}

int
CLI::print
(
        const char* format,
        ...
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	if (! format)
	{
		return 0;
	}
	va_list ap;
	va_start ( ap, format );
	char buf[5000];
	vsprintf (buf, format, ap);
	va_end ( ap );
	client << buf << CRLF;
	return check_pager();
}

}; // namespace LIBCLI

