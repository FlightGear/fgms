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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, U$
//

// Copyright (C) 2005-2010  Oliver Schroeder
//

//////////////////////////////////////////////////////////////////////
//
//  server for FlightGear
//
//////////////////////////////////////////////////////////////////////

#if !defined FG_SERVER_HXX
#define FG_SERVER_HXX

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#ifndef _MSC_VER
        #include <termios.h>
#endif
#include <plib/netSocket.h>
#include <flightgear/MultiPlayer/mpmessages.hxx>
#include <flightgear/MultiPlayer/tiny_xdr.hxx>
#include <simgear/debug/logstream.hxx>
#include <libcli/libcli.hxx>
#include "daemon.hxx"
#include "fg_geometry.hxx"
#include "fg_list.hxx"
#include "fg_tracker.hxx"

using namespace std;

//////////////////////////////////////////////////////////////////////
/**
 * @class FG_SERVER
 * @brief The Main fgms Class
 */
class FG_SERVER
{
public:

	friend class FG_CLI;

	/** @brief Internal Constants */
	enum FG_SERVER_CONSTANTS
	{
		// return values
		SUCCESS                 = 0,
		ERROR_COMMANDLINE       = 1,
		ERROR_CREATE_SOCKET     = 2,
		ERROR_COULDNT_BIND      = 3,
		ERROR_NOT_LISTENING     = 4,
		ERROR_COULDNT_LISTEN    = 5,

		// other constants
		MAX_PACKET_SIZE         = 1200, // to agree with FG multiplayermgr.cxx (since before  2008)
		UPDATE_INACTIVE_PERIOD  = 1,
		MAX_TELNETS             = 5,
		RELAY_MAGIC             = 0x53464746    // GSGF
	};
	//////////////////////////////////////////////////
	//
	//  constructors
	//
	//////////////////////////////////////////////////
	FG_SERVER ();
	~FG_SERVER ();

	//////////////////////////////////////////////////
	//
	//  methods
	//
	//////////////////////////////////////////////////
	int   Init ();
	int   Loop ();
	void  Done ();

	void  PrepareInit ();
	void  SetDataPort ( int Port );
	void  SetTelnetPort ( int Port );
	void  SetAdminPort ( int Port );
	void  SetAdminUser ( string User );
	void  SetAdminPass ( string Pass );
	void  SetAdminEnable ( string Enable );
	void  SetPlayerExpires ( int Seconds );
	void  SetOutOfReach ( int OutOfReach );
	void  SetHub ( bool IamHUB );
	void  SetLog ( int Facility, int Priority );
	void  SetLogfile ( const std::string& LogfileName );
	void  SetServerName ( const std::string& ServerName );
	void  SetBindAddress ( const std::string& BindAddress );
	void  AddRelay ( const string& Server, int Port );
	void  AddCrossfeed ( const string& Server, int Port );
	int   AddTracker ( const string& Server, int Port, bool IsTracked );
	void  AddBlacklist  ( const string& DottedIP, const string& Reason, time_t Timeout = 10 );
	void  CloseTracker ();
	int   check_keyboard();
	void  Show_Stats ( void );
	void* HandleTelnet  ( int Fd );
	void* HandleAdmin  ( int Fd );
	pid_t m_TrackerPID; // tracker child pid

protected:

	//////////////////////////////////////////////////
	//
	//  private variables
	//
	//////////////////////////////////////////////////
	typedef std::map<uint32_t,string>		mT_IP2Relay;
	typedef std::map<uint32_t,string>::iterator	mT_RelayMapIt;
	bool		m_Initialized;
	bool		m_ReinitData;
	bool		m_ReinitTelnet;
	bool		m_ReinitAdmin;
	bool		m_Listening;
	int		m_ListenPort;
	int		m_TelnetPort;
	int		m_AdminPort;
	int		m_TrackingPort;
	int		m_PlayerExpires;
	int		m_PlayerIsOutOfReach;
	string		m_AdminUser;
	string		m_AdminPass;
	string		m_AdminEnable;
	ofstream	m_LogFile;
	string		m_LogFileName;
	string		m_BindAddress;
	size_t		m_NumMaxClients;
	int16_t		m_ProtoMinorVersion;
	int16_t		m_ProtoMajorVersion;
	bool		m_IsParent;
	bool		m_IsTracked;
	string		m_ServerName;
	string		m_TrackerServer;
	netSocket*	m_DataSocket;
	netSocket*	m_TelnetSocket;
	netSocket*	m_AdminSocket;
	mT_IP2Relay	m_RelayMap;
	FG_List		m_CrossfeedList;
	FG_List		m_BlackList;
	FG_List		m_RelayList;
	PlayerList	m_PlayerList;
	int		m_ipcid;
	int		m_childpid;
	FG_TRACKER*	m_Tracker;
	bool		m_IamHUB;
	time_t		m_UpdateSecs;
	bool		m_WantExit;
	//////////////////////////////////////////////////
	//
	//  statistics
	//
	//////////////////////////////////////////////////
	size_t		m_PacketsReceived;	// rw data packet received
	size_t		m_BlackRejected;	// in black list
	size_t		m_PacketsInvalid;	// invalid packet
	size_t		m_UnknownRelay;		// unknown relay
	size_t		m_RelayMagic;		// known relay packet
	size_t		m_PositionData;		// position data packet
	size_t		m_NotPosData;		// packet NOT pos data
	size_t		m_TelnetReceived;
	size_t		m_AdminReceived;
	size_t		mT_PacketsReceived, mT_BlackRejected, mT_PacketsInvalid;
	size_t		mT_UnknownRelay, mT_PositionData, mT_TelnetReceived;
	size_t		mT_RelayMagic, mT_NotPosData;
	size_t		m_CrossFeedFailed, m_CrossFeedSent;
	size_t		mT_CrossFeedFailed, mT_CrossFeedSent;
	size_t		m_TrackerConnect, m_TrackerDisconnect,m_TrackerPostion;
	time_t		m_Uptime;
#ifndef _MSC_VER
	struct termios	OldModes;
#endif

	//////////////////////////////////////////////////
	//
	//  private methods
	//
	//////////////////////////////////////////////////
	void  AddClient     ( const netAddress& Sender, char* Msg );
	void  AddBadClient  ( const netAddress& Sender, string& ErrorMsg,
	                      bool IsLocal );
	bool  IsKnownRelay ( const netAddress& SenderAddress, size_t Bytes );
	bool  PacketIsValid ( int Bytes, T_MsgHdr* MsgHdr,
	                      const netAddress& SenderAddress );
	void  HandlePacket  ( char* sMsg, int Bytes,
	                      const netAddress& SenderAdress );
	int   UpdateTracker ( string callsign, string passwd, string modelname,
	                      time_t time, int type );
	bool  IsInRange ( FG_ListElement& Relay,  FG_Player& SendingPlayer );
	void  SendToCrossfeed ( char* Msg, int Bytes, const netAddress& SenderAddress );
	void  SendToRelays ( char* Msg, int Bytes, FG_Player& SendingPlayer );
	void  WantExit ();
}; // FG_SERVER

typedef struct st_telnet
{
	FG_SERVER* Instance;
	int        Fd;
} st_telnet;
#endif

