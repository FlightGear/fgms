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

#ifndef CLI_CLIENT_H
#define CLI_CLIENT_H

#include <string>
#include <sstream>
#include <iomanip>
#include <memory>
#include <plib/netSocket.h>
#ifndef _MSC_VER
#include <termios.h>
#endif
#include "common.hxx"
#include "filter.hxx"

namespace libcli
{

class cli;

enum class PRINT_MODE
{
	PLAIN = 0x00,
	FILTERED = 0x01
};

class client
{
public:
	using filter_list = std::vector <  filter* >;
	//using filter_list = std::vector < std::shared_ptr < filter > >;
	friend class cli;
	client ( int fd );
	~client ();
	int		wait_for_input ( int seconds );	// select()
	int		read_char ( unsigned char& c );
	void	put_char ( const char& c );
	int		get_input ( unsigned char& c );

	template <class T> client& operator << ( T v );
	client& operator << ( client& ( *f ) ( client& ) );

	friend client& commit ( client& );
	friend client& crlf ( client& );
	friend client& unfiltered ( client& );
	size_t  	lines_out;
	size_t  	max_screen_lines;
protected:
	PRINT_MODE	m_print_mode;
	netSocket* m_socket;
	std::ostringstream	m_output;
	filter_list			m_active_filters;	///< list of active filters
#ifndef _MSC_VER
	struct termios OldModes;
#endif
};

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
template <class T>
client& client::operator << ( T v )
{
	m_output << v;
	return *this;
} // operator << ( class T );
//////////////////////////////////////////////////////////////////////

client& commit ( client& );
client& crlf ( client& out );
client& unfiltered ( client& out );

}; // namespace libcli

#endif
