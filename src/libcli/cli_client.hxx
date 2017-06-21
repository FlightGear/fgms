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

#ifndef CLI_CLIENT_H
#define CLI_CLIENT_H

#include <string>
#include <sstream>
#include <iomanip>
#include <fglib/netsocket.hxx>
#include <fglib/fg_util.hxx>
#ifndef _MSC_VER
	#include <termios.h>
#endif
#include "common.hxx"
#include "filter.hxx"

namespace LIBCLI
{

class CLI;

class match_filter_state                                                                                                     
{       
public: 
	int flags;
	char* str;
};      

class range_filter_state
{       
public: 
	int matched;
	char* from;
	char* to;
};      

class Client
{
public:
	friend class CLI;
	Client ( int fd );
	~Client ();
	int wait_for_input ( int seconds );	// select()
	int read_char ( unsigned char& c );
	void put_char ( const char& c );

	template <class T> Client& operator << ( T v );
	Client& operator << ( Client& (*f) (Client&) );

	friend Client& commit ( Client& );
	friend Client& CRLF ( Client& );
	friend Client& UNFILTERED ( Client& );
	size_t  lines_out;
	size_t  max_screen_lines;
	filter_t*   filters;
protected:
	char*	join_words ( int argc, char** argv );
	int     match_filter_init ( int argc, char** argv, filter_t* filt );
	int     range_filter_init ( int argc, char** argv, filter_t* filt );
	int     count_filter_init ( int argc, char** argv, filter_t* filt );                                                 
	int     match_filter ( char* cmd, void* data );
	int     range_filter ( char* cmd, void* data );
	int     count_filter ( char* cmd, void* data );
	PRINT_MODE		m_print_mode;
	NetSocket*		m_socket;
	std::ostringstream	m_output;
	#ifndef _MSC_VER
	struct termios OldModes;
	#endif
};

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
template <class T>
Client& Client::operator << ( T v )
{
	m_output << v;
	return *this;
} // operator << ( class T );
//////////////////////////////////////////////////////////////////////

Client& commit ( Client& );
Client& CRLF ( Client& out );
Client& UNFILTERED ( Client& out );

}; // namespace LIBCLI

#endif
