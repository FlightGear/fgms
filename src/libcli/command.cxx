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
// Copyright (C) 2011-2021 Oliver Schroeder
//

#include "command.hxx"

namespace libcli
{

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new command_callback
 * 
 * Constructs an empty callback.
 * 
 * @code {.cpp}
 * command_callback callme {}
 * @endcode
 * 
 */
command_callback::command_callback
()
: member { nullptr }, m_instance { nullptr }
{} // command_callback::command_callback ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new command_callback for a c-function
 * 
 * @param callback 	pointer to the c-function
 * 
 * Constructs a callback to a c-function or a static member method.
 * @code {.cpp}
 * int internal_help (const std::string& name, const libcli::tokens& args )
 * {
 * 		// ...
 * }
 * 
 * command_callback callme { &internal_help }
 * @endcode
 * 
 */
command_callback::command_callback
(
	command_callback_c callback
	)
: c_func { callback }, m_instance { nullptr }
{} // command_callback::command_callback ( command_callback_c )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new command callback::command callback object
 * 
 * @param callback pointer to a member method
 * @param instance pointer to the instance of a @ref cli derived class
 * 
 * @code {.cpp}
 * class any : public cli
 * {
 * 		int internal_help (const std::string& name, const libcli::tokens& args )
 * 		{
 * 			// ...
 * 		}
 * 		// ...
 * };
 * 
 * using callback = int ( cli::* ) ( const std::string& command, const libcli::tokens& args );
 * command_callback callme { static_cast< callback > ( &any::internal_help ) }
 * @endcode
 * 
 */
command_callback::command_callback
(
	command_callback_cpp callback,
	cli* instance
)
: member { callback }, m_instance { instance }
{} // command_callback::command_callback ( command_callback_cpp )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

/**
 * @brief execute (call) the callback function
 * 
 * @param name name of the command
 * @param args arguments to the command
 * @return int a return value, normally a @ref libcli::RESULT code
 * 
 * The name is just a reference, so the called function knows with what name
 * it was called.
 * @code {.cpp}
 * int internal_help (const std::string& name, const libcli::tokens& args )
 * {
 * 		// ...
 * }
 * 
 * command_callback callme { &internal_help }
 * callme ( "help", args );
 * @endcode
 * @warning never execute a callback to a member method when the instance was already destroyed
 */
RESULT
command_callback::operator ()
(
	const std::string& name,
	const libcli::tokens& args
	)
{
	if ( m_instance == nullptr )
	{
		return c_func ( name, args );
	}
	return ( ( *m_instance ).*( member ) ) ( name, args );
} // command_callback::operator () ( name, args )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new command object without a callback
 *
 * @param name	The name of the command
 * @param level	The privilege level needed to execute this command, normally a @ref libcli::PRIVLEVEL
 * @param mode	The mode of this command, normally a @ref libcli::MODE
 * @param help	A brief description of this command
 */
command::command
(
	const std::string	name,
	int					level,
	int					mode,
	const std::string	help
)
	: m_name { name }
	, m_help { help }
	, m_privilege { level }
	, m_mode { mode }
	, m_unique_len { name.length () }
	, m_callback {}
{
	DEBUG d ( __FUNCTION__, __FILE__, __LINE__ );
	if ( name.length () == 0 )
	{
		throw arg_error ( "C1: bad argument" );
	}
} // command::command ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new command with a c-callback
 *
 * @param name		The name of the command
 * @param callback	Pointer to the c-function
 * @param level		The privilege level needed to execute this command, normally a @ref libcli::PRIVLEVEL
 * @param mode		The mode of this command, normally a @ref libcli::MODE
 * @param help		A brief description of this command
 */
command::command
(
	const std::string	name,
	command_callback_c	callback,
	int					level,
	int					mode,
	const std::string	help
)
	: m_name { name }
	, m_help { help }
	, m_privilege { level }
	, m_mode { mode }
	, m_unique_len { name.length () }
	, m_callback { callback }
{
	DEBUG d ( __FUNCTION__, __FILE__, __LINE__ );
	if ( name.length () == 0 )
	{
		throw arg_error ( "C2: bad argument" );
	}
} // command::command ( c_callback )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Construct a new command with a cpp-callback
 *
 * @param instance	Pointer to the instance of the member method callback
 * @param name		The name of the command
 * @param callback	Pointer to the member method
 * @param level		The privilege level needed to execute this command, normally a @ref libcli::PRIVLEVEL
 * @param mode		The mode of this command, normally a @ref libcli::MODE
 * @param help		A brief description of this command
 */
command::command
(
	cli* instance,
	const std::string		name,
	command_callback_cpp 	callback,
	int						level,
	int						mode,
	const std::string		help
)
	: m_name { name }
	, m_help { help }
	, m_privilege { level }
	, m_mode { mode }
	, m_unique_len { name.length () }
	, m_callback { callback, instance }
{
	DEBUG d ( __FUNCTION__, __FILE__, __LINE__ );
	if ( name.length () == 0 )
	{
		throw arg_error ( "C3: bad argument" );
	}
} // command::command ( cpp_callback )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief execute this command
 *
 * @param command 	The name of this command
 * @param args 		Argument list
 * @return int 		Return code
 */
RESULT
command::operator ()
(
	const std::string& name,
	const libcli::tokens& args
)
{
	DEBUG d ( __FUNCTION__, __FILE__, __LINE__ );
	return m_callback ( name, args );
} // command::exec ( name, args )
//////////////////////////////////////////////////////////////////////

}; // namespace libcli
