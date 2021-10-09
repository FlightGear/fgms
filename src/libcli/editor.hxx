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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, U$
//
// derived from libcli by David Parrish (david@dparrish.com)
// Copyright (C) 2011  Oliver Schroeder
//

#ifndef libcli_editor_H
#define libcli_editor_H

#include <string>
#include <functional>
#ifndef _MSC_VER
#include <termios.h>
#endif
#include "cli_client.hxx"
#include "common.hxx"

namespace libcli
{

class cli;

class editor
{
public:
	editor ( int fd );
	~editor ();
	int		get_line ( std::string& line );
	void	set_prompt ( const std::string& prompt );
	void	password_mode ( bool pw_mode );
	void	set_regular_callback ( std::function <int ()> callback );
	void	set_regular_callback ( std::function <int ( cli& )> callback );

protected:
	void	clear_line ( int cursor );
	void	show_prompt ();
	void	prompt_user ();
	bool	try_logout ();
	void	handle_telnet_option ();
	void	delete_backwards ( const unsigned char c );
	void	redraw_line ( bool new_line );
	void	clear_line ();
	void	clear_to_eol ();
	void	cursor_left ();
	void	cursor_right ();
	void	jump_start_of_line ();
	void	jump_end_of_line ();
	void	append ( const unsigned char& c );
	void	insert ( const unsigned char& c );
	unsigned char	map_esc ();
	// Variables
	bool		m_showprompt;	///< present the prompt?
	std::string	m_prompt;		///< the prompt string
	bool		m_password_mode;///< don't do some action when typing a password
	client		m_client;		///< the client connection
	size_t		m_cursor;		///< cursor position within input line
	std::string	m_input_line;	///< content of current input line
	struct regular_callback_t
	{
		union
		{
			std::function <int ( cli& )>	member;		// pointer to a class member
			std::function <int ()>			c_func;	// pointer to c- or staic function
		};
		regular_callback_t () : member { nullptr } {};
		~regular_callback_t () {};
	} m_regular_callback;
#ifndef _MSC_VER
	struct termios  OldModes;
#endif
};

}; // namespace libcli

#endif
