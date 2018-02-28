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

#ifndef fgls_cli_HEADER
#define fgls_cli_HEADER

#include <libcli.hxx>
#include "fgls.hxx"

using namespace libcli;

class fgls_cli : public cli
{
public:
        fgls_cli ( FGLS* fgls, int fd );
private:
        void setup ();
        //////////////////////////////////////////////////
        // general commands
        //////////////////////////////////////////////////
        RESULT cmd_show_settings ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cmd_show_version ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cmd_show_uptime ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cmd_show_log ( const std::string& command,
                   const strvec& args, size_t first_arg );
        RESULT cmd_die ( const std::string& command,
                   const strvec& args, size_t first_arg );

        FGLS*   fgls;
}; // class fgls_cli

#endif

