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

#include <fg_util.hxx>
#include <fg_cli.hxx>
#include <fg_common.hxx>

FG_CLI::FG_CLI
(
	FG_SERVER* fgms,
	int fd
): CLI(fd)
{
	this->fgms = fgms;
	this->setup ();
}

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
	set_hostname (this->fgms->m_ServerName.c_str());
	set_banner (
		"\r\n"
		"------------------------------------------------\r\n"
		"FlightGear Multiplayer Server CLI\r\n"
		"------------------------------------------------\r\n"
	);
	//////////////////////////////////////////////////
	// setup authentication (if required)
	//////////////////////////////////////////////////
	if (fgms->m_AdminUser != "")
	{
		allow_user ( fgms->m_AdminUser.c_str(), fgms->m_AdminPass.c_str() );
	}
	if (fgms->m_AdminEnable != "")
	{
		allow_enable (fgms->m_AdminEnable.c_str());
	}
	//////////////////////////////////////////////////
	// general commands
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"show",
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_EXEC,
		"show system information"
	);
	register_command (c);

	register_command ( new Command<CLI> (
		this,
		"stats",
		static_cast<callback_ptr> (&FG_CLI::cmd_show_stats),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show statistical information"
	), c);

	register_command ( new Command<CLI> (
		this,
		"version",
		static_cast<callback_ptr> (&FG_CLI::cmd_show_version),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show running version information"
	), c);

	register_command ( new Command<CLI> (
		this,
		"uptime",
		static_cast<callback_ptr> (&FG_CLI::cmd_show_uptime),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show uptime information"
	), c);

	register_command (new Command<CLI> (
		this,
		"blacklist",
		static_cast<callback_ptr> (&FG_CLI::cmd_blacklist_show),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show entries in the blacklist"
	), c);

	register_command (new Command<CLI> (
		this,
		"crossfeeds",
		static_cast<callback_ptr> (&FG_CLI::cmd_crossfeed_show),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show entries in the crossfeeds"
	), c);

	register_command (new Command<CLI> (
		this,
		"relay",
		static_cast<callback_ptr> (&FG_CLI::cmd_relay_show),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show list of relays"
	), c);

	register_command (new Command<CLI> (
		this,
		"tracker",
		static_cast<callback_ptr> (&FG_CLI::cmd_tracker_show),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show status of tracker"
	), c);

	register_command (new Command<CLI> (
		this,
		"users",
		static_cast<callback_ptr> (&FG_CLI::cmd_user_show),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show list of users"
	), c);

	register_command (new Command<CLI> (
		this,
		"die",
		static_cast<callback_ptr> (&FG_CLI::cmd_fgms_die),
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_EXEC,
		"force fgms to exit"
	));

	//////////////////////////////////////////////////
	// modify blacklist
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"blacklist",
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_CONFIG,
		"show/modify blacklist"
	);
	register_command (c);
	register_command (new Command<CLI> (
		this,
		"delete",
		static_cast<callback_ptr> (&FG_CLI::cmd_blacklist_delete),
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_CONFIG,
		"Show entries in the blacklist"
	), c);
	register_command (new Command<CLI> (
		this,
		"add",
		static_cast<callback_ptr> (&FG_CLI::cmd_blacklist_add),
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_CONFIG,
		"Show entries in the blacklist"
	), c);

	//////////////////////////////////////////////////
	// modify crossfeeds
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"crossfeed",
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_CONFIG,
		"modify crossfeeds"
	);
	register_command (c);
	register_command (new Command<CLI> (
		this,
		"delete",
		static_cast<callback_ptr> (&FG_CLI::cmd_crossfeed_delete),
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_CONFIG,
		"Delete crossfeeds"
	), c);
	register_command (new Command<CLI> (
		this,
		"add",
		static_cast<callback_ptr> (&FG_CLI::cmd_crossfeed_add),
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_CONFIG,
		"Add crossfeeds"
	), c);

	//////////////////////////////////////////////////
	// modify relays
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"relay",
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_CONFIG,
		"modify relays"
	);
	register_command (c);
	register_command (new Command<CLI> (
		this,
		"delete",
		static_cast<callback_ptr> (&FG_CLI::cmd_relay_delete),
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_CONFIG,
		"Delete relay"
	), c);
	register_command (new Command<CLI> (
		this,
		"add",
		static_cast<callback_ptr> (&FG_CLI::cmd_relay_add),
		LIBCLI::PRIVILEGED,
		LIBCLI::MODE_CONFIG,
		"Add relay"
	), c);

}

