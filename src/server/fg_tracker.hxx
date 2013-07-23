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
	FG_TRACKER (int port, string server, int id);
	~FG_TRACKER ();

	//////////////////////////////////////////////////
	//
	//  public methods
	//
	//////////////////////////////////////////////////
	int	InitTracker ();
	int	TcpConnect (char *server_address, int server_port);
	int	TrackerLoop ();
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
	void	Disconnect ();
	int	TrackerWrite (const string& str);
	void	ReplyToServer (const string& str);
	void	WriteQueue ();
	void	ReadQueue ();

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
	bool	m_connected; /*If connected to fgtracker*/
	netSocket* m_TrackerSocket;

	typedef std::vector<std::string> vMSG;	/* string vector */
	typedef vMSG::iterator VI;		/* string vector iterator */
	pthread_mutex_t msg_mutex;		/* message queue mutext */
	pthread_cond_t  condition_var;		/* message queue condition */
	vMSG    msg_queue;			/* the single message queue */
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
};
#endif

// eof -fg_tracker.hxx
