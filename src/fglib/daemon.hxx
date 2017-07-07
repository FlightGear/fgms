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
 * @file	daemon.hxx
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	2006
 */

//////////////////////////////////////////////////////////////////////
//
// interface for the daemon-class
//
//////////////////////////////////////////////////////////////////////


#ifndef CDAEMON_HDR
#define CDAEMON_HDR

#ifndef _MSC_VER

/** Implement everything necessary to become a daemon
 */
class Daemon
{
	/** @brief remember who we are */
	static int my_pid;     

	/** Keep track if we are already daemonized */
	static bool already_daemon;

public:
	Daemon();
	/** handle children which died unexpectedly */
	static void sig_chld ( int SigType );
	/** make ourself a daemon */
	static int  daemonize ();
	/** Return  our pid */
	static int  get_pid() { return my_pid; };
};

#endif // #ifndef _MSC_VER


#endif


