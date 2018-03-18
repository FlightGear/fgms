//
// This file is part of fgms, the flightgear multiplayer server
// https://sourceforge.net/projects/fgms/
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
// along with this program; if not see <http://www.gnu.org/licenses/>
//

/**
 * @file        filter.cxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        03/2018
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdexcept>
#include "cli_client.hxx"
#include "common.hxx"
#include "filter.hxx"

namespace libcli
{

//////////////////////////////////////////////////////////////////////

match_filter::match_filter
(
	const std::string& match,
	bool invert
) : m_match_this ( match ), m_invert ( invert )
{} // match_filter::match_filter

//////////////////////////////////////////////////////////////////////

bool
match_filter::operator ()
(
	const std::string& line
)
{
	bool r = ( line.find ( m_match_this ) != std::string::npos );
	if ( m_invert )
		r = ( r == false );
	return r;
} // match_filter::operator () ()

//////////////////////////////////////////////////////////////////////

begin_filter::begin_filter
(
	const std::string& match
) : m_match_this ( match )
{} // begin_filter::begin_filter

//////////////////////////////////////////////////////////////////////

bool
begin_filter::operator ()
(
	const std::string& line
)
{
	if ( m_have_match )
		return true;
	if ( line.find ( m_match_this ) != std::string::npos )
		m_have_match = true;
	return m_have_match;
} // begin_filter::operator () ()

//////////////////////////////////////////////////////////////////////

between_filter::between_filter
(
	const std::string& start_match,
	const std::string& end_match
) : m_start_match ( start_match ), m_end_match ( end_match )
{} // between_filter::between_filter

//////////////////////////////////////////////////////////////////////

bool
between_filter::operator ()
(
	const std::string& line
)
{
	if ( ! m_have_match )
	{
		if ( line.find ( m_start_match ) != std::string::npos )
			m_have_match = true;
		return m_have_match;
	}
	if ( line.find ( m_end_match ) != std::string::npos )
		m_have_match = false;
	return m_have_match;
} // between_filter::operator () ()

//////////////////////////////////////////////////////////////////////

count_filter::count_filter
(
	cli_client* client
) : m_client ( client )
{} // count_filter::count_filter ()

//////////////////////////////////////////////////////////////////////

/**
 * print out counter when destroyed
 */
count_filter::~count_filter
()
{
	m_client->operator << ( m_counter );
	m_client->operator << ( "\r\n" );
} // count_filter::count_filter

//////////////////////////////////////////////////////////////////////

bool
count_filter::operator ()
(
	const std::string& line
)
{
	if ( line.find_first_not_of ( " " ) != std::string::npos )
		m_counter++;
	return false; // no output
} // count_filter::operator () ()

//////////////////////////////////////////////////////////////////////

pager_filter::pager_filter
(
	cli_client* client,
	size_t max_lines
) : m_client ( client )
{
	m_global_max = m_client->max_screen_lines ();
	m_client->max_screen_lines ( max_lines - 1 );
} // pager_filter::pager_filter ()

//////////////////////////////////////////////////////////////////////

pager_filter::~pager_filter
()
{
	m_client->max_screen_lines ( m_global_max ); // restore original value
} // pager_filter::~pager_filter ()

//////////////////////////////////////////////////////////////////////

bool
pager_filter::operator ()
(
	const std::string& line
)
{
	return true;
} // pager_filter::operator () ()

//////////////////////////////////////////////////////////////////////

file_filter::file_filter
(
	const std::string& filename,
	bool append
)
{
	if ( append )
		file.open ( filename, std::ofstream::out | std::ofstream::app );
	else
		file.open ( filename, std::ofstream::out );
	if ( ! file )
		throw std::runtime_error( "could not open '" + filename + "'" );
} // file_filter::file_filter ()

//////////////////////////////////////////////////////////////////////

file_filter::~file_filter
()
{
	if ( file )
		file.close ();
} // file_filter::~file_filter ()

//////////////////////////////////////////////////////////////////////

bool
file_filter::operator ()
(
	const std::string& line
)
{
	file << line;
	return false; // no output to screen
} // file_filter::operator () ()

//////////////////////////////////////////////////////////////////////

} // namespace libcli

