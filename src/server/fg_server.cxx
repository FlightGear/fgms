/**
 * @file fg_server.cxx
 */
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
//

#define FGMS_USE_THREADS

//////////////////////////////////////////////////////////////////////
//
//      Server for FlightGear
//      (c) 2005-2012 Oliver Schroeder
//      and contributors (see AUTHORS)
//
//////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#ifndef _MSC_VER
	#ifndef __FreeBSD__
		#include <endian.h>
	#endif
	#include <sys/ipc.h>
	#include <sys/msg.h>
	#include <netinet/in.h>
#endif
#include <string>

#include "fg_cli.hxx"
#include "fg_server.hxx"    /* includes pthread.h */
#include "fg_common.hxx"
#include "fg_util.hxx"
#include <simgear/math/SGEuler.hxx>

#ifdef _MSC_VER
	#include <conio.h> // for _kbhit(), _getch
	typedef int pid_t;
#else
	cDaemon Myself;
#endif

bool    RunAsDaemon = false;
bool    AddCLI = true;

#ifndef DEF_SERVER_LOG
	#define DEF_SERVER_LOG "fg_server.log"
#endif

#ifndef DEF_UPDATE_SECS
	#define DEF_UPDATE_SECS 10
#endif

extern void SigHUPHandler ( int SigType );
#ifndef DEF_EXIT_FILE
	#define DEF_EXIT_FILE "fgms_exit"
#endif
#ifndef DEF_RESET_FILE
	#define DEF_RESET_FILE "fgms_reset"
#endif
#ifndef DEF_STAT_FILE
	#define DEF_STAT_FILE "fgms_stat"
#endif

#ifdef ADD_TRACKER_LOG
	#ifndef DEF_MESSAGE_LOG
		#define DEF_MESSAGE_LOG "fg_message.log"
	#endif
	static char* msg_log = ( char* ) DEF_MESSAGE_LOG;
	static FILE* msg_file = NULL;
#endif // #ifdef ADD_TRACKER_LOG

#if _MSC_VER
	static char* exit_file   = ( char* ) DEF_EXIT_FILE; // "fgms_exit"
	static char* reset_file  = ( char* ) DEF_RESET_FILE; // "fgms_reset"
	static char* stat_file   = ( char* ) DEF_STAT_FILE; // "fgms_stat"
#else // !_MSC_VER
	static char* exit_file   = ( char* ) "/tmp/" DEF_EXIT_FILE;
	static char* reset_file  = ( char* ) "/tmp/" DEF_RESET_FILE;
	static char* stat_file   = ( char* ) "/tmp/" DEF_STAT_FILE;
#endif // _MSC_VER y/n

#ifdef ADD_TRACKER_LOG

// FIXME: use SG_LOG !

static void write_time_string ( FILE* file )
{
	time_t Timestamp = time ( 0 );
	char TimeStr[100];
	tm*  tm;
	// Creates the UTC time string
	tm = gmtime ( & Timestamp );
	int len = sprintf (
	                  TimeStr,
	                  "%04d-%02d-%02d %02d:%02d:%02d: ",
	                  tm->tm_year+1900,
	                  tm->tm_mon+1,
	                  tm->tm_mday,
	                  tm->tm_hour,
	                  tm->tm_min,
	                  tm->tm_sec );
	fwrite ( TimeStr,1,len,file );
}

void write_msg_log ( const char* msg, int len, char* src )
{
	if ( msg_file == NULL )
	{
		msg_file = fopen ( msg_log, "ab" );
		if ( !msg_file )
		{
			printf ( "ERROR: Failed to OPEN/append %s log file!\n", msg_log );
			msg_file = ( FILE* )-1;
		}
	}
	if ( len && msg_file && ( msg_file != ( FILE* )-1 ) )
	{
		write_time_string ( msg_file );
		if ( src && strlen ( src ) )
		{
			fwrite ( src,1,strlen ( src ),msg_file );
		}
		int wtn = ( int ) fwrite ( msg,1,len,msg_file );
		if ( wtn != len )
		{
			fclose ( msg_file );
			msg_file = ( FILE* )-1;
			printf ( "ERROR: Failed to WRITE %d != %d to %s log file!\n", wtn, len, msg_log );
		}
		else
		{
			if ( msg[len-1] != '\n' )
			{
				fwrite ( ( char* ) "\n",1,1,msg_file );
			}
			fflush ( msg_file ); // push to disk now
		}
	}
}
#endif // #ifdef ADD_TRACKER_LOG

//////////////////////////////////////////////////////////////////////
/**
 * @class FG_SERVER
 */
/** @brief Constructor */
FG_SERVER::FG_SERVER
() : m_CrossfeedList("Crossfeed"),
     m_BlackList("Blacklist"),
     m_RelayList("Relays"),
     m_PlayerList("Users")
{
	typedef union
	{
		uint32_t    complete;
		int16_t     High;
		int16_t     Low;
	} converter;
	converter*    tmp;
	m_Initialized		= false;// Init() will do it
	m_ReinitData		= true; // init the data port
	m_ReinitTelnet		= true; // init the telnet port
	m_ReinitAdmin		= true; // init the telnet port
	m_ListenPort		= 5000; // port for client connections
	m_PlayerExpires		= 10; // standard expiration period
	m_Listening		= false;
	m_DataSocket		= 0;
	m_TelnetPort		= m_ListenPort+1;
	m_AdminPort		= m_ListenPort+2;
	m_NumMaxClients		= 0;
	m_PlayerIsOutOfReach	= 100;  // standard 100 nm
	m_IsParent		= false;
	m_ServerName		= "* Server *";
	m_BindAddress		= "";
	m_domain	= "";
	tmp			= ( converter* ) ( & PROTO_VER );
	m_ProtoMinorVersion	= tmp->High;
	m_ProtoMajorVersion	= tmp->Low;
	m_LogFileName		= DEF_SERVER_LOG; // "fg_server.log";
	m_RelayMap		= map<uint32_t, string>();
	m_IsTracked		= false; // off until config file read
	m_Tracker		= 0; // no tracker yet
	m_UpdateTrackerFreq	= DEF_UPDATE_SECS;
	// clear stats - should show what type of packet was received
	m_PacketsReceived	= 0;
	m_TelnetReceived	= 0;
	m_AdminReceived		= 0;
	m_BlackRejected		= 0;  // in black list
	m_PacketsInvalid	= 0;  // invalid packet
	m_UnknownRelay		= 0;  // unknown relay
	m_RelayMagic		= 0;  // relay magic packet
	m_PositionData		= 0;  // position data packet
	m_NotPosData		= 0;
	// clear totals
	mT_PacketsReceived	= 0;
	mT_BlackRejected	= 0;
	mT_PacketsInvalid	= 0;
	mT_UnknownRelay		= 0;
	mT_PositionData		= 0;
	mT_TelnetReceived	= 0;
	mT_RelayMagic		= 0;
	mT_NotPosData		= 0;
	m_CrossFeedFailed	= 0;
	m_CrossFeedSent		= 0;
	mT_CrossFeedFailed	= 0;
	mT_CrossFeedSent	= 0;
	m_TrackerConnect	= 0;
	m_TrackerDisconnect	= 0;
	m_TrackerPosition	= 0; // Tracker messages queued
	m_LocalClients		= 0;
	m_RemoteClients		= 0;

    // Be able to enable/disable file interface
    // On start-up if the file already exists, disable
    struct stat buf;
    m_useExitFile       = ( stat ( exit_file,&buf ) ) ? true : false;
    m_useResetFile      = ( stat ( reset_file,&buf ) ) ? true : false; // caution: this has failed in the past
    m_useStatFile       = ( stat ( stat_file,&buf ) ) ? true : false;

	m_Uptime		= time(0);
	m_WantExit		= false;
	ConfigFile		= "";
	SetLog (SG_FGMS|SG_FGTRACKER, SG_INFO);
	// SetLog (SG_ALL, SG_DISABLED);
} // FG_SERVER::FG_SERVER()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Standard destructor
 */
FG_SERVER::~FG_SERVER()
{
	Done();
} // FG_SERVER::~FG_SERVER()

static void*
telnet_helper( void* context )
{
	pthread_detach ( pthread_self() );
	st_telnet* t = reinterpret_cast<st_telnet*> ( context );
	FG_SERVER* tmp_server = t->Instance;
	tmp_server->HandleTelnet ( t->Fd );
	delete t;
	return 0;
}

void*
admin_helper( void* context )
{
	st_telnet* t = reinterpret_cast<st_telnet*> ( context );
	FG_SERVER* tmp_server = t->Instance;
	pthread_detach ( pthread_self() );
	tmp_server->HandleAdmin ( t->Fd );
	delete t;
	return 0;
}

