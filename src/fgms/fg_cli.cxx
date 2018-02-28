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
 * @file        fg_cli.cxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2013
 */

#include <sstream>
#include <fglib/fg_util.hxx>
#include "fg_cli.hxx"

namespace fgmp
{

using std::left;
using std::setw;
using std::setfill;
using std::setprecision;

//////////////////////////////////////////////////////////////////////

fgcli::fgcli
(
        fgmp::fgms* fgms,
        int fd
): cli(fd)
{
        this->fgms = fgms;
        this->setup ();
} // fgcli::fgcli ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Set up all commands
 *
 */
void
fgcli::setup
()
{
        command* c;

        //////////////////////////////////////////////////
        // general setup
        //////////////////////////////////////////////////
        set_hostname (this->fgms->m_server_name);
        std::stringstream banner;
        banner  << "\r\n"
                << "------------------------------------------------\r\n"
                << "FlightGear Multiplayer Server cli\r\n"
                << "This is "
                << fgms->m_server_name << " (" << fgms->m_FQDN << ")\r\n"
                << "------------------------------------------------\r\n";
        set_banner ( banner.str() );
        //////////////////////////////////////////////////
        // setup authentication (if required)
        //////////////////////////////////////////////////
        if (fgms->m_admin_user != "")
                allow_user ( fgms->m_admin_user, fgms->m_admin_pass );
        if (fgms->m_admin_enable != "")
                set_enable_password ( fgms->m_admin_enable );

        using namespace std::placeholders;
        #define _ptr(X) (std::bind (& X, this, _1, _2, _3))

        //////////////////////////////////////////////////
        // general commands
        //////////////////////////////////////////////////
        c = new command (
                "show",
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::EXEC,
                "show system information"
        );
        register_command (c);

        register_command ( new command (
                "stats",
                _ptr ( fgcli::cmd_show_stats ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show statistical information"
        ), c);

        register_command ( new command (
                "settings",
                _ptr ( fgcli::cmd_show_settings ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show general settings"
        ), c);

        register_command ( new command (
                "version",
                _ptr ( fgcli::cmd_show_version ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show running version information"
        ), c);

        register_command ( new command (
                "uptime",
                _ptr ( fgcli::cmd_show_uptime ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show uptime information"
        ), c);

        register_command ( new command (
                "log",
                _ptr ( fgcli::cmd_show_log ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show log buffer"
        ), c);

        register_command (new command (
                "whitelist",
                _ptr ( fgcli::cmd_whitelist_show ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show entries in the whitelist"
        ), c);

        register_command (new command (
                "blacklist",
                _ptr ( fgcli::cmd_blacklist_show ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show entries in the blacklist"
        ), c);

        register_command (new command (
                "crossfeeds",
                _ptr ( fgcli::cmd_crossfeed_show ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show entries in the crossfeeds"
        ), c);

        register_command (new command (
                "relay",
                _ptr ( fgcli::cmd_relay_show ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show list of relays"
        ), c);

        register_command (new command (
                "tracker",
                _ptr ( fgcli::cmd_tracker_show ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show status of tracker"
        ), c);

        register_command (new command (
                "users",
                _ptr ( fgcli::cmd_show_user ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show list of users"
        ), c);

        register_command (new command (
                "die",
                _ptr ( fgcli::cmd_die ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::EXEC,
                "force fgms to exit"
        ));

        //////////////////////////////////////////////////
        // modify blacklist
        //////////////////////////////////////////////////
        c = new command (
                "blacklist",
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "show/modify blacklist"
        );
        register_command (c);
        register_command (new command (
                "delete",
                _ptr ( fgcli::cmd_blacklist_delete ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "delete entries in the blacklist"
        ), c);
        register_command (new command (
                "add",
                _ptr ( fgcli::cmd_blacklist_add ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "Add entries to the blacklist"
        ), c);

        //////////////////////////////////////////////////
        // modify crossfeeds
        //////////////////////////////////////////////////
        c = new command (
                "crossfeed",
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "modify crossfeeds"
        );
        register_command (c);
        register_command (new command (
                "delete",
                _ptr ( fgcli::cmd_crossfeed_delete ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "delete crossfeeds"
        ), c);
        register_command (new command (
                "add",
                _ptr ( fgcli::cmd_crossfeed_add ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "Add crossfeeds"
        ), c);

        //////////////////////////////////////////////////
        // modify relays
        //////////////////////////////////////////////////
        c = new command (
                "relay",
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "modify relays"
        );
        register_command (c);
        register_command (new command (
                "delete",
                _ptr ( fgcli::cmd_relay_delete ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "delete relay"
        ), c);
        register_command (new command (
                "add",
                _ptr ( fgcli::cmd_relay_add ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "Add relay"
        ), c);

        //////////////////////////////////////////////////
        // modify whitelist
        //////////////////////////////////////////////////
        c = new command (
                "whitelist",
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "modify whitelist"
        );
        register_command (c);
        register_command (new command (
                "delete",
                _ptr ( fgcli::cmd_whitelist_delete ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "delete whitelist entry"
        ), c);
        register_command (new command (
                "add",
                _ptr ( fgcli::cmd_whitelist_add ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "Add entries to the whitelist"
        ), c);

        #undef _ptr

} // fgcli::setup ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show general statistics
 */
RESULT
fgcli::cmd_show_stats
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;

        time_t  difftime;
        time_t  now;
        uint64_t        accumulated_sent        = 0;
        uint64_t        accumulated_rcvd        = 0;
        uint64_t        accumulated_sent_pkts   = 0;
        uint64_t        accumulated_rcvd_pkts   = 0;

        now = time(0);
        difftime = now - fgms->m_uptime;
        cmd_show_version (command, args, first_arg);
        m_client << cli_client::endl;
        m_client  << "I have " << fgms->m_black_list.size ()
                << " entries in my blacklist"
                << cli_client::endl;
        m_client << "I have " << fgms->m_cross_list.size () << " crossfeeds"
                << cli_client::endl;
        m_client << "I have " << fgms->m_relay_list.size () << " relays"
                << cli_client::endl;
        m_client << "I have " << fgms->m_player_list.size () << " users ("
                << fgms->m_local_clients << " local, "
                << fgms->m_remote_clients << " remote, "
                << fgms->m_num_max_clients << " max)"
                << cli_client::endl;

        m_client << "Sent counters:" << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "to crossfeeds:"
                << fgms->m_cross_list.pkts_sent << " packets"
                << " (" << fgms->m_cross_list.pkts_sent / difftime << "/s)"
                << " / " << byte_counter (fgms->m_cross_list.bytes_sent)
                << " (" << byte_counter (
                  (double) fgms->m_cross_list.bytes_sent / difftime) << "/s)"
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "to relays:"
                << fgms->m_relay_list.pkts_sent << " packets"
                << " (" << fgms->m_relay_list.pkts_sent / difftime << "/s)"
                << " / " << byte_counter (fgms->m_relay_list.bytes_sent)
                << " (" << byte_counter (
                  (double) fgms->m_relay_list.bytes_sent / difftime) << "/s)"
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "to users:"
                << fgms->m_player_list.pkts_sent << " packets"
                << " (" << fgms->m_player_list.pkts_sent / difftime << "/s)"
                << " / " << byte_counter (fgms->m_player_list.bytes_sent)
                << " (" << byte_counter (
                  (double) fgms->m_player_list.bytes_sent / difftime) << "/s)"
                << cli_client::endl;

        m_client << "Receive counters:" << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "total:" <<  fgms->m_packets_received
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "pings:" <<  fgms->m_ping_received
                << " (" << fgms->m_pong_received << " pongs)"
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "errors:"
                << "invalid packets:" << fgms->m_packets_invalid
                << " rejected:" << fgms->m_black_rejected
                << " unknown relay:" << fgms->m_unknown_relay
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "valid data:"
                << "pos data:" << fgms->m_pos_data
                << " other:" << fgms->m_unknown_data
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "tracker:"
                << "connects:" << fgms->m_tracker_connect
                << " disconnects:" << fgms->m_tracker_disconnect
                << " positions:" << fgms->m_tracker_position
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "admin connections:" << fgms->m_admin_received
                << cli_client::endl;
        float telnet_per_second;
        if (fgms->m_queries_received)
                telnet_per_second = (float) fgms->m_queries_received /
                (time(0) - fgms->m_uptime);
        else
                telnet_per_second = 0;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "telnet connections: "
                << fgms->m_queries_received
                << " (" << setprecision(2) << telnet_per_second << " t/s)"
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "blacklist:"
                << fgms->m_black_list.pkts_rcvd << " packets"
                << " (" << fgms->m_black_list.pkts_rcvd / difftime << "/s)"
                << " / " << byte_counter (fgms->m_black_list.bytes_rcvd)
                << " (" << byte_counter (
                  (double) fgms->m_black_list.bytes_rcvd / difftime) << "/s)"
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "relays:"
                << fgms->m_relay_list.pkts_rcvd << " packets"
                << " (" << fgms->m_relay_list.pkts_rcvd / difftime << "/s)"
                << " / " << byte_counter (fgms->m_relay_list.bytes_rcvd)
                << " (" << byte_counter (
                  (double) fgms->m_relay_list.bytes_rcvd / difftime) << "/s)"
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "users:"
                << fgms->m_player_list.pkts_rcvd << " packets"
                << " (" << fgms->m_player_list.pkts_rcvd / difftime << "/s)"
                << " / " << byte_counter (fgms->m_player_list.bytes_rcvd)
                << " (" << byte_counter (
                  (double) fgms->m_player_list.bytes_rcvd / difftime) << "/s)"
                << cli_client::endl;
        accumulated_sent        += fgms->m_cross_list.bytes_sent;
        accumulated_sent_pkts   += fgms->m_cross_list.pkts_sent;
        accumulated_sent        += fgms->m_relay_list.bytes_sent;
        accumulated_sent_pkts   += fgms->m_relay_list.pkts_sent;
        accumulated_sent        += fgms->m_player_list.bytes_sent;
        accumulated_sent_pkts   += fgms->m_player_list.pkts_sent;
        accumulated_rcvd        += fgms->m_black_list.bytes_rcvd;
        accumulated_rcvd_pkts   += fgms->m_black_list.pkts_rcvd;
        accumulated_rcvd        += fgms->m_relay_list.bytes_rcvd;
        accumulated_rcvd_pkts   += fgms->m_relay_list.pkts_rcvd;
        accumulated_rcvd        += fgms->m_player_list.bytes_rcvd;
        accumulated_rcvd_pkts   += fgms->m_player_list.pkts_rcvd;
        m_client << "Totals:" << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "sent:"
                << accumulated_sent_pkts << " packets"
                << " (" << accumulated_sent_pkts / difftime << "/s)"
                << " / " << byte_counter (accumulated_sent)
                << " (" << byte_counter (
                  (double) accumulated_sent / difftime) << "/s)"
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "received:"
                << accumulated_rcvd_pkts << " packets"
                << " (" << accumulated_rcvd_pkts / difftime << "/s)"
                << " / " << byte_counter (accumulated_rcvd)
                << " (" << byte_counter (
                  (double) accumulated_rcvd / difftime) << "/s)"
                << cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_show_stats ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show general settings
 */
RESULT
fgcli::cmd_show_settings
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;

        std::string bind_addr;
        if ( fgms->m_bind_addr == "" )
                bind_addr = "*";
        else
                bind_addr = fgms->m_bind_addr;
        cmd_show_version (command, args, first_arg);
        m_client << cli_client::endl;
        m_client << "current settings:" << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "listen port:" << fgms->m_data_port
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "telnet port:" << fgms->m_query_port
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "admin port:" << fgms->m_admin_port
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "player expires:" << fgms->m_player_expires
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "out of reach:" << fgms->m_out_of_reach
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "radar range:" << fgms->m_max_radar_range
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "logfile:" << fgms->m_logfile_name
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "bind address:" << bind_addr
                << cli_client::endl;
        m_client << "  " << left << setfill(' ') << setw(22)
                << "FQDN:" << fgms->m_FQDN
                << cli_client::endl;

        return RESULT::OK;
} // fgcli::cmd_show_settings ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Shutdown the server
 */
RESULT
fgcli::cmd_die
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        fgms->m_want_exit = true;
        return RESULT::OK;
} // fgcli::cmd_die

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show the uptime of the the server
 *         in a human readable form.
 */
RESULT
fgcli::cmd_show_uptime
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << "UP since " << timestamp_to_datestr(fgms->m_uptime)
                << "(" << timestamp_to_days(fgms->m_uptime) << ")"
                << cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_show_uptime

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show log buffer of the the server
 */
RESULT
fgcli::cmd_show_log
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        fgmp::str_list*  buf = logger.logbuf();
        fgmp::str_it     it;
        buf->lock ();
        m_client.enable_filters ();
        try
        {
                for ( it = buf->begin(); it != buf->end(); it++ )
                {
                        m_client << *it << cli_client::endl;
                }
        }
        catch ( pager_wants_quit ) { /* do nothing */ }
        buf->unlock ();
        m_client.disable_filters ();
        return RESULT::OK;
} // fgcli::cmd_show_uptime

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show the version number of the the server
 */
RESULT
fgcli::cmd_show_version
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        std::string s;
        if (fgms->m_me_is_hub)
                s = "HUB";
        else
                s = "LEAVE";
        m_client << "This is " << fgms->m_server_name
                << " (" << fgms->m_FQDN << ")"
                << cli_client::endl;
        m_client << "FlightGear Multiplayer " << s << " Server version "
               << fgms->m_version.str() << cli_client::endl; 
        m_client << "using protocol version v"
                << fgms->m_proto_major_version << "."
                << fgms->m_proto_minor_version << cli_client::endl;
        if (fgms->m_is_tracked)
                m_client << "This server is tracked: "
                        << fgms->m_tracker->get_server () << cli_client::endl;
        else
                m_client << "This server is NOT tracked" << cli_client::endl;
        cmd_show_uptime (command, args, first_arg);
        return RESULT::OK;
} // fgcli::cmd_show_version

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show Whitelist
 *
 *  possible arguments:
 *  show whitelist ?
 *  show whitelist <cr>
 *  show whitelist id
 *  show whitelist IP-address
 */
RESULT
fgcli::cmd_whitelist_show
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        size_t id { 0 };
        bool   brief { false };
        fgmp::netaddr address;

        if ( num_args > 1 )
                return no_more_args ( args, first_arg + 1 );
        if ( num_args == 1 )
        {
                if ( wants_help ( args[first_arg] ) )
                {
                        show_help ( "id", "show entry with id" );
                        show_help ( "IP", "show entry with IP-address");
                        show_help ( "brief", "show brief listing");
                        show_help ( "<cr>", "show all entries" );
                        show_help ( "|", "output modifier" );
                        return RESULT::OK;
                }
                if ( compare ( args[first_arg], "brief" ) )
                        brief = true;
                else
                {
                        int id_invalid { -1 };
                        id  = str_to_num<size_t>( args[first_arg], id_invalid );
                        if ( id_invalid )
                        {
                                id = 0;
                                address.assign ( args[first_arg], 0 );
                                if ( ! address.is_valid() )
                                        return RESULT::INVALID_ARG;
                        }
                }
        }
        size_t EntriesFound { 0 };
        size_t Count = fgms->m_white_list.size ();
        fgmp::list_item Entry("");
        m_client << cli_client::endl;
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - fgms->m_uptime;
        m_client << fgms->m_white_list.name << ":" << cli_client::endl;
        m_client << cli_client::endl;
        for (size_t i = 0; i < Count; i++ )
        {
                Entry = fgms->m_white_list[i];
                // only list matching entries
                if ( address.is_valid() )
                {
                        if (Entry.address != address)
                                continue;
                }
                else if (id)
                {
                        if (Entry.id != id)
                                continue;
                }
                EntriesFound++;
                m_client << "id " << Entry.id << ": "
                        << Entry.address.to_string() << " : " << Entry.name
                        << cli_client::endl;
                if ( ! brief )
                {
                        m_client << "  entered      : "
                                << timestamp_to_datestr (Entry.join_time)
                                <<cli_client::endl;
                        m_client << "  last seen    : "
                                << timestamp_to_days (Entry.last_seen)
                                <<cli_client::endl;
                        m_client << "  rcvd packets : " << Entry.pkts_rcvd
                                <<cli_client::endl;
                        m_client << "  rcvd bytes   : "
                                << byte_counter (Entry.bytes_rcvd)
                                <<cli_client::endl;
                }
        }
        m_client << EntriesFound << " entries found" <<cli_client::endl;
        if ( (EntriesFound) && ( ! brief ) )
        {
                m_client <<cli_client::endl;
                m_client << "Total rcvd: "
                        << fgms->m_white_list.pkts_rcvd << " packets"
                        << " (" << fgms->m_white_list.pkts_rcvd / difftime
                        << "/s)"
                        << " / " << byte_counter (fgms->m_white_list.bytes_rcvd)
                        << " (" << byte_counter (
                          (double) (fgms->m_white_list.bytes_rcvd/difftime))
                        << "/s)"
                        <<cli_client::endl;
        }
        return RESULT::OK;
} // fgcli::cmd_whitelist_show

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_whitelist_delete
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        fgmp::netaddr address;
        size_t id;

        if ( num_args == 0 )
                return RESULT::MISSING_ARG;
        else if ( num_args == 1 )
        {
                if ( wants_help ( args[first_arg] ) )
                {
                        show_help ( "id", "delete entry with id" );
                        show_help ( "IP", "delete entry with IP-address" );
                        return RESULT::OK;
                }
                int id_invalid;
                id = str_to_num<size_t> ( args[first_arg], id_invalid );
                if ( id_invalid )
                {
                        id = 0;
                        address.assign ( args[first_arg], 0 );
                        if ( ! address.is_valid() )
                                return RESULT::INVALID_ARG;
                }
        }
        else return no_more_args ( args, first_arg + 1 );
        if ( ( id == 0 ) && ( ! address.is_valid() ) )
        {
                return RESULT::MISSING_ARG;;
        }
        fglistit Entry;
        if ( (id == 0) && (address.is_valid()) )
        {       // match IP
                Entry = fgms->m_white_list.find (address);
                if (Entry != fgms->m_white_list.end())
                        fgms->m_white_list.erase (Entry);
                else
                        m_client << "no entry found!" <<cli_client::endl;
                return RESULT::OK;
        }
        Entry = fgms->m_white_list.find_by_id (id);
        if (Entry != fgms->m_white_list.end())
        {
                fgms->m_white_list.erase (Entry);
                m_client << "deleted!" <<cli_client::endl;
        }
        else
                m_client << "no entry found!" <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_whitelist_delete

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Add Whitelist entry
 *
 *  ONLY in config mode
 *
 *  possible arguments:
 *  blacklist add ?
 *  blacklist add TTL IP-address [reason]
 */
RESULT
fgcli::cmd_whitelist_add
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        fgmp::netaddr address;
        std::string   Reason;
        time_t TTL { -1 };
        int    I;

        size_t arg_num { 0 };
        for ( size_t i=first_arg; i < args.size(); i++ )
        {
                switch ( arg_num )
                {
                case 0: // must be TTL or '?'
                        if ( wants_help ( args[i] ) )
                        {
                                show_help ( "TTL",
                                  "timeout of the new entry in seconds" );
                                return RESULT::OK;
                        }
                        TTL = str_to_num<size_t> ( args[i], I );
                        if (I)
                        {
                                m_client << "% invalid TTL" <<cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                        break;
                case 1: // IP or '?'
                        if ( wants_help ( args[i] ) )
                        {
                                show_help ( "IP",
                                  "IP address which should be whitelisted" );
                                return RESULT::OK;
                        }
                        address.assign ( args[i], 0 );
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                        <<  cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                        break;
                case 2: // reason or '?'
                        if ( wants_help ( args[i] ) )
                        {
                                show_help ( "STRING",
                                  "a reason for this whitelist entry" );
                                show_help ( "<cr>", "add this IP" );
                                return RESULT::OK;
                        }
                        Reason = args[i];
                        break;
                default:
                        return no_more_args ( args, i );
                }
                arg_num++;
        }
        if ( ( TTL == -1 ) || ( ! address.is_valid() ) )
                return RESULT::MISSING_ARG;
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
                m_client << "% entry already exists (id "
                        << CurrentEntry->id << ")!"
                        << cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "added with id " << Newid <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_whitelist_add

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_blacklist_show
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        fgmp::netaddr   address;
        bool   brief = false;
        size_t EntriesFound = 0;
        size_t id = 0;

        if ( num_args > 1 )
                return no_more_args ( args, first_arg + 1 );
        if ( num_args == 1 )
        {
                if ( wants_help ( args[first_arg] ) )
                {
                        show_help ( "brief", "show brief listing" );
                        show_help ( "id", "show entry with id" );
                        show_help ( "IP","show entry with IP-address" );
                        show_help ( "<cr>", "show long listing" );
                        show_help ( "|", "output modifier" );
                        return RESULT::OK;
                }
                else if ( compare ( args[first_arg], "brief" ) )
                        brief = true;
                else
                {
                        int e;
                        id  = str_to_num<size_t>( args[first_arg], e );
                        if ( e )
                        {
                                id = 0;
                                address.assign ( args[first_arg], 0 );
                                if (! address.is_valid() )
                                        return RESULT::INVALID_ARG;
                        }
                }
        }
        int Count = fgms->m_black_list.size ();
        fgmp::list_item Entry("");
        m_client <<cli_client::endl;
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - fgms->m_uptime;
        m_client << fgms->m_black_list.name << ":" <<cli_client::endl;
        m_client <<cli_client::endl;
        for (int i = 0; i < Count; i++)
        {
                Entry = fgms->m_black_list[i];
                if ( (id == 0) && (address.is_valid()) )
                {       // only list matching entries
                        if (Entry.address != address)
                                continue;
                }
                else if (id)
                {
                        if (Entry.id != id)
                                continue;
                }
                EntriesFound++;
                m_client << "id " << Entry.id << ": "
                        << Entry.address.to_string() << " : " << Entry.name
                        <<cli_client::endl;
                if (brief == true)
                {
                        continue;
                }
                std::string expire = "NEVER";
                if (Entry.timeout != 0)
                {
                        expire = num_to_str (Entry.timeout, 0) + " seconds";
                }
                m_client << "  entered      : "
                        << timestamp_to_datestr (Entry.join_time)
                        << cli_client::endl;
                m_client << "  last seen    : "
                        << timestamp_to_days (Entry.last_seen)
                        << cli_client::endl;
                m_client << "  rcvd packets : "
                        << Entry.pkts_rcvd
                        << cli_client::endl;
                m_client << "  rcvd bytes   : "
                        << byte_counter (Entry.bytes_rcvd)
                        << cli_client::endl;
                m_client << "  expire in    : "
                        << expire
                        << cli_client::endl;
        }
        if (EntriesFound)
        {
                m_client <<cli_client::endl;
        }
        m_client << EntriesFound << " entries found" <<cli_client::endl;
        if (EntriesFound)
        {
                m_client << "Total rcvd: "
                        << fgms->m_black_list.pkts_rcvd << " packets"
                        << " (" << fgms->m_black_list.pkts_rcvd / difftime
                        << "/s) / "
                        << byte_counter (fgms->m_black_list.bytes_rcvd)
                        << " (" << byte_counter (
                          (double) (fgms->m_black_list.bytes_rcvd/difftime))
                        << "/s)"
                        <<cli_client::endl;
        }
        return RESULT::OK;
} // fgcli::cmd_blacklist_show

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_blacklist_delete
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        fgmp::netaddr   address;
        fglistit        Entry;
        size_t          id;

        if ( num_args == 0 )
                return RESULT::MISSING_ARG;
        if ( num_args == 1 )
        {
                if ( wants_help ( args[first_arg] ) )
                {
                        show_help ( "id", "delete entry with id" );
                        show_help ( "IP", "delete entry with IP address" );
                        return RESULT::OK;
                }
                int id_invalid;
                id  = str_to_num<size_t>( args[first_arg], id_invalid );
                if ( id_invalid )
                {
                        id = 0;
                        address.assign (args[first_arg], 0);
                        if (! address.is_valid())
                                return RESULT::INVALID_ARG;
                }
        }
        else return no_more_args ( args, first_arg + 1 );
        if ( (id == 0) && (! address.is_valid()) )
                return RESULT::MISSING_ARG;;
        if ( address.is_valid() )
        {       // match IP
                Entry = fgms->m_black_list.find (address);
                if (Entry != fgms->m_black_list.end())
                        fgms->m_black_list.erase (Entry);
                else
                        m_client << "no entry found!" <<cli_client::endl;
                return RESULT::OK;
        }
        Entry = fgms->m_black_list.find_by_id (id);
        if (Entry != fgms->m_black_list.end())
                fgms->m_black_list.erase (Entry);
        else
                m_client << "no entry found!" <<cli_client::endl;
        m_client << "deleted!" <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_blacklist_delete

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_blacklist_add
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        time_t         TTL = -1;
        int            I;
        fgmp::netaddr  address;
        std::string    Reason;
        fglistit       Entry;
        size_t arg_num { 0 };

        for ( size_t i=first_arg; i < args.size(); i++ )
        {
                switch ( arg_num )
                {
                case 0: // must be TTL or '?'
                        if ( wants_help (args[i]) )
                        {
                                show_help ( "TTL",
                                  "timeout of the new entry in seconds" );
                                return RESULT::OK;
                        }
                        TTL  = str_to_num<size_t> ( args[i], I );
                        if (I)
                        {
                                m_client << "% invalid TTL" <<cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                        break;
                case 1: // IP or '?'
                        if ( wants_help (args[i]) )
                        {
                                show_help ( "IP",
                                  "IP address which should be blacklisted" );
                                return RESULT::OK;
                        }
                        address.assign (args[i], 0);
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                        <<  cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                        break;
                case 2: // reason or '?'
                        if ( wants_help ( args[i] ) )
                        {
                                show_help ( "STRING",
                                  "a reason for this blacklist entry" );
                                show_help ( "<cr>", "add this IP" );
                                return RESULT::OK;
                        }
                        Reason = args[i];
                        break;
                default:
                        return no_more_args ( args, i );
                }
                arg_num++;
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
                m_client << "% entry already exists (id "
                        << CurrentEntry->id << ")!"
                        <<cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "added with id " << Newid <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_blacklist_add

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_crossfeed_delete
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        fgmp::netaddr   address;
        size_t id = 0;

        if ( num_args == 0 )
                return RESULT::MISSING_ARG;
        if ( num_args == 1 )
        {
                if ( wants_help (args[first_arg]) )
                {
                        show_help ( "id", "delete entry with id" );
                        show_help ( "IP",
                          "delete entry with IP address" );
                        return RESULT::OK;
                }
                int id_invalid;
                id  = str_to_num<size_t>( args[first_arg], id_invalid );
                if ( id_invalid )
                {
                        id = 0;
                        address.assign ( args[first_arg], 0 );
                        if ( ! address.is_valid() )
                                return RESULT::INVALID_ARG;
                }
        }
        if ( (id == 0) && (! address.is_valid()) )
                return RESULT::MISSING_ARG;

        fglistit Entry;
        if ( (id == 0) && ( address.is_valid() ) )
        {       // match IP
                Entry = fgms->m_cross_list.find (address);
                if (Entry != fgms->m_cross_list.end())
                {
                        fgms->m_cross_list.erase (Entry);
                }
                else
                {
                        m_client << "no entry found" <<cli_client::endl;
                        return RESULT::OK;
                }
                return RESULT::OK;
        }
        Entry = fgms->m_cross_list.find_by_id (id);
        if (Entry != fgms->m_cross_list.end())
        {
                fgms->m_cross_list.erase (Entry);
        }
        else
        {
                m_client << "no entry found" <<cli_client::endl;
                return RESULT::OK;
        }
        m_client << "deleted" <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_crossfeed_delete

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_crossfeed_add
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        fgmp::netaddr address;
        std::string   name;
        int           port { 0 };
        size_t arg_num  { 0 };

        for ( size_t i=first_arg; i < args.size(); i++ )
        {
                switch ( arg_num )
                {
                case 0: // IP or '?'
                        if ( wants_help (args[i]) )
                        {
                                show_help ("IP","IP address of the crossfeed");
                                return RESULT::OK;
                        }
                        address.assign (args[i], 0);
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                  << cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                        break;
                case 1: // Port or '?'
                        if ( wants_help (args[i]) )
                        {
                                show_help ( "Port", "Port of the relay" );
                                return RESULT::OK;
                        }
                        int I;
                        port  = str_to_num<int> ( args[i], I );
                        if (I)
                        {
                                m_client << "% invalid port " << port
                                  << cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                        break;
                case 2: // Name of the crossfeed
                        if ( wants_help (args[i]) )
                        {
                                show_help ( "NAME",
                                          "The name of this crossfeed" );
                                return RESULT::OK;
                        }
                        name = args[i];
                        break;
                default:
                        return no_more_args ( args, i );
                }
                arg_num++;
        }
        if ( ( ! address.is_valid() ) || ( port == 0 ) )
                return RESULT::MISSING_ARG;
        fgmp::list_item E (name);
        E.address = address;
        E.address.port (port);
        size_t Newid;
        fglistit CurrentEntry = fgms->m_cross_list.find ( E.address, true );
        if ( CurrentEntry == fgms->m_cross_list.end() )
        {
                Newid = fgms->m_cross_list.add (E, port);
        }
        else
        {
                m_client << "entry already exists (id "
                        << CurrentEntry->id << ")"
                        << cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "added with id " << Newid <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_crossfeed_add

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show Crossfeed
 *
 *  possible arguments:
 *  show crossfeed ?
 *  show crossfeed <cr>
 *  show crossfeed id
 *  show crossfeed IP-address
 *  show crossfeed [...] brief
 */
RESULT
fgcli::cmd_crossfeed_show
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        size_t          id = 0;
        fgmp::netaddr   address;
        bool            brief = false;

        if ( num_args > 1 )
                return no_more_args ( args, first_arg + 1 );
        if ( num_args == 1 )
        {
                if ( wants_help ( args[first_arg]) )
                {
                        show_help ( "brief", "show brief listing" );
                        show_help ( "id", "show entry with id" );
                        show_help ( "IP","show entry with IP-address" );
                        show_help ( "<cr>", "show long listing" );
                        show_help ( "|", "output modifier" );
                        return RESULT::OK;
                }
                else if ( compare ( args[first_arg], "brief" ) )
                {
                        brief = true;
                }
                else
                {
                        int id_invalid = -1;
                        id  = str_to_num<size_t> ( args[first_arg],id_invalid );
                        if (id_invalid)
                        {
                                id = 0;
                                address.assign (args[first_arg], 0);
                                if (! address.is_valid())
                                        return RESULT::INVALID_ARG;
                        }
                }
        }
        size_t EntriesFound = 0;
        int Count = fgms->m_cross_list.size ();
        fgmp::list_item Entry("");
        m_client << fgms->m_cross_list.name << ":" <<cli_client::endl;
        m_client <<cli_client::endl;
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - fgms->m_uptime;
        for (int i = 0; i < Count; i++)
        {
                Entry = fgms->m_cross_list[i];
                if ( (id == 0) && (address.is_valid()) )
                {       // only list matching entries
                        if (Entry.address != address)
                                continue;
                }
                else if (id)
                {
                        if (Entry.id != id)
                                continue;
                }
                EntriesFound++;
                m_client << "id " << Entry.id << ": "
                        << Entry.address.to_string() << ":"
                        << Entry.address.port()
                        << " : " << Entry.name
                        << cli_client::endl;
                if (brief == true)
                {
                        continue;
                }
                m_client << "  entered      : "
                        << timestamp_to_datestr (Entry.join_time)
                        << cli_client::endl;
                m_client << "  last sent    : "
                        << timestamp_to_days (Entry.last_sent)
                        << cli_client::endl;
                m_client << "  sent packets : " << Entry.pkts_sent
                        << "(" << (double) (Entry.pkts_sent / difftime)
                        << " packets/s)"
                        << cli_client::endl;
                m_client << "  sent bytes   : "
                        << byte_counter (Entry.bytes_sent)
                        << "(" << byte_counter (
                          (double) Entry.bytes_sent / difftime) << "/s)"
                        << cli_client::endl;
        }
        if (EntriesFound)
        {
                m_client <<cli_client::endl;
        }
        m_client << EntriesFound << " entries found" <<cli_client::endl;
        if (EntriesFound)
        {
                m_client << "Total sent: "
                        << fgms->m_cross_list.pkts_sent << " packets"
                        << "(" << fgms->m_cross_list.pkts_sent / difftime
                        << "/s) / "
                        << byte_counter (fgms->m_cross_list.bytes_sent)
                        << "(" << byte_counter (
                          (double) (fgms->m_cross_list.bytes_sent/difftime))
                        << "/s)"
                        <<cli_client::endl;
        }
        return RESULT::OK;
} // fgcli::cmd_crossfeed_show

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_relay_show
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        size_t          id = 0;
        fgmp::netaddr   address;
        bool            brief = false;
        std::string     name;

        if ( num_args > 1 )
                return no_more_args ( args, first_arg + 1 );
        if ( num_args == 1 )
        {
                if ( wants_help (args[first_arg]) )
                {
                        show_help ( "brief", "show brief listing" );
                        show_help ( "id", "show entry with id" );
                        show_help ( "IP", "show entry with IP-address");
                        show_help ( "NAME", "show user with NAME" );
                        show_help ( "<cr>", "show log listing" );
                        show_help ( "|", "output modifier" );
                        return RESULT::OK;
                }
                else if ( compare ( args[first_arg], "brief" ) )
                        brief = true;
                else
                {
                        int id_invalid = -1;
                        id = str_to_num<size_t> ( args[first_arg], id_invalid );
                        if (id_invalid)
                        {
                                id = 0;
                                address.assign (args[first_arg], 0);
                                if (! address.is_valid())
                                {
                                        name = args[first_arg];
                                }
                        }
                }
        }
        size_t EntriesFound = 0;
        int Count = fgms->m_relay_list.size ();
        fgmp::list_item Entry("");
        m_client << fgms->m_relay_list.name << ":" <<cli_client::endl;
        m_client <<cli_client::endl;
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - fgms->m_uptime;
        for (int i = 0; i < Count; i++)
        {
                Entry = fgms->m_relay_list[i];
                if ( (id == 0) && (address.is_valid()) )
                {       // only list matching entries
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
                        if (Entry.name.find(name) == std::string::npos)
                                continue;
                }
                EntriesFound++;
                m_client << "id " << Entry.id
                        << ": " << Entry.address.to_string()
                        << ":" << Entry.address.port()
                        << " : " << Entry.name
                        << cli_client::endl;
                if (brief == true)
                {
                        continue;
                }
                m_client << "  entered   : "
                        << timestamp_to_datestr (Entry.join_time)
                        << cli_client::endl;
                m_client << "  last seen : "
                        << timestamp_to_datestr (Entry.last_seen)
                        << cli_client::endl;
                m_client << "  sent      : "
                        << Entry.pkts_sent << " packets"
                        << " (" << Entry.pkts_sent / difftime << "/s)"
                        << " / " << byte_counter (Entry.bytes_sent)
                        << " (" << byte_counter (
                          (double) Entry.bytes_sent / difftime) << "/s)"
                        <<cli_client::endl;
                m_client << "  rcvd      : "
                        << Entry.pkts_rcvd << " packets"
                        << " (" << Entry.pkts_rcvd / difftime << "/s)"
                        << " / " << byte_counter (Entry.bytes_rcvd)
                        << " (" << byte_counter (
                          (double) Entry.bytes_rcvd / difftime) << "/s)"
                        << cli_client::endl;
        }
        m_client <<cli_client::endl;
        m_client << EntriesFound << " entries found"
                <<cli_client::endl;
        if (brief)
        {
                return RESULT::OK;
        }
        m_client << "Totals:" <<cli_client::endl;
        m_client << "  sent      : "
                << fgms->m_relay_list.pkts_sent << " packets"
                << " (" << fgms->m_relay_list.pkts_sent / difftime << "/s)"
                << " / " << byte_counter (fgms->m_relay_list.bytes_sent)
                << " (" << byte_counter (
                  (double) fgms->m_relay_list.bytes_sent / difftime) << "/s)"
                <<cli_client::endl;
        m_client << "  received  : "
                << fgms->m_relay_list.pkts_rcvd << " packets"
                << " (" << fgms->m_relay_list.pkts_rcvd / difftime << "/s)"
                << " / " << byte_counter (fgms->m_relay_list.bytes_rcvd)
                << " (" << byte_counter (
                  (double) fgms->m_relay_list.bytes_rcvd / difftime) << "/s)"
                <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_relay_show

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show status of tracker server
 *
 *  possible arguments:
 *  show tracker ?
 *  show tracker <cr>
 */
RESULT
fgcli::cmd_tracker_show
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };

        if ( num_args > 0 )
                return no_more_args ( args, first_arg + 1 );
        if (! fgms->m_is_tracked)
        {
                m_client << "This server is NOT tracked" << cli_client::endl;
                m_client << cli_client::endl;
                return RESULT::OK;
        }
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - fgms->m_uptime;
        m_client << "This server is tracked: "
                << fgms->m_tracker->get_server () << ":"
                << fgms->m_tracker->get_port()
                <<cli_client::endl;
        if (fgms->m_tracker->is_connected ())
        {
                m_client << "state: connected since "
                        << timestamp_to_datestr(fgms->m_tracker->last_connected)
                        << " ("
                        << timestamp_to_days (fgms->m_tracker->last_connected)
                        << " ago)"
                        <<cli_client::endl;
        }
        else
        {
                m_client << "state: NOT connected!" <<cli_client::endl;
        }
        std::string A = "NEVER";
        if (fgms->m_tracker->last_seen != 0)
        {
                A = timestamp_to_days (fgms->m_tracker->last_seen);
                A += " ago";
        }
        std::string B = "NEVER";
        if (fgms->m_tracker->last_sent != 0)
        {
                B = timestamp_to_days (fgms->m_tracker->last_sent);
                B += " ago";
        }
        m_client << "last seen " << A << ", last sent " << B <<cli_client::endl;
        m_client << "I had " << fgms->m_tracker->lost_connections
          << " lost connections" <<cli_client::endl;
        m_client <<cli_client::endl;
        m_client << "Counters:" <<cli_client::endl;
        m_client << "  sent    : " << fgms->m_tracker->pkts_sent << " packets";
        m_client << " (" << fgms->m_tracker->pkts_sent / difftime << "/s)";
        m_client << " / " << byte_counter (fgms->m_tracker->bytes_sent);
        m_client << " (" << byte_counter (
          (double) fgms->m_tracker->bytes_sent / difftime) << "/s)";
        m_client <<cli_client::endl;
        m_client << "  received: " << fgms->m_tracker->pkts_rcvd << " packets";
        m_client << " (" << fgms->m_tracker->pkts_rcvd / difftime << "/s)";
        m_client << " / " << byte_counter (fgms->m_tracker->bytes_rcvd);
        m_client << " (" << byte_counter (
          (double) fgms->m_tracker->bytes_rcvd / difftime) << "/s)";
        m_client <<cli_client::endl;
        m_client << "  queue size: " << fgms->m_tracker->queue_size ()
          << " messages" <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_tracker_show

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_relay_delete
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        size_t          id = 0;
        int             id_invalid = -1;
        fgmp::netaddr   address;

        if ( num_args > 1 )
                return no_more_args ( args, first_arg + 1 );
        if ( num_args == 1 )
        {
                if ( wants_help ( args[first_arg]) )
                {
                        show_help ( "id", "delete entry with id" );
                        show_help ( "IP", "delete entry with IP address" );
                        return RESULT::OK;
                }
                id = str_to_num<size_t> ( args[first_arg], id_invalid );
                if (id_invalid)
                {
                        id = 0;
                        address.assign (args[first_arg], 0);
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                  << cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                }
        }
        if ( (id == 0) && (! address.is_valid()) )
                return RESULT::MISSING_ARG;
        fglistit Entry;
        if ( (id == 0) && (address.is_valid()) )
        {       // match IP
                Entry = fgms->m_relay_list.find (address);
                if (Entry != fgms->m_relay_list.end())
                {
                        fgms->m_relay_list.erase (Entry);
                }
                else
                {
                        m_client << "no entry found" <<cli_client::endl;
                        return RESULT::ERROR_ANY;
                }
                return RESULT::OK;
        }
        Entry = fgms->m_relay_list.find_by_id (id);
        if (Entry != fgms->m_relay_list.end())
        {
                fgms->m_relay_list.erase (Entry);
        }
        else
        {
                m_client << "no entry found" <<cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "deleted" <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_relay_delete

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_relay_add
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        fgmp::netaddr   address;
        std::string     name;
        int             Port;
        int             I;
        size_t arg_num { 0 };

        for ( size_t i=first_arg; i < args.size(); i++ )
        {
                switch ( arg_num )
                {
                case 0: // IP or '?'
                        if ( wants_help (args[i]) )
                        {
                                show_help ( "IP", "IP address of the relay" );
                                return RESULT::OK;
                        }
                        address.assign (args[i], 0);
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                  << cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                        break;
                case 1: // Port or '?'
                        if ( wants_help (args[i]) )
                        {
                                show_help ( "Port", "Port of the relay" );
                                return RESULT::OK;
                        }
                        Port  = str_to_num<int> ( args[i], I );
                        if (I)
                        {
                                m_client << "% invalid port "
                                  << Port << cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                        break;
                case 2: // name
                        if ( wants_help (args[i]) )
                        {
                                show_help ( "NAME", "the name of this relay" );
                                return RESULT::OK;
                        }
                        name = args[i];
                        break;
                default:
                        return no_more_args ( args, i );
                }
                arg_num++;
        }
        fglistit Entry;
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
                m_client << "entry already exists (id "
                        << CurrentEntry->id << ")"
                        << cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "added with id " << Newid <<cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_relay_add

//////////////////////////////////////////////////////////////////////

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
RESULT
fgcli::cmd_show_user
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        size_t          id = 0;
        fgmp::netaddr   address;
        std::string     name;
        bool            brief = false;

        if ( num_args > 1 )
                return no_more_args ( args, first_arg + 1 );
        if ( num_args == 1 )
        {
                if ( wants_help ( args[first_arg]) )
                {
                        show_help ( "brief", "show brief listing" );
                        show_help ( "id", "show user with id" );
                        show_help ( "IP", "show user with IP-address" );
                        show_help ( "NAME", "show user with NAME" );
                        show_help ( "local", "show only local users" );
                        show_help ( "remote", "show only local users" );
                        show_help ( "<cr>", "show long listing" );
                        show_help ( "|", "output modifier" );
                        return RESULT::OK;
                }
                else if ( compare ( args[first_arg], "brief" ) )
                        brief = true;
                else
                {
                        int e = -1;
                        id  = str_to_num<size_t> ( args[first_arg], e );
                        if ( e )
                        {
                                id = 0;
                                address.assign (args[first_arg], 0);
                                if (! address.is_valid())
                                        name = args[first_arg];
                        }
                }
        }
        bool            OnlyLocal = false;
        bool            OnlyRemote = false;
        size_t          EntriesFound = 0;
        time_t          difftime;
        time_t          now = time(0);
        int Count = fgms->m_player_list.size ();
        pilot   Player;
        point3d PlayerPosGeod;
        std::string  Origin;
        std::string  Fullname;
        m_client << fgms->m_player_list.name << ":" <<cli_client::endl;
        m_client <<cli_client::endl;
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
                {       // only list matching entries
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
                        if (Player.name.find(name) == std::string::npos)
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
                        fgms::ip2relay_it Relay =
                          fgms->m_relay_map.find ( Player.address );
                        if ( Relay != fgms->m_relay_map.end() )
                                Origin = Relay->second;
                        else
                                Origin = Player.origin;
                }
                Fullname = Player.name + std::string("@") + Origin;
                std::string ATC;
                using fgmp::ATC_TYPE;
                switch ( Player.is_ATC )
                {
                case ATC_TYPE::NONE:    ATC = ", a normal pilot"; break;
                case ATC_TYPE::ATC:     ATC = ", an ATC"; break;
                case ATC_TYPE::ATC_DL:  ATC = ", a clearance delivery ATC"; break;
                case ATC_TYPE::ATC_GN:  ATC = ", a ground ATC"; break;
                case ATC_TYPE::ATC_TW:  ATC = ", a tower ATC"; break;
                case ATC_TYPE::ATC_AP:  ATC = ", an approach ATC"; break;
                case ATC_TYPE::ATC_DE:  ATC = ", a departure ATC"; break;
                case ATC_TYPE::ATC_CT:  ATC = ", a center ATC"; break;
                }
                m_client << "id " << Player.id << ": "
                        << Fullname << ATC
                        <<cli_client::endl;
                EntriesFound++;
                if (brief == true)
                {
                        continue;
                }
                if (Player.has_errors == true)
                {
                        m_client << "         "
                                << left << setfill(' ') << setw(15)
                                << "ERROR" << Player.error
                                <<cli_client::endl;
                }
                m_client << "         "
                        << left << setfill(' ') << setw(15)
                        << "protocoll" << Player.proto_major
                        << "." << Player.proto_minor
                        << cli_client::endl;
                m_client << "         " <<
                        left << setfill(' ') << setw(15)
                        << "radar range" << Player.radar_range
                        << cli_client::endl;
                int expires = Player.timeout - (now - Player.last_seen);
                m_client << "         " << left << setfill(' ') << setw(15)
                        << "entered" << timestamp_to_days (Player.join_time)
                        << " ago" << cli_client::endl;
                m_client << "         " << left << setfill(' ') << setw(15)
                        << "joined" << timestamp_to_datestr(Player.join_time)
                        << cli_client::endl;
                m_client << "         " << left << setfill(' ') << setw(15)
                        << "last seen" << timestamp_to_datestr(Player.last_seen)
                        << cli_client::endl;
                m_client << "         " << left << setfill(' ') << setw(15)
                        << "using model" << Player.model_name
                        << cli_client::endl;
                if (Player.is_local)
                {
                        m_client << "         "
                                << left << setfill(' ') << setw(15)
                                << "real origin" << Player.origin
                                <<cli_client::endl;
                        m_client << "         "
                                << left << setfill(' ') << setw(15)
                                << "sent" << Player.pkts_sent << " packets "
                                << "(" << Player.pkts_sent / difftime << "/s)"
                                << " / " << byte_counter (Player.bytes_sent)
                                << " (" << byte_counter (
                                  (double) Player.bytes_sent / difftime)
                                << "/s)"
                                << cli_client::endl;
                }

                m_client << "         "
                        << left << setfill(' ') << setw(15)
                        << "rcvd" << Player.pkts_rcvd << " packets "
                        << "(" << Player.pkts_rcvd / difftime << "/s)"
                        << " / " << byte_counter (Player.bytes_rcvd)
                        << " (" << byte_counter (
                          (double) Player.bytes_rcvd / difftime) << "/s)"
                        << cli_client::endl;
                m_client << "         " << left << setfill(' ') << setw(15)
                        << "expires in" << expires
                        <<cli_client::endl;
                m_client << "         " << left << setfill(' ') << setw(15)
                        << "inactive" << now - Player.last_relayed_to_inactive
                        <<cli_client::endl;
        }
        difftime = now - fgms->m_uptime;
        m_client <<cli_client::endl;
        m_client << EntriesFound << " entries found" <<cli_client::endl;
        if (! brief)
        {
                m_client << "Totals:" <<cli_client::endl;
                m_client << "          sent    : "
                        << fgms->m_player_list.pkts_sent << " packets"
                        << " (" << fgms->m_player_list.pkts_sent / difftime
                        << "/s) / "
                        << byte_counter (fgms->m_player_list.bytes_sent)
                        << " (" << byte_counter (
                          (double) fgms->m_player_list.bytes_sent / difftime)
                        << "/s)"
                        << cli_client::endl;
                m_client << "          received: "
                        << fgms->m_player_list.pkts_rcvd << " packets"
                        << " (" << fgms->m_player_list.pkts_rcvd / difftime
                        << "/s) / "
                        << byte_counter (fgms->m_player_list.bytes_rcvd) 
                        << " (" << byte_counter (
                          (double) fgms->m_player_list.bytes_rcvd / difftime)
                        << "/s)"
                        << cli_client::endl;
        }
        return RESULT::OK;
} // fgcli::cmd_show_user

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

