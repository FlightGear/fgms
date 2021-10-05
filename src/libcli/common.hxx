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


#ifndef CLI_COMMON_H
#define CLI_COMMON_H

#include <exception>
#include <iostream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include "debug.hxx"

namespace libcli
{

/**
 * @brief What the cli interprets as space characters
 */
constexpr const char* WHITESPACE { " \t\n\v\f\r" };

/**
 * @brief Return codes used throughout libcli
 *
 * You can define your own return codes starting with RESULT::MAX
 * @code {.cpp}
 * enum MY_RESULT
 * {
 * 		MY_FIRST = libcli::RESULT::DEFAULT_MAX,
 * 		MY_SECOND
 * 		// ...
 * }
 * @endcode
 */
enum RESULT
{
	OK,				///< everything went well
	ERROR_ANY,		///< some error occured
	ERROR_ARG,		///< there was an error parsing the arguments
	TOO_MANY_ARGS,	///< command got too many args
	END_OF_ARGS,	///< command wants no more args
	MISSING_ARG,	///< too few arguments
	MISSING_QUOTE,  ///< missing ' or "
	UNKNOWN_COMMAND,///< command is unknown
	UNKNOWN_FILTER, ///< filter is unknown
	INVALID_ARG,    ///< the argument is invalid
	OTHER,			///< execution went OK, but mark a special event
	QUIT,			///< user wants to quit
	DEFAULT_MAX		///< marker for maximum defined @a RESULT
};

/**
 * @brief Predefined privilege levels
 *
 * The concept of privilege levels allows to have users with different access to commands,
 * depending on the privilege level they have. The authentication method can set a
 * privilege level for the user using @ref cli::set_privilege.
 *
 * @ref libcli::commands can be connected to a @a PRIVLEVEL. The @ref cli is running
 * in @a PRIVLEVEL @ref UNPRIVILEGED by default but can change the level using
 * @ref cli::set_privilege. The user can only execute commands which are below or equal
 * to his level, that is @ref cli.m_privilege >= @ref command.m_privilege
 *
 * The privilege levels are defined in their own namespace, so it is easy to define
 * your own without confilicting with other definitions.
 * @code {.cpp}
 * namespace libcli { namespace PRIVLEVEL {
 * 		enum
 * 		{
 * 		MY_FIRST = UNPRIVILEGED + 1,
 * 		MY_SECOND
 * 		// ...
 * 		}
 * }}
 * @endcode
 *
 */
namespace PRIVLEVEL
{
enum
{
	UNPRIVILEGED = 0,	///< no privilege level needed
	PRIVILEGED = 15  	///< user needs to have this level to execute a command
};
}

/**
 * @brief Prefined modes
 *
 * The concept of modes allows to provide commands only if the cli is in the
 * corresponding mode. So you can have a set of commands which are accessible in
 * all modes (@a MODE @ref ANY) and another set of commands which are only
 * accessible in your selfdefined mode (eg. a 'config' mode for configure commands).
 *
 * The predefined modes are defined in their own namespace, so it is easy to define
 * your own without confilicting with other definitions.
 * @code {.cpp}
 * namespace cli { namespace MODE
 * 		enum
 * 		{
 * 		MY_FIRST = DEFAULT_MAX + 1,
 * 		MY_SECOND
 * 		// ...
 * 		}
 * }}
 * @endcode
 *
 * Since the pure cli itself is agnostic to modes you might implement, we only
 * define @ref ANY (meaning all modes) and @ref STANDARD, the standar operation
 * mode of the cli.
 */
namespace MODE
{
enum
{
	ANY,		///< All modes
	STANDARD,	///< The standard opration mode
	CONFIGURE,	///< configure mode
	DEFAULT_MAX ///< Marker to define your own modes.
};
}

using tokens = std::vector<std::string>;

class arg_error : public std::exception
{
public:
	arg_error ( const char* r ) { reason = r; };
	virtual const char* what () const throw( )
	{
		return reason;
	}
	const char* reason;
};

class mem_error : public std::exception
{
public:
	virtual const char* what () const throw( )
	{
		return "could not allocate memory";
	}
};

#ifdef __GNUC__
# define UNUSED(d) d __attribute__ ((unused))
#else
# define UNUSED(d) d
#endif

}; // namespace libcli

#endif
