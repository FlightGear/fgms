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
 * @file        fgls.hxx
 *              flightgear list server
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        07/2017
 *
 * This is the list server for the flightgear multiplayer
 * network. All servers register at this server. All clients ask fgls
 * which server they should use.
 *
 */

#ifndef _fgls_header
#define _fgls_header

#include <stdint.h>
#include <string>
#include <vector>
#include <fstream>
#include <fglib/daemon.hxx>
#include <fglib/fg_thread.hxx>
#include <fglib/fg_list.hxx>
#include <fglib/fg_version.hxx>
#include <fglib/fg_proto.hxx>

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

/** An fgms server as seen from fgls
 */
class server
{
public:
        uint64_t        id;             ///< internal ID
        sender_type     type;           ///< type of server
        netaddr         addr;           ///< sockaddr of server
        std::string     name;           ///< eg. mpserver01
        std::string     location;       ///< "city/province/country"
        std::string     admin_email;    ///< email address of admin
        fgmp::version   version;        ///< version of server
        time_t          last_seen;
        time_t          registered_at;
        server ();
        friend std::ostream& operator << ( std::ostream& o, const server& s );
}; // class server

using ServerList = fgmp::lock_list_t<server>;
using ServerIt   = fgmp::lock_list_t<server>::iterator;

//////////////////////////////////////////////////////////////////////

/** The List server
 */
class fgls
{
public:
        /** @brief internal constants */
        enum
        {
                MAX_TELNETS = 5
        };
        static const version m_version;
        fgls ();
        bool    init ();
        void    loop ();
        int     parse_params ( int argc, char* argv[] );
        bool    read_configs ( bool reinit = false );
        void    shutdown ();
        friend class fgls_cli;
protected:
        //////////////////////////////////////////////////
        // configurable via cli
        //////////////////////////////////////////////////
        bool            m_run_as_daemon = false;
        bool            m_tty_cli       = true;
        std::string     m_bind_addr     = "";
        std::string     m_admin_user    = "";
        std::string     m_admin_pass    = "";
        std::string     m_admin_enable  = "";
        uint16_t        m_data_port     = 5004;
        uint16_t        m_query_port    = m_data_port + 1;
        uint16_t        m_admin_port    = m_data_port + 2;
        bool            m_admin_cli     = true;
        std::string     m_logfile_name  = "";
        fglog::prio     m_debug_level   = fglog::prio::MEDIUM;
        int             m_check_interval = 5;
        std::string     m_hostname       = "fgls";

        /// Maximum number of concurrent telnets.
        /// List of known servers.
        ServerList      m_server_list;
        /// Current selected HUB.
        ServerIt        m_cur_hub;
        /// Next server presented to clients.
        ServerIt        m_cur_fgms;
        /// True in main thread.
        bool            m_is_parent;
        /// True if the data channel needs to be (re-) initialised.
        bool            m_reinit_data;  // data channel needs to be reopened
        netsocket*        m_data_channel;
        /// True if the query channel needs to be (re-) initialised.
        bool            m_reinit_query; // query channel needs to be reopened
        netsocket*        m_query_channel;
        /// True if the admin channel needs to be (re-) initialised.
        bool            m_reinit_admin;
        netsocket*        m_admin_channel;
        /// true if logfile needs to be reopened.
        bool            m_reinit_log;
        /// true if have read a config file
        bool            m_have_config;
        bool            m_want_exit;
        time_t          m_uptime;

        bool    init_data_channel ();
        bool    init_query_channel ();
        bool    init_admin_channel ();
        void    set_bind_addr ( const std::string& addr );
        void    open_logfile ();
        void    print_version ();
        void    print_help ();
        bool    process_config ( const std::string & config_name );
        void    handle_admin ( int fd );

        int     m_argc; // number of commandline arguments
        char**  m_argv; // pointer to commandline arguments (copy)
}; // class fgls

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

#endif
