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

namespace fgmp
{

//////////////////////////////////////////////////

fgcli::fgcli
(
	fgmp::fgms* fgms,
	int fd
): cli(fd)
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
fgcli::setup
()
{
	using callback_ptr	= Command<cli>::cpp_callback_func;
	using auth_callback	= cli::cpp_auth_func;
	using enable_callback	= cli::cpp_enable_func;
	Command<cli>* c;
	// Command<cli>* c2;

	//////////////////////////////////////////////////
	// general setup
	//////////////////////////////////////////////////
	set_hostname (this->fgms->m_server_name);
	std::stringstream banner;
	banner	<< "\r\n"
		<< "------------------------------------------------\r\n"
		<< "FlightGear Multiplayer Server cli\r\n"
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
	c = new Command<cli> (
		this,
		"show",
		libcli::UNPRIVILEGED,
		libcli::MODE_EXEC,
		"show system information"
	);
	register_command (c);

	register_command ( new Command<cli> (
		this,
		"stats",
		static_cast<callback_ptr> (&fgcli::cmd_show_stats),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show statistical information"
	), c);

	register_command ( new Command<cli> (
		this,
		"settings",
		static_cast<callback_ptr> (&fgcli::cmd_show_settings),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show general settings"
	), c);

	register_command ( new Command<cli> (
		this,
		"version",
		static_cast<callback_ptr> (&fgcli::cmd_show_version),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show running version information"
	), c);

	register_command ( new Command<cli> (
		this,
		"uptime",
		static_cast<callback_ptr> (&fgcli::cmd_show_uptime),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show uptime information"
	), c);

	register_command ( new Command<cli> (
		this,
		"log",
		static_cast<callback_ptr> (&fgcli::cmd_show_log),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show log buffer"
	), c);

	register_command (new Command<cli> (
		this,
		"whitelist",
		static_cast<callback_ptr> (&fgcli::cmd_whitelist_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show entries in the whitelist"
	), c);

	register_command (new Command<cli> (
		this,
		"blacklist",
		static_cast<callback_ptr> (&fgcli::cmd_blacklist_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show entries in the blacklist"
	), c);

	register_command (new Command<cli> (
		this,
		"crossfeeds",
		static_cast<callback_ptr> (&fgcli::cmd_crossfeed_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show entries in the crossfeeds"
	), c);

	register_command (new Command<cli> (
		this,
		"relay",
		static_cast<callback_ptr> (&fgcli::cmd_relay_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show list of relays"
	), c);

	register_command (new Command<cli> (
		this,
		"tracker",
		static_cast<callback_ptr> (&fgcli::cmd_tracker_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show status of tracker"
	), c);

	register_command (new Command<cli> (
		this,
		"users",
		static_cast<callback_ptr> (&fgcli::cmd_user_show),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show list of users"
	), c);

	register_command (new Command<cli> (
		this,
		"die",
		static_cast<callback_ptr> (&fgcli::cmd_fgms_die),
		libcli::PRIVILEGED,
		libcli::MODE_EXEC,
		"force fgms to exit"
	));

	//////////////////////////////////////////////////
	// modify blacklist
	//////////////////////////////////////////////////
	c = new Command<cli> (
		this,
		"blacklist",
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"show/modify blacklist"
	);
	register_command (c);
	register_command (new Command<cli> (
		this,
		"delete",
		static_cast<callback_ptr> (&fgcli::cmd_blacklist_delete),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"delete entries in the blacklist"
	), c);
	register_command (new Command<cli> (
		this,
		"add",
		static_cast<callback_ptr> (&fgcli::cmd_blacklist_add),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Add entries to the blacklist"
	), c);

	//////////////////////////////////////////////////
	// modify crossfeeds
	//////////////////////////////////////////////////
	c = new Command<cli> (
		this,
		"crossfeed",
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"modify crossfeeds"
	);
	register_command (c);
	register_command (new Command<cli> (
		this,
		"delete",
		static_cast<callback_ptr> (&fgcli::cmd_crossfeed_delete),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"delete crossfeeds"
	), c);
	register_command (new Command<cli> (
		this,
		"add",
		static_cast<callback_ptr> (&fgcli::cmd_crossfeed_add),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Add crossfeeds"
	), c);

	//////////////////////////////////////////////////
	// modify relays
	//////////////////////////////////////////////////
	c = new Command<cli> (
		this,
		"relay",
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"modify relays"
	);
	register_command (c);
	register_command (new Command<cli> (
		this,
		"delete",
		static_cast<callback_ptr> (&fgcli::cmd_relay_delete),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"delete relay"
	), c);
	register_command (new Command<cli> (
		this,
		"add",
		static_cast<callback_ptr> (&fgcli::cmd_relay_add),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Add relay"
	), c);

	//////////////////////////////////////////////////
	// modify whitelist
	//////////////////////////////////////////////////
	c = new Command<cli> (
		this,
		"whitelist",
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"modify whitelist"
	);
	register_command (c);
	register_command (new Command<cli> (
		this,
		"delete",
		static_cast<callback_ptr> (&fgcli::cmd_whitelist_delete),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"delete whitelist entry"
	), c);
	register_command (new Command<cli> (
		this,
		"add",
		static_cast<callback_ptr> (&fgcli::cmd_whitelist_add),
		libcli::PRIVILEGED,
		libcli::MODE_CONFIG,
		"Add entries to the whitelist"
	), c);

} // fgcli::setup ()

//////////////////////////////////////////////////
/**
 *  @brief Show general statistics
 */
int
fgcli::cmd_show_stats
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
	client	<< "I have " << fgms->m_black_list.size ()
		<< " entries in my blacklist"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgms->m_cross_list.size () << " crossfeeds"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgms->m_relay_list.size () << " relays"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgms->m_player_list.size () << " users ("
		<< fgms->m_local_clients << " local, "
		<< fgms->m_remote_clients << " remote, "
		<< fgms->m_num_max_clients << " max)"
		<< CRLF; if (check_pager()) return 0;

	client << "Sent counters:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "to crossfeeds:"
		<< fgms->m_cross_list.pkts_sent << " packets"
		<< " (" << fgms->m_cross_list.pkts_sent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_cross_list.bytes_sent)
		<< " (" << byte_counter ((double) fgms->m_cross_list.bytes_sent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "to relays:"
		<< fgms->m_relay_list.pkts_sent << " packets"
		<< " (" << fgms->m_relay_list.pkts_sent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_relay_list.bytes_sent)
		<< " (" << byte_counter ((double) fgms->m_relay_list.bytes_sent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0; 
	client << "  " << left << setfill(' ') << setw(22)
		<< "to users:"
		<< fgms->m_player_list.pkts_sent << " packets"
		<< " (" << fgms->m_player_list.pkts_sent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_player_list.bytes_sent)
		<< " (" << byte_counter ((double) fgms->m_player_list.bytes_sent / difftime) << "/s)"
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
		<< fgms->m_black_list.pkts_rcvd << " packets"
		<< " (" << fgms->m_black_list.pkts_rcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_black_list.bytes_rcvd)
		<< " (" << byte_counter ((double) fgms->m_black_list.bytes_rcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "relays:"
		<< fgms->m_relay_list.pkts_rcvd << " packets"
		<< " (" << fgms->m_relay_list.pkts_rcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_relay_list.bytes_rcvd)
		<< " (" << byte_counter ((double) fgms->m_relay_list.bytes_rcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "users:"
		<< fgms->m_player_list.pkts_rcvd << " packets"
		<< " (" << fgms->m_player_list.pkts_rcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_player_list.bytes_rcvd)
		<< " (" << byte_counter ((double) fgms->m_player_list.bytes_rcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	accumulated_sent	+= fgms->m_cross_list.bytes_sent;
	accumulated_sent_pkts	+= fgms->m_cross_list.pkts_sent;
	accumulated_sent	+= fgms->m_relay_list.bytes_sent;
	accumulated_sent_pkts	+= fgms->m_relay_list.pkts_sent;
	accumulated_sent	+= fgms->m_player_list.bytes_sent;
	accumulated_sent_pkts	+= fgms->m_player_list.pkts_sent;
	accumulated_rcvd	+= fgms->m_black_list.bytes_rcvd;
	accumulated_rcvd_pkts	+= fgms->m_black_list.pkts_rcvd;
	accumulated_rcvd	+= fgms->m_relay_list.bytes_rcvd;
	accumulated_rcvd_pkts	+= fgms->m_relay_list.pkts_rcvd;
	accumulated_rcvd	+= fgms->m_player_list.bytes_rcvd;
	accumulated_rcvd_pkts	+= fgms->m_player_list.pkts_rcvd;
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
} // fgcli::cmd_show_stats ()

//////////////////////////////////////////////////
/**
 *  @brief Show general settings
 */
int
fgcli::cmd_show_settings
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
} // fgcli::cmd_show_settings ()

//////////////////////////////////////////////////
/**
 *  @brief Shutdown the server
 */
int
fgcli::cmd_fgms_die
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
} // fgcli::cmd_fgms_die

//////////////////////////////////////////////////
/**
 *  @brief Show the uptime of the the server
 *         in a human readable form.
 */
int
fgcli::cmd_show_uptime
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
} // fgcli::cmd_show_uptime

//////////////////////////////////////////////////
/**
 *  @brief Show log buffer of the the server
 */
int
fgcli::cmd_show_log
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
	fgmp::str_list*  buf = logger.logbuf();
	fgmp::str_it     it;
	buf->lock ();
	for ( it = buf->begin(); it != buf->end(); it++ )
	{
		client << *it << commit;
		if (check_pager()) return 0;
	}
	buf->unlock ();
	return (0);
} // fgcli::cmd_show_uptime

