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
 * @file        fg_cli.hxx
 * @author      Oliver Schroeder <fgms@o-schroeder.de>
 * @date        2013
 */

//////////////////////////////////////////////////////////////////////
//
//      cisco like command line interface (cli)
//
//////////////////////////////////////////////////////////////////////

#ifndef fgcli_HEADER
#define fgcli_HEADER

#include <libcli/libcli.hxx>
#include "fgms.hxx"

using namespace libcli;

namespace fgmp
{

/**
 * @class fgcli 
 * @brief cisco like command line interface
 * 
 */
class fgcli : public cli
{
public:
        fgcli ( fgmp::fgms* fgms, int fd );
private:
        void setup ();
        //////////////////////////////////////////////////
        // general  commands
        //////////////////////////////////////////////////
        RESULT cmd_show_stats    ( const std::string& command,
                                   const strvec& args, size_t first_arg );
        RESULT cmd_show_settings ( const std::string& command,
                                   const strvec& args, size_t first_arg );
        RESULT cmd_show_version  ( const std::string& command,
                                   const strvec& args, size_t first_arg );
        RESULT cmd_show_uptime   ( const std::string& command,
                                   const strvec& args, size_t first_arg );
        RESULT cmd_show_user     ( const std::string& command,
                                   const strvec& args, size_t first_arg );
        RESULT cmd_die           ( const std::string& command,
                                   const strvec& args, size_t first_arg );
        //////////////////////////////////////////////////
        // show/modify whitelist
        //////////////////////////////////////////////////
        RESULT cmd_whitelist_show   ( const std::string& command,
                                      const strvec& args, size_t first_arg );
        RESULT cmd_whitelist_add    ( const std::string& command,
                                      const strvec& args, size_t first_arg );
        RESULT cmd_whitelist_delete ( const std::string& command,
                                      const strvec& args, size_t first_arg );
        //////////////////////////////////////////////////
        // show/modify blacklist
        //////////////////////////////////////////////////
        RESULT cmd_blacklist_show   ( const std::string& command,
                                      const strvec& args, size_t first_arg );
        RESULT cmd_blacklist_add    ( const std::string& command,
                                      const strvec& args, size_t first_arg );
        RESULT cmd_blacklist_delete ( const std::string& command,
                                      const strvec& args, size_t first_arg );
        //////////////////////////////////////////////////
        // show/modify list of crossfeeds
        //////////////////////////////////////////////////
        RESULT cmd_crossfeed_show   ( const std::string& command,
                                      const strvec& args, size_t first_arg );
        RESULT cmd_crossfeed_add    ( const std::string& command,
                                      const strvec& args, size_t first_arg );
        RESULT cmd_crossfeed_delete ( const std::string& command,
                                      const strvec& args, size_t first_arg );
        //////////////////////////////////////////////////
        // show/modify list of relays
        //////////////////////////////////////////////////
        RESULT cmd_relay_show   ( const std::string& command,
                                  const strvec& args, size_t first_arg );
        RESULT cmd_relay_add    ( const std::string& command,
                                  const strvec& args, size_t first_arg );
        RESULT cmd_relay_delete ( const std::string& command,
                                  const strvec& args, size_t first_arg );
        //////////////////////////////////////////////////
        // show status of tracker
        //////////////////////////////////////////////////
        RESULT cmd_tracker_show ( const std::string& command,
                                  const strvec& args, size_t first_arg );
        //////////////////////////////////////////////////
        // show/modify log
        //////////////////////////////////////////////////
        RESULT cmd_show_log ( const std::string& command,
                              const strvec& args, size_t first_arg );
        // TODO: change the size of the logbuffer
private:
        fgmp::fgms* fgms;
};

} // namespace fgmp

#endif

