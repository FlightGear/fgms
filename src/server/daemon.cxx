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
#include <simgear/debug/logstream.hxx>
#include "daemon.hxx"

pid_t cDaemon::PidOfDaemon; // remember who we are
list <pid_t> cDaemon::Children; // keep track of our children

//////////////////////////////////////////////////////////////////////
// SigHandler ()
//////////////////////////////////////////////////////////////////////
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
		int stat;
		pid_t childpid;
		childpid = getpid();
		printf("[%d] SIGPIPE received. Connection error.\n",childpid);
		return;
	}
	switch (SigType)
	{
		case  1:cout << "killed by SIGHUP! ";
			cout << "Hangup (POSIX)" << endl;
			break;
		case  2:cout << "killed by SIGINT! ";
			cout << "Interrupt (ANSI)" << endl;
			break;
		case  3:cout << "killed by SIGQUIT! ";
			cout << "Quit (POSIX)" << endl;
			break;
		case  4:cout << "killed by SIGILL! ";
			cout << "Illegal instruction (ANSI)" << endl;
			break;
		case  5:cout << "killed by SIGTRAP! ";
			cout << "Trace trap (POSIX)" << endl;
			break;
		case  6:cout << "killed by SIGABRT! ";
			cout << "IOT trap (4.2 BSD)" << endl;
			break;
		case  7:cout << "killed by SIGBUS! ";
			cout << "BUS error (4.2 BSD)" << endl;
			break;
		case  8:cout << "killed by SIGFPE! ";
			cout << "Floating-point exception (ANSI)" << endl;
			break;
		case  9:cout << "killed by SIGKILL! ";
			cout << "Kill, unblockable (POSIX)" << endl;
			break;
		case 10:cout << "killed by SIGUSR1! ";
			cout << "User-defined signal 1 (POSIX)" << endl;
			break;
		case 11:cout << "killed by SIGSEGV! ";
			cout << "Segmentation violation (ANSI)" << endl;
			break;
		case 12:cout << "killed by SIGUSR2! ";
			cout << "User-defined signal 2 (POSIX)" << endl;
			break;
		case 13:cout << "killed by SIGPIPE! ";
			cout << "Broken pipe (POSIX)" << endl;
			break;
		case 14:cout << "killed by SIGALRM! ";
			cout << "Alarm clock (POSIX)" << endl;
			break;
		case 15:cout << "killed by SIGTERM! ";
			cout << "Termination (ANSI)" << endl;
			break;
		case 16:cout << "killed by SIGSTKFLT! ";
			cout << "Stack fault" << endl;
			break;
		case 17:cout << "killed by SIGCHLD! ";
			cout << "Child status has changed (POSIX)" << endl;
			break;
		case 18:cout << "killed by SIGCONT! ";
			cout << "Continue (POSIX)" << endl;
			break;
		case 19:cout << "killed by SIGSTOP! ";
			cout << "Stop, unblockable (POSIX)" << endl;
			break;
		case 20: cout << "killed by SIGTSTP! ";
			cout << "Keyboard stop (POSIX)" << endl;
			break;
		case 21:cout << "killed by SIGTTIN! ";
			cout << "Background read from tty (POSIX)" << endl;
			break;
		case 22:cout << "killed by SIGTTOU! ";
			cout << "Background write to tty (POSIX)" << endl;
			break;
		case 23:cout << "killed by SIGURG! ";
			cout << "Urgent condition on socket (4.2 BSD)" << endl;
			break;
		case 24:cout << "killed by SIGXCPU! ";
			cout << "CPU limit exceeded (4.2 BSD)" << endl;
			break;
		case 25:cout << "killed by SIGXFSZ! ";
			cout << "File size limit exceeded (4.2 BSD)" << endl;
			break;
		case 26:cout << "killed by SIGVTALRM! ";
			cout << "Virtual alarm clock (4.2 BSD)" << endl;
			break;
		case 27:cout << "killed by SIGPROF! ";
			cout << "Profiling alarm clock (4.2 BSD)" << endl;
			break;
		case 28:cout << "killed by SIGWINCH! ";
			cout << "Window size change (4.3 BSD, Sun)" << endl;
			break;
		case 29: cout << "killed by SIGIO! ";
			cout << "I/O now possible (4.2 BSD)" << endl;
			break;
		case 30:cout << "killed by SIGPWR! ";
			cout << "Power failure restart (System V)" << endl;
			break;
		default:cout << "killed by signal " << SigType << "!" << endl;
	}
	exit (0);
}

//////////////////////////////////////////////////////////////////////
// Daemonize ()
// installs the signal-handler and makes ourself a daemon
//////////////////////////////////////////////////////////////////////
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
	SG_ALERT (SG_SYSTEMS, SG_ALERT, "# My PID is " << PidOfDaemon);
	setsid ();	// become a session leader
	// chdir ("/");	// make sure, we're not on a mounted fs
	umask (0);	// clear the file creation mode
	return (0);	// ok, that's all volks!
}

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
void cDaemon::KillAllChildren ()  // kill our children and ourself
{
	list <pid_t>::iterator aChild;

	aChild = Children.begin ();
	while ( aChild != Children.end () )
	{
		SG_LOG (SG_SYSTEMS, SG_ALERT, "cDaemon: killing child " << (*aChild));
		if ( kill ((*aChild), SIGTERM))
			kill ((*aChild), SIGKILL);
		aChild++;
	}
	Children.clear ();
	// exit (0);
}

//////////////////////////////////////////////////////////////////////
// AddChild ()
// inserts the ChildsPid in the list of our Children.
// So we can keep track of them and kill them if necessary,
// e.g. the daemon dies.
//////////////////////////////////////////////////////////////////////
void cDaemon::AddChild ( pid_t ChildsPid )
{
	Children.push_back (ChildsPid);
}

int cDaemon::NumChildren ()
{
	return (Children.size ());
}

cDaemon::cDaemon()
{
	//
	// catch some signals
	//
	signal (SIGINT,SigHandler);
	signal (SIGHUP,SigHandler);
	signal (SIGTERM,SigHandler);
	signal (SIGCHLD,SigHandler);
	signal (SIGPIPE,SigHandler);
	PidOfDaemon = getpid(); 
}

cDaemon::~cDaemon ()
{
	// KillAllChildren ();
}
#endif // !_MSC_VER

// vim: ts=2:sw=2:sts=0
