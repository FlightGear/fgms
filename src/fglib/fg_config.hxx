/**
 * @file fg_config.hxx
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

/**
 * @class FG_CONFIG 
 * @brief Simple config file parser
 * 
 * * The FG_CONFIG class, loads and parses, if it can,  the \ref fgms_conf file.
 * * An instance is created the ::ProcessConfig  function if  main.cxx 
 */

//////////////////////////////////////////////////////////////////////
//
//      Simple configfile parser for fgms
//
//////////////////////////////////////////////////////////////////////

#ifndef FG_CONFIG_HEADER
#define FG_CONFIG_HEADER

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <utility>

class FG_CONFIG
{
public:
	int   read ( const std::string &ConfigName );
	void  dump ();
	void  set_start ();
	int   next ();
	std::string get ( const std::string &VarName );
	std::string get_name ();
	std::string get_value ();
	std::string get_next ();
	int set_section ( const std::string &SecName );
	int sec_next ();
	std::string get_sec_next_var ();
	std::string get_sec_next_val ();
private:
	typedef std::pair<std::string,std::string>  var_value_t;
	typedef std::list<var_value_t>              var_list_t;
	int   parse_line ( const std::string & config_line );
	var_list_t            m_var_list;
	var_list_t::iterator  m_current_var;
	std::string           m_current_section;
};

#endif

