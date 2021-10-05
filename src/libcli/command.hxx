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
// along with this program; if not, see http://www.gnu.org/licenses/
//
// derived from libcli by David Parrish (david@dparrish.com)
// Copyright (C) 2011-2021  Oliver Schroeder
//

#ifndef CLI_COMMAND_H
#define CLI_COMMAND_H

#include <string>
#include <vector>
#include <functional>
#include "common.hxx"

namespace libcli
{

class cli;

using command_callback_c = RESULT ( * ) ( const std::string& name, const libcli::tokens& args );
using command_callback_cpp = RESULT ( cli::* ) ( const std::string& name, const libcli::tokens& args );
/**
 * @brief a callback function of a command
 * 
 * @note C++ is a pile of bullshit regarding callback functions. I want to store
 * a pointer to a callback function and I do not care if it is a member of
 * some class or a static c function.
 * Using a std::function<> does not help. It would just make this class considerably
 * more complex.
 * You can try to find a solution for this. Let me know if you have a
 * better approach.
 */
struct command_callback
{
	union
	{
		command_callback_cpp	member;	///< pointer to a class member
		command_callback_c		c_func;	///< pointer to c- or static function
	};
	command_callback ();
	command_callback ( command_callback_c callback );
	command_callback ( command_callback_cpp callback, cli* instance );
	RESULT operator () ( const std::string& name, const libcli::tokens& args );
	cli* m_instance;	///< pointer to a @ref cli derived class (when using member methods)
};

/**
 * @brief a command of the cli
 *
 * A @a command connects a name with a @ref command_callback
 */
class command
{
public:
	using cmd_list = std::vector<command>;
	friend class cli;

	command (
		const std::string		name,
		int						level,
		int						mode,
		const std::string		help
	);
	command (
		const std::string		name,
		command_callback_c		callback,
		int						level,
		int						mode,
		const std::string		help
	);
	command (
		cli* instance,
		const std::string		name,
		command_callback_cpp	callback,
		int						level,
		int						mode,
		const std::string		help
	);
	RESULT	operator () ( const std::string& name, const libcli::tokens& args );

private:
	std::string		m_name;			///< The name of this command
	std::string		m_help;			///< A brief description what this command does
	int				m_privilege;	///< Privilege level needed to execute this command
	int				m_mode;			///< In which mode this command can be executed
	size_t			m_unique_len;	///< In a list of commands, this is the unique number of characters to distinguish this command from others
	cmd_list		m_children;		///< A list of child commands
	command_callback m_callback;	///< A command_callback for this command (might be empty)
	command () = delete;
}; // class command

}; // namespace libcli

#endif
