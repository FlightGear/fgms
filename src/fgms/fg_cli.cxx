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
 * @file	fg_cli.cxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	2013
 */

#include <sstream>
#include <fglib/fg_util.hxx>
#include <fg_cli.hxx>

//////////////////////////////////////////////////

FG_CLI::FG_CLI
(
	FGMS* fgms,
	int fd
): CLI(fd)
{
	this->fgms = fgms;
	this->setup ();
}

//////////////////////////////////////////////////
/**
 *  @brief Set up all commands
 *
 */
void
FG_CLI::setup
()
{
	typedef Command<CLI>::cpp_callback_func callback_ptr;
	typedef Command<CLI>::cpp_callback_func callback_ptr;
	typedef CLI::cpp_auth_func auth_callback;                                                                            
	typedef CLI::cpp_enable_func enable_callback;
	Command<CLI>* c;
	// Command<CLI>* c2;

	//////////////////////////////////////////////////
	// general setup
	//////////////////////////////////////////////////
	set_hostname (this->fgms->m_server_name);
	std::stringstream banner;
	banner	<< "\r\n"
		<< "------------------------------------------------\r\n"
		<< "FlightGear Multiplayer Server CLI\r\n"
		<< "This is " << fgms->m_server_name << " (" << fgms->m_FQDN << ")\r\n"
		<< "------------------------------------------------\r\n";
	set_banner ( banner.str() );
	//////////////////////////////////////////////////
	// setup authentication (if required)
	//////////////////////////////////////////////////
	if (fgms->m_admin_user != "")
	{
		allow_user ( fgms->m_admin_user, fgms->m_admin_pass );
	}
	if (fgms->m_admin_enable != "")
	{
		allow_enable ( fgms->m_admin_enable );
	}
	//////////////////////////////////////////////////
	// general commands
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"show",
		libcli::UNPRIVILEGED,
		libcli::MODE_EXEC,
		"show system information"
	);
	register_command (c);

	register_command ( new Command<CLI> (
		this,
		"stats",
		static_cast<callback_ptr> (&FG_CLI::cmd_show_stats),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show statistical information"
	), c);

	register_command ( new Command<CLI> (
		this,
		"settings",
		static_cast<callback_ptr> (&FG_CLI::cmd_show_settings),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show general settings"
	), c);

	register_command ( new Command<CLI> (
		this,
		"version",
		static_cast<callback_ptr> (&FG_CLI::cmd_show_version),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show running version information"
	), c);

	register_command ( new Command<CLI> (
		this,
		"uptime",
		static_cast<callback_ptr> (&FG_CLI::cmd_show_uptime),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show uptime information"
	), c);

	register_command ( new Command<CLI> (
		this,
		"log",
		static_cast<callback_ptr> (&FG_CLI::cmd_show_log),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show log buffer"
	), c);

	register_command (new Command<CLI> (
		this,
		"whitelist",
		static_cast<callback_ptr> (&FG_CLI::cmd_whitelist_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show entries in the whitelist"
	), c);

	register_command (new Command<CLI> (
		this,
		"blacklist",
		static_cast<callback_ptr> (&FG_CLI::cmd_blacklist_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show entries in the blacklist"
	), c);

	register_command (new Command<CLI> (
		this,
		"crossfeeds",
		static_cast<callback_ptr> (&FG_CLI::cmd_crossfeed_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show entries in the crossfeeds"
	), c);

	register_command (new Command<CLI> (
		this,
		"relay",
		static_cast<callback_ptr> (&FG_CLI::cmd_relay_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show list of relays"
	), c);

	register_command (new Command<CLI> (
		this,
		"tracker",
		static_cast<callback_ptr> (&FG_CLI::cmd_tracker_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show status of tracker"
	), c);

	register_command (new Command<CLI> (
		this,
		"users",
		static_cast<callback_ptr> (&FG_CLI::cmd_user_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show list of users"
	), c);

	register_command (new Command<CLI> (
		this,
		"die",
		static_cast<callback_ptr> (&FG_CLI::cmd_fgms_die),
		libcli::PRIVILEGED,
		libcli::MODE_EXEC,
		"force fgms to exit"
	));

	//////////////////////////////////////////////////
	// modify blacklist
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"blacklist",
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"show/modify blacklist"
	);
	register_command (c);
	register_command (new Command<CLI> (
		this,
		"delete",
		static_cast<callback_ptr> (&FG_CLI::cmd_blacklist_delete),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Delete entries in the blacklist"
	), c);
	register_command (new Command<CLI> (
		this,
		"add",
		static_cast<callback_ptr> (&FG_CLI::cmd_blacklist_add),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Add entries to the blacklist"
	), c);

	//////////////////////////////////////////////////
	// modify crossfeeds
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"crossfeed",
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"modify crossfeeds"
	);
	register_command (c);
	register_command (new Command<CLI> (
		this,
		"delete",
		static_cast<callback_ptr> (&FG_CLI::cmd_crossfeed_delete),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Delete crossfeeds"
	), c);
	register_command (new Command<CLI> (
		this,
		"add",
		static_cast<callback_ptr> (&FG_CLI::cmd_crossfeed_add),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Add crossfeeds"
	), c);

	//////////////////////////////////////////////////
	// modify relays
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"relay",
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"modify relays"
	);
	register_command (c);
	register_command (new Command<CLI> (
		this,
		"delete",
		static_cast<callback_ptr> (&FG_CLI::cmd_relay_delete),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Delete relay"
	), c);
	register_command (new Command<CLI> (
		this,
		"add",
		static_cast<callback_ptr> (&FG_CLI::cmd_relay_add),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Add relay"
	), c);

	//////////////////////////////////////////////////
	// modify whitelist
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"whitelist",
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"modify whitelist"
	);
	register_command (c);
	register_command (new Command<CLI> (
		this,
		"delete",
		static_cast<callback_ptr> (&FG_CLI::cmd_whitelist_delete),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Delete whitelist entry"
	), c);
	register_command (new Command<CLI> (
		this,
		"add",
		static_cast<callback_ptr> (&FG_CLI::cmd_whitelist_add),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Add entries to the whitelist"
	), c);

} // FG_CLI::setup ()

//////////////////////////////////////////////////
/**
 *  @brief Show general statistics
 */
int
FG_CLI::cmd_show_stats
(
	UNUSED(char *command),
	UNUSED(char *argv[]),
	UNUSED(int argc)
)
{
	time_t	difftime;
	time_t	now;
	uint64_t	accumulated_sent	= 0;
	uint64_t	accumulated_rcvd	= 0;
	uint64_t	accumulated_sent_pkts	= 0;
	uint64_t	accumulated_rcvd_pkts	= 0;
	if (argc > 0)
	{
		client << "<cr>" << CRLF;
		return (0);
	}
	now = time(0);
	difftime = now - fgms->m_uptime;
	cmd_show_version (command, argv, argc);
	client << CRLF;
	client	<< "I have " << fgms->m_black_list.Size ()
		<< " entries in my blacklist"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgms->m_cross_list.Size () << " crossfeeds"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgms->m_relay_list.Size () << " relays"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgms->m_player_list.Size () << " users ("
		<< fgms->m_local_clients << " local, "
		<< fgms->m_remote_clients << " remote, "
		<< fgms->m_num_max_clients << " max)"
		<< CRLF; if (check_pager()) return 0;

	client << "Sent counters:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "to crossfeeds:"
		<< fgms->m_cross_list.PktsSent << " packets"
		<< " (" << fgms->m_cross_list.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_cross_list.BytesSent)
		<< " (" << byte_counter ((double) fgms->m_cross_list.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "to relays:"
		<< fgms->m_relay_list.PktsSent << " packets"
		<< " (" << fgms->m_relay_list.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_relay_list.BytesSent)
		<< " (" << byte_counter ((double) fgms->m_relay_list.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0; 
	client << "  " << left << setfill(' ') << setw(22)
		<< "to users:"
		<< fgms->m_player_list.PktsSent << " packets"
		<< " (" << fgms->m_player_list.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_player_list.BytesSent)
		<< " (" << byte_counter ((double) fgms->m_player_list.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;

	client << "Receive counters:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "total:" <<  fgms->m_packets_received
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "pings:" <<  fgms->m_ping_received
		<< " (" << fgms->m_pong_received << " pongs)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "errors:"
		<< "invalid packets:" << fgms->m_packets_invalid
		<< " rejected:" << fgms->m_black_rejected
		<< " unknown relay:" << fgms->m_unknown_relay
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "valid data:"
		<< "pos data:" << fgms->m_pos_data
		<< " other:" << fgms->m_unknown_data
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "tracker:"
		<< "connects:" << fgms->m_tracker_connect
		<< " disconnects:" << fgms->m_tracker_disconnect
		<< " positions:" << fgms->m_tracker_position
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "admin connections:" << fgms->m_admin_received
		<< CRLF; if (check_pager()) return 0;
	float telnet_per_second;
	if (fgms->m_queries_received)
		telnet_per_second = (float) fgms->m_queries_received /
		(time(0) - fgms->m_uptime);
	else
		telnet_per_second = 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "telnet connections: "
		<< fgms->m_queries_received
		<< " (" << setprecision(2) << telnet_per_second << " t/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "blacklist:"
		<< fgms->m_black_list.PktsRcvd << " packets"
		<< " (" << fgms->m_black_list.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_black_list.BytesRcvd)
		<< " (" << byte_counter ((double) fgms->m_black_list.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "relays:"
		<< fgms->m_relay_list.PktsRcvd << " packets"
		<< " (" << fgms->m_relay_list.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_relay_list.BytesRcvd)
		<< " (" << byte_counter ((double) fgms->m_relay_list.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "users:"
		<< fgms->m_player_list.PktsRcvd << " packets"
		<< " (" << fgms->m_player_list.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_player_list.BytesRcvd)
		<< " (" << byte_counter ((double) fgms->m_player_list.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	accumulated_sent	+= fgms->m_cross_list.BytesSent;
	accumulated_sent_pkts	+= fgms->m_cross_list.PktsSent;
	accumulated_sent	+= fgms->m_relay_list.BytesSent;
	accumulated_sent_pkts	+= fgms->m_relay_list.PktsSent;
	accumulated_sent	+= fgms->m_player_list.BytesSent;
	accumulated_sent_pkts	+= fgms->m_player_list.PktsSent;
	accumulated_rcvd	+= fgms->m_black_list.BytesRcvd;
	accumulated_rcvd_pkts	+= fgms->m_black_list.PktsRcvd;
	accumulated_rcvd	+= fgms->m_relay_list.BytesRcvd;
	accumulated_rcvd_pkts	+= fgms->m_relay_list.PktsRcvd;
	accumulated_rcvd	+= fgms->m_player_list.BytesRcvd;
	accumulated_rcvd_pkts	+= fgms->m_player_list.PktsRcvd;
	client << "Totals:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "sent:"
		<< accumulated_sent_pkts << " packets"
		<< " (" << accumulated_sent_pkts / difftime << "/s)"
		<< " / " << byte_counter (accumulated_sent)
		<< " (" << byte_counter ((double) accumulated_sent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "received:"
		<< accumulated_rcvd_pkts << " packets"
		<< " (" << accumulated_rcvd_pkts / difftime << "/s)"
		<< " / " << byte_counter (accumulated_rcvd)
		<< " (" << byte_counter ((double) accumulated_rcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	return (0);
} // FG_CLI::cmd_show_stats ()

//////////////////////////////////////////////////
/**
 *  @brief Show general settings
 */
int
FG_CLI::cmd_show_settings
(
	UNUSED(char *command),
	UNUSED(char *argv[]),
	UNUSED(int argc)
)
{
	if (argc > 0)
	{
		client << "<cr>" << CRLF;
		return (0);
	}
	std::string bind_addr;

	if ( fgms->m_bind_addr == "" )
		bind_addr = "*";
	else
		bind_addr = fgms->m_bind_addr;
	cmd_show_version (command, argv, argc);
	client << CRLF;
	client << "current settings:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "listen port:" << fgms->m_data_port
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "telnet port:" << fgms->m_query_port
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "admin port:" << fgms->m_admin_port
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "player expires:" << fgms->m_player_expires
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "out of reach:" << fgms->m_out_of_reach
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "radar range:" << fgms->m_max_radar_range
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "logfile:" << fgms->m_logfile_name
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "bind address:" << bind_addr
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "FQDN:" << fgms->m_FQDN
		<< CRLF; if (check_pager()) return 0;

	return (0);
} // FG_CLI::cmd_show_settings ()

//////////////////////////////////////////////////
/**
 *  @brief Shutdown the server
 */
int
FG_CLI::cmd_fgms_die
(
	UNUSED(char *command),
	UNUSED(char *argv[]),
	UNUSED(int argc)
)
{
	if (argc > 0)
	{
		if (strcmp (argv[0], "?") == 0)
		{
			client << "<cr>" << CRLF;
		}
		return (0);
	}
	fgms->m_want_exit = true;
	return libcli::QUIT;
} // FG_CLI::cmd_fgms_die

//////////////////////////////////////////////////
/**
 *  @brief Show the uptime of the the server
 *         in a human readable form.
 */
int
FG_CLI::cmd_show_uptime
(
	UNUSED(char *command),
	UNUSED(char *argv[]),
	UNUSED(int argc)
)
{
	if (argc > 0)
	{
		if (strcmp (argv[0], "?") == 0)
		{
			client << "<cr>" << CRLF;
		}
		return (0);
	}
	client << "UP since " << timestamp_to_datestr(fgms->m_uptime)
		<< "(" << timestamp_to_days(fgms->m_uptime) << ")" << CRLF;
	return (0);
} // FG_CLI::cmd_show_uptime

//////////////////////////////////////////////////
/**
 *  @brief Show log buffer of the the server
 */
int
FG_CLI::cmd_show_log
(
	UNUSED(char *command),
	UNUSED(char *argv[]),
	UNUSED(int argc)
)
{
	if (argc > 0)
	{
		if (strcmp (argv[0], "?") == 0)
		{
			client << "<cr>" << CRLF;
		}
		return (0);
	}
	fgmp::StrList*  buf = logger.logbuf();
	fgmp::StrIt     it;
	buf->Lock ();
	for ( it = buf->begin(); it != buf->end(); it++ )
	{
		client << *it << commit;
		if (check_pager()) return 0;
	}
	buf->Unlock ();
	return (0);
} // FG_CLI::cmd_show_uptime

//////////////////////////////////////////////////
/**
 *  @brief Show the version number of the the server
 */
int
FG_CLI::cmd_show_version
(
	UNUSED(char *command),
	UNUSED(char *argv[]),
	UNUSED(int argc)
)
{
	if (argc > 0)
	{
		if (strcmp (argv[0], "?") == 0)
		{
			client << "<cr>" << CRLF;
		}
		return (0);
	}
	string s;
	if (fgms->m_me_is_hub)
		s = "HUB";
	else
		s = "LEAVE";
	client << "This is " << fgms->m_server_name << " (" << fgms->m_FQDN << ")" << CRLF;
	client << "FlightGear Multiplayer " << s << " Server version "
	       << fgms->m_version.str() << CRLF; 
	client << "using protocol version v"
		<< fgms->m_proto_major_version << "."
		<< fgms->m_proto_minor_version << CRLF;
	if (fgms->m_is_tracked)
		client << "This server is tracked: " << fgms->m_tracker->GetTrackerServer() << CRLF;
	else
		client << "This server is NOT tracked" << CRLF;
	cmd_show_uptime (command, argv, argc);
	return (0);
} // FG_CLI::cmd_show_version

//////////////////////////////////////////////////
/**
 *  @brief Show Whitelist
 *
 *  possible arguments:
 *  show whitelist ?
 *  show whitelist <cr>
 *  show whitelist ID
 *  show whitelist IP-Address
 */
int
FG_CLI::cmd_whitelist_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	fgmp::netaddr	Address;
	size_t		EntriesFound = 0;
	for (int i=0; i < argc; i++)
	{
		ID  = StrToNum<size_t> ( argv[0], ID_invalid );
		if (ID_invalid)
			ID = 0;
		switch (i)
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "ID" << "show entry with ID" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "show entry with IP-Address" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (ID == 0)
			{
				Address.assign (argv[0], 0);
				if ( ! Address.is_valid() )
				{
					client << "% invalid IP address" << CRLF;
					return (1);
				}
			}
			break;
		case 1: // '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			break;
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	int Count = fgms->m_white_list.Size ();
	fgmp::ListElement Entry("");
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_uptime;
	client << fgms->m_white_list.Name << ":" << CRLF;
	client << CRLF;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_white_list[i];
		if ( (ID == 0) && (Address.is_valid()) )
		{	// only list matching entries
			if (Entry.Address != Address)
			{
			std::cout << "e1: " << Entry.Address << " != " << Address << std::endl;
				continue;
			}
		}
		else if (ID)
		{
			if (Entry.ID != ID)
			{
			std::cout << "e2: " << Entry.ID << " != " << ID << std::endl;
				continue;
			}
		}
		EntriesFound++;
		client << "ID " << Entry.ID << ": "
			<< Entry.Address.to_string() << " : " << Entry.Name
			<< CRLF; if (check_pager()) return 0;
		client << "  entered      : " << timestamp_to_datestr (Entry.JoinTime)
			<< CRLF; if (check_pager()) return 0;
		client << "  last seen    : " << timestamp_to_days (Entry.LastSeen)
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd packets : " << Entry.PktsRcvd
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd bytes   : " << byte_counter (Entry.BytesRcvd)
			<< CRLF; if (check_pager()) return 0;
	}
	if (EntriesFound)
		client << CRLF; if (check_pager()) return 0;
	client << EntriesFound << " entries found" << CRLF; if (check_pager()) return 0;
	if (EntriesFound)
	{
		client << "Total rcvd: "
			<< fgms->m_white_list.PktsRcvd << " packets"
			<< " (" << fgms->m_white_list.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_white_list.BytesRcvd)
			<< " (" << byte_counter ((double) (fgms->m_white_list.BytesRcvd/difftime)) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	return 0;
} // FG_CLI::cmd_whitelist_show

//////////////////////////////////////////////////
/**
 *  @brief Delete whitelist entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  whitelist delete ?
 *  whitelist delete ID
 *  whitelist delete IP-Address
 *  whitelist delete [...] <cr>
 */
int
FG_CLI::cmd_whitelist_delete
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	fgmp::netaddr		Address;
	ItList		Entry;
	for (int i=0; i < argc; i++)
	{
		ID  = StrToNum<size_t> ( argv[0], ID_invalid );
		if (ID_invalid)
			ID = 0;
		switch (i)
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "ID" << "delete entry with ID" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "delete entry with IP address" << CRLF;
				return (0);
			}
			else if (ID == 0)
			{
				Address.assign (argv[0], 0);
				if (! Address.is_valid())
				{
					client << "% invalid IP address" << CRLF;
					return (1);
				}
			}
			break;
		case 1: // only '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "delete entry" << CRLF;
				return 1;
			}
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	if ( (ID == 0) && (! Address.is_valid()) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (ID == 0) && (Address.is_valid()) )
	{	// match IP
		Entry = fgms->m_white_list.Find (Address);
		if (Entry != fgms->m_white_list.End())
		{
			fgms->m_white_list.Delete (Entry);
		}
		else
		{
			client << "no entry found!" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_white_list.FindByID (ID);
	if (Entry != fgms->m_white_list.End())
	{
		fgms->m_white_list.Delete (Entry);
	}
	else
	{
		client << "no entry found!" << CRLF;
		return 1;
	}
	client << "deleted!" << CRLF;
	return 0;
} // FG_CLI::cmd_whitelist_delete

//////////////////////////////////////////////////
/**
 *  @brief Add Whitelist entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  blacklist add ?
 *  blacklist add TTL IP-Address [reason]
 *  blacklist add [...] <cr>
 */
int
FG_CLI::cmd_whitelist_add
(
	char *command,
	char *argv[],
	int argc
)
{
	time_t		TTL = -1;
	int		I;
	fgmp::netaddr		Address;
	string		Reason;
	ItList		Entry;
	for (int i=0; i < argc; i++)
	{
		switch (i)
		{
		case 0: // must be TTL or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "TTL" << "Timeout of the new entry in seconds" << CRLF;
				return (0);
			}
			TTL  = StrToNum<size_t> ( argv[0], I );
			if (I)
			{
				client << "% invalid TTL" << CRLF;
				return (1);
			}
			break;
		case 1: // IP or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "IP address which should be whitelisted" << CRLF;
				return (0);
			}
			Address.assign (argv[i], 0);
			if (! Address.is_valid())
			{
				client << "% invalid IP address" << CRLF;
				return (1);
			}
			break;
		default:
			if ( need_help (argv[i]) )
			{
				if (Reason == "")
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "STRING" << "a reason for whitelisting this IP" << CRLF;
				}
				else
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "<cr>" << "add this IP" << CRLF;
				}
				return 0;
			}
			Reason += argv[i];
			if (i+1 < argc)
				Reason += " ";
			break;
		}
	}
	fgmp::ListElement E (Reason);
	E.Address = Address;
	size_t NewID;
	ItList CurrentEntry = fgms->m_white_list.Find ( E.Address );
	if ( CurrentEntry == fgms->m_white_list.End() )
	{       
		NewID = fgms->m_white_list.Add (E, TTL);
	}
	else
	{
		client << "% entry already exists (ID " << CurrentEntry->ID << ")!" << CRLF;
		return 1;
	}
	client << "added with ID " << NewID << CRLF;
	return (0);
} // FG_CLI::cmd_whitelist_add

