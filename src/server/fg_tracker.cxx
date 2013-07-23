/**
 * @file fg_tracker.cxx
 * @author (c) 2006 Julien Pierru
 * @author (c) 2012 Rob Dosogne ( FreeBSD friendly )
 *
 * @todo Pete To make a links here to the config and explain a bit
 *
 */

//////////////////////////////////////////////////////////////////////
//
//  server tracker for FlightGear
//  (c) 2006 Julien Pierru
//  (c) 2012 Rob Dosogne ( FreeBSD friendly )
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
#include <sstream>
#ifndef _MSC_VER
	#include <errno.h>
	#include <time.h>
	#include <stdint.h>
	#include <unistd.h>
	#include <sys/ipc.h>
	#include <sys/msg.h>
	#include <sys/types.h>
	#include <signal.h>
	#ifndef __FreeBSD__
		#include <endian.h>
	#endif
#endif
#include <unistd.h>
#include <stdio.h>
#include "fg_common.hxx"
#include "fg_tracker.hxx"
#include "fg_util.hxx"
#include <simgear/debug/logstream.hxx>
#include "daemon.hxx"
#include <libcli/debug.hxx>

#ifndef DEF_TRACKER_SLEEP
	// #define DEF_TRACKER_SLEEP 300   // try to connect each Five minutes
	#define DEF_TRACKER_SLEEP 30   // try to connect each Five minutes
#endif // DEF_TRACKER_SLEEP

#ifdef _MSC_VER
	typedef int pid_t;
	int getpid ( void )
	{
		return ( int ) GetCurrentThreadId();
	}
#endif // !_MSC_VER

logstream* fgms_logstream = NULL;
inline logstream&
tracklog()
{
        if ( fgms_logstream == NULL )
        {
                fgms_logstream = new logstream ( cerr );
        }
        return *fgms_logstream;
}

// #define TRACK_LOG(C,P,M) tracklog()   << loglevel(C,P) << tracklog().datestr() << M << std::endl
#define TRACK_LOG(C,P,M) sglog()   << loglevel(C,P) << sglog().datestr() << M << std::endl

//////////////////////////////////////////////////////////////////////
/**
 * @brief Initialize to standard values
 * @param port
 * @param server ip or domain
 * @param id  what is id? -- todo --
 */
FG_TRACKER::FG_TRACKER ( int port, string server, int id )
{
	m_TrackerPort	= port;
	m_TrackerServer = server;
	m_TrackerSocket = 0;
	TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::FG_TRACKER:"
		<< m_TrackerServer << ", Port: " << m_TrackerPort
	);
	LastSeen	= 0;
	LastSent	= 0;
	BytesSent	= 0;
	BytesRcvd	= 0;
	PktsSent	= 0;
	PktsRcvd	= 0;
	LostConnections = 0;
	LastConnected	= 0;
} // FG_TRACKER()

//////////////////////////////////////////////////////////////////////
/**
 * @brief xTerminate the tracker
 */
FG_TRACKER::~FG_TRACKER ()
{
	pthread_mutex_unlock ( &msg_mutex ); // give up the lock
	WriteQueue ();
	msg_queue.clear ();
} // ~FG_TRACKER()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::ReadQueue ()
{
	
	//////////////////////////////////////////////////
	// FIXME: this needs a better mechanism.
	// This is fire and forget, and forgets
	// messages if the server is not reachable
	//////////////////////////////////////////////////
	ifstream queue_file;
	queue_file.open ("queue_file");
	if (! queue_file)
		return;
	string line;
	while ( getline (queue_file, line, '\n') )
	{
		if (TrackerWrite (line) < 0)
		{
			m_connected = false;
			queue_file.close();
			return;
		}
	}
	queue_file.close();
	remove ("queue_file");
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::WriteQueue ()
{
	VI CurrentMessage;
	ofstream queue_file;

	pthread_mutex_lock ( &msg_mutex ); // give up the lock
	if (msg_queue.size() == 0)
		return;
	queue_file.open ( "queue_file", ios::out|ios::app );
	if (! queue_file)
	{
		cout << "could not open queuefile!" << endl;
		return;
	}
	CurrentMessage = msg_queue.begin(); // get first message
	while (CurrentMessage != msg_queue.end())
	{
		queue_file << (*CurrentMessage) << endl;
		CurrentMessage++;
	}
	pthread_mutex_unlock ( &msg_mutex ); // give up the lock
	queue_file.close ();
}
//////////////////////////////////////////////////////////////////////

void* func_Tracker ( void* vp )
{
	FG_TRACKER* pt = ( FG_TRACKER* ) vp;
	pt->TrackerLoop();
	return ( ( void* ) 0xdead );
}

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Initialize the tracker as a new process
 * @param pPIDS -- to do --
 */
int
FG_TRACKER::InitTracker ()
{
	pthread_t thread;
	if ( pthread_create ( &thread, NULL, func_Tracker, ( void* ) this ) )
	{
		TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::InitTracker: can't create thread..." );
		return 1;
	}
	return ( 0 );
} // InitTracker ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::AddMessage
(
	const string & message
)
{
	pthread_mutex_lock ( &msg_mutex ); // acquire the lock 
	if (msg_queue.size () > 512)
	{
		TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER queue full, writeing backlog...");
		WriteQueue ();
		msg_queue.clear();
	}
	msg_queue.push_back ( message.c_str() ); // queue the message
	pthread_cond_signal ( &condition_var ); // wake up the worker
	pthread_mutex_unlock ( &msg_mutex ); // give up the lock
} // FG_TRACKER::AddMessage()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::TrackerWrite (const string& str)
{
	size_t l   = str.size() + 1;
	LastSent   = time(0);
	size_t s   = m_TrackerSocket->send (str.c_str(), l, MSG_NOSIGNAL);
	BytesSent += s;
	PktsSent++;
	return s;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::ReplyToServer (const string& str)
{
	string reply;

	if (str == "OK")
	{
		// set timeout time to 0
		return;
	}
	else if (str == "PING")
	{
		reply = "PONG STATUS OK";
		if (TrackerWrite (reply) < 0)
		{
			m_connected = false;
			LostConnections++;
			TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop: "
				<< "lost connection to server"
			);
			return;
		}
		TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::TrackerLoop: "
			<< "PING from server received"
		);
		return;
	}
	TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop: "
		<< "Responce not recognized. Msg: '" << str
	);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Send the messages to the tracker server
 */