void* detach_tracker ( void* vp )
{
	FG_TRACKER* pt = reinterpret_cast<FG_TRACKER*> (vp);
	pt->Loop();
	delete pt;
	return ( ( void* ) 0xdead );
}

//////////////////////////////////////////////////////////////////////
/**
 * @brief Basic initialization
 *
 *  If we are already initialized, close
 *  all connections and re-init all variables
 */
int
FG_SERVER::Init()
{
	//////////////////////////////////////////////////
	//      if we are already initialized, close
	//      all connections and re-init all
	//      variables
	//////////////////////////////////////////////////
	if ( !m_LogFile.is_open() )
	{
		m_LogFile.open ( m_LogFileName.c_str(), ios::out|ios::app );
		sglog().setLogLevels ( SG_FGMS, SG_INFO );
		sglog().enable_with_date ( true );
        if (m_LogFile.is_open()) 
        {
    		sglog().set_output ( m_LogFile );
        }
        else
        {
			SG_CONSOLE ( SG_FGMS, SG_ALERT, "FG_SERVER::Init() - "
			           << "Failed to open log file "
                       << m_LogFileName );
        }
	}
	if ( m_Initialized == false )
	{
		if ( m_Listening )
		{
			Done();
		}
		m_Initialized       = true;
		m_Listening         = false;
		m_DataSocket        = 0;
		m_NumMaxClients     = 0;
		netInit (); // WinSocket initialisation
	}
	if ( m_ReinitData )
	{
		if ( m_DataSocket )
		{
			delete m_DataSocket;
			m_DataSocket = 0;
		}
		m_DataSocket = new netSocket();
		if ( m_DataSocket->open ( false ) == 0 ) // UDP-Socket
		{
			SG_CONSOLE ( SG_FGMS, SG_ALERT, "FG_SERVER::Init() - "
			           << "failed to create listener socket" );
			return ( ERROR_CREATE_SOCKET );
		}
		m_DataSocket->setBlocking ( false );
		m_DataSocket->setSockOpt ( SO_REUSEADDR, true );
		if ( m_DataSocket->bind ( m_BindAddress.c_str(), m_ListenPort ) != 0 )
		{
			SG_CONSOLE ( SG_FGMS, SG_ALERT, "FG_SERVER::Init() - "
			           << "failed to bind to port " << m_ListenPort );
			SG_CONSOLE ( SG_FGMS, SG_ALERT, "already in use?" );
			return ( ERROR_COULDNT_BIND );
		}
		m_ReinitData = false;
	}
	if ( m_ReinitTelnet )
	{
		if ( m_TelnetSocket )
		{
			delete m_TelnetSocket;
			m_TelnetSocket = 0;
		}
		m_TelnetSocket = 0;
		if ( m_TelnetPort != 0 )
		{
			m_TelnetSocket = new netSocket;
			if ( m_TelnetSocket->open ( true ) == 0 ) // TCP-Socket
			{
				SG_CONSOLE ( SG_FGMS, SG_ALERT, "FG_SERVER::Init() - "
				           << "failed to create telnet socket" );
				return ( ERROR_CREATE_SOCKET );
			}
			m_TelnetSocket->setBlocking ( false );
			m_TelnetSocket->setSockOpt ( SO_REUSEADDR, true );
			if ( m_TelnetSocket->bind ( m_BindAddress.c_str(), m_TelnetPort ) != 0 )
			{
				SG_CONSOLE ( SG_FGMS, SG_ALERT, "FG_SERVER::Init() - "
				           << "failed to bind telnet socket " << m_TelnetPort );
				SG_CONSOLE ( SG_FGMS, SG_ALERT, "already in use?" );
				return ( ERROR_COULDNT_BIND );
			}
			if ( m_TelnetSocket->listen ( MAX_TELNETS ) != 0 )
			{
				SG_CONSOLE ( SG_FGMS, SG_ALERT, "FG_SERVER::Init() - "
				           << "failed to listen to telnet port" );
				return ( ERROR_COULDNT_LISTEN );
			}
		}
		m_ReinitTelnet = false;
	}
	if ( m_ReinitAdmin )
	{
		if ( m_AdminSocket )
		{
			delete m_AdminSocket;
		}
		m_AdminSocket = 0;
		if ( m_AdminPort != 0 )
		{
			m_AdminSocket = new netSocket;
			if ( m_AdminSocket->open ( true ) == 0 ) // TCP-Socket
			{
				SG_CONSOLE ( SG_FGMS, SG_ALERT, "FG_SERVER::Init() - "
				           << "failed to create admin socket" );
				return ( ERROR_CREATE_SOCKET );
			}
			m_AdminSocket->setBlocking ( false );
			m_AdminSocket->setSockOpt ( SO_REUSEADDR, true );
			if ( m_AdminSocket->bind ( m_BindAddress.c_str(), m_AdminPort ) != 0 )
			{
				SG_CONSOLE ( SG_FGMS, SG_ALERT, "FG_SERVER::Init() - "
				           << "failed to bind admin socket " << m_AdminPort );
				SG_CONSOLE ( SG_FGMS, SG_ALERT, "already in use?" );
				return ( ERROR_COULDNT_BIND );
			}
			if ( m_AdminSocket->listen ( MAX_TELNETS ) != 0 )
			{
				SG_CONSOLE ( SG_FGMS, SG_ALERT, "FG_SERVER::Init() - "
				           << "failed to listen to admin port" );
				return ( ERROR_COULDNT_LISTEN );
			}
		}
		m_ReinitAdmin = false;
	}
	SG_CONSOLE (SG_FGMS, SG_ALERT, "# This is " << m_ServerName << "(" << m_domain << ")");
	SG_CONSOLE ( SG_FGMS, SG_ALERT, "# FlightGear Multiplayer Server v"
	           << VERSION << " started" );
	SG_CONSOLE ( SG_FGMS, SG_ALERT, "# using protocol version v"
	           << m_ProtoMajorVersion << "." << m_ProtoMinorVersion
	           << " (LazyRelay enabled)" );
	SG_CONSOLE ( SG_FGMS, SG_ALERT,"# listening to port " << m_ListenPort );
	if ( m_TelnetSocket )
	{
		SG_CONSOLE ( SG_FGMS, SG_ALERT,"# telnet port " << m_TelnetPort );
	}
	else
	{
		SG_CONSOLE ( SG_FGMS, SG_ALERT,"# telnet port DISABLED");
	}
	if ( m_AdminSocket )
	{
		SG_CONSOLE ( SG_FGMS, SG_ALERT,"# admin port " << m_AdminPort );
	}
	else
	{
		SG_CONSOLE ( SG_FGMS, SG_ALERT,"# admin port DISABLED" );
	}
	SG_CONSOLE ( SG_FGMS, SG_ALERT,"# using logfile " << m_LogFileName );
	if ( m_BindAddress != "" )
	{
		SG_CONSOLE ( SG_FGMS, SG_ALERT,"# listening on " << m_BindAddress );
	}
	if ( m_IamHUB )
	{
		SG_CONSOLE ( SG_FGMS, SG_ALERT, "# I am a HUB Server" );
	}
	if (( m_IsTracked ) && (m_Tracker != 0))
	{
		pthread_t th;
		pthread_create ( &th, NULL, &detach_tracker, m_Tracker );
		SG_CONSOLE ( SG_FGMS, SG_ALERT, "# tracked to "
			   << m_Tracker->GetTrackerServer ()
			   << ":" << m_Tracker->GetTrackerPort ()
			   << ", using a thread."
		);
	}
	else
	{
		SG_CONSOLE ( SG_FGMS, SG_ALERT, "# tracking is disabled." );
	}
	size_t Count;
	FG_ListElement Entry("");
	//////////////////////////////////////////////////
	// print list of all relays
	//////////////////////////////////////////////////
	Count = m_RelayList.Size();
	SG_CONSOLE ( SG_FGMS, SG_ALERT, "# I have " << Count << " relays" );
	for (size_t i = 0; i < Count; i++)
	{
		Entry = m_RelayList[i];
		if (Entry.ID == FG_ListElement::NONE_EXISTANT)
			continue;
		SG_CONSOLE ( SG_FGMS, SG_ALERT, "# relay " << Entry.Name
		           << ":" << Entry.Address.getPort()
			   << " (" << Entry.Address.getHost() << ")");
	}
	//////////////////////////////////////////////////
	// print list of all crossfeeds
	//////////////////////////////////////////////////
	Count = m_CrossfeedList.Size();
	SG_CONSOLE ( SG_FGMS, SG_ALERT, "# I have " << Count << " crossfeeds" );
	for (size_t i = 0; i < Count; i++)
	{
		Entry = m_CrossfeedList[i];
		if (Entry.ID == FG_ListElement::NONE_EXISTANT)
			continue;
		SG_CONSOLE ( SG_FGMS, SG_ALERT, "# crossfeed " << Entry.Name
		           << ":" << Entry.Address.getPort() );
	}
	SG_CONSOLE ( SG_FGMS, SG_ALERT, "# I have " << m_BlackList.Size() << " blacklisted IPs" );

    if (m_useExitFile && m_useStatFile) // only show this IFF both are enabled
    {
        SG_CONSOLE ( SG_FGMS, SG_ALERT, "# Files: exit=[" << exit_file << "] stat=[" << stat_file << "]" );
    }

	m_Listening = true;
	return ( SUCCESS );
} // FG_SERVER::Init()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Do anything necessary to (re-) init the server  used to handle kill -HUP
 */
