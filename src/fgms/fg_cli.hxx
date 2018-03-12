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
 * @file        fg_cli.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2013
 */

//////////////////////////////////////////////////////////////////////
//
//      cisco like command line interface (cli)
//
//////////////////////////////////////////////////////////////////////

#ifndef fgcli_HEADER
#define fgcli_HEADER

#include <libcli/libcli.hxx>
#include "fgms.hxx"

namespace fgmp
{

/**
 * @class fgcli
 * @brief cisco like command line interface
 *
 */
class fgcli : public libcli::cli
{
public:
	fgcli ( fgmp::fgms* fgms, int fd );
private:
	void setup ();
	using strvec = libcli::strvec;
	using RESULT = libcli::RESULT;

	//////////////////////////////////////////////////
	// show commands
	//////////////////////////////////////////////////
	RESULT show_daemon ( const std::string& command,
			     const strvec& args, size_t first_arg );
	RESULT show_bind_addr ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT show_admin_user ( const std::string& command,
				 const strvec& args, size_t first_arg );
	RESULT show_admin_pass ( const std::string& command,
				 const strvec& args, size_t first_arg );
	RESULT show_admin_enable ( const std::string& command,
				   const strvec& args, size_t first_arg );
	RESULT show_data_port ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT show_query_port ( const std::string& command,
				 const strvec& args, size_t first_arg );
	RESULT show_admin_port ( const std::string& command,
				 const strvec& args, size_t first_arg );
	RESULT show_admin_cli ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT show_logfile_name ( const std::string& command,
				   const strvec& args, size_t first_arg );
	RESULT show_debug_level ( const std::string& command,
				  const strvec& args, size_t first_arg );
	RESULT show_hostname ( const std::string& command,
			       const strvec& args, size_t first_arg );
	RESULT show_stats ( const std::string& command,
			    const strvec& args, size_t first_arg );
	RESULT show_settings ( const std::string& command,
			       const strvec& args, size_t first_arg );
	RESULT show_version ( const std::string& command,
			      const strvec& args, size_t first_arg );
	RESULT show_uptime ( const std::string& command,
			     const strvec& args, size_t first_arg );
	RESULT show_player_expires ( const std::string& command,
				     const strvec& args, size_t first_arg );
	RESULT show_max_radar_range ( const std::string& command,
				      const strvec& args, size_t first_arg );
	RESULT show_fqdn ( const std::string& command,
			   const strvec& args, size_t first_arg );
	RESULT show_out_of_reach ( const std::string& command,
				   const strvec& args, size_t first_arg );
	RESULT show_pilots ( const std::string& command,
			   const strvec& args, size_t first_arg );
	RESULT show_log ( const std::string& command,
			  const strvec& args, size_t first_arg );
	RESULT show_whitelist ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT show_blacklist ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT show_crossfeed ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT show_relay ( const std::string& command,
			    const strvec& args, size_t first_arg );
	RESULT show_tracker ( const std::string& command,
			      const strvec& args, size_t first_arg );

	//////////////////////////////////////////////////
	// configure fgms commands
	//////////////////////////////////////////////////
	RESULT cfg_fgms ( const std::string& command,
			  const strvec& args, size_t first_arg );
	RESULT cfg_daemon ( const std::string& command,
			    const strvec& args, size_t first_arg );
	RESULT cfg_bind_addr ( const std::string& command,
			       const strvec& args, size_t first_arg );
	RESULT cfg_admin_user ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT cfg_admin_pass ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT cfg_admin_enable ( const std::string& command,
				  const strvec& args, size_t first_arg );
	RESULT cfg_data_port ( const std::string& command,
			       const strvec& args, size_t first_arg );
	RESULT cfg_query_port ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT cfg_admin_port ( const std::string& command,
				const strvec& args, size_t first_arg );
	RESULT cfg_admin_cli ( const std::string& command,
			       const strvec& args, size_t first_arg );
	RESULT cfg_logfile_name ( const std::string& command,
				  const strvec& args, size_t first_arg );
	RESULT cfg_debug_level ( const std::string& command,
				 const strvec& args, size_t first_arg );
	RESULT cfg_hostname ( const std::string& command,
			      const strvec& args, size_t first_arg );
	RESULT cfg_player_expires ( const std::string& command,
				    const strvec& args, size_t first_arg );
	RESULT cfg_max_radar_range ( const std::string& command,
				     const strvec& args, size_t first_arg );
	RESULT cfg_fqdn ( const std::string& command,
			  const strvec& args, size_t first_arg );
	RESULT cfg_out_of_reach ( const std::string& command,
				  const strvec& args, size_t first_arg );

	//////////////////////////////////////////////////
	// show/modify whitelist
	//////////////////////////////////////////////////
	RESULT cmd_whitelist_add    ( const std::string& command,
				      const strvec& args, size_t first_arg );
	RESULT cmd_whitelist_delete ( const std::string& command,
				      const strvec& args, size_t first_arg );
	//////////////////////////////////////////////////
	// show/modify blacklist
	//////////////////////////////////////////////////
	RESULT cmd_blacklist_add    ( const std::string& command,
				      const strvec& args, size_t first_arg );
	RESULT cmd_blacklist_delete ( const std::string& command,
				      const strvec& args, size_t first_arg );
	//////////////////////////////////////////////////
	// show/modify list of crossfeeds
	//////////////////////////////////////////////////
	RESULT cmd_crossfeed_add    ( const std::string& command,
				      const strvec& args, size_t first_arg );
	RESULT cmd_crossfeed_delete ( const std::string& command,
				      const strvec& args, size_t first_arg );
	//////////////////////////////////////////////////
	// show/modify list of relays
	//////////////////////////////////////////////////
	RESULT cmd_relay_add    ( const std::string& command,
				  const strvec& args, size_t first_arg );
	RESULT cmd_relay_delete ( const std::string& command,
				  const strvec& args, size_t first_arg );

	//////////////////////////////////////////////////
	// general  commands
	//////////////////////////////////////////////////
	RESULT cmd_die           ( const std::string& command,
				   const strvec& args, size_t first_arg );
	// TODO: change the size of the logbuffer
private:
	fgmp::fgms* m_fgms;
};

} // namespace fgmp

#endif

