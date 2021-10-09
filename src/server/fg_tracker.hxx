/**
 * @file fg_tracker.hxx
 * @author (c) 2006 Julien Pierru
 * @author (c) 2015 Hazuki Amamiya
 *
 */

//////////////////////////////////////////////////////////////////////
//
//  server tracker for FlightGear
//  (c) 2006 Julien Pierru
//	(c) 2015 Hazuki Amamiya
//  Licenced under GPL
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

#if !defined FG_TRACKER_HPP
// #define FG_TRACKER_HPP

// #define ADD_TRACKER_LOG

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <plib/netSocket.h>
#include "daemon.hxx"
#include "fg_geometry.hxx"

#define CONNECT    0
#define DISCONNECT 1
#define UPDATE     2

void signal_handler(int s);

//////////////////////////////////////////////////////////////////////
/**
 * @class FG_TRACKER
 * @brief The tracker class
 */
class FG_TRACKER
{
public:
	
	//////////////////////////////////////////////////
	//
	//  private variables
	//  
	//////////////////////////////////////////////////
	int	m_TrackerPort;
	int	m_PingInterval;
	int	m_TimeoutStage;
	std::string	m_TrackerServer;
	std::string	m_FgmsName;
	std::string	m_domain;
	std::string	m_ProtocolVersion;
	bool	m_identified;			/* If fgtracker identified this fgms */
	netSocket* m_TrackerSocket;

	typedef std::vector<std::string> vMSG;	/* string vector */
	typedef vMSG::iterator VI;		/* string vector iterator */
	typedef struct buffsock { /*socket buffer*/
		char* buf;
		size_t maxlen;
		size_t curlen;
	} buffsock_t;
	pthread_mutex_t msg_mutex;		/* message queue mutext */
	pthread_mutex_t msg_sent_mutex;		/* message queue mutext */
	pthread_mutex_t msg_recv_mutex;		/* message queue mutext */
	pthread_cond_t  condition_var;		/* message queue condition */
	vMSG    msg_queue;			/* the single message queue */
	vMSG    msg_sent_queue;			/* the single message queue */
	vMSG    msg_recv_queue;			/* the single message queue */
	bool	WantExit;
	// static, so it can be set from outside (signal handler)
	static bool	m_connected;			/* If connected to fgtracker */
	static inline void set_connected ( bool b ) { m_connected = b; };
	static inline bool is_connected () { return m_connected; };
	//////////////////////////////////////////////////
	//
	//  constructors
	//
	//////////////////////////////////////////////////
	FG_TRACKER (int port, std::string server, std::string m_ServerName, std::string m_domain);
	~FG_TRACKER ();

	//////////////////////////////////////////////////
	//
	//  public methods
	//
	//////////////////////////////////////////////////
	int	Loop ();
	void	AddMessage ( const std::string & message );
	
	/** 
	 * @brief Return the server of the tracker
	 * @retval string Return tracker server as string 
	 */
	std::string	GetTrackerServer () { return m_TrackerServer; };
	
	/** 
	 * @brief Return the port no of the tracker 
	 * @retval int Port Number
	 */
	int	GetTrackerPort () { return m_TrackerPort; };
	pthread_t GetThreadID();
	

	//////////////////////////////////////////////////
	//
	//  private methods
	//
	//////////////////////////////////////////////////
	bool 	Connect ();
	void	WriteQueue ();
	void	ReadQueue ();
	void 	ReQueueSentMsg ();
	void 	buffsock_free(buffsock_t* bs);
	void	CheckTimeout();
	int		TrackerWrite (const std::string& str);
	void 	TrackerRead (buffsock_t* bs);
	void 	ReplyFromServer ();
	
	//////////////////////////////////////////////////
	//	stats
	//////////////////////////////////////////////////
	time_t		LastConnected;
	time_t		LastSeen;
	time_t		LastSent;
	uint64_t	BytesSent;
	uint64_t	BytesRcvd;
	uint64_t	PktsSent;
	uint64_t	PktsRcvd;
	size_t		LostConnections;
	pthread_t 	MyThreadID;
};
#endif

// eof -fg_tracker.hxx
