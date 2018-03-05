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
 * @file        cli_client.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2011/2018
 */


#ifndef _cli_client_header
#define _cli_client_header

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <unistd.h>
#ifndef _MSC_VER
        #include <termios.h>
#endif
#include "common.hxx"
#include "filter.hxx"

namespace libcli
{

/**
 * This class ist throw as an exception by the pager
 * when the user presses 'q' to exit from a longer
 * listing.
 */
class pager_wants_quit
{};

/**
 * A stream like class for cli input/output operations
 */
class cli_client
{
public:
        cli_client () = delete;
        cli_client ( int fd );
        int  wait_for_input ( const int seconds ) const;
        inline int  read_char ( unsigned char& c ) const;
        inline void write_direct ( const char& c ) const;
        inline void write_direct ( const std::string& str ) const;
        inline void flush ();

        inline void   register_filter ( filter* f );
        inline size_t max_screen_lines () const;
        inline void   max_screen_lines ( size_t max );
        inline void   enable_filters ();
        inline void   disable_filters ();

        template <class T> cli_client& operator << ( T v );
        inline cli_client& operator << ( cli_client& (*f) (cli_client&) );
        static cli_client& endl ( cli_client& out );
        static inline cli_client& flush ( cli_client& );

        ~cli_client ();
private:
        enum class PRINT_MODE
        {
                PLAIN     = 0x00,
                FILTERED  = 0x01
        };
        /// Number of lines written to the client.
        size_t m_lines_out = 0;
        /// Maximum number of lines to write to the client
        /// before presenting the pager ('more')
        /// disabled by default
        size_t m_max_screen_lines = 0;
        /// Current output mode. If set to PRINT_MODE::FILTERED
        /// all lines are processed by active filters and the
        /// internal pager.
        PRINT_MODE m_print_mode = PRINT_MODE::FILTERED;
        /// The fildescriptor for reading input from the client.
        /// If using a socket, the socket must be completly 
        /// initialised.
        int m_read_fd;
        /// The fildescriptor for writing output to the client.
        /// If using a socket, the socket must be completly 
        /// initialised.
        int m_write_fd;
        /// The current list of active filters. If \ref m_print_mode
        /// is set to PRINT_MODE::FILTERED all output is filterd by these
        /// filters.
        filterlist m_filters;
        /// All output is bufferd in m_output (except when using
        /// write_direct()). Output is written to the client only when
        /// cli_client::endl is processed.
        std::ostringstream m_output;
        #ifndef _MSC_VER
        struct termios OldModes;
        #endif
        static bool pager ( cli_client& );
        static bool check_pager ( cli_client& );
        static cli_client& apply_filters ( cli_client& );
};

//////////////////////////////////////////////////////////////////////

class align_left
{
public:
        align_left () = delete;
        align_left ( const char width );
        align_left ( const char spacer, const char width );
        cli_client &operator () (cli_client & out) const;
private:
        char spacer;
        char width;
}; // class align_left

cli_client& operator << ( cli_client& out, align_left align );

//////////////////////////////////////////////////////////////////////

/**
 * Read a single byte from the client.
 * @return >0: read was successfult
 * @return -1: An error occured and errno is set appropriately.
 * @return =0: end of file (connection was closed)
 * For further details see read(2)
 */
int
cli_client::read_char
(
        unsigned char& c
) const
{
        return read ( m_read_fd, &c, 1 );
} // cli_client::read_char ()

//////////////////////////////////////////////////////////////////////

/**
 * Write a single byte directly to the client. No filters are applied.
 */
void
cli_client::write_direct
(
        const char& c
) const
{
        write ( m_write_fd, &c, 1 );
} // cli_client::write_direct ()

//////////////////////////////////////////////////////////////////////

/**
 * Write a string directly to the client. No filters are applied.
 */

void
cli_client::write_direct
(
        const std::string& str
) const
{
        write ( m_write_fd, str.c_str(), str.size() );
} // cli_client::write_direct ()

//////////////////////////////////////////////////////////////////////

cli_client&
cli_client::operator <<
(
        cli_client& (*f) (cli_client&)
)
{
        return ((*f)(*this));
} // operator << ( cli_client& (*f) )

//////////////////////////////////////////////////////////////////////

/**
 * Write m_output to the client.
 */
void
cli_client::flush
()
{
        write_direct ( m_output.str() );
        m_output.str ("");
} // cli_client::flush ()

//////////////////////////////////////////////////////////////////////

cli_client&
cli_client::flush
(
        cli_client& out
)
{
        out.flush ();
        return out;
} // flush ( cli_client &)

//////////////////////////////////////////////////////////////////////

/**
 * Register a \ref filter
 * you can register as many filters as you want. All filters are
 * applied in the order they are registerd.
 * Filters are registered automatically by the \ref cli when entered
 * on the command line.
 */
void
cli_client::register_filter
(
        filter* f
)
{
        m_filters.push_back ( filter_p ( f ) );
} // cli_client::add_filter ()

//////////////////////////////////////////////////////////////////////

/**
 * @return The number of lines written to the client before prompting
 * the user with the pager.
 */
size_t
cli_client::max_screen_lines
() const
{
        return m_max_screen_lines;
} // cli_client::max_screen_lines ()

//////////////////////////////////////////////////////////////////////

/**
 * Set the number of lines written to the client before prompting
 * the user with the pager.
 */
void
cli_client::max_screen_lines
(
        size_t max
)
{
        m_max_screen_lines = max;
} // cli_client::max_screen_lines ( max )

//////////////////////////////////////////////////////////////////////

/**
 * Set m_print_mode to PRINT_MODE::FILTERED
 */
void
cli_client::enable_filters
()
{
        m_lines_out  = 0;
        m_print_mode = PRINT_MODE::FILTERED;
} // cli_client::max_screen_lines ()

//////////////////////////////////////////////////////////////////////

/**
 * Set m_print_mode to PRINT_MODE::PLAIN
 */
void
cli_client::disable_filters
()
{
        m_print_mode = PRINT_MODE::PLAIN;
        m_filters.clear ();
} // cli_client::max_screen_lines ()

//////////////////////////////////////////////////////////////////////

/**
 * Write something into the buffer.
 */
template <class T>
cli_client&
cli_client::operator << ( T v )
{
        m_output << v;
        return *this;
} // operator << ( class T );

//////////////////////////////////////////////////////////////////////

} // namespace libcli

#endif
