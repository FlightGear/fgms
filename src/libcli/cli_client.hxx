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

#ifndef CLI_COMMON_H
#define CLI_COMMON_H

#include <string>
#include <sstream>
#include <plib/netSocket.h>
#ifndef _MSC_VER
	#include <termios.h>
#endif


namespace LIBCLI
{

class Client
{
public:
	Client ( int fd );
	~Client ();
	int wait_for_input ();	// select()
	void close ();
	template <class T>
	Client& operator << ( T v );
	Client& operator << ( const char& c );
	Client& operator << ( Client& (*f) (Client&) );
	friend Client& endl ( Client& );
private:
	netSocket*		socket;
	std::ostringstream	m_output;
	#ifndef _MSC_VER
	struct termios NewModes;
	#endif
};

}; // namespace LIBCLI

#endif
