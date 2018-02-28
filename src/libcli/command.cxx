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

#include <command.hxx>

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
        const std::string & name,
        int level,
        int mode,
        const std::string & help
) : m_name {name}, m_help{help}
{
        m_privilege     = level;
        m_mode          = mode;
}

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
        const std::string & name,
        cli_callback_func callback,
        int level,
        int mode,
        const std::string &     help
) : m_name {name}, m_help{help}, m_cli_callback{callback}
{
        m_privilege     = level;
        m_mode          = mode;
}

command::~command
()
{
}

/**
 * Execute the callback function when this command is invoked
 */
RESULT
command::exec
(
        const std::string & name,
        const strvec & args,
        size_t first_arg
)
{
        if ( m_cli_callback == nullptr )
        {
                throw arg_error ( "command::exec: no cli_callback" );
        }
        return m_cli_callback ( name, args, first_arg );
}

} // namespace libcli

