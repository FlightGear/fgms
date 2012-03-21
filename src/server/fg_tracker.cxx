//////////////////////////////////////////////////////////////////////
//
//  server tracker for FlightGear
//  (c) 2006 Julien Pierru
//
//  Licenced under GPL
//
//////////////////////////////////////////////////////////////////////
#include "config.h"

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <string.h>
#ifndef _MSC_VER
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <endian.h>
#endif
#include <unistd.h>
#include "common.h"
#include "fg_tracker.hxx"
#include "typcnvt.hxx"
#include <simgear/debug/logstream.hxx>
#include "daemon.hxx"

#define MAXLINE 4096

#ifdef _MSC_VER
	typedef int pid_t;
#else
	extern  cDaemon Myself;
#endif // !_MSC_VER

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
	#ifndef NO_TRACKER_PORT
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
		#ifndef _MSC_VER
		if (ChildsPID > 0)
		{
			SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER PID:" << ChildsPID);
		}
		#endif // _MSC_VER
	}
	#endif // NO_TRACKER_PORT
	return (0);
} // InitTracker (int port, string server, int id, int pid)
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
	#define SWRITE(a,b,c) send(a,b,c,0)
	#define SREAD(a,b,c)  recv(a,b,c,0)
	#define SCLOSE closesocket
#else
	#define SWRITE write
	#define SREAD  read
	#define SCLOSE close
#endif
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
			#ifndef NO_TRACKER_PORT
			length = msgrcv (ipcid, &buf, MAXLINE, 0, MSG_NOERROR);
			#endif // NO_TRACKER_PORT
			buf.mtext[length] = '\0';
			sent = false;
		}
		if ( length > 0 )
		{
			// send message via tcp
			if (SWRITE (m_TrackerSocket,buf.mtext,strlen(buf.mtext)) < 0)
			{
				SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::TrackerLoop: can't write to server...");
				Connect ();
			}
			sleep (1);
			// receive answer from server
			if ( SREAD (m_TrackerSocket,res,MAXLINE) <= 0 )
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
		SCLOSE (m_TrackerSocket);
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
	SWRITE(m_TrackerSocket,"REPLY",sizeof("REPLY"));
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
		SCLOSE (m_TrackerSocket);
} // Disconnect ()
//////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#ifndef EAFNOSUPPORT
#define	EAFNOSUPPORT	97	/* not present in errno.h provided with VC */
#endif
int inet_aton(const char *cp, struct in_addr *addr)
{
	addr->s_addr = inet_addr(cp);
	return (addr->s_addr == INADDR_NONE) ? -1 : 0;
}
int inet_pton(int af, const char *src, void *dst)
{
	if (af != AF_INET)
	{
		errno = EAFNOSUPPORT;
		return -1;
	}
	return inet_aton (src, (struct in_addr *)dst);
}
#endif // _MSC_VER
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
#ifdef _MSC_VER
	if ( inet_pton(AF_INET, server_address, &serveraddr.sin_addr) == -1 )
		return -1;
#else
	inet_pton(AF_INET, server_address, &serveraddr.sin_addr);
#endif
	if (connect(sockfd, (SA *) &serveraddr, sizeof(serveraddr))<0 )
		return -1;
	else
		return (sockfd);
}  // TcpConnect  ()
//////////////////////////////////////////////////////////////////////
