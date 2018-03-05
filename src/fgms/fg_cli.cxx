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

namespace libcli
{

/// new configure modes
namespace CLI_MODE
{
        enum
        {
                CONFIG_FGMS = CLI_MODE::EXTENSION
        };
}

}

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

fgcli::fgcli
(
        fgmp::fgms* fgms,
        int fd
): cli(fd)
{
        m_fgms = fgms;
        setup ();
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
        using namespace libcli;
        command* c;

        //////////////////////////////////////////////////
        // general setup
        //////////////////////////////////////////////////
        set_hostname (m_fgms->m_hostname);
        std::stringstream banner;
        banner  << "\r\n"
                << "------------------------------------------------\r\n"
                << "FlightGear Multiplayer Server cli\r\n"
                << "This is "
                << m_fgms->m_hostname << " (" << m_fgms->m_FQDN << ")\r\n"
                << "------------------------------------------------\r\n";
        set_banner ( banner.str() );
        //////////////////////////////////////////////////
        // setup authentication (if required)
        //////////////////////////////////////////////////
        if (m_fgms->m_admin_user != "")
                allow_user ( m_fgms->m_admin_user, m_fgms->m_admin_pass );
        if (m_fgms->m_admin_enable != "")
                set_enable_password ( m_fgms->m_admin_enable );

        using namespace std::placeholders;
        #define _ptr(X) (std::bind (& X, this, _1, _2, _3))

        //////////////////////////////////////////////////
        //
        // show commands
        //
        //////////////////////////////////////////////////
        c = new command (
                "show",
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::EXEC,
                "show system information"
        );
        register_command (c);

        //////////////////////////////////////////////////
        // show sub commands
        //////////////////////////////////////////////////

        // // 'show daemon'
        register_command ( new command (
                "daemon",
                _ptr ( fgcli::show_daemon ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show if running as a daemon"
        ), c);
        // 'show bind_addr'
        register_command ( new command (
                "bind_addr",
                _ptr ( fgcli::show_bind_addr ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show bind address"
        ), c );
        // 'show admin_user'
        register_command ( new command (
                "admin_user",
                _ptr ( fgcli::show_admin_user ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show admin user"
        ), c );
        // 'show admin_pass'
        register_command ( new command (
                "admin_pass",
                _ptr ( fgcli::show_admin_pass ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show admin password"
        ), c );
        // 'show enable_pass'
        register_command ( new command (
                "enable_pass",
                _ptr ( fgcli::show_admin_enable ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show enable password"
        ), c );
        // 'show data_port'
        register_command ( new command (
                "data_port",
                _ptr ( fgcli::show_data_port ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show data port"
        ), c );
        // 'show query_port'
        register_command ( new command (
                "query_port",
                _ptr ( fgcli::show_query_port ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show query port"
        ), c );
        // 'show admin_port'
        register_command ( new command (
                "admin_port",
                _ptr ( fgcli::show_admin_port ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show admin port"
        ), c );
        // 'show admin_cli'
        register_command ( new command (
                "admin_cli",
                _ptr ( fgcli::show_admin_cli ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show admin cli"
        ), c );
        // 'show logfile'
        register_command ( new command (
                "logfile",
                _ptr ( fgcli::show_logfile_name ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show logfile name"
        ), c );
        // 'show debug'
        register_command ( new command (
                "debug",
                _ptr ( fgcli::show_debug_level ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show debug level"
        ), c );
        // 'show hostname'
        register_command ( new command (
                "hostname",
                _ptr ( fgcli::show_debug_level ),
                PRIVLEVEL::UNPRIVILEGED,
                CLI_MODE::ANY,
                "Show server name"
        ), c );
        // 'show settings'
        register_command ( new command (
                "settings",
                _ptr ( fgcli::show_settings ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show general settings"
        ), c);
        // 'show version'
        register_command ( new command (
                "version",
                _ptr ( fgcli::show_version ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show running version information"
        ), c);
        // 'show uptime'
        register_command ( new command (
                "uptime",
                _ptr ( fgcli::show_uptime ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show uptime information"
        ), c);
        // 'show player_expires'
        register_command ( new command (
                "player_expires",
                _ptr ( fgcli::show_player_expires ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show grace time of pilots before they expire"
        ), c);
        // 'show max_radar_range'
        register_command ( new command (
                "max_radar_range",
                _ptr ( fgcli::show_max_radar_range ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show maximum radar range"
        ), c);
        // 'show fqdn'
        register_command ( new command (
                "fqdn",
                _ptr ( fgcli::show_fqdn ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show the fully qualified domain name"
        ), c);
        // 'show out_of_reach'
        register_command ( new command (
                "fqdn",
                _ptr ( fgcli::show_out_of_reach ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show out of reach"
        ), c);
        // 'show log'
        register_command ( new command (
                "log",
                _ptr ( fgcli::show_log ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show log buffer"
        ), c);
        // 'show stats'
        register_command ( new command (
                "stats",
                _ptr ( fgcli::show_stats ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show statistical information"
        ), c);
        // 'show whitelist'
        register_command (new command (
                "whitelist",
                _ptr ( fgcli::show_whitelist ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show entries in the whitelist"
        ), c);
        // 'show blacklist'
        register_command (new command (
                "blacklist",
                _ptr ( fgcli::show_blacklist ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show entries in the blacklist"
        ), c);
        // 'show crossfeeds'
        register_command (new command (
                "crossfeeds",
                _ptr ( fgcli::show_crossfeed ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show entries in the crossfeeds"
        ), c);
        // 'show relays'
        register_command (new command (
                "relays",
                _ptr ( fgcli::show_relay ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show list of relays"
        ), c);
        // 'show tracker'
        register_command (new command (
                "tracker",
                _ptr ( fgcli::show_tracker ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show status of tracker"
        ), c);
        // 'show users'
        register_command (new command (
                "users",
                _ptr ( fgcli::show_user ),
                libcli::PRIVLEVEL::UNPRIVILEGED,
                libcli::CLI_MODE::ANY,
                "Show list of users"
        ), c);

        //////////////////////////////////////////////////
        //
        // Configure commands
        //
        //////////////////////////////////////////////////

        // 'configure'
        c = new command (
                "configure",
                _ptr ( cli::internal_configure ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::EXEC,
                "Enter configuration mode"
        );
        register_command (c);

        //////////////////////////////////////////////////
        // configure subcommands
        //////////////////////////////////////////////////

        // 'configure fgms' in EXEC mode
        register_command ( new command (
                "fgms",
                _ptr ( fgcli::cfg_fgms ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::EXEC,
                "configure fgms internal properties"
        ), c);

        //////////////////////////////////////////////////
        // config commands in CONFIG mode
        //////////////////////////////////////////////////

        // 'fgms'
        c = new command (
                "fgms",
                _ptr ( fgcli::cfg_fgms ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "configure fgms internal properties"
        );
        register_command (c);
        // enable 'exit' command
        register_command ( new command (
                "exit",
                _ptr ( cli::internal_exit ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "return to EXEC mode"
        ));
        // enable 'exit' command
        register_command ( new command (
                "end",
                _ptr ( cli::internal_end ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::CONFIG,
                "return to previous mode"
        ));

        //////////////////////////////////////////////////
        // 'fgms' subcommands
        //////////////////////////////////////////////////

        // 'fgms daemon'
        register_command ( new command (
                "daemon",
                _ptr ( fgcli::cfg_daemon ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "en-/disable daemon mode (windows only)"
        ), c);
        // 'fgms bind_addr'
        register_command ( new command (
                "bind_address",
                _ptr ( fgcli::cfg_bind_addr ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set bind address (only listen to this address)"
        ), c);
        // 'fgms admin_user'
        register_command ( new command (
                "admin_user",
                _ptr ( fgcli::cfg_admin_user ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set admin user name"
        ), c);
        // 'fgms admin_pass'
        register_command ( new command (
                "admin_pass",
                _ptr ( fgcli::cfg_admin_pass ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set admin password"
        ), c);
        // 'fgms admin_enable'
        register_command ( new command (
                "admin_enable",
                _ptr ( fgcli::cfg_admin_enable ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set enable password"
        ), c);
        // 'fgms data_port'
        register_command ( new command (
                "data_port",
                _ptr ( fgcli::cfg_data_port ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set data port"
        ), c);
        // 'fgms query_port'
        register_command ( new command (
                "query_port",
                _ptr ( fgcli::cfg_query_port ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set query port"
        ), c);
        // 'fgms admin_port'
        register_command ( new command (
                "admin_port",
                _ptr ( fgcli::cfg_admin_port ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set port of admin cli"
        ), c);
        // 'fgms admin_cli'
        register_command ( new command (
                "admin_cli",
                _ptr ( fgcli::cfg_admin_cli ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "en-/disable admin cli"
        ), c);
        // 'fgms logfile'
        register_command ( new command (
                "logfile",
                _ptr ( fgcli::cfg_logfile_name ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set logfile name"
        ), c);
        // 'fgms debug_level'
        register_command ( new command (
                "debug_level",
                _ptr ( fgcli::cfg_debug_level ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set debug level"
        ), c);
        // 'fgms hostname'
        register_command ( new command (
                "hostname",
                _ptr ( fgcli::cfg_hostname ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set server name"
        ), c);
        // 'fgms player_expires'
        register_command ( new command (
                "player_expires",
                _ptr ( fgcli::cfg_player_expires ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Configure player expires"
        ), c);
        // 'fgms max_radar_range'
        register_command ( new command (
                "max_radar_range",
                _ptr ( fgcli::cfg_max_radar_range ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Configure max radar range"
        ), c);
        // 'fgms fqdn'
        register_command ( new command (
                "fqdn",
                _ptr ( fgcli::cfg_fqdn ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Set FQDN"
        ), c);
        // 'fgms out_of_reach'
        register_command ( new command (
                "out_of_reach",
                _ptr ( fgcli::cfg_out_of_reach ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG,
                "Configure out of reach"
        ), c);

        //////////////////////////////////////////////////
        // config commands in CONFIG_FGMS mode
        //////////////////////////////////////////////////

        register_command ( new command (
                "daemon",
                _ptr ( fgcli::cfg_daemon ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "en-/disable daemon mode (windows only)"
        ));
        register_command ( new command (
                "bind_address",
                _ptr ( fgcli::cfg_bind_addr ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set bind address (only listen to this address)"
        ));
        register_command ( new command (
                "admin_user",
                _ptr ( fgcli::cfg_admin_user ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set admin user name"
        ));
        register_command ( new command (
                "admin_pass",
                _ptr ( fgcli::cfg_admin_pass ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set admin password"
        ));
        register_command ( new command (
                "admin_enable",
                _ptr ( fgcli::cfg_admin_enable ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set enable password"
        ));
        register_command ( new command (
                "data_port",
                _ptr ( fgcli::cfg_data_port ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set data port"
        ));
        register_command ( new command (
                "query_port",
                _ptr ( fgcli::cfg_query_port ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set query port"
        ));
        register_command ( new command (
                "admin_port",
                _ptr ( fgcli::cfg_admin_port ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set port of admin cli"
        ));
        register_command ( new command (
                "admin_cli",
                _ptr ( fgcli::cfg_admin_cli ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "en-/disable admin cli"
        ));
        register_command ( new command (
                "logfile",
                _ptr ( fgcli::cfg_logfile_name ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set logfile name"
        ));
        register_command ( new command (
                "debug_level",
                _ptr ( fgcli::cfg_debug_level ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set debug level"
        ));
        register_command ( new command (
                "hostname",
                _ptr ( fgcli::cfg_hostname ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set server name"
        ));
        register_command ( new command (
                "player_expires",
                _ptr ( fgcli::cfg_player_expires ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Configure player expires"
        ));
        register_command ( new command (
                "max_radar_range",
                _ptr ( fgcli::cfg_max_radar_range ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Configure max radar range"
        ));
        register_command ( new command (
                "fqdn",
                _ptr ( fgcli::cfg_fqdn ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Set FQDN"
        ));
        register_command ( new command (
                "out_of_reach",
                _ptr ( fgcli::cfg_out_of_reach ),
                PRIVLEVEL::PRIVILEGED,
                CLI_MODE::CONFIG_FGMS,
                "Configure out of reach"
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

        //////////////////////////////////////////////////
        //
        // general commands
        //
        //////////////////////////////////////////////////
        register_command (new command (
                "die",
                _ptr ( fgcli::cmd_die ),
                libcli::PRIVLEVEL::PRIVILEGED,
                libcli::CLI_MODE::EXEC,
                "force fgms to exit"
        ));

        #undef _ptr

} // fgcli::setup ()

//////////////////////////////////////////////////////////////////////


libcli::RESULT
fgcli::show_daemon
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "I am daemon" << ": " << m_fgms->m_run_as_daemon
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_daemon ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_bind_addr
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        std::string bind_addr;
        if ( m_fgms->m_bind_addr == "" )
                bind_addr = "*";
        else
                bind_addr = m_fgms->m_bind_addr;
        m_client << libcli::align_left ( 22 )
                << "bind address"<< ": " << bind_addr
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_bind_addr ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_admin_user
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "admin user" << ": " << m_fgms->m_admin_user
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_admin_user ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_admin_pass
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "admin pass" << ": " << m_fgms->m_admin_pass
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_admin_pass ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_admin_enable
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "admin enable" << ": " << m_fgms->m_admin_enable
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_admin_enable

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_data_port
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "data port" << ": " << m_fgms->m_data_port
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_data_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_query_port
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "query port" << ": " << m_fgms->m_query_port
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_query_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_admin_port
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "admin port" << ": " << m_fgms->m_admin_port
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_admin_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_admin_cli
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "admin cli" << ": " << m_fgms->m_admin_cli
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_admin_cli ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_logfile_name
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "logfile name" << ": " << m_fgms->m_logfile_name
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_logfile_name ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_debug_level
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "debug level" << ": " << (int) m_fgms->m_debug_level
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_debug_level

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::show_hostname
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        libcli::RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "server name" << ": " << m_fgms->m_hostname
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_hostname ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show general statistics
 */
libcli::RESULT
fgcli::show_stats
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;

        time_t   difftime;
        time_t   now;
        uint64_t accumulated_sent        = 0;
        uint64_t accumulated_rcvd        = 0;
        uint64_t accumulated_sent_pkts   = 0;
        uint64_t accumulated_rcvd_pkts   = 0;

        now = time(0);
        difftime = now - m_fgms->m_uptime;
        show_version (command, args, first_arg);
        m_client << libcli::cli_client::endl;
        m_client  << "I have " << m_fgms->m_black_list.size ()
                << " entries in my blacklist"
                << libcli::cli_client::endl;
        m_client << "I have " << m_fgms->m_cross_list.size () << " crossfeeds"
                << libcli::cli_client::endl;
        m_client << "I have " << m_fgms->m_relay_list.size () << " relays"
                << libcli::cli_client::endl;
        m_client << "I have " << m_fgms->m_player_list.size () << " users ("
                << m_fgms->m_local_clients << " local, "
                << m_fgms->m_remote_clients << " remote, "
                << m_fgms->m_num_max_clients << " max)"
                << libcli::cli_client::endl;

        m_client << "Sent counters:" << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "to crossfeeds:"
                << m_fgms->m_cross_list.pkts_sent << " packets"
                << " (" << m_fgms->m_cross_list.pkts_sent / difftime << "/s)"
                << " / " << byte_counter (m_fgms->m_cross_list.bytes_sent)
                << " (" << byte_counter (
                  (double) m_fgms->m_cross_list.bytes_sent / difftime) << "/s)"
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "to relays:"
                << m_fgms->m_relay_list.pkts_sent << " packets"
                << " (" << m_fgms->m_relay_list.pkts_sent / difftime << "/s)"
                << " / " << byte_counter (m_fgms->m_relay_list.bytes_sent)
                << " (" << byte_counter (
                  (double) m_fgms->m_relay_list.bytes_sent / difftime) << "/s)"
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "to users:"
                << m_fgms->m_player_list.pkts_sent << " packets"
                << " (" << m_fgms->m_player_list.pkts_sent / difftime << "/s)"
                << " / " << byte_counter (m_fgms->m_player_list.bytes_sent)
                << " (" << byte_counter (
                  (double) m_fgms->m_player_list.bytes_sent / difftime) << "/s)"
                << libcli::cli_client::endl;

        m_client << "Receive counters:" << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "total:" <<  m_fgms->m_packets_received
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "pings:" <<  m_fgms->m_ping_received
                << " (" << m_fgms->m_pong_received << " pongs)"
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "errors:"
                << "invalid packets:" << m_fgms->m_packets_invalid
                << " rejected:" << m_fgms->m_black_rejected
                << " unknown relay:" << m_fgms->m_unknown_relay
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "valid data:"
                << "pos data:" << m_fgms->m_pos_data
                << " other:" << m_fgms->m_unknown_data
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "tracker:"
                << "connects:" << m_fgms->m_tracker_connect
                << " disconnects:" << m_fgms->m_tracker_disconnect
                << " positions:" << m_fgms->m_tracker_position
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "admin connections:" << m_fgms->m_admin_received
                << libcli::cli_client::endl;
        float telnet_per_second;
        if (m_fgms->m_queries_received)
                telnet_per_second = (float) m_fgms->m_queries_received /
                (time(0) - m_fgms->m_uptime);
        else
                telnet_per_second = 0;
        m_client << libcli::align_left ( 22 )
                << "telnet connections: "
                << m_fgms->m_queries_received
                << " (" << std::setprecision(2) << telnet_per_second << " t/s)"
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "blacklist:"
                << m_fgms->m_black_list.pkts_rcvd << " packets"
                << " (" << m_fgms->m_black_list.pkts_rcvd / difftime << "/s)"
                << " / " << byte_counter (m_fgms->m_black_list.bytes_rcvd)
                << " (" << byte_counter (
                  (double) m_fgms->m_black_list.bytes_rcvd / difftime) << "/s)"
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "relays:"
                << m_fgms->m_relay_list.pkts_rcvd << " packets"
                << " (" << m_fgms->m_relay_list.pkts_rcvd / difftime << "/s)"
                << " / " << byte_counter (m_fgms->m_relay_list.bytes_rcvd)
                << " (" << byte_counter (
                  (double) m_fgms->m_relay_list.bytes_rcvd / difftime) << "/s)"
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "users:"
                << m_fgms->m_player_list.pkts_rcvd << " packets"
                << " (" << m_fgms->m_player_list.pkts_rcvd / difftime << "/s)"
                << " / " << byte_counter (m_fgms->m_player_list.bytes_rcvd)
                << " (" << byte_counter (
                  (double) m_fgms->m_player_list.bytes_rcvd / difftime) << "/s)"
                << libcli::cli_client::endl;
        accumulated_sent        += m_fgms->m_cross_list.bytes_sent;
        accumulated_sent_pkts   += m_fgms->m_cross_list.pkts_sent;
        accumulated_sent        += m_fgms->m_relay_list.bytes_sent;
        accumulated_sent_pkts   += m_fgms->m_relay_list.pkts_sent;
        accumulated_sent        += m_fgms->m_player_list.bytes_sent;
        accumulated_sent_pkts   += m_fgms->m_player_list.pkts_sent;
        accumulated_rcvd        += m_fgms->m_black_list.bytes_rcvd;
        accumulated_rcvd_pkts   += m_fgms->m_black_list.pkts_rcvd;
        accumulated_rcvd        += m_fgms->m_relay_list.bytes_rcvd;
        accumulated_rcvd_pkts   += m_fgms->m_relay_list.pkts_rcvd;
        accumulated_rcvd        += m_fgms->m_player_list.bytes_rcvd;
        accumulated_rcvd_pkts   += m_fgms->m_player_list.pkts_rcvd;
        m_client << "Totals:" << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "sent:"
                << accumulated_sent_pkts << " packets"
                << " (" << accumulated_sent_pkts / difftime << "/s)"
                << " / " << byte_counter (accumulated_sent)
                << " (" << byte_counter (
                  (double) accumulated_sent / difftime) << "/s)"
                << libcli::cli_client::endl;
        m_client << libcli::align_left ( 22 )
                << "received:"
                << accumulated_rcvd_pkts << " packets"
                << " (" << accumulated_rcvd_pkts / difftime << "/s)"
                << " / " << byte_counter (accumulated_rcvd)
                << " (" << byte_counter (
                  (double) accumulated_rcvd / difftime) << "/s)"
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_stats ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show general settings
 */
libcli::RESULT
fgcli::show_settings
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        strvec noargs;

        m_client << libcli::cli_client::endl;
        show_version        ( "", noargs, 0 );
        m_client << libcli::cli_client::endl;
        m_client << "current settings:" << libcli::cli_client::endl;
        show_bind_addr      ( "", noargs, 0 );
        show_admin_user     ( "", noargs, 0 );
        show_admin_pass     ( "", noargs, 0 );
        show_admin_enable   ( "", noargs, 0 );
        show_data_port      ( "", noargs, 0 );
        show_query_port     ( "", noargs, 0 );
        show_admin_port     ( "", noargs, 0 );
        show_admin_cli      ( "", noargs, 0 );
        show_logfile_name   ( "", noargs, 0 );
        show_debug_level    ( "", noargs, 0 );
        show_player_expires ( "", noargs, 0 );
        show_max_radar_range( "", noargs, 0 );
        show_out_of_reach   ( "", noargs, 0 );
        show_fqdn           ( "", noargs, 0 );
        show_hostname       ( "", noargs, 0 );

        return RESULT::OK;
} // fgcli::show_settings ()

//////////////////////////////////////////////////////////////////////
/**
 *  @brief Show the uptime of the the server
 *         in a human readable form.
 */
libcli::RESULT
fgcli::show_uptime
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << "UP since " << timestamp_to_datestr(m_fgms->m_uptime)
                << "(" << timestamp_to_days(m_fgms->m_uptime) << ")"
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_uptime ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show expire time of pilots
 */
libcli::RESULT
fgcli::show_player_expires
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "player expires" << ": "
                << m_fgms->m_player_expires
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_player_expires ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show maximum radar range
 */
libcli::RESULT
fgcli::show_max_radar_range
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "radar range" << ": "
                << m_fgms->m_max_radar_range
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_max_radar_range ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show out_of_reach
 */
libcli::RESULT
fgcli::show_out_of_reach
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "out of reach" << ": "
                << m_fgms->m_out_of_reach
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_out_of_reach ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show fully qualified domain name (FQDN)
 */
libcli::RESULT
fgcli::show_fqdn
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r = no_more_args ( args, first_arg );
        if ( r != RESULT::OK )
                return r;
        m_client << libcli::align_left ( 22 )
                << "fqdn" << ": "
                << m_fgms->m_FQDN
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_fqdn ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show log buffer of the the server
 */
libcli::RESULT
fgcli::show_log
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
                        m_client << *it << libcli::cli_client::endl;
                }
        }
        catch ( libcli::pager_wants_quit ) { /* do nothing */ }
        buf->unlock ();
        m_client.disable_filters ();
        return RESULT::OK;
} // fgcli::show_log ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show the version number of the the server
 */
libcli::RESULT
fgcli::show_version
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
        if (m_fgms->m_me_is_hub)
                s = "HUB";
        else
                s = "LEAVE";
        m_client << "This is " << m_fgms->m_hostname
                << " (" << m_fgms->m_FQDN << ")"
                << libcli::cli_client::endl;
        m_client << "FlightGear Multiplayer " << s << " Server version "
               << m_fgms->m_version.str() << libcli::cli_client::endl; 
        m_client << "using protocol version v"
                << m_fgms->m_proto_major_version << "."
                << m_fgms->m_proto_minor_version << libcli::cli_client::endl;
        if (m_fgms->m_is_tracked)
                m_client << "This server is tracked: "
                        << m_fgms->m_tracker->get_server () << libcli::cli_client::endl;
        else
                m_client << "This server is NOT tracked" << libcli::cli_client::endl;
        show_uptime (command, args, first_arg);
        return RESULT::OK;
} // fgcli::show_version ()

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
libcli::RESULT
fgcli::show_whitelist
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
                                address.assign ( args[first_arg] );
                                if ( ! address.is_valid() )
                                        return RESULT::INVALID_ARG;
                        }
                }
        }
        size_t EntriesFound { 0 };
        size_t Count = m_fgms->m_white_list.size ();
        fgmp::list_item Entry("");
        m_client << libcli::cli_client::endl;
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - m_fgms->m_uptime;
        m_client << m_fgms->m_white_list.name << ":" << libcli::cli_client::endl;
        m_client << libcli::cli_client::endl;
        for (size_t i = 0; i < Count; i++ )
        {
                Entry = m_fgms->m_white_list[i];
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
                        << libcli::cli_client::endl;
                if ( ! brief )
                {
                        m_client << "  entered      : "
                                << timestamp_to_datestr (Entry.join_time)
                                << libcli::cli_client::endl;
                        m_client << "  last seen    : "
                                << timestamp_to_days (Entry.last_seen)
                                << libcli::cli_client::endl;
                        m_client << "  rcvd packets : " << Entry.pkts_rcvd
                                << libcli::cli_client::endl;
                        m_client << "  rcvd bytes   : "
                                << byte_counter (Entry.bytes_rcvd)
                                << libcli::cli_client::endl;
                }
        }
        m_client << EntriesFound << " entries found"
                << libcli::cli_client::endl;
        if ( (EntriesFound) && ( ! brief ) )
        {
                m_client << libcli::cli_client::endl;
                m_client << "Total rcvd: "
                        << m_fgms->m_white_list.pkts_rcvd << " packets"
                        << " (" << m_fgms->m_white_list.pkts_rcvd / difftime
                        << "/s)"
                        << " / " << byte_counter (m_fgms->m_white_list.bytes_rcvd)
                        << " (" << byte_counter (
                          (double) (m_fgms->m_white_list.bytes_rcvd/difftime))
                        << "/s)"
                        << libcli::cli_client::endl;
        }
        return RESULT::OK;
} // fgcli::show_whitelist ()

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
libcli::RESULT
fgcli::show_blacklist
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
                                address.assign ( args[first_arg] );
                                if (! address.is_valid() )
                                        return RESULT::INVALID_ARG;
                        }
                }
        }
        int Count = m_fgms->m_black_list.size ();
        fgmp::list_item Entry("");
        m_client << libcli::cli_client::endl;
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - m_fgms->m_uptime;
        m_client << m_fgms->m_black_list.name << ":"
                << libcli::cli_client::endl;
        m_client << libcli::cli_client::endl;
        for (int i = 0; i < Count; i++)
        {
                Entry = m_fgms->m_black_list[i];
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
                        << libcli::cli_client::endl;
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
                        << libcli::cli_client::endl;
                m_client << "  last seen    : "
                        << timestamp_to_days (Entry.last_seen)
                        << libcli::cli_client::endl;
                m_client << "  rcvd packets : "
                        << Entry.pkts_rcvd
                        << libcli::cli_client::endl;
                m_client << "  rcvd bytes   : "
                        << byte_counter (Entry.bytes_rcvd)
                        << libcli::cli_client::endl;
                m_client << "  expire in    : "
                        << expire
                        << libcli::cli_client::endl;
        }
        if (EntriesFound)
        {
                m_client << libcli::cli_client::endl;
        }
        m_client << EntriesFound << " entries found"
                << libcli::cli_client::endl;
        if (EntriesFound)
        {
                m_client << "Total rcvd: "
                        << m_fgms->m_black_list.pkts_rcvd << " packets"
                        << " (" << m_fgms->m_black_list.pkts_rcvd / difftime
                        << "/s) / "
                        << byte_counter (m_fgms->m_black_list.bytes_rcvd)
                        << " (" << byte_counter (
                          (double) (m_fgms->m_black_list.bytes_rcvd/difftime))
                        << "/s)"
                        << libcli::cli_client::endl;
        }
        return RESULT::OK;
} // fgcli::show_blacklist ()

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
libcli::RESULT
fgcli::show_crossfeed
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
                                address.assign ( args[first_arg] );
                                if (! address.is_valid())
                                        return RESULT::INVALID_ARG;
                        }
                }
        }
        size_t EntriesFound = 0;
        int Count = m_fgms->m_cross_list.size ();
        fgmp::list_item Entry("");
        m_client << m_fgms->m_cross_list.name << ":"
                << libcli::cli_client::endl;
        m_client << libcli::cli_client::endl;
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - m_fgms->m_uptime;
        for (int i = 0; i < Count; i++)
        {
                Entry = m_fgms->m_cross_list[i];
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
                        << libcli::cli_client::endl;
                if (brief == true)
                {
                        continue;
                }
                m_client << "  entered      : "
                        << timestamp_to_datestr (Entry.join_time)
                        << libcli::cli_client::endl;
                m_client << "  last sent    : "
                        << timestamp_to_days (Entry.last_sent)
                        << libcli::cli_client::endl;
                m_client << "  sent packets : " << Entry.pkts_sent
                        << "(" << (double) (Entry.pkts_sent / difftime)
                        << " packets/s)"
                        << libcli::cli_client::endl;
                m_client << "  sent bytes   : "
                        << byte_counter (Entry.bytes_sent)
                        << "(" << byte_counter (
                          (double) Entry.bytes_sent / difftime) << "/s)"
                        << libcli::cli_client::endl;
        }
        if (EntriesFound)
        {
                m_client << libcli::cli_client::endl;
        }
        m_client << EntriesFound << " entries found"
                << libcli::cli_client::endl;
        if (EntriesFound)
        {
                m_client << "Total sent: "
                        << m_fgms->m_cross_list.pkts_sent << " packets"
                        << "(" << m_fgms->m_cross_list.pkts_sent / difftime
                        << "/s) / "
                        << byte_counter (m_fgms->m_cross_list.bytes_sent)
                        << "(" << byte_counter (
                          (double) (m_fgms->m_cross_list.bytes_sent/difftime))
                        << "/s)"
                        << libcli::cli_client::endl;
        }
        return RESULT::OK;
} // fgcli::show_crossfeed ()

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
libcli::RESULT
fgcli::show_relay
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
                                address.assign ( args[first_arg] );
                                if (! address.is_valid())
                                {
                                        name = args[first_arg];
                                }
                        }
                }
        }
        size_t EntriesFound = 0;
        int Count = m_fgms->m_relay_list.size ();
        fgmp::list_item Entry("");
        m_client << m_fgms->m_relay_list.name << ":"
                << libcli::cli_client::endl;
        m_client << libcli::cli_client::endl;
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - m_fgms->m_uptime;
        for (int i = 0; i < Count; i++)
        {
                Entry = m_fgms->m_relay_list[i];
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
                        << libcli::cli_client::endl;
                if (brief == true)
                {
                        continue;
                }
                m_client << "  entered   : "
                        << timestamp_to_datestr (Entry.join_time)
                        << libcli::cli_client::endl;
                m_client << "  last seen : "
                        << timestamp_to_datestr (Entry.last_seen)
                        << libcli::cli_client::endl;
                m_client << "  sent      : "
                        << Entry.pkts_sent << " packets"
                        << " (" << Entry.pkts_sent / difftime << "/s)"
                        << " / " << byte_counter (Entry.bytes_sent)
                        << " (" << byte_counter (
                          (double) Entry.bytes_sent / difftime) << "/s)"
                        << libcli::cli_client::endl;
                m_client << "  rcvd      : "
                        << Entry.pkts_rcvd << " packets"
                        << " (" << Entry.pkts_rcvd / difftime << "/s)"
                        << " / " << byte_counter (Entry.bytes_rcvd)
                        << " (" << byte_counter (
                          (double) Entry.bytes_rcvd / difftime) << "/s)"
                        << libcli::cli_client::endl;
        }
        m_client << libcli::cli_client::endl;
        m_client << EntriesFound << " entries found"
                << libcli::cli_client::endl;
        if (brief)
        {
                return RESULT::OK;
        }
        m_client << "Totals:" << libcli::cli_client::endl;
        m_client << "  sent      : "
                << m_fgms->m_relay_list.pkts_sent << " packets"
                << " (" << m_fgms->m_relay_list.pkts_sent / difftime << "/s)"
                << " / " << byte_counter (m_fgms->m_relay_list.bytes_sent)
                << " (" << byte_counter (
                  (double) m_fgms->m_relay_list.bytes_sent / difftime) << "/s)"
                << libcli::cli_client::endl;
        m_client << "  received  : "
                << m_fgms->m_relay_list.pkts_rcvd << " packets"
                << " (" << m_fgms->m_relay_list.pkts_rcvd / difftime << "/s)"
                << " / " << byte_counter (m_fgms->m_relay_list.bytes_rcvd)
                << " (" << byte_counter (
                  (double) m_fgms->m_relay_list.bytes_rcvd / difftime) << "/s)"
                << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_relay ()

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Show status of tracker server
 *
 *  possible arguments:
 *  show tracker ?
 *  show tracker <cr>
 */
libcli::RESULT
fgcli::show_tracker
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        size_t num_args { args.size() - first_arg };
        if ( num_args > 0 )
                return no_more_args ( args, first_arg + 1 );
        if (! m_fgms->m_is_tracked)
        {
                m_client << "This server is NOT tracked" << libcli::cli_client::endl;
                m_client << libcli::cli_client::endl;
                return RESULT::OK;
        }
        time_t  difftime;
        time_t  now;
        now = time(0);
        difftime = now - m_fgms->m_uptime;
        m_client << "This server is tracked: "
                << m_fgms->m_tracker->get_server () << ":"
                << m_fgms->m_tracker->get_port()
                << libcli::cli_client::endl;
        if (m_fgms->m_tracker->is_connected ())
        {
                m_client << "state: connected since "
                        << timestamp_to_datestr(m_fgms->m_tracker->last_connected)
                        << " ("
                        << timestamp_to_days (m_fgms->m_tracker->last_connected)
                        << " ago)"
                        << libcli::cli_client::endl;
        }
        else
        {
                m_client << "state: NOT connected!" << libcli::cli_client::endl;
        }
        std::string A = "NEVER";
        if (m_fgms->m_tracker->last_seen != 0)
        {
                A = timestamp_to_days (m_fgms->m_tracker->last_seen);
                A += " ago";
        }
        std::string B = "NEVER";
        if (m_fgms->m_tracker->last_sent != 0)
        {
                B = timestamp_to_days (m_fgms->m_tracker->last_sent);
                B += " ago";
        }
        m_client << "last seen " << A << ", last sent " << B << libcli::cli_client::endl;
        m_client << "I had " << m_fgms->m_tracker->lost_connections
          << " lost connections" << libcli::cli_client::endl;
        m_client << libcli::cli_client::endl;
        m_client << "Counters:" << libcli::cli_client::endl;
        m_client << "  sent    : " << m_fgms->m_tracker->pkts_sent << " packets";
        m_client << " (" << m_fgms->m_tracker->pkts_sent / difftime << "/s)";
        m_client << " / " << byte_counter (m_fgms->m_tracker->bytes_sent);
        m_client << " (" << byte_counter (
          (double) m_fgms->m_tracker->bytes_sent / difftime) << "/s)";
        m_client << libcli::cli_client::endl;
        m_client << "  received: " << m_fgms->m_tracker->pkts_rcvd << " packets";
        m_client << " (" << m_fgms->m_tracker->pkts_rcvd / difftime << "/s)";
        m_client << " / " << byte_counter (m_fgms->m_tracker->bytes_rcvd);
        m_client << " (" << byte_counter (
          (double) m_fgms->m_tracker->bytes_rcvd / difftime) << "/s)";
        m_client << libcli::cli_client::endl;
        m_client << "  queue size: " << m_fgms->m_tracker->queue_size ()
          << " messages" << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::show_tracker ()

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
libcli::RESULT
fgcli::show_user
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
                                address.assign ( args[first_arg] );
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
        int Count = m_fgms->m_player_list.size ();
        pilot   Player;
        point3d PlayerPosGeod;
        std::string  Origin;
        std::string  Fullname;
        m_client << m_fgms->m_player_list.name << ":" << libcli::cli_client::endl;
        m_client << libcli::cli_client::endl;
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
                difftime = now - m_fgms->m_uptime;
                Player = m_fgms->m_player_list[i];
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
                          m_fgms->m_relay_map.find ( Player.address );
                        if ( Relay != m_fgms->m_relay_map.end() )
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
                        << libcli::cli_client::endl;
                EntriesFound++;
                if (brief == true)
                {
                        continue;
                }
                if (Player.has_errors == true)
                {
                        m_client << "         "
                                << libcli::align_left (15)
                                << "ERROR" << Player.error
                                << libcli::cli_client::endl;
                }
                m_client << "         "
                        << libcli::align_left (15)
                        << "protocoll" << Player.proto_major
                        << "." << Player.proto_minor
                        << libcli::cli_client::endl;
                m_client << "         "
                        << libcli::align_left (15)
                        << "radar range" << Player.radar_range
                        << libcli::cli_client::endl;
                int expires = Player.timeout - (now - Player.last_seen);
                m_client << "         "
                        << libcli::align_left (15)
                        << "entered" << timestamp_to_days (Player.join_time)
                        << " ago" << libcli::cli_client::endl;
                m_client << "         "
                        << libcli::align_left (15)
                        << "joined" << timestamp_to_datestr(Player.join_time)
                        << libcli::cli_client::endl;
                m_client << "         "
                        << libcli::align_left (15)
                        << "last seen" << timestamp_to_datestr(Player.last_seen)
                        << libcli::cli_client::endl;
                m_client << "         "
                        << libcli::align_left (15)
                        << "using model" << Player.model_name
                        << libcli::cli_client::endl;
                if (Player.is_local)
                {
                        m_client << "         "
                                << libcli::align_left (15)
                                << "real origin" << Player.origin
                                << libcli::cli_client::endl;
                        m_client << "         "
                                << libcli::align_left (15)
                                << "sent" << Player.pkts_sent << " packets "
                                << "(" << Player.pkts_sent / difftime << "/s)"
                                << " / " << byte_counter (Player.bytes_sent)
                                << " (" << byte_counter (
                                  (double) Player.bytes_sent / difftime)
                                << "/s)"
                                << libcli::cli_client::endl;
                }
                m_client << "         "
                        << libcli::align_left (15)
                        << "rcvd" << Player.pkts_rcvd << " packets "
                        << "(" << Player.pkts_rcvd / difftime << "/s)"
                        << " / " << byte_counter (Player.bytes_rcvd)
                        << " (" << byte_counter (
                          (double) Player.bytes_rcvd / difftime) << "/s)"
                        << libcli::cli_client::endl;
                m_client << "         "
                        << libcli::align_left (15)
                        << "expires in" << expires
                        << libcli::cli_client::endl;
                m_client << "         "
                        << libcli::align_left (15)
                        << "inactive" << now - Player.last_relayed_to_inactive
                        << libcli::cli_client::endl;
        }
        difftime = now - m_fgms->m_uptime;
        m_client << libcli::cli_client::endl;
        m_client << EntriesFound << " entries found" << libcli::cli_client::endl;
        if (! brief)
        {
                m_client << "Totals:" << libcli::cli_client::endl;
                m_client << "          sent    : "
                        << m_fgms->m_player_list.pkts_sent << " packets"
                        << " (" << m_fgms->m_player_list.pkts_sent / difftime
                        << "/s) / "
                        << byte_counter (m_fgms->m_player_list.bytes_sent)
                        << " (" << byte_counter (
                          (double) m_fgms->m_player_list.bytes_sent / difftime)
                        << "/s)"
                        << libcli::cli_client::endl;
                m_client << "          received: "
                        << m_fgms->m_player_list.pkts_rcvd << " packets"
                        << " (" << m_fgms->m_player_list.pkts_rcvd / difftime
                        << "/s) / "
                        << byte_counter (m_fgms->m_player_list.bytes_rcvd) 
                        << " (" << byte_counter (
                          (double) m_fgms->m_player_list.bytes_rcvd / difftime)
                        << "/s)"
                        << libcli::cli_client::endl;
        }
        return RESULT::OK;
} // fgcli::show_user ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_fgms
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r { no_more_args ( args, first_arg) };
        if ( RESULT::OK != r )
                return r;
        set_configmode ( libcli::CLI_MODE::CONFIG_FGMS, "fgms" );
        return RESULT::OK;
} // fgcli::cfg_fgms ()

//////////////////////////////////////////////////////////////////////


libcli::RESULT
fgcli::cfg_daemon
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        std::pair <libcli::RESULT, bool> r
          { get_bool ( args[first_arg] ) };
        if ( r.first == RESULT::OK )
        {
                // if set from false to true a restart is needed
                m_fgms->m_run_as_daemon = r.second;
        }
        return r.first;
} // fgcli::cfg_daemon ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_bind_addr
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "IP",
                  "only listen on IP" );
                return RESULT::ERROR_ANY;
        }
        fgmp::netaddr address;
        address.assign ( args[first_arg]  );
        if ( ! address.is_valid () )
        {
                if ( compare ( "any", args[first_arg] ) )
                {
                        m_fgms->set_bind_addr ( "" );
                        return RESULT::OK;
                }
                m_client << command << " : ";
                return RESULT::INVALID_ARG;
        }
        m_fgms->set_bind_addr ( args[first_arg] );
        return RESULT::OK;
} // fgcli::cfg_bind_addr ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_admin_user
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "USERNAME",
                  "set admin username to USERNAME" );
                return RESULT::ERROR_ANY;
        }
        m_fgms->m_admin_user = args[first_arg];
        return RESULT::OK;
} // fgcli::cfg_admin_user ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_admin_pass
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "PASSWORD",
                  "set admin password to PASSWORD" );
                return RESULT::ERROR_ANY;
        }
        m_fgms->m_admin_pass = args[first_arg];
        return RESULT::OK;
} // fgcli::cfg_admin_pass ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_admin_enable
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "PASSWORD",
                  "set enable password to PASSWORD" );
                return RESULT::ERROR_ANY;
        }
        m_fgms->m_admin_enable = args[first_arg];
        return RESULT::OK;
} // fgcli::cfg_admin_enable ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_data_port
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "PORT",
                  "set data port to PORT" );
                return RESULT::ERROR_ANY;
        }
        int e;
        int p { str_to_num<uint16_t> ( args[first_arg], e ) };
        if ( e )
                return RESULT::INVALID_ARG;
        if ( m_fgms->m_data_port == p )
                return RESULT::OK;
        m_fgms->m_data_port = p;
        m_fgms->m_reinit_data = true;
        m_fgms->init_data_channel ();
        return RESULT::OK;
}  // fgcli::cfg_data_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_query_port
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "PORT",
                  "set query port to PORT" );
                return RESULT::ERROR_ANY;
        }
        int e;
        int p { str_to_num<uint16_t> ( args[first_arg], e ) };
        if ( e )
                return RESULT::INVALID_ARG;
        if ( m_fgms->m_query_port == p )
                return RESULT::OK;
        m_fgms->m_query_port = p;
        m_fgms->m_reinit_query = true;
        m_fgms->init_query_channel ();
        return RESULT::OK;
} // fgcli::cfg_query_port ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_admin_port
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "PORT",
                  "set admin port to PORT" );
                return RESULT::ERROR_ANY;
        }
        int e;
        int p { str_to_num<uint16_t> ( args[first_arg], e ) };
        if ( e )
                return RESULT::INVALID_ARG;
        if ( m_fgms->m_admin_port == p )
                return RESULT::OK;
        m_fgms->m_admin_port = p;
        m_fgms->m_reinit_admin = true;
        m_fgms->init_admin_channel ();
        return RESULT::OK;
} // fgcli::cfg_admin_port

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_admin_cli
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        std::pair <libcli::RESULT, bool> r { get_bool ( args[first_arg] ) };
        if ( r.first == RESULT::OK )
        {
                m_fgms->m_admin_cli = r.second;
        }
        return RESULT::OK;
} // fgcli::cfg_admin_cli ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_logfile_name
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "NAME",
                  "set logfile to NAME" );
                return RESULT::ERROR_ANY;
        }
        m_fgms->m_reinit_log = true;
        m_fgms->m_logfile_name = args[first_arg];
        m_fgms->open_logfile ();
        return RESULT::OK;
} // fgcli::cfg_logfile_name ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_debug_level
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "LEVEL",
                  "set debug level to LEVEL seconds" );
                return RESULT::ERROR_ANY;
        }
        int e;
        int p { str_to_num<uint16_t> ( args[first_arg], e ) };
        if ( e )
                return RESULT::INVALID_ARG;
        m_fgms->m_debug_level = fgmp::make_prio ( p );
        logger.priority ( m_fgms->m_debug_level );
        return RESULT::OK;
} // fgcli::cfg_debug_level ()

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_hostname
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "NAME", "set NAME as hostname" );
                return RESULT::ERROR_ANY;
        }
        m_fgms->m_hostname = args[first_arg];
        set_hostname ( m_fgms->m_hostname );
        return RESULT::OK;
} // fgcli::cfg_hostname

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_fqdn
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "NAME", "set NAME as hostname" );
                return RESULT::ERROR_ANY;
        }
        m_fgms->m_FQDN = args[first_arg];
        set_hostname ( m_fgms->m_hostname );
        return RESULT::OK;
} // fgcli::cfg_fqdn

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_player_expires
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "SECONDS", "set player_expires to SECONDS" );
                return RESULT::ERROR_ANY;
        }
        int e;
        int p { str_to_num<uint16_t> ( args[first_arg], e ) };
        if ( e )
                return RESULT::INVALID_ARG;
        m_fgms->m_player_expires = p;
        return RESULT::OK;
} // fgcli::cfg_player_expires

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_max_radar_range
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "NM", "set max radar range to NM" );
                return RESULT::ERROR_ANY;
        }
        int e;
        int p { str_to_num<uint16_t> ( args[first_arg], e ) };
        if ( e )
                return RESULT::INVALID_ARG;
        m_fgms->m_max_radar_range = p;
        return RESULT::OK;
} // fgcli::cfg_max_radar_range

