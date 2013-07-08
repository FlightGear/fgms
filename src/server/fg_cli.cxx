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
	FG_SERVER* fgms
)
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
		"\n"
		"------------------------------------------------\n"
		"FlightGear Multiplayer Server CLI\n"
		"------------------------------------------------\n"
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
		LIBCLI::MODE_ANY,
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
	int n;
	string	temp;
	time_t	difftime;
	time_t	now;
	if (argc > 0)
	{
		print ("<br>");
		return (0);
	}
	now = time(0);
	difftime = now - fgms->m_Uptime;
	cmd_show_version (command, argv, argc);
	n = print (" ");
	if (n) return 0;
	n = print ("I have %lu entries in my blacklist", fgms->m_BlackList.Size ());
	if (n) return 0;
	n = print ("I have %lu crossfeeds", fgms->m_CrossfeedList.Size ());
	if (n) return 0;
	n = print ("I have %lu relays", fgms->m_RelayList.Size ());
	if (n) return 0;
	n = print ("I have %lu users", fgms->m_PlayerList.Size ());
	if (n) return 0;

	n = print ("Sent counters:");
	if (n) return 0;

	temp = byte_counter ((double) fgms->m_CrossfeedList.BytesSent / difftime);
	n = print ("  %-20s %lu packets / %s (%s/s)",
	  "to crossfeeds:",
	  fgms->m_CrossfeedList.PktsSent,
	  byte_counter (fgms->m_CrossfeedList.BytesSent).c_str(),
	  temp.c_str());
	if (n) return 0;

	temp = byte_counter ((double) fgms->m_RelayList.BytesSent / difftime);
	n = print ("  %-20s %lu packets %s (%s/s)",
	  "to relays:",
	  fgms->m_RelayList.PktsSent,
	  byte_counter (fgms->m_RelayList.BytesSent).c_str(),
	  temp.c_str());
	if (n) return 0;

	n = print ("Receive counters:");
	if (n) return 0;
	n = print ("  %-20s %lu", "invalid packets", fgms->m_PacketsInvalid);
	if (n) return 0;
	n = print ("  %-20s %lu", "rejected:", fgms->m_BlackRejected);
	if (n) return 0;
	n = print ("  %-20s %lu", "unknown relay:", fgms->m_UnknownRelay);
	if (n) return 0;
	n = print ("  %-20s %lu", "pos data:", fgms->m_PositionData);
	if (n) return 0;
	n = print ("  %-20s %lu", "other data:", fgms->m_NotPosData);
	if (n) return 0;
	temp = byte_counter ((double) fgms->m_RelayList.BytesRcvd / difftime);
	n = print ("  %-20s %lu packets %s (%s/s)",
	  "From Relays:",
	  fgms->m_RelayList.PktsRcvd,
	  byte_counter (fgms->m_RelayList.BytesRcvd).c_str(),
	  temp.c_str());
	if (n) return 0;
	temp = byte_counter ((double) fgms->m_BlackList.BytesRcvd / difftime);
	n = print ("  %-20s %lu packets %s (%s/s)",
	  "Blacklist:",
	  fgms->m_BlackList.PktsRcvd,
	  byte_counter (fgms->m_BlackList.BytesRcvd).c_str(),
	  temp.c_str());
	if (n) return 0;

	float telnet_per_second;
	if (fgms->m_TelnetReceived)
		telnet_per_second = fgms->m_TelnetReceived / (time(0) - fgms->m_Uptime);
	else
		telnet_per_second = 0;
	n = print ("  %-20s %lu (%.2f t/sec)", "telnet connections:", fgms->m_TelnetReceived, telnet_per_second);
	if (n) return 0;
	n = print ("  %-20s %lu", "admin connections:", fgms->m_AdminReceived);
	if (n) return 0;
	n = print ("  %-20s %lu", "tracker connections:", fgms->m_TrackerConnect);
	if (n) return 0;
	n = print ("  %-20s %lu", "tracker disconnets:", fgms->m_TrackerDisconnect);
	if (n) return 0;
	n = print ("  %-20s %lu", "tracker positions:", fgms->m_TrackerPostion);

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
			print ("<br>");
		}
		return (0);
	}
	fgms->m_WantExit = true;
	CLI::internal_quit (command, argv, argc);
	return 0;
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
			print ("<br>");
		}
		return (0);
	}
	string A = timestamp_to_datestr (fgms->m_Uptime);
	string B = timestamp_to_days    (fgms->m_Uptime);
	print ("UP since %s (%s)", A.c_str(), B.c_str());
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
			print ("<br>");
		}
		return (0);
	}
	string s;
	if (fgms->m_IamHUB)
		s = "HUB";
	else
		s = "LEAVE";
	print ("This is %s", this->fgms->m_ServerName.c_str());
	print ("FlightGear Multiplayer %s Server version %s", s.c_str(), VERSION);
	print ("using protocol version v%u.%u (LazyRelay enabled)", 
	  fgms->m_ProtoMajorVersion, fgms->m_ProtoMinorVersion);
	print (" ");
	if (fgms->m_IsTracked)
		print ("This server is tracked: %s", fgms->m_Tracker->GetTrackerServer().c_str());
	else
		print ("This server is NOT tracked");
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
	int		n;
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
				print ("  %-20s %s", "brief", "show brief listing");
				print ("  %-20s %s", "ID",    "show entry with ID");
				print ("  %-20s %s", "IP",    "show entry with IP-Address");
				print ("  %-20s %s", "<cr>",  "show long listing");
				print ("  %-20s %s", "|",     "output modifier");
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
					error ("%% invalid IP address");
					return (1);
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				if (! Brief)
					print ("  %-20s %s", "brief", "show brief listing");
				print ("  %-20s %s", "<cr>",  "show long listing");
				print ("  %-20s %s", "|",     "output modifier");
				return (0);
			}
			else if (strncmp (argv[1], "brief", strlen (argv[0])) == 0)
			{
				Brief = true;
			}
			break;
		default:
			error ("%% invalid argument");
			break;
		}
	}
	int Count = fgms->m_BlackList.Size ();
	FG_ListElement Entry("");
	n = print (" ");
	if (n) return 0;
	time_t  difftime;
	time_t  now;
	now = time(0);
	difftime = now - fgms->m_Uptime;
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
		n = print ("ID %lu: %s : %s", Entry.ID, Entry.Address.getHost().c_str(), Entry.Name.c_str ());
		if (n) return 0;
		if (Brief == true)
		{
			continue;
		}
		string A = timestamp_to_datestr (Entry.JoinTime);
		string B = timestamp_to_days    (Entry.LastSeen);
		string C = "NEVER";
		if (Entry.Timeout != 0)
		{
			C = NumToStr (Entry.Timeout, 0) + " seconds";
			C += " ago";
		}
		n = print ("  entered      : %s", A.c_str());
		if (n) return 0;
		n = print ("  last seen    : %s", B.c_str());
		if (n) return 0;
		n = print ("  rcvd packets : %lu", Entry.PktsRcvd);
		if (n) return 0;
		n = print ("  rcvd bytes   : %s", byte_counter (Entry.BytesRcvd).c_str());
		if (n) return 0;
		n = print ("  expire in    : %s", C.c_str());
		if (n) return 0;
	}
	n = print (" ");
	if (n) return 0;
	n = print ("%lu entries found", EntriesFound);
	if (n) return 0;
	n = print ("Total rcvd: %lu packets (%lu/s) / %s (%s/s)",
	  fgms->m_BlackList.PktsSent,
	  fgms->m_BlackList.PktsSent / difftime,
	  byte_counter (fgms->m_BlackList.BytesSent).c_str(),
	  byte_counter ((double) (fgms->m_BlackList.BytesSent/difftime)).c_str());
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
				print ("  %-20s %s", "ID", "delete entry with ID");
				print ("  %-20s %s", "IP", "delete entry with IP address");
				return (0);
			}
			else if (ID == 0)
			{
				Address.set (argv[0], 0);
				if (Address.getIP() == 0)
				{
					error ("%% invalid IP address");
					return (1);
				}
			}
			break;
		case 1: // only '?'
			if ( need_help (argv[i]) )
			{
				print ("  %-20s %s", "<cr>", "delete entry");
				return 1;
			}
		default:
			error ("%% invalid argument");
			break;
		}
	}
	if ( (ID == 0) && (Address.getIP() == 0) )
	{
		error ("%% missing argument!");
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
			error ("no entry found!");
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
		error ("no entry found!");
		return 1;
	}
	error ("deleted");
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
				print ("  %-20s %s", "TTL", "Timeout of the new entry in seconds");
				return (0);
			}
			TTL  = StrToNum<size_t> ( argv[0], I );
			if (I)
			{
				error ("%% invalid TTL");
				return (1);
			}
			break;
		case 1: // IP or '?'
			if ( need_help (argv[i]) )
			{
				print ("  %-20s %s", "IP", "IP address which should be blacklisted");
				return (0);
			}
			Address.set (argv[i], 0);
			if (Address.getIP() == 0)
			{
				error ("%% invalid IP address");
				return (1);
			}
			break;
		default:
			if ( need_help (argv[i]) )
			{
				if (Reason == "")
					print ("  %-20s %s", "STRING", "a reason for blacklisting this IP");
				else
					print ("  %-20s %s", "<cr>", "add this IP");
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
		error ("entry already exists (ID %lu)!", CurrentEntry->ID);
		return 1;
	}
	error ("added with ID %lu", NewID);
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
				print ("  %-20s %s", "ID", "delete entry with ID");
				print ("  %-20s %s", "IP", "delete entry with IP address");
				return (0);
			}
			else if (ID == 0)
			{
				Address.set (argv[0], 0);
				if (Address.getIP() == 0)
				{
					error ("%% invalid IP address");
					return (1);
				}
			}
			break;
		case 1: // only '?'
			if ( need_help (argv[i]) )
			{
				print ("  %-20s %s", "<cr>", "delete this crossfeed");
				return 1;
			}
		default:
			error ("%% invalid argument");
			break;
		}
	}
	if ( (ID == 0) && (Address.getIP() == 0) )
	{
		error ("%% missing argument!");
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
			error ("no entry found!");
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
		error ("no entry found!");
		return 1;
	}
	error ("deleted");
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
				print ("  %-20s %s", "IP",    "IP address of the crossfeed");
				return (0);
			}
			Address.set (argv[i], 0);
			if (Address.getIP() == 0)
			{
				error ("%% invalid IP address");
				return (1);
			}
			break;
		default:
			if ( need_help (argv[i]) )
			{
				if (Name == "")
					print ("  %-20s %s", "NAME", "The name of this crossfeed");
				else
					print ("  %-20s %s", "<cr>", "add this crossfeed");
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
		error ("entry already exists (ID %lu)!", CurrentEntry->ID);
		return 1;
	}
	error ("added with ID %lu", NewID);
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
	int		n;
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
				print ("  %-20s %s", "brief", "show brief listing");
				print ("  %-20s %s", "ID",    "show entry with ID");
				print ("  %-20s %s", "IP",    "show entry with IP-Address");
				print ("  %-20s %s", "<cr>",  "show long listing");
				print ("  %-20s %s", "|",     "output modifier");
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
					error ("%% invalid IP address");
					return (1);
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( need_help (argv[i]) )
			{
				if (! Brief)
					print ("  %-20s %s", "brief", "show brief listing");
				print ("  %-20s %s", "<cr>",  "show long listing");
				print ("  %-20s %s", "|",     "output modifier");
				return (0);
			}
			else if (strncmp (argv[1], "brief", strlen (argv[0])) == 0)
			{
				Brief = true;
			}
			break;
		default:
			error ("%% invalid argument");
			break;
		}
	}
	int Count = fgms->m_CrossfeedList.Size ();
	FG_ListElement Entry("");
	print ("%s : ", fgms->m_CrossfeedList.Name.c_str());
	print (" ");
	string  temp;
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
		n = print ("ID %lu: %s : %s", Entry.ID, Entry.Address.getHost().c_str(), Entry.Name.c_str ());
		if (n) return 0;
		if (Brief == true)
		{
			continue;
		}
		string A = timestamp_to_datestr (Entry.JoinTime);
		string B = timestamp_to_days    (Entry.LastSent);
		n = print ("  entered      : %s", A.c_str());
		if (n) return 0;
		n = print ("  last sent    : %s ago", B.c_str());
		if (n) return 0;
		n = print ("  sent packets : %lu (%f packets/s)",
			Entry.PktsSent,
			(double) (Entry.PktsSent / difftime));
		if (n) return 0;
		temp = byte_counter ((double) Entry.BytesSent / difftime);
		n = print ("  sent bytes   : %s (%s/s)",
			byte_counter (Entry.BytesSent).c_str(),
			temp.c_str());
		if (n) return 0;
	}
	n = print (" ");
	if (n) return 0;
	n = print ("%lu entries found", EntriesFound);
	if (n) return 0;
	n = print ("Total sent: %lu packets (%lu/s) / %s (%s/s)",
	  fgms->m_CrossfeedList.PktsSent,
	  fgms->m_CrossfeedList.PktsSent / difftime,
	  byte_counter (fgms->m_CrossfeedList.BytesSent).c_str(),
	  byte_counter ((double) (fgms->m_CrossfeedList.BytesSent/difftime)).c_str());
	if (n) return 0;
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
	int		n;
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
					print ("  %-20s %s", "brief", "show brief listing");
					print ("  %-20s %s", "ID",    "show entry with ID");
					print ("  %-20s %s", "IP",    "show entry with IP-Address");
					print ("  %-20s %s", "<cr>",  "show long listing");
					print ("  %-20s %s", "|",     "output modifier");
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
						error ("%% invalid IP address");
						return (1);
					}
				}
				break;
			case 1: // 'brief' or '?'
				if ( need_help (argv[i]) )
				{
					if (! Brief)
						print ("  %-20s %s", "brief", "show brief listing");
					print ("  %-20s %s", "<cr>",  "show long listing");
					print ("  %-20s %s", "|",     "output modifier");
					return (0);
				}
				else if (strncmp (argv[1], "brief", strlen (argv[0])) == 0)
				{
					Brief = true;
				}
				break;
			default:
				error ("%% invalid argument");
				break;
		}
	}
	int Count = fgms->m_RelayList.Size ();
	FG_ListElement Entry("");
	print ("%s : ", fgms->m_RelayList.Name.c_str());
	print (" ");
	string  temp;
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
		EntriesFound++;
		n = print ("ID %lu: %s : %s", Entry.ID, Entry.Address.getHost().c_str(), Entry.Name.c_str ());
		if (n) return 0;
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
		n = print ("  %-15s: %s", "entered", A.c_str());
		if (n) return 0;
		n = print ("  %-15s: %s", "last seen", B.c_str());
		if (n) return 0;
		temp = byte_counter ((double) Entry.BytesSent / difftime);
		n = print ("  %-15s: %lu packets (%lu/s) / %s (%s/s)", "sent",
			Entry.PktsSent,
			Entry.PktsSent / difftime,
			byte_counter (Entry.BytesSent).c_str(),
			temp.c_str()
			);
		if (n) return 0;
		temp = byte_counter ((double) Entry.BytesRcvd / difftime);
		n = print ("  %-15s: %lu packets (%lu/s) / %s (%s/s)", "rcvd",
			Entry.PktsRcvd,
			Entry.PktsRcvd / difftime,
			byte_counter (Entry.BytesRcvd).c_str(),
			temp.c_str());
		if (n) return 0;
	}
	print ("\n%lu entries found", EntriesFound);
	if (! Brief)
	{
		n = print ("Totals:");
		if (n) return 0;
		temp = byte_counter ((double) fgms->m_RelayList.BytesSent / difftime);
		n = print ("  sent    : %lu packets (%lu/s) / %s (%s/s)",
			fgms->m_RelayList.PktsSent,
			fgms->m_RelayList.PktsSent / difftime,
			byte_counter (fgms->m_RelayList.BytesSent).c_str(),
			temp.c_str());
		if (n) return 0;
		temp = byte_counter ((double) fgms->m_RelayList.BytesRcvd / difftime);
		n = print ("  received: %lu packets (%lu/s) / %s (%s/s)",
			fgms->m_RelayList.PktsRcvd,
			fgms->m_RelayList.PktsRcvd / difftime,
			byte_counter (fgms->m_RelayList.BytesRcvd).c_str(),
			temp.c_str());
		if (n) return 0;
	}
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
					print ("  %-20s %s", "ID", "delete entry with ID");
					print ("  %-20s %s", "IP", "delete entry with IP address");
					return (0);
				}
				else if (ID == 0)
				{
					Address.set (argv[0], 0);
					if (Address.getIP() == 0)
					{
						error ("%% invalid IP address");
						return (1);
					}
				}
				break;
			case 1: // only '?'
				if ( need_help (argv[i]) )
				{
					print ("  %-20s %s", "<cr>", "delete entry");
					return 1;
				}
			default:
				error ("%% invalid argument");
				break;
		}
	}
	if ( (ID == 0) && (Address.getIP() == 0) )
	{
		error ("%% missing argument!");
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
			error ("no entry found!");
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
		error ("no entry found!");
		return 1;
	}
	error ("deleted");
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
					print ("  %-20s %s", "IP", "IP address of the relay");
					return (0);
				}
				Address.set (argv[i], 0);
				if (Address.getIP() == 0)
				{
					error ("%% invalid IP address");
					return (1);
				}
				break;
			default:
				if ( need_help (argv[i]) )
				{
					if (Name == "")
						print ("  %-20s %s", "NAME", "the name of this relay");
					else
						print ("  %-20s %s", "<cr>", "add this relay");
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
		error ("entry already exists (ID %lu)!", CurrentEntry->ID);
		return 1;
	}
	error ("added with ID %lu", NewID);
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
	int		n;
	time_t		difftime;
	time_t		now;
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
					print ("  %-20s %s", "brief", "show brief listing");
					print ("  %-20s %s", "ID",    "show entry with ID");
					print ("  %-20s %s", "IP",    "show entry with IP-Address");
					print ("  %-20s %s", "NAME",  "show entry with IP-Address");
					print ("  %-20s %s", "<cr>",  "show long listing");
					print ("  %-20s %s", "|",     "output modifier");
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
						print ("  %-20s %s", "brief", "show brief listing");
					print ("  %-20s %s", "<cr>",  "show long listing");
					print ("  %-20s %s", "|",     "output modifier");
					return (0);
				}
				else if (strncmp (argv[1], "brief", strlen (argv[0])) == 0)
				{
					Brief = true;
				}
				break;
			default:
				error ("%% invalid argument");
				break;
		}
	}
	string temp;
	int Count = fgms->m_PlayerList.Size ();
	FG_Player	Player;
	Point3D		PlayerPosGeod;
	string		Origin;
	string		FullName;
	print ("%s : ", fgms->m_PlayerList.Name.c_str());
	print (" ");
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
			if (Player.Name != Name)
				continue;
		}
		EntriesFound++;
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
		string A = timestamp_to_days (Player.JoinTime);
		n = print ("%-20s (ID %lu) entered: %s ago",
			FullName.c_str (),
			Player.ID,
			A.c_str()
			);
		if (n) return 0;
		if (Brief == true)
		{
			continue;
		}
		if (Player.HasErrors == true)
		{
			n = print ("         %-15s: %s",  "ERROR", Player.Error.c_str());
			if (n) return 0;
		}
		int expires = Player.Timeout - (now - Player.LastSeen);
		n = print ("          %-15s: %s",
			"joined",
			timestamp_to_datestr(Player.JoinTime).c_str());
		if (n) return 0;
		n = print ("          %-15s: %s",
			"last seen",
			timestamp_to_datestr(Player.LastSeen).c_str());
		if (n) return 0;
		n = print ("          %-15s: %u seconds",
			"exprires in",
			expires);
		if (n) return 0;
		n = print ("          %-15s: %s",
			"using model",
			Player.ModelName.c_str());
		if (n) return 0;
		n = print ("          %-15s: %s",
			"real origin",
			Player.Origin.c_str());
		if (n) return 0;

		temp = byte_counter ((double) Player.BytesSent / difftime);
		n = print ("          %-15s: %lu packets (%lu/s) / %s (%s/s)",
			"sent",
			Player.PktsSent,
			Player.PktsSent / difftime,
			byte_counter (Player.BytesSent).c_str(),
			temp.c_str());
		if (n) return 0;
		temp = byte_counter ((double) Player.BytesRcvd / difftime);
		n = print ("          %-15s: %lu packets (%lu/s) / %s (%s/s)",
			"rcvd",
			Player.PktsRcvd,
			Player.PktsRcvd / difftime,
			byte_counter (Player.BytesRcvd).c_str(),
			temp.c_str());
		if (n) return 0;
		n = print ("          %-15s: %lu",
			"inactive",
			now - Player.LastRelayedToInactive);
		if (n) return 0;

