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
 * @file fgls_cli.cxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	07/2017
 */

#include <sstream>
#include "fgls_cli.hxx"

using namespace fgmp;

//////////////////////////////////////////////////
fgls_cli::fgls_cli
(
	FGLS*	fgls,
	int	fd
) : cli (fd)
{
	this->fgls = fgls;
	this->setup ();
} // fgls_cli::fgls_cli ()

//////////////////////////////////////////////////
/** Setup all commands
 */
void
fgls_cli::setup
()
{
	typedef Command<cli>::cpp_callback_func callback_ptr;
	typedef Command<cli>::cpp_callback_func callback_ptr;
	typedef cli::cpp_auth_func auth_callback;          
	typedef cli::cpp_enable_func enable_callback;
	Command<cli>* c;

	set_hostname ( fgls->m_server_name );
	std::stringstream banner;
	banner << "\r\n"
		<< "------------------------------------------------\r\n"
		<< "FlightGear List Server cli\r\n"
		<< "This is " << fgls->m_server_name << "\r\n"
		<< "------------------------------------------------\r\n";
	set_banner ( banner.str() );
	if ( fgls->m_admin_user != "" )
		allow_user ( fgls->m_admin_user, fgls->m_admin_pass );
	if ( fgls->m_admin_enable != "" )
		allow_enable ( fgls->m_admin_enable );
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
		"log",
		static_cast<callback_ptr> (&fgls_cli::cmd_show_log),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show log buffer"
	), c );
	register_command ( new Command<cli> (
		this,
		"settings",
		static_cast<callback_ptr> (&fgls_cli::cmd_show_settings),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show general settings"
	), c );
	register_command ( new Command<cli> (
		this,
		"version",
		static_cast<callback_ptr> (&fgls_cli::cmd_show_version),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show running version information"
	), c );
	register_command ( new Command<cli> (
		this,
		"uptime",
		static_cast<callback_ptr> (&fgls_cli::cmd_show_uptime),
		libcli::UNPRIVILEGED,
		libcli::MODE_ANY,
		"Show uptime information"
	), c );
	register_command ( new Command<cli> (
		this,
		"die",
		static_cast<callback_ptr> (&fgls_cli::cmd_fgls_die),
		libcli::PRIVILEGED,
		libcli::MODE_ANY,
		"force fgls to exit"
	));
} // fgls_cli::setup()

//////////////////////////////////////////////////
/**
 *  @brief Show log buffer of the the server
 */
int
fgls_cli::cmd_show_log
(
	UNUSED(char *command),
	UNUSED(char *argv[]),
	UNUSED(int argc)
)
{
	if ( argc > 0 )
	{
		if ( strcmp ( argv[0], "?" ) == 0 )
			client << "<cr>" << CRLF;
		return libcli::OK;
	}
	fgmp::str_list*  buf = logger.logbuf();
	fgmp::str_it     it;
	buf->lock ();
	for ( it = buf->begin(); it != buf->end(); it++ )
	{
		client << *it << commit;
		if (check_pager()) return libcli::OK;
	}
	buf->unlock ();
	return libcli::OK;
} // fgls_cli::cmd_show_log()

//////////////////////////////////////////////////
/**
 *  @brief Show general statistics
 */
