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

#include <cli_client.hxx>

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
Client::Client ( int fd )
{
	if (fd == fileno ( stdin ))
	{	// setup terminal attributes
		socket = 0;
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
		#endif
	}
	else
	{	// setup telnet session
		socket = new netSocket();
		socket->setHandle (fd);
		const char* negotiate =
			"\xFF\xFB\x03"
			"\xFF\xFB\x01"
			"\xFF\xFD\x03"
			"\xFF\xFD\x01";
		socket->send (negotiate, strlen ( negotiate ), 0 );
	}
} Client::Client ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
Client::~Client ()
{
	if (m_socket == 0)
	{	// restore terminal attributes
		#ifndef _MSC_VER
		( void ) tcsetattr ( fileno ( stdin ), TCSANOW, &OldModes );
		#endif
	}
} // Client::~Client ()
//////////////////////////////////////////////////////////////////////

int wait_for_input ();	// select()
void close ();
template <class T>
Client& operator << ( T v );
Client& operator << ( const char& c );
Client& operator << ( Client& (*f) (Client&) );
friend Client& endl ( Client& );


private:
	bool			have_socket;
	netSocket*		socket;
	std::ostringstream	m_output;

