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
 * @file        generic_commands.cxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2018
 */

#include "generic_commands.hxx"

namespace libcli
{

//////////////////////////////////////////////////////////////////////

template<>
RESULT
cmd_set<bool>::operator ()
(
	const std::string& name,
	const strvec& args,
	size_t& first_arg
)
{
	RESULT n { m_cli->need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	std::pair <RESULT, bool> r { m_cli->get_bool ( args[first_arg] ) };
	if ( r.first == RESULT::OK )
		m_variable = r.second;
	return r.first;
} // cmd_set<bool>::operator ()

//////////////////////////////////////////////////////////////////////

template <>
RESULT
cmd_set<std::string>::operator ()
(
	const std::string& name,
	const strvec& args,
	size_t& first_arg
)
{
	RESULT n { m_cli->need_n_args ( 1, args, first_arg ) };
	if ( RESULT::OK != n )
		return n;
	if ( m_cli->wants_help ( args[first_arg] ) )
	{
		m_cli->show_help ( "STRING", "set this value to STRING" );
		return RESULT::ERROR_ANY;
	}
	m_variable == args[first_arg];
	return RESULT::OK;
} // cmd_set<std::string>::operator ()

//////////////////////////////////////////////////////////////////////

} // namespace libcli

