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
  int   Read ( const std::string &ConfigName );
  void  Dump ();
  void  SetStart ();
  int   Next ();
  std::string Get ( const std::string &VarName );
  std::string GetName ();
  std::string GetValue ();
  std::string GetNext ();
  int SetSection ( const std::string &SecName );
  int SecNext ();
  std::string GetSecNextVar ();
  std::string GetSecNextVal ();
private:
  typedef std::pair<std::string,std::string>  mT_VarValue;
  typedef std::list<mT_VarValue>              mT_VarList;
  int   ParseLine ( const std::string &ConfigLine );
  mT_VarList            m_VarList;
  mT_VarList::iterator  m_CurrentVar;
  std::string           m_CurrentSection;
};

#endif

// vim: ts=2:sw=2:sts=0