//////////////////////////////////////////////////////////////////////

libcli::RESULT
fgcli::cfg_out_of_reach
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT n { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != n )
                return n;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "NM", "set out of reach to NM" );
                return RESULT::ERROR_ANY;
        }
        int e;
        int p { str_to_num<uint16_t> ( args[first_arg], e ) };
        if ( e )
                return RESULT::INVALID_ARG;
        m_fgms->m_out_of_reach = p;
        return RESULT::OK;
} // fgcli::cfg_out_of_reach

//////////////////////////////////////////////////////////////////////

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
libcli::RESULT
fgcli::cmd_whitelist_delete
(
        const std::string& command,
        const strvec& args,
        size_t first_arg
)
{
        RESULT r { need_n_args ( 1, args, first_arg) };
        if ( RESULT::OK != r )
                return r;
        if ( wants_help ( args[first_arg] ) )
        {
                show_help ( "id", "delete entry with id" );
                show_help ( "IP", "delete entry with IP-address" );
                return RESULT::OK;
        }
        fgmp::netaddr address;
        size_t id;
        int id_invalid;
        id = str_to_num<size_t> ( args[first_arg], id_invalid );
        if ( id_invalid )
        {
                id = 0;
                address.assign ( args[first_arg] );
                if ( ! address.is_valid() )
                        return RESULT::INVALID_ARG;
        }
        if ( ( id == 0 ) && ( ! address.is_valid() ) )
        {
                return RESULT::MISSING_ARG;
        }
        fglistit Entry;
        if ( (id == 0) && (address.is_valid()) )
        {       // match IP
                Entry = m_fgms->m_white_list.find (address);
                if (Entry != m_fgms->m_white_list.end())
                        m_fgms->m_white_list.erase (Entry);
                else
                        m_client << "no entry found!" << libcli::cli_client::endl;
                return RESULT::OK;
        }
        Entry = m_fgms->m_white_list.find_by_id (id);
        if (Entry != m_fgms->m_white_list.end())
        {
                m_fgms->m_white_list.erase (Entry);
                m_client << "deleted!" << libcli::cli_client::endl;
        }
        else
                m_client << "no entry found!" << libcli::cli_client::endl;
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
libcli::RESULT
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
                                m_client << "% invalid TTL" << libcli::cli_client::endl;
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
                        address.assign ( args[i] );
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                        <<  libcli::cli_client::endl;
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
        fglistit CurrentEntry = m_fgms->m_white_list.find ( E.address );
        if ( CurrentEntry == m_fgms->m_white_list.end() )
        {
                Newid = m_fgms->m_white_list.add (E, TTL);
        }
        else
        {
                m_client << "% entry already exists (id "
                        << CurrentEntry->id << ")!"
                        << libcli::cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "added with id " << Newid << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_whitelist_add

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
libcli::RESULT
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
        if ( num_args > 1 )
                return no_more_args ( args, first_arg + 1 );
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
                address.assign ( args[first_arg] );
                if (! address.is_valid())
                        return RESULT::INVALID_ARG;
        }
        if ( (id == 0) && (! address.is_valid()) )
                return RESULT::MISSING_ARG;;
        if ( address.is_valid() )
        {       // match IP
                Entry = m_fgms->m_black_list.find (address);
                if (Entry != m_fgms->m_black_list.end())
                        m_fgms->m_black_list.erase (Entry);
                else
                        m_client << "no entry found!" << libcli::cli_client::endl;
                return RESULT::OK;
        }
        Entry = m_fgms->m_black_list.find_by_id (id);
        if (Entry != m_fgms->m_black_list.end())
                m_fgms->m_black_list.erase (Entry);
        else
                m_client << "no entry found!" << libcli::cli_client::endl;
        m_client << "deleted!" << libcli::cli_client::endl;
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
libcli::RESULT
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
                                m_client << "% invalid TTL" << libcli::cli_client::endl;
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
                        address.assign ( args[i] );
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                        <<  libcli::cli_client::endl;
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
        fglistit CurrentEntry = m_fgms->m_black_list.find ( E.address );
        if ( CurrentEntry == m_fgms->m_black_list.end() )
        {
                Newid = m_fgms->m_black_list.add (E, TTL);
        }
        else
        {
                m_client << "% entry already exists (id "
                        << CurrentEntry->id << ")!"
                        << libcli::cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "added with id " << Newid << libcli::cli_client::endl;
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
libcli::RESULT
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
                        address.assign ( args[first_arg] );
                        if ( ! address.is_valid() )
                                return RESULT::INVALID_ARG;
                }
        }
        if ( (id == 0) && (! address.is_valid()) )
                return RESULT::MISSING_ARG;

        fglistit Entry;
        if ( (id == 0) && ( address.is_valid() ) )
        {       // match IP
                Entry = m_fgms->m_cross_list.find (address);
                if (Entry != m_fgms->m_cross_list.end())
                {
                        m_fgms->m_cross_list.erase (Entry);
                }
                else
                {
                        m_client << "no entry found" << libcli::cli_client::endl;
                        return RESULT::OK;
                }
                return RESULT::OK;
        }
        Entry = m_fgms->m_cross_list.find_by_id (id);
        if (Entry != m_fgms->m_cross_list.end())
        {
                m_fgms->m_cross_list.erase (Entry);
        }
        else
        {
                m_client << "no entry found" << libcli::cli_client::endl;
                return RESULT::OK;
        }
        m_client << "deleted" << libcli::cli_client::endl;
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
libcli::RESULT
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
                        address.assign ( args[i] );
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                  << libcli::cli_client::endl;
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
                                  << libcli::cli_client::endl;
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
        fglistit CurrentEntry = m_fgms->m_cross_list.find ( E.address, true );
        if ( CurrentEntry == m_fgms->m_cross_list.end() )
        {
                Newid = m_fgms->m_cross_list.add (E, port);
        }
        else
        {
                m_client << "entry already exists (id "
                        << CurrentEntry->id << ")"
                        << libcli::cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "added with id " << Newid << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_crossfeed_add

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
libcli::RESULT
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
                        address.assign ( args[first_arg] );
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                  << libcli::cli_client::endl;
                                return RESULT::ERROR_ANY;
                        }
                }
        }
        if ( (id == 0) && (! address.is_valid()) )
                return RESULT::MISSING_ARG;
        fglistit Entry;
        if ( (id == 0) && (address.is_valid()) )
        {       // match IP
                Entry = m_fgms->m_relay_list.find (address);
                if (Entry != m_fgms->m_relay_list.end())
                {
                        m_fgms->m_relay_list.erase (Entry);
                }
                else
                {
                        m_client << "no entry found" << libcli::cli_client::endl;
                        return RESULT::ERROR_ANY;
                }
                return RESULT::OK;
        }
        Entry = m_fgms->m_relay_list.find_by_id (id);
        if (Entry != m_fgms->m_relay_list.end())
        {
                m_fgms->m_relay_list.erase (Entry);
        }
        else
        {
                m_client << "no entry found" << libcli::cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "deleted" << libcli::cli_client::endl;
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
libcli::RESULT
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
                        address.assign ( args[i] );
                        if (! address.is_valid())
                        {
                                m_client << "% invalid IP address"
                                  << libcli::cli_client::endl;
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
                                  << Port << libcli::cli_client::endl;
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
        fglistit CurrentEntry = m_fgms->m_relay_list.find ( E.address, true );
        if ( CurrentEntry == m_fgms->m_relay_list.end() )
        {
                Newid = m_fgms->m_relay_list.add (E, 0);
        }
        else
        {
                m_client << "entry already exists (id "
                        << CurrentEntry->id << ")"
                        << libcli::cli_client::endl;
                return RESULT::ERROR_ANY;
        }
        m_client << "added with id " << Newid << libcli::cli_client::endl;
        return RESULT::OK;
} // fgcli::cmd_relay_add

//////////////////////////////////////////////////////////////////////

/**
 *  @brief Shutdown the server
 */
libcli::RESULT
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
        m_fgms->m_want_exit = true;
        return RESULT::OK;
} // fgcli::cmd_die

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

