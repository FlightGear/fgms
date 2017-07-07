/**
 * @file fg_tracker.cxx
 * @author (c) 2006 Julien Pierru
 * @author (c) 2012 Rob Dosogne ( FreeBSD friendly )
 * @author (c) 2015 Hazuki Amamiya
 * @todo Pete To make a links here to the config and explain a bit
 *
 */

//////////////////////////////////////////////////////////////////////
//
//  server tracker for FlightGear
//  (c) 2006 Julien Pierru
//  (c) 2012 Rob Dosogne ( FreeBSD friendly )
//	(c) 2015 Hazuki Amamiya
//  Licenced under GPL
//
//	Socket read buffer code is copied from Mark Tolonen's answer at
//	http://stackoverflow.com/questions/5051701/recv-until-a-nul-byte-is-received
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, US
//////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h" // for MSVC, always first
#endif

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <string.h>
#include <sstream>
#ifdef _MSC_VER
#include <sys/timeb.h>
#include <libmsc/msc_unistd.hxx>
#else
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include "fg_tracker.hxx"
#include <fglib/fg_util.hxx>
#include <fglib/debug.hxx>
#include <fglib/daemon.hxx>
#include <fglib/fg_log.hxx>

#ifndef DEF_CONN_SECS
#define DEF_CONN_SECS 30
#endif

// Maximun character in msg from FGTracker: 1023990 (Slightly less than 100KB).
// Char in [1023991] = '\0'. Note that MSGMAXLINE should be 512n-8 bytes.
// The "reserved" 8 bytes is for 64 bit pointer.
#define MSGMAXLINE	1023991

int tracker_conn_secs = DEF_CONN_SECS;
bool FG_TRACKER::m_connected = false;

