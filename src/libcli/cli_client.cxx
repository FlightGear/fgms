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

#ifdef __CYGWIN__
#include <_timeval.h>
#endif

namespace LIBCLI
{

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
Client::Client
(
	int fd
)
{
	lines_out = 0;
	filters   = 0;
	max_screen_lines = 22;
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
			AllocConsole();
			freopen ( "conin$", "r", stdin );
			freopen ( "conout$", "w", stdout );
			freopen ( "conout$", "w", stderr );
		#endif
	}
	else
	{	// setup telnet session
		m_socket = new netSocket();
		m_socket->setHandle (fd);
		const char* negotiate =
			"\xFF\xFB\x03"	// WILL SUPPRESS GO AHEAD OPTION
			"\xFF\xFB\x01"	// WILL ECHO
			"\xFF\xFD\x03"	// DO SUPPRESS GO AHEAD OPTION
			"\xFF\xFD\x01";	// DO ECHO
		m_socket->send (negotiate, strlen ( negotiate ), 0 );
	}
} // Client::Client ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
Client::~Client
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
} // Client::~Client ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//	return 0:	timeout
//	return >0:	input available
//	return <0:	error
//////////////////////////////////////////////////////////////////////
int
Client::wait_for_input
(
	int seconds
)
{
#ifdef _MSC_VER
	if (m_socket != 0)
	{
		return wait_for_key (seconds * 1000);
	}
#endif
	if (m_socket != 0)
	{
		netSocket*  ListenSockets[2];
		ListenSockets[0] = m_socket;
		ListenSockets[1] = 0;
		return m_socket->select ( ListenSockets, 0, seconds );
	}
	else
	{
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
Client::read_char
(
	unsigned char& c
)
{
	if (m_socket != 0)
	{
		return m_socket->read_char(c);
	}
	c = getchar();
	return 1;
} // :read_char ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
void
Client::put_char
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
		cout << c;	
		cout.flush();
	}
	// m_output.str("");
} // operator << ( c );
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
Client&
Client::operator <<
(
	Client& (*f) (Client&)
)
{
	return ((*f)(*this));
} // operator << ( Client& (*f) )
//////////////////////////////////////////////////////////////////////

char*
Client::join_words
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
Client::match_filter_init
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
		return LIBCLI::ERROR_ANY;
	}
	filt->filter = &Client::match_filter;
	state = new match_filter_state;
	filt->data = state;
	state->flags = MATCH_NORM;
	if ( argv[0][0] == 'e' )
	{
		state->flags = MATCH_INVERT;
	}
	state->str = join_words ( argc-1, argv+1 );
	return LIBCLI::OK;
}

int
Client::range_filter_init
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
			return LIBCLI::ERROR_ANY;
		}
		if ( ! ( from = strdup ( argv[1] ) ) )
		{
			return LIBCLI::ERROR_ANY;
		}
		to = join_words ( argc-2, argv+2 );
	}
	else // begin
	{
		if ( argc < 2 )
		{
			*this << UNFILTERED << "Begin filter requires an argument" << CRLF;
			return LIBCLI::ERROR_ANY;
		}
		from = join_words ( argc-1, argv+1 );
	}
	filt->filter = &Client::range_filter;
	state = new range_filter_state;
	filt->data = state;
	state->matched = 0;
	state->from = from;
	state->to = to;
	return LIBCLI::OK;
}

int
Client::count_filter_init
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
		return LIBCLI::ERROR_ANY;
	}
	filt->filter = &Client::count_filter;
	filt->data = new int(0);
	if ( ! filt->data )
	{
		return LIBCLI::ERROR_ANY;
	}
	return LIBCLI::OK;
}

int
Client::match_filter
(
	char* cmd,
	void* data
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	match_filter_state* state = reinterpret_cast<match_filter_state*> ( data );
	int r = LIBCLI::ERROR_ANY;
	if ( !cmd ) // clean up
	{
		free ( state->str );
		free ( state );
		return LIBCLI::OK;
	}
	if ( strstr ( cmd, state->str ) )
	{
		r = LIBCLI::OK;
	}
	if ( state->flags & MATCH_INVERT )
	{
		if ( r == LIBCLI::OK )
		{
			r = LIBCLI::ERROR_ANY;
		}
		else
		{
			r = LIBCLI::OK;
		}
	}
	return r;
}

int
Client::range_filter
(
	char* cmd,
	void* data
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	range_filter_state* state = ( range_filter_state* ) data;
	int r = LIBCLI::ERROR_ANY;
	if ( !cmd ) // clean up
	{
		free_z ( state->from );
		free_z ( state->to );
		free_z ( state );
		return LIBCLI::OK;
	}
	if ( !state->matched )
	{
		state->matched = !!strstr ( cmd, state->from );
	}
	if ( state->matched )
	{
		r = LIBCLI::OK;
		if ( state->to && strstr ( cmd, state->to ) )
		{
			state->matched = 0;
		}
	}
	return r;
}

int     
Client::count_filter
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
		*this << UNFILTERED << NumToStr (*count, 0) << CRLF;
		free ( count ); 
	return LIBCLI::OK; 
	}
	while ( isspace ( *cmd ) )
	{
		cmd++;
	}
	if ( *cmd )
	{
		( *count ) ++;        // only count non-blank lines
	}
	return LIBCLI::ERROR_ANY; // no output
}

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
Client& commit
(
	Client& out
)
{
	filter_t* f = (out.m_print_mode & PRINT_FILTERED) ? out.filters : 0;
	bool print = true;

	char* p = (char*) out.m_output.str().c_str();
	while (print && f)
	{
		print = ( f->exec ( out, p, f->data ) == LIBCLI::OK );
		f = f->next;
	}
	if (print)
	{
		if (out.m_socket != 0)
		{
			string s = out.m_output.str();
			size_t l = s.size();
			out.m_socket->send (s.c_str(), l);
		}
		else
		{
			cout << out.m_output.str();
			cout.flush();
		}
		if (out.m_print_mode == PRINT_FILTERED)
		{	// only count lines in filtered mode
			out.lines_out++;
		}
	}
	out.m_output.str ("");
	out.m_print_mode = PRINT_FILTERED;
	return out;
} // Client& commit()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
Client& CRLF
(
	Client& out
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
Client& UNFILTERED
(
	Client& out
)
{
	out.m_print_mode = PRINT_PLAIN;
	return out;
} // CRLF()
//////////////////////////////////////////////////////////////////////

}; // namespace LIBCLI
