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

#define MAXLINE 4096

//////////////////////////////////////////////////////////////////////
//
//      Initilize to standard values
//
//////////////////////////////////////////////////////////////////////
FG_TRACKER::FG_TRACKER (int port, string server, int id)
{
  ipcid         = id;
  trackerPort   = port;
  max_children  = 2; // good for 32 users/server
  strcpy (trackerServer, server.c_str());
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
FG_TRACKER::InitTracker ()
{
  int i;
  
  for ( i=0 ; i<max_children ; i++)
  if ( (fork())==0)
  {
    trackerSocket = tcp_connect(trackerServer, trackerPort);
    
    if (trackerSocket < 0) return (2);
    
    sleep (5);
    write(trackerSocket,"REPLY",sizeof("REPLY"));
    sleep (2);
      
    TrackerLoop ();
  
    exit (0);
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
  struct  msgbuffer buf;
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
      length = msgrcv(ipcid,&buf,MAXLINE,0,MSG_NOERROR);
      buf.mtext[length] = '\0';
      sent = false;
    }
    
    if ( length > 0 )
    {
      // send message via tcp
      write (trackerSocket,buf.mtext,strlen(buf.mtext));
      sleep (1);
      
      // receive answer from server
      if ( read (trackerSocket,res,MAXLINE) <= 0 )
      {
        while ( Reconnect () != 0)
        {
          sleep (2);
        }
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
      return (2);
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
FG_TRACKER::Reconnect ()
{
  printf ("Reconnecting...\n");
  trackerSocket = tcp_connect(trackerServer, trackerPort);
    
  if (trackerSocket < 0) return (2);
    
  sleep (5);
  write(trackerSocket,"REPLY",sizeof("REPLY"));
  sleep (2);

  return (0);
} // Reconnect ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  disconnect the tracker from its server
//
//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::Disconnect ()
{
  if ( trackerSocket > 0 )
    close (trackerSocket);
} // Disconnect ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  creates a TCP connection
//
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::tcp_connect(char *server_address,int server_port)
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
}  // tcp_connect ()
//////////////////////////////////////////////////////////////////////