#if defined(_MSC_VER) || defined(__MINGW32__)
/* windows work around for gettimeofday() */
int
gettimeofday
(
	struct timeval* tp,
	void* tzp
)
{
	struct __timeb64 tm;
	_ftime64_s ( &tm ); // Time since Epoch, midnight (00:00:00), January 1, 1970, UTC.
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
FG_TRACKER::FG_TRACKER
(
	int port,
	string server,
	string m_ServerName,
	string domain,
	string version
)
{
	m_version		= version;
	m_TrackerPort	= port;
	m_TrackerServer = server;
	m_FgmsName = m_ServerName;
	m_domain = domain;
	m_ProtocolVersion = "20151207";
	m_TrackerSocket = 0;
	LOG ( log::DEBUG, "# FG_TRACKER::FG_TRACKER:"
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
	want_exit	= false;
	m_identified	= false;
	m_PingInterval	= 55;
	m_TimeoutStage	= 0;
	set_connected ( false );
} // FG_TRACKER()

//////////////////////////////////////////////////////////////////////
//Destructor - call FG_TRACKER::WriteQueue () and terminate TCP stream
//////////////////////////////////////////////////////////////////////
FG_TRACKER::~FG_TRACKER
()
{
	pthread_mutex_unlock ( &msg_mutex ); // give up the lock
	pthread_mutex_unlock ( &msg_sent_mutex ); // give up the lock
	WriteQueue ();
	msg_queue.clear ();
	msg_sent_queue.clear ();
	msg_recv_queue.clear ();
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
FG_TRACKER::GetThreadID
()
{
	return MyThreadID;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::ReadQueue - Read message in file to cache
/////////////////////////////////////////////////////////////////////
void
FG_TRACKER::ReadQueue
()
{
	/*Any message in msg_sent_queue must be pushed back to msg_queue before calling this function*/
	ifstream queue_file;
	queue_file.open ( "queue_file" );
	if ( ! queue_file )
	{
		return;
	}
	string line_str ( "" );
	string Msg ( "" );
	int line_cnt = 0;
	while ( getline ( queue_file, line_str, '\n' ) )
	{
		if ( line_cnt==25 )
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
FG_TRACKER::WriteQueue
()
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
		LOG ( log::HIGH, "# FG_TRACKER::WriteQueue: "
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
FG_TRACKER::ReQueueSentMsg
()
{
	pthread_mutex_lock ( &msg_mutex );
	pthread_mutex_lock ( &msg_sent_mutex );
	while ( msg_sent_queue.begin() !=msg_sent_queue.end() )
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
//cache. fgms.cxx use this to insert messages
/////////////////////////////////////////////////////////////////////
void
FG_TRACKER::AddMessage
(
	const string& message
)
{
	pthread_mutex_lock ( &msg_mutex ); // acquire the lock
	msg_queue.push_back ( message ); // queue the message at the end of queue
	pthread_cond_signal ( &condition_var );  // wake up the worker
	pthread_mutex_unlock ( &msg_mutex );	 // give up the lock
} // FG_TRACKER::AddMessage()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::buffsock_free - Reset the non-data part of
//socket read buffer
/////////////////////////////////////////////////////////////////////
void
FG_TRACKER::buffsock_free
(
	buffsock_t* bs
)
{
	memset ( bs->buf+bs->curlen,0x17, MSGMAXLINE-bs->curlen );
}

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::CheckTimeout - Check connection timeout between FGMS and
//FGTracker by sending PING to FGTracker
/////////////////////////////////////////////////////////////////////
void
FG_TRACKER::CheckTimeout()
{
	time_t curtime = time ( 0 );
	double seconds;
	seconds = difftime ( curtime, LastSeen );
	if ( ( int ) seconds - m_PingInterval * m_TimeoutStage > m_PingInterval )
	{
		m_TimeoutStage++;
		if ( m_TimeoutStage>3 )
		{
			set_connected ( false );
			LostConnections++;
			LOG ( log::URGENT,
			  "# FG_TRACKER::CheckTimeout: "
			  << "No data received from FGTracker for "
			  << seconds
			  << " seconds. Lost connection to FGTracker"
			);
			return;
		}
		string reply = "PING";
		if ( TrackerWrite ( reply ) < 0 )
		{
			LOG ( log::HIGH, "# FG_TRACKER::CheckTimeout: "
			  << "Tried to PING FGTracker "
			  << "(No data from FGTracker for "
			  << seconds
			  << " seconds) but failed in sending "
			  << "PING to FGTracker"
			);
		}
		else
		{
			LOG ( log::DEBUG, "# FG_TRACKER::CheckTimeout: "
			  << "PING FGTracker (No data from FGTracker for "
			  << seconds << " seconds)"
			);
		}
	}
}

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::TrackerRead - Read TCP stream (non blocking) and call
//FG_TRACKER::ReplyFromServer
//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::TrackerRead
(
	buffsock_t* bs
)
{
	char	res[MSGMAXLINE];	/*Msg from/to server*/
	char* pch;
	errno = 0;
	//int bytes = m_TrackerSocket->recv ( res, MSGMAXLINE, MSG_NOSIGNAL );
	/* -1 below is used to retain last byte of 0x23*/
	int bytes = m_TrackerSocket->recv ( bs->buf + bs->curlen, bs->maxlen - bs->curlen -1, MSG_NOSIGNAL );
	if ( bytes <= 0 )
	{
		/*check if NOT
		EAGAIN (No data on non blocking socket)
		EWOULDBLOCK (non-blocking socket and there is not enough space in the kernel's outgoing-data-buffer)
		EINTR (sth hard to explain - Linux only)*/
		if ( ( errno != EAGAIN ) && ( errno != EWOULDBLOCK ) && ( errno != EINTR ) )
		{
			set_connected ( false );
			LostConnections++;
			LOG ( log::HIGH, "# FG_TRACKER::TrackerRead: "
			  << "lost connection to FGTracker"
			);
		}
	}
	else
	{
		// something received from tracker server
		bs->curlen += bytes;
		LastSeen = time ( 0 );
		m_TimeoutStage = 0;
		while ( 1 )
		{
			if ( bs->buf[0] == 0x17 )
			{
				/*nothing to be read. Reset buffer*/
				bs->curlen=0;
				buffsock_free ( bs );
				break;
			}
			pch = strchr ( bs->buf,'\0' );
			if ( pch == NULL )
			{
				/* Not full packet received. break.*/
				break;
			}
			int pos=pch-bs->buf+1; /* position of '\0', starting with 1 */
			memcpy ( res,bs->buf,pos );
			bs->curlen -= pos;
			/*move array element one packet forward*/
			memmove ( bs->buf,bs->buf + pos,MSGMAXLINE-pos );
			msg_recv_queue.push_back ( res );
			buffsock_free ( bs );
			PktsRcvd++;
		}
		BytesRcvd += bytes;
		LOG ( log::DEBUG, "# FG_TRACKER::TrackerRead: "
		  << "received message from FGTracker - " << res );
		ReplyFromServer ();
	}
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::TrackerWrite - Write a feedin string to TCP stream
/////////////////////////////////////////////////////////////////////
int
FG_TRACKER::TrackerWrite
(
	const string& str
)
{
	size_t l   = str.size()+1;
	LastSent   = time ( 0 );
	errno      = 0;
	size_t s   = -1;

	s = m_TrackerSocket->send ( str.c_str(), l ,0 );
	if ( s != l )
	{
		set_connected ( false );
		LostConnections++;
		LOG ( log::HIGH, "# FG_TRACKER::TrackerWrite: "
		  << "lost connection to server. Netsocket returned size ="
		  << s << ", actual size should be =" <<l
		);
		return -1;
	}
	/*put the sent message to msg_sent_queue*/
	size_t pos = str.find ( m_ProtocolVersion );
	size_t pos2 = str.find ( "ERROR" );
	if ( pos == 1 )
		{}
	else if ( pos2 == 0 )
		{}
	else if ( str == "PING" )
		{}
	else if ( str == "PONG" )
		{}
	else
	{
		pthread_mutex_lock ( &msg_sent_mutex );
		msg_sent_queue.push_back ( str );
		pthread_mutex_unlock ( &msg_sent_mutex );
	}
	stringstream debug;
	debug <<"DEBUG last_msg.size="
		<< l <<", msg_queue.size = "
		<< msg_queue.size()
		<< ", msg_sent_queue.size = "  << msg_sent_queue.size();
	m_TrackerSocket->send ( debug.str().c_str(), debug.str().size() + 1, 0 );
	BytesSent += s;
	PktsSent++;
	return s;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::ReplyFromServer - Process the reply from server
/////////////////////////////////////////////////////////////////////
void
FG_TRACKER::ReplyFromServer
()
{
	string reply;
	while ( msg_recv_queue.begin() != msg_recv_queue.end() )
	{
		string str=msg_recv_queue.front();
		msg_recv_queue.erase ( msg_recv_queue.begin() );
		size_t pos_ident = str.find ( "IDENTIFIED" );
		size_t pos_err = str.find ( "ERROR" );
		if ( pos_ident == 0 )
		{
			m_identified=true;
		}
		else if ( str == "OK" )
		{
			pthread_mutex_lock ( &msg_sent_mutex ); // acquire the lock
			if ( msg_sent_queue.begin() != msg_sent_queue.end() )
			{
				msg_sent_queue.erase ( msg_sent_queue.begin() );
			}
			pthread_mutex_unlock ( &msg_sent_mutex );	 // give up the lock
		}
		else if ( str == "PING" )
		{
			reply = "PONG";
			if ( TrackerWrite ( reply ) < 0 )
			{
				LOG ( log::HIGH,
				  "# FG_TRACKER::ReplyFromServer: "
				  << "PING from FGTracker received "
				  << "but failed to sent PONG to FGTracker"
				);
			}
			else
			{
				LOG ( log::DEBUG,
				  "# FG_TRACKER::ReplyFromServer: "
				  << "PING from FGTracker received"
				);
			}
		}
		else if ( str == "PONG" )
		{
			LOG ( log::DEBUG, "# FG_TRACKER::ReplyFromServer: "
			  << "PONG from FGTracker received"
			);
		}
		else if ( pos_err == 0 )
		{
			LOG ( log::HIGH, "# FG_TRACKER::ReplyFromServer: "
			  << "Received error message from FGTracker. Msg: '"
			  << str <<"'"
			);
		}
		else
		{
			reply = "ERROR Unrecognized message \"" + str + "\"";
			if ( TrackerWrite ( reply ) < 0 )
			{
				LOG ( log::HIGH,
				  "# FG_TRACKER::ReplyFromServer: "
				  << "Responce not recognized and failed "
				  << "to notify FGTracker. Msg: '"
				  << str <<"'"
				);
			}
			else
			{
				LOG ( log::HIGH,
				  "# FG_TRACKER::ReplyFromServer: "
				  << "Responce from FGTracker not "
				  << "recognized. Msg: '" << str << "'"
				);
			}
		}
	}
	return;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//FG_TRACKER::loop () - The infinite loop running the service
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::loop
()
{
	string	Msg;
	pthread_mutex_init ( &msg_mutex, 0 );
	pthread_mutex_init ( &msg_sent_mutex, 0 );
	pthread_mutex_init ( &msg_recv_mutex, 0 );
	pthread_cond_init  ( &condition_var, 0 );
	MyThreadID = pthread_self();
	/*Initialize socket read buffer*/
	buffsock_t bs;
	bs.buf = ( char* ) malloc ( MSGMAXLINE );
	bs.maxlen = MSGMAXLINE;
	bs.curlen = 0;
#ifdef WIN32
	LOG ( log::HIGH, "# FG_TRACKER::loop: "
	  << "started, thread ID " << MyThreadID.p
	);
#else
	LOG ( log::HIGH, "# FG_TRACKER::loop: "
	  << "started, thread ID " << MyThreadID
	);
#endif
	/*Infinite loop*/
	while ( ! want_exit )
	{
		if ( ! is_connected() )
		{
			/*requeue the outstanding message to message queue*/
			ReQueueSentMsg ();
			/*Reset Socket Read Buffer*/
			bs.curlen=0;
			buffsock_free ( &bs );
			m_identified=false;
			LOG ( log::HIGH, "# FG_TRACKER::loop: "
			  << "trying to connect to FGTracker"
			);
			set_connected ( Connect() );
			if ( ! is_connected() )
			{
				LOG ( log::HIGH, "# FG_TRACKER::loop: "
				  << "not connected, will sleep for "
				  << tracker_conn_secs << " seconds"
				);
				struct timeval now;
				struct timespec timeout;
				gettimeofday ( &now, 0 );
				timeout.tv_sec  = now.tv_sec + tracker_conn_secs;
				timeout.tv_nsec = now.tv_usec * 1000;
				pthread_mutex_lock ( &msg_mutex );
				pthread_cond_timedwait ( &condition_var, &msg_mutex, &timeout );
				pthread_mutex_unlock ( &msg_mutex );
				continue;
			}
			m_TimeoutStage = 0;
			ReadQueue (); 	// read backlog, if any
		}
		if ( !msg_queue.size () )
		{
			// wait for data (1 sec at most)
			struct timeval now;
			struct timespec timeout;
			gettimeofday ( &now, 0 );
			timeout.tv_sec  = now.tv_sec + 1;
			timeout.tv_nsec = now.tv_usec * 1000;
			pthread_mutex_lock ( &msg_mutex );
			pthread_cond_timedwait ( &condition_var,
			  &msg_mutex, &timeout );
			pthread_mutex_unlock ( &msg_mutex );
		}
		else
		{
			usleep ( 10000 );
		}
		while ( msg_queue.size ()
			&& is_connected()
			&& m_identified
			&& msg_sent_queue.size() < 25 )
		{
			/*Get message from msg_queue*/
			Msg = "";
			pthread_mutex_lock ( &msg_mutex );
			Msg = msg_queue.front();
			msg_queue.erase ( msg_queue.begin() );
			pthread_mutex_unlock ( &msg_mutex );
#ifdef ADD_TRACKER_LOG
			write_msg_log ( Msg.c_str(), Msg.size(),
			  ( char* ) "OUT: "
			);
#endif // #ifdef ADD_TRACKER_LOG
			LOG ( log::DEBUG, "# FG_TRACKER::loop: "
			  << "sending msg " << Msg.size() << "  bytes: " << Msg
			);
			if ( TrackerWrite ( Msg ) < 0 )
			{
				pthread_mutex_lock ( &msg_mutex );
				// requeue message at the beginning of queue
				msg_queue.insert ( msg_queue.begin() , Msg );
				// wake up the worker
				pthread_cond_signal ( &condition_var );
				pthread_mutex_unlock ( &msg_mutex );
				break;
			}
			TrackerRead ( &bs );
		}
		TrackerRead ( &bs ); /*usually read PING*/
		CheckTimeout();
	}
	return ( 0 );
} // loop ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  (Re)connect the tracker to its server
//	RETURN: true = success, false = failed
//
//////////////////////////////////////////////////////////////////////
bool
FG_TRACKER::Connect
()
{
	if ( m_TrackerSocket )
	{
		m_TrackerSocket->close ();
		delete m_TrackerSocket;
		m_TrackerSocket = 0;
	}
	m_TrackerSocket = new fgmp::netsocket();
	LOG ( log::DEBUG, "# FG_TRACKER::Connect: "
	  << "Server: " << m_TrackerServer << ", Port: " << m_TrackerPort
	);
	if ( ! m_TrackerSocket->connect ( m_TrackerServer, m_TrackerPort, fgmp::netsocket::TCP ) )
	{
		LOG ( log::HIGH, "# FG_TRACKER::Connect: "
		  << "Connect to " << m_TrackerServer
		  << ":" << m_TrackerPort << " failed!"
		);
		delete m_TrackerSocket;
		m_TrackerSocket = 0;
		return false;
	}
	m_TrackerSocket->set_blocking ( false );
	LOG ( log::HIGH, "# FG_TRACKER::Connect: success" );
	LastConnected	= time ( 0 );
	std::stringstream ss;
	ss.str("Please initialize");
	m_TrackerSocket->send ( ss.str().c_str(), ss.str().size()+1 ,0 );
	sleep ( 2 );
	
	/*Write Version header to FGTracker*/
	ss.str("");
	ss << "V" << m_ProtocolVersion << " " << m_version
	   << " "<< m_domain << " " << m_FgmsName;
	TrackerWrite ( ss.str() );
	LOG ( log::DEBUG, "# FG_TRACKER::Connect: "
	  << "Written Version header"
	);
	sleep ( 1 );
	return true;
} // Connect ()

// eof - fg_tracker.cxx
//////////////////////////////////////////////////////////////////////
