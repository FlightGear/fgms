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
 * @file        cli_line.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        07/2017
 */

#include "cli_line.hxx"

namespace libcli
{

namespace TTY
{
        enum
        {
                BS = '\b',      // backspace
                CR = '\r',      // carriage return
                LF = '\n',      // line feed
                BELL = '\a',
        };
}

//////////////////////////////////////////////////////////////////////

line_editor::line_editor
(
        cli_client& client
) : m_client ( client ), m_in_history ( m_history.end() )
{
} // line_editor::line_editor

//////////////////////////////////////////////////////////////////////

void
line_editor::max_history
(
        size_t max
)
{
        while ( max > m_history.size() )
                m_history.pop_back ();
        m_max_history = max;
} // line_editor::max_history ()

//////////////////////////////////////////////////////////////////////

/**
 * Show history of commands.
 * @TODO: implement !# to reuse a history command (like in bash)
 */
void
line_editor::show_history
() const
{
        m_client.enable_filters ();
        try
        {
                m_client << cli_client::endl
                  << "Command history:" << cli_client::endl;
                size_t i { 0 };
                strlist::const_iterator h { m_history.begin () };
                while ( h != m_history.end() )
                {
                        m_client << std::setw(3) << i++
                          << " " << *h << cli_client::endl;
                        h++;
                }
        }
        catch ( pager_wants_quit ) { /* do nothing */ }
        m_client.disable_filters ();
} // line_editor::show_history ()

//////////////////////////////////////////////////////////////////////

/**
 * Add the current input buffer to the history
 * Only add if m_echo is true.
 */
void
line_editor::add_history
()
{
        if ( ( ! m_echo ) || ( m_line.size () == 0 ) )
                return;
        if ( m_max_history == m_history.size() )
                m_history.pop_back ();
        m_history.push_back ( m_line );
} // line_editor::add_history ()

//////////////////////////////////////////////////////////////////////

/**
 * Step one line up in the history and put the (historic) line into
 * the input buffer.
 */
void
line_editor::do_history_up
()
{
        if ( ! m_echo )
                return;
        if ( m_history.size () == 0 )
        {
                m_client.write_direct ( TTY::BELL );
                return;
        }
        if ( m_in_history == m_history.end() )
        {
                if ( m_line.size() > 0 )
                { // we were not browsing the history but have
                  // something on the command line => add to history
                        add_history ();
                        m_in_history--;
                }
        }
        if ( m_in_history != m_history.begin () )
        {
                clear_line ();
                m_in_history--;
                m_line   = *m_in_history;
                m_cursor = m_line.size ();
                m_client << m_line;
                m_client.flush ();
        }
} // line_editor::do_history_up ()

//////////////////////////////////////////////////////////////////////

/**
 * Step one line down in the history and put the (historic) line into
 * the input buffer.
 */
void
line_editor::do_history_down
()
{
        if ( ! m_echo )
                return;
        if ( m_in_history == m_history.end() )
        {
                m_client.write_direct ( TTY::BELL );
                return;
        }
        clear_line ();
        m_in_history++;
        if ( m_in_history == m_history.end() )
                return;
        m_line   = *m_in_history;
        m_cursor = m_line.size ();
        m_client << m_line;
        m_client.flush ();
} // line_editor::do_history_down ()

//////////////////////////////////////////////////////////////////////

/**
 * Write the current prompt to the user
 */
void
line_editor::prompt_user
() const
{
        if ( ! m_show_prompt )
                return;
        m_client << m_prompt;
        m_client.flush ();
} // line_editor::prompt ()

//////////////////////////////////////////////////////////////////////

/**
 * If we receive any telnet options, ignore them
 */
void
line_editor::handle_telnet_option
() const
{
        character c;
        m_client.read_char (c );
        switch ( c )
        {
        case 0xfb:      // WILL
        case 0xfc:      // WON'T
        case 0xfd:      // DO
        case 0xfe:      // DON'T
                m_client.read_char (c);
                break;
        }
} // line_editor::handle_telnet_option ()

//////////////////////////////////////////////////////////////////////

/**
 * Read a character of input from the user
 */
int
line_editor::get_input
(
        character& c
) const
{
        int ret { 0 };
        while ( ret == 0 )
        {
                ret = m_client.wait_for_input ( 1 );
                if ( ret == -1 )
                {       // error
                        perror ("read");
                        return ret;
                }
                if ( ret == 0 )
                {       // timeout every second
                        if ( m_callback && m_callback() != RESULT::OK )
                                break;
                        continue;
                }
                if ( m_client.read_char ( c ) < 0 )
                {
                        if ( errno == EINTR )
                                continue;
                }
                return ret;
        }
        return ret;
} // line_editor::get_input ()

//////////////////////////////////////////////////////////////////////

/**
 * Delete a character backwards.
 */
void
line_editor::delete_back_char
(
        line_editor::character c
)
{
        if ( ( m_cursor == 0 ) && ( c != KEY::DEL ) )
        {
                m_client.write_direct ( TTY::BELL );
                return;
        }
        if ( c == KEY::BACKSPACE )
                m_cursor--;
        if ( c == KEY::DEL )
                m_client.write_direct ( ' ' );
        m_line.erase (m_cursor, 1);
        if ( m_echo )
        {
                m_client.write_direct ( TTY::BS );
                print_line_from_cursor ();
                m_client.write_direct ( ' ' );
                m_client.write_direct ( TTY::BS );
                back_to_cursor ();
        }
} // line_editor::delete_back_char ()

//////////////////////////////////////////////////////////////////////

namespace
{

// little helper for line_editor::delete_back_word
// return true if 'c' is a delimiter
bool
is_delimiter
(
        line_editor::character c
)
{
        std::string delimiter = " |";
        if (delimiter.find (c) != std::string::npos)
                return true;
        return false;
} // is_delimiter

}

//////////////////////////////////////////////////////////////////////

/**
 * Delete a word backwards from the current cursor position
 */
void
line_editor::delete_back_word
()
{
        if ( m_cursor == 0 )
        {
                m_client.write_direct ( TTY::BELL );
                return;
        }
        size_t nc { 0 };
        while ( ( m_cursor > 0 ) && ( is_delimiter ( m_line[m_cursor - 1] ) ) )
        {       // delete leading spaces
                m_cursor--;
                m_line.erase ( m_cursor, 1 );
                if ( m_echo )
                        m_client.write_direct ( TTY::BS );
                nc++;
        }
        while ( ( m_cursor > 0 ) && ( !is_delimiter ( m_line[m_cursor - 1] ) ) )
        {       // delete the leading word
                m_cursor--;
                m_line.erase ( m_cursor, 1 );
                if ( m_echo )
                        m_client.write_direct ( TTY::BS );
                nc++;
        }
        if ( m_echo )
        {       // delete trailing characters
                print_line_from_cursor ();
                if ( m_echo )
                {
                        for ( size_t i = 0; i < nc; i++ )
                                m_client.write_direct (' ');
                        for ( size_t i = 0; i < nc; i++ )
                                m_client.write_direct ( TTY::BS );
                }
                back_to_cursor ();
        }
} // line_editor::delete_back_word ()

//////////////////////////////////////////////////////////////////////

/**
 * Replace the screen cursor to the current cursor position
 */
void
line_editor::back_to_cursor
() const
{
        if ( ! m_echo )
                return;
        for ( size_t i = m_cursor; i < m_line.size(); i++ )
                m_client.write_direct ( TTY::BS );
} // line_editor::back_to_cursor ()

//////////////////////////////////////////////////////////////////////

/**
 * Reprint the intput line from the current (screen) cursor position
 */
void
line_editor::print_line_from_cursor
() const
{
        if ( ! m_echo )
                return;
        const char* s = &m_line[m_cursor];
        m_client << s;
        m_client.flush ();
}

//////////////////////////////////////////////////////////////////////

/**
 * Redraw the complete input line
 */
void
line_editor::redraw_line
() const
{
        if ( ! m_echo )
                return;
        m_client.write_direct ( TTY::CR );
        m_client.write_direct ( TTY::LF );
        prompt_user ();
        m_client << m_line;
        m_client.flush ();
        back_to_cursor ();
} // line_editor::redraw_line ()

//////////////////////////////////////////////////////////////////////

/**
 * Clear the input line (internally and on screen)
 */
void
line_editor::clear_line
()
{
        if ( m_echo )
        {
                m_client.write_direct ( TTY::CR );
                prompt_user ();
                for ( size_t i = 0; i < m_line.size(); i++ )
                        m_client.write_direct ( ' ' );
                m_client.write_direct ( TTY::CR );
                prompt_user ();
        }
        m_line.clear ();
        m_cursor = 0;
} // line_editor::clear_line ()

//////////////////////////////////////////////////////////////////////

/**
 * Clear the input line from the current cursor position to the end
 * of the line.
 */
void
line_editor::clear_eol
()
{
        if ( m_cursor == m_line.size() )
                return;
        if ( m_echo )
        {
                for ( size_t i = m_cursor; i < m_line.size(); i++ )
                        m_client.write_direct ( ' ' );
                back_to_cursor ();
        }
        m_line.erase ( m_cursor );
} // line_editor::clear_eol ()

//////////////////////////////////////////////////////////////////////

void
line_editor::cursor_left
()
{
        if ( m_cursor == 0 )
                return;
        if ( m_echo )
                m_client.write_direct ( TTY::BS );
        m_cursor--;
} // line_editor::cursor_left ()

//////////////////////////////////////////////////////////////////////

void
line_editor::cursor_right
()
{
        if ( m_cursor >= m_line.size() )
                return;
        if ( m_echo )
                m_client.write_direct (m_line[m_cursor]);
        m_cursor++;
} // line_editor::cursor_right ()

//////////////////////////////////////////////////////////////////////

void
line_editor::jump_start_of_line
()
{
        if ( m_echo )
        {
                m_client.write_direct ( TTY::CR );
                prompt_user ();
        }
        m_cursor = 0;
} // line_editor::jump_start_of_line ()

//////////////////////////////////////////////////////////////////////

void
line_editor::jump_end_of_line
()
{
        if ( m_echo )
                print_line_from_cursor ();
        m_cursor = m_line.size ();
} // line_editor::jump_end_of_line ()

//////////////////////////////////////////////////////////////////////

void
line_editor::append
(
        const character c
)
{
        m_line.append ( 1, c );
        if ( m_echo )
                m_client.write_direct ( c );
        m_cursor++;
} // line_editor::append ()

//////////////////////////////////////////////////////////////////////

/**
 * Insert 'c' at the cursor position. If m_insertmode is 'true', 'c'
 * is inserted and the rest of the line is moved to the right.
 * If m_insertmode is 'false' 'c' overwrites the character at the cursor
 * position. In either case the cursor is advanced to the right.
 */
void
line_editor::insert
(
        const character c
)
{
        if ( m_insertmode )
                m_line.insert ( m_cursor, 1, c );
        else
                m_line[m_cursor] = c;
        print_line_from_cursor ();      // checks for m_echo
        m_cursor++;
        back_to_cursor ();
} // line_editor::insert ()

//////////////////////////////////////////////////////////////////////

/**
 * If we receive an ESC-character try as good as possible to map the
 * key to an internal command.
 */
line_editor::character
line_editor::map_esc
()
{
        character c;
        character code;
        m_client.read_char ( code );
        m_client.read_char ( c );
        switch ( c )
        {
        case 'A': // Up
                c = ctrl ( 'P' );
                break;
        case 'B': // Down
                c = ctrl ( 'N' );
                break;
        case 'C': // Right
                c = ctrl ( 'F' );
                break;
        case 'D': // Left
                c = ctrl ( 'B' );
                break;
        case 'H': // pos1
                c = ctrl ( 'A' );
                break;
        case 'F': // end
                c = ctrl ( 'E' );
                break;
        case '2': // insert
                m_client.read_char ( c ); // one additional character to ignore
                m_insertmode = (m_insertmode == false);
                break;
        case '3': // Del
                m_client.read_char ( c ); // one additional character to ignore
                c = KEY::DEL;
                break;
        default:
                c = 0;
        }
        return c;
} // line_editor::map_esc ()

//////////////////////////////////////////////////////////////////////

/**
 * User wants to logout. Only let him logout if the input line is empty.
 */
bool
line_editor::try_logout
()
{
        if ( m_line.size () )
                return false;
        m_line = "quit";
        m_client << "quit" << cli_client::endl;
        return true;
} // line_editor::try_logout

//////////////////////////////////////////////////////////////////////

/**
 * Replace the current word (backwards) with 'word'
 */
void
line_editor::replace_word
(
const std::string word
)
{
        delete_back_word ();
        for ( size_t i = 0; i < word.size(); i++ )
        {
                m_line.insert ( m_cursor, 1, word[i] );
                m_cursor++;
        }
}

//////////////////////////////////////////////////////////////////////

/**
 * Read an input line from the user.
 * @return '?' if the user typed a question mark
 * @return return KEY::LF: if the user pressed enter
 * @return ctrl('Z') or KEY::TAB:
 */
line_editor::character
line_editor::read_line
(
        bool clear_the_buffer
)
{
        bool show_prompt { true };
        character c;

        if ( clear_the_buffer )
        {
                m_cursor = 0;
                m_line.clear ();
        }
        else
        {
                if ( m_line[ m_line.size() - 1 ] == '?' )
                        delete_back_char ( KEY::BACKSPACE );
                redraw_line ();
                show_prompt = false;
        }
        while ( 1 )
        {
                if ( show_prompt )
                {
                        prompt_user ();
                        show_prompt = false;
                }
                if ( get_input (c) < 0 )
                        break;
                switch ( c )
                {
                case KEY::SPECIAL:
                        handle_telnet_option ();
                        break;
                case KEY::ESC:
                        c = map_esc ();
                        switch ( c )
                        {
                        case KEY::ARROW_LEFT:
                                cursor_left ();
                                continue;
                        case KEY::ARROW_RIGHT:
                                cursor_right ();
                                continue;
                        case KEY::ARROW_UP:
                                do_history_up ();
                                continue;
                        case KEY::ARROW_DOWN:
                                do_history_down ();
                                continue;
                        case KEY::DEL:
                                delete_back_char ( c );
                                continue;
                        case ctrl ( 'A' ):
                                jump_start_of_line ();
                                continue;
                        case ctrl ( 'E' ):
                                jump_end_of_line ();
                                continue;
                        }
                        continue;
                case KEY::LF:
                case KEY::CR:
                        if ( m_in_history != m_history.end () )
                                m_in_history = m_history.end ();
                        add_history ();
                        m_client << cli_client::endl;
                        return KEY::LF;
                case KEY::NONE:
                case KEY::BREAK:
                        m_client.write_direct ( TTY::BELL );
                        continue;
                case KEY::BACK_WORD:
                        delete_back_word ();
                        continue;
                case KEY::BACKSPACE:
                        delete_back_char ( c );
                        continue;
                case KEY::REDRAW:
                        redraw_line ();
                        continue;
                case KEY::CLEAR_LINE:
                        clear_line ();
                        continue;
                case KEY::CLEAR_EOL:
                        clear_eol ();
                        continue;
                case KEY::LOGOUT:
                        if ( try_logout () == true )
                                break;
                        continue;
                case ctrl ( 'A' ):
                        jump_start_of_line ();
                        continue;
                case ctrl ( 'E' ):
                        jump_end_of_line ();
                        continue;
                // return special keys not used by line editor
                case ctrl ( 'Z' ):
                case KEY::TAB:
                        return c;
                default:        // normal character
                        if ( ( c < 0x20 ) || ( c > 0x7e ) )
                        {       // none printable character
                                // silently ignore
                                continue;
                        }
                        if ( m_cursor == m_line.size () )
                        {
                                if ( c == '?' )
                                {
                                        append ( c );
                                        m_client << cli_client::endl;
                                        return c;
                                }
                                append ( c );
                        }
                        else
                        {
                                insert ( c );
                        }
                } // switch
        }
        return c;
} // line_editor::read_line ()

//////////////////////////////////////////////////////////////////////

} // namespace libcli

