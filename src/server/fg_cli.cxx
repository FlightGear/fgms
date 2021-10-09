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

#include <sstream>
#include <fg_util.hxx>
#include <fg_cli.hxx>
#include <fg_common.hxx>

FG_CLI::FG_CLI
(
	FG_SERVER* fgms,
	int fd
) : cli ( fd )
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
	//////////////////////////////////////////////////
	// general setup
	//////////////////////////////////////////////////
	set_hostname ( this->fgms->m_ServerName.c_str () );
	std::stringstream banner;
	banner << "\r\n"
		<< "------------------------------------------------\r\n"
		<< "FlightGear Multiplayer Server CLI\r\n"
		<< "This is " << fgms->m_ServerName << " (" << fgms->m_FQDN << ")\r\n"
		<< "------------------------------------------------\r\n";
	set_banner ( banner.str () );
	//////////////////////////////////////////////////
	// setup authentication (if required)
	//////////////////////////////////////////////////
	if ( fgms->m_AdminUser != "" )
	{
		allow_user ( fgms->m_AdminUser.c_str (), fgms->m_AdminPass.c_str () );
	}
	if ( fgms->m_AdminEnable != "" )
	{
		allow_enable ( fgms->m_AdminEnable.c_str () );
	}
	using callback = RESULT ( cli::* ) ( const std::string& command, const libcli::tokens& args );
	//////////////////////////////////////////////////
	// general commands
	//////////////////////////////////////////////////
	//////////////////////////////////////////////////
	// 'show' command and its children
	//////////////////////////////////////////////////
	{
		command show_cmd (
			"show",
			PRIVLEVEL::UNPRIVILEGED,
			MODE::STANDARD,
			"show system information"
		);

		register_command ( show_cmd, command (
			this,
			"stats",
			static_cast< callback > ( &FG_CLI::cmd_show_stats ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show statistical information"
		) );

		register_command ( show_cmd, command (
			this,
			"settings",
			static_cast< callback > ( &FG_CLI::cmd_show_settings ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show general settings"
		) );

		register_command ( show_cmd, command (
			this,
			"version",
			static_cast< callback > ( &FG_CLI::cmd_show_version ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show running version information"
		) );

		register_command ( show_cmd, command (
			this,
			"uptime",
			static_cast< callback > ( &FG_CLI::cmd_show_uptime ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show uptime information"
		) );

		register_command ( show_cmd, command (
			this,
			"whitelist",
			static_cast< callback > ( &FG_CLI::cmd_whitelist_show ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show entries in the whitelist"
		) );

		register_command ( show_cmd, command (
			this,
			"blacklist",
			static_cast< callback > ( &FG_CLI::cmd_blacklist_show ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show entries in the blacklist"
		) );

		register_command ( show_cmd, command (
			this,
			"crossfeeds",
			static_cast< callback > ( &FG_CLI::cmd_crossfeed_show ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show entries in the crossfeeds"
		) );

		register_command ( show_cmd, command (
			this,
			"relay",
			static_cast< callback > ( &FG_CLI::cmd_relay_show ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show list of relays"
		) );

		register_command ( show_cmd, command (
			this,
			"tracker",
			static_cast< callback > ( &FG_CLI::cmd_tracker_show ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show status of tracker"
		) );

		register_command ( show_cmd, command (
			this,
			"users",
			static_cast< callback > ( &FG_CLI::cmd_user_show ),
			PRIVLEVEL::UNPRIVILEGED,
			MODE::ANY,
			"Show list of users"
		) );

		register_command ( show_cmd );
	} // 'show' command

	register_command ( command (
		this,
		"die",
		static_cast< callback > ( &FG_CLI::cmd_fgms_die ),
		PRIVLEVEL::PRIVILEGED,
		MODE::STANDARD,
		"force fgms to exit"
	) );

	//////////////////////////////////////////////////
	// 'blacklist' command
	//////////////////////////////////////////////////
	{
		command blacklist (
			"blacklist",
			PRIVLEVEL::PRIVILEGED,
			MODE::CONFIGURE,
			"show/modify blacklist"
		);

		register_command ( blacklist, command (
			this,
			"delete",
			static_cast< callback > ( &FG_CLI::cmd_blacklist_delete ),
			PRIVLEVEL::PRIVILEGED,
			MODE::CONFIGURE,
			"Show entries in the blacklist"
		) );

		register_command ( blacklist, command (
			this,
			"add",
			static_cast< callback > ( &FG_CLI::cmd_blacklist_add ),
			PRIVLEVEL::PRIVILEGED,
			MODE::CONFIGURE,
			"Show entries in the blacklist"
		) );

		register_command ( blacklist );
	} // 'blacklist' command

	//////////////////////////////////////////////////
	// 'crossfeeds' command
	//////////////////////////////////////////////////
	{
		command crossfeed (
			"crossfeed",
			PRIVLEVEL::PRIVILEGED,
			MODE::CONFIGURE,
			"modify crossfeeds"
		);

		register_command ( crossfeed, command (
			this,
			"delete",
			static_cast< callback > ( &FG_CLI::cmd_crossfeed_delete ),
			PRIVLEVEL::PRIVILEGED,
			MODE::CONFIGURE,
			"Delete crossfeeds"
		) );

		register_command ( crossfeed, command (
			this,
			"add",
			static_cast< callback > ( &FG_CLI::cmd_crossfeed_add ),
			PRIVLEVEL::PRIVILEGED,
			MODE::CONFIGURE,
			"Add crossfeeds"
		) );

		register_command ( crossfeed );
	} // 'crossfeed' command

	//////////////////////////////////////////////////
	// 'relay' command
	//////////////////////////////////////////////////
	{
		command relay (
			"relay",
			PRIVLEVEL::PRIVILEGED,
			MODE::CONFIGURE,
			"modify relays"
		);

		register_command ( relay, command (
			this,
			"delete",
			static_cast< callback > ( &FG_CLI::cmd_relay_delete ),
			PRIVLEVEL::PRIVILEGED,
			MODE::CONFIGURE,
			"Delete relay"
		) );

		register_command ( relay, command (
			this,
			"add",
			static_cast< callback > ( &FG_CLI::cmd_relay_add ),
			PRIVLEVEL::PRIVILEGED,
			MODE::CONFIGURE,
			"Add relay"
		) );

		register_command ( relay );
	} // 'relay' command

} // FG_CLI::setup ()

//////////////////////////////////////////////////
/**
 *  @brief Show general statistics
 */
RESULT
FG_CLI::cmd_show_stats
(
	const std::string& command,
	const libcli::tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	time_t	    difftime;
	time_t      now;
	uint64_t	accumulated_sent = 0;
	uint64_t	accumulated_rcvd = 0;
	uint64_t	accumulated_sent_pkts = 0;
	uint64_t	accumulated_rcvd_pkts = 0;

	now = time ( 0 );
	difftime = now - fgms->m_Uptime;
	cmd_show_version ( command, args );
	m_client << crlf;
	m_client << "I have " << fgms->m_BlackList.Size ()
		<< " entries in my blacklist"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "I have " << fgms->m_CrossfeedList.Size () << " crossfeeds"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "I have " << fgms->m_RelayList.Size () << " relays"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "I have " << fgms->m_PlayerList.Size () << " users ("
		<< fgms->m_LocalClients << " local, "
		<< fgms->m_RemoteClients << " remote, "
		<< fgms->m_NumMaxClients << " max)"
		<< crlf; if ( check_pager () ) return libcli::OK;

	m_client << "Sent counters:" << crlf; if ( check_pager () ) return OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "to crossfeeds:"
		<< fgms->m_CrossfeedList.PktsSent << " packets"
		<< " (" << fgms->m_CrossfeedList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter ( fgms->m_CrossfeedList.BytesSent )
		<< " (" << byte_counter ( ( double ) fgms->m_CrossfeedList.BytesSent / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "to relays:"
		<< fgms->m_RelayList.PktsSent << " packets"
		<< " (" << fgms->m_RelayList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter ( fgms->m_RelayList.BytesSent )
		<< " (" << byte_counter ( ( double ) fgms->m_RelayList.BytesSent / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "to users:"
		<< fgms->m_PlayerList.PktsSent << " packets"
		<< " (" << fgms->m_PlayerList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter ( fgms->m_PlayerList.BytesSent )
		<< " (" << byte_counter ( ( double ) fgms->m_PlayerList.BytesSent / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;

	m_client << "Receive counters:" << crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "total:" << fgms->m_PacketsReceived
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "pings:" << fgms->m_PingReceived
		<< " (" << fgms->m_PongReceived << " pongs)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "errors:"
		<< "invalid packets:" << fgms->m_PacketsInvalid
		<< " rejected:" << fgms->m_BlackRejected
		<< " unknown relay:" << fgms->m_UnknownRelay
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "valid data:"
		<< "pos data:" << fgms->m_PositionData
		<< " other:" << fgms->m_UnkownMsgID
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "tracker:"
		<< "connects:" << fgms->m_TrackerConnect
		<< " disconnects:" << fgms->m_TrackerDisconnect
		<< " positions:" << fgms->m_TrackerPosition
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "admin connections:" << fgms->m_AdminReceived
		<< crlf; if ( check_pager () ) return libcli::OK;
	float telnet_per_second;
	if ( fgms->m_TelnetReceived )
		telnet_per_second = ( float ) fgms->m_TelnetReceived / ( time ( 0 ) - fgms->m_Uptime );
	else
		telnet_per_second = 0;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "telnet connections: "
		<< fgms->m_TelnetReceived
		<< " (" << std::setprecision ( 2 ) << telnet_per_second << " t/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "blacklist:"
		<< fgms->m_BlackList.PktsRcvd << " packets"
		<< " (" << fgms->m_BlackList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter ( fgms->m_BlackList.BytesRcvd )
		<< " (" << byte_counter ( ( double ) fgms->m_BlackList.BytesRcvd / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "relays:"
		<< fgms->m_RelayList.PktsRcvd << " packets"
		<< " (" << fgms->m_RelayList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter ( fgms->m_RelayList.BytesRcvd )
		<< " (" << byte_counter ( ( double ) fgms->m_RelayList.BytesRcvd / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "users:"
		<< fgms->m_PlayerList.PktsRcvd << " packets"
		<< " (" << fgms->m_PlayerList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter ( fgms->m_PlayerList.BytesRcvd )
		<< " (" << byte_counter ( ( double ) fgms->m_PlayerList.BytesRcvd / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	accumulated_sent += fgms->m_CrossfeedList.BytesSent;
	accumulated_sent_pkts += fgms->m_CrossfeedList.PktsSent;
	accumulated_sent += fgms->m_RelayList.BytesSent;
	accumulated_sent_pkts += fgms->m_RelayList.PktsSent;
	accumulated_sent += fgms->m_PlayerList.BytesSent;
	accumulated_sent_pkts += fgms->m_PlayerList.PktsSent;
	accumulated_rcvd += fgms->m_BlackList.BytesRcvd;
	accumulated_rcvd_pkts += fgms->m_BlackList.PktsRcvd;
	accumulated_rcvd += fgms->m_RelayList.BytesRcvd;
	accumulated_rcvd_pkts += fgms->m_RelayList.PktsRcvd;
	accumulated_rcvd += fgms->m_PlayerList.BytesRcvd;
	accumulated_rcvd_pkts += fgms->m_PlayerList.PktsRcvd;
	m_client << "Totals:" << crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "sent:"
		<< accumulated_sent_pkts << " packets"
		<< " (" << accumulated_sent_pkts / difftime << "/s)"
		<< " / " << byte_counter ( accumulated_sent )
		<< " (" << byte_counter ( ( double ) accumulated_sent / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "received:"
		<< accumulated_rcvd_pkts << " packets"
		<< " (" << accumulated_rcvd_pkts / difftime << "/s)"
		<< " / " << byte_counter ( accumulated_rcvd )
		<< " (" << byte_counter ( ( double ) accumulated_rcvd / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	return libcli::OK;
} // FG_CLI::cmd_show_stats ()

//////////////////////////////////////////////////
/**
 *  @brief Show general settings
 */
RESULT
FG_CLI::cmd_show_settings
(
	const std::string& command,
	const libcli::tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	std::string bind_addr;

	if ( fgms->m_BindAddress == "" )
		bind_addr = "*";
	else
		bind_addr = fgms->m_BindAddress;
	cmd_show_version ( command, args );
	m_client << crlf;
	m_client << "current settings:" << crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "listen port:" << fgms->m_ListenPort
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "telnet port:" << fgms->m_TelnetPort
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "admin port:" << fgms->m_AdminPort
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "player expires:" << fgms->m_PlayerExpires
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "out of reach:" << fgms->m_PlayerIsOutOfReach
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "radar range:" << fgms->m_MaxRadarRange
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "logfile:" << fgms->m_LogFileName
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "bind address:" << bind_addr
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
		<< "FQDN:" << fgms->m_FQDN
		<< crlf; if ( check_pager () ) return libcli::OK;

	return libcli::OK;
} // FG_CLI::cmd_show_settings ()

//////////////////////////////////////////////////
/**
 *  @brief Shutdown the server
 */
RESULT
FG_CLI::cmd_fgms_die
(
	const std::string& command,
	const libcli::tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	fgms->m_WantExit = true;
	return libcli::QUIT;
} // FG_CLI::cmd_fgms_die
//////////////////////////////////////////////////

//////////////////////////////////////////////////
/**
 *  @brief Show the uptime of the the server
 *         in a human readable form.
 */
RESULT
FG_CLI::cmd_show_uptime
(
	const std::string& command,
	const libcli::tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	m_client << "UP since " << timestamp_to_datestr ( fgms->m_Uptime )
		<< "(" << timestamp_to_days ( fgms->m_Uptime ) << ")" << crlf;
	return libcli::OK;
} // FG_CLI::cmd_show_uptime

//////////////////////////////////////////////////
/**
 *  @brief Show the version number of the the server
 */
RESULT
FG_CLI::cmd_show_version
(
	const std::string& command,
	const libcli::tokens& args
)
{
	RESULT r = have_unwanted_args ( args );
	if ( libcli::OK != r )
	{
		return r;
	}
	string s;
	if ( fgms->m_IamHUB )
		s = "HUB";
	else
		s = "LEAVE";
	m_client << "This is " << fgms->m_ServerName << " (" << fgms->m_FQDN << ")" << crlf;
	m_client << "FlightGear Multiplayer " << s << " Server version " << VERSION << crlf;
	m_client << "using protocol version v"
		<< fgms->m_ProtoMajorVersion << "." << fgms->m_ProtoMinorVersion << crlf;
	if ( fgms->m_IsTracked )
		m_client << "This server is tracked: " << fgms->m_Tracker->GetTrackerServer () << crlf;
	else
		m_client << "This server is NOT tracked" << crlf;
	cmd_show_uptime ( command, args );
	return libcli::OK;
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
RESULT
FG_CLI::cmd_whitelist_show
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	netAddress	Address ( "0.0.0.0", 0 );
	size_t		EntriesFound = 0;
	size_t i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "ID" << "show entry with ID" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "show entry with IP-Address" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show long listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			else
			{
				ID = StrToNum<size_t> ( a, ID_invalid );
				if ( ID_invalid )
				{
					ID = 0;
					Address.set ( a.c_str (), 0 );
					if ( Address.getIP () == 0 )
					{
						return libcli::INVALID_ARG;
					}
				}
			}
			break;
		case 1: // '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show long listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	int Count = fgms->m_WhiteList.Size ();
	FG_ListElement Entry ( "" );
	m_client << crlf;
	time_t  difftime;
	time_t  now;
	now = time ( 0 );
	m_client << fgms->m_WhiteList.Name << ":" << crlf;
	m_client << crlf;
	for ( int i = 0; i < Count; i++ )
	{
		Entry = fgms->m_WhiteList[i];
		if ( ( ID == 0 ) && ( Address.getIP () != 0 ) )
		{	// only list matching entries
			if ( Entry.Address != Address )
				continue;
		}
		else if ( ID )
		{
			if ( Entry.ID != ID )
				continue;
		}
		EntriesFound++;
		difftime = now - Entry.JoinTime;
		m_client << "ID " << Entry.ID << ": "
			<< Entry.Address.getHost () << " : " << Entry.Name
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  entered      : " << timestamp_to_datestr ( Entry.JoinTime )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  last seen    : " << timestamp_to_days ( Entry.LastSeen )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  rcvd packets : " << Entry.PktsRcvd
			<< " (" << Entry.PktsRcvd / difftime << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  rcvd bytes   : " << byte_counter ( Entry.BytesRcvd )
			<< " (" << Entry.BytesRcvd / difftime << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
	}
	if ( EntriesFound )
		m_client << crlf;
	if ( check_pager () ) return libcli::OK;
	m_client << EntriesFound << " entries found" << crlf; if ( check_pager () ) return libcli::OK;
	if ( EntriesFound )
	{
		difftime = now - fgms->m_Uptime;
		m_client << "Total rcvd: "
			<< fgms->m_WhiteList.PktsRcvd << " packets"
			<< " (" << fgms->m_WhiteList.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter ( fgms->m_WhiteList.BytesRcvd )
			<< " (" << byte_counter ( ( double ) ( fgms->m_WhiteList.BytesRcvd / difftime ) ) << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
	}
	return libcli::OK;
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
RESULT
FG_CLI::cmd_whitelist_delete
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t		ID = 0;
	int		    ID_invalid = -1;
	netAddress	Address;
	ItList		Entry;

	size_t i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "ID" << "delete entry with ID" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "delete entry with IP address" << crlf;
				return libcli::OK;
			}
			else
			{
				ID = StrToNum<size_t> ( a, ID_invalid );
				if ( ID_invalid )
				{
					ID = 0;
					Address.set ( a.c_str (), 0 );
					if ( Address.getIP () == 0 )
					{
						return libcli::INVALID_ARG;
					}
				}
			}
			break;
		case 1: // only '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "delete entry" << crlf;
				return libcli::OK;
			}
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	if ( ( ID == 0 ) && ( Address.getIP () == 0 ) )
	{
		m_client << "% missing argument" << crlf;
		return libcli::MISSING_ARG;
	}
	if ( ( ID == 0 ) && ( Address.getIP () != 0 ) )
	{	// match IP
		Entry = fgms->m_WhiteList.Find ( Address, "" );
		if ( Entry != fgms->m_WhiteList.End () )
		{
			fgms->m_WhiteList.Delete ( Entry );
		}
		else
		{
			m_client << "no entry found!" << crlf;
			return libcli::OK;
		}
		return libcli::OK;
	}
	Entry = fgms->m_WhiteList.FindByID ( ID );
	if ( Entry != fgms->m_WhiteList.End () )
	{
		fgms->m_WhiteList.Delete ( Entry );
	}
	else
	{
		m_client << "no entry found!" << crlf;
		return libcli::OK;
	}
	m_client << "deleted!" << crlf;
	return libcli::OK;
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
RESULT
FG_CLI::cmd_whitelist_add
(
	const std::string& command,
	const libcli::tokens& args
)
{
	time_t		TTL = -1;
	int			I;
	netAddress	Address;
	string		Reason;
	ItList		Entry;
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // must be TTL or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "TTL" << "Timeout of the new entry in seconds" << crlf;
				return libcli::OK;
			}
			TTL = StrToNum<size_t> ( a, I );
			if ( I )
			{
				m_client << "% '" << a << "' invalid TTL" << crlf;
				return libcli::ERROR_ARG;
			}
			break;
		case 1: // IP or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "IP address which should be whitelisted" << crlf;
				return libcli::OK;
			}
			Address.set ( a.c_str (), 0 );
			if ( Address.getIP () == 0 )
			{
				return libcli::INVALID_ARG;
			}
			break;
		default:
			if ( arg_wants_help ( a ) )
			{
				if ( Reason == "" )
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "STRING" << "a reason for whitelisting this IP" << crlf;
				}
				else
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "<cr>" << "add this IP" << crlf;
				}
				return libcli::OK;
			}
			Reason += a;
			if ( i + 1 < args.size () )
				Reason += " ";
			break;
		}
		++i;
	}
	FG_ListElement E ( Reason );
	E.Address = Address;
	size_t NewID;
	ItList CurrentEntry = fgms->m_WhiteList.Find ( E.Address, "" );
	if ( CurrentEntry == fgms->m_WhiteList.End () )
	{
		NewID = fgms->m_WhiteList.Add ( E, TTL );
	}
	else
	{
		m_client << "% entry already exists (ID " << CurrentEntry->ID << ")!" << crlf;
		return libcli::ERROR_ARG;
	}
	m_client << "added with ID " << NewID << crlf;
	return libcli::OK;
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
RESULT
FG_CLI::cmd_blacklist_show
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t		ID = 0;
	int			ID_invalid = -1;
	netAddress	Address ( "0.0.0.0", 0 );
	bool		Brief = false;
	size_t		EntriesFound = 0;
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "brief" << "show brief listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "ID" << "show entry with ID" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "show entry with IP-Address" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show long listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			else if ( strncmp ( a.c_str (), "brief", a.size () ) == 0 )
			{
				Brief = true;
			}
			else
			{
				ID = StrToNum<size_t> ( a, ID_invalid );
				if ( ID_invalid )
				{
					ID = 0;
					Address.set ( a.c_str (), 0 );
					if ( Address.getIP () == 0 )
					{
						return libcli::INVALID_ARG;
					}
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				if ( !Brief )
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "brief" << "show brief listing" << crlf;
				}
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show long listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			// FIXME: string("brief").rfind (a, 0)
			else if ( strncmp ( a.c_str (), "brief", a.length () ) == 0 )
			{
				Brief = true;
			}
			break;
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	int Count = fgms->m_BlackList.Size ();
	FG_ListElement Entry ( "" );
	m_client << crlf;
	time_t  difftime;
	time_t  now;
	now = time ( 0 );
	m_client << fgms->m_BlackList.Name << ":" << crlf;
	m_client << crlf;
	for ( int i = 0; i < Count; i++ )
	{
		Entry = fgms->m_BlackList[i];
		if ( ( ID == 0 ) && ( Address.getIP () != 0 ) )
		{	// only list matching entries
			if ( Entry.Address != Address )
				continue;
		}
		else if ( ID )
		{
			if ( Entry.ID != ID )
				continue;
		}
		EntriesFound++;
		m_client << "ID " << Entry.ID << ": "
			<< Entry.Address.getHost () << " : " << Entry.Name
			<< crlf; if ( check_pager () ) return libcli::OK;
		if ( Brief == true )
		{
			continue;
		}
		string expire = "NEVER";
		difftime = now - Entry.JoinTime;
		if ( Entry.Timeout != 0 )
		{
			expire = NumToStr ( Entry.Timeout, 0 ) + " seconds";
		}
		m_client << "  entered      : " << timestamp_to_datestr ( Entry.JoinTime )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  last seen    : " << timestamp_to_days ( Entry.LastSeen )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  rcvd packets : " << Entry.PktsRcvd
			<< " (" << Entry.PktsRcvd / difftime << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  rcvd bytes   : " << byte_counter ( Entry.BytesRcvd )
			<< " (" << Entry.BytesRcvd / difftime << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  expire in    : " << expire
			<< crlf; if ( check_pager () ) return libcli::OK;
	}
	if ( EntriesFound )
	{
		m_client << crlf; if ( check_pager () ) return libcli::OK;
	}
	m_client << EntriesFound << " entries found" << crlf; if ( check_pager () ) return libcli::OK;
	if ( EntriesFound )
	{
		difftime = now - fgms->m_Uptime;
		m_client << "Total rcvd: "
			<< fgms->m_BlackList.PktsRcvd << " packets"
			<< " (" << fgms->m_BlackList.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter ( fgms->m_BlackList.BytesRcvd )
			<< " (" << byte_counter ( ( double ) ( fgms->m_BlackList.BytesRcvd / difftime ) ) << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
	}
	return libcli::OK;
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
RESULT
FG_CLI::cmd_blacklist_delete
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t		ID = 0;
	int			ID_invalid = -1;
	netAddress	Address;
	ItList		Entry;
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "ID" << "delete entry with ID" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "delete entry with IP address" << crlf;
				return libcli::OK;
			}
			else if ( ID == 0 )
			{
				ID = StrToNum<size_t> ( a, ID_invalid );
				if ( ID_invalid )
				{
					ID = 0;
					Address.set ( a.c_str (), 0 );
					if ( Address.getIP () == 0 )
					{
						return libcli::INVALID_ARG;
					}
				}
			}
			break;
		case 1: // only '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "delete entry" << crlf;
				return libcli::OK;
			}
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	if ( ( ID == 0 ) && ( Address.getIP () == 0 ) )
	{
		m_client << "% missing argument" << crlf;
		return libcli::MISSING_ARG;
	}
	if ( ( ID == 0 ) && ( Address.getIP () != 0 ) )
	{	// match IP
		Entry = fgms->m_BlackList.Find ( Address, "" );
		if ( Entry != fgms->m_BlackList.End () )
		{
			fgms->m_BlackList.Delete ( Entry );
		}
		else
		{
			m_client << "no entry found!" << crlf;
			return libcli::OK;
		}
		return libcli::OK;
	}
	Entry = fgms->m_BlackList.FindByID ( ID );
	if ( Entry != fgms->m_BlackList.End () )
	{
		fgms->m_BlackList.Delete ( Entry );
	}
	else
	{
		m_client << "no entry found!" << crlf;
		return libcli::OK;
	}
	m_client << "deleted!" << crlf;
	return libcli::OK;
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
RESULT
FG_CLI::cmd_blacklist_add
(
	const std::string& command,
	const libcli::tokens& args
)
{
	time_t		TTL = -1;
	int         I;
	netAddress	Address;
	string		Reason;
	ItList		Entry;
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // must be TTL or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "TTL" << "Timeout of the new entry in seconds" << crlf;
				return libcli::OK;
			}
			TTL = StrToNum<size_t> ( a, I );
			if ( I )
			{
				m_client << "% invalid TTL" << crlf;
				return libcli::ERROR_ARG;
			}
			break;
		case 1: // IP or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "IP address which should be blacklisted" << crlf;
				return libcli::OK;
			}
			Address.set ( a.c_str (), 0 );
			if ( Address.getIP () == 0 )
			{
				return libcli::INVALID_ARG;
			}
			break;
		default:
			if ( arg_wants_help ( a ) )
			{
				if ( Reason == "" )
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "STRING" << "a reason for blacklisting this IP" << crlf;
				}
				else
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "<cr>" << "add this IP" << crlf;
				}
				return libcli::OK;
			}
			Reason += a;
			if ( i + 1 < args.size () )
				Reason += " ";
			break;
		}
		++i;
	}
	FG_ListElement E ( Reason );
	E.Address = Address;
	size_t NewID;
	ItList CurrentEntry = fgms->m_BlackList.Find ( E.Address, "" );
	if ( CurrentEntry == fgms->m_BlackList.End () )
	{
		NewID = fgms->m_BlackList.Add ( E, TTL );
	}
	else
	{
		m_client << "% entry already exists (ID " << CurrentEntry->ID << ")!" << crlf;
		return libcli::ERROR_ARG;
	}
	m_client << "added with ID " << NewID << crlf;
	return libcli::OK;
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
RESULT
FG_CLI::cmd_crossfeed_delete
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t		ID { 0 };
	int         ID_invalid { -1 };
	netAddress	Address;
	ItList		Entry;
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "ID" << "delete entry with ID" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "delete entry with IP address" << crlf;
				return libcli::OK;
			}
			else
			{
				ID = StrToNum<size_t> ( a, ID_invalid );
				if ( ID_invalid )
				{
					ID = 0;
					Address.set ( a.c_str (), 0 );
					if ( Address.getIP () == 0 )
					{
						return libcli::INVALID_ARG;
					}
				}
			}
			break;
		case 1: // only '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "delete this crossfeed" << crlf;
				return libcli::OK;
			}
		default:
			return libcli::INVALID_ARG;
		}
		i++;
	}
	if ( ( ID == 0 ) && ( Address.getIP () == 0 ) )
	{
		return libcli::MISSING_ARG;
	}
	if ( ( ID == 0 ) && ( Address.getIP () != 0 ) )
	{	// match IP
		Entry = fgms->m_CrossfeedList.Find ( Address, "" );
		if ( Entry != fgms->m_CrossfeedList.End () )
		{
			fgms->m_CrossfeedList.Delete ( Entry );
		}
		else
		{
			m_client << "no entry found" << crlf;
			return libcli::OK;
		}
		return libcli::OK;
	}
	Entry = fgms->m_CrossfeedList.FindByID ( ID );
	if ( Entry != fgms->m_CrossfeedList.End () )
	{
		fgms->m_CrossfeedList.Delete ( Entry );
	}
	else
	{
		m_client << "no entry found" << crlf;
		return libcli::OK;
	}
	m_client << "deleted" << crlf;
	return libcli::OK;
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
RESULT
FG_CLI::cmd_crossfeed_add
(
	const std::string& command,
	const libcli::tokens& args
)
{
	netAddress	Address;
	string	    Name;
	int	        Port;
	int		    I;
	ItList		Entry;
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // IP or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "IP address of the crossfeed" << crlf;
				return libcli::OK;
			}
			Address.set ( a.c_str (), 0 );
			if ( Address.getIP () == 0 )
			{
				return libcli::INVALID_ARG;
			}
			break;
		case 1: // Port or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "Port" << "Port of the relay" << crlf;
				return libcli::OK;
			}
			Port = StrToNum<int> ( a, I );
			if ( I )
			{
				return libcli::INVALID_ARG;
			}
			break;
		default:    // name of the crossfeed
			if ( arg_wants_help ( a ) )
			{
				if ( Name == "" )
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "NAME" << "The name of this crossfeed" << crlf;
				}
				else
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "<cr>" << "add this crossfeed" << crlf;
				}
				return libcli::OK;
			}
			Name += a;
			if ( i + 1 < args.size () )
				Name += " ";
			break;
		}
		++i;
	}
	FG_ListElement E ( Name );
	E.Address = Address;
	E.Address.setPort ( Port );
	size_t NewID;
	ItList CurrentEntry = fgms->m_CrossfeedList.Find ( E.Address, "" );
	if ( CurrentEntry == fgms->m_CrossfeedList.End () )
	{
		NewID = fgms->m_CrossfeedList.Add ( E, Port );
	}
	else
	{
		m_client << "entry already exists (ID " << CurrentEntry->ID << ")" << crlf;
		return libcli::ERROR_ARG;
	}
	m_client << "added with ID " << NewID << crlf;
	return libcli::OK;
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
RESULT
FG_CLI::cmd_crossfeed_show
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t		ID = 0;
	int		    ID_invalid = -1;
	netAddress	Address ( "0.0.0.0", 0 );
	bool		Brief = false;
	size_t		EntriesFound = 0;
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "brief" << "show brief listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "ID" << "show entry with ID" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "show entry with IP-Address" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show long listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			else if ( strncmp ( a.c_str (), "brief", a.length () ) == 0 )
			{
				Brief = true;
			}
			else
			{
				ID = StrToNum<size_t> ( a, ID_invalid );
				if ( ID_invalid )
				{
					ID = 0;
					Address.set ( a.c_str (), 0 );
					if ( Address.getIP () == 0 )
					{
						return libcli::INVALID_ARG;
					}
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				if ( !Brief )
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "brief" << "show brief listing" << crlf;
				}
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show long listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			else if ( strncmp ( a.c_str (), "brief", a.length () ) == 0 )
			{
				Brief = true;
			}
			break;
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	int Count = fgms->m_CrossfeedList.Size ();
	FG_ListElement Entry ( "" );
	m_client << fgms->m_CrossfeedList.Name << ":" << crlf;
	m_client << crlf;
	time_t  difftime;
	time_t  now;
	now = time ( 0 );
	for ( int i = 0; i < Count; i++ )
	{
		Entry = fgms->m_CrossfeedList[i];
		if ( ( ID == 0 ) && ( Address.getIP () != 0 ) )
		{	// only list matching entries
			if ( Entry.Address != Address )
				continue;
		}
		else if ( ID )
		{
			if ( Entry.ID != ID )
				continue;
		}
		EntriesFound++;
		difftime = now - Entry.JoinTime;
		m_client << "ID " << Entry.ID << ": "
			<< Entry.Address.getHost () << ":" << Entry.Address.getPort ()
			<< " : " << Entry.Name
			<< crlf; if ( check_pager () ) return libcli::OK;
		if ( Brief == true )
		{
			continue;
		}
		m_client << "  entered      : " << timestamp_to_datestr ( Entry.JoinTime )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  last sent    : " << timestamp_to_days ( Entry.LastSent )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  sent packets : " << Entry.PktsSent
			<< "(" << ( double ) ( Entry.PktsSent / difftime ) << " packets/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  sent bytes   : " << byte_counter ( Entry.BytesSent )
			<< "(" << byte_counter ( ( double ) Entry.BytesSent / difftime ) << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
	}
	if ( EntriesFound )
	{
		m_client << crlf; if ( check_pager () ) return libcli::OK;
	}
	m_client << EntriesFound << " entries found" << crlf; if ( check_pager () ) return libcli::OK;
	if ( EntriesFound )
	{
		difftime = now - fgms->m_Uptime;
		m_client << "Total sent: "
			<< fgms->m_CrossfeedList.PktsSent << " packets"
			<< "(" << fgms->m_CrossfeedList.PktsSent / difftime << "/s)"
			<< " / " << byte_counter ( fgms->m_CrossfeedList.BytesSent )
			<< "(" << byte_counter ( ( double ) ( fgms->m_CrossfeedList.BytesSent / difftime ) ) << "/s)"
			<< crlf;
	}
	return libcli::OK;
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
RESULT
FG_CLI::cmd_relay_show
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t		ID = 0;
	int			ID_invalid = -1;
	netAddress	Address ( "0.0.0.0", 0 );
	bool		Brief = false;
	size_t		EntriesFound = 0;
	string		Name;
	size_t		i { 0 };
	for ( auto a : args )
	{

		switch ( i )
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "brief" << "show brief listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "ID" << "show entry with ID" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "show entry with IP-address" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "NAME" << "show user with NAME" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show log listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			else if ( strncmp ( a.c_str (), "brief", a.length () ) == 0 )
			{
				Brief = true;
			}
			else
			{
				ID = StrToNum<size_t> ( a, ID_invalid );
				if ( ID_invalid )
				{
					ID = 0;
					Address.set ( a.c_str (), 0 );
					if ( Address.getIP () == 0 )
					{
						Name = a;
					}
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				if ( !Brief )
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "brief" << "show brief listing" << crlf;
				}
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show log listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			else if ( strncmp ( a.c_str (), "brief", a.length () ) == 0 )
			{
				Brief = true;
			}
			break;
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	int Count = fgms->m_RelayList.Size ();
	FG_ListElement Entry ( "" );
	m_client << fgms->m_RelayList.Name << ":" << crlf;
	m_client << crlf;
	time_t  difftime;
	time_t  now;
	now = time ( 0 );
	for ( int i = 0; i < Count; i++ )
	{
		Entry = fgms->m_RelayList[i];
		if ( ( ID == 0 ) && ( Address.getIP () != 0 ) )
		{	// only list matching entries
			if ( Entry.Address != Address )
				continue;
		}
		else if ( ID )
		{
			if ( Entry.ID != ID )
				continue;
		}
		else if ( Name != "" )
		{
			if ( Entry.Name.find ( Name ) == string::npos )
				continue;
		}
		EntriesFound++;
		difftime = now - Entry.JoinTime;
		m_client << "ID " << Entry.ID << ": "
			<< Entry.Address.getHost () << ":" << Entry.Address.getPort ()
			<< " : " << Entry.Name
			<< crlf; if ( check_pager () ) return libcli::OK;
		if ( Brief == true )
		{
			continue;
		}
		m_client << "  entered   : " << timestamp_to_datestr ( Entry.JoinTime )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  last seen : " << timestamp_to_datestr ( Entry.LastSeen )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  sent      : "
			<< Entry.PktsSent << " packets"
			<< " (" << Entry.PktsSent / difftime << "/s)"
			<< " / " << byte_counter ( Entry.BytesSent )
			<< " (" << byte_counter ( ( double ) Entry.BytesSent / difftime ) << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "  rcvd      : "
			<< Entry.PktsRcvd << " packets"
			<< " (" << Entry.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter ( Entry.BytesRcvd )
			<< " (" << byte_counter ( ( double ) Entry.BytesRcvd / difftime ) << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
	}
	m_client << crlf;
	m_client << EntriesFound << " entries found"
		<< crlf; if ( check_pager () ) return libcli::OK;
	if ( Brief )
	{
		return libcli::OK;
	}
	difftime = now - fgms->m_Uptime;
	m_client << "Totals:" << crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  sent      : "
		<< fgms->m_RelayList.PktsSent << " packets"
		<< " (" << fgms->m_RelayList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter ( fgms->m_RelayList.BytesSent )
		<< " (" << byte_counter ( ( double ) fgms->m_RelayList.BytesSent / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	m_client << "  received  : "
		<< fgms->m_RelayList.PktsRcvd << " packets"
		<< " (" << fgms->m_RelayList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter ( fgms->m_RelayList.BytesRcvd )
		<< " (" << byte_counter ( ( double ) fgms->m_RelayList.BytesRcvd / difftime ) << "/s)"
		<< crlf; if ( check_pager () ) return libcli::OK;
	return libcli::OK;
} // FG_CLI::cmd_relay_show

//////////////////////////////////////////////////
/**
 *  @brief Show status of tracker server
 *
 *  possible arguments:
 *  show tracker ?
 *  show tracker <cr>
 */
RESULT
FG_CLI::cmd_tracker_show
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show long listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			else	// FIXME: check what happens when there are more args
			{
				return libcli::INVALID_ARG;
			}
		default:
			//return invalid_arg (a);
			m_client << "% invalid argument" << crlf;
			return libcli::INVALID_ARG;
		}
		++i;
	}
	if ( !fgms->m_IsTracked )
	{
		m_client << "This server is NOT tracked" << crlf;
		m_client << crlf;
		return libcli::OK;
	}
	time_t  difftime;
	time_t  now;
	now = time ( 0 );
	difftime = now - fgms->m_Uptime;
	m_client << "This server is tracked: "
		<< fgms->m_Tracker->GetTrackerServer () << ":"
		<< fgms->m_Tracker->GetTrackerPort ()
		<< crlf;
	if ( fgms->m_Tracker->m_connected )
	{
		m_client << "state: connected since "
			<< timestamp_to_datestr ( fgms->m_Tracker->LastConnected )
			<< " (" << timestamp_to_days ( fgms->m_Tracker->LastConnected ) << " ago)"
			<< crlf;
	}
	else
	{
		m_client << "state: NOT connected!" << crlf;
	}
	string A = "NEVER";
	if ( fgms->m_Tracker->LastSeen != 0 )
	{
		A = timestamp_to_days ( fgms->m_Tracker->LastSeen );
		A += " ago";
	}
	string B = "NEVER";
	if ( fgms->m_Tracker->LastSent != 0 )
	{
		B = timestamp_to_days ( fgms->m_Tracker->LastSent );
		B += " ago";
	}
	m_client << "last seen " << A << ", last sent " << B << crlf;
	m_client << "I had " << fgms->m_Tracker->LostConnections << " lost connections" << crlf;
	m_client << crlf;
	m_client << "Counters:" << crlf;
	m_client << "  sent    : " << fgms->m_Tracker->PktsSent << " packets";
	m_client << " (" << fgms->m_Tracker->PktsSent / difftime << "/s)";
	m_client << " / " << byte_counter ( fgms->m_Tracker->BytesSent );
	m_client << " (" << byte_counter ( ( double ) fgms->m_Tracker->BytesSent / difftime ) << "/s)";
	m_client << crlf;
	m_client << "  received: " << fgms->m_Tracker->PktsRcvd << " packets";
	m_client << " (" << fgms->m_Tracker->PktsRcvd / difftime << "/s)";
	m_client << " / " << byte_counter ( fgms->m_Tracker->BytesRcvd );
	m_client << " (" << byte_counter ( ( double ) fgms->m_Tracker->BytesRcvd / difftime ) << "/s)";
	m_client << crlf;
	m_client << "  queue size: " << fgms->m_Tracker->msg_queue.size () << " messages" << crlf;
	return libcli::OK;
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
RESULT
FG_CLI::cmd_relay_delete
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	netAddress	Address;
	ItList		Entry;
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "ID" << "delete entry with ID" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "delete entry with IP address" << crlf;
				return libcli::OK;
			}
			else if ( ID == 0 )
			{
				ID = StrToNum<size_t> ( a, ID_invalid );
				if ( ID_invalid )
				{
					ID = 0;
					Address.set ( a.c_str (), 0 );
					if ( Address.getIP () == 0 )
					{
						return libcli::INVALID_ARG;
					}
				}
			}
			break;
		case 1: // only '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "delete entry" << crlf;
				return libcli::OK;
			}
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	if ( ( ID == 0 ) && ( Address.getIP () == 0 ) )
	{
		m_client << "% missing argument" << crlf;
		return libcli::MISSING_ARG;
	}
	if ( ( ID == 0 ) && ( Address.getIP () != 0 ) )
	{	// match IP
		Entry = fgms->m_RelayList.Find ( Address, "" );
		if ( Entry != fgms->m_RelayList.End () )
		{
			fgms->m_RelayList.Delete ( Entry );
		}
		else
		{
			m_client << "no entry found" << crlf;
			return libcli::OK;
		}
		return libcli::OK;
	}
	Entry = fgms->m_RelayList.FindByID ( ID );
	if ( Entry != fgms->m_RelayList.End () )
	{
		fgms->m_RelayList.Delete ( Entry );
	}
	else
	{
		m_client << "no entry found" << crlf;
		return libcli::OK;
	}
	m_client << "deleted" << crlf;
	return libcli::OK;
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
RESULT
FG_CLI::cmd_relay_add
(
	const std::string& command,
	const libcli::tokens& args
)
{
	netAddress	Address;
	string		Name;
	int			Port;
	int			I;
	ItList		Entry;
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // IP or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "IP address of the relay" << crlf;
				return libcli::OK;
			}
			Address.set ( a.c_str (), 0 );
			if ( Address.getIP () == 0 )
			{
				return libcli::INVALID_ARG;
			}
			break;
		case 1: // Port or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "Port" << "Port of the relay" << crlf;
				return libcli::OK;
			}
			Port = StrToNum<int> ( a, I );
			if ( I )
			{
				return libcli::INVALID_ARG;
			}
			break;
		default: // '?' or <CR>
			if ( arg_wants_help ( a ) )
			{
				if ( Name == "" )
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "NAME" << "the name of this relay" << crlf;
				else
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "add this relay" << crlf;
				return libcli::OK;
			}
			Name += a;
			if ( i + 1 < args.size () )
				Name += " ";
			break;
		}
		i++;
	}
	FG_ListElement E ( Name );
	E.Address = Address;
	E.Address.setPort ( Port );
	size_t NewID;
	ItList CurrentEntry = fgms->m_RelayList.Find ( E.Address, "" );
	if ( CurrentEntry == fgms->m_RelayList.End () )
	{
		NewID = fgms->m_RelayList.Add ( E, 0 );
	}
	else
	{
		m_client << "entry already exists (ID " << CurrentEntry->ID << ")" << crlf;
		return libcli::ERROR_ARG;
	}
	m_client << "added with ID " << NewID << crlf;
	return libcli::OK;
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
RESULT
FG_CLI::cmd_user_show
(
	const std::string& command,
	const libcli::tokens& args
)
{
	size_t		ID = 0;
	int		ID_invalid = -1;
	netAddress	Address ( "0.0.0.0", 0 );
	string		Name;
	bool		Brief = false;
	bool		OnlyLocal = false;
	bool		OnlyRemote = false;
	size_t		EntriesFound = 0;
	time_t		difftime;
	time_t		now = time ( 0 );
	size_t		i { 0 };
	for ( auto a : args )
	{
		switch ( i )
		{
		case 0: // ID or IP or 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "brief" << "show brief listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "ID" << "show user with ID" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "IP" << "show user with IP-Address" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "NAME" << "show user with NAME" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "local" << "show only local users" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "remote" << "show only local users" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show long listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			else if ( strncmp ( a.c_str (), "brief", a.length () ) == 0 )
			{
				Brief = true;
			}
			else if ( ID == 0 )
			{
				ID = StrToNum<size_t> ( a, ID_invalid );
				if ( ID_invalid )
				{
					ID = 0;
					Address.set ( a.c_str (), 0 );
					if ( Address.getIP () == 0 )
					{
						Name = a;
					}
				}
			}
			break;
		case 1: // 'brief' or '?'
			if ( arg_wants_help ( a ) )
			{
				if ( !Brief )
				{
					m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
						<< "brief" << "show brief listing" << crlf;
				}
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "<cr>" << "show long listing" << crlf;
				m_client << "  " << std::left << std::setfill ( ' ' ) << std::setw ( 22 )
					<< "|" << "output modifier" << crlf;
				return libcli::OK;
			}
			else if ( strncmp ( a.c_str (), "brief", a.length () ) == 0 )
			{
				Brief = true;
			}
			break;
		default:
			return libcli::INVALID_ARG;
		}
		++i;
	}
	int Count = fgms->m_PlayerList.Size ();
	FG_Player	Player;
	Point3D		PlayerPosGeod;
	string		Origin;
	string		FullName;
	m_client << fgms->m_PlayerList.Name << ":" << crlf;
	m_client << crlf;
	if ( Name == "local" )
	{
		OnlyLocal = true;
		Name = "";
	}
	if ( Name == "remote" )
	{
		OnlyRemote = true;
		Name = "";
	}
	for ( int i = 0; i < Count; i++ )
	{
		now = time ( 0 );
		Player = fgms->m_PlayerList[i];
		if ( ( ID == 0 ) && ( Address.getIP () != 0 ) )
		{	// only list matching entries
			if ( Player.Address != Address )
				continue;
		}
		else if ( ID )
		{
			if ( Player.ID != ID )
				continue;
		}
		else if ( Name != "" )
		{
			if ( Player.Name.find ( Name ) == string::npos )
				continue;
		}
		else if ( OnlyLocal == true )
		{
			if ( Player.IsLocal == false )
				continue;
		}
		else if ( OnlyRemote == true )
		{
			if ( Player.IsLocal == true )
				continue;
		}
		sgCartToGeod ( Player.LastPos, PlayerPosGeod );
		if ( Player.IsLocal )
		{
			Origin = "LOCAL";
		}
		else
		{
			FG_SERVER::mT_RelayMapIt Relay = fgms->m_RelayMap.find ( Player.Address.getIP () );
			if ( Relay != fgms->m_RelayMap.end () )
			{
				Origin = Relay->second;
			}
			else
			{
				Origin = Player.Origin;
			}
		}
		FullName = Player.Name + string ( "@" ) + Origin;
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
		m_client << "ID " << Player.ID << ": "
			<< FullName << ATC
			<< crlf; if ( check_pager () ) return libcli::OK;
		EntriesFound++;
		if ( Brief == true )
		{
			continue;
		}
		if ( Player.HasErrors == true )
		{
			m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
				<< "ERROR" << Player.Error
				<< crlf; if ( check_pager () ) return libcli::OK;
		}
		difftime = now - Player.JoinTime;
		m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
			<< "protocoll" << Player.ProtoMajor << "." << Player.ProtoMinor
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
			<< "radar range" << Player.RadarRange
			<< crlf; if ( check_pager () ) return libcli::OK;
		int expires = Player.Timeout - ( now - Player.LastSeen );
		m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
			<< "entered" << timestamp_to_days ( Player.JoinTime ) << " ago"
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
			<< "joined" << timestamp_to_datestr ( Player.JoinTime )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
			<< "last seen" << timestamp_to_datestr ( Player.LastSeen )
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
			<< "using model" << Player.ModelName
			<< crlf; if ( check_pager () ) return libcli::OK;
		if ( Player.IsLocal )
		{
			m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
				<< "real origin" << Player.Origin
				<< crlf; if ( check_pager () ) return libcli::OK;
			m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
				<< "sent" << Player.PktsSent << " packets "
				<< "(" << Player.PktsSent / difftime << "/s)"
				<< " / " << byte_counter ( Player.BytesSent )
				<< " (" << byte_counter ( ( double ) Player.BytesSent / difftime ) << "/s)"
				<< crlf; if ( check_pager () ) return libcli::OK;
		}

		m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
			<< "rcvd" << Player.PktsRcvd << " packets "
			<< "(" << Player.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter ( Player.BytesRcvd )
			<< " (" << byte_counter ( ( double ) Player.BytesRcvd / difftime ) << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
			<< "expires in" << expires
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "         " << std::left << std::setfill ( ' ' ) << std::setw ( 15 )
			<< "inactive" << now - Player.LastRelayedToInactive
			<< crlf; if ( check_pager () ) return libcli::OK;
	}
	difftime = now - fgms->m_Uptime;
	m_client << crlf;
	m_client << EntriesFound << " entries found" << crlf;
	if ( !Brief )
	{
		m_client << "Totals:" << crlf; if ( check_pager () ) return libcli::OK;
		m_client << "          sent    : " << fgms->m_PlayerList.PktsSent << " packets"
			<< " (" << fgms->m_PlayerList.PktsSent / difftime << "/s)"
			<< " / " << byte_counter ( fgms->m_PlayerList.BytesSent )
			<< " (" << byte_counter ( ( double ) fgms->m_PlayerList.BytesSent / difftime ) << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
		m_client << "          received: " << fgms->m_PlayerList.PktsRcvd << " packets"
			<< " (" << fgms->m_PlayerList.PktsRcvd / difftime << "/s)"
			<< " / " << byte_counter ( fgms->m_PlayerList.BytesRcvd )
			<< " (" << byte_counter ( ( double ) fgms->m_PlayerList.BytesRcvd / difftime ) << "/s)"
			<< crlf; if ( check_pager () ) return libcli::OK;
	}
	return libcli::OK;
} // FG_CLI::cmd_user_show

RESULT
FG_CLI::cmd_NOT_IMPLEMENTED
(
	const std::string& command,
	const libcli::tokens& args
)
{
	m_client << "Command '" << command << "' NOT IMPLEMENTED YET" << crlf;
	m_client << "  args:" << crlf;
	for ( auto a : args )
		m_client << "  '" << a << "'" << crlf;
	return libcli::OK;
}

