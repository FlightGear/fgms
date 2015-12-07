/**
 * @file fg_tracker.hxx
 * @author (c) 2006 Julien Pierru
 *
 */

//////////////////////////////////////////////////////////////////////
//
//  server tracker for FlightGear
//  (c) 2006 Julien Pierru
//
//  Licenced under GPL
//
//////////////////////////////////////////////////////////////////////

#if !defined FG_TRACKER_HPP
// #define FG_TRACKER_HPP

using namespace std;

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
	//  constructors
	//
	//////////////////////////////////////////////////
	FG_TRACKER (int port, string server, string m_ServerName);
	~FG_TRACKER ();

	//////////////////////////////////////////////////
	//
	//  public methods
	//
	//////////////////////////////////////////////////
	int	Loop ();
	void	AddMessage ( const string & message );
	
	/** 
	 * @brief Return the server of the tracker
	 * @retval string Return tracker server as string 
	 */
	string	GetTrackerServer () { return m_TrackerServer; };
	
	/** 
	 * @brief Return the port no of the tracker 
	 * @retval int Port Number
	 */
	int	GetTrackerPort () { return m_TrackerPort; };
	int	TrackerWrite (const string& str);
	void	TrackerRead ();
	void	ReplyToServer (const string& str);
	void	WriteQueue ();
	void	ReadQueue ();
	pthread_t GetThreadID();

	//////////////////////////////////////////////////
	//
	//  private methods
	//
	//////////////////////////////////////////////////
	bool	Connect ();
	//////////////////////////////////////////////////
	//
	//  private variables
	//  
	//////////////////////////////////////////////////
	int	m_TrackerPort;
	string	m_TrackerServer;
	string	m_FgmsName;
	bool	m_connected; /*If connected to fgtracker*/
	netSocket* m_TrackerSocket;

	typedef std::vector<std::string> vMSG;	/* string vector */
	typedef vMSG::iterator VI;		/* string vector iterator */
	pthread_mutex_t msg_mutex;		/* message queue mutext */
	pthread_cond_t  condition_var;		/* message queue condition */
	vMSG    msg_queue;			/* the single message queue */
	bool	WantExit;
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
