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
#include "fg_server.hxx"
#include "fg_config.hxx"
#include "daemon.hxx"
#include "typcnvt.hxx"

using namespace std;
/*DEF_TRACKER_CHILDS (Was 3 but now set to 1 due to a bug on unproper handling of out-of-sequence messages)*/
#ifndef DEF_TRACKER_CHILDS
  #define DEF_TRACKER_CHILDS 1 
#endif

FG_SERVER       Servant;
extern  bool    RunAsDaemon;
#ifdef _MSC_VER
	#define M_IS_DIR _S_IFDIR
#else // !_MSC_VER
	#define M_IS_DIR S_IFDIR
	extern  cDaemon Myself;
#endif
static bool     bHadConfig = false; // must have a config file, with server name

#ifndef DEF_CONF_FILE
  #define DEF_CONF_FILE "fgms.conf"
#endif

#ifndef SYSCONFDIR
  #define SYSCONFDIR "/usr/etc"
#endif

static int
is_file_or_directory ( char * path )
{
	struct stat buf;
	if (stat(path,&buf) == 0)
	{
		if (buf.st_mode & M_IS_DIR)
			return 2;
		else
			return 1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
//
//      print a help screen for command line parameters
//
//////////////////////////////////////////////////////////////////////
void
PrintHelp ()
{
	cout << endl;
	cout << "\n"
	"options are:\n"
	"-h            print this help screen\n"
	"-a PORT       listen to PORT for telnet\n"
	"-c config     read 'config' as configuration file\n"
	"-p PORT       listen to PORT\n"
	"-t TTL        Time a client is active while not sending packets\n"
	"-o OOR        nautical miles two players must be apart to be out of reach\n"
	"-l LOGFILE    Log to LOGFILE\n"
	"-v LEVEL      verbosity (loglevel) in range 1 (few) and 5 (much)\n"
	"-d            do _not_ run as a daemon (stay in foreground)\n"
	"-D            do run as a daemon\n"
	"\n"
	"the default is to run as a daemon, which can be overridden in the\n"
	"config file.\n"
	"\n";
	exit (0);
} // PrintHelp ()
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
// kludge for getopt() for WIN32
static char * optarg;
static int curr_arg = 0;
int getopt ( int argcount, char* argvars[], char * args )
{
	size_t len = strlen(args);
	size_t i;
	int c = 0;
	if (curr_arg == 0)
	{
		curr_arg = 1;
	}
	if (curr_arg < argcount)
	{
		char * arg = argvars[curr_arg];
		if (*arg == '-')
		{
			arg++;
			c = *arg; // get first char
			for (i = 0; i < len; i++)
			{
				if (c == args[i])
				{	// found 
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
//
//      read a config file and set internal variables accordingly
//
//////////////////////////////////////////////////////////////////////
bool
ProcessConfig ( const string& ConfigName )
{
	FG_CONFIG   Config;
	string      Val;
	int         E;

	if (bHadConfig)	// we already have a config, so ignore
		return (true);
	if (Config.Read (ConfigName))
	{
		return (false);
	}
	SG_ALERT (SG_SYSTEMS, SG_ALERT, "processing " << ConfigName);
	Val = Config.Get ("server.name");
	if (Val != "")
	{
		Servant.SetServerName (Val);
		bHadConfig = true; // got a serve name - minimum 
	}
	Val = Config.Get ("server.address");
	if (Val != "")
	{
		Servant.SetBindAddress (Val);
	}
	Val = Config.Get ("server.port");
	if (Val != "")
	{
		Servant.SetDataPort (StrToNum<int> (Val.c_str (), E));
		if (E)
		{
			SG_ALERT (SG_SYSTEMS, SG_ALERT, "invalid value for DataPort: '" << optarg << "'");
			exit (1);
		}
	}
	Val = Config.Get ("server.telnet_port");
	if (Val != "")
	{
		Servant.SetTelnetPort (StrToNum<int> (Val.c_str (), E));
		if (E)
		{
			SG_ALERT (SG_SYSTEMS, SG_ALERT, "invalid value for TelnetPort: '" << optarg << "'");
			exit (1);
		}
	}
	Val = Config.Get("server.out_of_reach");
	if (Val != "")
	{
		Servant.SetOutOfReach (StrToNum<int> (Val.c_str (), E));
		if (E)
		{
			SG_ALERT (SG_SYSTEMS, SG_ALERT, "invalid value for OutOfReach: '" << optarg << "'");
			exit (1);
		}
	}
	Val = Config.Get("server.playerexpires");
	if (Val != "")
	{
		Servant.SetPlayerExpires (StrToNum<int> (Val.c_str (), E));
		if (E)
		{
			SG_ALERT (SG_SYSTEMS, SG_ALERT, "invalid value for Expire: '" << optarg << "'");
			exit (1);
		}
	}
	Val = Config.Get ("server.logfile");
	if (Val != "")
	{
		Servant.SetLogfile (Val);
	}
	Val = Config.Get ("server.daemon");
	if (Val != "")
	{
		if ((Val == "on") || (Val == "true"))
		{
			RunAsDaemon = true;
		}
		else if ((Val == "off") || (Val == "false"))
		{
			RunAsDaemon = false;
		}
		else
		{
			SG_ALERT (SG_SYSTEMS, SG_ALERT, "unknown value for 'server.daemon'!" << " in file " << ConfigName);
		}
	}
	Val = Config.Get ("server.tracked");
	if (Val != "")
	{
		string  Server;
		int     Port;
		bool    tracked;
		if (Val == "true")
		{
			tracked = true;
		}
		else
		{
			tracked = false;
		}
		Server = Config.Get ("server.tracking_server");
		Val = Config.Get ("server.tracking_port");
		Port = StrToNum<int> (Val.c_str (), E);
		if (E)
		{
			SG_ALERT (SG_SYSTEMS, SG_ALERT, "invalid value for tracking_port: '" << Val << "'");
			exit (1);
		}
    if ( tracked && ( Servant.AddTracker (Server, Port, tracked) != FG_SERVER::SUCCESS ) ) // set master m_IsTracked
    {
			SG_ALERT (SG_SYSTEMS, SG_ALERT, "Failed to get IPC msg queue ID! error " << errno );
			exit (1); // do NOT continue if a requested 'tracker' FAILED
    }
	}
	Val = Config.Get ("server.is_hub");
	if (Val != "")
	{
		if (Val == "true")
		{
			Servant.SetHub (true);
		}
		else
		{
			Servant.SetHub (false);
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
	if (! Config.SetSection (Section))
	{
		MoreToRead = false;
	}
	while (MoreToRead)
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if (Var == "relay.host")
		{ 
			Server = Val;
		}
		if (Var == "relay.port")
		{ 
			Port = StrToNum<int> (Val.c_str(), E);
			if (E)
			{ 
				SG_ALERT (SG_SYSTEMS, SG_ALERT, "invalid value for RelayPort: '" << Val << "'");
				exit (1);
			}
		}
		if ((Server != "") && (Port != 0))
		{ 
			Servant.AddRelay (Server, Port);
			Server = "";
			Port   = 0;
		}
		if (Config.SecNext () == 0)
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
	if (! Config.SetSection (Section))
	{
		MoreToRead = false;
	}
	while (MoreToRead)
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if (Var == "crossfeed.host")
		{
			Server = Val;
		}
		if (Var == "crossfeed.port")
		{
			Port = StrToNum<int> (Val.c_str(), E);
			if (E)
			{
				SG_ALERT (SG_SYSTEMS, SG_ALERT, "invalid value for crossfeed.port: '" << Val << "'");
				exit (1);
			}
		}
		if ((Server != "") && (Port != 0))
		{
			Servant.AddCrossfeed (Server, Port);
			Server = "";
			Port   = 0;
		}
		if (Config.SecNext () == 0)
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
	if (! Config.SetSection (Section))
	{
		MoreToRead = false;
	}
	while (MoreToRead)
	{
		Var = Config.GetName ();
		Val = Config.GetValue();
		if (Var == "blacklist")
		{
			Servant.AddBlacklist (Val);
		}
		if (Config.SecNext () == 0)
		{
			MoreToRead = false;
		}
	}
	//////////////////////////////////////////////////
	return (true);
} // ProcessConfig ( const string& ConfigName )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      parse commandline parameters
//
//////////////////////////////////////////////////////////////////////
int
ParseParams ( int argcount, char* argvars[] )
{
	int     m;
	int     E;

	while ((m=getopt(argcount,argvars,"a:c:dDhl:o:p:t:v:")) != -1) 
	{
		switch (m)
		{
			case 'h':
				cerr << endl;
				cerr << "syntax: " << argvars[0] << " options" << endl;
				PrintHelp ();
				break; // never reached
			case 'a':
				Servant.SetTelnetPort (StrToNum<int> (optarg, E));
				if (E)
				{
					cerr << "invalid value for TelnetPort: '" << optarg << "'" << endl;
					exit(1);
				}
				break;
			case 'c':
				if (is_file_or_directory(optarg) == 1)
				{
					ProcessConfig (optarg);
				} else {
					cerr << "could not read '"
					<< optarg << "' for input!" 
					<< endl;
					exit (1);
				}
				break;
			case 'p':
				Servant.SetDataPort (StrToNum<int>  (optarg, E));
				if (E)
				{
					cerr << "invalid value for DataPort: '"
					<< optarg << "'" << endl;
					exit(1);
				}
				break;
			case 'o':
				Servant.SetOutOfReach (StrToNum<int>  (optarg, E));
				if (E)
				{
					cerr << "invalid value for OutOfReach: '"
					<< optarg << "'" << endl;
					exit(1);
				}
				break;
			case 'v':
				Servant.SetLoglevel (StrToNum<int>  (optarg, E));
				if (E)
				{
					cerr << "invalid value for Loglevel: '"
					<< optarg << "'" << endl;
					exit(1);
				}
				break;
			case 't':
				Servant.SetPlayerExpires (StrToNum<int>  (optarg, E));
				if (E)
				{
					cerr << "invalid value for expire: '"
					<< optarg << "'" << endl;
					exit(1);
				}
				break;
			case 'l':
				Servant.SetLogfile (optarg);
				break;
			case 'd':
				cout << "param daemon = false" << endl;
				RunAsDaemon = false;
				break;
			case 'D':
				cout << "param daemon = true" << endl;
				RunAsDaemon = true;
				break;
			default:
				cerr << endl << endl;
				PrintHelp ();
				exit (1);
		} // switch ()
	} // while ()
	return (1); // success
} // ParseParams()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      read config files
//
//////////////////////////////////////////////////////////////////////
int
ReadConfigs ( bool ReInit = false )
{
	string Path;
#ifndef _MSC_VER
	Path = SYSCONFDIR;
	Path += "/" DEF_CONF_FILE; // fgms.conf
	if (ProcessConfig (Path) == true)
		return 1;
	Path = getenv ("HOME");
#else
	char *cp = getenv("HOME");
	if (cp)
		Path = cp;
	else
	{
		cp = getenv("USERPROFILE"); // XP=C:\Documents and Settings\<name>, Win7=C:\Users\<user>
		if (cp)
			Path = cp;
	}
#endif
	if (Path != "")
	{
		Path += "/" DEF_CONF_FILE;
		if (ProcessConfig (Path))
			return 1;
	}
	if (ProcessConfig (DEF_CONF_FILE))
		return 1;
	return 0;
} // ReadConfigs ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  if we receive a SIGHUP, reinit application
//
//////////////////////////////////////////////////////////////////////
void SigHUPHandler ( int SigType )
{
	Servant.PrepareInit();
	if ( !ReadConfigs (true) )
	{
		SG_ALERT (SG_SYSTEMS, SG_ALERT, "received HUP signal, but read config file failed!");
		exit (1);
	}
	if (Servant.Init () != 0)
	{
		SG_ALERT (SG_SYSTEMS, SG_ALERT, "received HUP signal, but reinit failed!");
		exit (1);
	}
#ifndef _MSC_VER
	signal (SigType, SigHUPHandler);
#endif
} // SigHUPHandler ()
//////////////////////////////////////////////////////////////////////

#ifndef _MSC_VER
//////////////////////////////////////////////////////////////////////
//
//  add the pid of the child to the main exit pon receiving SIGCHLD
//
//////////////////////////////////////////////////////////////////////
void
SigCHLDHandler (int s)
{
	while (waitpid (-1, NULL, WNOHANG) > 0)
		/* intentionally empty */ ;
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "SigCHLDHandler: PID "
    << getpid() << " reason " << s << " called!");
} // SigCHLDHandler ()
//////////////////////////////////////////////////////////////////////
#endif // !_MSC_VER

//////////////////////////////////////////////////////////////////////
//
//      MAIN routine
//
//////////////////////////////////////////////////////////////////////
int
main ( int argc, char* argv[] )
{
	int     I;
#ifndef _MSC_VER
	struct  sigaction sig_child;
#endif
#if defined ENABLE_DEBUG
	//  logbuf::set_log_classes(SG_GENERAL);
#endif
#ifndef _MSC_VER
	// SIGHUP
	signal (SIGHUP, SigHUPHandler);
	// SIGCHLD
	sig_child.sa_handler = SigCHLDHandler;
	sigemptyset (&sig_child.sa_mask);
	sig_child.sa_flags = SA_RESTART;
	if (sigaction (SIGCHLD, &sig_child, NULL) == -1)
	{
    perror("sigaction reported SIGCHLD failed!");
		exit(1);
	}
#endif
	ParseParams (argc, argv);
	ReadConfigs ();
	if ( !bHadConfig )
	{
		SG_ALERT (SG_SYSTEMS, SG_ALERT, "No configuration file '" << DEF_CONF_FILE << "' found!");
		exit(1);
	}
	sglog().setLogLevels( SG_ALL, SG_INFO );
	sglog().enable_with_date (true);
	I = Servant.Init ();
	if (I != 0)
	{
		Servant.CloseTracker();
		return (I);
	}
	#ifndef _MSC_VER
	if (RunAsDaemon)
	{
		Myself.Daemonize ();
		SG_ALERT (SG_SYSTEMS, SG_ALERT, "Main server started!");
	}
	#endif
	I = Servant.Loop();
	if (I != 0)
	{
		Servant.CloseTracker();
		return (I);
	}
	Servant.Done();
	return (0);
} // main()
//////////////////////////////////////////////////////////////////////

// vim: ts=2:sw=2:sts=0

