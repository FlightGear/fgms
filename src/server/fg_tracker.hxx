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
#define FG_TRACKER_HPP

using namespace std;

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include "daemon.hxx"
#include "fg_geometry.hxx"

#define CONNECT    0
#define DISCONNECT 1
#define UPDATE     2

#ifndef IPCPERMS
#define IPCPERMS    0600
#endif

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
	int       InitTracker ( pid_t *pPid );
	int       TcpConnect (char *server_address, int server_port);
	int       TrackerLoop ();
	
	/** 
	 * @brief Return the server of the tracker
	 * @retval string Return tracker server as string 
	 */
	string    GetTrackerServer () { return m_TrackerServer; };
	
	/** 
	 * @brief Return the port no of the tracker 
	 * @retval int Port Number
	 */
	int       GetTrackerPort () { return m_TrackerPort; };
	
	void      Disconnect ();

	
	/** 
	 * @class m_MsgBuffer
	 * @brief Message Buffer Class
	 */
	class  m_MsgBuffer
	{
	public:
		/** 
		 * @brief Message Type
		 */
		long mtype; 
		
		/** 
		 * @brief Message contents 
		 */
		char mtext[1200];
	};
	pthread_t thread;
	//////////////////////////////////////////////////
	//
	//  private methods
	//
	//////////////////////////////////////////////////
	bool Connect ();
	//////////////////////////////////////////////////
	//
	//  private variables
	//  
	//////////////////////////////////////////////////
	int   ipcid;
	int   m_TrackerPort;
	char  m_TrackerServer[128];
	int   m_TrackerSocket;
	bool  m_connected; /*If connected to fgtracker*/
};
#endif

// eof -fg_tracker.hxx
