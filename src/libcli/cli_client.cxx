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
 * @file        cli_client.cxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2011/2018
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <string.h>

#ifdef __CYGWIN__
#include <sys/select.h>
#include <_timeval.h>
#endif

#ifdef _MSC_VER
        #include <conio.h> // for _kbhit(), _getch
        #define getchar _getch

        bool wait_for_key ( unsigned timeout_ms = 0 )
        {
                return WaitForSingleObject(
                        GetStdHandle( STD_INPUT_HANDLE ),
                        timeout_ms
                ) == WAIT_OBJECT_0;
        }
#endif

#include <fglib/fg_log.hxx>
#include "cli_client.hxx"

namespace libcli
{

//////////////////////////////////////////////////////////////////////

/** Construct a client connection.
 *
 * @param fd 0 for stdin/stdout. >0 for socket connections.
 */
cli_client::cli_client
(
        int fd
)
{
        m_read_fd = fd;
        if (fd == fileno ( stdin ))
        {       // setup terminal attributes
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
                        (void) tcsetattr ( fileno (stdin), TCSANOW, &NewModes );
                #else
                        // not required, but does not seem to harm
                        AllocConsole();
                        // needed to use WaitForSingleObject(GetStdHandle
                        //   ( STD_INPUT_HANDLE ),timeout_ms);
                        freopen ( "conin$", "r", stdin );
                        // only required IFF console output redirected
                        freopen ( "conout$", "w", stdout );
                        // this break the redirection, so cli can be
                        // always seen
                        freopen ( "conout$", "w", stderr );
                #endif
                m_write_fd = fileno ( stdout );
        }
        else
        {       // setup telnet session
                const char* negotiate =
                        "\xFF\xFB\x03"  // WILL SUPPRESS GO AHEAD OPTION
                        "\xFF\xFB\x01"  // WILL ECHO
                        "\xFF\xFD\x03"  // DO SUPPRESS GO AHEAD OPTION
                        "\xFF\xFD\x01"; // DO ECHO
                m_write_fd = fd;
                write_direct ( negotiate );
        }
} // cli_client::cli_client ()

//////////////////////////////////////////////////////////////////////

/**
 * Close the client connection.
 */
cli_client::~cli_client
()
{
        if ( m_read_fd == fileno ( stdin ) )
        {       // restore terminal attributes
                #ifndef _MSC_VER
                ( void ) tcsetattr ( fileno ( stdin ), TCSANOW, &OldModes );
                #endif
        }
        else
        {       // close socket
                #if defined(UL_CYGWIN) || !defined (UL_WIN32)
                        ::close ( m_read_fd );
                #else
                        ::closesocket ( m_read_fd );
                #endif
        }
} // cli_client::~cli_client ()

//////////////////////////////////////////////////////////////////////

/**
 * Wait until input is available from the client.
 *
 * return 0:  timeout
 * return >0: input available
 * return <0: error
 */
int
cli_client::wait_for_input
(
        const int seconds
) const
{
#ifdef _MSC_VER
        if ( m_read_fd == fileno ( stdin ) )
        {
                return wait_for_key (seconds * 1000);
        }
#endif
        struct timeval tv ;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        fd_set r;
        FD_ZERO (&r);
        FD_SET (m_read_fd, &r);
        if ( seconds == 0 )
                return select (m_read_fd+1, &r, 0, 0, 0 );
        return select (m_read_fd+1, &r, 0, 0, &tv);
} // cli_client::wait_for_input ()

//////////////////////////////////////////////////////////////////////

/**
 * @return true if user pressed 'q'
 * @return false on any other key
 */
bool
cli_client::pager
(
        cli_client& out
)
{
        out << "--- more ---" << flush;
        unsigned char c { ' ' };
        bool done { false };
        do
        {
                out.wait_for_input ( 0 ); // wait until key pressed
                out.read_char (c );
                if ( c == 'q' )
                {
                        out.m_lines_out = 0;
                        out.write_direct ( "\r\n" );
                        return true;
                }
                if ( c == ' ' )
                {
                        out.m_lines_out = 0;
                        out.write_direct ( "\r\n" );
                        done = true;
                }
                if ( ( c == '\r' ) || ( c == '\n' ) )
                {       // show next line and reprompt for more
                        done = true;
                        //out << "\r";
                        out.write_direct ( '\r' );
                }
        } while ( ! done );
        return false;
} // cli_client::pager ()

//////////////////////////////////////////////////////////////////////

/**
 * @return true if user pressed 'q'
 * @return false on any other key
 */
bool
cli_client::check_pager
(
        cli_client& out
)
{
        if ( out.m_max_screen_lines == 0 ) // pager not active
                return false;
        if ( out.m_lines_out > out.m_max_screen_lines )
                return pager ( out );
        return false;
} // cli_client::check_pager ()

//////////////////////////////////////////////////////////////////////

/**
 * Pass m_output to all active filters. Write output only if all
 * filters returned 'true'. Also, check if the pager is active.
 * @throw pager_wants_quit if the user wants to see no more output
 * (and pressed 'q').
 */
cli_client&
cli_client::apply_filters
(
        cli_client& out
)
{
        bool print { true };

        for ( auto f : out.m_filters )
        {
                print = (*f) ( out.m_output.str() );
                if ( ! print )
                        break;
        }
        if (print)
        {
                flush ( out );
                out.m_lines_out++;
        }
        out.m_output.str ("");
        out.m_print_mode = PRINT_MODE::FILTERED;
        if ( check_pager ( out ) )
                throw pager_wants_quit ();
        return out;
} // cli_client& cli_client::apply_filters()

//////////////////////////////////////////////////////////////////////

/**
 * Write a 'newline' to the client. While on a host system 'newline'
 * can be anything from '\r','\n','\n\r' and '\r\n', telnet defines
 * this to be '\r\n', regardless of the host definition.
 *
 * This method looks trivial but really is black magic.
 * If filters are active, all output is passed to them prior to actually
 * writing to the client. Addionally the pager is queried. If the pager is
 * active, the user is prompted with a '--- more ---' prompt. If the user
 * presses 'q' at this point, an exception is thrown.
 * @throw pager_wants_quit
 */
cli_client&
cli_client::endl
(
        cli_client& out
)
{
        out.m_output << "\r\n";
        if ( out.m_print_mode == PRINT_MODE::FILTERED )
                apply_filters ( out );
        else
                flush ( out );
        return out;
} // endl()

//////////////////////////////////////////////////////////////////////

align_left::align_left
(
        const char width
)
: spacer {' '}, width {width}
{}

//////////////////////////////////////////////////////////////////////

align_left::align_left
(
        const char spacer,
        const char width
)
: spacer {spacer}, width {width}
{}

//////////////////////////////////////////////////////////////////////

cli_client&
align_left::operator ()
(
        cli_client & out
) const
{
        out << std::left
                << std::setfill ( spacer )
                << std::setw ( width );
        return out;
}

//////////////////////////////////////////////////////////////////////

cli_client&
operator <<
(
        cli_client& out,
        align_left align
)
{
        return align ( out );
} // cli_client& align_left()

//////////////////////////////////////////////////////////////////////

/**
 * Write a bool into the buffer.
 */
template <>
cli_client&
cli_client::operator << ( bool b )
{
        m_output << ( b ? "true" : "false" );
        return *this;
} // cli_client& operator << ( bool );

//////////////////////////////////////////////////////////////////////

} // namespace libcli
