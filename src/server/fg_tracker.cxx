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
#ifdef _MSC_VER
#include <sys/timeb.h>
#else
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

#ifndef DEF_CONN_SECS
#define DEF_CONN_SECS 30
#endif

int tracker_conn_secs = DEF_CONN_SECS;

#if defined(_MSC_VER) || defined(__MINGW32__)
/* windows work around for gettimeofday() */
int gettimeofday(struct timeval* tp, void* tzp) 
{
    struct __timeb64 tm;
    _ftime64_s(&tm); // Time since Epoch, midnight (00:00:00), January 1, 1970, UTC.
    tp->tv_sec = tm.time;
    tp->tv_usec = 1000000 * tm.millitm; // milliseconds to nanoseconds
    return 0;
}
#endif /* #if defined(_MSC_VER) || defined(__MINGW32__) */

//////////////////////////////////////////////////////////////////////
/**
 * @brief Initialize to standard values
 * @param port
 * @param server ip or domain
 * @param fgms name
 */
FG_TRACKER::FG_TRACKER ( int port, string server, string m_ServerName, string domain )
{
	m_TrackerPort	= port;
	m_TrackerServer = server;
	m_FgmsName = m_ServerName;
	m_domain = domain;
	m_protocalVersion = "20151207";
	m_TrackerSocket = 0;
	SG_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::FG_TRACKER:"
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
    m_connected = false;
	m_identified= false;
} // FG_TRACKER()

