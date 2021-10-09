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
	return WaitForSingleObject (
		GetStdHandle ( STD_INPUT_HANDLE ),
		timeout_ms
	) == WAIT_OBJECT_0;
}
#endif

namespace libcli
{

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
client::client
(
	int fd
)
{
	lines_out = 0;
	max_screen_lines = 22;
	m_print_mode = PRINT_MODE::FILTERED;
	if ( fd == fileno ( stdin ) )
	{	// setup terminal attributes
		m_socket = 0;
#ifndef _MSC_VER
		struct termios NewModes;
		setbuf ( stdin, ( char* ) 0 );
		( void ) tcgetattr ( fileno ( stdin ), &OldModes );
		NewModes = OldModes;
		NewModes.c_lflag &= ~( ICANON );
		NewModes.c_lflag &= ~( ECHO | ECHOE | ECHOK );
		NewModes.c_lflag |= ECHONL;
		NewModes.c_cc[VMIN] = 0;
		NewModes.c_cc[VTIME] = 1;
		( void ) tcsetattr ( fileno ( stdin ), TCSANOW, &NewModes );
#else
		AllocConsole (); // not required, but does not seem to harm
		freopen ( "conin$", "r", stdin ); // needed to use WaitForSingleObject(GetStdHandle( STD_INPUT_HANDLE ),timeout_ms);
		freopen ( "conout$", "w", stdout ); // only required IFF console output redirected
		freopen ( "conout$", "w", stderr ); // this break the redirection, so CLI can be always seen
#endif
	}
	else
	{	// setup telnet session
		m_socket = new netSocket ();
		m_socket->setHandle ( fd );
		const char* negotiate =
			"\xFF\xFB\x03"	// WILL SUPPRESS GO AHEAD OPTION
			"\xFF\xFB\x01"	// WILL ECHO
			"\xFF\xFD\x03"	// DO SUPPRESS GO AHEAD OPTION
			"\xFF\xFD\x01";	// DO ECHO
		m_socket->send ( negotiate, strlen ( negotiate ), 0 );
	}
} // client::client ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
client::~client
()
{
	if ( m_socket == 0 )
	{	// restore terminal attributes
#ifndef _MSC_VER
		( void ) tcsetattr ( fileno ( stdin ), TCSANOW, &OldModes );
#endif
	}
	else
	{
		m_socket->close ();
		delete m_socket;
	}
} // client::~client ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//	return 0:	timeout
//	return >0:	input available
//	return <0:	error
//////////////////////////////////////////////////////////////////////
int
client::wait_for_input
(
	int seconds
)
{
#ifdef _MSC_VER
	if ( m_socket == 0 )
	{
		return wait_for_key ( seconds * 1000 );
	}
#endif
	if ( m_socket != 0 )
	{
		netSocket* ListenSockets[2];
		ListenSockets[0] = m_socket;
		ListenSockets[1] = 0;
		return m_socket->select ( ListenSockets, 0, seconds );
	}
	else
	{
		struct timeval tv;
		tv.tv_sec = seconds;
		tv.tv_usec = 0;
		fd_set r;
		FD_ZERO ( &r );
		FD_SET ( 0, &r );
		return ::select ( FD_SETSIZE, &r, 0, 0, &tv );
	}
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
int
client::read_char
(
	unsigned char& c
)
{
	if ( m_socket != 0 )
	{
		return m_socket->read_char ( c );
	}
	c = getchar ();
	return 1;
} // :read_char ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
void
client::put_char
(
	const char& c
)
{
	if ( m_socket != 0 )
	{
		m_socket->send ( &c, 1 );
	}
	else
	{
		std::cout << c;
		std::cout.flush ();
	}
	// m_output.str("");
} // operator << ( c );
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
int
client::get_input
(
	unsigned char& c
)
{
	int ret = 0;
	c = 0x00;
	while ( ret == 0 )
	{
		ret = wait_for_input ( 1 );
		if ( ret == SOCKET_ERROR )
		{	// error
			if ( RECOVERABLE_ERROR )
				continue;
			perror ( "read" );
			return ret;
		}
		if ( ret == 0 )
		{	/* timeout every second */
			return ret;
		}
		ret = read_char ( c );
		if ( ret == SOCKET_ERROR )
		{
			if ( errno == EINTR )
				continue;
			return ret;
		}
		return ret;
	}
	return ret;
} // client::get_input ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
client&
client::operator <<
(
	client& ( *f ) ( client& )
	)
{
	return ( ( *f )( *this ) );
} // operator << ( client& (*f) )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
client& commit
(
	client& out
)
{
	bool print { true };

	const std::string output = out.m_output.str ();
	//
	// apply all filters to output line
	//
	if ( ( out.m_print_mode == PRINT_MODE::FILTERED ) && ( out.m_active_filters.size () > 0 ) )
	{
		for ( auto& f : out.m_active_filters )
		{
			print = f->exec ( output );
			if ( !print )
			{
				break;
			}
		}
	}
	//
	// put the output line on the stream
	//
	if ( print )
	{
		if ( out.m_socket != 0 )
		{
			std::string s = out.m_output.str ();
			size_t l = s.size ();
			out.m_socket->send ( s.c_str (), l );
		}
		else
		{
			std::cout << out.m_output.str ();
			std::cout.flush ();
		}
		if ( out.m_print_mode == PRINT_MODE::FILTERED )
		{	// only count lines in filtered mode
			out.lines_out++;
		}
	}
	out.m_output.str ( "" );
	out.m_print_mode = PRINT_MODE::FILTERED;
	return out;
} // client& commit()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
client& crlf
(
	client& out
)
{
	out.m_output << "\r\n";
	commit ( out );
	return out;
} // crlf()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
client& unfiltered
(
	client& out
)
{
	out.m_print_mode = PRINT_MODE::PLAIN;
	return out;
} // crlf()
//////////////////////////////////////////////////////////////////////

}; // namespace libcli
