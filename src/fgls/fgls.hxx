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
 * @file        fgls.hxx
 *              flightgear list server
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        07/2017
 *
 * This is the list server for the flightgear multiplayer
 * network. All servers register at this server. All clients ask fgls
 * which server they should use.
 *
 */

#ifndef _fgls_header
#define _fgls_header

#include <stdint.h>
#include <string>
#include <fglib/daemon.hxx>
#include <fglib/fg_thread.hxx>
#include <fglib/fg_list.hxx>
#include <fglib/fg_version.hxx>
#include <fglib/fg_proto.hxx>
#include "relay.hxx"

namespace fgmp
{

/** The List server
 */
class fgls
{
public:
	/** @brief internal constants */
	enum
	{
		MAX_TELNETS = 5
	};
	static const version m_version;
	fgls ();
	bool    init ();
	void    loop ();
	void    parse_params ( int argc, char* argv[] );
	bool    read_configs ();
	void    shutdown ();
	friend class fgls_cli;
protected:
	//////////////////////////////////////////////////
	// configurable via cli
	//////////////////////////////////////////////////
	bool            m_run_as_daemon = false;
	std::string     m_bind_addr     = "";
	std::string     m_admin_user    = "";
	std::string     m_admin_pass    = "";
	std::string     m_admin_enable  = "";
	uint16_t        m_data_port     = 5004;
	uint16_t        m_query_port    = m_data_port + 1;
	uint16_t        m_admin_port    = m_data_port + 2;
	bool            m_admin_cli     = true;
	std::string     m_logfile_name  = "";
	fglog::prio     m_debug_level   = fglog::prio::MEDIUM;
	int             m_check_interval = 5;
	std::string     m_hostname       = "fgls";

	serverlist      m_server_list;
	server_it       m_cur_hub;
	server_it       m_cur_fgms;
	bool            m_reinit_data   = true;
	bool            m_reinit_query  = true;
	bool            m_reinit_admin  = true;
	bool            m_reinit_log    = true;
	bool            m_is_parent     = false;
	bool            m_have_config   = false;
	bool            m_want_exit     = false;
	netsocket*      m_data_channel  = nullptr;
	netsocket*      m_query_channel = nullptr;
	netsocket*      m_admin_channel = nullptr;
	time_t          m_uptime;

	bool    init_data_channel ();
	bool    init_query_channel ();
	bool    init_admin_channel ();
	void    set_bind_addr ( const std::string& addr );
	void    open_logfile ();
	void    print_version ();
	void    print_help ();
	bool    process_config ( const std::string& config_name );
	void    handle_admin ( int fd );

	int     m_argc  = 0;
	char**  m_argv;
}; // class fgls

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

#endif
