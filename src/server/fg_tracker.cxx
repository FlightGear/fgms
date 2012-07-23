//////////////////////////////////////////////////////////////////////
//
//  server tracker for FlightGear
//  (c) 2006 Julien Pierru
//
//  Licenced under GPL
//
//////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#ifndef DEF_TRACKER_SLEEP
#define DEF_TRACKER_SLEEP 600   // try to connect each ten minutes
#endif // DEF_TRACKER_SLEEP

#ifdef _MSC_VER
	typedef int pid_t;
int getpid(void) {
    return (int)GetCurrentThreadId();
}
#else
	extern  cDaemon Myself;
#endif // !_MSC_VER

#ifndef DEF_DEBUG_OUTPUT
#define DEF_DEBUG_OUTPUT false
#endif

extern bool RunAsDaemon;
static bool AddDebug = DEF_DEBUG_OUTPUT;

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
    if (!RunAsDaemon || AddDebug) printf("FG_TRACKER::FG_TRACKER: Server: %s, Port: %d\n", m_TrackerServer, m_TrackerPort);
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

#ifdef USE_TRACKER_PORT
void *func_Tracker(void *vp)
{
    FG_TRACKER *pt = (FG_TRACKER *)vp;
    pt->Connect();
    pt->TrackerLoop();
    return ((void *)0xdead);
}

#endif // #ifdef USE_TRACKER_PORT

//////////////////////////////////////////////////////////////////////
//
//      Initilize the tracker
//
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::InitTracker ( const int MaxChildren, pid_t *pPIDS )
{
    if (!RunAsDaemon || AddDebug) printf("FG_TRACKER::InitTracker: %d children\n", MaxChildren);
#ifndef NO_TRACKER_PORT
#ifdef USE_TRACKER_PORT
    if (pthread_create( &thread, NULL, func_Tracker, (void*)this )) {
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "# FG_TRACKER::InitTracker: can't create thread...");
        return 1;
    }
#else // !#ifdef USE_TRACKER_PORT
	int i;
	pid_t ChildsPID;
	for ( i=0 ; i<MaxChildren ; i++)
	{
		ChildsPID = fork ();
		if (ChildsPID < 0)
		{
            SG_ALERT (SG_SYSTEMS, SG_ALERT, "# FG_TRACKER::InitTracker: fork(" << i << ") FAILED!");
            return 1;
		}
		else if (ChildsPID == 0)
		{
			Connect ();
			TrackerLoop ();
			exit (0);
		}
		else
		{
            pPIDS[i] = ChildsPID; // parent - store child PID
        }
	}
#endif // #ifdef USE_TRACKER_PORT y/n
#endif // NO_TRACKER_PORT
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
    pid_t pid = getpid();

	sent = true;
	strcpy ( res, "" );

    if (!RunAsDaemon || AddDebug) printf("FG_TRACKER::TrackerLoop entered PID %d\n",pid);
	for ( ; ; )
	{
		length = 0;
		// get message from queue
		if (sent)
		{
#ifndef NO_TRACKER_PORT
#ifdef USE_TRACKER_PORT
            pthread_mutex_lock( &msg_mutex );   // acquire the lock
            pthread_cond_wait( &condition_var, &msg_mutex );    // go wait for the condition
            VI vi = msg_queue.begin(); // get first message
            if (vi != msg_queue.end()) {
                std::string s = *vi;
                msg_queue.erase(vi);    // remove from queue
                length = (int)s.size(); // should I worry about LENGTH???
                strcpy( buf.mtext, s.c_str() ); // mtext is 1024 bytes!!!
            }
            pthread_mutex_unlock( &msg_mutex ); // unlock the mutex
#else // !#ifdef USE_TRACKER_PORT
			length = msgrcv (ipcid, &buf, MAXLINE, 0, MSG_NOERROR);
#endif // #ifdef USE_TRACKER_PORT y/n
#endif // NO_TRACKER_PORT
			buf.mtext[length] = '\0';
			sent = false;
#ifdef ADD_TRACKER_LOG
            if (length)
                write_msg_log(&buf.mtext[0], length, (char *)"OUT: ");
#endif // #ifdef ADD_TRACKER_LOG
		}
		if ( length > 0 )
		{
            if (!RunAsDaemon || AddDebug) printf("FG_TRACKER::TrackerLoop sending msg %d bytes PID %d\n", length,pid);
			// send message via tcp
			if (SWRITE (m_TrackerSocket,buf.mtext,strlen(buf.mtext)) < 0)
			{
				SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::TrackerLoop: can't write to server... PID " << pid);
				Connect ();
			}
			sleep (1);
			// receive answer from server
			if ( SREAD (m_TrackerSocket,res,MAXLINE) <= 0 )
			{
				SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::TrackerLoop: can't read from server... PID " << pid);
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
				else
				{
                    SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::TrackerLoop: Responce " << res << " not OK! Send again... PID " << pid);
				}
			}
		}
		else
		{
			// an error with the queue has occured
			// avoid an infinite loop
			// return (2);
            SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::TrackerLoop: message queue error... PID " << pid);
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
    pid_t pid = getpid();

	//////////////////////////////////////////////////
	// close all inherited open sockets, but
	// leave cin, cout, cerr open
	//////////////////////////////////////////////////
#ifndef _MSC_VER
	for (int i=3; i<32; i++)
		SCLOSE (i);
#endif // !_MSC_VER
	if ( m_TrackerSocket > 0 )
		SCLOSE (m_TrackerSocket);
    if (!RunAsDaemon || AddDebug) printf("FG_TRACKER::Connect: Server: %s, Port: %d PID %d\n", m_TrackerServer, m_TrackerPort, pid);
	while (connected == false)
	{
		m_TrackerSocket = TcpConnect (m_TrackerServer, m_TrackerPort);
		if (m_TrackerSocket >= 0)
		{
			SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::Connect: success PID " << pid);
			connected = true;
		}
		else
		{
			SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::Connect: failed! sleep " << DEF_TRACKER_SLEEP << " secs PID " << pid);
			sleep (DEF_TRACKER_SLEEP);  // sleep 10 minutes and try again
		}
	}
	sleep (5);
	SWRITE(m_TrackerSocket,"REPLY",sizeof("REPLY"));
    if (!RunAsDaemon || AddDebug) printf("FG_TRACKER::Connect: Written 'REPLY' PID %d\n",pid);
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
    m_TrackerSocket = 0;
} // Disconnect ()
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#if !defined(NTDDI_VERSION) || !defined(NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA)   // if less than VISTA, provide alternative
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
#endif // #if (NTDDI_VERSION < NTDDI_VISTA)
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
    if (SERROR(sockfd))
    {
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "# FG_TRACKER::TcpConnect: can't get socket...");
        return -1;

    }
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(server_port);
#ifdef _MSC_VER
	if ( inet_pton(AF_INET, server_address, &serveraddr.sin_addr) == -1 ) {
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "# FG_TRACKER::TcpConnect: inet_pton failed!");
        if (!SERROR(sockfd))
            SCLOSE(sockfd);
		return -1;
    }
#else
	inet_pton(AF_INET, server_address, &serveraddr.sin_addr);
#endif
	if (connect(sockfd, (SA *) &serveraddr, sizeof(serveraddr))<0 ) {
        if (!SERROR(sockfd))
            SCLOSE(sockfd); // close the socket
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "# FG_TRACKER::TcpConnect: connect failed!");
		return -1;
    } else
		return (sockfd);
}  // TcpConnect  ()
//////////////////////////////////////////////////////////////////////
