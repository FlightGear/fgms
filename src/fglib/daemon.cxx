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
 * @file	daemon.cxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	2006
 */


//////////////////////////////////////////////////////////////////////
//
// implement the class "daemon", which does everything necessary
// to become a daemon
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
	#error "never include <fglib/daemon.hxx> on windows"
#endif

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fglib/fg_log.hxx>
#include "daemon.hxx"

namespace fgmp
{

int  daemon::my_pid; // remember who we are
bool daemon::already_daemon{ false };

//////////////////////////////////////////////////////////////////////

/**
 * @brief Signal Handler connected in constructor
 * @param SigType The type of signal
 */
void
daemon::sig_chld
(
	int sig_type
)
{
	if (sig_type == SIGCHLD)
	{
		int stat;
		pid_t childpid;
		while ((childpid=waitpid (-1, &stat, WNOHANG)) > 0)
			printf("Child stopped: %d\n",childpid);
		return;
	}
	exit (0);
}

//////////////////////////////////////////////////////////////////////

/** Installs the signal-handler and makes ourself a daemon
 *
 * @retval int -1 or error, 0 for success
 */
int
daemon::daemonize
()
{
	if ( already_daemon )
		return 0;
	pid_t pid;

	//
	// fork to get rid of our parent
	//
	if ( (pid = fork ()) < 0)
		return (-1);	// something went wrong!
	else if ( pid > 0 )	// parent-process
	{
		my_pid = 0;
		exit (0);	// good-bye dad!
	}
	//
	// well, my child, do well!
	//
	my_pid = getpid();
	setsid ();	// become a session leader
	// chdir ("/");	// make sure, we're not on a mounted fs
	umask (0);	// clear the file creation mode
	already_daemon = true;
	return (0);	// ok, that's all volks!
}

//////////////////////////////////////////////////////////////////////

/** Connect some signals at startup. These are connected to sig_chld
 */
daemon::daemon
()
{
	//
	// catch some signals
	//
	signal (SIGCHLD,sig_chld);
	my_pid = getpid(); 
}

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

