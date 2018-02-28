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
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        07/2017
 */

#include <sstream>
#include "fgls_cli.hxx"
#include <fglib/fg_util.hxx>

using namespace fgmp;

//////////////////////////////////////////////////
fgls_cli::fgls_cli
(
        FGLS*   fgls,
        int     fd
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
        command* c;

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
                set_enable_password ( fgls->m_admin_enable );
        c = new command (
                "show",
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::EXEC,
                "show system information"
        );

        using namespace std::placeholders;
        #define _ptr(X) (std::bind (& X, this, _1, _2, _3))

        register_command (c);
        register_command ( new command (
                "log",
                _ptr ( fgls_cli::cmd_show_log ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show log buffer"
        ), c );
        register_command ( new command (
                "settings",
                _ptr ( fgls_cli::cmd_show_settings ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show general settings"
        ), c );
        register_command ( new command (
                "version",
                _ptr ( fgls_cli::cmd_show_version ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show running version information"
        ), c );
        register_command ( new command (
                "uptime",
                _ptr ( fgls_cli::cmd_show_uptime ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show uptime information"
        ), c );
        register_command ( new command (
                "die",
                _ptr ( fgls_cli::cmd_die ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::ANY,
                "force fgls to exit"
        ));
        #undef _ptr
} // fgls_cli::setup()

//////////////////////////////////////////////////
/**
 *  @brief Show log buffer of the the server
 */
RESULT
fgls_cli::cmd_show_log
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        if ( first_arg > 0 )
        {
                if ( args[first_arg] == "?" )
                        m_client << "<cr>" << cli_client::endl;
                return RESULT::OK;
        }
        fgmp::str_list*  buf = logger.logbuf();
        fgmp::str_it     it;
        buf->lock ();
        for ( it = buf->begin(); it != buf->end(); it++ )
        {
                m_client << *it << cli_client::flush;
        }
        buf->unlock ();
        return RESULT::OK;
} // fgls_cli::cmd_show_log()

//////////////////////////////////////////////////
/**
 *  @brief Show general statistics
 */
#if 0
RESULT
fgls_cli::cmd_show_stats
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        time_t  difftime;
        time_t  now;
        uint64_t        accumulated_sent        = 0;
        uint64_t        accumulated_rcvd        = 0;
        uint64_t        accumulated_sent_pkts   = 0;
        uint64_t        accumulated_rcvd_pkts   = 0;
        if (first_arg > 0)
        {
                m_client << "<cr>" << endl;
                return (0);
        }
        now = time(0);
        difftime = now - fgls->m_Uptime;
        cmd_show_version (command, args, first_arg);
        m_client << endl;
        m_client  << "I have " << fgls->m_BlackList.Size ()
                << " entries in my blacklist"
                << endl;
        m_client << "I have " << fgls->m_CrossfeedList.Size () << " crossfeeds"
                << endl;
        m_client << "I have " << fgls->m_RelayList.Size () << " relays"
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "I have " << fgls->m_PlayerList.Size () << " users ("
                << fgls->m_LocalClients << " local, "
                << fgls->m_RemoteClients << " remote, "
                << fgls->m_NumMaxClients << " max)"
                << endl; if (check_pager()) return RESULT::OK;

        m_client << "Sent counters:" << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "to crossfeeds:"
                << fgls->m_CrossfeedList.PktsSent << " packets"
                << " (" << fgls->m_CrossfeedList.PktsSent / difftime << "/s)"
                << " / " << byte_counter (fgls->m_CrossfeedList.BytesSent)
                << " (" << byte_counter ((double) fgls->m_CrossfeedList.BytesSent / difftime) << "/s)"
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "to relays:"
                << fgls->m_RelayList.PktsSent << " packets"
                << " (" << fgls->m_RelayList.PktsSent / difftime << "/s)"
                << " / " << byte_counter (fgls->m_RelayList.BytesSent)
                << " (" << byte_counter ((double) fgls->m_RelayList.BytesSent / difftime) << "/s)"
                << endl; if (check_pager()) return RESULT::OK; 
        m_client << "  " << left << setfill(' ') << setw(22)
                << "to users:"
                << fgls->m_PlayerList.PktsSent << " packets"
                << " (" << fgls->m_PlayerList.PktsSent / difftime << "/s)"
                << " / " << byte_counter (fgls->m_PlayerList.BytesSent)
                << " (" << byte_counter ((double) fgls->m_PlayerList.BytesSent / difftime) << "/s)"
                << endl; if (check_pager()) return RESULT::OK;

        m_client << "Receive counters:" << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "total:" <<  fgls->m_PacketsReceived
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "pings:" <<  fgls->m_PingReceived
                << " (" << fgls->m_PongReceived << " pongs)"
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "errors:"
                << "invalid packets:" << fgls->m_PacketsInvalid
                << " rejected:" << fgls->m_BlackRejected
                << " unknown relay:" << fgls->m_UnknownRelay
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "valid data:"
                << "pos data:" << fgls->m_PositionData
                << " other:" << fgls->m_UnkownMsgID
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "tracker:"
                << "connects:" << fgls->m_TrackerConnect
                << " disconnects:" << fgls->m_TrackerDisconnect
                << " positions:" << fgls->m_TrackerPosition
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "admin connections:" << fgls->m_AdminReceived
                << endl; if (check_pager()) return RESULT::OK;
        float telnet_per_second;
        if (fgls->m_TelnetReceived)
                telnet_per_second = (float) fgls->m_TelnetReceived / (time(0) - fgls->m_Uptime);
        else
                telnet_per_second = 0;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "telnet connections: "
                << fgls->m_TelnetReceived
                << " (" << setprecision(2) << telnet_per_second << " t/s)"
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "blacklist:"
                << fgls->m_BlackList.PktsRcvd << " packets"
                << " (" << fgls->m_BlackList.PktsRcvd / difftime << "/s)"
                << " / " << byte_counter (fgls->m_BlackList.BytesRcvd)
                << " (" << byte_counter ((double) fgls->m_BlackList.BytesRcvd / difftime) << "/s)"
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "relays:"
                << fgls->m_RelayList.PktsRcvd << " packets"
                << " (" << fgls->m_RelayList.PktsRcvd / difftime << "/s)"
                << " / " << byte_counter (fgls->m_RelayList.BytesRcvd)
                << " (" << byte_counter ((double) fgls->m_RelayList.BytesRcvd / difftime) << "/s)"
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "users:"
                << fgls->m_PlayerList.PktsRcvd << " packets"
                << " (" << fgls->m_PlayerList.PktsRcvd / difftime << "/s)"
                << " / " << byte_counter (fgls->m_PlayerList.BytesRcvd)
                << " (" << byte_counter ((double) fgls->m_PlayerList.BytesRcvd / difftime) << "/s)"
                << endl; if (check_pager()) return RESULT::OK;
        accumulated_sent        += fgls->m_CrossfeedList.BytesSent;
        accumulated_sent_pkts   += fgls->m_CrossfeedList.PktsSent;
        accumulated_sent        += fgls->m_RelayList.BytesSent;
        accumulated_sent_pkts   += fgls->m_RelayList.PktsSent;
        accumulated_sent        += fgls->m_PlayerList.BytesSent;
        accumulated_sent_pkts   += fgls->m_PlayerList.PktsSent;
        accumulated_rcvd        += fgls->m_BlackList.BytesRcvd;
        accumulated_rcvd_pkts   += fgls->m_BlackList.PktsRcvd;
        accumulated_rcvd        += fgls->m_RelayList.BytesRcvd;
        accumulated_rcvd_pkts   += fgls->m_RelayList.PktsRcvd;
        accumulated_rcvd        += fgls->m_PlayerList.BytesRcvd;
        accumulated_rcvd_pkts   += fgls->m_PlayerList.PktsRcvd;
        m_client << "Totals:" << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "sent:"
                << accumulated_sent_pkts << " packets"
                << " (" << accumulated_sent_pkts / difftime << "/s)"
                << " / " << byte_counter (accumulated_sent)
                << " (" << byte_counter ((double) accumulated_sent / difftime) << "/s)"
                << endl; if (check_pager()) return RESULT::OK;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "received:"
                << accumulated_rcvd_pkts << " packets"
                << " (" << accumulated_rcvd_pkts / difftime << "/s)"
                << " / " << byte_counter (accumulated_rcvd)
                << " (" << byte_counter ((double) accumulated_rcvd / difftime) << "/s)"
                << endl; if (check_pager()) return RESULT::OK;
        return (0);
} // fgls_cli::cmd_show_stats ()
#endif

//////////////////////////////////////////////////
/**
 *  @brief Show general settings
 */
RESULT
fgls_cli::cmd_show_settings
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        if ( first_arg < args.size() )
        {
                m_client << "<cr>" << cli_client::endl;
                return RESULT::OK;
        }
        std::string bind_addr;

        if ( fgls->m_bind_addr == "" )
                bind_addr = "*";
        else
                bind_addr = fgls->m_bind_addr;
        cmd_show_version (command, args, first_arg);
        using std::left;
        using std::setw;
        using std::setfill;
        m_client << cli_client::endl;
        m_client << "current settings:" << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "data port:" << fgls->m_data_port
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "query port:" << fgls->m_query_port
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "admin port:" << fgls->m_admin_port
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "logfile:" << fgls->m_logfile_name
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "bind address:" << bind_addr
                << cli_client::endl;
        return RESULT::OK;
} // fgls_cli::cmd_show_settings ()

//////////////////////////////////////////////////
/**
 *  @brief Shutdown the server
 */
RESULT
fgls_cli::cmd_die
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        if (first_arg > 0)
        {
                if (args[first_arg] == "?")
                {
                        m_client << "<cr>" << cli_client::endl;
                }
                return RESULT::OK;
        }
        fgls->m_want_exit = true;
        return RESULT::OK;
} // fgls_cli::cmd_die

//////////////////////////////////////////////////
/**
 *  @brief Show the uptime of the the server
 *         in a human readable form.
 */
RESULT
fgls_cli::cmd_show_uptime
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        if (first_arg > 0)
        {
                if (args[first_arg] == "?")
                {
                        m_client << "<cr>" << cli_client::endl;
                }
                return RESULT::OK;
        }
        m_client << "UP since " << timestamp_to_datestr(fgls->m_uptime)
                << "(" << timestamp_to_days(fgls->m_uptime) << ")" << cli_client::endl;
        return RESULT::OK;
} // fgls_cli::cmd_show_uptime

//////////////////////////////////////////////////
/**
 *  @brief Show the version number of the the server
 */
RESULT
fgls_cli::cmd_show_version
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        if (first_arg > 0)
        {
                if (args[first_arg] == "?")
                {
                        m_client << "<cr>" << cli_client::endl;
                }
                return RESULT::OK;
        }
        m_client << "This is " << fgls->m_server_name << cli_client::endl;
        m_client << "FlightGear List Server version "
               << fgls->m_version.str() << cli_client::endl; 
        m_client << "compiled on " << __DATE__ << " at " << __TIME__  << cli_client::endl;
        m_client << "using protocol version v ! TODO !" << cli_client::endl;
        cmd_show_uptime (command, args, first_arg);
        return RESULT::OK;
} // fgls_cli::cmd_show_version