//////////////////////////////////////////////////
/**
 *  @brief Show Blacklist
 *
 *  possible arguments:
 *  show blacklist ?
 *  show blacklist <cr>
 *  show blacklist ID
 *  show blacklist IP-Address
 *  show blacklist [...] brief
 */
int
FG_CLI::cmd_blacklist_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	fgmp::netaddr		Address;
	bool		Brief = false;
	size_t		EntriesFound = 0;
	for (int i=0; i < argc; i++)
	{
		ID  = StrToNum<size_t> ( argv[0], ID_invalid );
		if (ID_invalid)
			ID = 0;
		switch (i)
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "brief" << "show brief listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "ID" << "show entry with ID" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "show entry with IP-Address" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (strncmp (argv[0], "brief", strlen (argv[i])) == 0)
			{
				Brief = true;
			}
			else if (ID == 0)
			{
				Address.assign (argv[0], 0);
				if (! Address.is_valid())
				{
					client << "% invalid IP address" << CRLF;
					return (1);
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				if (! Brief)
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "brief" << "show brief listing" << CRLF;
				}
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (strncmp (argv[1], "brief", strlen (argv[0])) == 0)
			{
				Brief = true;
			}
			break;
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	int Count = fgms->m_black_list.Size ();
	fgmp::ListElement Entry("");
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_uptime;
	client << fgms->m_black_list.Name << ":" << CRLF;
	client << CRLF;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_black_list[i];
		if ( (ID == 0) && (Address.is_valid()) )
		{	// only list matching entries
			if (Entry.Address != Address)
				continue;
		}
		else if (ID)
		{
			if (Entry.ID != ID)
				continue;
		}
		EntriesFound++;
		client << "ID " << Entry.ID << ": "
			<< Entry.Address.to_string() << " : " << Entry.Name
			<< CRLF; if (check_pager()) return 0;
		if (Brief == true)
		{
			continue;
		}
		string expire = "NEVER";
		if (Entry.Timeout != 0)
		{
			expire = NumToStr (Entry.Timeout, 0) + " seconds";
		}
		client << "  entered      : " << timestamp_to_datestr (Entry.JoinTime)
			<< CRLF; if (check_pager()) return 0;
		client << "  last seen    : " << timestamp_to_days (Entry.LastSeen)
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd packets : " << Entry.PktsRcvd
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd bytes   : " << byte_counter (Entry.BytesRcvd)
			<< CRLF; if (check_pager()) return 0;
		client << "  expire in    : " << expire
			<< CRLF; if (check_pager()) return 0;
	}
	if (EntriesFound)
	{
		client << CRLF; if (check_pager()) return 0;
	}
	client << EntriesFound << " entries found" << CRLF; if (check_pager()) return 0;
	if (EntriesFound)
	{
		client << "Total rcvd: "
			<< fgms->m_black_list.PktsRcvd << " packets"
			<< " (" << fgms->m_black_list.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_black_list.BytesRcvd)
			<< " (" << byte_counter ((double) (fgms->m_black_list.BytesRcvd/difftime)) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	return 0;
} // FG_CLI::cmd_blacklist_show

