//////////////////////////////////////////////////////////////////////
//
//  server tracker for FlightGear
//  (c) 2006 Julien Pierru
//
//  Licenced under GPL
//
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <endian.h>
#include <unistd.h>
#include "common.h"
#include "fg_tracker.hxx"
#include "typcnvt.hxx"
#include <simgear/debug/logstream.hxx>
#include "daemon.hxx"

#define MAXLINE 4096

extern  cDaemon Myself;

//////////////////////////////////////////////////////////////////////
//
//      Initilize to standard values
//
//////////////////////////////////////////////////////////////////////
FG_TRACKER::FG_TRACKER (int port, string server, int id)
{
  ipcid         = id;
  m_TrackerPort = port;
  strcpy (m_TrackerServer, server.c_str());
} // FG_TRACKER()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      terminate the tracker
//
//////////////////////////////////////////////////////////////////////
FG_TRACKER::~FG_TRACKER ()
{

} // ~FG_TRACKER()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      Initilize the tracker
//
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::InitTracker ( const int MaxChildren )
{
  int i;
  pid_t ChildsPID;

  for ( i=0 ; i<MaxChildren ; i++)
  {
      ChildsPID = fork ();
      if (ChildsPID == 0)
      {
        Connect ();
        TrackerLoop ();
        exit (0);
      }
      if (ChildsPID > 0)
      {
          Myself.AddChild (ChildsPID);
          SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER PID:" << ChildsPID);
      }
  }
  return (0);
} // InitTracker (int port, string server, int id, int pid)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  send the messages to the tracker server
//
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::TrackerLoop ()
{
  m_MsgBuffer buf;
  bool  sent;
  int   length;
  char  res[MAXLINE];

  sent = true;
  strcpy ( res, "" );

  for ( ; ; )
  {
    length = 0;
    // get message from queue
    if (sent)
    {
      length = msgrcv (ipcid, &buf, MAXLINE, 0, MSG_NOERROR);
      buf.mtext[length] = '\0';
      sent = false;
    }
    if ( length > 0 )
    {
      // send message via tcp
      if (write (m_TrackerSocket,buf.mtext,strlen(buf.mtext)) < 0)
      {
        SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::TrackerLoop: can't write to server...");
        Connect ();
      }
      sleep (1);
      // receive answer from server
      if ( read (m_TrackerSocket,res,MAXLINE) <= 0 )
      {
        SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::TrackerLoop: can't read from server...");
        Connect ();
        sent = false;
      }
      else
      {
        if ( strncmp( res, "OK", 2 ) == 0 )
        {
          sent = true;
          strcpy ( res, "" );
        }
      }
    }
    else
    {
      // an error with the queue has occured
      // avoid an infinite loop
      // return (2);
      sent = true;
    }
  }
  return (0);
} // TrackerLoop ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  reconnect the tracker to its server
//
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::Connect ()
{
  bool connected = false;

  if ( m_TrackerSocket > 0 )
    close (m_TrackerSocket);
  while (connected == false)
  {
    m_TrackerSocket = TcpConnect (m_TrackerServer, m_TrackerPort);
    if (m_TrackerSocket >= 0)
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::Connect: success");
      connected = true;
    }
    else
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::Connect: failed");
      sleep (600);  // sleep 10 minutes and try again
    }
  }
  sleep (5);
  write(m_TrackerSocket,"REPLY",sizeof("REPLY"));
  sleep (2);
  return (0);
} // Connect ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  disconnect the tracker from its server
//
//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::Disconnect ()
{
  if ( m_TrackerSocket > 0 )
    close (m_TrackerSocket);
} // Disconnect ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  creates a TCP connection
//
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::TcpConnect (char *server_address,int server_port)
{
    struct sockaddr_in serveraddr;
    int sockfd;

    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_address, &serveraddr.sin_addr);
    if (connect(sockfd, (SA *) &serveraddr, sizeof(serveraddr))<0 )
        return -1;
    else
        return (sockfd);
}  // TcpConnect  ()
//////////////////////////////////////////////////////////////////////
