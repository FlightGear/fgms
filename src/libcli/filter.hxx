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
 * @file        filter.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        03/2018
 */


#ifndef _cli_filter_header
#define _cli_filter_header

#include <string>
#include <functional>
#include <memory>
#include "common.hxx"

namespace libcli
{

class cli_client;       // for count_filter
class filter;

using filter_p   = std::shared_ptr < filter >;
using filterlist = std::vector < filter_p >;

//////////////////////////////////////////////////////////////////////

/**
 * Pure virtual base class for output filters, from which all real
 * filter must be derived.
 * Filters are special internal commands. All output to clients is
 * filtered on request.
 */
class filter
{
public:
        virtual bool operator () ( const std::string & line ) = 0;
        virtual ~filter () {};
protected:
        filter () {}; // disable direct instantiation
};

//////////////////////////////////////////////////////////////////////

/**
 * 'include' / 'exclude' filters
 *
 * invoke with 'cmd | include WORD'. All lines containing 'WORD'
 * are written to the client. If 'exclude' is used, all lines
 * not containing 'WORD' are written to the client.
 *
 */
class match_filter : public filter
{
public:
        match_filter ( const std::string & match, bool invert );
        bool operator () ( const std::string & line ) override;
private:
        std::string m_match_this;
        bool  m_invert;
}; // class match_filter

//////////////////////////////////////////////////////////////////////

/**
 * 'begin' filter
 *
 * invoke with 'cmd | begin WORD'. Output starts when a line contains
 * 'WORD'.
 */
class begin_filter : public filter
{
public:
        begin_filter ( const std::string & match );
        bool operator () ( const std::string & line ) override;
private:
        std::string m_match_this;
        bool  m_have_match = false;
}; // class begin_filter

//////////////////////////////////////////////////////////////////////

/**
 * 'between' filter
 *
 * Invoke with 'cmd | between START END'
 * Output starts with the line containg 'START' and ends with the line
 * containing 'END'.
 *
 */
class between_filter : public filter
{
public:
        between_filter ( const std::string & start_match, const std::string & end_match );
        bool operator () ( const std::string & line ) override;
private:
        std::string m_start_match;
        std::string m_end_match;
        bool  m_have_match = false;
}; // class between_filter

//////////////////////////////////////////////////////////////////////

/**
 * 'count' filter
 *
 * Invoke with 
 * @code
 * cmd | count
 * @endcode
 * No output is written to the client except the number of lines which
 * would have been written without this filter.<br>
 * Filters can be chained, which is especially useful with this filter:
 * @code
 * cmd | begin START | count
 * @endcode
 */
class count_filter : public filter
{
public:
        count_filter ( cli_client* client );
        virtual ~count_filter ();
        bool operator () ( const std::string & line ) override;
private:
        size_t  m_counter = 0;
        cli_client* m_client;
}; // class count_filter

//////////////////////////////////////////////////////////////////////

} // namespace libcli

#endif