int
FG_TRACKER::TrackerLoop ()
{
	VI	CurrentMessage;
	int 	i=0;
	size_t	length;
	string	Msg;
	char	res[MSGMAXLINE];	/*Msg from/to server*/

	pthread_mutex_init ( &msg_mutex, 0 ); 
	pthread_cond_init  ( &condition_var, 0 ); 
	// tracklog().setLogLevels ( SG_FGTRACKER, SG_INFO );
	length = 0;
	/*Infinite loop*/
	for ( ; ; )
	{
		while (! m_connected)
		{
			TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop: "
				<< "trying to connect"
			);
			m_connected = Connect();
			if (! m_connected )
			{
				TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop: "
					<< "not connected, will slepp for " << DEF_TRACKER_SLEEP << " seconds"
				);
				sleep (DEF_TRACKER_SLEEP);
			}
			ReadQueue (); 	// read backlog, if any
		}
		while (length == 0)
		{
			pthread_mutex_lock ( &msg_mutex );
			length = msg_queue.size ();
			pthread_mutex_unlock ( &msg_mutex );
			if (length == 0)
			{	// wait for data
				pthread_mutex_lock ( &msg_mutex );
				pthread_cond_wait ( &condition_var, &msg_mutex );   // go wait for the condition
				length = msg_queue.size ();
				pthread_mutex_unlock ( &msg_mutex );
			}
		}
		while (length)
		{
			pthread_mutex_lock ( &msg_mutex );
			CurrentMessage = msg_queue.begin(); // get first message
			Msg = (*CurrentMessage).c_str();
			CurrentMessage = msg_queue.erase(CurrentMessage);
			length = msg_queue.size ();
			pthread_mutex_unlock ( &msg_mutex );
#ifdef ADD_TRACKER_LOG
			write_msg_log ( Msg.c_str(), Msg.size(), ( char* ) "OUT: " );
#endif // #ifdef ADD_TRACKER_LOG
			TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::TrackerLoop: "
				<< "sending msg " << Msg.size() << "  bytes: " << Msg
			);
			if ( TrackerWrite (Msg) < 0 )
			{
				TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop: "
					<< "Can't write to server..."
				);
				LostConnections++;
				m_connected = false;
				AddMessage (Msg);	// requeue message
				length = 0;
				continue;
			}
			Msg = "";
			i = m_TrackerSocket->recv (res, MSGMAXLINE, 0);
			if (i >= 0)
			{	// something received from tracker server
				res[i]='\0';
				LastSeen = time (0);
				PktsRcvd++;
				BytesRcvd += i;
				ReplyToServer (res);
			}
		}
	}
	return ( 0 );
} // TrackerLoop ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  (Re)connect the tracker to its server
//	RETURN: true = success, false = failed
//
//////////////////////////////////////////////////////////////////////
bool
FG_TRACKER::Connect()
{
	if ( m_TrackerSocket )
	{
		m_TrackerSocket->close ();
		delete m_TrackerSocket;
	}
	m_TrackerSocket = new netSocket();
	TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::Connect: "
		<< "Server: " << m_TrackerServer << ", Port: " << m_TrackerPort);
	if ( m_TrackerSocket->open (true) == false)
	{
		TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Connect: "
			<< "Can't get socket..."
		);
		return false;
	}
	if ( m_TrackerSocket->connect ( m_TrackerServer.c_str(), m_TrackerPort ) < 0 )
	{
		TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Connect: "
			<< "Connect failed!"
		);
		return false;
	}
	m_TrackerSocket->setBlocking ( false );
	TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Connect: "
		<< "success"
	);
	LastConnected	= time (0);
	m_TrackerSocket->write_char ('\0');
	sleep ( 2 );
	TrackerWrite ("NOWAIT");
	TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::Connect: "
		<< "Written 'NOWAIT'"
		);
	sleep ( 1 );
	return true;
} // Connect ()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Disconnect the tracker from its server
 */