#if 0
int
fgls_cli::cmd_show_stats
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
	difftime = now - fgls->m_Uptime;
	cmd_show_version (command, argv, argc);
	client << CRLF;
	client	<< "I have " << fgls->m_BlackList.Size ()
		<< " entries in my blacklist"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgls->m_CrossfeedList.Size () << " crossfeeds"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgls->m_RelayList.Size () << " relays"
		<< CRLF; if (check_pager()) return 0;
	client << "I have " << fgls->m_PlayerList.Size () << " users ("
		<< fgls->m_LocalClients << " local, "
		<< fgls->m_RemoteClients << " remote, "
		<< fgls->m_NumMaxClients << " max)"
		<< CRLF; if (check_pager()) return 0;

	client << "Sent counters:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "to crossfeeds:"
		<< fgls->m_CrossfeedList.PktsSent << " packets"
		<< " (" << fgls->m_CrossfeedList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgls->m_CrossfeedList.BytesSent)
		<< " (" << byte_counter ((double) fgls->m_CrossfeedList.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "to relays:"
		<< fgls->m_RelayList.PktsSent << " packets"
		<< " (" << fgls->m_RelayList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgls->m_RelayList.BytesSent)
		<< " (" << byte_counter ((double) fgls->m_RelayList.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0; 
	client << "  " << left << setfill(' ') << setw(22)
		<< "to users:"
		<< fgls->m_PlayerList.PktsSent << " packets"
		<< " (" << fgls->m_PlayerList.PktsSent / difftime << "/s)"
		<< " / " << byte_counter (fgls->m_PlayerList.BytesSent)
		<< " (" << byte_counter ((double) fgls->m_PlayerList.BytesSent / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;

	client << "Receive counters:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "total:" <<  fgls->m_PacketsReceived
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "pings:" <<  fgls->m_PingReceived
		<< " (" << fgls->m_PongReceived << " pongs)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "errors:"
		<< "invalid packets:" << fgls->m_PacketsInvalid
		<< " rejected:" << fgls->m_BlackRejected
		<< " unknown relay:" << fgls->m_UnknownRelay
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "valid data:"
		<< "pos data:" << fgls->m_PositionData
		<< " other:" << fgls->m_UnkownMsgID
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "tracker:"
		<< "connects:" << fgls->m_TrackerConnect
		<< " disconnects:" << fgls->m_TrackerDisconnect
		<< " positions:" << fgls->m_TrackerPosition
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "admin connections:" << fgls->m_AdminReceived
		<< CRLF; if (check_pager()) return 0;
	float telnet_per_second;
	if (fgls->m_TelnetReceived)
		telnet_per_second = (float) fgls->m_TelnetReceived / (time(0) - fgls->m_Uptime);
	else
		telnet_per_second = 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "telnet connections: "
		<< fgls->m_TelnetReceived
		<< " (" << setprecision(2) << telnet_per_second << " t/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "blacklist:"
		<< fgls->m_BlackList.PktsRcvd << " packets"
		<< " (" << fgls->m_BlackList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgls->m_BlackList.BytesRcvd)
		<< " (" << byte_counter ((double) fgls->m_BlackList.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "relays:"
		<< fgls->m_RelayList.PktsRcvd << " packets"
		<< " (" << fgls->m_RelayList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgls->m_RelayList.BytesRcvd)
		<< " (" << byte_counter ((double) fgls->m_RelayList.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "users:"
		<< fgls->m_PlayerList.PktsRcvd << " packets"
		<< " (" << fgls->m_PlayerList.PktsRcvd / difftime << "/s)"
		<< " / " << byte_counter (fgls->m_PlayerList.BytesRcvd)
		<< " (" << byte_counter ((double) fgls->m_PlayerList.BytesRcvd / difftime) << "/s)"
		<< CRLF; if (check_pager()) return 0;
	accumulated_sent	+= fgls->m_CrossfeedList.BytesSent;
	accumulated_sent_pkts	+= fgls->m_CrossfeedList.PktsSent;
	accumulated_sent	+= fgls->m_RelayList.BytesSent;
	accumulated_sent_pkts	+= fgls->m_RelayList.PktsSent;
	accumulated_sent	+= fgls->m_PlayerList.BytesSent;
	accumulated_sent_pkts	+= fgls->m_PlayerList.PktsSent;
	accumulated_rcvd	+= fgls->m_BlackList.BytesRcvd;
	accumulated_rcvd_pkts	+= fgls->m_BlackList.PktsRcvd;
	accumulated_rcvd	+= fgls->m_RelayList.BytesRcvd;
	accumulated_rcvd_pkts	+= fgls->m_RelayList.PktsRcvd;
	accumulated_rcvd	+= fgls->m_PlayerList.BytesRcvd;
	accumulated_rcvd_pkts	+= fgls->m_PlayerList.PktsRcvd;
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
} // fgls_cli::cmd_show_stats ()
#endif

//////////////////////////////////////////////////
/**
 *  @brief Show general settings
 */
int
fgls_cli::cmd_show_settings
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

	if ( fgls->m_bind_addr == "" )
		bind_addr = "*";
	else
		bind_addr = fgls->m_bind_addr;
	cmd_show_version (command, argv, argc);
	client << CRLF;
	client << "current settings:" << CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "data port:" << fgls->m_data_port
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "query port:" << fgls->m_query_port
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "admin port:" << fgls->m_admin_port
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "logfile:" << fgls->m_logfile_name
		<< CRLF; if (check_pager()) return 0;
	client << "  " << left << setfill(' ') << setw(22)
		<< "bind address:" << bind_addr
		<< CRLF; if (check_pager()) return 0;
	return (0);
} // fgls_cli::cmd_show_settings ()

//////////////////////////////////////////////////
/**
 *  @brief Shutdown the server
 */
int
fgls_cli::cmd_fgls_die
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
	fgls->m_want_exit = true;
	return libcli::QUIT;
} // fgls_cli::cmd_fgls_die

//////////////////////////////////////////////////
/**
 *  @brief Show the uptime of the the server
 *         in a human readable form.
 */
int
fgls_cli::cmd_show_uptime
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
	client << "UP since " << timestamp_to_datestr(fgls->m_uptime)
		<< "(" << timestamp_to_days(fgls->m_uptime) << ")" << CRLF;
	return (0);
} // fgls_cli::cmd_show_uptime

//////////////////////////////////////////////////
/**
 *  @brief Show the version number of the the server
 */
int
fgls_cli::cmd_show_version
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
	client << "This is " << fgls->m_server_name << CRLF;
	client << "FlightGear List Server version "
	       << fgls->m_version.str() << CRLF; 
	client << "compiled on " << __DATE__ << " at " << __TIME__  << CRLF;
	client << "using protocol version v ! TODO !" << CRLF;
	cmd_show_uptime (command, argv, argc);
	return (0);
} // fgls_cli::cmd_show_version

