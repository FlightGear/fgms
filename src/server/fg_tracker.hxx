//////////////////////////////////////////////////////////////////////
//
//  server tracker for FlightGear
//  (c) 2006 Julien Pierru
//
//  Licenced under GPL
//
//////////////////////////////////////////////////////////////////////

#if not defined FG_TRACKER_HPP
#define FG_TRACKER_HPP

using namespace std;

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include "daemon.hxx"
#include "fg_geometry.hxx"

#define CONNECT     0
#define DISCONNECT  1
#define UPDATE    2

#define IPCKEY    0xf9f5
#define IPCPERMS    0600

struct msgbuffer { long mtype; char mtext[1024];};

//////////////////////////////////////////////////////////////////////
//
//  the tracker class
//
//////////////////////////////////////////////////////////////////////
class FG_TRACKER
{
public:
  //////////////////////////////////////////////////
  //
  //  constructors
  //
  //////////////////////////////////////////////////
  FG_TRACKER (int port, string server, int id);
  ~FG_TRACKER ();

  //////////////////////////////////////////////////
  //
  //  public methods
  //
  //////////////////////////////////////////////////
  int InitTracker ();
  int   tcp_connect (char *server_address, int server_port);
  int TrackerLoop ();
  string  GetTrackerServer () { return trackerServer; };
  int GetTrackerPort () { return trackerPort; };
  void  Disconnect ();

private:
  //////////////////////////////////////////////////
  //
  //  private methods
  //
  //////////////////////////////////////////////////
  int Reconnect ();

  //////////////////////////////////////////////////
  //
  //  private variables
  //  
  //////////////////////////////////////////////////
  int ipcid;
  int trackerPort;
  char  trackerServer[128];
  int   trackerSocket;
  int max_children;
};
#endif