void
FG_TRACKER::Disconnect ()
{
	if ( m_TrackerSocket )
	{
		m_TrackerSocket->close ();
		delete m_TrackerSocket;
	}
} // Disconnect ()
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#if !defined(NTDDI_VERSION) || !defined(NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA)   // if less than VISTA, provide alternative
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT    97      /* not present in errno.h provided with VC */
#endif
int inet_aton ( const char* cp, struct in_addr* addr )
{
	addr->s_addr = inet_addr ( cp );
	return ( addr->s_addr == INADDR_NONE ) ? -1 : 0;
}
int inet_pton ( int af, const char* src, void* dst )
{
	if ( af != AF_INET )
	{
		errno = EAFNOSUPPORT;
		return -1;
	}
	return inet_aton ( src, ( struct in_addr* ) dst );
}
#endif // #if (NTDDI_VERSION < NTDDI_VISTA)
#endif // _MSC_VER

//////////////////////////////////////////////////////////////////////
/**
 * @brief Signal handling
 * @param s int with the signal
 */
void
signal_handler ( int s )
{
#ifndef _MSC_VER
	pid_t mypid = getpid();
	switch ( s )
	{
	case  1:
		printf ( "[%d] SIGHUP received, exiting...\n",mypid );
		exit ( 0 );
		break;
	case  2:
		printf ( "[%d] SIGINT received, exiting...\n",mypid );
		exit ( 0 );
		break;
	case  3:
		printf ( "[%d] SIGQUIT received, exiting...\n",mypid );
		break;
	case  4:
		printf ( "[%d] SIGILL received\n",mypid );
		break;
	case  5:
		printf ( "[%d] SIGTRAP received\n",mypid );
		break;
	case  6:
		printf ( "[%d] SIGABRT received\n",mypid );
		break;
	case  7:
		printf ( "[%d] SIGBUS received\n",mypid );
		break;
	case  8:
		printf ( "[%d] SIGFPE received\n",mypid );
		break;
	case  9:
		printf ( "[%d] SIGKILL received\n",mypid );
		exit ( 0 );
		break;
	case 10:
		printf ( "[%d] SIGUSR1 received\n",mypid );
		break;
	case 11:
		printf ( "[%d] SIGSEGV received. Exiting...\n",mypid );
		exit ( 1 );
		break;
	case 12:
		printf ( "[%d] SIGUSR2 received\n",mypid );
		break;
	case 13:
		printf ( "[%d] SIGPIPE received. Connection Error.\n",mypid );
		break;
	case 14:
		printf ( "[%d] SIGALRM received\n",mypid );
		break;
	case 15:
		printf ( "[%d] SIGTERM received\n",mypid );
		exit ( 0 );
		break;
	case 16:
		printf ( "[%d] SIGSTKFLT received\n",mypid );
		break;
	case 17:
		printf ( "[%d] SIGCHLD received\n",mypid );
		break;
	case 18:
		printf ( "[%d] SIGCONT received\n",mypid );
		break;
	case 19:
		printf ( "[%d] SIGSTOP received\n",mypid );
		break;
	case 20:
		printf ( "[%d] SIGTSTP received\n",mypid );
		break;
	case 21:
		printf ( "[%d] SIGTTIN received\n",mypid );
		break;
	case 22:
		printf ( "[%d] SIGTTOU received\n",mypid );
		break;
	case 23:
		printf ( "[%d] SIGURG received\n",mypid );
		break;
	case 24:
		printf ( "[%d] SIGXCPU received\n",mypid );
		break;
	case 25:
		printf ( "[%d] SIGXFSZ received\n",mypid );
		break;
	case 26:
		printf ( "[%d] SIGVTALRM received\n",mypid );
		break;
	case 27:
		printf ( "[%d] SIGPROF received\n",mypid );
		break;
	case 28:
		printf ( "[%d] SIGWINCH received\n",mypid );
		break;
	case 29:
		printf ( "[%d] SIGIO received\n",mypid );
		break;
	case 30:
		printf ( "[%d] SIGPWR received\n",mypid );
		break;
	default:
		printf ( "[%d] signal %d received\n",mypid,s );
	}
#endif
}

// eof - fg_tracker.cxx
//////////////////////////////////////////////////////////////////////
