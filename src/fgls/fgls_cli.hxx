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
 * @file        fgls_cli.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        07/2017
 */

/**
 * @class fgls_cli
 * @brief cisco like command line interface
 */

#ifndef _fgls_cli_header
#define _fgls_cli_header

#include <utility>
#include <libcli/libcli.hxx>
#include "fgls.hxx"

namespace libcli
{

namespace CLI_MODE
{
        enum
        {
                CONFIG_FGLS = CLI_MODE::CONFIG + 1
        };
}

}

namespace fgmp
{

class fgls_cli : public libcli::cli
{
public:
        fgls_cli ( fgmp::fgls* fgls, int fd );
private:
        void setup ();
        using strvec = libcli::strvec;
        using RESULT = libcli::RESULT;

        //////////////////////////////////////////////////
        // show commands
        //////////////////////////////////////////////////
        RESULT show_daemon ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_tty_cli ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_bind_addr ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_admin_user ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_admin_pass ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_admin_enable ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_data_port ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_query_port ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_admin_port ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_admin_cli ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_logfile_name ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_debug_level ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_check_interval ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_hostname ( const std::string& command,
                   const strvec& args, size_t first_arg );

        RESULT show_settings ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_version ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_uptime ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT show_log ( const std::string& command,
                   const strvec& args, size_t first_arg );

        //////////////////////////////////////////////////
        // configure commands
        //////////////////////////////////////////////////
        RESULT cfg_fgls ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_daemon ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_tty_cli ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_bind_addr ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_admin_user ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_admin_pass ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_admin_enable ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_data_port ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_query_port ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_admin_port ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_admin_cli ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_logfile_name ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_debug_level ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_check_interval ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cfg_hostname ( const std::string& command,
                   const strvec& args, size_t first_arg );

        //////////////////////////////////////////////////
        // general commands
        //////////////////////////////////////////////////
        RESULT cmd_die ( const std::string& command,
                   const strvec& args, size_t first_arg );

        //////////////////////////////////////////////////
        // helper methods
        //////////////////////////////////////////////////
        std::pair <libcli::RESULT,bool> get_cli_bool ( const std::string& arg );
        //////////////////////////////////////////////////
        // variables
        //////////////////////////////////////////////////
        fgmp::fgls* m_fgls;
}; // class fgls_cli

} // namespace fgmp

#endif