void
FG_SERVER::PrepareInit()
{
	if ( ! m_IsParent )
		return;
	SG_LOG ( SG_FGMS, SG_ALERT, "# caught SIGHUP, doing reinit!" );
	// release all locks
	m_PlayerList.Unlock ();
	m_RelayList.Unlock ();
	m_CrossfeedList.Unlock ();
	m_BlackList.Unlock ();
	// and clear all but the player list
	m_RelayList.Clear ();
	m_BlackList.Clear ();
	m_CrossfeedList.Clear ();
	m_RelayMap.clear ();	// clear(): is a std::map (NOT a FG_List)
	CloseTracker ();
} // FG_SERVER::PrepareInit ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle an admin session. 
 *
 * If a telnet connection to the admin port is established, a new FG_CLI
 * instance is created.
 * @param Fd -- docs todo --
 */
void*
FG_SERVER::HandleAdmin( int Fd )
{
	FG_CLI*	MyCLI;
	errno = 0;
	MyCLI = new FG_CLI ( this, Fd );
	MyCLI->loop ();
	if (Fd == 0)
	{	// reading from stdin
		WantExit();
	}
	delete MyCLI;
	return ( 0 );
}

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle a telnet session. if a telnet connection is opened, this
 *         method outputs a list  of all known clients.
 * @param Fd -- docs todo --
 */
void*
FG_SERVER::HandleTelnet( int Fd )
{
	errno = 0;
	string		Message;
	/** @brief  Geodetic Coordinates */
	Point3D		PlayerPosGeod;
	FG_Player	CurrentPlayer;
	netSocket	NewTelnet;
	unsigned int	it;
	NewTelnet.setHandle ( Fd );
	errno = 0;
	//////////////////////////////////////////////////
	//
	//      create the output message
	//      header
	//
	//////////////////////////////////////////////////
	Message  = "# This is " + m_ServerName;
	Message += "\n";
	Message += "# FlightGear Multiplayer Server v" + string ( VERSION );
	Message += "\n";
	Message += "# using protocol version v";
	Message += NumToStr ( m_ProtoMajorVersion, 0 );
	Message += "." + NumToStr ( m_ProtoMinorVersion, 0 );
	Message += " (LazyRelay enabled)";
	Message += "\n";
	if ( m_IsTracked )
	{
		Message += "# This server is tracked: ";
		Message += m_Tracker->GetTrackerServer();
		Message += "\n";
	}
	if ( NewTelnet.write_str ( Message ) < 0 )
	{
		if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
		{
			SG_LOG ( SG_FGMS, SG_ALERT, "FG_SERVER::HandleTelnet() - " << strerror ( errno ) );
		}
		return ( 0 );
	}
	Message  = "# "+ NumToStr ( m_PlayerList.Size(), 0 );
	Message += " pilot(s) online\n";
	if ( NewTelnet.write_str ( Message ) < 0 )
	{
		if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
		{
			SG_LOG ( SG_FGMS, SG_ALERT, "FG_SERVER::HandleTelnet() - " << strerror ( errno ) );
		}
		return ( 0 );
	}
	//////////////////////////////////////////////////
	//
	//      create list of players
	//
	//////////////////////////////////////////////////
	it = 0;
	for ( ;; )
	{
		if ( it < m_PlayerList.Size() )
		{
			CurrentPlayer = m_PlayerList[it];
			it++;
		}
		else
		{
			break;
		}
		if (CurrentPlayer.Name.compare (0, 3, "obs", 3) == 0)
		{
			continue;
		}
		sgCartToGeod ( CurrentPlayer.LastPos, PlayerPosGeod );
		Message = CurrentPlayer.Name + "@";
		if ( CurrentPlayer.IsLocal )
		{
			Message += "LOCAL: ";
		}
		else
		{
			mT_RelayMapIt Relay = m_RelayMap.find ( CurrentPlayer.Address.getIP() );
			if ( Relay != m_RelayMap.end() )
			{
				Message += Relay->second + ": ";
			}
			else
			{
				Message += CurrentPlayer.Origin + ": ";
			}
		}
		if ( CurrentPlayer.Error != "" )
		{
			Message += CurrentPlayer.Error + " ";
		}
		Message += NumToStr ( CurrentPlayer.LastPos[X], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.LastPos[Y], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.LastPos[Z], 6 ) +" ";
		Message += NumToStr ( PlayerPosGeod[Lat], 6 ) +" ";
		Message += NumToStr ( PlayerPosGeod[Lon], 6 ) +" ";
		Message += NumToStr ( PlayerPosGeod[Alt], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.LastOrientation[X], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.LastOrientation[Y], 6 ) +" ";
		Message += NumToStr ( CurrentPlayer.LastOrientation[Z], 6 ) +" ";
		Message += CurrentPlayer.ModelName;
		Message += "\n";
		if ( NewTelnet.write_str ( Message ) < 0 )
		{
			if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
			{
				SG_LOG ( SG_FGMS, SG_ALERT, "FG_SERVER::HandleTelnet() - " << strerror ( errno ) );
			}
			return ( 0 );
		}
	}
	NewTelnet.close ();
	return ( 0 );
} // FG_SERVER::HandleTelnet ()

//////////////////////////////////////////////////////////////////////
/**
 * @brief  If we receive bad data from a client, we add the client to
 *         the internal list anyway, but mark them as bad. But first
 *          we look if it isn't already there.
 *          Send an error message to the bad client.
 * @param Sender
 * @param ErrorMsg
 * @param IsLocal
 */
void
FG_SERVER::AddBadClient
( const netAddress& Sender, string&	ErrorMsg, bool IsLocal, int Bytes)
{
	string		Message;
	FG_Player	NewPlayer;
	PlayerIt	CurrentPlayer;
	//////////////////////////////////////////////////
	//      see, if we already know the client
	//////////////////////////////////////////////////
	m_PlayerList.Lock ();
	CurrentPlayer = m_PlayerList.Find (Sender);
	if ( CurrentPlayer != m_PlayerList.End () )
	{
		CurrentPlayer->UpdateRcvd (Bytes);
		m_PlayerList.UpdateRcvd (Bytes);
		m_PlayerList.Unlock();
		return;
	}
	//////////////////////////////////////////////////
	//      new client, add to the list
	//////////////////////////////////////////////////
	if (IsLocal)
		m_LocalClients++;
	else
		m_RemoteClients++;
	NewPlayer.Name      = "* Bad Client *";
	NewPlayer.ModelName     = "* unknown *";
	NewPlayer.Origin        = Sender.getHost ();
	NewPlayer.Address       = Sender;
	NewPlayer.IsLocal       = IsLocal;
	NewPlayer.HasErrors     = true;
	NewPlayer.Error         = ErrorMsg;
	NewPlayer.UpdateRcvd (Bytes);
	SG_LOG ( SG_FGMS, SG_WARN, "FG_SERVER::AddBadClient() - " << ErrorMsg );
	m_PlayerList.Add (NewPlayer, m_PlayerExpires);
	m_PlayerList.UpdateRcvd (Bytes);
	m_PlayerList.Unlock();
} // FG_SERVER::AddBadClient ()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new client to internal list
 * @param Sender
 * @param Msg
 */