//////////////////////////////////////////////////////////////////////
//Destructor - call FG_TRACKER::WriteQueue () and terminate TCP stream
//////////////////////////////////////////////////////////////////////
FG_TRACKER::~FG_TRACKER ()
{
	pthread_mutex_unlock ( &msg_mutex ); // give up the lock
	pthread_mutex_unlock ( &msg_sent_mutex ); // give up the lock
	WriteQueue ();
	msg_queue.clear ();
	msg_sent_queue.clear ();
	if ( m_TrackerSocket )
	{
		m_TrackerSocket->close ();
		delete m_TrackerSocket;
		m_TrackerSocket = 0;
	}
} // ~FG_TRACKER()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
pthread_t
FG_TRACKER::GetThreadID ()
{
	return MyThreadID;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::ReadQueue - Read message in file to cache
/////////////////////////////////////////////////////////////////////
void
FG_TRACKER::ReadQueue ()
{	
	/*Any message in msg_sent_queue must be pushed back to msg_queue before calling this function*/
	ifstream queue_file;
	queue_file.open ( "queue_file" );
	if ( ! queue_file )
	{
		return;
	}
	string line_str("");
	string Msg ("");
	int line_cnt;
	while ( getline ( queue_file, line_str, '\n' ) )
	{
		if(line_cnt==25)
		{
			pthread_mutex_lock ( &msg_sent_mutex );	 // set the lock
			msg_sent_queue.push_back ( Msg ); // queue the message
			pthread_mutex_unlock ( &msg_sent_mutex );	 // give up the lock
			Msg="";
			line_cnt=0;
		}
		line_str += "\n";
		Msg += line_str;
		line_cnt++;
	}
	/*fire remaining message to msg_queue*/
	pthread_mutex_lock ( &msg_sent_mutex );	 // set the lock
	msg_sent_queue.push_back ( Msg ); // queue the message
	pthread_mutex_unlock ( &msg_sent_mutex );	 // give up the lock
	ReQueueSentMsg (); 
	queue_file.close();
	remove ( "queue_file" );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::WriteQueue () - Write cached message to a file (only be 
//used when destructor of this object be called)
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
		SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::WriteQueue: "
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
	pthread_mutex_unlock ( &msg_mutex ); // give up the lock
	queue_file.close ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::ReQueueSentMsg () - Requeue the message in msg_sent_queue
//to msg_queue
//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::ReQueueSentMsg ()
{
	pthread_mutex_lock ( &msg_mutex );
	pthread_mutex_lock ( &msg_sent_mutex );
	while (msg_sent_queue.begin()!=msg_sent_queue.end())
	{
		msg_queue.insert ( msg_queue.begin() , msg_sent_queue.back() );// requeue message at the beginning of queue
		msg_sent_queue.pop_back();
	}
	pthread_cond_signal ( &condition_var );  // wake up the worker
	pthread_mutex_unlock ( &msg_sent_mutex );
	pthread_mutex_unlock ( &msg_mutex );
}

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::AddMessage - add feedin message to last position of 
//cache. fg_server.cxx use this to insert messages
/////////////////////////////////////////////////////////////////////
void
FG_TRACKER::AddMessage( const string& message )
{
	pthread_mutex_lock ( &msg_mutex ); // acquire the lock
	msg_queue.push_back ( message.c_str() ); // queue the message at the end of queue
	pthread_cond_signal ( &condition_var );  // wake up the worker
	pthread_mutex_unlock ( &msg_mutex );	 // give up the lock
} // FG_TRACKER::AddMessage()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::TrackerRead - Read TCP stream (non blocking) and call 
//FG_TRACKER::ReplyFromServer
//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::TrackerRead ()
{
	char	res[MSGMAXLINE];	/*Msg from/to server*/
	errno = 0;
	int i = m_TrackerSocket->recv ( res, MSGMAXLINE, MSG_NOSIGNAL );
	if ( i <= 0 )
	{
		/*check if NOT
		EAGAIN (No data on non blocking socket)
		EWOULDBLOCK (non-blocking socket and there is not enough space in the kernel's outgoing-data-buffer)
		EINTR (sth hard to explain - Linux only)*/
		if ( ( errno != EAGAIN ) && ( errno != EWOULDBLOCK ) && ( errno != EINTR ) )
		{
			m_connected = false;
			LostConnections++;
			SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerRead: "
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
		SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerRead: "
			            << "received message from server"
			          );
		ReplyFromServer ( res );
	}
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::TrackerWrite - Write a feedin string to TCP stream
/////////////////////////////////////////////////////////////////////
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
			SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerWrite: "
			            << "lost connection to server"
			          );
			return -1;
		}
	}
	/*put the sent message to msg_sent_queue*/
	size_t pos = str.find( m_protocalVersion );
	if ( pos == 1 )
	{} else if( str == "PONG" )
	{} else
	{
		pthread_mutex_lock ( &msg_sent_mutex );
		msg_sent_queue.push_back( str );
		pthread_mutex_unlock ( &msg_sent_mutex );
	}
	BytesSent += s;
	PktsSent++;
	return s;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::ReplyFromServer - Process the reply from server
/////////////////////////////////////////////////////////////////////
void
FG_TRACKER::ReplyFromServer ( const string& str )
{
	string reply;
	
	size_t pos = str.find("IDENTIFIED");
	if (pos==0 and pos!=std::string::npos)
	{
		m_identified=true;
	}else if ( str == "OK" )
	{
		pthread_mutex_lock ( &msg_sent_mutex ); // acquire the lock
		msg_sent_queue.erase ( msg_sent_queue.begin() );
		pthread_mutex_unlock ( &msg_sent_mutex );	 // give up the lock
	}
	else if ( str == "PING" )
	{
		reply = "PONG";
		if ( TrackerWrite ( reply ) < 0 )
		{
			SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::ReplyFromServer: "
		            << "PING from server received but failed to sent PONG to server" );
		}else
		SG_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::ReplyFromServer: "
		            << "PING from server received" );
	} else
	SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::ReplyFromServer: "
	            << "Responce not recognized. Msg: '" << str );
	return;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::Loop () - The infinite loop running the service
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::Loop ()
{
	string	Msg;
	pthread_mutex_init ( &msg_mutex, 0 );
	pthread_mutex_init ( &msg_sent_mutex, 0 );
	pthread_cond_init  ( &condition_var, 0 );
	MyThreadID = pthread_self();
#ifdef WIN32
	SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Loop: "
		    << "started, thread ID " << MyThreadID.p
		    );
#else
	SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Loop: "
		    << "started, thread ID " << MyThreadID
		    );
