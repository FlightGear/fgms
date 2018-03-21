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
 * @file        command.cxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2011/2018
 */

#include <string.h>
#include "command.hxx"

namespace libcli
{

/**
 * Define a command without a callback function.
 * @param name Name of the command.
 * @param mode CLI_MODE in which the command is available
 * @param level PRIVLEVEL needed to run the command
 * @see CLI_MODE
 * @see PRIVLEVEL
 */
command::command
(
	const std::string& name,
	const cmd_priv level,
	const cmd_mode mode,
	const std::string& help
) : m_name { std::move ( name ) }, m_help { std::move ( help ) }
  , m_privilege { level }, m_mode { mode }
{
} // command::command ()

//////////////////////////////////////////////////////////////////////

/**
 * Define a command with a callback function.
 *
 * The callback function has the signature
 * @code
 * RESULT ( const std::string& name, const strvec& args, size_t first_arg )
 * @endcode
 * and can be a free function or a class member. C++ really sucks like hell
 * when using member methods for callbacks. You have to use std::bind to get
 * a valid pointer to the class member. For convenience its useful to define
 * a (local!) macro, like this:
 * @code
 * #define _ptr(X) (std::bind (& X, this, _1, _2, _3))
 *
 * command cmd (
 *      "do_somthing",
 *      _ptr ( someclass::cmd_do ),
 *      libcli::PRIVLEVEL::UNPRIVILEGED,
 *      libcli::CLI_MODE::ANY,
 *      "do something fancy );
 *
 * #undef _ptr
 * @endcode
 *
 * @param name Name of the command.
 * @param callback Callback function to run when the command is invoked
 * @param mode CLI_MODE in which the command is available
 * @param level PRIVLEVEL needed to run the command
 * @see CLI_MODE
 * @see PRIVLEVEL
 */
command::command
(
	const std::string& name,
	cli_callback_func callback,
	const cmd_priv level,
	const cmd_mode mode,
	const std::string& help
) : m_name { std::move ( name ) }, m_help { std::move ( help ) },
    m_cli_callback { callback }
{
	m_privilege     = level;
	m_mode          = mode;
}

//////////////////////////////////////////////////////////////////////

command::~command
()
{
}

//////////////////////////////////////////////////////////////////////

/**
 * Execute the callback function when this command is invoked
 */
RESULT
command::operator ()
(
	const std::string& name,
	const strvec& args,
	size_t& first_arg
)
{
	if ( m_cli_callback == nullptr )
		throw arg_error ( "command::exec: no cli_callback" );
	return m_cli_callback ( name, args, first_arg );
} // command::operator () ()

//////////////////////////////////////////////////////////////////////

/**
 * Compare command name with 'word'
 *
 * If compare_case is true, the comparision is case sensitive.
 * Compare only word.size() characters.
 *
 * @return true if m_name and word are equal
 * @return false if they are not euqal.
 */
bool
command::compare
(
	const std::string& word,
	const bool compare_case,
	const size_t len
)
{
	size_t l { len };
	if ( l == 0 )
		l = word.size ();
	if ( compare_case )
		return ( 0 == strncmp ( m_name.c_str(), word.c_str(), l ) );
	return ( 0 == strncasecmp ( m_name.c_str(), word.c_str(), l ) );
} // command::compare ()

//////////////////////////////////////////////////////////////////////

/**
 * Compare command name with 'word'.
 *
 * If compare_case is true, the comparision is case sensitive.
 * @return index of first distinguishing character
 */
int
command::compare_len
(
	const std::string& word,
	const bool compare_case
)
{
	size_t max { std::min ( m_name.size(), word.size() ) };
	size_t n { 0 };
	while ( n < max )
	{
		if ( compare_case )
		{
			if ( m_name[n] != word[n] )
				return ++n;
		}
		else
		{
			if ( toupper ( m_name[n] ) != toupper ( word[n] ) )
				return ++n;
		}
		++n;
	}
	return max;
} // command::compare_len ()

//////////////////////////////////////////////////////////////////////

} // namespace libcli