void
FG_SERVER::AddClient( const netAddress& Sender,char* Msg )
{
	uint32_t	MsgMagic;
	string		Message;
	string		Origin;
	T_MsgHdr*	MsgHdr;
	T_PositionMsg*	PosMsg;
	FG_Player	NewPlayer;
	bool		IsLocal;
	MsgHdr		= ( T_MsgHdr* ) Msg;
	PosMsg		= ( T_PositionMsg* ) ( Msg + sizeof ( T_MsgHdr ) );
	MsgMagic	= XDR_decode<uint32_t> ( MsgHdr->Magic );
	IsLocal		= true;
	if ( MsgMagic == RELAY_MAGIC ) // not a local client
	{
		IsLocal = false;
	}
	NewPlayer.Name	    = MsgHdr->Name;
	NewPlayer.Passwd    = "test"; //MsgHdr->Passwd;
	NewPlayer.ModelName = "* unknown *";
	NewPlayer.Origin    = Sender.getHost ();
	NewPlayer.Address   = Sender;
	NewPlayer.IsLocal   = IsLocal;
	NewPlayer.LastPos.Set (
	        XDR_decode64<double> ( PosMsg->position[X] ),
	        XDR_decode64<double> ( PosMsg->position[Y] ),
	        XDR_decode64<double> ( PosMsg->position[Z] )
	);
	NewPlayer.LastOrientation.Set (
	        XDR_decode<float> ( PosMsg->orientation[X] ),
	        XDR_decode<float> ( PosMsg->orientation[Y] ),
	        XDR_decode<float> ( PosMsg->orientation[Z] )
	);
	NewPlayer.ModelName = PosMsg->Model;
	m_PlayerList.Add ( NewPlayer, m_PlayerExpires );
	size_t NumClients = m_PlayerList.Size ();
	if ( NumClients > m_NumMaxClients )
	{
		m_NumMaxClients = NumClients;
	}
	if ( IsLocal )
	{
		m_LocalClients++;
		UpdateTracker ( NewPlayer.Name, NewPlayer.Passwd, NewPlayer.ModelName, NewPlayer.LastSeen, CONNECT );
	}
	else
	{
		m_RemoteClients++;
	}
	Origin  = NewPlayer.Origin;
	if ( IsLocal )
	{
		Message = "New LOCAL Client: ";
	}
	else
	{
		Message = "New REMOTE Client: ";
		mT_RelayMapIt Relay = m_RelayMap.find ( NewPlayer.Address.getIP() );
		if ( Relay != m_RelayMap.end() )
		{
			Origin = Relay->second;
		}
	}
	SG_LOG ( SG_FGMS, SG_INFO, Message
	         << NewPlayer.Name << "@"
	         << Origin << ":" << Sender.getPort()
	         << " (" << NewPlayer.ModelName << ")"
	         << " current clients: "
	         << NumClients << " max: " << m_NumMaxClients
	       );
} // FG_SERVER::AddClient()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new relay server into internal list
 * @param Server
 * @param Port
 */
void
FG_SERVER::AddRelay( const string& Relay, int Port )
{
	FG_ListElement  B (Relay);

	B.Address.set ( ( char* ) Relay.c_str(), Port );
	if (B.Address.getIP() == 0)
	{
		SG_LOG ( SG_FGMS, SG_ALERT,
			"could not resolve '" << Relay << "'");
		return;
	}
	if ((B.Address.getIP() >= 0x7F000001) &&  (B.Address.getIP() <= 0x7FFFFFFF))
	{
		SG_LOG ( SG_FGMS, SG_ALERT,
			"relay points back to me '" << Relay << "'");
		return;
	}
	m_RelayList.Lock ();
	ItList CurrentEntry = m_RelayList.Find ( B.Address, "" );
	m_RelayList.Unlock ();
	if ( CurrentEntry == m_RelayList.End() )
	{	
		m_RelayList.Add (B, 0);
		string S;
		if (B.Address.getHost() == Relay)
		{
			S = Relay;
		}
		else
		{
			unsigned I;
			I = Relay.find ( "." );
			if ( I != string::npos )
			{
				S = Relay.substr ( 0, I );
			}
			else
			{
				S = Relay;
			}
		}
		m_RelayMap[B.Address.getIP()] = S;
	}
} // FG_SERVER::AddRelay()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Insert a new crossfeed server into internal list
 * @param Server char with server
 * @param Port int with port number
 */
void
FG_SERVER::AddCrossfeed( const string& Server, int Port )
{
	string s = Server;
#ifdef _MSC_VER
	if ( s == "localhost" )
	{
		s = "127.0.0.1";
	}
#endif // _MSC_VER
	FG_ListElement B (s);
	B.Address.set ( ( char* ) s.c_str(), Port );
	m_CrossfeedList.Lock ();
	ItList CurrentEntry = m_CrossfeedList.Find ( B.Address, "" );
	m_CrossfeedList.Unlock ();
	if ( CurrentEntry == m_CrossfeedList.End() )
	{	
		m_CrossfeedList.Add (B, 0);
	}
} // FG_SERVER::AddCrossfeed()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Add a tracking server
 * @param Server String with server
 * @param Port The port number
 * @param IsTracked Is Stracked
 * @retval int -1 for fail or SUCCESS
 */
int
FG_SERVER::AddTracker( const string& Server, int Port, bool IsTracked )
{
	CloseTracker();
	m_IsTracked = IsTracked;
	m_Tracker = new FG_TRACKER ( Port, Server, m_ServerName, m_domain );
	return ( SUCCESS );
} // FG_SERVER::AddTracker()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Add an IP to the blacklist
 * @param FourDottedIP IP to add to blacklist
 */
void
FG_SERVER::AddBlacklist( const string& DottedIP, const string& Reason, time_t Timeout)
{

	FG_ListElement B (Reason);
	B.Address.set (DottedIP.c_str(), 0);
	m_BlackList.Lock ();
	ItList CurrentEntry = m_BlackList.Find ( B.Address, "" );
	m_BlackList.Unlock ();
	if ( CurrentEntry == m_BlackList.End() )
	{	// FIXME: every list has its own standard TTL
		m_BlackList.Add (B, Timeout);
	}
} // FG_SERVER::AddBlacklist()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Check if the sender is a known relay
 * @param SenderAddress
 * @retval bool true if known relay
 */
bool
FG_SERVER::IsKnownRelay( const netAddress& SenderAddress, size_t Bytes)
{
	ItList CurrentEntry;
	m_RelayList.Lock ();
	CurrentEntry = m_RelayList.Find ( SenderAddress, "" );
	if ( CurrentEntry != m_RelayList.End() )
	{
		m_RelayList.UpdateRcvd (CurrentEntry, Bytes);
		m_RelayList.Unlock ();
		return true;
	}
	m_RelayList.Unlock ();
	string ErrorMsg;
	ErrorMsg  = SenderAddress.getHost();
	ErrorMsg += " is not a valid relay!";
	AddBlacklist ( SenderAddress.getHost(), "not a valid relay", 0);
	SG_LOG ( SG_FGMS, SG_ALERT, "UNKNOWN RELAY: " << ErrorMsg );
	return ( false );
} // FG_SERVER::IsKnownRelay ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      check if the packet is valid
//
//////////////////////////////////////////////////////////////////////
/**
 * @brief
 */
bool
FG_SERVER::PacketIsValid( int Bytes, T_MsgHdr*  MsgHdr, const netAddress& SenderAddress)
{
	uint32_t        MsgMagic;
	uint32_t        MsgLen;
	uint32_t        MsgId;
	string          ErrorMsg;
	string          Origin;
	typedef union
	{
		uint32_t    complete;
		int16_t     High;
		int16_t     Low;
	} converter;

	Origin   = SenderAddress.getHost();
	MsgMagic = XDR_decode<uint32_t> ( MsgHdr->Magic );
	MsgId    = XDR_decode<uint32_t> ( MsgHdr->MsgId );
	MsgLen   = XDR_decode<uint32_t> ( MsgHdr->MsgLen );
	if ( Bytes < ( int ) sizeof ( MsgHdr ) )
	{
		ErrorMsg  = SenderAddress.getHost();
		ErrorMsg += " packet size is too small!";
		AddBadClient ( SenderAddress, ErrorMsg, true, Bytes );
		return ( false );
	}
	if ( ( MsgMagic != MSG_MAGIC ) && ( MsgMagic != RELAY_MAGIC ) )
	{
		char m[5];
		memcpy ( m, ( char* ) &MsgMagic, 4 );
		m[4] = 0;
		ErrorMsg  = Origin;
		ErrorMsg += " BAD magic number: ";
		ErrorMsg += m;
		AddBadClient ( SenderAddress, ErrorMsg, true, Bytes );
		return ( false );
	}
	if ( XDR_decode<uint32_t> ( MsgHdr->Version ) != PROTO_VER )
	{
		MsgHdr->Version = XDR_decode<uint32_t> ( MsgHdr->Version );
		ErrorMsg  = Origin;
		ErrorMsg += " BAD protocol version! Should be ";
		converter*    tmp;
		tmp = ( converter* ) ( & PROTO_VER );
		ErrorMsg += NumToStr ( tmp->High, 0 );
		ErrorMsg += "." + NumToStr ( tmp->Low, 0 );
		ErrorMsg += " but is ";
		tmp = ( converter* ) ( & MsgHdr->Version );
		ErrorMsg += NumToStr ( tmp->Low, 0 );
		ErrorMsg += "." + NumToStr ( tmp->High, 0 );
		AddBadClient ( SenderAddress, ErrorMsg, true, Bytes );
		return ( false );
	}
	if ( MsgId == POS_DATA_ID )
	{
		if ( MsgLen < sizeof ( T_MsgHdr ) + sizeof ( T_PositionMsg ) )
		{
			ErrorMsg  = Origin;
			ErrorMsg += " Client sends insufficient position data, ";
			ErrorMsg += "should be ";
			ErrorMsg += NumToStr ( sizeof ( T_MsgHdr ) +sizeof ( T_PositionMsg ) );
			ErrorMsg += " is: " + NumToStr ( MsgHdr->MsgLen );
			AddBadClient ( SenderAddress, ErrorMsg, true, Bytes );
			return ( false );
		}
	}
	return ( true );
} // FG_SERVER::PacketIsValid ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Send message to all crossfeed servers.
 *         Crossfeed servers receive all traffic without condition,
 *         mainly used for testing and debugging
 */