//////////////////////////////////////////////////
/**
 *  @brief Show the version number of the the server
 */
int
fgcli::cmd_show_version
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
		client << "This server is tracked: " << fgms->m_tracker->get_server () << CRLF;
	else
		client << "This server is NOT tracked" << CRLF;
	cmd_show_uptime (command, argv, argc);
	return (0);
} // fgcli::cmd_show_version

//////////////////////////////////////////////////
/**
 *  @brief Show Whitelist
 *
 *  possible arguments:
 *  show whitelist ?
 *  show whitelist <cr>
 *  show whitelist id
 *  show whitelist IP-address
 */
int
fgcli::cmd_whitelist_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		id = 0;
	int		id_invalid = -1;
	fgmp::netaddr	address;
	size_t		EntriesFound = 0;
	for (int i=0; i < argc; i++)
	{
		id  = str_to_num<size_t> ( argv[0], id_invalid );
		if (id_invalid)
			id = 0;
		switch (i)
		{
		case 0: // id or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "id" << "show entry with id" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "show entry with IP-address" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "<cr>" << "show long listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "|" << "output modifier" << CRLF;
				return (0);
			}
			else if (id == 0)
			{
				address.assign (argv[0], 0);
				if ( ! address.is_valid() )
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
	int Count = fgms->m_white_list.size ();
	fgmp::list_item Entry("");
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_uptime;
	client << fgms->m_white_list.name << ":" << CRLF;
	client << CRLF;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_white_list[i];
		if ( (id == 0) && (address.is_valid()) )
		{	// only list matching entries
			if (Entry.address != address)
			{
			std::cout << "e1: " << Entry.address << " != " << address << std::endl;
				continue;
			}
		}
		else if (id)
		{
			if (Entry.id != id)
			{
			std::cout << "e2: " << Entry.id << " != " << id << std::endl;
				continue;
			}
		}
		EntriesFound++;
		client << "id " << Entry.id << ": "
			<< Entry.address.to_string() << " : " << Entry.name
			<< CRLF; if (check_pager()) return 0;
		client << "  entered      : " << timestamp_to_datestr (Entry.join_time)
			<< CRLF; if (check_pager()) return 0;
		client << "  last seen    : " << timestamp_to_days (Entry.last_seen)
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd packets : " << Entry.pkts_rcvd
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd bytes   : " << byte_counter (Entry.bytes_rcvd)
			<< CRLF; if (check_pager()) return 0;
	}
	if (EntriesFound)
		client << CRLF; if (check_pager()) return 0;
	client << EntriesFound << " entries found" << CRLF; if (check_pager()) return 0;
	if (EntriesFound)
	{
		client << "Total rcvd: "
			<< fgms->m_white_list.pkts_rcvd << " packets"
			<< " (" << fgms->m_white_list.pkts_rcvd / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_white_list.bytes_rcvd)
			<< " (" << byte_counter ((double) (fgms->m_white_list.bytes_rcvd/difftime)) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	return 0;
} // fgcli::cmd_whitelist_show

