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

#ifndef CLI_FILTER_H
#define CLI_FILTER_H

#include <string>
#include <list>

namespace libcli
{

class connection;

//////////////////////////////////////////////////////////////////////

/**
 * @brief Base class of a filter. All filters must be derived from
 *        this class.
 */
class filter
{
public:
	/// The actual filter action
	virtual bool exec ( const std::string& line ) = 0;
	virtual ~filter () {};
	/// 'final' is called after the command completes.
	virtual void final ( connection& ) {};
}; // filter
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// 'grab' or 'exclude'
class match_filter : public filter
{
public:
	enum class MATCH_MODE
	{
		NORM,	///< match m_match_me
		INVERT, ///< do not match m_match_me
	};
	match_filter ( const MATCH_MODE mode, const std::string& match_me );
	virtual bool exec ( const std::string& line ) override;
private:
	MATCH_MODE	m_mode;
	std::string	m_match_me;
}; // match_filter
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
class begin_filter : public filter
{
	bool		m_matched;
	std::string	m_match_me;
public:
	begin_filter ( const std::string& match_me );
	virtual bool exec ( const std::string& line ) override;
}; // begin_filter
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
class between_filter : public filter
{
	bool		m_matched;
	std::string	m_start_with;
	std::string	m_end_with;
public:
	between_filter ( const std::string& start_with, const std::string& end_with );
	virtual bool exec ( const std::string& line ) override;
}; // between_filter
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
class count_filter : public filter
{
	size_t m_count;
public:
	count_filter ();
	bool exec ( const std::string& line ) override;
	virtual void final ( connection& out ) override;
}; // count_filter
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
class limit_filter : public filter
{
	size_t m_limit;
	size_t m_count;
public:
	limit_filter ( size_t limit );
	bool exec ( const std::string& line ) override;
}; // limit_filter
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
class last_filter : public filter
{
	size_t m_limit;
	size_t m_count;
	std::list<std::string> m_lines;
public:
	last_filter ( size_t limit );
	bool exec ( const std::string& line ) override;
	virtual void final ( connection& out ) override;
}; // last_filter
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
class nomore_filter : public filter
{
	size_t m_max_screen_lines;
public:
	nomore_filter ( connection& out );
	bool exec ( const std::string& line ) override;
	virtual void final ( connection& out ) override;
}; // nomore_filter
//////////////////////////////////////////////////////////////////////

/* additional possible filters
  append               Append output text to file
  display              Show additional kinds of information
  save                 Save output text to file
  tee                  Write to standard output and file
  trim                 Trim specified number of columns from start of line
*/

}; // namespace libcli

#endif
