/**
 * @file fg_cli.hxx
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
 // Copyright (C) 2013  Oliver Schroeder
 //

 /**
  * @class FG_CLI
  * @brief cisco like command line interface
  *
  */

  //////////////////////////////////////////////////////////////////////
  //
  //      cisco like command line interface (cli)
  //
  //////////////////////////////////////////////////////////////////////

#ifndef FG_CLI_HEADER
#define FG_CLI_HEADER

#include <fg_server.hxx>
#include <libcli.hxx>

using namespace libcli;

class FG_CLI : public cli
{
public:
	FG_CLI ( FG_SERVER* fgms, int fd );
private:
	void setup ();
	//////////////////////////////////////////////////
	// general  commands
	//////////////////////////////////////////////////
	RESULT cmd_show_stats ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_show_settings ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_show_version ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_show_uptime ( const std::string& command,  const libcli::tokens& args );
	RESULT cmd_fgms_die ( const std::string& command, const libcli::tokens& args );
	//////////////////////////////////////////////////
	// show/modify whitelist
	//////////////////////////////////////////////////
	RESULT cmd_whitelist_show ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_whitelist_add ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_whitelist_delete ( const std::string& command, const libcli::tokens& args );
	//////////////////////////////////////////////////
	// show/modify blacklist
	//////////////////////////////////////////////////
	RESULT cmd_blacklist_show ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_blacklist_add ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_blacklist_delete ( const std::string& command, const libcli::tokens& args );
	//////////////////////////////////////////////////
	// show/modify list of crossfeeds
	//////////////////////////////////////////////////
	RESULT cmd_crossfeed_show ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_crossfeed_add ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_crossfeed_delete ( const std::string& command, const libcli::tokens& args );
	//////////////////////////////////////////////////
	// show/modify list of relays
	//////////////////////////////////////////////////
	RESULT cmd_relay_show ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_relay_add ( const std::string& command, const libcli::tokens& args );
	RESULT cmd_relay_delete ( const std::string& command, const libcli::tokens& args );
	//////////////////////////////////////////////////
	// show list of players
	//////////////////////////////////////////////////
	RESULT cmd_user_show ( const std::string& command, const libcli::tokens& args );
	//////////////////////////////////////////////////
	// show status of tracker
	//////////////////////////////////////////////////
	RESULT cmd_tracker_show ( const std::string& command, const libcli::tokens& args );
private:
	FG_SERVER* fgms;
	RESULT cmd_NOT_IMPLEMENTED ( const std::string& command, const libcli::tokens& args );
};

#endif

