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
 * @file	fg_cli.hxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	2013
 */

//////////////////////////////////////////////////////////////////////
//
//      cisco like command line interface (cli)
//
//////////////////////////////////////////////////////////////////////

#ifndef FG_CLI_HEADER
#define FG_CLI_HEADER

#include <fgms.hxx>
#include <libcli.hxx>

using namespace libcli;
// using namespace std;

/**
 * @class FG_CLI 
 * @brief cisco like command line interface
 * 
 */
class FG_CLI : public CLI
{
public:
	FG_CLI ( FGMS*  fgms, int fd);
private:
	void setup ();
	//////////////////////////////////////////////////
	// general  commands
	//////////////////////////////////////////////////
	int cmd_show_stats	( UNUSED(char *command),
				  UNUSED(char *argv[]), UNUSED(int argc) );
	int cmd_show_settings	( UNUSED(char *command),
				  UNUSED(char *argv[]), UNUSED(int argc) );
	int cmd_show_version	( UNUSED(char *command),
				  UNUSED(char *argv[]), UNUSED(int argc) );
	int cmd_show_uptime	( UNUSED(char *command),
				  UNUSED(char *argv[]), UNUSED(int argc) );
	int cmd_fgms_die	( UNUSED(char *command),
				  UNUSED(char *argv[]), UNUSED(int argc) );
	//////////////////////////////////////////////////
	// show/modify whitelist
	//////////////////////////////////////////////////
	int cmd_whitelist_show   (  char *command, char *argv[], int argc );
	int cmd_whitelist_add    (  char *command, char *argv[], int argc );
	int cmd_whitelist_delete (  char *command, char *argv[], int argc );
	//////////////////////////////////////////////////
	// show/modify blacklist
	//////////////////////////////////////////////////
	int cmd_blacklist_show   (  char *command, char *argv[], int argc );
	int cmd_blacklist_add    (  char *command, char *argv[], int argc );
	int cmd_blacklist_delete (  char *command, char *argv[], int argc );
	//////////////////////////////////////////////////
	// show/modify list of crossfeeds
	//////////////////////////////////////////////////
	int cmd_crossfeed_show   (  char *command, char *argv[], int argc );
	int cmd_crossfeed_add    (  char *command, char *argv[], int argc );
	int cmd_crossfeed_delete (  char *command, char *argv[], int argc );
	//////////////////////////////////////////////////
	// show/modify list of relays
	//////////////////////////////////////////////////
	int cmd_relay_show   (  char *command, char *argv[], int argc );
	int cmd_relay_add    (  char *command, char *argv[], int argc );
	int cmd_relay_delete (  char *command, char *argv[], int argc );
	//////////////////////////////////////////////////
	// show list of players
	//////////////////////////////////////////////////
	int cmd_user_show   (  char *command, char *argv[], int argc );
	//////////////////////////////////////////////////
	// show status of tracker
	//////////////////////////////////////////////////
	int cmd_tracker_show   (  char *command, char *argv[], int argc );
	//////////////////////////////////////////////////
	// show/modify log
	//////////////////////////////////////////////////
	int cmd_show_log ( UNUSED(char *command),
			   UNUSED(char *argv[]), UNUSED(int argc) );
	// TODO: change the size of the logbuffer
private:
	FGMS* fgms;
	int cmd_NOT_IMPLEMENTED (  char *command, char *argv[], int argc );
};

#endif