//////////////////////////////////////////////////
/**
 *  @brief delete whitelist entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  whitelist delete ?
 *  whitelist delete id
 *  whitelist delete IP-address
 *  whitelist delete [...] <cr>
 */
int
fgcli::cmd_whitelist_delete
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		id = 0;
	int		id_invalid = -1;
	fgmp::netaddr		address;
	fglistit		Entry;
	for (int i=0; i < argc; i++)
	{
		id  = str_to_num<size_t> ( argv[0], id_invalid );
		if (id_invalid)
			id = 0;
		switch (i)
		{
		case 0: // id or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "id" << "delete entry with id" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "delete entry with IP address" << CRLF;
				return (0);
			}
			else if (id == 0)
			{
				address.assign (argv[0], 0);
				if (! address.is_valid())
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
	if ( (id == 0) && (! address.is_valid()) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (id == 0) && (address.is_valid()) )
	{	// match IP
		Entry = fgms->m_white_list.find (address);
		if (Entry != fgms->m_white_list.end())
		{
			fgms->m_white_list.erase (Entry);
		}
		else
		{
			client << "no entry found!" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_white_list.find_by_id (id);
	if (Entry != fgms->m_white_list.end())
	{
		fgms->m_white_list.erase (Entry);
	}
	else
	{
		client << "no entry found!" << CRLF;
		return 1;
	}
	client << "deleted!" << CRLF;
	return 0;
} // fgcli::cmd_whitelist_delete

//////////////////////////////////////////////////
/**
 *  @brief Add Whitelist entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  blacklist add ?
 *  blacklist add TTL IP-address [reason]
 *  blacklist add [...] <cr>
 */
int
fgcli::cmd_whitelist_add
(
	char *command,
	char *argv[],
	int argc
)
{
	time_t		TTL = -1;
	int		I;
	fgmp::netaddr		address;
	string		Reason;
	fglistit		Entry;
	for (int i=0; i < argc; i++)
	{
		switch (i)
		{
		case 0: // must be TTL or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "TTL" << "timeout of the new entry in seconds" << CRLF;
				return (0);
			}
			TTL  = str_to_num<size_t> ( argv[0], I );
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
			address.assign (argv[i], 0);
			if (! address.is_valid())
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
	fgmp::list_item E (Reason);
	E.address = address;
	size_t Newid;
	fglistit CurrentEntry = fgms->m_white_list.find ( E.address );
	if ( CurrentEntry == fgms->m_white_list.end() )
	{       
		Newid = fgms->m_white_list.add (E, TTL);
	}
	else
	{
		client << "% entry already exists (id " << CurrentEntry->id << ")!" << CRLF;
		return 1;
	}
	client << "added with id " << Newid << CRLF;
	return (0);
} // fgcli::cmd_whitelist_add

//////////////////////////////////////////////////
/**
 *  @brief Show Blacklist
 *
 *  possible arguments:
 *  show blacklist ?
 *  show blacklist <cr>
 *  show blacklist id
 *  show blacklist IP-address
 *  show blacklist [...] brief
 */
int
fgcli::cmd_blacklist_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		id = 0;
	int		id_invalid = -1;
	fgmp::netaddr		address;
	bool		Brief = false;
	size_t		EntriesFound = 0;
	for (int i=0; i < argc; i++)
	{
		id  = str_to_num<size_t> ( argv[0], id_invalid );
		if (id_invalid)
			id = 0;
		switch (i)
		{
		case 0: // id or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "brief" << "show brief listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "id" << "show entry with id" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "show entry with IP-address" << CRLF;
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
			else if (id == 0)
			{
				address.assign (argv[0], 0);
				if (! address.is_valid())
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
	int Count = fgms->m_black_list.size ();
	fgmp::list_item Entry("");
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_uptime;
	client << fgms->m_black_list.name << ":" << CRLF;
	client << CRLF;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_black_list[i];
		if ( (id == 0) && (address.is_valid()) )
		{	// only list matching entries
			if (Entry.address != address)
				continue;
		}
		else if (id)
		{
			if (Entry.id != id)
				continue;
		}
		EntriesFound++;
		client << "id " << Entry.id << ": "
			<< Entry.address.to_string() << " : " << Entry.name
			<< CRLF; if (check_pager()) return 0;
		if (Brief == true)
		{
			continue;
		}
		string expire = "NEVER";
		if (Entry.timeout != 0)
		{
			expire = num_to_str (Entry.timeout, 0) + " seconds";
		}
		client << "  entered      : " << timestamp_to_datestr (Entry.join_time)
			<< CRLF; if (check_pager()) return 0;
		client << "  last seen    : " << timestamp_to_days (Entry.last_seen)
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd packets : " << Entry.pkts_rcvd
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd bytes   : " << byte_counter (Entry.bytes_rcvd)
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
			<< fgms->m_black_list.pkts_rcvd << " packets"
			<< " (" << fgms->m_black_list.pkts_rcvd / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_black_list.bytes_rcvd)
			<< " (" << byte_counter ((double) (fgms->m_black_list.bytes_rcvd/difftime)) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	return 0;
} // fgcli::cmd_blacklist_show

//////////////////////////////////////////////////
/**
 *  @brief delete Blacklist entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  blacklist delete ?
 *  blacklist delete id
 *  blacklist delete IP-address
 *  blacklist delete [...] <cr>
 */
int
fgcli::cmd_blacklist_delete
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		id = 0;
	int		id_invalid = -1;
	fgmp::netaddr		address;
	fglistit		Entry;
	for (int i=0; i < argc; i++)
	{
		id  = str_to_num<size_t> ( argv[0], id_invalid );
		if (id_invalid)
			id = 0;
		switch (i)
		{
		case 0: // id or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "id" << "delete entry with id" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "delete entry with IP address" << CRLF;
				return (0);
			}
			else if (id == 0)
			{
				address.assign (argv[0], 0);
				if (! address.is_valid())
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
	if ( (id == 0) && (! address.is_valid()) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (id == 0) && (address.is_valid()) )
	{	// match IP
		Entry = fgms->m_black_list.find (address);
		if (Entry != fgms->m_black_list.end())
		{
			fgms->m_black_list.erase (Entry);
		}
		else
		{
			client << "no entry found!" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_black_list.find_by_id (id);
	if (Entry != fgms->m_black_list.end())
	{
		fgms->m_black_list.erase (Entry);
	}
	else
	{
		client << "no entry found!" << CRLF;
		return 1;
	}
	client << "deleted!" << CRLF;
	return 0;
} // fgcli::cmd_blacklist_delete

//////////////////////////////////////////////////
/**
 *  @brief Add Blacklist entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  blacklist add ?
 *  blacklist add TTL IP-address [reason]
 *  blacklist add [...] <cr>
 */
int
fgcli::cmd_blacklist_add
(
	char *command,
	char *argv[],
	int argc
)
{
	time_t		TTL = -1;
	int		I;
	fgmp::netaddr		address;
	string		Reason;
	fglistit		Entry;
	for (int i=0; i < argc; i++)
	{
		switch (i)
		{
		case 0: // must be TTL or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "TTL" << "timeout of the new entry in seconds" << CRLF;
				return (0);
			}
			TTL  = str_to_num<size_t> ( argv[0], I );
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
			address.assign (argv[i], 0);
			if (! address.is_valid())
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
	fgmp::list_item E (Reason);
	E.address = address;
	size_t Newid;
	fglistit CurrentEntry = fgms->m_black_list.find ( E.address );
	if ( CurrentEntry == fgms->m_black_list.end() )
	{       
		Newid = fgms->m_black_list.add (E, TTL);
	}
	else
	{
		client << "% entry already exists (id " << CurrentEntry->id << ")!" << CRLF;
		return 1;
	}
	client << "added with id " << Newid << CRLF;
	return (0);
} // fgcli::cmd_blacklist_add

//////////////////////////////////////////////////
/**
 *  @brief delete Crossfeed entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  crossfeed delete ?
 *  crossfeed delete id
 *  crossfeed delete IP-address
 *  crossfeed delete [...] <cr>
 */
int
fgcli::cmd_crossfeed_delete
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		id = 0;
	int		id_invalid = -1;
	fgmp::netaddr		address;
	fglistit		Entry;
	for (int i=0; i < argc; i++)
	{
		id  = str_to_num<size_t> ( argv[0], id_invalid );
		if (id_invalid)
			id = 0;
		switch (i)
		{
		case 0: // id or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "id" << "delete entry with id" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "delete entry with IP address" << CRLF;
				return (0);
			}
			else if (id == 0)
			{
				address.assign (argv[0], 0);
				if (! address.is_valid())
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
	if ( (id == 0) && (! address.is_valid()) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (id == 0) && (address.is_valid()) )
	{	// match IP
		Entry = fgms->m_cross_list.find (address);
		if (Entry != fgms->m_cross_list.end())
		{
			fgms->m_cross_list.erase (Entry);
		}
		else
		{
			client << "no entry found" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_cross_list.find_by_id (id);
	if (Entry != fgms->m_cross_list.end())
	{
		fgms->m_cross_list.erase (Entry);
	}
	else
	{
		client << "no entry found" << CRLF;
		return 1;
	}
	client << "deleted" << CRLF;
	return 0;
} // fgcli::cmd_crossfeed_delete

//////////////////////////////////////////////////
/**
 *  @brief Add Crossfeed entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  crossfeed add ?
 *  crossfeed add IP-address Port name
 *  crossfeed add [...] <cr>
 */
int
fgcli::cmd_crossfeed_add
(
	char *command,
	char *argv[],
	int argc
)
{
	fgmp::netaddr		address;
	string		name;
	int		Port;
	int		I;
	fglistit		Entry;
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
			address.assign (argv[i], 0);
			if (! address.is_valid())
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
			Port  = str_to_num<int> ( argv[i], I );
			if (I)
			{
				client << "% invalid port " << Port << CRLF;
				return (1);
			}
			break;
		default:
			if ( need_help (argv[i]) )
			{
				if (name == "")
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
			name += argv[i];
			if (i+1 < argc)
				name += " ";
			break;
		}
	}
	fgmp::list_item E (name);
	E.address = address;
	E.address.port (Port);
	size_t Newid;
	fglistit CurrentEntry = fgms->m_cross_list.find ( E.address, true );
	if ( CurrentEntry == fgms->m_cross_list.end() )
	{       
		Newid = fgms->m_cross_list.add (E, Port);
	}
	else
	{
		client << "entry already exists (id " << CurrentEntry->id << ")" << CRLF;
		return 1;
	}
	client << "added with id " << Newid << CRLF;
	return (0);
} // fgcli::cmd_crossfeed_add

//////////////////////////////////////////////////
/**
 *  @brief Show Crossfeed
 *
 *  possible arguments:
 *  show blacklist ?
 *  show blacklist <cr>
 *  show blacklist id
 *  show blacklist IP-address
 *  show blacklist [...] brief
 */
int
fgcli::cmd_crossfeed_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		id = 0;
	int		id_invalid = -1;
	fgmp::netaddr		address;
	bool		Brief = false;
	size_t		EntriesFound = 0;
	for (int i=0; i < argc; i++)
	{
		id  = str_to_num<size_t> ( argv[0], id_invalid );
		if (id_invalid)
			id = 0;
		switch (i)
		{
		case 0: // id or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "brief" << "show brief listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "id" << "show entry with id" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "show entry with IP-address" << CRLF;
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
			else if (id == 0)
			{
				address.assign (argv[0], 0);
				if (! address.is_valid())
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
	int Count = fgms->m_cross_list.size ();
	fgmp::list_item Entry("");
	client << fgms->m_cross_list.name << ":" << CRLF;
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_uptime;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_cross_list[i];
		if ( (id == 0) && (address.is_valid()) )
		{	// only list matching entries
			if (Entry.address != address)
				continue;
		}
		else if (id)
		{
			if (Entry.id != id)
				continue;
		}
		EntriesFound++;
		client << "id " << Entry.id << ": "
			<< Entry.address.to_string() << ":" << Entry.address.port()
			<< " : " << Entry.name
			<< CRLF; if (check_pager()) return 0;
		if (Brief == true)
		{
			continue;
		}
		client << "  entered      : " << timestamp_to_datestr (Entry.join_time)
			<< CRLF; if (check_pager()) return 0;
		client << "  last sent    : " << timestamp_to_days (Entry.last_sent)
			<< CRLF; if (check_pager()) return 0;
		client << "  sent packets : " << Entry.pkts_sent
			<< "(" << (double) (Entry.pkts_sent / difftime) << " packets/s)"
			<< CRLF; if (check_pager()) return 0;
		client << "  sent bytes   : " << byte_counter (Entry.bytes_sent)
			<< "(" << byte_counter ((double) Entry.bytes_sent / difftime) << "/s)"
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
			<< fgms->m_cross_list.pkts_sent << " packets"
			<< "(" << fgms->m_cross_list.pkts_sent / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_cross_list.bytes_sent)
			<< "(" << byte_counter ((double) (fgms->m_cross_list.bytes_sent/difftime)) << "/s)"
			<< CRLF;
	}
	return 0;
} // fgcli::cmd_crossfeed_show

//////////////////////////////////////////////////
/**
 *  @brief Show Relays
 *
 *  possible arguments:
 *  show relay ?
 *  show relay <cr>
 *  show relay id
 *  show relay IP-address
 *  show relay [...] brief
 */
int
fgcli::cmd_relay_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		id = 0;
	int		id_invalid = -1;
	fgmp::netaddr		address;
	bool		Brief = false;
	size_t		EntriesFound = 0;
	string		name;
	for (int i=0; i < argc; i++)
	{
		id  = str_to_num<size_t> ( argv[0], id_invalid );
		if (id_invalid)
			id = 0;
		switch (i)
		{
		case 0: // id or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "brief" << "show brief listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "id" << "show entry with id" << CRLF;
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
			else if (id == 0)
			{
				address.assign (argv[0], 0);
				if (! address.is_valid())
				{
					name = argv[0];
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
	int Count = fgms->m_relay_list.size ();
	fgmp::list_item Entry("");
	client << fgms->m_relay_list.name << ":" << CRLF;
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_uptime;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_relay_list[i];
		if ( (id == 0) && (address.is_valid()) )
		{	// only list matching entries
			if (Entry.address != address)
				continue;
		}
		else if (id)
		{
			if (Entry.id != id)
				continue;
		}
		else if (name != "")
		{
			if (Entry.name.find(name) == string::npos)
				continue;
		}
		EntriesFound++;
		client << "id " << Entry.id << ": "
			<< Entry.address.to_string() << ":" << Entry.address.port()
			<< " : " << Entry.name
			<< CRLF; if (check_pager()) return 0;
		if (Brief == true)
		{
			continue;
		}
		client << "  entered   : " << timestamp_to_datestr (Entry.join_time)
			<< CRLF; if (check_pager()) return 0;
		client << "  last seen : " << timestamp_to_datestr (Entry.last_seen)
			<< CRLF; if (check_pager()) return 0;
		client << "  sent      : "
			<< Entry.pkts_sent << " packets"
			<< " (" << Entry.pkts_sent / difftime << "/s)"
			<< " / " << byte_counter (Entry.bytes_sent)
			<< " (" << byte_counter ((double) Entry.bytes_sent / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
		client << "  rcvd      : "
			<< Entry.pkts_rcvd << " packets"
			<< " (" << Entry.pkts_rcvd / difftime << "/s)"
			<< " / " << byte_counter (Entry.bytes_rcvd)
			<< " (" << byte_counter ((double) Entry.bytes_rcvd / difftime) << "/s)"
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
		<< fgms->m_relay_list.pkts_sent << " packets"
		<< " (" << fgms->m_relay_list.pkts_sent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_relay_list.bytes_sent)
		<< " (" << byte_counter ((double) fgms->m_relay_list.bytes_sent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  received  : "
		<< fgms->m_relay_list.pkts_rcvd << " packets"
		<< " (" << fgms->m_relay_list.pkts_rcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_relay_list.bytes_rcvd)
		<< " (" << byte_counter ((double) fgms->m_relay_list.bytes_rcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	return 0;
} // fgcli::cmd_relay_show

//////////////////////////////////////////////////
/**
 *  @brief Show status of tracker server
 *
 *  possible arguments:
 *  show tracker ?
 *  show tracker <cr>
 */
int
fgcli::cmd_tracker_show
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
		<< fgms->m_tracker->get_server () << ":"
		<< fgms->m_tracker->get_port()
		<< CRLF;
	if (fgms->m_tracker->is_connected ())
	{
		client << "state: connected since "
			<< timestamp_to_datestr(fgms->m_tracker->last_connected)
			<< " (" << timestamp_to_days (fgms->m_tracker->last_connected) << " ago)"
			<< CRLF;
	}
	else
	{
		client << "state: NOT connected!" << CRLF;
	}
	string A = "NEVER";
	if (fgms->m_tracker->last_seen != 0)
	{
		A = timestamp_to_days (fgms->m_tracker->last_seen);
		A += " ago";
	}
	string B = "NEVER";
	if (fgms->m_tracker->last_sent != 0)
	{
		B = timestamp_to_days (fgms->m_tracker->last_sent);
		B += " ago";
	}
	client << "last seen " << A << ", last sent " << B << CRLF;
	client << "I had " << fgms->m_tracker->lost_connections << " lost connections" << CRLF;
	client << CRLF;
	client << "Counters:" << CRLF;
	client << "  sent    : " << fgms->m_tracker->pkts_sent << " packets";
	client << " (" << fgms->m_tracker->pkts_sent / difftime << "/s)";
	client << " / " << byte_counter (fgms->m_tracker->bytes_sent);
	client << " (" << byte_counter ((double) fgms->m_tracker->bytes_sent / difftime) << "/s)";
	client << CRLF;
	client << "  received: " << fgms->m_tracker->pkts_rcvd << " packets";
	client << " (" << fgms->m_tracker->pkts_rcvd / difftime << "/s)";
	client << " / " << byte_counter (fgms->m_tracker->bytes_rcvd);
	client << " (" << byte_counter ((double) fgms->m_tracker->bytes_rcvd / difftime) << "/s)";
	client << CRLF;
	client << "  queue size: " << fgms->m_tracker->queue_size () << " messages" << CRLF;
	return 0;
} // fgcli::cmd_tracker_show

//////////////////////////////////////////////////
/**
 *  @brief delete Relay entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  relay delete ?
 *  relay delete id
 *  relay delete IP-address
 *  relay delete [...] <cr>
 */
int
fgcli::cmd_relay_delete
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		id = 0;
	int		id_invalid = -1;
	fgmp::netaddr		address;
	fglistit		Entry;
	for (int i=0; i < argc; i++)
	{
		id  = str_to_num<size_t> ( argv[0], id_invalid );
		if (id_invalid)
			id = 0;
		switch (i)
		{
		case 0: // id or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "id" << "delete entry with id" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "delete entry with IP address" << CRLF;
				return (0);
			}
			else if (id == 0)
			{
				address.assign (argv[0], 0);
				if (! address.is_valid())
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
	if ( (id == 0) && (! address.is_valid()) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (id == 0) && (address.is_valid()) )
	{	// match IP
		Entry = fgms->m_relay_list.find (address);
		if (Entry != fgms->m_relay_list.end())
		{
			fgms->m_relay_list.erase (Entry);
		}
		else
		{
			client << "no entry found" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_relay_list.find_by_id (id);
	if (Entry != fgms->m_relay_list.end())
	{
		fgms->m_relay_list.erase (Entry);
	}
	else
	{
		client << "no entry found" << CRLF;
		return 1;
	}
	client << "deleted" << CRLF;
	return 0;
} // fgcli::cmd_relay_delete

//////////////////////////////////////////////////
/**
 *  @brief Add Relay entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  relay add ?
 *  relay add IP-address Port [name]
 *  relay add [...] <cr>
 */
int
fgcli::cmd_relay_add
(
	char *command,
	char *argv[],
	int argc
)
{
	fgmp::netaddr		address;
	string		name;
	int		Port;
	int		I;
	fglistit		Entry;
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
			address.assign (argv[i], 0);
			if (! address.is_valid())
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
			Port  = str_to_num<int> ( argv[i], I );
			if (I)
			{
				client << "% invalid port " << Port << CRLF;
				return (1);
			}
			break;
		default: // '?' or <CR>
			if ( need_help (argv[i]) )
			{
				if (name == "")
					client << "  " << left << setfill(' ') << setw(22)
						<< "NAME" << "the name of this relay" << CRLF;
				else
					client << "  " << left << setfill(' ') << setw(22)
						<< "<cr>" << "add this relay" << CRLF;
				return 0;
			}
			name += argv[i];
			if (i+1 < argc)
				name += " ";
			break;
		}
	}
	fgmp::list_item E (name);
	E.address = address;
	E.address.port (Port);
	size_t Newid;
	fglistit CurrentEntry = fgms->m_relay_list.find ( E.address, true );
	if ( CurrentEntry == fgms->m_relay_list.end() )
	{       
		Newid = fgms->m_relay_list.add (E, 0);
	}
	else
	{
		client << "entry already exists (id " << CurrentEntry->id << ")" << CRLF;
		return 1;
	}
	client << "added with id " << Newid << CRLF;
	return (0);
} // fgcli::cmd_relay_add

//////////////////////////////////////////////////
/**
 *  @brief Show Players
 *
 *  possible arguments:
 *  show user ?
 *  show user <cr>
 *  show user id <cr>
 *  show user IP-address <cr>
 *  show user name <cr>
 *  show user local <cr>
 *  show user remote <cr>
 *  show user [...] brief <cr>
 */
int
fgcli::cmd_user_show
(
	char *command,
	char *argv[],
	int argc
)
{
	size_t		id = 0;
	int		id_invalid = -1;
	fgmp::netaddr		address;
	string		name;
	bool		Brief = false;
	bool		OnlyLocal = false;
	bool		OnlyRemote = false;
	size_t		EntriesFound = 0;
	time_t		difftime;
	time_t		now = time(0);
	for (int i=0; i < argc; i++)
	{
		id  = str_to_num<size_t> ( argv[0], id_invalid );
		if (id_invalid)
			id = 0;
		switch (i)
		{
		case 0: // id or IP or 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				client << "  " << left << setfill(' ') << setw(22)
					<< "brief" << "show brief listing" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "id" << "show user with id" << CRLF;
				client << "  " << left << setfill(' ') << setw(22)
					<< "IP" << "show user with IP-address" << CRLF;
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
			else if (id == 0)
			{
				address.assign (argv[0], 0);
				if (! address.is_valid())
				{
					name = argv[0];
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
	int Count = fgms->m_player_list.size ();
	pilot	Player;
	point3d	PlayerPosGeod;
	string	Origin;
	string	Fullname;
	client << fgms->m_player_list.name << ":" << CRLF;
	client << CRLF;
	if (name == "local")
	{
		OnlyLocal = true;
		name = "";
	}
	if (name == "remote")
	{
		OnlyRemote = true;
		name = "";
	}
	for (int i = 0; i < Count; i++)
	{
		now = time(0);
		difftime = now - fgms->m_uptime;
		Player = fgms->m_player_list[i];
		if ( (id == 0) && (address.is_valid()) )
		{	// only list matching entries
			if (Player.address != address)
				continue;
		}
		else if (id)
		{
			if (Player.id != id)
				continue;
		}
		else if (name != "")
		{
			if (Player.name.find(name) == string::npos)
				continue;
		}
		else if (OnlyLocal == true)
		{
			if (Player.is_local == false)
				continue;
		}
		else if (OnlyRemote == true)
		{
			if (Player.is_local == true)
				continue;
		}
		cart_to_geod ( Player.last_pos, PlayerPosGeod );
		if (Player.is_local)
		{
			Origin = "LOCAL";
		}
		else
		{
			fgms::ip2relay_it Relay = fgms->m_relay_map.find ( Player.address );
			if ( Relay != fgms->m_relay_map.end() )
			{
				Origin = Relay->second;
			}
			else
			{
				Origin = Player.origin;
			}
		}
		Fullname = Player.name + string("@") + Origin;
		std::string ATC;
		using fgmp::ATC_TYPE;
		switch ( Player.is_ATC )
		{
		case ATC_TYPE::NONE:	ATC = ", a normal pilot"; break;
		case ATC_TYPE::ATC:	ATC = ", an ATC"; break;
		case ATC_TYPE::ATC_DL:	ATC = ", a clearance delivery ATC"; break;
		case ATC_TYPE::ATC_GN:	ATC = ", a ground ATC"; break;
		case ATC_TYPE::ATC_TW:	ATC = ", a tower ATC"; break;
		case ATC_TYPE::ATC_AP:	ATC = ", an approach ATC"; break;
		case ATC_TYPE::ATC_DE:	ATC = ", a departure ATC"; break;
		case ATC_TYPE::ATC_CT:	ATC = ", a center ATC"; break;
		}
		client << "id " << Player.id << ": "
			<< Fullname << ATC
			<< CRLF; if (check_pager()) return 0;
		EntriesFound++;
		if (Brief == true)
		{
			continue;
		}
		if (Player.has_errors == true)
		{
			client << "         " << left << setfill(' ') << setw(15)
				<< "ERROR" << Player.error
				<< CRLF; if (check_pager()) return 0;
		}
		client << "         " << left << setfill(' ') << setw(15)
			<< "protocoll" << Player.proto_major << "." << Player.proto_minor
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "radar range" << Player.radar_range
			<< CRLF; if (check_pager()) return 0;
		int expires = Player.timeout - (now - Player.last_seen);
		client << "         " << left << setfill(' ') << setw(15)
			<< "entered" << timestamp_to_days (Player.join_time) << " ago"
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "joined" << timestamp_to_datestr(Player.join_time)
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "last seen" << timestamp_to_datestr(Player.last_seen)
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "using model" << Player.model_name
			<< CRLF; if (check_pager()) return 0;
		if (Player.is_local)
		{
			client << "         " << left << setfill(' ') << setw(15)
				<< "real origin" << Player.origin
				<< CRLF; if (check_pager()) return 0;
			client << "         " << left << setfill(' ') << setw(15)
				<< "sent" << Player.pkts_sent << " packets "
				<< "(" << Player.pkts_sent / difftime << "/s)"
				<< " / " << byte_counter (Player.bytes_sent)
				<< " (" << byte_counter ((double) Player.bytes_sent / difftime) << "/s)"
				<< CRLF; if (check_pager()) return 0;
		}

		client << "         " << left << setfill(' ') << setw(15)
			<< "rcvd" << Player.pkts_rcvd << " packets "
			<< "(" << Player.pkts_rcvd / difftime << "/s)"
			<< " / " << byte_counter (Player.bytes_rcvd)
			<< " (" << byte_counter ((double) Player.bytes_rcvd / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "expires in" << expires
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "inactive" << now - Player.last_relayed_to_inactive
			<< CRLF; if (check_pager()) return 0;
	}
	difftime = now - fgms->m_uptime;
	client << CRLF;
	client << EntriesFound << " entries found" << CRLF;
	if (! Brief)
	{
		client << "Totals:" << CRLF; if (check_pager()) return 0;
		client << "          sent    : " << fgms->m_player_list.pkts_sent << " packets"
			<< " (" << fgms->m_player_list.pkts_sent / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_player_list.bytes_sent)
			<< " (" << byte_counter ((double) fgms->m_player_list.bytes_sent / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
		client << "          received: " << fgms->m_player_list.pkts_rcvd << " packets"
			<< " (" << fgms->m_player_list.pkts_rcvd / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_player_list.bytes_rcvd) 
			<< " (" << byte_counter ((double) fgms->m_player_list.bytes_rcvd / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	return 0;
} // fgcli::cmd_user_show

int
fgcli::cmd_NOT_IMPLEMENTED
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

} // namespace fgmp

