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
 * @file        common.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2011/2018
 */

#ifndef _cli_common_header
#define _cli_common_header

#include <exception>
#include <iostream>
#include <vector>
#include <string>
#include <list>

namespace libcli
{

using strvec  = std::vector<std::string>;
using strlist = std::list<std::string>;

/** Return values for most libcli methods
 *
 */
enum class RESULT
{
	OK,		///< everything went good
	ERROR_ANY,	///< an error occurred, the method printed a message
	INVALID_ARG,	///< passed arguments do match requirements
	TOO_MANY_ARGS,	///< passed more arguments then required
	MISSING_ARG,	///< passed less arguments then required
	INVALID_COMMAND,///< the command name is invalid
	SHOW_HELP	///< syntax was ok, but please show a help message
};

/**
 * libcli supports privilege levels. Every command is associated with
 * a privilege level and is only available if the user has at least
 * this level.
 * With this scheme, there could be several users with different
 * access levels. Currently only UNPRIVILEGED and PRIVILEGED are defined.
 * For possible enhancements this type is defined as an integer (not an enum
 * class) so the levels can be extended from outside of libcli.
 */
namespace PRIVLEVEL
{
enum
{
	DENIED,
	UNPRIVILEGED,
	PRIVILEGED,
	EXTENSION
};
}

/// alias for easier understanding
using cmd_priv = int;

/**
 * libcli supports different modes of operation. Currently two modes are
 * defined: EXEC and CONFIG. When a user connects to the cli, he generally
 * starts off in EXEC mode. The 'configure' command switches into CONFIG
 * mode.
 * ANY means the command is available in any (i.e. all) modes.
 * For possible enhancements this type is defined as an integer (not an enum
 * class) so the levels can be extended from outside of libcli.
 */
namespace CLI_MODE
{
enum
{
	ANY,
	EXEC,
	CONFIG,
	EXTENSION       ///< use this for your own modes
};
}

/// alias for easier understanding
using cmd_mode = int;

class arg_error : public std::exception
{
public:
	arg_error ( const std::string& r ) :reason {r} {};
	virtual const char* what() const throw()
	{
		return reason.c_str();
	}
	const std::string reason;
};

} // namespace libcli

#endif
