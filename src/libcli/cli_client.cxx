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
// Copyright (C) 2011  Oliver Schroeder
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include "common.hxx"
#include "cli_client.hxx"

#ifdef __CYGWIN__
#include <sys/select.h>
#include <_timeval.h>
#endif


#ifdef _MSC_VER
	#include <conio.h> // for _kbhit(), _getch
	#define kbhit	_kbhit
	#define getchar	_getch

	bool wait_for_key ( unsigned timeout_ms = 0 )
	{
		return WaitForSingleObject(
			GetStdHandle( STD_INPUT_HANDLE ),
			timeout_ms
		) == WAIT_OBJECT_0;
	}
#endif

namespace libcli
{

//////////////////////////////////////////////////////////////////////
/** Construct a client connection
 */
cli_client::cli_client
(
	int fd
)
{
	lines_out = 0;
	filters   = 0;
	max_screen_lines = 0;
	m_print_mode	 = PRINT_FILTERED;
	if (fd == fileno ( stdin ))
	{	// setup terminal attributes
		m_socket = 0;
		#ifndef _MSC_VER
			struct termios NewModes;
			setbuf ( stdin, ( char* ) 0 );
			(void) tcgetattr (fileno (stdin), &OldModes);
			NewModes = OldModes;
			NewModes.c_lflag &= ~ ( ICANON );
			NewModes.c_lflag &= ~ ( ECHO | ECHOE | ECHOK );
			NewModes.c_lflag |= ECHONL;
			NewModes.c_cc[VMIN]  = 0;
			NewModes.c_cc[VTIME] = 1;
			( void ) tcsetattr ( fileno ( stdin ), TCSANOW, &NewModes );
		#else
			// not required, but does not seem to harm
			AllocConsole();
			// needed to use WaitForSingleObject(GetStdHandle
			//   ( STD_INPUT_HANDLE ),timeout_ms);
			freopen ( "conin$", "r", stdin );
			// only required IFF console output redirected
			freopen ( "conout$", "w", stdout );
			// this break the redirection, so CLI can be
			// always seen
			freopen ( "conout$", "w", stderr );
 		#endif
	}
	else
	{	// setup telnet session
		m_socket = new fgmp::netsocket();
		m_socket->handle (fd);
		const char* negotiate =
			"\xFF\xFB\x03"	// WILL SUPPRESS GO AHEAD OPTION
			"\xFF\xFB\x01"	// WILL ECHO
			"\xFF\xFD\x03"	// DO SUPPRESS GO AHEAD OPTION
			"\xFF\xFD\x01";	// DO ECHO
		m_socket->send (negotiate, strlen ( negotiate ), 0 );
	}
} // cli_client::cli_client ()

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 */
cli_client::~cli_client
()
{
	if (m_socket == 0)
	{	// restore terminal attributes
		#ifndef _MSC_VER
		( void ) tcsetattr ( fileno ( stdin ), TCSANOW, &OldModes );
		#endif
	}
	else
	{
		m_socket->close();
		delete m_socket;
	}
} // cli_client::~cli_client ()

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//	return 0:	timeout
//	return >0:	input available
//	return <0:	error
//////////////////////////////////////////////////////////////////////
int
cli_client::wait_for_input
(
	int seconds
)
{
#ifdef _MSC_VER
	if (m_socket == 0)
	{
		return wait_for_key (seconds * 1000);
	}
#endif
	if (m_socket != 0)
	{
		fgmp::netsocket*  ListenSockets[2];
		ListenSockets[0] = m_socket;
		ListenSockets[1] = 0;
		return m_socket->select ( ListenSockets, 0, seconds );
	}
	else
	{
	// FIXME: for console session
	// should use m_socket->select(), too
		struct timeval tv ;
		tv.tv_sec = seconds;
		tv.tv_usec = 0;
		fd_set r;
		FD_ZERO (&r);
		FD_SET (0, &r);
		return ::select (FD_SETSIZE, &r, 0, 0, &tv);
	}
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
int
cli_client::read_char
(
	unsigned char& c
)
{
	if (m_socket != 0)
	{
		return m_socket->recv_char(c);
	}
	c = getchar();
	return 1;
} // :read_char ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
void
cli_client::put_char
(
	const char& c
)
{
	if (m_socket != 0)
	{
		m_socket->send (&c, 1);
	}
	else
	{
		std::cout << c;	
		std::cout.flush();
	}
	// m_output.str("");
} // operator << ( c );
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
cli_client&
cli_client::operator <<
(
	cli_client& (*f) (cli_client&)
)
{
	return ((*f)(*this));
} // operator << ( cli_client& (*f) )
//////////////////////////////////////////////////////////////////////

char*
cli_client::join_words
(
	int argc,
	char** argv
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	char* p;
	int len = 0;
	int i;  
	for ( i = 0; i < argc; i++ )
	{
		if ( i )
		{
			len += 1;
		}
		len += strlen ( argv[i] );
	}
	p = ( char* ) malloc ( len + 1 );
	p[0] = 0;
	for ( i = 0; i < argc; i++ ) 
	{
		if ( i )
		{
			strcat ( p, " " );
		}
		strcat ( p, argv[i] );
	}
	return p;
}       

int
cli_client::match_filter_init
(
	int argc,
	char** argv,
	filter_t* filt
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	match_filter_state* state;
	if ( argc < 2 )
	{
		*this << UNFILTERED << "Match filter requires an argument" << CRLF;
		return libcli::ERROR_ANY;
	}
	filt->filter = &cli_client::match_filter;
	state = new match_filter_state;
	filt->data = state;
	state->flags = MATCH_NORM;
	if ( argv[0][0] == 'e' )
	{
		state->flags = MATCH_INVERT;
	}
	state->str = join_words ( argc-1, argv+1 );
	return libcli::OK;
}

int
cli_client::range_filter_init
(
	int argc,
	char** argv,
	filter_t* filt
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	range_filter_state* state;
	char* from = 0;
	char* to = 0;
	if ( !strncmp ( argv[0], "bet", 3 ) ) // between
	{
		if ( argc < 3 )
		{
			*this << UNFILTERED << "Between filter requires 2 arguments" << CRLF;
			return libcli::ERROR_ANY;
		}
		if ( ! ( from = strdup ( argv[1] ) ) )
		{
			return libcli::ERROR_ANY;
		}
		to = join_words ( argc-2, argv+2 );
	}
	else // begin
	{
		if ( argc < 2 )
		{
			*this << UNFILTERED << "Begin filter requires an argument" << CRLF;
			return libcli::ERROR_ANY;
		}
		from = join_words ( argc-1, argv+1 );
	}
	filt->filter = &cli_client::range_filter;
	state = new range_filter_state;
	filt->data = state;
	state->matched = 0;
	state->from = from;
	state->to = to;
	return libcli::OK;
}

int
cli_client::count_filter_init
(
	int argc,
	UNUSED ( char** argv ),
	filter_t* filt
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	if ( argc > 1 )
	{
		*this << UNFILTERED << "Count filter does not take arguments" << CRLF;
		return libcli::ERROR_ANY;
	}
	filt->filter = &cli_client::count_filter;
	filt->data = new int(0);
	if ( ! filt->data )
	{
		return libcli::ERROR_ANY;
	}
	return libcli::OK;
}

int
cli_client::match_filter
(
	char* cmd,
	void* data
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	match_filter_state* state = reinterpret_cast<match_filter_state*> ( data );
	int r = libcli::ERROR_ANY;
	if ( !cmd ) // clean up
	{
		free ( state->str );
		free ( state );
		return libcli::OK;
	}
	if ( strstr ( cmd, state->str ) )
	{
		r = libcli::OK;
	}
	if ( state->flags & MATCH_INVERT )
	{
		if ( r == libcli::OK )
		{
			r = libcli::ERROR_ANY;
		}
		else
		{
			r = libcli::OK;
		}
	}
	return r;
}

int
cli_client::range_filter
(
	char* cmd,
	void* data
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	range_filter_state* state = ( range_filter_state* ) data;
	int r = libcli::ERROR_ANY;
	if ( !cmd ) // clean up
	{
		free_z ( state->from );
		free_z ( state->to );
		free_z ( state );
		return libcli::OK;
	}
	if ( !state->matched )
	{
		state->matched = !!strstr ( cmd, state->from );
	}
	if ( state->matched )
	{
		r = libcli::OK;
		if ( state->to && strstr ( cmd, state->to ) )
		{
			state->matched = 0;
		}
	}
	return r;
}

int     
cli_client::count_filter
(
	char* cmd,
	void* data
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int* count = ( int* ) data;
	if ( !cmd ) // clean up
	{
		// print count
		*this << UNFILTERED << num_to_str ( *count ) << CRLF;
		free ( count ); 
	return libcli::OK; 
	}
	while ( isspace ( *cmd ) )
	{
		cmd++;
	}
	if ( *cmd )
	{
		( *count ) ++;        // only count non-blank lines
	}
	return libcli::ERROR_ANY; // no output
}

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
cli_client& commit
(
	cli_client& out
)
{
	filter_t* f = (out.m_print_mode & PRINT_FILTERED) ? out.filters : 0;
	bool print = true;

	char* p = (char*) out.m_output.str().c_str();
	while (print && f)
	{
		print = ( f->exec ( out, p, f->data ) == libcli::OK );
		f = f->next;
	}
	if (print)
	{
		if (out.m_socket != 0)
		{
			std::string s = out.m_output.str();
			size_t l = s.size();
			out.m_socket->send (s.c_str(), l);
		}
		else
		{
			std::cout << out.m_output.str();
			std::cout.flush();
		}
		if (out.m_print_mode == PRINT_FILTERED)
		{	// only count lines in filtered mode
			out.lines_out++;
		}
	}
	out.m_output.str ("");
	out.m_print_mode = PRINT_FILTERED;
	return out;
} // cli_client& commit()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
cli_client& CRLF
(
	cli_client& out
)
{
	out.m_output << "\r\n";
	commit (out);
	return out;
} // CRLF()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
cli_client& UNFILTERED
(
	cli_client& out
)
{
	out.m_print_mode = PRINT_PLAIN;
	return out;
} // UNFILTERED()
//////////////////////////////////////////////////////////////////////

} // namespace libcli
