/**
 * @file daemon.cxx
 * @author Oliver Schroeder
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
// implement the class "cDaemon", which does everything necessary
// to become a daemon
//
//////////////////////////////////////////////////////////////////////
#ifndef _MSC_VER
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <cstdlib>
#include <fglib/fg_log.hxx>
#include "daemon.hxx"

pid_t cDaemon::PidOfDaemon; // remember who we are
list <pid_t> cDaemon::Children; // keep track of our children

//////////////////////////////////////////////////////////////////////
/**
 * @brief Signal Handler connected in constructor
 * @param SigType The type of signal
 */
void cDaemon::SigHandler ( int SigType )
{
	if (SigType == SIGCHLD)
	{
		int stat;
		pid_t childpid;
		while ((childpid=waitpid (-1, &stat, WNOHANG)) > 0)
			printf("Child stopped: %d\n",childpid);
		return;
	}
	if (SigType == SIGPIPE)
	{
		pid_t childpid;
		childpid = getpid();
		printf("[%d] SIGPIPE received. Connection error.\n",childpid);
		return;
	}
	exit (0);
}

//////////////////////////////////////////////////////////////////////
/** 
 * @brief Installs the signal-handler and makes ourself a daemon
 * @retval int -1 or error, 0 for success
 */
int cDaemon::Daemonize () // make us a daemon
{
	pid_t pid;

	//
	// fork to get rid of our parent
	//
	if ( (pid = fork ()) < 0)
		return (-1);	// something went wrong!
	else if ( pid > 0 )	// parent-process
	{
		PidOfDaemon = 0;
		exit (0);	// good-bye dad!
	}
	//
	// well, my child, do well!
	//
	PidOfDaemon = getpid();
	LOG ( log::URGENT, "# My PID is " << PidOfDaemon );
	setsid ();	// become a session leader
	// chdir ("/");	// make sure, we're not on a mounted fs
	umask (0);	// clear the file creation mode
	return (0);	// ok, that's all volks!
}

//////////////////////////////////////////////////////////////////////
/** @brief Kill our children and ourself
 */
void cDaemon::KillAllChildren () 
{
	list <pid_t>::iterator aChild;

	aChild = Children.begin ();
	while ( aChild != Children.end () )
	{
		LOG ( log::URGENT,
		  "cDaemon: killing child " << (*aChild) );
		if ( kill ((*aChild), SIGTERM))
			kill ((*aChild), SIGKILL);
		aChild++;
	}
	Children.clear ();
	// exit (0);
}


//////////////////////////////////////////////////////////////////////
/** @brief Inserts the ChildsPid in the list of our Children.
 *         So we can keep track of them and kill them if necessary,
 *          e.g. the daemon dies.
 * @param ChildsPid int with pid
 */
void cDaemon::AddChild ( pid_t ChildsPid )
{
	Children.push_back (ChildsPid);
}


//////////////////////////////////////////////////////////////////////
/** @brief Get the count of child processes
 * @retval int No of child processes
 */
int cDaemon::NumChildren ()
{
	return (Children.size ());
}



//////////////////////////////////////////////////////////////////////
/** @brief Connect some signals at startup. These are connected to ::cDaemon::SigHandler
 */
cDaemon::cDaemon()
{
	//
	// catch some signals
	//
	signal (SIGCHLD,SigHandler);
	signal (SIGPIPE,SigHandler);
	PidOfDaemon = getpid(); 
}

//////////////////////////////////////////////////////////////////////
cDaemon::~cDaemon ()
{
	// KillAllChildren ();
}
#endif // !_MSC_VER

