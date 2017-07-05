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

//////////////////////////////////////////////////////////////////////
//
// main program
//
//////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdlib>
#ifndef _MSC_VER
#include <sys/wait.h>
#endif
#include <signal.h>
#include <fglib/fg_util.hxx>
#include <fglib/fg_config.hxx>
#include <fglib/daemon.hxx>
#include <fglib/fg_version.hxx>
#include <fglib/fg_log.hxx>
#include "fgms.hxx"

using namespace std;

/** @brief The running  ::FGMS server process */
FGMS       fgms;

/** @brief Flag whether instance is a Daemon  */
extern  bool    RunAsDaemon;
extern  bool    AddCLI;
#ifdef _MSC_VER
#define M_IS_DIR _S_IFDIR
#else // !_MSC_VER
#define M_IS_DIR S_IFDIR
/** @brief An instance of ::cDaemon */
extern  cDaemon Myself;
#endif

/** @brief Must have a config file, with server name */
static bool     bHadConfig = false;

/** @def DEF_CONF_FILE
 *  @brief The default config file to load unless overriden on \ref command_line
 */
#ifndef DEF_CONF_FILE
#define DEF_CONF_FILE "fgms.conf"
#endif

/** @def SYSCONFDIR
 *  @brief The default config directory
 */
#ifndef SYSCONFDIR
#define SYSCONFDIR "/usr/etc"
#endif

