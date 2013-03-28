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
//      Simple configfile parser for fg_server
//
//////////////////////////////////////////////////////////////////////

#include <cctype>
#include "fg_config.hxx"

/**
 * @class FG_CONFIG 
 * @brief Simple config file parser
 * 
 * @todo Pete To make a links here to the config and explain a bit
 */


//////////////////////////////////////////////////////////////////////
//
//      read in the config file named 'ConfigName'
//
//////////////////////////////////////////////////////////////////////
int
FG_CONFIG::Read
(
	const std::string &ConfigName
)
{
	std::ifstream   ConfigFile;
	std::string     ConfigLine;
	int             LineNumber;

	ConfigFile.open (ConfigName.c_str ());
	if (!ConfigFile)
	{
		return (1);
	}
	LineNumber = 0;
	while (ConfigFile)
	{
		getline (ConfigFile, ConfigLine);
		LineNumber++;
		if (ParseLine (ConfigLine))
		{
			std::cout << "error in line " << LineNumber
				<< " in file " << ConfigName
				<< std::endl;
		}
	}
	ConfigFile.close ();
	m_CurrentVar = m_VarList.begin ();
	return (0);
} // FG_CONFIG::Read ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      find a variable with name 'VarName' in the internal
//      list and return its value
//
//////////////////////////////////////////////////////////////////////
std::string
FG_CONFIG::Get
(
	const std::string &VarName
)
{
	m_CurrentVar = m_VarList.begin();
	while (m_CurrentVar != m_VarList.end())
	{
		if (m_CurrentVar->first == VarName)
		{
			return (m_CurrentVar->second);
		}
		m_CurrentVar++;
	}
	return ("");
} // FG_SERVER::Get ()



//////////////////////////////////////////////////////////////////////
/**
 * @brief Set internal pointer to the first variable in list
 */
void
FG_CONFIG::SetStart
()
{
	m_CurrentVar = m_VarList.begin();
} // FG_CONFIG::SetStart ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set internal pointer to the next var.
 * @retval int 1 for success, else 0
 */
int
FG_CONFIG::Next
()
{
	if (m_CurrentVar == m_VarList.end())
	{
		return (0);
	}
	m_CurrentVar++;
	if (m_CurrentVar == m_VarList.end())
	{
		return (0);
	}
	return (1);
} // FG_CONFIG::Next()



//////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the complete name of the current variable.
 * @retval string The variable name, or empty string if not found.
 */
std::string
FG_CONFIG::GetName
()
{
	if (m_CurrentVar != m_VarList.end())
	{
		return (m_CurrentVar->first);
	}
	return ("");
} // FG_CONFIG::GetName ()



//////////////////////////////////////////////////////////////////////
/**
 * \brief Returns the current variable's value
 * \retval string The variable name, or empty string if not found.
 */
std::string
FG_CONFIG::GetValue
()
{
	if (m_CurrentVar != m_VarList.end())
	{
		return (m_CurrentVar->second);
	}
	return ("");
} // FG_CONFIG::GetValue ()



//////////////////////////////////////////////////////////////////////
/**
 * @brief Set internal pointer to the first variable of a section 
 * @todo Sort the variable list
 * @param SecName  String with section name
 * @retval int 1 on success, else 0
 */
int
FG_CONFIG::SetSection
(
	const std::string &SecName
)
{
	SetStart ();
	while (m_CurrentVar != m_VarList.end())
	{
		if (m_CurrentVar->first.compare(0,SecName.size(),SecName) == 0)
		{
			m_CurrentSection = SecName;
			return (1);
		}
		m_CurrentVar++;
	}
	return (0);
} // FG_CONFIG::SetSection ( const std::string &SecName )



//////////////////////////////////////////////////////////////////////
/**
 * @brief Set internal pointer to next variable in a section.
 * @retval int 1 on success, 0 else
 */
