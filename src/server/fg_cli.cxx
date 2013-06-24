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

#include <typcnvt.hxx>
#include <fg_cli.hxx>
#include <fg_common.hxx>

FG_CLI::FG_CLI ( FG_SERVER* fgms )
{
	this->fgms = fgms;
	this->setup ();
}

void
FG_CLI::setup ()
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
		set_auth_callback( static_cast<auth_callback> (& FG_CLI::check_auth) );
	}
	if (fgms->m_AdminEnable != "")
	{
		set_enable_callback( static_cast<enable_callback> (& FG_CLI::check_enable) );  
	}
	//////////////////////////////////////////////////
	// general commands
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"show",
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"show system information"
	);
	register_command (c);

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
		"stats",
		static_cast<callback_ptr> (&FG_CLI::cmd_NOT_IMPLEMENTED),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show statistical information"
	), c);

	//////////////////////////////////////////////////
	// show/modify blacklist
	//////////////////////////////////////////////////
	c = new Command<CLI> (
		this,
		"blacklist",
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"show/modify blacklist"
	);
	register_command (c);

	register_command (new Command<CLI> (
		this,
		"show",
		static_cast<callback_ptr> (&FG_CLI::cmd_blacklist_show),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show entries in the blacklist"
	), c);

	register_command (new Command<CLI> (
		this,
		"delete",
		static_cast<callback_ptr> (&FG_CLI::cmd_blacklist_delete),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show entries in the blacklist"
	), c);

	register_command (new Command<CLI> (
		this,
		"add",
		static_cast<callback_ptr> (&FG_CLI::cmd_blacklist_add),
		LIBCLI::UNPRIVILEGED,
		LIBCLI::MODE_ANY,
		"Show entries in the blacklist"
	), c);

}
int
FG_CLI::check_auth
(
	const string& username,
	const string& password
)
{
	if (username != fgms->m_AdminUser)
		return LIBCLI::ERROR;
	if (password != fgms->m_AdminPass)
		return LIBCLI::ERROR;
	return LIBCLI::OK;
}

int
FG_CLI::check_enable
(
	const string& password
)
{
	return (password == fgms->m_AdminEnable);
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
	print ("using protocol version v%u.%u (LazyRelay enabled)\n", 
	  fgms->m_ProtoMajorVersion, fgms->m_ProtoMinorVersion);
	if (fgms->m_IsTracked)
		print ("This server is tracked: %s", fgms->m_Tracker->GetTrackerServer().c_str());
	else
		print ("This server is NOT tracked");
	print ("I have %lu entries in my blacklist", fgms->m_BlackList.Size ());
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
			error ("argv 0: '%s'", argv[0]);
			if (strcmp (argv[i], "?") == 0)
			{
				print (
					"  brief  show brief listing\n"
					"  ID     show entry with ID\n"
					"  IP     show entry with IP-Address\n"
					"  <cr>   show long listing\n"
					"  |      output modifier\n"
				);
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
			error ("argv 1: '%s'", argv[1]);
			if (strcmp (argv[1], "?") == 0)
			{
				print (
					"  brief  show brief listing\n"
					"  <cr>   show long listing\n"
					"  |      output modifier\n"
				);
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
	print (" ");
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
		print ("ID %lu: %s : %s", Entry.ID, Entry.Address.getHost().c_str(), Entry.Name.c_str ());
		if (Brief == true)
		{
			continue;
		}
		string A = timestamp_to_datestr (Entry.JoinTime);
		string B = timestamp_to_days    (Entry.LastSeen);
		string C = "NEVER";
		if (Entry.Timeout != 0)
			C = NumToStr (Entry.Timeout, 0) + " seconds";
		print ("  entered      : %s", A.c_str());
		print ("  last seen    : %sago", B.c_str());
		print ("  rcvd packets : %llu", Entry.PktsRcvdFrom);
		print ("  rcvd bytes   : %s", byte_counter (Entry.BytesRcvdFrom).c_str());
		print ("  expire in    : %s", C.c_str());
	}
	print ("\n%lu entries found", EntriesFound);
	print ("Total received: %llu packets %s",
	  fgms->m_BlackList.PktsRcvd,
	  byte_counter (fgms->m_BlackList.BytesRcvd).c_str());
	return 0;
}

//////////////////////////////////////////////////
/**
 *  @brief Show Blacklist
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
			if (strcmp (argv[i], "?") == 0)
			{
				print (
					"  ID     show entry with ID\n"
					"  IP     show entry with IP-Address\n"
					"  <cr>   show long listing\n"
					"  |      output modifier\n"
				);
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
 *  @brief Show Blacklist
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
			if (strcmp (argv[i], "?") == 0)
			{
				print (
					"  TTL    Timeout of the new entry in seconds\n"
				);
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
			if (strcmp (argv[i], "?") == 0)
			{
				print (
					"  IP     IP address which should be blacklisted\n"
				);
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
			if ( (i == 2) && (strcmp (argv[i], "?") == 0 ) )
			{
				print (
					"  string a reason for blacklisting this IP"
				);
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