void
FG_SERVER::SendToCrossfeed( char* Msg, int Bytes, const netAddress& SenderAddress)
{
	T_MsgHdr*       MsgHdr;
	uint32_t        MsgMagic;
	int             sent;
	ItList		Entry;
	MsgHdr    = ( T_MsgHdr* ) Msg;
	MsgMagic  = MsgHdr->Magic;
	MsgHdr->Magic = XDR_encode<uint32_t> ( RELAY_MAGIC );
	// pass on senders address and port to crossfeed server
	MsgHdr->ReplyAddress = XDR_encode<uint32_t> ( SenderAddress.getIP() );
	MsgHdr->ReplyPort = XDR_encode<uint32_t> ( SenderAddress.getPort() );
	m_CrossfeedList.Lock();
	for (Entry = m_CrossfeedList.Begin(); Entry != m_CrossfeedList.End(); Entry++)
	{
		sent = m_DataSocket->sendto ( Msg, Bytes, 0, &Entry->Address );
		m_CrossfeedList.UpdateSent (Entry, sent);
	}
	m_CrossfeedList.Unlock();
	MsgHdr->Magic = MsgMagic;  // restore the magic value
} // FG_SERVER::SendToCrossfeed ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Send message to all relay servers
 */
void
FG_SERVER::SendToRelays( char* Msg, int Bytes,FG_Player& SendingPlayer )
{
	T_MsgHdr*       MsgHdr;
	uint32_t        MsgMagic;
	unsigned int    PktsForwarded = 0;
	ItList		CurrentRelay;

	if ( (! SendingPlayer.IsLocal ) && ( ! m_IamHUB ) )
	{
		return;
	}
	MsgHdr    = ( T_MsgHdr* ) Msg;
	MsgMagic  = XDR_decode<uint32_t> ( MsgHdr->Magic );
	MsgHdr->Magic = XDR_encode<uint32_t> ( RELAY_MAGIC );
	m_RelayList.Lock ();
	CurrentRelay = m_RelayList.Begin();
	while ( CurrentRelay != m_RelayList.End() )
	{

		if ( CurrentRelay->Address.getIP() != SendingPlayer.Address.getIP() )
		{
			if ( SendingPlayer.DoUpdate || IsInRange ( *CurrentRelay, SendingPlayer ) )
			{
				m_DataSocket->sendto ( Msg, Bytes, 0, &CurrentRelay->Address );
				m_RelayList.UpdateSent (CurrentRelay, Bytes);
				PktsForwarded++;
			}
		}
		CurrentRelay++;
	}
	m_RelayList.Unlock ();
	MsgHdr->Magic = XDR_encode<uint32_t> ( MsgMagic ); // restore the magic value
} // FG_SERVER::SendToRelays ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//	Remove Player from list
void
FG_SERVER::DropClient( PlayerIt& CurrentPlayer )
{
	string Origin;
	if ((CurrentPlayer->IsLocal) && (CurrentPlayer->HasErrors == false))
	{
		UpdateTracker (CurrentPlayer->Name, CurrentPlayer->Passwd, CurrentPlayer->ModelName, CurrentPlayer->LastSeen, DISCONNECT);
	}
	if (CurrentPlayer->IsLocal)
		m_LocalClients--;
	else
		m_RemoteClients--;
	mT_RelayMapIt Relay = m_RelayMap.find ( CurrentPlayer->Address.getIP() );
	if ( Relay != m_RelayMap.end() )
	{
		Origin = Relay->second;
	}
	else
	{
		Origin = "LOCAL";
	}
	SG_LOG (SG_FGMS, SG_INFO, "TTL exceeded, dropping pilot "
		<< CurrentPlayer->Name << "@" << Origin
		<< " after " << time(0)-CurrentPlayer->JoinTime << " seconds. "
		<< "Current clients: "
		<< m_PlayerList.Size()-1 << " max: " << m_NumMaxClients
	);
	CurrentPlayer = m_PlayerList.Delete (CurrentPlayer);
} // FG_SERVER::DropClient()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Handle client connections
 * @param Msg
 * @param Bytes
 * @param SenderAddress
 */
