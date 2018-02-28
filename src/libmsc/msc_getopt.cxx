/**
 * @file msc_getopt.cxx
 * @brief msc_getopt is for Windows platform only.
 *        A simple displacement for gnu getopt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, US
//
#ifdef HAVE_CONFIG_H
#include "config.h" // always config.h first, if there is one...
#endif

#ifdef _MSC_VER
#include "msc_getopt.hxx"

// kludge for getopt() for WIN32
static char* optarg;
static int curr_arg = 0;
int getopt ( int argc, char* argv[], char* args )
{
        size_t len = strlen ( args );
        size_t i;
        int c = 0;
        if ( curr_arg == 0 )
        {
                curr_arg = 1;
        }
        if ( curr_arg < argc )
        {
                char* arg = argv[curr_arg];
                if ( *arg == '-' )
                {
                        arg++;
                        c = *arg; // get first char
                        for ( i = 0; i < len; i++ )
                        {
                                if ( c == args[i] )
                                {
                                        // found
                                        if ( args[i+1] == ':' )
                                        {
                                                // fill in following
                                                curr_arg++;
                                                optarg = argv[curr_arg];
                                        }
                                        break;
                                }
                        }
                        curr_arg++;
                        return c;
                }
                else
                {
                        return '-';
                }
        }
        return -1;
}

#endif

