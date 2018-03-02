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

#ifndef _cli_line_header
#define _cli_line_header

#include <string>
#include <list>
#include "cli_client.hxx"

namespace libcli
{

// Don't use some obscure #defines from stdlib.
// Use a local definition
constexpr int ctrl ( int c ) { return c & 0x1f; }

// using names instead of cryptic characters
namespace KEY
{
        enum
        {
                NONE,
                LF              = '\n',
                CR              = '\r',
                ESC             = 0x1b,
                BREAK           = ctrl ( 'C' ),
                LOGOUT          = ctrl ( 'D' ),
                ARROW_LEFT      = ctrl ( 'B' ),
                ARROW_RIGHT     = ctrl ( 'F' ),
                ARROW_UP        = ctrl ( 'P' ),
                ARROW_DOWN      = ctrl ( 'N' ),
                TAB             = ctrl ( 'I' ),
                BACKSPACE       = 0x7f,
                BACK_WORD       = ctrl ( 'W' ),
                DEL             = 0x08,
                REDRAW          = ctrl ( 'L' ),
                CLEAR_LINE      = ctrl ( 'U' ),
                CLEAR_EOL       = ctrl ( 'K' ),
                SPECIAL         = 0xff
        };
}

/** a simple line editor
 *
 * @TODO: can not handle UTF-X characters
 * @TODO: movement: jump_back_word, jump_fwd_word
 */
class line_editor
{
public:
        using std_callback = std::function<RESULT ( void )>;
        using character    = unsigned char;

        line_editor () = delete;
        line_editor ( cli_client& client );
        void show_history () const;
        character read_line ( bool clear_the_buffer = true );
        inline std::string& get_line () const;
        inline void do_echo ( bool echo );
        inline void do_prompt ( bool prompt );
        inline void set_callback ( std_callback callback );
        inline char char_before_cursor () const;
        inline size_t cursor_pos () const;
        inline size_t size () const;
        inline void set_prompt ( const std::string & prompt );
        void replace_word ( const std::string word );
private:
        void    prompt_user () const;
        void    handle_telnet_option () const;
        int     get_input ( character& c ) const;
        void    delete_back_char ( character c );
        void    delete_back_word ();
        void    redraw_line () const;
        void    clear_line ();
        void    clear_eol ();
        void    cursor_left ();
        void    cursor_right ();
        void    jump_start_of_line ();
        void    jump_end_of_line ();
        void    back_to_cursor () const;
        void    print_line_from_cursor () const ;
        void    append ( const character c );
        void    insert ( const character c );
        void    do_history_up ();
        void    do_history_down ();
        void    max_history ( size_t max );
        void    add_history ();
        bool    try_logout ();
        character map_esc ();

        /// Current prompt of the input line
        std::string m_prompt = ">";
        /// Current cursor position
        size_t m_cursor = 0;
        /// If 'true' insert input characters at the current cursor
        /// position. If 'false' then overwrite characters at the 
        /// cursor position. Defaults to 'true'
        bool m_insertmode = true;
        /// Echo typed in characters back to the client?
        /// Defaults to 'true'
        /// @see do_echo
        bool m_echo = true;
        /// Present a prompt to the user?
        /// Defaults to 'true'
        /// @see do_prompt
        bool m_show_prompt = true;
        /// Maximum number of commands stored in the internal history.
        /// Defaults to 256
        size_t m_max_history = 256;
        /// The client conection
        cli_client& m_client;
        /// The buffer for the input line
        std::string m_line;
        /// Internal buffer of input lines
        strlist m_history;
        std_callback m_callback = nullptr;
        /// are we moving in the history?
        strlist::iterator m_in_history;
}; // class line_editor

//////////////////////////////////////////////////////////////////////

/**
 * @return the current line buffer
 */
std::string&
line_editor::get_line
() const
{
        return (std::string&) * ( & m_line );
}

//////////////////////////////////////////////////////////////////////

/**
 * If set to 'true', all typed characters are written back to the client.
 * Should generally set to 'true', except for reading passwords.
 */
void
line_editor::do_echo
(
        bool echo
)
{
        m_echo = echo;
} // line_editor::do_echo ()

//////////////////////////////////////////////////////////////////////

/**
 * Present a prompt to the user?
 * If set to 'true' every input line is prefixed with the prompt.
 * @see set_prompt
 */
void
line_editor::do_prompt
(
        bool prompt
)
{
        m_show_prompt = prompt;
} // line_editor::do_prompt ()

//////////////////////////////////////////////////////////////////////

/**
 * Set a callback function which is called whenever the user provides
 * no input. This can be used eg. for automatic logout etc.
 * The callback function has the signature libcli::RESULT func_name ( void )
 */
void
line_editor::set_callback
(
        std_callback callback
)
{
        m_callback = callback;
} // line_editor::set_callback ()

//////////////////////////////////////////////////////////////////////

/**
 * @return the character directly in from of the cursor.
 * @return 0 if the cursor is at the start of the line.
 */
char
line_editor::char_before_cursor
() const
{
        if ( m_cursor == 0 )
        {
                return 0;
        }
        return m_line[m_cursor - 1];
}

//////////////////////////////////////////////////////////////////////

/**
 * @return the current cursor position.
 */
size_t
line_editor::cursor_pos
() const
{
        return m_cursor;
} // line_editor::cursor_pos ()

//////////////////////////////////////////////////////////////////////

/**
 * @return the size (length) of the current input line.
 */
size_t
line_editor::size
() const
{
        return m_line.size();
} // line_editor::cursor_pos ()

//////////////////////////////////////////////////////////////////////

/**
 * Set the prompt.
 * Every input line is prefixed with the prompt.
 */
void
line_editor::set_prompt
(
        const std::string & prompt
)
{
        m_prompt = prompt;
} // line_editor::set_prompt ()

//////////////////////////////////////////////////////////////////////

} // namespace libcli

#endif