void
FG_SERVER::HandlePacket( char* Msg, int Bytes, const netAddress& SenderAddress )
{
	T_MsgHdr*       MsgHdr;
	T_PositionMsg*  PosMsg;
	uint32_t        MsgId;
	uint32_t        MsgMagic;
	Point3D         SenderPosition;
	Point3D         SenderOrientation;
	Point3D         PlayerPosGeod;
	PlayerIt	CurrentPlayer;
	FG_Player	SendingPlayer;
	ItList		CurrentEntry;
	time_t          Now;
	unsigned int    PktsForwarded = 0;
	MsgHdr    = ( T_MsgHdr* ) Msg;
	MsgMagic  = XDR_decode<uint32_t> ( MsgHdr->Magic );
	MsgId     = XDR_decode<uint32_t> ( MsgHdr->MsgId );
	Now   = time ( 0 );
	//////////////////////////////////////////////////
	//
	//  First of all, send packet to all
	//  crossfeed servers.
	//
	//////////////////////////////////////////////////
	SendToCrossfeed ( Msg, Bytes, SenderAddress );
	//////////////////////////////////////////////////
	//
	//  Now do the local processing
	//
	//////////////////////////////////////////////////
	m_BlackList.Lock ();
	CurrentEntry = m_BlackList.Find ( SenderAddress, "" );
	if ( CurrentEntry != m_BlackList.End() )
	{
		m_BlackList.UpdateRcvd (CurrentEntry, Bytes);
		m_BlackRejected++;
		m_BlackList.Unlock ();
		return;
	}
	m_BlackList.Unlock ();
	if ( ! PacketIsValid ( Bytes, MsgHdr, SenderAddress ) )
	{
		m_PacketsInvalid++;
		return;
	}
	if ( MsgMagic == RELAY_MAGIC ) // not a local client
	{
		if ( ! IsKnownRelay ( SenderAddress, Bytes ) )
		{
			m_UnknownRelay++;
			return;
		}
		else
		{
			m_RelayMagic++; // bump relay magic packet
		}
	}
	//////////////////////////////////////////////////
	//
	//    Store senders position
	//
	//////////////////////////////////////////////////
	if ( MsgId == POS_DATA_ID )
	{
		m_PositionData++;
		PosMsg = ( T_PositionMsg* ) ( Msg + sizeof ( T_MsgHdr ) );
		double x = XDR_decode64<double> ( PosMsg->position[X] );
		double y = XDR_decode64<double> ( PosMsg->position[Y] );
		double z = XDR_decode64<double> ( PosMsg->position[Z] );
		if ( ( x == 0.0 ) || ( y == 0.0 ) || ( z == 0.0 ) )
		{
			// ignore while position is not settled
			return;
		}
		SenderPosition.Set ( x, y, z );
		SenderOrientation.Set (
		        XDR_decode<float> ( PosMsg->orientation[X] ),
		        XDR_decode<float> ( PosMsg->orientation[Y] ),
		        XDR_decode<float> ( PosMsg->orientation[Z] )
		);
	}
	else
	{
		m_NotPosData++;
	}
	//////////////////////////////////////////////////
	//
	//    Add Client to list if its not known
	//
	//////////////////////////////////////////////////
	m_PlayerList.Lock();
	CurrentPlayer = m_PlayerList.FindByName ( MsgHdr->Name );
	if (CurrentPlayer == m_PlayerList.End () )
	{
		// unknown, add to the list
		if ( MsgId != POS_DATA_ID )
		{
			// ignore clients until we have a valid position
			m_PlayerList.Unlock();
			return;
		}
		AddClient ( SenderAddress, Msg );
	}
	else
	{
		if ( CurrentPlayer->Address != SenderAddress )
		{
			m_PlayerList.Unlock();
			return;
		}
	}
	m_PlayerList.Unlock();
	//////////////////////////////////////////
	//
	//      send the packet to all clients.
	//      since we are walking through the list,
	//      we look for the sending client, too. if it
	//      is not already there, add it to the list
	//
	//////////////////////////////////////////////////
	MsgHdr->Magic = XDR_encode<uint32_t> ( MSG_MAGIC );
	CurrentPlayer = m_PlayerList.Begin();
	while ( CurrentPlayer != m_PlayerList.End() )
	{
		//////////////////////////////////////////////////
		//
		//      ignore clients with errors
		//
		//////////////////////////////////////////////////
		if ( CurrentPlayer->HasErrors )
		{
			if ((Now - CurrentPlayer->LastSeen) > CurrentPlayer->Timeout )
			{
				DropClient (CurrentPlayer); 
			}
			else
			{
				CurrentPlayer++;
			}
			continue;
		}
		//////////////////////////////////////////////////
		//        Sender == CurrentPlayer?
		//////////////////////////////////////////////////
		//  FIXME: if Sender is a Relay,
		//         CurrentPlayer->Address will be
		//         address of Relay and not the client's!
		//         so use a clientID instead
		if ( CurrentPlayer->Name == MsgHdr->Name )
		{
			if ( MsgId == POS_DATA_ID )
			{
				CurrentPlayer->LastPos         = SenderPosition;
				CurrentPlayer->LastOrientation = SenderOrientation;
			}
			else
			{
				SenderPosition    = CurrentPlayer->LastPos;
				SenderOrientation = CurrentPlayer->LastOrientation;
			}
			if ( CurrentPlayer->IsLocal )
			{
				m_PlayerList.UpdateRcvd (CurrentPlayer, Bytes);
			}
			//////////////////////////////////////////////////
			//	send update to inactive relays?
			//////////////////////////////////////////////////
			CurrentPlayer->DoUpdate = ( (Now - CurrentPlayer->LastRelayedToInactive) > UPDATE_INACTIVE_PERIOD );
			if ( CurrentPlayer->DoUpdate )
			{
				CurrentPlayer->LastRelayedToInactive = Now;
			}
			SendingPlayer = *CurrentPlayer;
			CurrentPlayer++;
			continue; // don't send packet back to sender
		}
		//////////////////////////////////////////////////
		//      do not send packets to clients if the
		//      origin is an observer, but do send
		//      chat messages anyway
		//      FIXME: MAGIC = SFGF!
		//////////////////////////////////////////////////
		if ( strncasecmp ( MsgHdr->Name, "obs", 3 ) == 0 )
		{
			return;
		}
		//////////////////////////////////////////////////
		//
		//      do not send packet to clients which
		//      are out of reach.
		//      FIX20140603 - compare fix by Markus Pargmann
		//////////////////////////////////////////////////
		if ( ( Distance ( SenderPosition, CurrentPlayer->LastPos ) > m_PlayerIsOutOfReach )
		&&   (CurrentPlayer->Name.compare (0, 3, "obs", 3) != 0 ) )
		{
			CurrentPlayer++;
			continue;
		}
		//////////////////////////////////////////////////
		//
		//  only send packet to local clients
		//
		//////////////////////////////////////////////////
		if ( CurrentPlayer->IsLocal )
		{
			m_DataSocket->sendto ( Msg, Bytes, 0, &CurrentPlayer->Address );
			m_PlayerList.UpdateSent (CurrentPlayer, Bytes);
			PktsForwarded++;
		}
		CurrentPlayer++;
	}
	if ( SendingPlayer.ID ==  FG_ListElement::NONE_EXISTANT )
	{
		// player not yet in our list
		// should not happen, but test just in case
		SG_LOG ( SG_FGMS, SG_ALERT, "## BAD => "
		         << MsgHdr->Name << ":" << SenderAddress.getHost()
		       );
		return;
	}
	SendToRelays ( Msg, Bytes, SendingPlayer );
} // FG_SERVER::HandlePacket ();
//////////////////////////////////////////////////////////////////////

/**
 * @brief Show Stats
 */
void FG_SERVER::Show_Stats ( void )
{
	int pilot_cnt, local_cnt;
	// update totals since start
	mT_PacketsReceived += m_PacketsReceived;
	mT_BlackRejected   += m_BlackRejected;
	mT_PacketsInvalid  += m_PacketsInvalid;
	mT_UnknownRelay    += m_UnknownRelay;
	mT_RelayMagic      += m_RelayMagic;
	mT_PositionData    += m_PositionData;
	mT_NotPosData      += m_NotPosData;
	mT_TelnetReceived  += m_TelnetReceived;
	mT_CrossFeedFailed += m_CrossFeedFailed;
	mT_CrossFeedSent   += m_CrossFeedSent;
	// output to LOG and cerr channels
	pilot_cnt = local_cnt = 0;
	FG_Player CurrentPlayer; // get LOCAL pilot count
	pilot_cnt = m_PlayerList.Size ();
	for (int i = 0; i < pilot_cnt; i++)
	{
		CurrentPlayer = m_PlayerList[i];
		if (CurrentPlayer.ID == FG_ListElement::NONE_EXISTANT)
			continue;
		if ( CurrentPlayer.IsLocal )
		{
			local_cnt++;
		}
	}
	SG_LOG ( SG_FGMS, SG_ALERT, "## Pilots: total " << pilot_cnt << ", local " << local_cnt );
	SG_LOG ( SG_FGMS, SG_ALERT, "## Since: Packets " <<
	           m_PacketsReceived << " BL=" <<
	           m_BlackRejected << " INV=" <<
	           m_PacketsInvalid << " UR=" <<
	           m_UnknownRelay << " RD=" <<
	           m_RelayMagic << " PD=" <<
	           m_PositionData << " NP=" <<
	           m_NotPosData << " CF=" <<
	           m_CrossFeedSent << "/" << m_CrossFeedFailed << " TN=" <<
	           m_TelnetReceived
	         );
	SG_LOG ( SG_FGMS, SG_ALERT, "## Total: Packets " <<
	           mT_PacketsReceived << " BL=" <<
	           mT_BlackRejected << " INV=" <<
	           mT_PacketsInvalid << " UR=" <<
	           mT_UnknownRelay << " RD=" <<
	           mT_RelayMagic << " PD=" <<
	           mT_PositionData << " NP=" <<
	           mT_NotPosData <<  " CF=" <<
	           mT_CrossFeedSent << "/" << mT_CrossFeedFailed << " TN=" <<
	           mT_TelnetReceived << " TC/D/P=" <<
	           m_TrackerConnect << "/" << m_TrackerDisconnect << "/" << m_TrackerPosition
	         );
	// restart 'since' last stat counter
	m_PacketsReceived = m_BlackRejected = m_PacketsInvalid = 0;
	m_UnknownRelay = m_PositionData = m_TelnetReceived = 0; // reset
	m_RelayMagic = m_NotPosData = 0; // reset
	m_CrossFeedFailed = m_CrossFeedSent = 0;
}

/**
 * @brief Check exit and stat files
 *
 * Do not think this is used by many, but is a convenient way to output some stats
 * to the LOG, or request an exit. In the past the reset action has failed, and 
 * although some fixes have been put in place, a caution about using this reset.
 *
 * 20150619:0.11.9: If running instance can NOT delete a detected file, usually due to wrong permissions,
 * that particular file interface will be disabled. Also if any of these files exist at start-up, again 
 * that file interface will be disable. This also gives a way to disable this file interface actions.
 *
 */
int
FG_SERVER::check_files()
{
	struct stat buf;
	if ( m_useExitFile && (stat ( exit_file,&buf ) == 0) )
	{
		SG_LOG ( SG_FGMS, SG_ALERT, "## Got EXIT file : " << exit_file );
		unlink ( exit_file );
		if ( stat ( exit_file,&buf ) == 0 )
		{
			SG_LOG ( SG_FGMS, SG_ALERT, "WARNING: Unable to delete exit file " << exit_file << "! Disabled interface..." );
            m_useExitFile = false;
		}
		return 1;
	}
	else if ( m_useResetFile && ( stat ( reset_file,&buf ) == 0 ) )
	{
		SG_LOG ( SG_FGMS, SG_ALERT, "## Got RESET file " << reset_file );
		unlink ( reset_file );
		if ( stat ( reset_file,&buf ) == 0 )
		{
			SG_LOG ( SG_FGMS, SG_ALERT, "WARNING: Unable to delete reset file " << reset_file << "! Disabled interface..." );
			m_useResetFile = false;
		}
		m_ReinitData	= true; // init the data port
		m_ReinitTelnet	= true; // init the telnet port
		m_ReinitAdmin	= true; // init the admin port
		SigHUPHandler ( 0 );
	}
	else if ( m_useStatFile && ( stat ( stat_file,&buf ) == 0 ) )
	{
		SG_LOG ( SG_FGMS, SG_ALERT, "## Got STAT file " << stat_file );
		unlink ( stat_file );
		if ( stat ( stat_file,&buf ) == 0 )
		{
			SG_LOG ( SG_FGMS, SG_ALERT, "WARNING: Unable to delete stat file " << stat_file << "! Disabled interface..." );
			m_useStatFile = false;
		}
		Show_Stats();
	}
#ifdef _MSC_VER
	if (!AddCLI && _kbhit())
	{
		int ch = _getch ();
		if ( ch == 0x1b )
		{
			printf("Got ESC key to exit...\n");
			return 1;
		}
		else
		{
			printf("Got UNKNOWN keyboard! %#X - Only ESC, to exit\n", ch);
		}
	}
#endif // _MSC_VER
	return 0;
}

