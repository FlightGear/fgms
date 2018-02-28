//
// This file is part of fgms, the flightgear multiplayer server
// https://sourceforge.net/projects/fgms/
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
// along with this program; if not see <http://www.gnu.org/licenses/>
//
// Socket read buffer code is copied from Mark Tolonen's answer at
// http://stackoverflow.com/questions/5051701/recv-until-a-nul-byte-is-received
//

/**
 * @file fg_tracker.cxx
 *
 * Send data of online pilots to a tracking server.
 *
 * @author (c) 2006 Julien Pierru
 * @author (c) 2012 Rob Dosogne ( FreeBSD friendly )
 * @author (c) 2015 Hazuki Amamiya
 * @author	Oliver Schroeder <fgms@o-schroeder.de>
 * @date	2006
 * @todo Pete To make a links here to the config and explain a bit
 */

#ifdef HAVE_CONFIG_H
	#include "config.h" // for MSVC, always first
#endif

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <sstream>
#include <string.h>
#include <pthread.h>
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
#include <fglib/fg_util.hxx>
#include <fglib/debug.hxx>
#ifndef _MSC_VER
#include <fglib/daemon.hxx>
#endif
#include <fglib/fg_log.hxx>
#include "fg_tracker.hxx"


namespace
{
	constexpr int RECONNECT_WAIT { 30 };
	// Maximun character in msg from FGTracker: 1023990 (Slightly less than 100KB).
	// Char in [1023991] = '\0'. Note that MAX_MSG_LINE should be 512n-8 bytes.
	// The "reserved" 8 bytes is for 64 bit pointer.
	constexpr int MAX_MSG_LINE { 1023991 };

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
} // anonymous namespace

