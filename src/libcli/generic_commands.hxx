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
 * @file        generic_commands.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2018
 */


#ifndef _cli_pre_command_header
#define _cli_pre_command_header

#include "command.hxx"
#include "libcli.hxx"

namespace libcli
{

template <typename var_type>
class cmd_set : public command
{
public:
	cmd_set (
		const std::string& name,
		const cmd_priv level,
		const cmd_mode mode,
		var_type & variable,
		cli* cli,
		const std::string& help
	);
	virtual RESULT operator () (
		const std::string& name,
		const strvec& args,
		size_t& first_arg ) override;
	virtual bool has_callback () const override { return true; };
private:
	var_type & m_variable;
	cli* m_cli;
}; // class cmd_set

//////////////////////////////////////////////////////////////////////

template <typename var_type>
cmd_set<var_type>::cmd_set
(
	const std::string& name,
	const cmd_priv level,
	const cmd_mode mode,
	var_type & variable,
	cli* cli,
	const std::string& help
) : command ( name, level, mode, help ), m_variable { std::ref(variable) }, m_cli { cli }
{
}

//////////////////////////////////////////////////////////////////////

template <>
RESULT
cmd_set<std::string>::operator ()
(
	const std::string& name,
	const strvec& args,
	size_t& first_arg
);

//////////////////////////////////////////////////////////////////////

template <>
RESULT
cmd_set<bool>::operator ()
(
	const std::string& name,
	const strvec& args,
	size_t& first_arg
);

} // namespace libcli

#endif