bool
FG_CLI::need_help
(
	char* argv
)
{
	if (! argv)
		return false;
	size_t l = strlen (argv);
	if (argv[l-1] == '?')
		return true;
	return false;
}

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
	difftime = now - fgms->m_Uptime;
	cmd_show_version (command, argv, argc);
	client << CRLF;
	client << "I have " << fgms->m_BlackList.Size () << " entries in my blacklist"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgms->m_CrossfeedList.Size () << " crossfeeds"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgms->m_RelayList.Size () << " relays"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgms->m_PlayerList.Size () << " users (" << fgms->m_NumMaxClients << " max)"
		<< CRLF; if (check_pager()) return 0;
	client << "Sent counters:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "to crossfeeds:"
		<< fgms->m_CrossfeedList.PktsSent << " packets"
		<< " (" << fgms->m_CrossfeedList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_CrossfeedList.BytesSent)
		<< " (" << byte_counter ((double) fgms->m_CrossfeedList.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "to relays:"
		<< fgms->m_RelayList.PktsSent << " packets"
		<< " (" << fgms->m_RelayList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_RelayList.BytesSent)
		<< " (" << byte_counter ((double) fgms->m_RelayList.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0; 
	client << "  " << left << setfill(' ') << setw(22)
		<< "to users:"
		<< fgms->m_PlayerList.PktsSent << " packets"
		<< " (" << fgms->m_PlayerList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_PlayerList.BytesSent)
		<< " (" << byte_counter ((double) fgms->m_PlayerList.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "Receive counters:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "errors:"
		<< "invalid packets:" << fgms->m_PacketsInvalid
		<< " rejected:" << fgms->m_BlackRejected
		<< " unknown relay:" << fgms->m_UnknownRelay
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "valid data:"
		<< "pos data:" << fgms->m_PositionData
		<< " other:" << fgms->m_NotPosData
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "tracker:"
		<< "connects:" << fgms->m_TrackerConnect
		<< " disconnects:" << fgms->m_TrackerDisconnect
		<< " positions:" << fgms->m_TrackerPosition
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "admin connections:" << fgms->m_AdminReceived
		<< CRLF; if (check_pager()) return 0;
	float telnet_per_second;
	if (fgms->m_TelnetReceived)
		telnet_per_second = (float) fgms->m_TelnetReceived / (time(0) - fgms->m_Uptime);
	else
		telnet_per_second = 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "telnet connections: "
		<< fgms->m_TelnetReceived
		<< " (" << setprecision(2) << telnet_per_second << " t/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "blacklist:"
		<< fgms->m_BlackList.PktsRcvd << " packets"
		<< " (" << fgms->m_BlackList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_BlackList.BytesRcvd)
		<< " (" << byte_counter ((double) fgms->m_BlackList.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "relays:"
		<< fgms->m_RelayList.PktsRcvd << " packets"
		<< " (" << fgms->m_RelayList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_RelayList.BytesRcvd)
		<< " (" << byte_counter ((double) fgms->m_RelayList.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "users:"
		<< fgms->m_PlayerList.PktsRcvd << " packets"
		<< " (" << fgms->m_PlayerList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_PlayerList.BytesRcvd)
		<< " (" << byte_counter ((double) fgms->m_PlayerList.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	accumulated_sent	+= fgms->m_CrossfeedList.BytesSent;
	accumulated_sent_pkts	+= fgms->m_CrossfeedList.PktsSent;
	accumulated_sent	+= fgms->m_RelayList.BytesSent;
	accumulated_sent_pkts	+= fgms->m_RelayList.PktsSent;
	accumulated_sent	+= fgms->m_PlayerList.BytesSent;
	accumulated_sent_pkts	+= fgms->m_PlayerList.PktsSent;
	accumulated_rcvd	+= fgms->m_BlackList.BytesRcvd;
	accumulated_rcvd_pkts	+= fgms->m_BlackList.PktsRcvd;
	accumulated_rcvd	+= fgms->m_RelayList.BytesRcvd;
	accumulated_rcvd_pkts	+= fgms->m_RelayList.PktsRcvd;
	accumulated_rcvd	+= fgms->m_PlayerList.BytesRcvd;
	accumulated_rcvd_pkts	+= fgms->m_PlayerList.PktsRcvd;
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
}

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
	fgms->m_WantExit = true;
	return LIBCLI::QUIT;
}

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
	client << "UP since " << timestamp_to_datestr(fgms->m_Uptime)
		<< "(" << timestamp_to_days(fgms->m_Uptime) << ")" << CRLF;
	return (0);
}

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
	if (fgms->m_IamHUB)
		s = "HUB";
	else
		s = "LEAVE";
	client << "This is " << fgms->m_ServerName << CRLF;
	client << "FlightGear Multiplayer " << s << " Server version " << VERSION << CRLF; 
	client << "using protocol version v"
		<< fgms->m_ProtoMajorVersion << "." << fgms->m_ProtoMinorVersion << CRLF;
	if (fgms->m_IsTracked)
		client << "This server is tracked: " << fgms->m_Tracker->GetTrackerServer() << CRLF;
	else
		client << "This server is NOT tracked" << CRLF;
	cmd_show_uptime (command, argv, argc);
	return (0);
}

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
	netAddress	Address ("0.0.0.0", 0);
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
				Address.set (argv[0], 0);
				if (Address.getIP() == 0)
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
			break;
		}
	}
	int Count = fgms->m_BlackList.Size ();
	FG_ListElement Entry("");
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_Uptime;
	client << fgms->m_BlackList.Name << ":" << CRLF;
	client << CRLF;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_BlackList[i];
		if ( (ID == 0) && (Address.getIP() != 0) )
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
			<< Entry.Address.getHost() << " : " << Entry.Name
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
		client << CRLF; if (check_pager()) return 0;
	client << EntriesFound << " entries found" << CRLF; if (check_pager()) return 0;
	if (EntriesFound)
	{
		client << "Total rcvd: "
			<< fgms->m_BlackList.PktsRcvd << " packets"
			<< " (" << fgms->m_BlackList.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_BlackList.BytesRcvd)
			<< " (" << byte_counter ((double) (fgms->m_BlackList.BytesRcvd/difftime)) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	return 0;
}

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
	netAddress	Address;
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
				Address.set (argv[0], 0);
				if (Address.getIP() == 0)
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
			break;
		}
	}
	if ( (ID == 0) && (Address.getIP() == 0) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (ID == 0) && (Address.getIP() != 0) )
	{	// match IP
		Entry = fgms->m_BlackList.Find (Address, "");
		if (Entry != fgms->m_BlackList.End())
		{
			fgms->m_BlackList.Delete (Entry);
		}
		else
		{
			client << "no entry found!" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_BlackList.FindByID (ID);
	if (Entry != fgms->m_BlackList.End())
	{
		fgms->m_BlackList.Delete (Entry);
	}
	else
	{
		client << "no entry found!" << CRLF;
		return 1;
	}
	client << "deleted!" << CRLF;
	return 0;
}

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
	netAddress	Address;
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
			Address.set (argv[i], 0);
			if (Address.getIP() == 0)
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
	FG_ListElement E (Reason);
	E.Address = Address;
	size_t NewID;
	ItList CurrentEntry = fgms->m_BlackList.Find ( E.Address, "" );
	if ( CurrentEntry == fgms->m_BlackList.End() )
	{       
		NewID = fgms->m_BlackList.Add (E, TTL);
	}
	else
	{
		client << "% entry already exists (ID " << CurrentEntry->ID << ")!" << CRLF;
		return 1;
	}
	client << "added with ID " << NewID << CRLF;
	return (0);
}

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
	netAddress	Address;
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
				Address.set (argv[0], 0);
				if (Address.getIP() == 0)
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
			break;
		}
	}
	if ( (ID == 0) && (Address.getIP() == 0) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (ID == 0) && (Address.getIP() != 0) )
	{	// match IP
		Entry = fgms->m_CrossfeedList.Find (Address, "");
		if (Entry != fgms->m_CrossfeedList.End())
		{
			fgms->m_CrossfeedList.Delete (Entry);
		}
		else
		{
			client << "no entry found" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_CrossfeedList.FindByID (ID);
	if (Entry != fgms->m_CrossfeedList.End())
	{
		fgms->m_CrossfeedList.Delete (Entry);
	}
	else
	{
		client << "no entry found" << CRLF;
		return 1;
	}
	client << "deleted" << CRLF;
	return 0;
}

//////////////////////////////////////////////////
/**
 *  @brief Add Crossfeed entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  crossfeed add ?
 *  crossfeed add IP-Address Name
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
	netAddress	Address;
	string		Name;
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
			Address.set (argv[i], 0);
			if (Address.getIP() == 0)
			{
				client << "% invalid IP address" << CRLF;
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
	FG_ListElement E (Name);
	E.Address = Address;
	size_t NewID;
	ItList CurrentEntry = fgms->m_CrossfeedList.Find ( E.Address, "" );
	if ( CurrentEntry == fgms->m_CrossfeedList.End() )
	{       
		NewID = fgms->m_CrossfeedList.Add (E, 0);
	}
	else
	{
		client << "entry already exists (ID " << CurrentEntry->ID << ")" << CRLF;
		return 1;
	}
	client << "added with ID " << NewID << CRLF;
	return (0);
}

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
	netAddress	Address ("0.0.0.0", 0);
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
				Address.set (argv[0], 0);
				if (Address.getIP() == 0)
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
			break;
		}
	}
	int Count = fgms->m_CrossfeedList.Size ();
	FG_ListElement Entry("");
	client << fgms->m_CrossfeedList.Name << ":" << CRLF;
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_Uptime;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_CrossfeedList[i];
		if ( (ID == 0) && (Address.getIP() != 0) )
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
			<< Entry.Address.getHost() << " : " << Entry.Name
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
		client << CRLF; if (check_pager()) return 0;
	client << EntriesFound << " entries found" << CRLF; if (check_pager()) return 0;
	if (EntriesFound)
	{
		client << "Total sent: "
			<< fgms->m_CrossfeedList.PktsSent << " packets"
			<< "(" << fgms->m_CrossfeedList.PktsSent / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_CrossfeedList.BytesSent)
			<< "(" << byte_counter ((double) (fgms->m_CrossfeedList.BytesSent/difftime)) << "/s)"
			<< CRLF;
	}
	return 0;
}

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
	netAddress	Address ("0.0.0.0", 0);
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
				Address.set (argv[0], 0);
				if (Address.getIP() == 0)
				{
					Name = argv[0];
#if 0
			//		client << "% invalid IP address" << CRLF;
			//		return (1);
#endif
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
			break;
		}
	}
	int Count = fgms->m_RelayList.Size ();
	FG_ListElement Entry("");
	client << fgms->m_RelayList.Name << ":" << CRLF;
	client << CRLF;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_Uptime;
	for (int i = 0; i < Count; i++)
	{
		Entry = fgms->m_RelayList[i];
		if ( (ID == 0) && (Address.getIP() != 0) )
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
			<< Entry.Address.getHost() << " : " << Entry.Name
			<< CRLF; if (check_pager()) return 0;
		if (Brief == true)
		{
			continue;
		}
		string A = timestamp_to_datestr (Entry.JoinTime);
		string B = "NEVER";
		if (Entry.JoinTime != Entry.LastSeen)
		{
			B = timestamp_to_days    (Entry.LastSeen);
			B += " ago";
		}
		client << "  entered   : " << timestamp_to_datestr (Entry.JoinTime)
			<< CRLF; if (check_pager()) return 0;
		client << "  last seen : " << timestamp_to_datestr (Entry.JoinTime)
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
		<< fgms->m_RelayList.PktsSent << " packets"
		<< " (" << fgms->m_RelayList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_RelayList.BytesSent)
		<< " (" << byte_counter ((double) fgms->m_RelayList.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  received  : "
		<< fgms->m_RelayList.PktsRcvd << " packets"
		<< " (" << fgms->m_RelayList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgms->m_RelayList.BytesRcvd)
		<< " (" << byte_counter ((double) fgms->m_RelayList.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	return 0;
}

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
			else
				client << "% invalid argument" << CRLF;
			break;
		default:
			client << "% invalid argument" << CRLF;
			break;
		}
	}
	if (! fgms->m_IsTracked)
	{
		client << "This server is NOT tracked" << CRLF;
		client << CRLF;
		return 0;
	}
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_Uptime;
	client << "This server is tracked: "
		<< fgms->m_Tracker->GetTrackerServer() << ":"
		<< fgms->m_Tracker->GetTrackerPort()
		<< CRLF;
	if (fgms->m_Tracker->m_connected)
	{
		client << "state: connected since "
			<< timestamp_to_datestr(fgms->m_Tracker->LastConnected)
			<< " (" << timestamp_to_days (fgms->m_Tracker->LastConnected) << " ago)"
			<< CRLF;
	}
	else
	{
		client << "state: NOT connected!" << CRLF;
	}
	string A = "NEVER";
	if (fgms->m_Tracker->LastSeen != 0)
	{
		A = timestamp_to_days (fgms->m_Tracker->LastSeen);
		A += " ago";
	}
	string B = "NEVER";
	if (fgms->m_Tracker->LastSent != 0)
	{
		B = timestamp_to_days (fgms->m_Tracker->LastSent);
		B += " ago";
	}
	client << "last seen " << A << ", last sent " << B << CRLF;
	client << "I had " << fgms->m_Tracker->LostConnections << " lost connections" << CRLF;
	client << CRLF;
	client << "Counters:" << CRLF;
	client << "  sent    : " << fgms->m_Tracker->PktsSent << " packets";
	client << " (" << fgms->m_Tracker->PktsSent / difftime << "/s)";
	client << " / " << byte_counter (fgms->m_Tracker->BytesSent);
	client << " (" << byte_counter ((double) fgms->m_Tracker->BytesSent / difftime) << "/s)";
	client << CRLF;
	client << "  received: " << fgms->m_Tracker->PktsRcvd << " packets";
	client << " (" << fgms->m_Tracker->PktsRcvd / difftime << "/s)";
	client << " / " << byte_counter (fgms->m_Tracker->BytesRcvd);
	client << " (" << byte_counter ((double) fgms->m_Tracker->BytesRcvd / difftime) << "/s)";
	client << CRLF;
	client << "  queue size: " << fgms->m_Tracker->msg_queue.size() << " messages" << CRLF;
	return 0;
}

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
	netAddress	Address;
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
				Address.set (argv[0], 0);
				if (Address.getIP() == 0)
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
			break;
		}
	}
	if ( (ID == 0) && (Address.getIP() == 0) )
	{
		client << "% missing argument" << CRLF;
		return 1;
	}
	if ( (ID == 0) && (Address.getIP() != 0) )
	{	// match IP
		Entry = fgms->m_RelayList.Find (Address, "");
		if (Entry != fgms->m_RelayList.End())
		{
			fgms->m_RelayList.Delete (Entry);
		}
		else
		{
			client << "no entry found" << CRLF;
			return 1;
		}
		return 0;
	}
	Entry = fgms->m_RelayList.FindByID (ID);
	if (Entry != fgms->m_RelayList.End())
	{
		fgms->m_RelayList.Delete (Entry);
	}
	else
	{
		client << "no entry found" << CRLF;
		return 1;
	}
	client << "deleted" << CRLF;
	return 0;
}

//////////////////////////////////////////////////
/**
 *  @brief Add Relay entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  relay add ?
 *  relay add IP-Address [Name]
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
	netAddress	Address;
	string		Name;
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
			Address.set (argv[i], 0);
			if (Address.getIP() == 0)
			{
				client << "% invalid IP address" << CRLF;
				return (1);
			}
			break;
		default:
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
	FG_ListElement E (Name);
	E.Address = Address;
	size_t NewID;
	ItList CurrentEntry = fgms->m_RelayList.Find ( E.Address, "" );
	if ( CurrentEntry == fgms->m_RelayList.End() )
	{       
		NewID = fgms->m_RelayList.Add (E, 0);
	}
	else
	{
		client << "entry already exists (ID " << CurrentEntry->ID << CRLF;
		return 1;
	}
	client << "added with ID " << NewID << CRLF;
	return (0);
}

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
	netAddress	Address ("0.0.0.0", 0);
	string		Name;
	bool		Brief = false;
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
				Address.set (argv[0], 0);
				if (Address.getIP() == 0)
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
			break;
		}
	}
	int Count = fgms->m_PlayerList.Size ();
	FG_Player	Player;
	Point3D		PlayerPosGeod;
	string		Origin;
	string		FullName;
	client << fgms->m_PlayerList.Name << ":" << CRLF;
	client << CRLF;
	for (int i = 0; i < Count; i++)
	{
		now = time(0);
		difftime = now - fgms->m_Uptime;
		Player = fgms->m_PlayerList[i];
		if ( (ID == 0) && (Address.getIP() != 0) )
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
		sgCartToGeod ( Player.LastPos, PlayerPosGeod );
		if (Player.IsLocal)
		{
			Origin = "LOCAL";
		}
		else
		{
			FG_SERVER::mT_RelayMapIt Relay = fgms->m_RelayMap.find ( Player.Address.getIP() );
			if ( Relay != fgms->m_RelayMap.end() )
			{
				Origin = Relay->second;
			}
			else
			{
				Origin = Player.Origin;
			}
		}
		FullName = Player.Name + string("@") + Origin;
		client << "ID " << Player.ID << ": "
			<< FullName << " entered: "
			<< timestamp_to_days (Player.JoinTime) << " ago"
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
		int expires = Player.Timeout - (now - Player.LastSeen);
		client << "         " << left << setfill(' ') << setw(15)
			<< "joined" << timestamp_to_datestr(Player.JoinTime)
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "last seen" << timestamp_to_datestr(Player.LastSeen)
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "using model" << Player.ModelName
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "real origin" << Player.Origin
			<< CRLF; if (check_pager()) return 0;
		if (Player.IsLocal)
		{
			client << "         " << left << setfill(' ') << setw(15)
				<< "sent" << Player.PktsSent << " packets "
				<< "(" << Player.PktsSent / difftime << "/s)"
				<< " / " << byte_counter (Player.BytesSent)
				<< " (" << byte_counter ((double) Player.BytesSent / difftime) << "/s)"
				<< CRLF; if (check_pager()) return 0;
			client << "         " << left << setfill(' ') << setw(15)
				<< "rcvd" << Player.PktsRcvd << " packets "
				<< "(" << Player.PktsRcvd / difftime << "/s)"
				<< " / " << byte_counter (Player.BytesRcvd)
				<< " (" << byte_counter ((double) Player.BytesRcvd / difftime) << "/s)"
				<< CRLF; if (check_pager()) return 0;
		}
		client << "         " << left << setfill(' ') << setw(15)
			<< "expires in" << expires
			<< CRLF; if (check_pager()) return 0;
		client << "         " << left << setfill(' ') << setw(15)
			<< "inactive" << now - Player.LastRelayedToInactive
			<< CRLF; if (check_pager()) return 0;
	}
	difftime = now - fgms->m_Uptime;
	client << CRLF;
	client << EntriesFound << " entries found" << CRLF;
	if (! Brief)
	{
		client << "Totals:" << CRLF; if (check_pager()) return 0;
		client << "          sent    : " << fgms->m_PlayerList.PktsSent << " packets"
			<< " (" << fgms->m_PlayerList.PktsSent / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_PlayerList.BytesSent)
			<< " (" << byte_counter ((double) fgms->m_PlayerList.BytesSent / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
		client << "          received: " << fgms->m_PlayerList.PktsRcvd << " packets"
			<< " (" << fgms->m_PlayerList.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter (fgms->m_PlayerList.BytesRcvd) 
			<< " (" << byte_counter ((double) fgms->m_PlayerList.BytesRcvd / difftime) << "/s)"
			<< CRLF; if (check_pager()) return 0;
	}
	return 0;
}

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