#endif
	/*Infinite loop*/
	while ( ! WantExit )
	{
		if (! m_connected)
		{
			/*requeue the outstanding message to message queue*/
			ReQueueSentMsg ();
			
			m_identified=false;
			SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Loop: "
			            << "trying to connect"
			          );
			m_connected = Connect();
			if ( ! m_connected )
			{
				SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Loop: "
				            << "not connected, will sleep for " << tracker_conn_secs << " seconds"
				          );
				struct timeval now;
				struct timespec timeout;
				gettimeofday(&now, 0);
				timeout.tv_sec  = now.tv_sec + tracker_conn_secs;
				timeout.tv_nsec = now.tv_usec * 1000;
				pthread_mutex_lock ( &msg_mutex );
				pthread_cond_timedwait ( &condition_var, &msg_mutex, &timeout );
				pthread_mutex_unlock ( &msg_mutex );
				continue;
			}
			ReadQueue (); 	// read backlog, if any
		}
		pthread_mutex_lock ( &msg_mutex );
		pthread_mutex_unlock ( &msg_mutex );
		if ( !msg_queue.size ())
		{
			// wait for data (1 sec at most)
			struct timeval now;
			struct timespec timeout;
			gettimeofday(&now, 0);
			timeout.tv_sec  = now.tv_sec + 1;
			pthread_mutex_lock ( &msg_mutex );	
			pthread_cond_timedwait ( &condition_var, &msg_mutex, &timeout );
			pthread_mutex_unlock ( &msg_mutex );
		}
		while ( msg_queue.size () && m_connected && m_identified)
		{
			/*Get message from msg_queue*/
			pthread_mutex_lock ( &msg_mutex );
			Msg = msg_queue.front();
			msg_queue.erase ( msg_queue.begin() );
			pthread_mutex_unlock ( &msg_mutex );
			
			
#ifdef ADD_TRACKER_LOG
			write_msg_log ( Msg.c_str(), Msg.size(), ( char* ) "OUT: " );
#endif // #ifdef ADD_TRACKER_LOG
			SG_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::Loop: "
			            << "sending msg " << Msg.size() << "  bytes: " << Msg
			          );
			if ( TrackerWrite ( Msg ) < 0 )
			{
				pthread_mutex_lock ( &msg_mutex );
				msg_queue.insert ( msg_queue.begin() , Msg );// requeue message at the beginning of queue
				pthread_cond_signal ( &condition_var );  // wake up the worker
				pthread_mutex_unlock ( &msg_mutex );
				break;
			}
			
			TrackerRead ();
			Msg = "";
			if ( msg_sent_queue.size() > 25 )
			{
				break; /*too much outstanding packets*/
			}		
		}
		TrackerRead (); /*usually read PING*/
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
		m_TrackerSocket = 0;
	}
	m_TrackerSocket = new netSocket();
	SG_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::Connect: "
	            << "Server: " << m_TrackerServer << ", Port: " << m_TrackerPort );
	if ( m_TrackerSocket->open ( true ) == false )
	{
		SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Connect: "
		            << "Can't get socket..."
		          );
		delete m_TrackerSocket;
		m_TrackerSocket = 0;
		return false;
	}
	if ( m_TrackerSocket->connect ( m_TrackerServer.c_str(), m_TrackerPort ) < 0 )
	{
		SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Connect: "
		            << "Connect failed!"
		          );
		delete m_TrackerSocket;
		m_TrackerSocket = 0;
		return false;
	}
	m_TrackerSocket->setBlocking ( false );
	SG_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Connect: "
	            << "success"
	          );
	LastConnected	= time ( 0 );
	m_TrackerSocket->write_char ( '\0' );
	sleep ( 2 );
	/*Write Version header to FGTracker*/
	std::stringstream ss;
	ss << "V" << m_protocalVersion << " " << VERSION << " "<< m_domain << " " << m_FgmsName;
	std::string s = ss.str();
	TrackerWrite ( s );
	SG_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::Connect: "
	            << "Written Version header"
	          );
	sleep ( 1 );
	return true;
} // Connect ()

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