#ifdef _MSC_VER
// kludge for getopt() for WIN32
// FIXME: put in libmsc
static char* optarg;
static int curr_arg = 0;
int getopt ( int argcount, char* argvars[], char* args )
{
	size_t len = strlen ( args );
	size_t i;
	int c = 0;
	if ( curr_arg == 0 )
	{
		curr_arg = 1;
	}
	if ( curr_arg < argcount )
	{
		char* arg = argvars[curr_arg];
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
						optarg = argvars[curr_arg];
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
#endif // _MSC_VER

//////////////////////////////////////////////////////////////////////
/**
 * @brief Read a config file and set internal variables accordingly
 * @param ConfigName Path of config file to load
 * @retval int  -- todo--
 */
bool
ProcessConfig ( const string& ConfigName )
{
	FG_CONFIG   Config;
	string      Val;
	int         E;
	if ( bHadConfig )	// we already have a config, so ignore
	{
		return ( true );
	}
	if ( Config.Read ( ConfigName ) )
	{
		LOG ( log::URGENT,
		  "Could not read config file '" << ConfigName
		  << "' => using defaults");
		return ( false );
	}
	LOG ( log::ERROR, "processing " << ConfigName );
	fgms.ConfigFile =  ConfigName;
	Val = Config.Get ( "server.name" );
	if ( Val != "" )
	{
		fgms.SetServerName ( Val );
		bHadConfig = true; // got a serve name - minimum
	}
	Val = Config.Get ( "server.address" );
	if ( Val != "" )
	{
		fgms.SetBindAddress ( Val );
	}
	Val = Config.Get ( "server.FQDN" );
	if ( Val != "" )
	{
		fgms.SetFQDN ( Val );
	}
	Val = Config.Get ( "server.port" );
	if ( Val != "" )
	{
		fgms.SetDataPort ( StrToNum<int> ( Val.c_str (), E ) );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for DataPort: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.telnet_port" );
	if ( Val != "" )
	{
		fgms.SetTelnetPort ( StrToNum<int> ( Val.c_str (), E ) );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for TelnetPort: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.admin_cli" );
	if ( Val != "" )
	{
		if ( ( Val == "on" ) || ( Val == "true" ) )
		{
			AddCLI = true;
		}
		else if ( ( Val == "off" ) || ( Val == "false" ) )
		{
			AddCLI = false;
		}
		else
		{
			LOG ( log::URGENT,
			  "unknown value for 'server.admin_cli'!"
			  << " in file " << ConfigName
			);
		}
	}
	Val = Config.Get ( "server.admin_port" );
	if ( Val != "" )
	{
		fgms.SetAdminPort ( StrToNum<int> ( Val.c_str (), E ) );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for AdminPort: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.admin_user" );
	if ( Val != "" )
	{
		fgms.SetAdminUser ( Val );
	}
	Val = Config.Get ( "server.admin_pass" );
	if ( Val != "" )
	{
		fgms.SetAdminPass ( Val );
	}
	Val = Config.Get ( "server.admin_enable" );
	if ( Val != "" )
	{
		fgms.SetAdminEnable ( Val );
	}
	Val = Config.Get ( "server.out_of_reach" );
	if ( Val != "" )
	{
		fgms.SetOutOfReach ( StrToNum<int> ( Val.c_str (), E ) );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for out_of_reach: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.max_radar_range" );
	if ( Val != "" )
	{
		fgms.SetMaxRadarRange ( StrToNum<int> ( Val.c_str (), E ) );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for max_radar_range: '" << Val
			  << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.playerexpires" );
	if ( Val != "" )
	{
		fgms.SetPlayerExpires ( StrToNum<int> ( Val.c_str (), E ) );
		if ( E )
		{
			LOG ( log::URGENT,
			  "invalid value for Expire: '" << Val << "'"
			);
			exit ( 1 );
		}
	}
	Val = Config.Get ( "server.logfile" );
	if ( Val != "" )
	{
		fgms.SetLogfile ( Val );
	}
	Val = Config.Get ( "server.daemon" );
	if ( Val != "" )
	{
		if ( ( Val == "on" ) || ( Val == "true" ) )
		{
			RunAsDaemon = true;
		}
		else if ( ( Val == "off" ) || ( Val == "false" ) )
		{
			RunAsDaemon = false;
		}
		else
		{
			LOG ( log::URGENT,
			  "unknown value for 'server.daemon'!"
			  << " in file " << ConfigName
			);
		}
	}
	Val = Config.Get ( "server.tracked" );
	if ( Val != "" )
	{
		string  Server;
		int     Port;
		bool    tracked = false;

		if ( Val == "true" )
		{
			tracked = true;
			Server = Config.Get ( "server.tracking_server" );
			Val = Config.Get ( "server.tracking_port" );
			Port = StrToNum<int> ( Val.c_str (), E );
			if ( E )
			{
				LOG ( log::URGENT,
				  "invalid value for tracking_port: '"
				  << Val << "'"
				);
				exit ( 1 );
			}
			if ( tracked && ( fgms.AddTracker ( Server, Port, tracked ) != FGMS::SUCCESS ) ) // set master m_IsTracked
			{
				LOG ( log::URGENT,
				  "Failed to get IPC msg queue ID! error "
				  << errno );
				exit ( 1 ); // do NOT continue if a requested 'tracker' FAILED
			}
		}
	}
	Val = Config.Get ( "server.is_hub" );
	if ( Val != "" )
	{
		if ( Val == "true" )
		{
			fgms.SetHub ( true );
		}
		else
		{
			fgms.SetHub ( false );
		}
	}
	//////////////////////////////////////////////////
	//      read the list of relays
	//////////////////////////////////////////////////
	bool    MoreToRead  = true;
	string  Section = "relay";
	string  Var;
	string  Server = "";
	int     Port   = 0;
	if ( ! Config.SetSection ( Section ) )
	{
		MoreToRead = false;
	}
	while ( MoreToRead )
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if ( Var == "relay.host" )
		{
			Server = Val;
		}
		if ( Var == "relay.port" )
		{
			Port = StrToNum<int> ( Val.c_str(), E );
			if ( E )
			{
				LOG ( log::URGENT,
				  "invalid value for RelayPort: '"
				  << Val << "'"
				);
				exit ( 1 );
			}
		}
		if ( ( Server != "" ) && ( Port != 0 ) )
		{
			fgms.AddRelay ( Server, Port );
			Server = "";
			Port   = 0;
		}
		if ( Config.SecNext () == 0 )
		{
			MoreToRead = false;
		}
	}
	//////////////////////////////////////////////////
	//      read the list of crossfeeds
	//////////////////////////////////////////////////
	MoreToRead  = true;
	Section = "crossfeed";
	Var    = "";
	Server = "";
	Port   = 0;
	if ( ! Config.SetSection ( Section ) )
	{
		MoreToRead = false;
	}
	while ( MoreToRead )
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if ( Var == "crossfeed.host" )
		{
			Server = Val;
		}
		if ( Var == "crossfeed.port" )
		{
			Port = StrToNum<int> ( Val.c_str(), E );
			if ( E )
			{
				LOG ( log::URGENT,
				  "invalid value for crossfeed.port: '"
				  << Val << "'"
				);
				exit ( 1 );
			}
		}
		if ( ( Server != "" ) && ( Port != 0 ) )
		{
			fgms.AddCrossfeed ( Server, Port );
			Server = "";
			Port   = 0;
		}
		if ( Config.SecNext () == 0 )
		{
			MoreToRead = false;
		}
	}

	//////////////////////////////////////////////////
	//      read the list of whitelisted IPs
	//      (a crossfeed might list the sender here
	//      to avoid blacklisting without defining the
	//      sender as a relay)
	//////////////////////////////////////////////////
	MoreToRead  = true;
	Section = "whitelist";
	Var    = "";
	Val    = "";
	if ( ! Config.SetSection ( Section ) )
	{
		MoreToRead = false;
	}
	while ( MoreToRead )
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if ( Var == "whitelist" )
		{
			fgms.AddWhitelist ( Val.c_str() );
		}
		if ( Config.SecNext () == 0 )
		{
			MoreToRead = false;
		}
	}

	//////////////////////////////////////////////////
	//      read the list of blacklisted IPs
	//////////////////////////////////////////////////
	MoreToRead  = true;
	Section = "blacklist";
	Var    = "";
	Val    = "";
	if ( ! Config.SetSection ( Section ) )
	{
		MoreToRead = false;
	}
	while ( MoreToRead )
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if ( Var == "blacklist" )
		{
			fgms.AddBlacklist ( Val.c_str(),
			  "static config entry", 0 );
		}
		if ( Config.SecNext () == 0 )
		{
			MoreToRead = false;
		}
	}

	//////////////////////////////////////////////////
	return ( true );
} // ProcessConfig ( const string& ConfigName )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print a help screen for command line parameters, see \ref command_line
 */
void
PrintVersion
()
{
	cout << endl;
	cout << "fgms version " << fgms.m_version
	     << ", compiled on " << __DATE__ << " at " << __TIME__ << endl;
	cout << endl;
} // PrintVersion()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Print a help screen for command line parameters, see \ref command_line
 */
void
PrintHelp
(
	char* prog
)
{
	PrintVersion ();
	cout << "syntax: " << prog << " options" << endl;
	cout << "\n"
	     "options are:\n"
	     "-h            print this help screen\n"
	     "-a PORT       listen to PORT for telnet\n"
	     "-c config     read 'config' as configuration file\n"
	     "-p PORT       listen to PORT\n"
	     "-t TTL        Time a client is active while not sending packets\n"
	     "-o OOR        nautical miles two players must be apart to be out of reach\n"
	     "-l LOGFILE    Log to LOGFILE\n"
	     "-v LEVEL      verbosity (loglevel) in range 0 (few) and 3 (much). 5 to disable. (def=" << logger.priority() << ")\n"
	     "-d            do _not_ run as a daemon (stay in foreground)\n"
	     "-D            do run as a daemon (default)\n"
	     "\n";
	exit ( 0 );
} // PrintHelp ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Parse commandline parameters
 * @param argcount
 * @param argvars
 * @retval int 1 on success
 */
int
ParseParams ( int argcount, char* argvars[] )
{
	int     m;
	int     E;
	while ( ( m=getopt ( argcount,argvars,"a:b:c:dDhl:o:p:t:v:" ) ) != -1 )
	{
		switch ( m )
		{
		case 'h':
			PrintHelp (argvars[0]);
			break; // never reached
		case 'a':
			fgms.SetTelnetPort ( StrToNum<int> ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for TelnetPort: '" << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'b':
			fgms.SetAdminPort ( StrToNum<int> ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for AdminPort: '" << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'c':
			ProcessConfig ( optarg );
			break;
		case 'p':
			fgms.SetDataPort ( StrToNum<int>  ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for DataPort: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'o':
			fgms.SetOutOfReach ( StrToNum<int>  ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for OutOfReach: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'v':
			logger.priority ( StrToNum<int> ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for Loglevel: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 't':
			fgms.SetPlayerExpires ( StrToNum<int>  ( optarg, E ) );
			if ( E )
			{
				cerr << "invalid value for expire: '"
				     << optarg << "'" << endl;
				exit ( 1 );
			}
			break;
		case 'l':
			fgms.SetLogfile ( optarg );
			break;
		case 'd':
			RunAsDaemon = false;
			break;
		case 'D':
			RunAsDaemon = true;
			break;
		default:
			cerr << endl << endl;
			PrintHelp (argvars[0]);
			exit ( 1 );
		} // switch ()
	} // while ()
	return ( 1 ); // success
} // ParseParams()

//////////////////////////////////////////////////////////////////////
/**
 * @brief  (re)Read config files
 * @param ReInit True to reinitialize
 */
int
ReadConfigs ( bool ReInit = false )
{
	string Path;
#ifndef _MSC_VER
	Path = SYSCONFDIR;
	Path += "/" DEF_CONF_FILE; // fgms.conf
	if ( ProcessConfig ( Path ) == true )
	{
		return 1;
	}
	Path = getenv ( "HOME" );
#else
	char* cp = getenv ( "HOME" );
	if ( cp )
	{
		Path = cp;
	}
	else
	{
		cp = getenv ( "USERPROFILE" ); // XP=C:\Documents and Settings\<name>, Win7=C:\Users\<user>
		if ( cp )
		{
			Path = cp;
		}
	}
#endif
	if ( Path != "" )
	{
		Path += "/" DEF_CONF_FILE;
		if ( ProcessConfig ( Path ) )
		{
			return 1;
		}
	}
	if ( ProcessConfig ( DEF_CONF_FILE ) )
	{
		return 1;
	}
	return 0;
} // ReadConfigs ()



//////////////////////////////////////////////////////////////////////
/**
 * @brief If we receive a SIGHUP, reinit application
 * @param SigType int with signal type
 */
void SigHUPHandler ( int SigType )
{
	fgms.PrepareInit();
	bHadConfig = false;
	if (fgms.ConfigFile == "")
	{
		if ( !ReadConfigs ( true ) )
		{
			LOG ( log::HIGH,
			  "received HUP signal, but read config file failed!" );
			exit ( 1 );
		}
	}
	else
	{
		if ( ProcessConfig ( fgms.ConfigFile ) == false )
		{
			LOG ( log::HIGH,
			  "received HUP signal, but read config file failed!" );
			exit ( 1 );
		}
	}
	if ( fgms.Init () != 0 )
	{
		LOG ( log::HIGH,
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
	int     I;

#ifndef _MSC_VER
	signal ( SIGHUP, SigHUPHandler );
#endif
	ParseParams ( argc, argv );
	ReadConfigs ();
	if ( !bHadConfig )
	{
		LOG ( log::HIGH,
		  "No configuration file '" << DEF_CONF_FILE << "' found!"
		);
		exit ( 1 );
	}
#ifndef _MSC_VER
	if ( RunAsDaemon )
	{
		Myself.Daemonize ();
	}
#endif
	I = fgms.Init ();
	if ( I != 0 )
	{
		fgms.CloseTracker();
		return ( I );
	}
	I = fgms.Loop();
	fgms.Done();
	return ( I );
} // main()
//////////////////////////////////////////////////////////////////////

