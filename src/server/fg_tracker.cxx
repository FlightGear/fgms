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
#endif
#include <unistd.h>
#include <stdio.h>
#include "fg_common.hxx"
#include "fg_tracker.hxx"
#include "fg_util.hxx"
#include <simgear/debug/logstream.hxx>
#include "daemon.hxx"
#include <libcli/debug.hxx>

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
	WantExit	= false;
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
	if ( m_TrackerSocket )
	{
		m_TrackerSocket->close ();
		delete m_TrackerSocket;
	}
} // ~FG_TRACKER()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::WriteQueue ()
{
	VI CurrentMessage;
	ofstream queue_file;
	pthread_mutex_lock ( &msg_mutex ); // set the lock
	if ( msg_queue.size() == 0 )
	{
		pthread_mutex_unlock ( &msg_mutex ); // give up the lock
		return;
	}
	queue_file.open ( "queue_file", ios::out|ios::app );
	if ( ! queue_file )
	{
		TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::WriteQueue: "
		            << "could not open queuefile!" );
		pthread_mutex_unlock ( &msg_mutex ); // give up the lock
		return;
	}
	CurrentMessage = msg_queue.begin(); // get first message
	while ( CurrentMessage != msg_queue.end() )
	{
		queue_file << ( *CurrentMessage ) << endl;
		CurrentMessage++;
	}
	pthread_mutex_unlock ( &msg_mutex ); // set the lock
	queue_file.close ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::AddMessage
(
        const string& message
)
{
	pthread_mutex_lock ( &msg_mutex ); // acquire the lock
#if 0
	if ( msg_queue.size () > 512 )
	{
		TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER queue full, writeing backlog..." );
		pthread_mutex_unlock ( &msg_mutex ); // give up the lock
		WriteQueue ();
		msg_queue.clear();
	}
#endif
	msg_queue.push_back ( message.c_str() ); // queue the message
	pthread_cond_signal ( &condition_var );  // wake up the worker
	pthread_mutex_unlock ( &msg_mutex );	 // give up the lock
} // FG_TRACKER::AddMessage()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::TrackerWrite ( const string& str )
{
	size_t l   = str.size() + 1;
	LastSent   = time ( 0 );
	errno      = 0;
	int s   = -1;
	while ( s < 0 )
	{
		s = m_TrackerSocket->send ( str.c_str(), l, MSG_NOSIGNAL );
		if ( s < 0 )
		{
			if ( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) )
			{
				continue;
			}
			m_connected = false;
			LostConnections++;
			TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::ReplyToServer: "
			            << "lost connection to server"
			          );
			return -1;
		}
	}
	BytesSent += s;
	PktsSent++;
	return s;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::ReplyToServer ( const string& str )
{
	string reply;
	if ( str == "OK" )
	{
		// set timeout time to 0
		return;
	}
	else if ( str == "PING" )
	{
		reply = "PONG STATUS OK";
		if ( TrackerWrite ( reply ) < 0 )
		{
			return;
		}
		TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::ReplyToServer: "
		            << "PING from server received"
		          );
		return;
	}
	TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::ReplyToServer: "
	            << "Responce not recognized. Msg: '" << str
	          );
}
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
	queue_file.open ( "queue_file" );
	if ( ! queue_file )
	{
		return;
	}
	string line;
	int    line_number = 0;
	while ( getline ( queue_file, line, '\n' ) )
	{
		line_number++;
		pthread_mutex_lock ( &msg_mutex );	 // set the lock
		msg_queue.push_back ( line ); // queue the message
		pthread_mutex_unlock ( &msg_mutex );	 // give up the lock
#if 0
		if ( TrackerWrite ( line ) < 0 )
		{
			TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::FG_TRACKER: "
			            << "lost connection while sending queue after " << line_number
			            << " entries"
			          );
			m_connected = false;
			queue_file.close();
			return;
		}
		TrackerRead ();
