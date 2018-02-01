/**
 * @file fg_config.cxx
 * @author Oliver Schroeder
 */
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
// Copyright (C) 2005  Oliver Schroeder
//

//////////////////////////////////////////////////////////////////////
//
//      Simple configfile parser for fgms
//
//////////////////////////////////////////////////////////////////////

#include <cctype>
#include "fg_config.hxx"

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

/** @brief  Read in the config file 
 * @param config_name - the file to read
 * @retval int 1 for error, 0 for success
 */
int
config::read
(
	const std::string &config_name
)
{
	std::ifstream   config_file;
	std::string     config_line;
	int             line_number;

	config_file.open (config_name.c_str ());
	if (!config_file)
	{
		return (1);
	}
	line_number = 0;
	while (config_file)
	{
		getline (config_file, config_line);
		line_number++;
		if (parse_line (config_line))
		{
			std::cout << "error in line " << line_number
				<< " in file " << config_name
				<< std::endl;
		}
	}
	config_file.close ();
	m_current_var = m_var_list.begin ();
	return (0);
} // config::read ()

//////////////////////////////////////////////////////////////////////

/** @brief Find a variable in the internal list and return its value
 * @param VarName - the variable to find
 * @retval string - with contents of variable, or blank string if not found
*/
std::string
config::get
(
	const std::string & var_name
)
{
	m_current_var = m_var_list.begin();
	while (m_current_var != m_var_list.end())
	{
		if (m_current_var->first == var_name)
		{
			return (m_current_var->second);
		}
		m_current_var++;
	}
	return ("");
} // FGMS::get ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Set internal pointer to the first variable in list
 */
void
config::set_start
()
{
	m_current_var = m_var_list.begin();
} // config::SetStart ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Set internal pointer to the next var.
 * @retval int 1 for success, else 0
 */
int
config::next
()
{
	if (m_current_var == m_var_list.end())
	{
		return (0);
	}
	m_current_var++;
	if (m_current_var == m_var_list.end())
	{
		return (0);
	}
	return (1);
} // config::next()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Returns the complete name of the current variable.
 * @retval string The variable name, or empty string if not found.
 */
std::string
config::get_name
()
{
	if (m_current_var != m_var_list.end())
	{
		return (m_current_var->first);
	}
	return ("");
} // config::get_name ()

//////////////////////////////////////////////////////////////////////

/**
 * \brief Returns the current variable's value
 * \retval string The variable name, or empty string if not found.
 */
std::string
config::get_value
()
{
	if (m_current_var != m_var_list.end())
	{
		return (m_current_var->second);
	}
	return ("");
} // config::get_value ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Set internal pointer to the first variable of a section 
 * @todo Sort the variable list
 * @param SecName  String with section name
 * @retval int 1 on success, else 0
 */
int
config::set_section
(
	const std::string &sec_name
)
{
	set_start ();
	while (m_current_var != m_var_list.end())
	{
		if (m_current_var->first.compare(0,
		    sec_name.size(),sec_name) == 0)
		{
			m_current_section = sec_name;
			return (1);
		}
		m_current_var++;
	}
	return (0);
} // config::set_section ( const std::string &SecName )

//////////////////////////////////////////////////////////////////////

/**
 * @brief Set internal pointer to next variable in a section.
 * @retval int 1 on success, 0 else
 */
int
config::sec_next
()
{
	if ( ! next() )
	{
		return (0);
	}
	if (m_current_var->first.compare (0,
	    m_current_section.size(), m_current_section) != 0)
	{
		return (0);
	}
	return (1);
} // config::GetSecNextVar ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Get the next variable name in current section
 * @retval string The variable name, or empty string if not found
 */
std::string
config::get_sec_next_var
()
{
	if ( ! next() )
	{
		return ("");
	}
	if (m_current_var->first.compare (0,
	    m_current_section.size(), m_current_section) != 0)
	{
		return ("");
	}
	return (m_current_var->first);
} // config::get_sec_next_var ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Return variable value of next variable in section
 * @retval string The value or empty string if not found
 */
std::string
config::get_sec_next_val
()
{
	if ( ! next() )
	{
		return ("");
	}
	if (m_current_var->first.compare (0,
	    m_current_section.size(), m_current_section) != 0)
	{
		return ("");
	}
	return (m_current_var->second);
} // config::get_sec_next_val ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Get the value of the next variable in list
 * @retval string The value or empty string if not found
 */
std::string
config::get_next
()
{
	if ( ! next() )
	{
		return ("");
	}
	return (m_current_var->second);
} // config::get_next ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Just for debugging to cout
 */
void
config::dump
()
{
	m_current_var = m_var_list.begin();
	std::cout << std::endl;
	std::cout << "dumping variables:" << std::endl;
	while (m_current_var != m_var_list.end())
	{
		std::cout << " Var: '" << m_current_var->first << "'";
		std::cout << " Val: '" << m_current_var->second << "'";
		std::cout << std::endl;
		m_current_var++;
	}
	std::cout << std::endl;
	std::cout << "done." << std::endl;
	std::cout << std::endl;
} // FGMS::dump ()

//////////////////////////////////////////////////////////////////////

/**
 * @brief Parse the given line, split it into name/value pairs
 *        and put in the internal list
 * @param config_line The line to parse
 * @retval int
 */
int
config::parse_line
(
	const std::string & config_line
)
{
	size_t          var_name_start, var_name_end;
	size_t          var_value_start, var_value_end;
	var_value_t     new_var;

	if (config_line.size() < 2) // ignore dos/windows 0x0d only
	{
		return (0);
	}
	if (config_line[0] == '#')
	{
		return (0);
	}
	var_name_start = config_line.find_first_not_of (" \t\n");
	var_name_end = config_line.find ('=');
	if ((var_name_start == std::string::npos)
	||  (var_name_end == std::string::npos))
	{
		return (1);
	}
	var_value_start = config_line.find_first_not_of (
	  " \t\n", var_name_end+1 );
	var_value_end = config_line.size();
	while (isspace (config_line[var_name_end-1]))
	{
		var_name_end--;
	}
	while (isspace (config_line[var_value_end-1]))
	{
		var_value_end--;
	}
	new_var.first = config_line.substr
		(var_name_start, var_name_end - var_name_start);
	new_var.second = config_line.substr
		(var_value_start, var_value_end - var_value_start);
	m_var_list.push_back (new_var);
	return (0);
} // config::ParseLine ()

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