//////////////////////////////////////////////////
/**
 *  @brief Delete Blacklist entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  blacklist delete ?
 *  blacklist delete ID
 *  blacklist delete IP-Address
 *  blacklist delete [...] <cr>
 */
int
FG_CLI::cmd_blacklist_delete
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	fgmp::netaddr		Address;
	ItList		Entry;
	for (int i=0; i < argc; i++)
	{
		ID  = StrToNum<size_t> ( argv[0], ID_invalid );
		if (ID_invalid)
			ID = 0;
		switch (i)
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "ID" << "delete entry with ID" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "delete entry with IP address" << CRLF;
				return (0);
			}
			else if (ID == 0)
			{
				Address.assign (argv[0], 0);
				if (! Address.is_valid())
				{
					client << "% invalid IP address" << CRLF;
					return (1);
				}
			}
			break;
		case 1: // only '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "delete entry" << CRLF;
				return 1;
			}
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	if ( (ID == 0) && (! Address.is_valid()) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (ID == 0) && (Address.is_valid()) )
	{	// match IP
		Entry = fgms->m_black_list.Find (Address);
		if (Entry != fgms->m_black_list.End())
		{
			fgms->m_black_list.Delete (Entry);
		}
		else
		{
			client << "no entry found!" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_black_list.FindByID (ID);
	if (Entry != fgms->m_black_list.End())
	{
		fgms->m_black_list.Delete (Entry);
	}
	else
	{
		client << "no entry found!" << CRLF;
		return 1;
	}
	client << "deleted!" << CRLF;
	return 0;
} // FG_CLI::cmd_blacklist_delete

//////////////////////////////////////////////////
/**
 *  @brief Add Blacklist entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  blacklist add ?
 *  blacklist add TTL IP-Address [reason]
 *  blacklist add [...] <cr>
 */
int
FG_CLI::cmd_blacklist_add
(
	char *command,
	char *argv[],
	int argc
)
{
	time_t		TTL = -1;
	int		I;
	fgmp::netaddr		Address;
	string		Reason;
	ItList		Entry;
	for (int i=0; i < argc; i++)
	{
		switch (i)
		{
		case 0: // must be TTL or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "TTL" << "Timeout of the new entry in seconds" << CRLF;
				return (0);
			}
			TTL  = StrToNum<size_t> ( argv[0], I );
			if (I)
			{
				client << "% invalid TTL" << CRLF;
				return (1);
			}
			break;
		case 1: // IP or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "IP address which should be blacklisted" << CRLF;
				return (0);
			}
			Address.assign (argv[i], 0);
			if (! Address.is_valid())
			{
				client << "% invalid IP address" << CRLF;
				return (1);
			}
			break;
		default:
			if ( need_help (argv[i]) )
			{
				if (Reason == "")
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "STRING" << "a reason for blacklisting this IP" << CRLF;
				}
				else
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "<cr>" << "add this IP" << CRLF;
				}
				return 0;
			}
			Reason += argv[i];
			if (i+1 < argc)
				Reason += " ";
			break;
		}
	}
	fgmp::ListElement E (Reason);
	E.Address = Address;
	size_t NewID;
	ItList CurrentEntry = fgms->m_black_list.Find ( E.Address );
	if ( CurrentEntry == fgms->m_black_list.End() )
	{       
		NewID = fgms->m_black_list.Add (E, TTL);
	}
	else
	{
		client << "% entry already exists (ID " << CurrentEntry->ID << ")!" << CRLF;
		return 1;
	}
	client << "added with ID " << NewID << CRLF;
	return (0);
} // FG_CLI::cmd_blacklist_add

