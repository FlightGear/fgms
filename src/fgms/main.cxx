/**
 * @file main.cxx
 * @author Oliver Schroeder
 * @brief Main Program
 *
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
// Copyright (C) 2006  Oliver Schroeder
//

#ifdef HAVE_CONFIG_H
        #include "config.h"
#endif
#include <cstdlib>
#ifndef _MSC_VER
        #include <sys/wait.h>
#endif
#include <signal.h>
#include "fgms.hxx"

/** @brief The running fgms server process */
fgmp::fgms fgms;

//////////////////////////////////////////////////////////////////////

/**
 * @brief If we receive a SIGHUP, reinit application
 * @param SigType int with signal type
 */
void
SigHUPHandler
(
        int SigType
)
{
        using fglog = fgmp::fglog;

        fgms.prepare_init ();
        if (fgms.m_config_file == "")
        {
                if ( ! fgms.read_configs ( true ) )
                {
                        LOG ( fglog::prio::HIGH,
                          "received HUP signal, but read config file failed!" );
                        exit ( 1 );
                }
        }
        else
        {
                if ( fgms.process_config ( fgms.m_config_file ) == false )
                {
                        LOG ( fglog::prio::HIGH,
                          "received HUP signal, but read config file failed!" );
                        exit ( 1 );
                }
        }
        if ( fgms.init () != 0 )
        {
                LOG ( fgmp::fglog::prio::HIGH,
                  "received HUP signal, but reinit failed!" );
                exit ( 1 );
        }
#ifndef _MSC_VER
        signal ( SigType, SigHUPHandler );
#endif
} // SigHUPHandler ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief MAIN routine
 * @param argc
 * @param argv*[]
 */
int
main
(
        int argc,
        char* argv[]
)
{
        fgms.parse_params ( argc, argv );
        fgms.read_configs ();
        if ( ! fgms.check_config() )
        {
                exit ( 1 );
        }
#ifndef _MSC_VER
        signal ( SIGHUP, SigHUPHandler );
#endif
        if ( ! fgms.init () )
        {
                return 1;
        }
        fgms.loop();
        fgms.done();
        return ( 0 );
} // main()
//////////////////////////////////////////////////////////////////////

