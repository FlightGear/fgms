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
 * @file        command.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2011/2018
 */


#ifndef _cli_command_header
#define _cli_command_header

#include <functional>
#include <memory>
#include <vector>
#include "common.hxx"

namespace libcli
{

class cli;
class command;

/**
 * Define a cli command.
 */
class command
{
public:
        using cli_callback_func = std::function <
          RESULT ( const std::string& name, const strvec& args,
          size_t& first_arg ) >;
        using command_p = std::shared_ptr<command>;
        using cmdlist   = std::vector<command_p>;
        friend class cli;

        command () = delete;
        command ( const command& ) = delete;
        command (
                const std::string & name,
                int level,
                int mode,
                const std::string & help
        );
        command (
                const std::string & name,
                cli_callback_func callback,
                int level,
                int mode,
                const std::string & help
        );
        ~command ();
        RESULT exec ( const std::string& name, const strvec& args,
                size_t& first_arg );
        inline bool has_callback () const;
        inline bool has_children () const;
        inline const std::string& name () const;
        inline const std::string& help () const;
        inline int privilege () const;
        inline int mode () const;
        inline size_t unique_len () const;
        inline void unique_len (size_t len );
        bool compare ( const std::string& word,
                bool compare_case = false, const size_t len = 0 );
        int  compare_len ( const std::string& word,
                bool compare_case = false );
private:
        /// the command name
        std::string m_name;
        /// a brief desciption what the command does
        std::string m_help;
        /// which privilege is needed to execute the command
        /// @see PRIVLEVEL
        int m_privilege;
        /// in which mode the command is available
        /// @see CLI_MODE
        int m_mode;
        size_t m_unique_len = 1;
        /// every command can have an abitrary number of subcommands
        cmdlist m_children;
        /// every command can have a callback funtion.
        cli_callback_func m_cli_callback = nullptr;
}; // class command

//////////////////////////////////////////////////////////////////////

const std::string&
command::name
() const
{
        return m_name;
} // command::name ()

//////////////////////////////////////////////////////////////////////

const std::string&
command::help
() const
{
        return m_help;
} // command::help ()

//////////////////////////////////////////////////////////////////////

int
command::privilege
() const
{
        return m_privilege;
} // command::privilege ()

//////////////////////////////////////////////////////////////////////

int
command::mode
() const
{
        return m_mode;
} // command::mode ()

//////////////////////////////////////////////////////////////////////

size_t
command::unique_len
() const
{
        return m_unique_len;
} // command::unique_len()

//////////////////////////////////////////////////////////////////////

void
command::unique_len
(
        const size_t len
)
{
         m_unique_len = len;
} // command::unique_len(len)

//////////////////////////////////////////////////////////////////////

/**
 * @return true if this command has a callback function.
 */
bool
command::has_callback
() const
{
        return ( m_cli_callback != nullptr );
} // command::has_callback ()

//////////////////////////////////////////////////////////////////////

/**
 * @return true if this command has subcommands.
 */
bool
command::has_children
() const
{
        return m_children.size() > 0;
} // command::has_children ()

//////////////////////////////////////////////////////////////////////


} // namespace libcli

#endif
