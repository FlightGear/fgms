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
// derived from libcli by David Parrish (david@dparrish.com)
// Copyright (C) 2011  Oliver Schroeder
//
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "editor.hxx"

#if defined(_MSC_VER) || defined(__CYGWIN__)
// some windows quick fixes
#ifndef __CYGWIN__	
#define CTRL(a)  ( a & 037 )
#endif	
#ifdef __cplusplus
extern "C" {
#endif
	extern char* crypt ( const char* key, const char* salt );
#ifdef __cplusplus
}
#endif
#endif

namespace
{
namespace ASCII
{
const char BELL = 0x07;
const char BACKSPACE = 0x08;
const char LF = 0x0a;
const char CR = 0x0d;
}
}

namespace libcli
{

//////////////////////////////////////////////////////////////////////
editor::editor
(
	int fd
)
	: m_password_mode { false }
	, m_client { fd }
	, m_cursor { 0 }
{} // editor::editor ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
editor::~editor
()
{} // editor::~editor()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::set_prompt
(
	const std::string& prompt
)
{
	m_prompt = prompt;
} // editor::set_prompt ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::password_mode
(
	bool pw_mode
)
{
	m_password_mode = pw_mode;
} // editor::password_mode ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::set_regular_callback
(
	std::function <int ()> callback
)
{
	m_regular_callback.c_func = callback;
} // cli::regular ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::set_regular_callback
(
	std::function <int ( cli& )> callback
)
{
	m_regular_callback.member = callback;
} // cli::regular ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief clear the current line
 */
void
editor::clear_line
()
{
	m_cursor = 0;
	if ( m_password_mode )
	{	// no characters are written on screen
		return;
	}
	while ( m_cursor > 0 )
	{
		m_client.put_char ( ASCII::BACKSPACE );
		--m_cursor;
	}
	for ( size_t i = 0; i < m_input_line.length (); ++i )
	{
		m_client.put_char ( ' ' );
	}
	for ( size_t i = 0; i < m_input_line.length (); ++i )
	{
		m_client.put_char ( ASCII::BACKSPACE );
	}
	m_input_line.clear ();

} // editor::clear_line ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
unsigned char
editor::map_esc
()
{
	unsigned char c;
	m_client.read_char ( c );
	m_client.read_char ( c );
	/* remap to readline control codes */
	switch ( c )
	{
	case 'A': /* Up */
		c = CTRL ( 'P' );
		break;
	case 'B': /* Down */
		c = CTRL ( 'N' );
		break;
	case 'C': /* Right */
		c = CTRL ( 'F' );
		break;
	case 'D': /* Left */
		c = CTRL ( 'B' );
		break;
	default:
		c = 0;
	}
	return c;
} // editor::map_esc ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::handle_telnet_option ()
{
	unsigned char c;
	m_client.read_char ( c );
	switch ( c )
	{
	case 0xfb:	// WILL
	case 0xfc:	// WON'T
	case 0xfd:	// DO
	case 0xfe:	// DON'T
		m_client.read_char ( c );
		break;
	}
} // editor::handle_telnet_options ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::delete_backwards
(
	const unsigned char c
)
{
	if ( ( m_input_line.empty () ) || ( m_cursor == 0 ) )
	{
		m_client.put_char ( ASCII::BELL );
		return;
	}
	if ( m_password_mode )
	{
		return;
	}
	size_t len { m_input_line.length () };
	if ( c == CTRL ( 'W' ) ) /* word */
	{
		int nc = m_cursor - 1;
		while ( ( nc >= 0 ) && ( m_input_line[nc] == ' ' ) )
		{
			m_input_line.erase ( nc, 1 );
			//m_client << ASCII::BACKSPACE << " " << ASCII::BACKSPACE << commit;
			--m_cursor;
			--nc;
		}
		while ( ( nc >= 0 ) && ( m_input_line[nc] != ' ' ) )
		{
			m_input_line.erase ( nc, 1 );
			//m_client << ASCII::BACKSPACE << " " << ASCII::BACKSPACE << commit;
			--m_cursor;
			--nc;
		}
	}
	else /* char */
	{
		m_input_line.erase ( m_cursor - 1, 1 );
		//m_client << ASCII::BACKSPACE << " " << ASCII::BACKSPACE << commit;
		--m_cursor;
	}
	size_t cur { m_cursor };
	redraw_line ( false );
	//
	// clear to the end of line
	//
	size_t n;
	for ( n = m_cursor; n < len; ++n )
	{
		m_client.put_char ( ' ' );
	}
	for ( n = m_cursor; n < len; ++n )
	{
		m_client.put_char ( ASCII::BACKSPACE );
	}
	//
	// go to current cusor position
	//
	for ( n = m_input_line.length(); n > cur; --n )
	{
		m_client.put_char ( ASCII::BACKSPACE );
	}
} // editor::delete_backwards ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::show_prompt
()
{
	m_client << m_prompt << commit;
} // CLI::show_prompt ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::prompt_user ()
{
	show_prompt ();
	m_client << m_input_line << commit;
	if ( m_cursor < m_input_line.length () )
	{
		int n = m_input_line.length () - m_cursor;
		while ( n-- )
		{
			m_client.put_char ( '\b' );
		}
	}
	m_showprompt = false;
} // editor::prompt_user ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
bool
editor::try_logout
()
{
	if ( m_input_line.length () )
	{
		return false;
	}
	return true;
} // CLI::try_logout ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::redraw_line
(
	bool new_line
)
{
	if ( m_password_mode )
	{
		return;
	}
	if ( new_line )
	{
		m_client << crlf;
	}
	else
	{
		m_client.put_char ( ASCII::CR );
	}
	show_prompt ();
	m_client << m_input_line << commit;
	m_cursor = m_input_line.length ();
} // editor::redraw_line ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::clear_to_eol
()
{
	if ( m_cursor == m_input_line.length () )
	{
		return;
	}
	if ( !m_password_mode )
	{
		size_t c;
		for ( c = m_cursor; c < m_input_line.length (); ++c )
		{
			m_client.put_char ( ' ' );
		}
		for ( c = m_cursor; c < m_input_line.length (); ++c )
		{
			m_client.put_char ( ASCII::BACKSPACE );
		}
	}
	m_input_line.erase ( m_input_line.begin () + m_cursor, m_input_line.end () );
} // editor::clear_to_eol ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::cursor_left
()
{
	if ( m_cursor == 0 )
	{
		return;
	}
	if ( !m_password_mode )
	{
		m_client.put_char ( ASCII::BACKSPACE );
	}
	m_cursor--;
} // editor::m_cursor_left ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::cursor_right
()
{
	if ( m_cursor >= m_input_line.length () )
	{
		return;
	}
	if ( !m_password_mode )
	{
		m_client.put_char ( m_input_line[m_cursor] );
	}
	++m_cursor;
} // editor::m_cursor_right ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::jump_start_of_line
()
{
	if ( m_cursor )
	{
		if ( !m_password_mode )
		{
			m_client.put_char ( ASCII::CR );
			show_prompt ();
		}
		m_cursor = 0;
	}
} // editor::jump_start_of_line ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::jump_end_of_line
()
{
	if ( m_cursor < m_input_line.length () )
	{
		if ( !m_password_mode )
		{
			std::string t { m_input_line.begin () + m_cursor, m_input_line.end () };
			m_client << t << commit;
		}
		m_cursor = m_input_line.length ();
	}
} // editor::jump_end_of_line ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::append
(
	const unsigned char& c
)
{
	m_input_line += c;
	++m_cursor;
} // editor::append ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
editor::insert
(
	const unsigned char& c
)
{
	m_input_line.insert ( m_input_line.begin () + m_cursor, c );
	std::string t { m_input_line.begin () + m_cursor, m_input_line.end () };
	m_client << t << commit;
	for ( size_t i = 0; i < ( m_input_line.length () - m_cursor + 1 ); ++i )
	{
		m_client.put_char ( ASCII::BACKSPACE );
	}
	++m_cursor;
} // editor::insert ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
int
editor::get_line
(
	std::string& line
)
{
	m_input_line = line;
	m_cursor = m_input_line.length ();
	unsigned char c;
	m_showprompt = true;
	bool finished { false };
	m_client.put_char ( '\r' );	// go to start of line
	while ( !finished )
	{
		c = 0;
		if ( m_showprompt == true )
		{
			prompt_user ();
		}
		if ( m_client.get_input ( c ) == SOCKET_ERROR )
		{
			break;
		}
		switch ( c )
		{
		case 255:
			handle_telnet_option ();
			continue;
		case 27:		// handle ANSI arrows
			c = map_esc ();
			switch ( c )
			{
			case CTRL ( 'B' ):	// m_cursor left
				cursor_left ();
				continue;
			case CTRL ( 'F' ):	// m_cursor right
				cursor_right ();
				continue;
			case CTRL ( 'P' ):	// m_cursor Up
			case CTRL ( 'N' ):	// m_cursor Down
				finished = true;
				break;
			}
			continue;
		case '\n':
		case ASCII::CR:
			if ( !m_password_mode )
			{
				m_client << crlf;
			}
			m_showprompt = true;
			finished = true;
			break;
		case 0:
			if ( m_regular_callback.c_func != nullptr )
				m_regular_callback.c_func ();
			continue;
		case CTRL ( 'C' ):
			m_client.put_char ( ASCII::BELL );
			continue;
		case CTRL ( 'W' ):	// back word
		case CTRL ( 'H' ):	// backspace
		case 0x7f:		// delete
			delete_backwards ( c );
			continue;
		case CTRL ( 'L' ):	// redraw
			redraw_line ( true );
			continue;
		case CTRL ( 'U' ):	// clear line
			clear_line ();
			continue;
		case CTRL ( 'K' ):
			clear_to_eol ();
			continue;
		case CTRL ( 'D' ):	// EOT
			if ( try_logout () == true )
			{
				finished = true;
				break;
			}
			continue;
		case CTRL ( 'Z' ):	// leave config mode
			// FIXME: leave_config_mode ();
			continue;
		case CTRL ( 'I' ):	// TAB completion
			finished = true;
			break;
		case CTRL ( 'A' ):	// start of line
			jump_start_of_line ();
			continue;
		case CTRL ( 'E' ):	// end of line
			jump_end_of_line ();
			continue;
		default:		// normal character typed
			if ( m_cursor == m_input_line.length () )
			{
				append ( c );
				if ( c == '?' )
				{
					m_client << crlf;
					finished = true;
					break;
				}
			}
			else
			{	// Middle of text
				insert ( c );
			}
			if ( !m_password_mode )
			{
				m_client.put_char ( c );
			}
			continue;
		} // switch
		if ( m_input_line.empty () )
		{
			continue;
		}
		m_showprompt = true;
	}
	jump_end_of_line (); // make sure cursor is at end of line
	line = m_input_line;
	m_input_line.clear ();
	m_cursor = 0;
	return c;
} // editor::loop ()
//////////////////////////////////////////////////////////////////////

}; // namespace libcli

