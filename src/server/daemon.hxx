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
// interface for the daemon-class
//
//////////////////////////////////////////////////////////////////////

#ifndef CDAEMON_HDR
#define CDAEMON_HDR

using namespace std;

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <list>
#include <fcntl.h>

class cDaemon
{
  static pid_t PidOfDaemon;       // remember who we are
  static list <pid_t> Children; // keep track of our children

public:
  cDaemon();
  ~cDaemon ();
  static void SigHandler ( int SigType );
  static int  Daemonize (); // make us a daemon
  static void KillAll (); // kill our children and ourself
  static void AddChild ( pid_t ChildsPid );
  static int  NumChilds ();
  static int  GetPid() { return PidOfDaemon; };
};
#endif

// vim: ts=2:sw=2:sts=0