//////////////////////////////////////////////////
/**
 *  @brief Delete Crossfeed entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  crossfeed delete ?
 *  crossfeed delete ID
 *  crossfeed delete IP-Address
 *  crossfeed delete [...] <cr>
 */
int
FG_CLI::cmd_crossfeed_delete
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	fgmp::netaddr		Address;
	ItList		Entry;
	for (int i=0; i < argc; i++)
	{
		ID  = StrToNum<size_t> ( argv[0], ID_invalid );
		if (ID_invalid)
			ID = 0;
		switch (i)
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "ID" << "delete entry with ID" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "delete entry with IP address" << CRLF;
				return (0);
			}
			else if (ID == 0)
			{
				Address.assign (argv[0], 0);
				if (! Address.is_valid())
				{
					client << "% invalid IP address" << CRLF;
					return (1);
				}
			}
			break;
		case 1: // only '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "delete this crossfeed" << CRLF;
				return 1;
			}
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	if ( (ID == 0) && (! Address.is_valid()) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (ID == 0) && (Address.is_valid()) )
	{	// match IP
		Entry = fgms->m_cross_list.Find (Address);
		if (Entry != fgms->m_cross_list.End())
		{
			fgms->m_cross_list.Delete (Entry);
		}
		else
		{
			client << "no entry found" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_cross_list.FindByID (ID);
	if (Entry != fgms->m_cross_list.End())
	{
		fgms->m_cross_list.Delete (Entry);
	}
	else
	{
		client << "no entry found" << CRLF;
		return 1;
	}
	client << "deleted" << CRLF;
	return 0;
} // FG_CLI::cmd_crossfeed_delete

//////////////////////////////////////////////////
/**
 *  @brief Add Crossfeed entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  crossfeed add ?
 *  crossfeed add IP-Address Port Name
 *  crossfeed add [...] <cr>
 */
int
FG_CLI::cmd_crossfeed_add
(
	char *command,
	char *argv[],
	int argc
)
{
	fgmp::netaddr		Address;
	string		Name;
	int		Port;
	int		I;
	ItList		Entry;
	for (int i=0; i < argc; i++)
	{
		switch (i)
		{
		case 0: // IP or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "IP address of the crossfeed" << CRLF;
				return (0);
			}
			Address.assign (argv[i], 0);
			if (! Address.is_valid())
			{
				client << "% invalid IP address" << CRLF;
				return (1);
			}
			break;
		case 1: // Port or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "Port" << "Port of the relay" << CRLF;
				return (0);
			}
			Port  = StrToNum<int> ( argv[i], I );
			if (I)
			{
				client << "% invalid port " << Port << CRLF;
				return (1);
			}
			break;
		default:
			if ( need_help (argv[i]) )
			{
				if (Name == "")
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "NAME" << "The name of this crossfeed" << CRLF;
				}
				else
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "<cr>" << "add this crossfeed" << CRLF;
				}
				return 0;
			}
			Name += argv[i];
			if (i+1 < argc)
				Name += " ";
			break;
		}
	}
	fgmp::ListElement E (Name);
	E.Address = Address;
	E.Address.port (Port);
	size_t NewID;
	ItList CurrentEntry = fgms->m_cross_list.Find ( E.Address, true );
	if ( CurrentEntry == fgms->m_cross_list.End() )
	{       
		NewID = fgms->m_cross_list.Add (E, Port);
	}
	else
	{
		client << "entry already exists (ID " << CurrentEntry->ID << ")" << CRLF;
		return 1;
	}
	client << "added with ID " << NewID << CRLF;
	return (0);
} // FG_CLI::cmd_crossfeed_add