int
FG_CONFIG::SecNext
()
{
	if (!Next())
	{
		return (0);
	}
	if (m_CurrentVar->first.compare (0,
	    m_CurrentSection.size(), m_CurrentSection) != 0)
	{
		return (0);
	}
	return (1);
} // FG_CONFIG::GetSecNextVar ()


//////////////////////////////////////////////////////////////////////
/**
 * @brief Get the next variable name in current section
 * @retval string The variable name, or empty string if not found
 */
std::string
FG_CONFIG::GetSecNextVar
()
{
	if (!Next())
	{
		return ("");
	}
	if (m_CurrentVar->first.compare (0,
	    m_CurrentSection.size(), m_CurrentSection) != 0)
	{
		return ("");
	}
	return (m_CurrentVar->first);
} // FG_CONFIG::GetSecNextVar ()



//////////////////////////////////////////////////////////////////////
/**
 * @brief Return variable value of next variable in section
 * @retval string The value or empty string if not found
 */
std::string
FG_CONFIG::GetSecNextVal
()
{
	if (!Next())
	{
		return ("");
	}
	if (m_CurrentVar->first.compare (0,
	    m_CurrentSection.size(), m_CurrentSection) != 0)
	{
		return ("");
	}
	return (m_CurrentVar->second);
} // FG_CONFIG::GetSecNextVal ()



//////////////////////////////////////////////////////////////////////
/**
 * @brief Get the value of the next variable in list
 * @retval string The value or empty string if not found
 */
std::string
FG_CONFIG::GetNext
()
{
	if (!Next())
	{
		return ("");
	}
	return (m_CurrentVar->second);
} // FG_CONFIG::GetNext ()



//////////////////////////////////////////////////////////////////////
/**
 * @brief Just for debugging to cout
 */
void
FG_CONFIG::Dump
()
{
	m_CurrentVar = m_VarList.begin();
	std::cout << std::endl;
	std::cout << "dumping variables:" << std::endl;
	while (m_CurrentVar != m_VarList.end())
	{
		std::cout << " Var: '" << m_CurrentVar->first << "'";
		std::cout << " Val: '" << m_CurrentVar->second << "'";
		std::cout << std::endl;
		m_CurrentVar++;
	}
	std::cout << std::endl;
	std::cout << "done." << std::endl;
	std::cout << std::endl;
} // FG_SERVER::Dump ()



//////////////////////////////////////////////////////////////////////
/**
 * @brief Parse the given line, split it into name/value pairs
 *        and put in the internal list
 * @param ConfigLine The line to parse
 * @retval int
 */
int
FG_CONFIG::ParseLine
(
	const std::string &ConfigLine
)
{
	size_t          nVarNameStart, nVarNameEnd;
	size_t          nVarValueStart, nVarValueEnd;
	mT_VarValue     NewVar;

	if (ConfigLine.size() < 2) // ignore dos/windows 0x0d only
	{
		return (0);
	}
	if (ConfigLine[0] == '#')
	{
		return (0);
	}
	nVarNameStart = ConfigLine.find_first_not_of (" \t\n");
	nVarNameEnd = ConfigLine.find ('=');
	if ((nVarNameStart == std::string::npos)
	||  (nVarNameEnd == std::string::npos))
	{
		return (1);
	}
	nVarValueStart = ConfigLine.find_first_not_of (" \t\n", nVarNameEnd+1);
	nVarValueEnd = ConfigLine.size();
	while (isspace (ConfigLine[nVarNameEnd-1]))
	{
		nVarNameEnd--;
	}
	while (isspace (ConfigLine[nVarValueEnd-1]))
	{
		nVarValueEnd--;
	}
	NewVar.first = ConfigLine.substr
		(nVarNameStart, nVarNameEnd - nVarNameStart);
	NewVar.second = ConfigLine.substr
		(nVarValueStart, nVarValueEnd - nVarValueStart);
	m_VarList.push_back (NewVar);
	return (0);
} // FG_SERVER::ParseLine ()
//////////////////////////////////////////////////////////////////////

// vim: ts=2:sw=2:sts=0