namespace fgmp
{

//////////////////////////////////////////////////////////////////////
/**
 * @brief Initialize to standard values
 * @param port
 * @param server ip or domain
 * @param fgms name
 */
tracker::tracker
(
	int port,
	std::string server,
	std::string server_name,
	std::string domain,
	std::string version
)
{
	m_version	= version;
	m_tracker_port	= port;
	m_tracker_server= server;
	m_fgms_name	= server_name;
	m_domain	= domain;
	m_proto_version = "20151207";
	m_tracker_socket = 0;
	LOG ( fglog::prio::DEBUG, "# tracker::tracker:"
	  << m_tracker_server << ", Port: " << m_tracker_port
	);
	last_seen	= 0;
	last_sent	= 0;
	bytes_sent	= 0;
	bytes_rcvd	= 0;
	pkts_sent	= 0;
	pkts_rcvd	= 0;
	lost_connections = 0;
	last_connected	= 0;
	want_exit	= false;
	m_identified	= false;
	m_ping_interval	= 55;
	m_timeout_stage	= 0;
	m_connected = false;
} // tracker()

//////////////////////////////////////////////////////////////////////
//Destructor - call tracker::write_queue () and terminate TCP stream
//////////////////////////////////////////////////////////////////////
tracker::~tracker
()
{
	pthread_mutex_unlock ( &msg_mutex ); // give up the lock
	pthread_mutex_unlock ( &msg_sent_mutex ); // give up the lock
	write_queue ();
	msg_queue.clear ();
	msg_sent_queue.clear ();
	msg_recv_queue.clear ();
	if ( m_tracker_socket )
	{
		m_tracker_socket->close ();
		delete m_tracker_socket;
		m_tracker_socket = 0;
	}
} // ~tracker()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
pthread_t
tracker::get_thread_id
()
{
	return m_thread_id;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//tracker::read_queue - Read message in file to cache
/////////////////////////////////////////////////////////////////////
void
tracker::read_queue
()
{
	/*Any message in msg_sent_queue must be pushed back to msg_queue before calling this function*/
	std::ifstream queue_file;
	queue_file.open ( "queue_file" );
	if ( ! queue_file )
	{
		return;
	}
	std::string line_str ( "" );
	std::string Msg ( "" );
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
	requeue_msg ();
	queue_file.close();
	remove ( "queue_file" );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//tracker::write_queue () - Write cached message to a file (only be
//used when destructor of this object be called)
//////////////////////////////////////////////////////////////////////
void
tracker::write_queue
()
{
	VI CurrentMessage;
	std::ofstream queue_file;
	pthread_mutex_lock ( &msg_mutex ); // set the lock
	if ( msg_queue.size() == 0 )
	{
		pthread_mutex_unlock ( &msg_mutex ); // give up the lock
		return;
	}
	queue_file.open ( "queue_file", std::ios::out|std::ios::app );
	if ( ! queue_file )
	{
		LOG ( fglog::prio::HIGH, "# tracker::write_queue: "
		  << "could not open queuefile!" );
		pthread_mutex_unlock ( &msg_mutex ); // give up the lock
		return;
	}
	CurrentMessage = msg_queue.begin(); // get first message
	while ( CurrentMessage != msg_queue.end() )
	{
		queue_file << ( *CurrentMessage ) << std::endl;
		CurrentMessage++;
	}
	pthread_mutex_unlock ( &msg_mutex ); // give up the lock
	queue_file.close ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//tracker::requeue_msg () - Requeue the message in msg_sent_queue
//to msg_queue
//////////////////////////////////////////////////////////////////////
void
tracker::requeue_msg
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
//tracker::add_message - add feedin message to last position of
//cache. fgms.cxx use this to insert messages
/////////////////////////////////////////////////////////////////////
void
tracker::add_message
(
	const std::string& message
)
{
	pthread_mutex_lock ( &msg_mutex ); // acquire the lock
	msg_queue.push_back ( message ); // queue the message at the end of queue
	pthread_cond_signal ( &condition_var );  // wake up the worker
	pthread_mutex_unlock ( &msg_mutex );	 // give up the lock
} // tracker::add_message()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//tracker::buffsock_free - Reset the non-data part of
//socket read buffer
/////////////////////////////////////////////////////////////////////
void
tracker::buffsock_free
(
	buffsock_t* bs
)
{
	memset ( bs->buf+bs->curlen,0x17, MAX_MSG_LINE-bs->curlen );
}

//////////////////////////////////////////////////////////////////////
//tracker::check_timeout - Check connection timeout between FGMS and
//FGTracker by sending PING to FGTracker
/////////////////////////////////////////////////////////////////////
void
tracker::check_timeout()
{
	time_t curtime = time ( 0 );
	double seconds;
	seconds = difftime ( curtime, last_seen );
	if ( ( int ) seconds - m_ping_interval * m_timeout_stage > m_ping_interval )
	{
		m_timeout_stage++;
		if ( m_timeout_stage>3 )
		{
			m_connected = false;
			lost_connections++;
			LOG ( fglog::prio::URGENT,
			  "# tracker::check_timeout: "
			  << "No data received from FGTracker for "
			  << seconds
			  << " seconds. Lost connection to FGTracker"
			);
			return;
		}
		std::string reply = "PING";
		if ( tracker_write ( reply ) < 0 )
		{
			LOG ( fglog::prio::HIGH, "# tracker::check_timeout: "
			  << "Tried to PING FGTracker "
			  << "(No data from FGTracker for "
			  << seconds
			  << " seconds) but failed in sending "
			  << "PING to FGTracker"
			);
		}
		else
		{
			LOG ( fglog::prio::DEBUG, "# tracker::check_timeout: "
			  << "PING FGTracker (No data from FGTracker for "
			  << seconds << " seconds)"
			);
		}
	}
}

//////////////////////////////////////////////////////////////////////
//tracker::tracker_read - Read TCP stream (non blocking) and call
//tracker::reply_from_server
//////////////////////////////////////////////////////////////////////
void
tracker::tracker_read
(
	buffsock_t* bs
)
{
	char	res[MAX_MSG_LINE];	/*Msg from/to server*/
	char* pch;
	errno = 0;
	//int bytes = m_tracker_socket->recv ( res, MAX_MSG_LINE, MSG_NOSIGNAL );
	/* -1 below is used to retain last byte of 0x23*/
	int bytes = m_tracker_socket->recv ( bs->buf + bs->curlen, bs->maxlen - bs->curlen -1, MSG_NOSIGNAL );
	if ( bytes <= 0 )
	{
		/*check if NOT
		EAGAIN (No data on non blocking socket)
		EWOULDBLOCK (non-blocking socket and there is not enough space in the kernel's outgoing-data-buffer)
		EINTR (sth hard to explain - Linux only)*/
		if ( ( errno != EAGAIN ) && ( errno != EWOULDBLOCK ) && ( errno != EINTR ) )
		{
			m_connected = false;
			lost_connections++;
			LOG ( fglog::prio::HIGH, "# tracker::tracker_read: "
			  << "lost connection to FGTracker"
			);
		}
	}
	else
	{
		// something received from tracker server
		bs->curlen += bytes;
		last_seen = time ( 0 );
		m_timeout_stage = 0;
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
			memmove ( bs->buf,bs->buf + pos,MAX_MSG_LINE-pos );
			msg_recv_queue.push_back ( res );
			buffsock_free ( bs );
			pkts_rcvd++;
		}
		bytes_rcvd += bytes;
		LOG ( fglog::prio::DEBUG, "# tracker::tracker_read: "
		  << "received message from FGTracker - " << res );
		reply_from_server ();
	}
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//tracker::tracker_write - Write a feedin string to TCP stream
/////////////////////////////////////////////////////////////////////
int
tracker::tracker_write
(
	const std::string& str
)
{
	size_t l   = str.size()+1;
	last_sent   = time ( 0 );
	errno      = 0;
	size_t s   = -1;

	s = m_tracker_socket->send ( str.c_str(), l ,0 );
	if ( s != l )
	{
		m_connected = false;
		lost_connections++;
		LOG ( fglog::prio::HIGH, "# tracker::tracker_write: "
		  << "lost connection to server. Netsocket returned size ="
		  << s << ", actual size should be =" <<l
		);
		return -1;
	}
	/*put the sent message to msg_sent_queue*/
	size_t pos = str.find ( m_proto_version );
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
	std::stringstream debug;
	debug <<"DEBUG last_msg.size="
		<< l <<", msg_queue.size = "
		<< msg_queue.size()
		<< ", msg_sent_queue.size = "  << msg_sent_queue.size();
	m_tracker_socket->send ( debug.str().c_str(), debug.str().size() + 1, 0 );
	bytes_sent += s;
	pkts_sent++;
	return s;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//tracker::reply_from_server - Process the reply from server
/////////////////////////////////////////////////////////////////////
void
tracker::reply_from_server
()
{
	std::string reply;
	while ( msg_recv_queue.begin() != msg_recv_queue.end() )
	{
		std::string str=msg_recv_queue.front();
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
			if ( tracker_write ( reply ) < 0 )
			{
				LOG ( fglog::prio::HIGH,
				  "# tracker::reply_from_server: "
				  << "PING from FGTracker received "
				  << "but failed to sent PONG to FGTracker"
				);
			}
			else
			{
				LOG ( fglog::prio::DEBUG,
				  "# tracker::reply_from_server: "
				  << "PING from FGTracker received"
				);
			}
		}
		else if ( str == "PONG" )
		{
			LOG ( fglog::prio::DEBUG, "# tracker::reply_from_server: "
			  << "PONG from FGTracker received"
			);
		}
		else if ( pos_err == 0 )
		{
			LOG ( fglog::prio::HIGH, "# tracker::reply_from_server: "
			  << "Received error message from FGTracker. Msg: '"
			  << str <<"'"
			);
		}
		else
		{
			reply = "ERROR Unrecognized message \"" + str + "\"";
			if ( tracker_write ( reply ) < 0 )
			{
				LOG ( fglog::prio::HIGH,
				  "# tracker::reply_from_server: "
				  << "Responce not recognized and failed "
				  << "to notify FGTracker. Msg: '"
				  << str <<"'"
				);
			}
			else
			{
				LOG ( fglog::prio::HIGH,
				  "# tracker::reply_from_server: "
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
//tracker::loop () - The infinite loop running the service
//////////////////////////////////////////////////////////////////////
int
tracker::loop
()
{
	std::string	Msg;
	pthread_mutex_init ( &msg_mutex, 0 );
	pthread_mutex_init ( &msg_sent_mutex, 0 );
	pthread_mutex_init ( &msg_recv_mutex, 0 );
	pthread_cond_init  ( &condition_var, 0 );
	m_thread_id = pthread_self();
	/*Initialize socket read buffer*/
	buffsock_t bs;
	bs.buf = ( char* ) malloc ( MAX_MSG_LINE );
	bs.maxlen = MAX_MSG_LINE;
	bs.curlen = 0;
#ifdef WIN32
	LOG ( fglog::prio::HIGH, "# tracker::loop: "
	  << "started, thread ID " << m_thread_id.p
	);
#else
	LOG ( fglog::prio::HIGH, "# tracker::loop: "
	  << "started, thread ID " << m_thread_id
	);
#endif
	/*Infinite loop*/
	while ( ! want_exit )
	{
		if ( ! is_connected() )
		{
			/*requeue the outstanding message to message queue*/
			requeue_msg ();
			/*Reset Socket Read Buffer*/
			bs.curlen=0;
			buffsock_free ( &bs );
			m_identified=false;
			LOG ( fglog::prio::HIGH, "# tracker::loop: "
			  << "trying to connect to FGTracker"
			);
			m_connected = false;
			if ( ! is_connected() )
			{
				LOG ( fglog::prio::HIGH, "# tracker::loop: "
				  << "not connected, will sleep for "
				  << RECONNECT_WAIT << " seconds"
				);
				struct timeval now;
				struct timespec timeout;
				gettimeofday ( &now, 0 );
				timeout.tv_sec  = now.tv_sec + RECONNECT_WAIT;
				timeout.tv_nsec = now.tv_usec * 1000;
				pthread_mutex_lock ( &msg_mutex );
				pthread_cond_timedwait ( &condition_var, &msg_mutex, &timeout );
				pthread_mutex_unlock ( &msg_mutex );
				continue;
			}
			m_timeout_stage = 0;
			read_queue (); 	// read backlog, if any
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
			LOG ( fglog::prio::DEBUG, "# tracker::loop: "
			  << "sending msg " << Msg.size() << "  bytes: " << Msg
			);
			if ( tracker_write ( Msg ) < 0 )
			{
				pthread_mutex_lock ( &msg_mutex );
				// requeue message at the beginning of queue
				msg_queue.insert ( msg_queue.begin() , Msg );
				// wake up the worker
				pthread_cond_signal ( &condition_var );
				pthread_mutex_unlock ( &msg_mutex );
				break;
			}
			tracker_read ( &bs );
		}
		tracker_read ( &bs ); /*usually read PING*/
		check_timeout();
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
tracker::connect
()
{
	if ( m_tracker_socket )
	{
		m_tracker_socket->close ();
		delete m_tracker_socket;
		m_tracker_socket = 0;
	}
	m_tracker_socket = new fgmp::netsocket();
	LOG ( fglog::prio::DEBUG, "# tracker::connect: "
	  << "Server: " << m_tracker_server << ", Port: " << m_tracker_port
	);
	if ( ! m_tracker_socket->connect ( m_tracker_server, m_tracker_port, fgmp::netsocket::TCP ) )
	{
		LOG ( fglog::prio::HIGH, "# tracker::connect: "
		  << "connect to " << m_tracker_server
		  << ":" << m_tracker_port << " failed!"
		);
		delete m_tracker_socket;
		m_tracker_socket = 0;
		return false;
	}
	m_tracker_socket->set_blocking ( false );
	LOG ( fglog::prio::HIGH, "# tracker::connect: success" );
	last_connected	= time ( 0 );
	std::stringstream ss;
	ss.str("Please initialize");
	m_tracker_socket->send ( ss.str().c_str(), ss.str().size()+1 ,0 );
	sleep ( 2 );
	
	/*Write Version header to FGTracker*/
	ss.str("");
	ss << "V" << m_proto_version << " " << m_version
	   << " "<< m_domain << " " << m_fgms_name;
	tracker_write ( ss.str() );
	LOG ( fglog::prio::DEBUG, "# tracker::connect: "
	  << "Written Version header"
	);
	sleep ( 1 );
	return true;
} // connect ()

} // namespace fgmp

// eof - fg_tracker.cxx
//////////////////////////////////////////////////////////////////////