//////////////////////////////////////////////////
/**
 *  @brief Show Crossfeed
 *
 *  possible arguments:
 *  show blacklist ?
 *  show blacklist <cr>
 *  show blacklist ID
 *  show blacklist IP-Address
 *  show blacklist [...] brief
 */
int
FG_CLI::cmd_crossfeed_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	fgmp::netaddr		Address;
	bool		Brief = false;
	size_t		EntriesFound = 0;
	for (int i=0; i < argc; i++)
	{
		ID  = StrToNum<size_t> ( argv[0], ID_invalid );
		if (ID_invalid)
			ID = 0;
		switch (i)
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "brief" << "show brief listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "ID" << "show entry with ID" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "show entry with IP-Address" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (strncmp (argv[0], "brief", strlen (argv[i])) == 0)
			{
				Brief = true;
			}
			else if (ID == 0)
			{
				Address.assign (argv[0], 0);
				if (! Address.is_valid())
				{
					client << "% invalid IP address" << CRLF;
					return (1);
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				if (! Brief)
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "brief" << "show brief listing" << CRLF;
				}
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (strncmp (argv[1], "brief", strlen (argv[0])) == 0)
			{
				Brief = true;
			}
			break;
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	int Count = fgms->m_cross_list.Size ();
	fgmp::ListElement Entry("");
	client << fgms->m_cross_list.Name << ":" << CRLF;
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_uptime;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_cross_list[i];
		if ( (ID == 0) && (Address.is_valid()) )
		{	// only list matching entries
			if (Entry.Address != Address)
				continue;
		}
		else if (ID)
		{
			if (Entry.ID != ID)
				continue;
		}
		EntriesFound++;
		client << "ID " << Entry.ID << ": "
			<< Entry.Address.to_string() << ":" << Entry.Address.port()
			<< " : " << Entry.Name
			<< CRLF; if (check_pager()) return 0;
		if (Brief == true)
		{
			continue;
		}
		client << "  entered      : " << timestamp_to_datestr (Entry.JoinTime)
			<< CRLF; if (check_pager()) return 0;
		client << "  last sent    : " << timestamp_to_days (Entry.LastSent)
			<< CRLF; if (check_pager()) return 0;
		client << "  sent packets : " << Entry.PktsSent
			<< "(" << (double) (Entry.PktsSent / difftime) << " packets/s)"
			<< CRLF; if (check_pager()) return 0;
		client << "  sent bytes   : " << byte_counter (Entry.BytesSent)
			<< "(" << byte_counter ((double) Entry.BytesSent / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	if (EntriesFound)
	{
		client << CRLF; if (check_pager()) return 0;
	}
	client << EntriesFound << " entries found" << CRLF; if (check_pager()) return 0;
	if (EntriesFound)
	{
		client << "Total sent: "
			<< fgms->m_cross_list.PktsSent << " packets"
			<< "(" << fgms->m_cross_list.PktsSent / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_cross_list.BytesSent)
			<< "(" << byte_counter ((double) (fgms->m_cross_list.BytesSent/difftime)) << "/s)"
			<< CRLF;
	}
	return 0;
} // FG_CLI::cmd_crossfeed_show

//////////////////////////////////////////////////
/**
 *  @brief Show Relays
 *
 *  possible arguments:
 *  show relay ?
 *  show relay <cr>
 *  show relay ID
 *  show relay IP-Address
 *  show relay [...] brief
 */
int
FG_CLI::cmd_relay_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	fgmp::netaddr		Address;
	bool		Brief = false;
	size_t		EntriesFound = 0;
	string		Name;
	for (int i=0; i < argc; i++)
	{
		ID  = StrToNum<size_t> ( argv[0], ID_invalid );
		if (ID_invalid)
			ID = 0;
		switch (i)
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "brief" << "show brief listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "ID" << "show entry with ID" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "show entry with IP-address" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "NAME" << "show user with NAME" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show log listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (strncmp (argv[0], "brief", strlen (argv[i])) == 0)
			{
				Brief = true;
			}
			else if (ID == 0)
			{
				Address.assign (argv[0], 0);
				if (! Address.is_valid())
				{
					Name = argv[0];
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				if (! Brief)
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "brief" << "show brief listing" << CRLF;
				}
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show log listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (strncmp (argv[1], "brief", strlen (argv[0])) == 0)
			{
				Brief = true;
			}
			break;
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	int Count = fgms->m_relay_list.Size ();
	fgmp::ListElement Entry("");
	client << fgms->m_relay_list.Name << ":" << CRLF;
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_uptime;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_relay_list[i];
		if ( (ID == 0) && (Address.is_valid()) )
		{	// only list matching entries
			if (Entry.Address != Address)
				continue;
		}
		else if (ID)
		{
			if (Entry.ID != ID)
				continue;
		}
		else if (Name != "")
		{
			if (Entry.Name.find(Name) == string::npos)
				continue;
		}
		EntriesFound++;
		client << "ID " << Entry.ID << ": "
			<< Entry.Address.to_string() << ":" << Entry.Address.port()
			<< " : " << Entry.Name
			<< CRLF; if (check_pager()) return 0;
		if (Brief == true)
		{
			continue;
		}
		client << "  entered   : " << timestamp_to_datestr (Entry.JoinTime)
			<< CRLF; if (check_pager()) return 0;
		client << "  last seen : " << timestamp_to_datestr (Entry.LastSeen)
			<< CRLF; if (check_pager()) return 0;
		client << "  sent      : "
			<< Entry.PktsSent << " packets"
			<< " (" << Entry.PktsSent / difftime << "/s)"
			<< " / " << byte_counter (Entry.BytesSent)
			<< " (" << byte_counter ((double) Entry.BytesSent / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd      : "
			<< Entry.PktsRcvd << " packets"
			<< " (" << Entry.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter (Entry.BytesRcvd)
			<< " (" << byte_counter ((double) Entry.BytesRcvd / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	client << CRLF;
	client << EntriesFound << " entries found"
		<< CRLF; if (check_pager()) return 0;
	if (Brief)
	{
		return 0;
	}
	client << "Totals:" << CRLF; if (check_pager()) return 0;
	client << "  sent      : "
		<< fgms->m_relay_list.PktsSent << " packets"
		<< " (" << fgms->m_relay_list.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_relay_list.BytesSent)
		<< " (" << byte_counter ((double) fgms->m_relay_list.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  received  : "
		<< fgms->m_relay_list.PktsRcvd << " packets"
		<< " (" << fgms->m_relay_list.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_relay_list.BytesRcvd)
		<< " (" << byte_counter ((double) fgms->m_relay_list.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	return 0;
} // FG_CLI::cmd_relay_show

//////////////////////////////////////////////////
/**
 *  @brief Show status of tracker server
 *
 *  possible arguments:
 *  show tracker ?
 *  show tracker <cr>
 */
int
FG_CLI::cmd_tracker_show
(
	char *command,
	char *argv[],
	int argc
)
{
	for (int i=0; i < argc; i++)
	{
		switch (i)
		{
		case 0: // '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	if (! fgms->m_is_tracked)
	{
		client << "This server is NOT tracked" << CRLF;
		client << CRLF;
		return 0;
	}
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_uptime;
	client << "This server is tracked: "
		<< fgms->m_tracker->GetTrackerServer() << ":"
		<< fgms->m_tracker->GetTrackerPort()
		<< CRLF;
	if (fgms->m_tracker->m_connected)
	{
		client << "state: connected since "
			<< timestamp_to_datestr(fgms->m_tracker->LastConnected)
			<< " (" << timestamp_to_days (fgms->m_tracker->LastConnected) << " ago)"
			<< CRLF;
	}
	else
	{
		client << "state: NOT connected!" << CRLF;
	}
	string A = "NEVER";
	if (fgms->m_tracker->LastSeen != 0)
	{
		A = timestamp_to_days (fgms->m_tracker->LastSeen);
		A += " ago";
	}
	string B = "NEVER";
	if (fgms->m_tracker->LastSent != 0)
	{
		B = timestamp_to_days (fgms->m_tracker->LastSent);
		B += " ago";
	}
	client << "last seen " << A << ", last sent " << B << CRLF;
	client << "I had " << fgms->m_tracker->LostConnections << " lost connections" << CRLF;
	client << CRLF;
	client << "Counters:" << CRLF;
	client << "  sent    : " << fgms->m_tracker->PktsSent << " packets";
	client << " (" << fgms->m_tracker->PktsSent / difftime << "/s)";
	client << " / " << byte_counter (fgms->m_tracker->BytesSent);
	client << " (" << byte_counter ((double) fgms->m_tracker->BytesSent / difftime) << "/s)";
	client << CRLF;
	client << "  received: " << fgms->m_tracker->PktsRcvd << " packets";
	client << " (" << fgms->m_tracker->PktsRcvd / difftime << "/s)";
	client << " / " << byte_counter (fgms->m_tracker->BytesRcvd);
	client << " (" << byte_counter ((double) fgms->m_tracker->BytesRcvd / difftime) << "/s)";
	client << CRLF;
	client << "  queue size: " << fgms->m_tracker->msg_queue.size() << " messages" << CRLF;
	return 0;
} // FG_CLI::cmd_tracker_show

//////////////////////////////////////////////////
/**
 *  @brief Delete Relay entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  relay delete ?
 *  relay delete ID
 *  relay delete IP-Address
 *  relay delete [...] <cr>
 */
int
FG_CLI::cmd_relay_delete
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	fgmp::netaddr		Address;
	ItList		Entry;
	for (int i=0; i < argc; i++)
	{
		ID  = StrToNum<size_t> ( argv[0], ID_invalid );
		if (ID_invalid)
			ID = 0;
		switch (i)
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "ID" << "delete entry with ID" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "delete entry with IP address" << CRLF;
				return (0);
			}
			else if (ID == 0)
			{
				Address.assign (argv[0], 0);
				if (! Address.is_valid())
				{
					client << "% invalid IP address" << CRLF;
					return (1);
				}
			}
			break;
		case 1: // only '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "delete entry" << CRLF;
				return 1;
			}
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	if ( (ID == 0) && (! Address.is_valid()) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (ID == 0) && (Address.is_valid()) )
	{	// match IP
		Entry = fgms->m_relay_list.Find (Address);
		if (Entry != fgms->m_relay_list.End())
		{
			fgms->m_relay_list.Delete (Entry);
		}
		else
		{
			client << "no entry found" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_relay_list.FindByID (ID);
	if (Entry != fgms->m_relay_list.End())
	{
		fgms->m_relay_list.Delete (Entry);
	}
	else
	{
		client << "no entry found" << CRLF;
		return 1;
	}
	client << "deleted" << CRLF;
	return 0;
} // FG_CLI::cmd_relay_delete

//////////////////////////////////////////////////
/**
 *  @brief Add Relay entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  relay add ?
 *  relay add IP-Address Port [Name]
 *  relay add [...] <cr>
 */
int
FG_CLI::cmd_relay_add
(
	char *command,
	char *argv[],
	int argc
)
{
	fgmp::netaddr		Address;
	string		Name;
	int		Port;
	int		I;
	ItList		Entry;
	for (int i=0; i < argc; i++)
	{
		switch (i)
		{
		case 0: // IP or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "IP address of the relay" << CRLF;
				return (0);
			}
			Address.assign (argv[i], 0);
			if (! Address.is_valid())
			{
				client << "% invalid IP address" << CRLF;
				return (1);
			}
			break;
		case 1: // Port or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "Port" << "Port of the relay" << CRLF;
				return (0);
			}
			Port  = StrToNum<int> ( argv[i], I );
			if (I)
			{
				client << "% invalid port " << Port << CRLF;
				return (1);
			}
			break;
		default: // '?' or <CR>
			if ( need_help (argv[i]) )
			{
				if (Name == "")
					client << "  " << left << setfill(' ') << setw(22)
						<< "NAME" << "the name of this relay" << CRLF;
				else
					client << "  " << left << setfill(' ') << setw(22)
						<< "<cr>" << "add this relay" << CRLF;
				return 0;
			}
			Name += argv[i];
			if (i+1 < argc)
				Name += " ";
			break;
		}
	}
	fgmp::ListElement E (Name);
	E.Address = Address;
	E.Address.port (Port);
	size_t NewID;
	ItList CurrentEntry = fgms->m_relay_list.Find ( E.Address, true );
	if ( CurrentEntry == fgms->m_relay_list.End() )
	{       
		NewID = fgms->m_relay_list.Add (E, 0);
	}
	else
	{
		client << "entry already exists (ID " << CurrentEntry->ID << ")" << CRLF;
		return 1;
	}
	client << "added with ID " << NewID << CRLF;
	return (0);
} // FG_CLI::cmd_relay_add

//////////////////////////////////////////////////
/**
 *  @brief Show Players
 *
 *  possible arguments:
 *  show user ?
 *  show user <cr>
 *  show user ID <cr>
 *  show user IP-Address <cr>
 *  show user Name <cr>
 *  show user local <cr>
 *  show user remote <cr>
 *  show user [...] brief <cr>
 */
int
FG_CLI::cmd_user_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	fgmp::netaddr		Address;
	string		Name;
	bool		Brief = false;
	bool		OnlyLocal = false;
	bool		OnlyRemote = false;
	size_t		EntriesFound = 0;
	time_t		difftime;
	time_t		now = time(0);
	for (int i=0; i < argc; i++)
	{
		ID  = StrToNum<size_t> ( argv[0], ID_invalid );
		if (ID_invalid)
			ID = 0;
		switch (i)
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "brief" << "show brief listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "ID" << "show user with ID" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "show user with IP-Address" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "NAME" << "show user with NAME" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "local" << "show only local users" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "remote" << "show only local users" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (strncmp (argv[0], "brief", strlen (argv[i])) == 0)
			{
				Brief = true;
			}
			else if (ID == 0)
			{
				Address.assign (argv[0], 0);
				if (! Address.is_valid())
				{
					Name = argv[0];
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				if (! Brief)
				{
					client << "  " << left << setfill(' ') << setw(22)
						<< "brief" << "show brief listing" << CRLF;
				}
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (strncmp (argv[1], "brief", strlen (argv[0])) == 0)
			{
				Brief = true;
			}
			break;
		default:
			client << "% invalid argument" << CRLF;
			return libcli::ERROR_ARG;
		}
	}
	int Count = fgms->m_player_list.Size ();
	FG_Player	Player;
	Point3D		PlayerPosGeod;
	string		Origin;
	string		FullName;
	client << fgms->m_player_list.Name << ":" << CRLF;
	client << CRLF;
	if (Name == "local")
	{
		OnlyLocal = true;
		Name = "";
	}
	if (Name == "remote")
	{
		OnlyRemote = true;
		Name = "";
	}
	for (int i = 0; i < Count; i++)
	{
		now = time(0);
		difftime = now - fgms->m_uptime;
		Player = fgms->m_player_list[i];
		if ( (ID == 0) && (Address.is_valid()) )
		{	// only list matching entries
			if (Player.Address != Address)
				continue;
		}
		else if (ID)
		{
			if (Player.ID != ID)
				continue;
		}
		else if (Name != "")
		{
			if (Player.Name.find(Name) == string::npos)
				continue;
		}
		else if (OnlyLocal == true)
		{
			if (Player.IsLocal == false)
				continue;
		}
		else if (OnlyRemote == true)
		{
			if (Player.IsLocal == true)
				continue;
		}
		sgCartToGeod ( Player.LastPos, PlayerPosGeod );
		if (Player.IsLocal)
		{
			Origin = "LOCAL";
		}
		else
		{
			FGMS::ip2relay_it Relay = fgms->m_relay_map.find ( Player.Address );
			if ( Relay != fgms->m_relay_map.end() )
			{
				Origin = Relay->second;
			}
			else
			{
				Origin = Player.Origin;
			}
		}
		FullName = Player.Name + string("@") + Origin;
		std::string ATC;
		switch ( Player.IsATC )
		{
		case FG_Player::ATC_NONE:	ATC = ", a normal pilot"; break;
		case FG_Player::ATC:		ATC = ", an ATC"; break;
		case FG_Player::ATC_DL:		ATC = ", a clearance delivery ATC"; break;
		case FG_Player::ATC_GN:		ATC = ", a ground ATC"; break;
		case FG_Player::ATC_TW:		ATC = ", a tower ATC"; break;
		case FG_Player::ATC_AP:		ATC = ", an approach ATC"; break;
		case FG_Player::ATC_DE:		ATC = ", a departure ATC"; break;
		case FG_Player::ATC_CT:		ATC = ", a center ATC"; break;
		}
		client << "ID " << Player.ID << ": "
			<< FullName << ATC
			<< CRLF; if (check_pager()) return 0;
		EntriesFound++;
		if (Brief == true)
		{
			continue;
		}
		if (Player.HasErrors == true)
		{
			client << "         " << left << setfill(' ') << setw(15)
				<< "ERROR" << Player.Error
				<< CRLF; if (check_pager()) return 0;
		}
		client << "         " << left << setfill(' ') << setw(15)
			<< "protocoll" << Player.ProtoMajor << "." << Player.ProtoMinor
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "radar range" << Player.RadarRange
			<< CRLF; if (check_pager()) return 0;
		int expires = Player.Timeout - (now - Player.LastSeen);
		client << "         " << left << setfill(' ') << setw(15)
			<< "entered" << timestamp_to_days (Player.JoinTime) << " ago"
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "joined" << timestamp_to_datestr(Player.JoinTime)
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "last seen" << timestamp_to_datestr(Player.LastSeen)
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "using model" << Player.ModelName
			<< CRLF; if (check_pager()) return 0;
		if (Player.IsLocal)
		{
			client << "         " << left << setfill(' ') << setw(15)
				<< "real origin" << Player.Origin
				<< CRLF; if (check_pager()) return 0;
			client << "         " << left << setfill(' ') << setw(15)
				<< "sent" << Player.PktsSent << " packets "
				<< "(" << Player.PktsSent / difftime << "/s)"
				<< " / " << byte_counter (Player.BytesSent)
				<< " (" << byte_counter ((double) Player.BytesSent / difftime) << "/s)"
				<< CRLF; if (check_pager()) return 0;
		}

		client << "         " << left << setfill(' ') << setw(15)
			<< "rcvd" << Player.PktsRcvd << " packets "
			<< "(" << Player.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter (Player.BytesRcvd)
			<< " (" << byte_counter ((double) Player.BytesRcvd / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "expires in" << expires
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "inactive" << now - Player.LastRelayedToInactive
			<< CRLF; if (check_pager()) return 0;
	}
	difftime = now - fgms->m_uptime;
	client << CRLF;
	client << EntriesFound << " entries found" << CRLF;
	if (! Brief)
	{
		client << "Totals:" << CRLF; if (check_pager()) return 0;
		client << "          sent    : " << fgms->m_player_list.PktsSent << " packets"
			<< " (" << fgms->m_player_list.PktsSent / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_player_list.BytesSent)
			<< " (" << byte_counter ((double) fgms->m_player_list.BytesSent / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
		client << "          received: " << fgms->m_player_list.PktsRcvd << " packets"
			<< " (" << fgms->m_player_list.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_player_list.BytesRcvd) 
			<< " (" << byte_counter ((double) fgms->m_player_list.BytesRcvd / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	return 0;
} // FG_CLI::cmd_user_show

int
FG_CLI::cmd_NOT_IMPLEMENTED
(
	char *command,
	char *argv[],
	int argc
)
{
	client << "Command '" << command << "' NOT IMPLEMENTED YET" << CRLF;
	if (argc > 0)
	{
		client << "  args:" << CRLF;
		for (int i=0; i<argc; i++)
			client << "  '" << argv[i] << "'" << CRLF;
	}
	return (0);
}

