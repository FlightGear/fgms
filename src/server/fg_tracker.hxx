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
  int       InitTracker ( const int MaxChildren );
  int       TcpConnect (char *server_address, int server_port);
  int       TrackerLoop ();
  string    GetTrackerServer () { return m_TrackerServer; };
  int       GetTrackerPort () { return m_TrackerPort; };
  void      Disconnect ();

  class  m_MsgBuffer
  {
    public:
    long mtype; 
    char mtext[1024];
  };

private:
  //////////////////////////////////////////////////
  //
  //  private methods
  //
  //////////////////////////////////////////////////
  int Connect ();

  //////////////////////////////////////////////////
  //
  //  private variables
  //  
  //////////////////////////////////////////////////
  int   ipcid;
  int   m_TrackerPort;
  char  m_TrackerServer[128];
  int   m_TrackerSocket;
};
#endif