//////////////////////////////////////////////////////////////////////
/**
 * @brief allow the Admin CLI to shut down fgms
 */
void
FG_SERVER::WantExit()
{
	m_WantExit = true;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Main loop of the server
 */
int
FG_SERVER::Loop()
{
	int         Bytes;
	char        Msg[MAX_PACKET_SIZE];
	netAddress  SenderAddress;
	netSocket*  ListenSockets[3 + MAX_TELNETS];
	time_t      LastTrackerUpdate;
	time_t      CurrentTime;
	LastTrackerUpdate = time ( 0 );
	PlayerIt	CurrentPlayer;
	m_IsParent = true;
	if ( m_Listening == false )
	{
		SG_LOG ( SG_FGMS, SG_ALERT, "FG_SERVER::Loop() - "
		           << "not listening on any socket!" );
		return ( ERROR_NOT_LISTENING );
	}
#ifdef _MSC_VER
	SG_LOG ( SG_FGMS, SG_ALERT,  "ESC key to EXIT (after select " << m_PlayerExpires << " sec timeout)." );
#endif
	if ( (m_AdminUser == "" ) || (m_AdminPass == "") )
	{
		if (m_AdminSocket)
		{
			m_AdminSocket->close();
			delete m_AdminSocket;
			m_AdminSocket = 0;
			SG_CONSOLE (SG_FGMS, SG_ALERT, "# Admin port disabled, please set user and password");
		}
	}
	if (! RunAsDaemon && AddCLI )
	{	// Run admin CLI in foreground reading from stdin
		st_telnet* t = new st_telnet;
		t->Instance = this;
		t->Fd       = 0;
		pthread_t th;
		pthread_create ( &th, NULL, &admin_helper, t );
	}
	//////////////////////////////////////////////////
	//
	//      infinite listening loop
	//
	//////////////////////////////////////////////////
	while ( m_WantExit == false )
	{
		if (! m_Listening )
		{
			cout << "bummer 1!" << endl;
			return 1;
		}
		if (m_DataSocket == 0)
		{
			cout << "bummer 2!" << endl;
			return 2;
		}
		CurrentTime = time ( 0 );
		// check timeout
		CurrentPlayer = m_PlayerList.Begin();
		for (size_t i = 0; i < m_PlayerList.Size(); i++)
		{	
			if(!m_PlayerList.CheckTTL ( i ) || (((CurrentTime - CurrentPlayer->LastSeen) > CurrentPlayer->Timeout ) &&  ((CurrentTime - CurrentPlayer->JoinTime) > 30)))
			{
				DropClient (CurrentPlayer); 
			}
			CurrentPlayer++;
		}
		for (size_t i = 0; i < m_BlackList.Size(); i++)
		{
			if(!m_BlackList.CheckTTL ( i ))
			{
				m_BlackList.DeleteByPosition ( i );
			}
		}
		
		// Update some things every (default) 10 secondes
		if ( ( CurrentTime - LastTrackerUpdate ) >= m_UpdateTrackerFreq )
		{
			LastTrackerUpdate = time ( 0 );
			if ( m_PlayerList.Size() >0 )
			{
				// updates the position of the users
				// regularly (tracker)
				UpdateTracker ("" , "", "", LastTrackerUpdate, UPDATE );
			}
			if ( check_files() )
			{
				break;
			}
		} // position (tracker)
		errno = 0;
		ListenSockets[0] = m_DataSocket;
		ListenSockets[1] = m_TelnetSocket;
		ListenSockets[2] = m_AdminSocket;
		ListenSockets[3] = 0;
		Bytes = m_DataSocket->select ( ListenSockets, 0, m_PlayerExpires );
		if ( Bytes < 0 )
		{	// error
			continue;
		}
		else if (Bytes == 0)
		{
			continue;
		}
		
		if ( ListenSockets[0] > 0 )
		{
			// something on the wire (clients)
			Bytes = m_DataSocket->recvfrom ( Msg,MAX_PACKET_SIZE, 0, &SenderAddress );
			if ( Bytes <= 0 )
			{
				continue;
			}
			m_PacketsReceived++;
			HandlePacket ( ( char* ) &Msg, Bytes, SenderAddress );
		} // DataSocket
		else if ( ListenSockets[1] > 0 )
		{
			// something on the wire (telnet)
			netAddress TelnetAddress;
			m_TelnetReceived++;
			int Fd = m_TelnetSocket->accept ( &TelnetAddress );
			if ( Fd < 0 )
			{
				if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
				{
					SG_LOG ( SG_FGMS, SG_ALERT, "FG_SERVER::Loop() - " << strerror ( errno ) );
				}
				continue;
			}
			st_telnet* t = new st_telnet;
			t->Instance = this;
			t->Fd       = Fd;
			pthread_t th;
			pthread_create ( &th, NULL, &telnet_helper, t );
		} // TelnetSocket
		else if ( ListenSockets[2] > 0 )
		{
			// something on the wire (admin port)
			netAddress AdminAddress;
			m_AdminReceived++;
			int Fd = m_AdminSocket->accept ( &AdminAddress );
			if ( Fd < 0 )
			{
				if ( ( errno != EAGAIN ) && ( errno != EPIPE ) )
				{
					SG_LOG ( SG_FGMS, SG_ALERT, "FG_SERVER::Loop() - " << strerror ( errno ) );
				}
				continue;
			}
			SG_LOG ( SG_FGMS, SG_ALERT, "FG_SERVER::Loop() - new Admin connection from "
			  << AdminAddress.getHost());
			st_telnet* t = new st_telnet;
			t->Instance = this;
			t->Fd       = Fd;
			pthread_t th;
			pthread_create ( &th, NULL, &admin_helper, t );
		} // TelnetSocket
	}
	return ( 0 );
} // FG_SERVER::Loop()

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Set listening port for incoming clients
 */
void
FG_SERVER::SetDataPort( int Port)
{
	if ( Port != m_ListenPort )
	{
		m_ListenPort = Port;		m_ReinitData   = true;
		m_TelnetPort = m_ListenPort+1;	m_ReinitTelnet = true;
		m_AdminPort  = m_ListenPort+2;	m_ReinitAdmin  = true;
	}
} // FG_SERVER::SetPort ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set listening port for telnets
 */
void
FG_SERVER::SetTelnetPort( int Port )
{
	if ( m_TelnetPort != Port )
	{
		m_TelnetPort = Port;
		m_ReinitTelnet = true;
	}
} // FG_SERVER::SetTelnetPort ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set listening port for admin connections
 */
void
FG_SERVER::SetAdminPort( int Port )
{
	if ( m_AdminPort != Port )
	{
		m_AdminPort = Port;
		m_ReinitAdmin = true;
	}
} // FG_SERVER::SetAdminPort ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set User for admin connections
 */
void
FG_SERVER::SetAdminUser( string User )
{
	m_AdminUser = User;
} // FG_SERVER::SetAdminUser ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set Password for admin connections
 */
void
FG_SERVER::SetAdminPass( string Pass )
{
	m_AdminPass = Pass;
} // FG_SERVER::SetAdminPass ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set enable password for admin connections
 */
void
FG_SERVER::SetAdminEnable( string Enable )
{
	m_AdminEnable = Enable;
} // FG_SERVER::SetAdminPass ( unsigned int iPort )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set time in seconds. if no packet arrives from a client
 *        within this time, the connection is dropped.
 */
void
FG_SERVER::SetPlayerExpires( int Seconds )
{
	m_PlayerExpires = Seconds;
} // FG_SERVER::SetPlayerExpires ( int iSeconds )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set nautical miles two players must be apart to be out of reach
 */
void
FG_SERVER::SetOutOfReach( int OutOfReach)
{
	m_PlayerIsOutOfReach = OutOfReach;
} // FG_SERVER::SetOutOfReach ( int iOutOfReach )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set the default loglevel
 */
void
FG_SERVER::SetLog( int Facility, int Priority)
{
	sglog().setLogLevels ( (sgDebugClass) Facility, (sgDebugPriority) Priority );
} // FG_SERVER::SetLoglevel ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Set the logfile
 */
void
FG_SERVER::SetLogfile( const std::string& LogfileName )
{
	m_LogFileName = LogfileName;
	SG_LOG ( SG_FGMS, SG_ALERT,"# using logfile " << m_LogFileName );
	if ( m_LogFile.is_open() )
	{
		m_LogFile.close ();
	}
	m_LogFile.open ( m_LogFileName.c_str(), ios::out|ios::app );
	sglog().enable_with_date ( true );
	sglog().set_output ( m_LogFile );
} // FG_SERVER::SetLogfile ( const std::string &LogfileName )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set if we are running as a Hubserver
 */
void
FG_SERVER::SetHub( bool IamHUB)
{
	m_IamHUB = IamHUB;
} // FG_SERVER::SetHub ( int iLoglevel )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Set the server name
 */
void
FG_SERVER::SetServerName( const std::string& ServerName )
{
	m_ServerName = ServerName;
} // FG_SERVER::SetServerName ( const std::string &ServerName )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set the address this server listens on
 */
void
FG_SERVER::SetBindAddress( const std::string& BindAddress )
{
	m_BindAddress = BindAddress;
} // FG_SERVER::SetBindAddress ( const std::string &BindAddress )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Set the external address this server on
 */
void
FG_SERVER::Setdomain( const std::string& domain )
{
	m_domain = domain;
} // FG_SERVER::Setdomain ( const std::string &domain )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Close sockets, logfile etc.
 */
void
FG_SERVER::Done()
{
	if ( ! m_IsParent )
	{
		return;
	}
	Show_Stats();   // 20150619:0.11.9: Add stats to the LOG on exit
	SG_LOG ( SG_FGMS, SG_ALERT, "FG_SERVER::Done() - exiting" );
	m_LogFile.close();
	if ( m_Listening == false )
	{
		return;
	}
	if ( m_TelnetSocket )
	{
		m_TelnetSocket->close();
		delete m_TelnetSocket;
		m_TelnetSocket = 0;
	}
	if ( m_AdminSocket )
	{
		m_AdminSocket->close();
		delete m_AdminSocket;
		m_AdminSocket = 0;
	}
	if ( m_DataSocket )
	{
		m_DataSocket->close();
		delete m_DataSocket;
		m_DataSocket = 0;
	}
	CloseTracker ();
	m_PlayerList.Unlock ();		m_PlayerList.Clear ();
	m_RelayList.Unlock ();		m_RelayList.Clear ();
	m_CrossfeedList.Unlock ();	m_CrossfeedList.Clear ();
	m_BlackList.Unlock ();		m_BlackList.Clear ();
	m_RelayMap.clear ();	// clear(): is a std::map (NOT a FG_List)
	m_Listening = false;
} // FG_SERVER::Done()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Updates the remote tracker web server
 */
int
FG_SERVER::UpdateTracker( const string& Name,const string& Passwd,const string& Modelname, const time_t Timestamp,const int type)
{
	char            TimeStr[100];
	FG_Player	CurrentPlayer;
	Point3D         PlayerPosGeod;
	string          Aircraft;
	string          Message;
	tm*              tm;
	if ( ! m_IsTracked || ( Name == "mpdummy" ) )
	{
		return ( 1 );
	}
	// Creates the UTC time string
	tm = gmtime ( & Timestamp );
	sprintf (
	        TimeStr,
	        "%04d-%02d-%02d %02d:%02d:%02d",
	        tm->tm_year+1900,
	        tm->tm_mon+1,
	        tm->tm_mday,
	        tm->tm_hour,
	        tm->tm_min,
	        tm->tm_sec
	);
	// Edits the aircraft name string
	size_t Index = Modelname.rfind ( "/" );
	if ( Index != string::npos )
	{
		Aircraft = Modelname.substr ( Index + 1 );
	}
	else
	{
		Aircraft = Modelname;
	}
	Index = Aircraft.find ( ".xml" );
	if ( Index != string::npos )
	{
		Aircraft.erase ( Index );
	}
	// Creates the message
	if ( type == CONNECT )
	{
		Message  = "CONNECT ";
		Message += Name;
		Message += " ";
		Message += Passwd;
		Message += " ";
		Message += Aircraft;
		Message += " ";
		Message += TimeStr;
		// queue the message
		m_Tracker->AddMessage (Message);
#ifdef ADD_TRACKER_LOG
		write_msg_log ( Message.c_str(), Message.size(), ( char* ) "IN: " ); // write message log
#endif // #ifdef ADD_TRACKER_LOG
		m_TrackerConnect++; // count a CONNECT message queued
		return ( 0 );
	}
	else if ( type == DISCONNECT )
	{
		Message  = "DISCONNECT ";
		Message += Name;
		Message += " ";
		Message += Passwd;
		Message += " ";
		Message += Aircraft;
		Message += " ";
		Message += TimeStr;
		// queue the message
		m_Tracker->AddMessage (Message);
#ifdef ADD_TRACKER_LOG
		write_msg_log ( Message.c_str(), Message.size(), ( char* ) "IN: " ); // write message log
#endif // #ifdef ADD_TRACKER_LOG
		m_TrackerDisconnect++; // count a DISCONNECT message queued
		return ( 0 );
	}
	// we only arrive here if type!=CONNECT and !=DISCONNECT
	Message = "";
    float heading, pitch, roll;
	for (size_t i = 0; i < m_PlayerList.Size(); i++)
	{
		CurrentPlayer = m_PlayerList[i];
		if (CurrentPlayer.ID == FG_ListElement::NONE_EXISTANT)
			continue;
		if ((CurrentPlayer.IsLocal) && (CurrentPlayer.HasErrors == false))
		{
			if(i!=0)
				Message += "\n";
			sgCartToGeod ( CurrentPlayer.LastPos, PlayerPosGeod );
			Message +=  "POSITION ";
			Message += CurrentPlayer.Name;
			Message += " ";
			Message += CurrentPlayer.Passwd;
			Message += " ";
			Message += NumToStr ( PlayerPosGeod[Lat], 6 ) +" "; //lat
			Message += NumToStr ( PlayerPosGeod[Lon], 6 ) +" "; //lon
			Message += NumToStr ( PlayerPosGeod[Alt], 6 ) +" "; //alt
            euler_get(PlayerPosGeod[Lat], PlayerPosGeod[Lon],
                CurrentPlayer.LastOrientation[X], CurrentPlayer.LastOrientation[Y], CurrentPlayer.LastOrientation[Z],
                &heading, &pitch, &roll );
			Message += NumToStr ( heading, 6 ) +" ";
			Message += NumToStr ( pitch,   6 ) +" ";
			Message += NumToStr ( roll,    6 ) +" ";
			Message += TimeStr;
			// queue the message
		}
	} // while
	m_Tracker->AddMessage (Message);
	m_TrackerPosition++; // count a POSITION messge queued
	Message.erase ( 0 );
	return ( 0 );
} // UpdateTracker (...)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Cleanly closes the tracker
 */
void
FG_SERVER::CloseTracker()
{
	if ( m_IsTracked )
	{
		if ( m_Tracker )
		{
			m_Tracker->WantExit = true;
			pthread_cond_signal ( &m_Tracker->condition_var );  // wake up the worker
			pthread_join (m_Tracker->GetThreadID(), 0);
		}
		m_Tracker = 0;
		m_IsTracked = false;
	}
} // CloseTracker ( )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Decide whether the relay is interested in full rate updates.
 * @see \ref server_out_of_reach config.
 * @param Relay
 * @param SendingPlayer
 * @retval true is within range
 */
bool
FG_SERVER::IsInRange( FG_ListElement& Relay, FG_Player& SendingPlayer )
{
	FG_Player CurrentPlayer;
	size_t	 Cnt;
	Cnt = m_PlayerList.Size ();
	for (size_t i = 0; i < Cnt; i++)
	{
		CurrentPlayer = m_PlayerList[i];
		if (CurrentPlayer.ID == FG_ListElement::NONE_EXISTANT)
			continue;
		if ( ( CurrentPlayer.Address == Relay.Address )
		&& ( Distance ( SendingPlayer.LastPos, CurrentPlayer.LastPos ) <= m_PlayerIsOutOfReach ) )
		{
			return true;
		}
	}
	return false;
} // FG_SERVER::IsInRange()
//////////////////////////////////////////////////////////////////////