#endif
	}
	queue_file.close();
	remove ( "queue_file" );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::TrackerRead ()
{
	char	res[MSGMAXLINE];	/*Msg from/to server*/
	errno = 0;
	int i = m_TrackerSocket->recv ( res, MSGMAXLINE, MSG_NOSIGNAL );
	if ( i <= 0 )
	{
		// error
		if ( ( errno != EAGAIN ) && ( errno != EWOULDBLOCK ) && ( errno != EINTR ) )
		{
			m_connected = false;
			LostConnections++;
			TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::ReplyToServer: "
			            << "lost connection to server"
			          );
		}
	}
	else
	{
		// something received from tracker server
		res[i]='\0';
		LastSeen = time ( 0 );
		PktsRcvd++;
		BytesRcvd += i;
		ReplyToServer ( res );
	}
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
* @brief Send the messages to the tracker server
*/
int
FG_TRACKER::Loop ()
{
	VI	CurrentMessage;
	size_t	length;
	string	Msg;
	int	MsgCounter;
	pthread_mutex_init ( &msg_mutex, 0 );
	pthread_cond_init  ( &condition_var, 0 );
	length = 0;
	MsgCounter = 0;
	/*Infinite loop*/
	while ( ! WantExit )
	{
		while ( ! m_connected )
		{
			TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Loop: "
			            << "trying to connect"
			          );
			m_connected = Connect();
			if ( ! m_connected )
			{
				TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Loop: "
				            << "not connected, will slepp for 30 seconds"
				          );
				sleep ( 30 );
				continue;
			}
			ReadQueue (); 	// read backlog, if any
		}
		while ( length == 0 )
		{
			pthread_mutex_lock ( &msg_mutex );
			length = msg_queue.size ();
			pthread_mutex_unlock ( &msg_mutex );
			if ( length == 0 )
			{
				// wait for data
				pthread_mutex_lock ( &msg_mutex );
				pthread_cond_wait ( &condition_var, &msg_mutex );
				length = msg_queue.size ();
				pthread_mutex_unlock ( &msg_mutex );
			}
		}
		while ( length )
		{
			pthread_mutex_lock ( &msg_mutex );
			CurrentMessage = msg_queue.begin(); // get first message
			Msg = ( *CurrentMessage ).c_str();
			CurrentMessage = msg_queue.erase ( CurrentMessage );
			length = msg_queue.size ();
			pthread_mutex_unlock ( &msg_mutex );
#ifdef ADD_TRACKER_LOG
			write_msg_log ( Msg.c_str(), Msg.size(), ( char* ) "OUT: " );
#endif // #ifdef ADD_TRACKER_LOG
			TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::Loop: "
			            << "sending msg " << Msg.size() << "  bytes: " << Msg
			          );
			if ( TrackerWrite ( Msg ) < 0 )
			{
				AddMessage ( Msg );	// requeue message
				length = 0;
				break;
			}
			MsgCounter++;
			if ( MsgCounter == 25 )
			{
				sleep ( 1 );	// give tracker server some time to write data
				MsgCounter = 0;
			}
			Msg = "";
			TrackerRead ();
		}
	}
	return ( 0 );
} // Loop ()
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
	            << "Server: " << m_TrackerServer << ", Port: " << m_TrackerPort );
	if ( m_TrackerSocket->open ( true ) == false )
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
	LastConnected	= time ( 0 );
	m_TrackerSocket->write_char ( '\0' );
	sleep ( 2 );
	TrackerWrite ( "NOWAIT" );
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
	switch ( s )
	{
	case  1:
		printf ( "SIGHUP received, exiting...\n" );
		exit ( 0 );
		break;
	case  2:
		printf ( "SIGINT received, exiting...\n" );
		exit ( 0 );
		break;
	case  3:
		printf ( "SIGQUIT received, exiting...\n" );
		break;
	case  4:
		printf ( "SIGILL received\n" );
		break;
	case  5:
		printf ( "SIGTRAP received\n" );
		break;
	case  6:
		printf ( "SIGABRT received\n" );
		break;
	case  7:
		printf ( "SIGBUS received\n" );
		break;
	case  8:
		printf ( "SIGFPE received\n" );
		break;
	case  9:
		printf ( "SIGKILL received\n" );
		exit ( 0 );
		break;
	case 10:
		printf ( "SIGUSR1 received\n" );
		break;
	case 11:
		printf ( "SIGSEGV received. Exiting...\n" );
		exit ( 1 );
		break;
	case 12:
		printf ( "SIGUSR2 received\n" );
		break;
	case 13:
		printf ( "SIGPIPE received. Connection Error.\n" );
		break;
	case 14:
		printf ( "SIGALRM received\n" );
		break;
	case 15:
		printf ( "SIGTERM received\n" );
		exit ( 0 );
		break;
	case 16:
		printf ( "SIGSTKFLT received\n" );
		break;
	case 17:
		printf ( "SIGCHLD received\n" );
		break;
	case 18:
		printf ( "SIGCONT received\n" );
		break;
	case 19:
		printf ( "SIGSTOP received\n" );
		break;
	case 20:
		printf ( "SIGTSTP received\n" );
		break;
	case 21:
		printf ( "SIGTTIN received\n" );
		break;
	case 22:
		printf ( "SIGTTOU received\n" );
		break;
	case 23:
		printf ( "SIGURG received\n" );
		break;
	case 24:
		printf ( "SIGXCPU received\n" );
		break;
	case 25:
		printf ( "SIGXFSZ received\n" );
		break;
	case 26:
		printf ( "SIGVTALRM received\n" );
		break;
	case 27:
		printf ( "SIGPROF received\n" );
		break;
	case 28:
		printf ( "SIGWINCH received\n" );
		break;
	case 29:
		printf ( "SIGIO received\n" );
		break;
	case 30:
		printf ( "SIGPWR received\n" );
		break;
	default:
		printf ( "signal %d received\n",s );
	}
#endif
}

// eof - fg_tracker.cxx
//////////////////////////////////////////////////////////////////////
