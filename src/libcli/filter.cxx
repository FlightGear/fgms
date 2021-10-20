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
// derived from libcli by David Parrish (david@dparrish.com)
// Copyright (C) 2011  Oliver Schroeder
//
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.hxx"
#include "filter.hxx"
#include "connection.hxx"

namespace libcli
{

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new match filter::match_filter object
 *
 * This is either 'grab' or 'exclude'.
 * 'grab' only prints lines which contain \a match_me.
 * 'exclude' only prints lines which do NOT contain \a match_me.
 * \a mode defines if we are 'grab' or 'exclude'
 *
 * @param mode 		If mode==MATCH_MODE::NORM we are 'grab', else we are 'exclude'
 * @param match_me 	String to match
 */
match_filter::match_filter
(
	const MATCH_MODE mode,
	const std::string& match_me
) : m_mode { mode }, m_match_me { match_me }
{} // match_filter::match_filter ( mode, matchme )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief check if \a line contains \a match_me
 *
 * If \a m_mode is @ref MATCH_MODE::INVERT, the result is inverted.
 *
 * @param line 		The output line
 * @return true 	If \a line contains \a match_me
 * @return false	If \a line does not contain \a match_me
 */
bool
match_filter::exec
(
	const::std::string& line
)
{
	bool r = false;
	if ( ( line.find ( m_match_me ) != std::string::npos ) || ( line.length () == 0 ) )
	{
		r = true;
	}
	if ( m_mode == MATCH_MODE::INVERT )
	{
		r = !r;
	}
	return r;
} // match_filter::operator () ( line )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new begin filter::begin_filter object
 * 
 * The begin filter starts output with a line conaining \a match_me.
 * 
 * @param match_me String to match
 */
begin_filter::begin_filter
(
	const std::string& match_me
) : m_matched { false }, m_match_me { match_me }
{} // begin_filter::begin_filter
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check if \a line contains \a m_match_me
 * 
 * @param line The output line to check.
 * @return true Always returns \a true when we have found a match
 * @return false Until we found the first match.
 */
bool
begin_filter::exec
(
	const::std::string& line
)
{
	if ( m_matched )
	{	// return true if we already found a match.
		return true;
	}
	if ( ( line.find ( m_match_me ) != std::string::npos ) || ( line.length () == 0 ) )
	{
		m_matched = true;
	}
	return m_matched;
} // begin_filter::exec ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new between filter::between_filter object
 * 
 * The between filter starts output with the first line containing
 * \a start_with, and stops output with a line containing
 * \a end_with
 * 
 * @param start_with String to start output with
 * @param end_with String to end output with
 */
between_filter::between_filter
(
	const std::string& start_with,
	const std::string& end_with
) : m_matched { false }, m_start_with { start_with }, m_end_with { end_with }
{} // between_filter::between_filter ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

/**
 * @brief Check for \a m_start_with and \a m_end_with
 * 
 * @param line The output line to check.
 * @return true  When the first line contains \a m_start_with and while \a m_end_with was not found.
 * @return false If the output is not between \a m_start_with and \a m_end_with
 */
bool
between_filter::exec
(
	const::std::string& line
)
{
	if ( m_matched )
	{	// we had a match, so look if we find the end
		if ( ( line.find ( m_end_with ) != std::string::npos ) || ( line.length () == 0 ) )
		{
			m_matched = false;
		}
	}
	else if ( ( line.find ( m_start_with ) != std::string::npos ) || ( line.length () == 0 ) )
	{
		m_matched = true;
	}
	return m_matched;
} // between_filter::exec ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new count filter::count filter object
 *
 * Count simply counts the lines which made it on the output stream.
 * Initialise counter to zero.
 */
count_filter::count_filter
() : m_count { 0 }
{} // count_filter::count_filter ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief count the lines which made it on the output stream.
 *
 * Only counts none-empty lines.
 *
 * @param line		The output line.
 * @return true		Always returns true.
 */
bool
count_filter::exec
(
	const::std::string& line
)
{
	if ( line.find_first_of ( WHITESPACE ) != std::string::npos )
	{	// only count none-empty lines
		++m_count;
	}
	return true;
} // count_filter::exec ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print a summary line with the counter.
 *
 * @param out A reference to a @ref connection
 */
void
count_filter::final
(
	connection& out
	)
{
	out << unfiltered << "count: " << m_count << " lines" << crlf;
} // count_filter::final ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new limit filter::limit_filter object
 * 
 * The limit filter only prints \a limit lines of output.
 * 
 * Sets internal variable \a m_limit to \a limit and initialises
 * \a m_count to zero.
 * 
 * @param limit The number of lines to output.
 */
limit_filter::limit_filter
(
	size_t limit
) : m_limit { limit }, m_count { 0 }
{} // limit_filter::limit_filter ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

/**
 * @brief Count the number of lines.
 * 
 * Like @ref count_filter \a limit_filter only counts none-empty lines.
 * 
 * @param line   The output line.
 * @return true  While \a m_count <= \a m_limit
 * @return false If \a m_count > \a m_limit
 */
bool
limit_filter::exec
(
	const::std::string& line
)
{
	if ( line.find_first_of ( WHITESPACE ) != std::string::npos )
	{	// only count none-empty lines
		++m_count;
	}
	if ( m_count <= m_limit )
	{
		return true;
	}
	return false;
} // limit_filter::exec ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new last filter::last_filter object
 * 
 * The \a last filter only prints the last \a limit lines
 * of output.
 * 
 * Sets internal variable \a m_limit.
 * 
 * @param limit The number of lines to output.
 */
last_filter::last_filter
(
	size_t limit
) : m_limit { limit }
{} // last_filter::last_filter ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Put the output line into a buffer list.
 * 
 * Only hold \a m_limit lines of output in our internal buffer.
 * 
 * @param line   The output line.
 * @return false Always returns false.
 */
bool
last_filter::exec
(
	const::std::string& line
)
{
	m_lines.push_back ( line );
	if ( m_lines.size() > m_limit )
	{
		m_lines.pop_front ();
	}
	return false;
} // last_filter::exec ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print all lines of putput from out internal list.
 *
 * @param out A reference to a @ref connection, used for the output.
 */
void
last_filter::final
(
	connection& out
	)
{
	for ( auto a : m_lines )
	{
		out << unfiltered << a;
	}
	out << crlf;
} // last_filter::final ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new nomore filter::nomore_filter object
 * 
 * Disable the pager.
 * 
 * @param out A reference to a @ref connection, used for the output.
 */
nomore_filter::nomore_filter
(
	connection& out
)
{
	m_max_screen_lines = out.max_screen_lines;
	out.max_screen_lines = 0;
} // nomore_filter::nomore_filter ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

/**
 * @brief Do nothing, just return true.
 * 
 * @param line  The output line.
 * @return true Always.
 */
bool
nomore_filter::exec
(
	const::std::string& line
)
{
	return true;
} // nomore_filter::exec ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Re-nable the pager when all output was done.
 * 
 * @param out A reference to a @ref connection, used for the output.
 */
void
nomore_filter::final
(
	connection& out
	)
{
	out.max_screen_lines = m_max_screen_lines;
} // nomore_filter::final ()
//////////////////////////////////////////////////////////////////////

}; // namespace libcli

