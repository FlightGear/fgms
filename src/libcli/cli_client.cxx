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
#include "cli_client.hxx"

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
	max_screen_lines = 22;
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
		netSocket*  ListenSockets[1];
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

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
Client& commit
(
	Client& out
)
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
	out.m_output.str ("");
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
	out.lines_out++;
	return commit (out);
} // CRLF()
//////////////////////////////////////////////////////////////////////

}; // namespace LIBCLI