#if 0
	// do we really want to see the position in the admin interface?
		n = print ("  %-15s: %.6f %.6f %.6f", "last pos (centerd)",
			Player.LastPos[X],
			Player.LastPos[Y],
			Player.LastPos[Z]
			); if (n) return 0;
		n = print ("  %-15s: %.6f %.6f %.6f", "last pos (geod.)",
			PlayerPosGeod[Lat],
			PlayerPosGeod[Lon],
			PlayerPosGeod[Alt]
			); if (n) return 0;
#endif

	}
	difftime = now - fgms->m_Uptime;
	print ("\n%lu entries found", EntriesFound);
	if (! Brief)
	{
		n = print ("Totals:");
		if (n) return 0;
		temp = byte_counter ((double) fgms->m_PlayerList.BytesSent / difftime);
		n = print ("          sent    : %lu packets (%lu/s) / %s (%s/s)",
			fgms->m_PlayerList.PktsSent,
			fgms->m_PlayerList.PktsSent / difftime,
			byte_counter (fgms->m_PlayerList.BytesSent).c_str(),
			temp.c_str());
		if (n) return 0;
		temp = byte_counter ((double) fgms->m_PlayerList.BytesRcvd / difftime);
		n = print ("          received: %lu packets (%lu/s) / %s (%s/s)",
			fgms->m_PlayerList.PktsRcvd,
			fgms->m_PlayerList.PktsRcvd / difftime,
			byte_counter (fgms->m_PlayerList.BytesRcvd).c_str(),
			temp.c_str());
		if (n) return 0;
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
	print ("Command '%s' NOT IMPLEMENTED yet!", command);
	if (argc > 0)
	{
		print ("  args:");
		for (int i=0; i<argc; i++)
			print ("  '%s'", argv[i]);
	}
	return (0);
}

