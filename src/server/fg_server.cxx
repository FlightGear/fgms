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

#define FGMS_USE_THREADS

//////////////////////////////////////////////////////////////////////
//
//      Server for FlightGear
//      (c) 2005-2012 Oliver Schroeder
//      and contributors (see AUTHORS)
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

/* From netSocket.cxx */
#ifndef INADDR_NONE
  #define INADDR_NONE ((unsigned long)-1)
#endif

#include "fg_server.hxx"    /* includes pthread.h */
#include "common.h"
#include "typcnvt.hxx"

#ifdef _MSC_VER
  #include <conio.h> // for _kbhit(), _getch
  typedef int pid_t;
#ifndef MSG_NOSIGNAL
  #define MSG_NOSIGNAL 0
#endif
#else
  cDaemon Myself;
#endif

bool    RunAsDaemon = false;
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
  static char *msg_log = (char *)DEF_MESSAGE_LOG;
  static FILE *msg_file = NULL;
#endif // #ifdef ADD_TRACKER_LOG

#if _MSC_VER
  static char * exit_file   = (char *)DEF_EXIT_FILE;  // "fgms_exit"
  static char * reset_file  = (char *)DEF_RESET_FILE; // "fgms_reset"
  static char * stat_file   = (char *)DEF_STAT_FILE;  // "fgms_stat"
#else // !_MSC_VER
  static char * exit_file   = (char *)"/tmp/" DEF_EXIT_FILE;
  static char * reset_file  = (char *)"/tmp/" DEF_RESET_FILE;
  static char * stat_file   = (char *)"/tmp/" DEF_STAT_FILE;
#endif // _MSC_VER y/n

#ifdef USE_TRACKER_PORT
  pthread_mutex_t msg_mutex     = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t  condition_var = PTHREAD_COND_INITIALIZER;
  vMSG msg_queue; // queue for messages
#endif // #ifdef USE_TRACKER_PORT

#ifdef ADD_TRACKER_LOG
static void write_time_string(FILE *file)
{
  time_t Timestamp = time(0);
  char TimeStr[100];
  tm  *tm;
  // Creates the UTC time string
  tm = gmtime (& Timestamp);
  int len = sprintf (
        TimeStr,
        "%04d-%02d-%02d %02d:%02d:%02d: ",
        tm->tm_year+1900,
        tm->tm_mon+1,
        tm->tm_mday,
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec );
  fwrite(TimeStr,1,len,file);
}
void write_msg_log(const char *msg, int len, char *src)
{
  if (msg_file == NULL)
  {
    msg_file = fopen(msg_log,"ab");
    if (!msg_file)
    {
      printf("ERROR: Failed to OPEN/append %s log file!\n", msg_log);
      msg_file = (FILE *)-1;
    }
  }
  if (len && msg_file && (msg_file != (FILE *)-1))
  {
    write_time_string(msg_file);
    if (src && strlen(src))
    {
      fwrite(src,1,strlen(src),msg_file);
    }
    int wtn = (int)fwrite(msg,1,len,msg_file);
    if (wtn != len)
    {
      fclose(msg_file);
      msg_file = (FILE *)-1;
      printf("ERROR: Failed to WRITE %d != %d to %s log file!\n", wtn, len, msg_log);
    }
    else
    {
      if (msg[len-1] != '\n')
      {
        fwrite((char *)"\n",1,1,msg_file);
      }
      fflush(msg_file); // push to disk now
    }
  }
}
#endif // #ifdef ADD_TRACKER_LOG

//////////////////////////////////////////////////////////////////////
//
//      Initilize to standard values
//
//////////////////////////////////////////////////////////////////////
FG_SERVER::FG_SERVER
()
{
  typedef union
  {
    uint32_t    complete;
    int16_t     High;
    int16_t     Low;
  } converter;
  converter*    tmp;
  m_Initialized         = true; // Init() will do it
  m_ReinitData          = true; // init the data port
  m_ReinitTelnet        = true; // init the telnet port
  m_ListenPort          = 5000; // port for client connections
  m_PlayerExpires       = 10; // standard expiration period
  m_Listening           = false;
  m_Loglevel            = SG_INFO;
  m_DataSocket          = 0;
  m_TelnetPort          = m_ListenPort+1;
  m_NumMaxClients       = 0;
  m_PlayerIsOutOfReach  = 100;  // standard 100 nm
  m_NumCurrentClients   = 0;
  m_IsParent            = false;
  m_MaxClientID         = 0;
  m_ServerName          = "* Server *";
  m_BindAddress         = "";
  tmp                   = (converter*) (& PROTO_VER);
  m_ProtoMinorVersion   = tmp->High;
  m_ProtoMajorVersion   = tmp->Low;
  m_LogFileName         = DEF_SERVER_LOG; // "fg_server.log";
  //wp                  = fopen("wp.txt", "w");
  m_BlackList           = map<uint32_t, bool>();
  m_RelayMap            = map<uint32_t, string>();
  m_IsTracked           = false; // off until config file read
  m_Tracker             = 0; // no tracker yet
  m_UpdateSecs          = DEF_UPDATE_SECS;
  // clear stats - should show what type of packet was received
  m_PacketsReceived     = 0;
  m_TelnetReceived      = 0;
  m_BlackRejected       = 0;  // in black list
  m_PacketsInvalid      = 0;  // invalid packet
  m_UnknownRelay        = 0;  // unknown relay
  m_RelayMagic          = 0;  // relay magic packet
  m_PositionData        = 0;  // position data packet
  m_NotPosData          = 0;
  // clear totals
  mT_PacketsReceived    = 0;
  mT_BlackRejected      = 0;
  mT_PacketsInvalid     = 0;
  mT_UnknownRelay       = 0;
  mT_PositionData       = 0;
  mT_TelnetReceived     = 0;
  mT_RelayMagic         = 0;
  mT_NotPosData         = 0;
  m_CrossFeedFailed     = 0;
  m_CrossFeedSent       = 0;
  mT_CrossFeedFailed    = 0;
  mT_CrossFeedSent      = 0;
  m_TrackerConnect      = 0;
  m_TrackerDisconnect   = 0;
  m_TrackerPostion      = 0; // Tracker messages queued
  pthread_mutex_init( &m_PlayerMutex, 0 );
} // FG_SERVER::FG_SERVER()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      standard destructor
//
//////////////////////////////////////////////////////////////////////
FG_SERVER::~FG_SERVER
()
{
  Done();
} // FG_SERVER::~FG_SERVER()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      basic initialization
//
//////////////////////////////////////////////////////////////////////
int
FG_SERVER::Init
()
{
  //////////////////////////////////////////////////
  //      if we are already initialized, close
  //      all connections and re-init all
  //      variables
  //////////////////////////////////////////////////
  if (!m_LogFile)
  {
    m_LogFile.open (m_LogFileName.c_str(), ios::out|ios::app);
    sglog().setLogLevels( SG_ALL, SG_INFO );
    sglog().enable_with_date (true);
    sglog().set_output(m_LogFile);
  }
  if (m_Initialized == true)
  {
    if (m_Listening)
    {
      Done();
    }
    m_Initialized       = false;
    m_Listening         = false;
    m_DataSocket        = 0;
    m_NumMaxClients     = 0;
    m_NumCurrentClients = 0;
  }
  if (m_ReinitData || m_ReinitTelnet)
  {
    netInit ();
  }
  if (m_ReinitData)
  {
    if (m_DataSocket)
    {
      delete m_DataSocket;
      m_DataSocket = 0;
    }
    m_DataSocket = new netSocket();
    if (m_DataSocket->open (false) == 0)    // UDP-Socket
    {
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Init() - "
        << "failed to create listener socket");
      return (ERROR_CREATE_SOCKET);
    }
    m_DataSocket->setBlocking (false);
    m_DataSocket->setSockOpt (SO_REUSEADDR, true);
    if (m_DataSocket->bind (m_BindAddress.c_str(), m_ListenPort) != 0)
    {
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Init() - "
        << "failed to bind to port " << m_ListenPort);
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "already in use?");
      return (ERROR_COULDNT_BIND);
    }
    m_ReinitData = false;
  }
  if (m_ReinitTelnet)
  {
    if (m_TelnetSocket)
    {
      delete m_TelnetSocket;
      m_TelnetSocket = 0;
    }
    m_TelnetSocket = 0;
    if (m_TelnetPort != 0)
    {
      m_TelnetSocket = new netSocket;
      if (m_TelnetSocket->open (true) == 0)   // TCP-Socket
      {
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Init() - "
          << "failed to create telnet socket");
        return (ERROR_CREATE_SOCKET);
      }
      m_TelnetSocket->setBlocking (false);
      m_TelnetSocket->setSockOpt (SO_REUSEADDR, true);
      if (m_TelnetSocket->bind (m_BindAddress.c_str(), m_TelnetPort) != 0)
      {
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Init() - "
          << "failed to bind to port " << m_TelnetPort);
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "already in use?");
        return (ERROR_COULDNT_BIND);
      }
      if (m_TelnetSocket->listen (MAX_TELNETS) != 0)
      {
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Init() - "
          << "failed to listen to telnet port");
        return (ERROR_COULDNT_LISTEN);
      }
    }
    m_ReinitTelnet = false;
  }
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# This is " << m_ServerName);
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# FlightGear Multiplayer Server v"
    << VERSION << " started");
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# using protocol version v"
    << m_ProtoMajorVersion << "." << m_ProtoMinorVersion
    << " (LazyRelay enabled)");
  if (m_BindAddress != "")
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT,"# listening on " << m_BindAddress);
  }
  SG_ALERT (SG_SYSTEMS, SG_ALERT,"# listening to port " << m_ListenPort);
  SG_ALERT (SG_SYSTEMS, SG_ALERT,"# telnet port " << m_TelnetPort);
  SG_ALERT (SG_SYSTEMS, SG_ALERT,"# using logfile " << m_LogFileName);
  if (m_IamHUB)
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "# I am a HUB Server");
  }
  if (m_IsTracked)
  {
    if ( m_Tracker->InitTracker(&m_TrackerPID) )
    {
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "# InitTracker FAILED! Disabling tracker!");
            m_IsTracked = false;
    }
    else
    {
#ifdef USE_TRACKER_PORT
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "# tracked to "
        << m_Tracker->GetTrackerServer ()
        << ":" << m_Tracker->GetTrackerPort ()
        << ", using a thread." );
#else // #ifdef USE_TRACKER_PORT
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "# tracked to "
        << m_Tracker->GetTrackerServer ()
        << ":" << m_Tracker->GetTrackerPort ());
#endif // #ifdef USE_TRACKER_PORT y/n
    }
  }
  else
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "# tracking is disabled.");
  }
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# I have " << m_RelayList.size() << " relays");
  mT_RelayListIt CurrentRelay = m_RelayList.begin();
  while (CurrentRelay != m_RelayList.end())
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "# relay " << CurrentRelay->Name);
    CurrentRelay++;
  }
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# I have " << m_CrossfeedList.size() << " crossfeeds");
  mT_RelayListIt CurrentCrossfeed = m_CrossfeedList.begin();
  while (CurrentCrossfeed != m_CrossfeedList.end())
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "# crossfeed " << CurrentCrossfeed->Name
      << ":" << CurrentCrossfeed->Address.getPort());
    CurrentCrossfeed++;
  }
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# I have " << m_BlackList.size() << " blacklisted IPs");
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# Files: exit=[" << exit_file << "] stat=[" << stat_file << "]");
  m_Listening = true;
  return (SUCCESS);
} // FG_SERVER::Init()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      do anything necessary to (re-) init the server
//      used to handle kill -HUP
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::PrepareInit
()
{
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# caught SIGHUP, doing reinit!");
  m_RelayList.clear ();
  m_BlackList.clear ();
  m_RelayMap.clear ();
  m_CrossfeedList.clear ();
} // FG_SERVER::PrepareInit ()
//////////////////////////////////////////////////////////////////////

static void*
telnet_helper
(
  void *context
)
{
  pthread_detach (pthread_self());
  st_telnet* t = reinterpret_cast<st_telnet*> (context);
  FG_SERVER *tmp_server = t->Instance;
  tmp_server->HandleTelnet(t->Fd);
  delete t;
  return 0;
}

//////////////////////////////////////////////////////////////////////
//
//      handle a telnet session
//      if a telnet connection is opened, this method outputs a list
//      of all known clients.
//
//////////////////////////////////////////////////////////////////////
void*
FG_SERVER::HandleTelnet
(
  int Fd
)
{
  errno = 0;
  string          Message;
  Point3D         PlayerPosGeod;  // Geodetic Coordinates
  FG_Player CurrentPlayer;
  netSocket       NewTelnet;
  unsigned int  it;
  NewTelnet.setHandle (Fd);
  errno = 0;
  //////////////////////////////////////////////////
  //
  //      create the output message
  //      header
  //
  //////////////////////////////////////////////////
  Message  = "# This is " + m_ServerName;
  Message += "\n";
  Message += "# FlightGear Multiplayer Server v" + string(VERSION);
  Message += "\n";
  Message += "# using protocol version v";
  Message += NumToStr (m_ProtoMajorVersion, 0);
  Message += "." + NumToStr (m_ProtoMinorVersion, 0);
  Message += " (LazyRelay enabled)";
  Message += "\n";
  if ( m_IsTracked )
  {
    Message += "# This server is tracked: ";
    Message += m_Tracker->GetTrackerServer();
    Message += "\n";
  }
  if (NewTelnet.send (Message.c_str(),Message.size(), MSG_NOSIGNAL) < 0)
  {
    if ((errno != EAGAIN) && (errno != EPIPE))
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_SERVER::HandleTelnet() - " << strerror (errno));
    }
    return (0);
  }
  pthread_mutex_lock (& m_PlayerMutex);
  Message  = "# "+ NumToStr (m_PlayerList.size(), 0);
  pthread_mutex_unlock (& m_PlayerMutex);
  Message += " pilot(s) online\n";
  if (NewTelnet.send (Message.c_str(),Message.size(), MSG_NOSIGNAL) < 0)
  {
    if ((errno != EAGAIN) && (errno != EPIPE))
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_SERVER::HandleTelnet() - " << strerror (errno));
    }
    return (0);
  }
  //////////////////////////////////////////////////
  //
  //      create list of players
  //
  //////////////////////////////////////////////////
  it = 0;
  for (;;)
  {
    pthread_mutex_lock (& m_PlayerMutex);
    if (it < m_PlayerList.size())
    {
      CurrentPlayer = m_PlayerList[it]; 
      it++;
    }
    else
    {
      pthread_mutex_unlock (& m_PlayerMutex);
      break;
    }
    pthread_mutex_unlock (& m_PlayerMutex);
    sgCartToGeod (CurrentPlayer.LastPos, PlayerPosGeod);
    Message = CurrentPlayer.Callsign + "@";
    if (CurrentPlayer.IsLocal)
    {
      Message += "LOCAL: ";
    }
    else
    {
      mT_RelayMapIt Relay = m_RelayMap.find(CurrentPlayer.Address.getIP());
      if (Relay != m_RelayMap.end())
      {
        Message += Relay->second + ": ";
      }
      else
      {
        Message += CurrentPlayer.Origin + ": ";
      }
    }
    if (CurrentPlayer.Error != "")
    {
      Message += CurrentPlayer.Error + " ";
    }
    Message += NumToStr (CurrentPlayer.LastPos[X], 6)+" ";
    Message += NumToStr (CurrentPlayer.LastPos[Y], 6)+" ";
    Message += NumToStr (CurrentPlayer.LastPos[Z], 6)+" ";
    Message += NumToStr (PlayerPosGeod[Lat], 6)+" ";
    Message += NumToStr (PlayerPosGeod[Lon], 6)+" ";
    Message += NumToStr (PlayerPosGeod[Alt], 6)+" ";
    Message += NumToStr (CurrentPlayer.LastOrientation[X], 6)+" ";
    Message += NumToStr (CurrentPlayer.LastOrientation[Y], 6)+" ";
    Message += NumToStr (CurrentPlayer.LastOrientation[Z], 6)+" ";
    Message += CurrentPlayer.ModelName;
    Message += "\n";
    if (NewTelnet.send (Message.c_str(),Message.size(), MSG_NOSIGNAL) < 0)
    {
      if ((errno != EAGAIN) && (errno != EPIPE))
      {
        SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_SERVER::HandleTelnet() - " << strerror (errno));
      }
      return (0);
    }
  }
  NewTelnet.close ();
  return (0);
} // FG_SERVER::HandleTelnet ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      if we receive bad data from a client, we add the client to
//      the internal list anyway, but mark them as bad. But first 
//      we look if it isn't already there.
//      Send an error message to the bad client.
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::AddBadClient
(
  const netAddress& Sender,
  string&     ErrorMsg,
  bool      IsLocal
)
{
  string                  Message;
  FG_Player               NewPlayer;
  mT_PlayerListIt         CurrentPlayer;

  CurrentPlayer = m_PlayerList.begin();
  //////////////////////////////////////////////////
  //      see, if we already know the client
  //////////////////////////////////////////////////
  while (CurrentPlayer != m_PlayerList.end())
  {
    if (CurrentPlayer->Address.getIP() == Sender.getIP())
    {
      CurrentPlayer->Timestamp = time (0);
      return;
    }
    CurrentPlayer++;
  }
  //////////////////////////////////////////////////
  //      new client, send an error message
  //////////////////////////////////////////////////
  m_MaxClientID++;
  NewPlayer.Callsign      = "* Bad Client *";
  NewPlayer.ModelName     = "* unknown *";
  NewPlayer.Timestamp     = time(0);
  NewPlayer.JoinTime      = NewPlayer.Timestamp;
  NewPlayer.Origin        = Sender.getHost ();
  NewPlayer.Address       = Sender;
  NewPlayer.IsLocal       = IsLocal;
  NewPlayer.HasErrors     = true;
  NewPlayer.Error         = ErrorMsg;
  NewPlayer.ClientID      = m_MaxClientID;
  NewPlayer.PktsReceivedFrom      = 0;
  NewPlayer.PktsSentTo            = 0;
  NewPlayer.PktsForwarded         = 0;
  NewPlayer.LastRelayedToInactive = 0;
  SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_SERVER::AddBadClient() - " << ErrorMsg);
  Message = "bad client connected: ";
  Message += Sender.getHost() + string(": ");
  Message += ErrorMsg;
  CreateChatMessage (NewPlayer.ClientID, Message);
  pthread_mutex_lock (& m_PlayerMutex);
  m_PlayerList.push_back (NewPlayer);
  m_NumCurrentClients++;
  pthread_mutex_unlock (& m_PlayerMutex);
} // FG_SERVER::AddBadClient ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      insert a new client into internal list
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::AddClient
(
  const netAddress& Sender,
  char* Msg
)
{
  time_t          Timestamp;
  //uint32_t        MsgLen;
  //uint32_t        MsgId;
  uint32_t        MsgMagic;
  string          Message;
  string          Origin;
  T_MsgHdr*       MsgHdr;
  T_PositionMsg*  PosMsg;
  FG_Player       NewPlayer;
  bool    IsLocal;

  Timestamp           = time(0);
  MsgHdr              = (T_MsgHdr *) Msg;
  PosMsg              = (T_PositionMsg *) (Msg + sizeof(T_MsgHdr));
  //MsgId               = XDR_decode<uint32_t> (MsgHdr->MsgId);
  //MsgLen              = XDR_decode<uint32_t> (MsgHdr->MsgLen);
  MsgMagic            = XDR_decode<uint32_t> (MsgHdr->Magic);
  IsLocal             = true;
  if (MsgMagic == RELAY_MAGIC) // not a local client
  {
    IsLocal = false;
  }
  NewPlayer.Callsign  = MsgHdr->Callsign;
  NewPlayer.Passwd    = "test"; //MsgHdr->Passwd;
  NewPlayer.ModelName = "* unknown *";
  NewPlayer.Timestamp = Timestamp;
  NewPlayer.JoinTime  = Timestamp;
  NewPlayer.Origin    = Sender.getHost ();
  NewPlayer.HasErrors = false;
  NewPlayer.Address   = Sender;
  NewPlayer.IsLocal   = IsLocal;
  NewPlayer.LastPos.clear();
  NewPlayer.LastOrientation.clear();
  NewPlayer.PktsReceivedFrom = 0;
  NewPlayer.PktsSentTo       = 0;
  NewPlayer.PktsForwarded    = 0;
  NewPlayer.LastRelayedToInactive = 0;
  NewPlayer.LastPos.Set (
    XDR_decode64<double> (PosMsg->position[X]),
    XDR_decode64<double> (PosMsg->position[Y]),
    XDR_decode64<double> (PosMsg->position[Z])
  );
  NewPlayer.LastOrientation.Set (
    XDR_decode<float> (PosMsg->orientation[X]),
    XDR_decode<float> (PosMsg->orientation[Y]),
    XDR_decode<float> (PosMsg->orientation[Z])
  );
  NewPlayer.ModelName = PosMsg->Model;
  m_MaxClientID++;
  NewPlayer.ClientID = m_MaxClientID;
  pthread_mutex_lock (& m_PlayerMutex);
  m_PlayerList.push_back (NewPlayer);
  pthread_mutex_unlock (& m_PlayerMutex);
  m_NumCurrentClients++;
  if (m_NumCurrentClients > m_NumMaxClients)
  {
    m_NumMaxClients = m_NumCurrentClients;
  }
  if (IsLocal)
  {
    Message  = "Welcome to ";
    Message += m_ServerName;
    CreateChatMessage (NewPlayer.ClientID , Message);
    Message = "this is version v" + string(VERSION);
    Message += " (LazyRelay enabled)";
    CreateChatMessage (NewPlayer.ClientID , Message);
    Message  ="using protocol version v";
    Message += NumToStr (m_ProtoMajorVersion, 0);
    Message += "." + NumToStr (m_ProtoMinorVersion, 0);
    if (m_IsTracked)
    {
      Message += "This server is tracked.";
    }
    CreateChatMessage (NewPlayer.ClientID , Message);
    UpdateTracker (NewPlayer.Callsign, NewPlayer.Passwd,
      NewPlayer.ModelName, NewPlayer.Timestamp, CONNECT);
  }
  Message  = NewPlayer.Callsign;
  Message += " is now online, using ";
  CreateChatMessage (0, Message);
  Message  = NewPlayer.ModelName;
  CreateChatMessage (0, Message);
  Origin  = NewPlayer.Origin;
  if (IsLocal)
  {
    Message = "New LOCAL Client: ";
  }
  else
  {
    Message = "New REMOTE Client: ";
    mT_RelayMapIt Relay = m_RelayMap.find(NewPlayer.Address.getIP());
    if (Relay != m_RelayMap.end())
    {
      Origin = Relay->second;
    }
  }
  SG_LOG (SG_SYSTEMS, SG_INFO, Message
    << NewPlayer.Callsign << " "
    << Origin << ":" << Sender.getPort()
    << " (" << NewPlayer.ModelName << ")"
    << " current clients: "
    << m_NumCurrentClients << " max: " << m_NumMaxClients
  );
} // FG_SERVER::AddClient()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      insert a new relay server into internal list
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::AddRelay
(
  const string & Server,
  int Port
)
{
  mT_Relay        NewRelay;
  unsigned int    IP;

  NewRelay.Name = Server;
  NewRelay.Address.set ((char*) Server.c_str(), Port);
  IP = NewRelay.Address.getIP();
  if ( IP != INADDR_ANY && IP != INADDR_NONE )
  {
    m_RelayList.push_back (NewRelay);
    string S; unsigned I;
    I = NewRelay.Name.find (".");
    if (I != string::npos)
    {
      S = NewRelay.Name.substr (0, I);
    }
    else
    {
      S = NewRelay.Name;
    }
    m_RelayMap[NewRelay.Address.getIP()] = S;
  }
} // FG_SERVER::AddRelay()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      insert a new crossfeed server into internal list
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::AddCrossfeed
(
  const string & Server,
  int Port
)
{
  mT_Relay        NewRelay;
  unsigned int    IP;
  string s = Server;
#ifdef _MSC_VER
  if (s == "localhost")
  {
    s = "127.0.0.1";
  }
#endif // _MSC_VER
  NewRelay.Name = s;
  NewRelay.Address.set ((char*) s.c_str(), Port);
  IP = NewRelay.Address.getIP();
  if ( IP != INADDR_ANY && IP != INADDR_NONE )
  {
    m_CrossfeedList.push_back (NewRelay);
  }
  else
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "AddCrossfeed: FAILED on " << Server << ", port " << Port);
  }
} // FG_SERVER::AddCrossfeed()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      adds a tracking server
//
//////////////////////////////////////////////////////////////////////
int
FG_SERVER::AddTracker
(
  const string & Server,
  int Port,
  bool IsTracked
)
{
  m_IsTracked     = IsTracked;
#ifndef NO_TRACKER_PORT
#ifdef USE_TRACKER_PORT
  if ( m_Tracker )
  {
    delete m_Tracker;
  }
  m_Tracker = new FG_TRACKER(Port,Server,0);
#else // !#ifdef USE_TRACKER_PORT
  if ( m_Tracker )
  {
    msgctl(m_ipcid,IPC_RMID,NULL);
    delete m_Tracker;
    m_Tracker = 0; // just deleted
  }
  printf("Establishing IPC\n");
  m_ipcid         = msgget(IPC_PRIVATE,IPCPERMS);
  if (m_ipcid <= 0)
  {
    perror("msgget getting ipc id failed");
    return -1;
  }
  m_Tracker = new FG_TRACKER(Port,Server,m_ipcid);
#endif // #ifdef USE_TRACKER_PORT y/n
#endif // NO_TRACKER_PORT
  return (SUCCESS);
} // FG_SERVER::AddTracker()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      Add an IP to the blacklist
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::AddBlacklist
(
  const string& FourDottedIP
)
{
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "Adding to blacklist: " << FourDottedIP);
  m_BlackList[netAddress(FourDottedIP.c_str(), 0).getIP()] = true;
} // FG_SERVER::AddBlacklist()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      Remove player from list
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::DropClient
(
  mT_PlayerListIt& CurrentPlayer
)
{
  string Origin;

  if (CurrentPlayer->IsLocal) 
  {
    UpdateTracker (CurrentPlayer->Callsign,
      CurrentPlayer->Passwd, 
      CurrentPlayer->ModelName,
      CurrentPlayer->Timestamp,
      DISCONNECT
    );
  }
  mT_RelayMapIt Relay = m_RelayMap.find(CurrentPlayer->Address.getIP());
  if (Relay != m_RelayMap.end())
  {
    Origin = Relay->second;
  }
  else
  {
    Origin = "LOCAL";
  }
  m_NumCurrentClients--;
  SG_LOG (SG_SYSTEMS, SG_INFO, "TTL exceeded, dropping pilot "
    << CurrentPlayer->Callsign << "@" << Origin
    << "  after " << time(0)-CurrentPlayer->JoinTime << " seconds."
    << "  Usage #packets in: " << CurrentPlayer->PktsReceivedFrom
    << " forwarded: " << CurrentPlayer->PktsForwarded
    << " out: " << CurrentPlayer->PktsSentTo
    << ". Current clients: "
    << m_NumCurrentClients << " max: " << m_NumMaxClients
  );
  if (m_NumCurrentClients > 0)
  {
    string Message = "client '";
    Message += CurrentPlayer->Callsign;
    Message += "' has left";
    CreateChatMessage (0, Message);
  }
  pthread_mutex_lock (& m_PlayerMutex);
  CurrentPlayer = m_PlayerList.erase (CurrentPlayer);
  pthread_mutex_unlock (& m_PlayerMutex);
} // FG_SERVER::DropClient ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  create a chat message and put it into the internal message queue
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::CreateChatMessage
(
  int ID,
  string Msg
)
{
  T_MsgHdr        MsgHdr;
  T_ChatMsg       ChatMsg;
  unsigned int    NextBlockPosition = 0;
  char*           Message;
  int             len =  sizeof(T_MsgHdr) + sizeof(T_ChatMsg);

  MsgHdr.Magic            = XDR_encode<uint32_t> (MSG_MAGIC);
  MsgHdr.Version          = XDR_encode<uint32_t> (PROTO_VER);
  MsgHdr.MsgId            = XDR_encode<uint32_t> (CHAT_MSG_ID);
  MsgHdr.MsgLen           = XDR_encode<uint32_t> (len);
  MsgHdr.ReplyAddress     = 0;
  MsgHdr.ReplyPort        = XDR_encode<uint32_t> (m_ListenPort);
  strncpy(MsgHdr.Callsign, "*FGMS*", MAX_CALLSIGN_LEN);
  MsgHdr.Callsign[MAX_CALLSIGN_LEN - 1] = '\0';
  while (NextBlockPosition < Msg.length())
  {
    strncpy (ChatMsg.Text, 
    Msg.substr (NextBlockPosition, MAX_CHAT_MSG_LEN - 1).c_str(),
    MAX_CHAT_MSG_LEN);
    ChatMsg.Text[MAX_CHAT_MSG_LEN - 1] = '\0';
    Message = new char[len];
    memcpy (Message, &MsgHdr, sizeof(T_MsgHdr));
    memcpy (Message + sizeof(T_MsgHdr), &ChatMsg,
    sizeof(T_ChatMsg));
    m_MessageList.push_back (mT_ChatMsg(ID,Message));
    NextBlockPosition += MAX_CHAT_MSG_LEN - 1;
  }
} // FG_SERVER::CreateChatMessage ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      check if the user is black listed.
//
//////////////////////////////////////////////////////////////////////
bool
FG_SERVER::IsBlackListed
(
  const netAddress &SenderAddress
)
{
  if (m_BlackList.find(SenderAddress.getIP()) != m_BlackList.end())
  {
    return (true);
  }
  return (false);
} // FG_SERVER::IsBlackListed ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      check if the sender is a known relay
//
//////////////////////////////////////////////////////////////////////
bool
FG_SERVER::IsKnownRelay
(
  const netAddress &SenderAddress
)
{
  mT_RelayListIt  CurrentRelay = m_RelayList.begin();
  while (CurrentRelay != m_RelayList.end())
  {
    if (CurrentRelay->Address.getIP() == SenderAddress.getIP())
    {
      return (true);
    }
    CurrentRelay++;
  }
  string ErrorMsg;
  ErrorMsg  = SenderAddress.getHost();
  ErrorMsg += " is not a valid relay!";
  AddBlacklist (SenderAddress.getHost());
  SG_LOG (SG_SYSTEMS, SG_ALERT, "UNKNOWN RELAY: " << ErrorMsg);
  return (false);
} // FG_SERVER::IsKnownRelay ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      check if the packet is valid
//
//////////////////////////////////////////////////////////////////////
bool
FG_SERVER::PacketIsValid
(
  int        Bytes,
  T_MsgHdr   *MsgHdr,
  const netAddress &SenderAddress
)
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

  Origin = SenderAddress.getHost();
  MsgMagic = XDR_decode<uint32_t> (MsgHdr->Magic);
  MsgId  = XDR_decode<uint32_t> (MsgHdr->MsgId);
  MsgLen = XDR_decode<uint32_t> (MsgHdr->MsgLen);
  if (Bytes < (int)sizeof(MsgHdr))
  {
    ErrorMsg  = SenderAddress.getHost();
    ErrorMsg += " packet size is too small!";
    AddBadClient (SenderAddress, ErrorMsg, true);
    return (false);
  }
  if ((MsgMagic != MSG_MAGIC) && (MsgMagic != RELAY_MAGIC))
  {
    char m[5];
    memcpy (m, (char*) &MsgMagic, 4);
    m[4] = 0;
    ErrorMsg  = Origin;
    ErrorMsg += " BAD magic number: ";
    ErrorMsg += m;
    AddBadClient (SenderAddress, ErrorMsg, true);
    return (false);
  }
  if (XDR_decode<uint32_t> (MsgHdr->Version) != PROTO_VER)
  {
    MsgHdr->Version = XDR_decode<uint32_t> (MsgHdr->Version);
    ErrorMsg  = Origin;
    ErrorMsg += " BAD protocol version! Should be ";
    converter*    tmp;
    tmp = (converter*) (& PROTO_VER);
    ErrorMsg += NumToStr (tmp->High, 0);
    ErrorMsg += "." + NumToStr (tmp->Low, 0);
    ErrorMsg += " but is ";
    tmp = (converter*) (& MsgHdr->Version);
    ErrorMsg += NumToStr (tmp->Low, 0);
    ErrorMsg += "." + NumToStr (tmp->High, 0);
    AddBadClient (SenderAddress, ErrorMsg, true);
    return (false);
  }
  if (MsgId == POS_DATA_ID) 
  {
    if (MsgLen < sizeof(T_MsgHdr) + sizeof(T_PositionMsg))
    {
      ErrorMsg  = Origin;
      ErrorMsg += " Client sends insufficient position data, ";
      ErrorMsg += "should be ";
      ErrorMsg += NumToStr (sizeof(T_MsgHdr)+sizeof(T_PositionMsg));
      ErrorMsg += " is: " + NumToStr (MsgHdr->MsgLen);
      AddBadClient (SenderAddress, ErrorMsg, true);
      return (false);
    }
  }
  return (true);
} // FG_SERVER::PacketIsValid ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      send any message in m_MessageList to client
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SendChatMessages
(
  mT_PlayerListIt& CurrentPlayer
)
{
  mT_MessageIt  CurrentMessage;

  if ((CurrentPlayer->IsLocal) && (m_MessageList.size()))
  {
    CurrentMessage = m_MessageList.begin();
    while (CurrentMessage != m_MessageList.end())
    {
      if ((CurrentMessage->Target == 0)
      ||  (CurrentMessage->Target == CurrentPlayer->ClientID))
      {
        int len = sizeof(T_MsgHdr) + sizeof(T_ChatMsg);
        m_DataSocket->sendto (CurrentMessage->Msg, len, 0,
          &CurrentPlayer->Address);
      }
      CurrentMessage++;
    }
  }
} // FG_SERVER::SendChatMessages ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      delete internal chat message queue
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::DeleteMessageQueue
()
{
  mT_MessageIt    CurrentMessage;
  if (! m_MessageList.size())
  {
    return;
  }
  CurrentMessage = m_MessageList.begin();
  while (CurrentMessage != m_MessageList.end())
  {
    delete[] CurrentMessage->Msg;
    CurrentMessage = m_MessageList.erase (CurrentMessage);
  }
} // FG_SERVER::DeleteMessageQueue ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      Send message to all crossfeed servers
//  Crossfeed servers receive all traffic without condition,
//  mainly used for testing and debugging
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SendToCrossfeed
(
  char* Msg,
  int Bytes,
  const netAddress & SenderAddress
)
{
  T_MsgHdr*       MsgHdr;
  uint32_t        MsgMagic;
  int             sent;

  MsgHdr    = (T_MsgHdr *) Msg;
  MsgMagic  = MsgHdr->Magic;
  MsgHdr->Magic = XDR_encode<uint32_t> (RELAY_MAGIC);
  // pass on senders address and port to crossfeed server
  MsgHdr->ReplyAddress = XDR_encode<uint32_t> (SenderAddress.getIP());
  MsgHdr->ReplyPort = XDR_encode<uint32_t> (SenderAddress.getPort());
  mT_RelayListIt CurrentCrossfeed = m_CrossfeedList.begin();
  while (CurrentCrossfeed != m_CrossfeedList.end())
  {
    //if (CurrentCrossfeed->Address.getIP() != SenderAddress.getIP())
    //{
      sent = m_DataSocket->sendto(Msg, Bytes, 0, &CurrentCrossfeed->Address);
      if (SERROR(sent))
      {
          PERROR("sendto crossfeed failed!");
        m_CrossFeedFailed++;
      }
      else
      {
        m_CrossFeedSent++;
      }
    //}
    CurrentCrossfeed++;
  }
  MsgHdr->Magic = MsgMagic;  // restore the magic value
} // FG_SERVER::SendToCrossfeed ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      Send message to all relay servers
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SendToRelays
(
  char* Msg,
  int Bytes,
  mT_PlayerListIt& SendingPlayer
)
{
  T_MsgHdr*       MsgHdr;
  uint32_t        MsgMagic;
  unsigned int    PktsForwarded = 0;
  mT_RelayListIt  CurrentRelay;
  time_t          Now;

  if ((! SendingPlayer->IsLocal) && (! m_IamHUB))
  {
    return;
  }
  Now   = time (0);
  MsgHdr    = (T_MsgHdr *) Msg;
  MsgMagic  = XDR_decode<uint32_t> (MsgHdr->Magic);
  MsgHdr->Magic = XDR_encode<uint32_t> (RELAY_MAGIC);
  bool UpdateInactive = (Now - SendingPlayer->LastRelayedToInactive) > UPDATE_INACTIVE_PERIOD;
  if (UpdateInactive)
  {
    SendingPlayer->LastRelayedToInactive = Now;
  }
  CurrentRelay = m_RelayList.begin();
  while (CurrentRelay != m_RelayList.end())
  {
    if (UpdateInactive || IsInRange(*CurrentRelay, *SendingPlayer))
    {
      if (CurrentRelay->Address.getIP() != SendingPlayer->Address.getIP())
      {
        m_DataSocket->sendto(Msg, Bytes, 0, &CurrentRelay->Address);
        PktsForwarded++;
      }
    }
    CurrentRelay++;
  }
  SendingPlayer->PktsForwarded += PktsForwarded;
  MsgHdr->Magic = XDR_encode<uint32_t> (MsgMagic);  // restore the magic value
} // FG_SERVER::SendToRelays ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      look if we know the sending client
//      return:
//       0: Sender is unknown
//       1: Sender is known
//       2: Sender is known, but has a different IP
//
//////////////////////////////////////////////////////////////////////
int
FG_SERVER::SenderIsKnown
(
  const string& SenderCallsign,
  const netAddress &SenderAddress
)
{
  mT_PlayerListIt CurrentPlayer;
  for (CurrentPlayer = m_PlayerList.begin();
       CurrentPlayer != m_PlayerList.end();
       CurrentPlayer++)
  {
    if (CurrentPlayer->Callsign == SenderCallsign)
    {
      if (CurrentPlayer->Address.getIP() == SenderAddress.getIP())
      {
        return (1); // Sender is known
      }
      // Same callsign, but different IP.
      // Quietly ignore this packet.
      return (2);
    }
  }
  // Sender is unkown
  return (0);
} // FG_SERVER::SenderIsKnown ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      handle client connections
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::HandlePacket
(
  char * Msg,
  int Bytes,
  const netAddress &SenderAddress
)
{
  T_MsgHdr*       MsgHdr;
  T_PositionMsg*  PosMsg;
  uint32_t        MsgId;
  uint32_t        MsgMagic;
  time_t          Timestamp;
  Point3D         SenderPosition;
  Point3D         SenderOrientation;
  Point3D         PlayerPosGeod;
  mT_PlayerListIt CurrentPlayer;
  mT_PlayerListIt SendingPlayer;
  unsigned int    PktsForwarded = 0;

  Timestamp = time(0);
  MsgHdr    = (T_MsgHdr *) Msg;
  MsgMagic  = XDR_decode<uint32_t> (MsgHdr->Magic);
  MsgId     = XDR_decode<uint32_t> (MsgHdr->MsgId);
  //////////////////////////////////////////////////
  //
  //  First of all, send packet to all
  //  crossfeed servers.
  //
  //////////////////////////////////////////////////
  SendToCrossfeed (Msg, Bytes, SenderAddress);
  //////////////////////////////////////////////////
  //
  //  Now do the local processing
  //
  //////////////////////////////////////////////////
  if (IsBlackListed (SenderAddress))
  {
    m_BlackRejected++;
    return;
  }
  if (! PacketIsValid (Bytes, MsgHdr, SenderAddress))
  {
    m_PacketsInvalid++;
    return;
  }
  if (MsgMagic == RELAY_MAGIC) // not a local client
  {
    if (! IsKnownRelay (SenderAddress))
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
  if (MsgId == POS_DATA_ID)
  {
    m_PositionData++;
    PosMsg = (T_PositionMsg *) (Msg + sizeof(T_MsgHdr));
    double x = XDR_decode64<double> (PosMsg->position[X]);
    double y = XDR_decode64<double> (PosMsg->position[Y]);
    double z = XDR_decode64<double> (PosMsg->position[Z]);
    if ( (x == 0.0) || (y == 0.0) || (z == 0.0) )
    { // ignore while position is not settled
      return;
    }
    SenderPosition.Set (x, y, z);
    SenderOrientation.Set (
      XDR_decode<float> (PosMsg->orientation[X]),
      XDR_decode<float> (PosMsg->orientation[Y]),
      XDR_decode<float> (PosMsg->orientation[Z])
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
  int ClientInList = SenderIsKnown (MsgHdr->Callsign, SenderAddress);
  if (ClientInList == 0)
  { // unknown, add to the list
    if (MsgId != POS_DATA_ID)
    { // ignore clients until we have a valid position
      return;
    }
    AddClient (SenderAddress, Msg);
  }
  else if (ClientInList == 2)
  { // known, but different IP => ignore
    return;
  }
  //////////////////////////////////////////
  //
  //      send the packet to all clients.
  //      since we are walking through the list,
  //      we look for the sending client, too. if it
  //      is not already there, add it to the list
  //
  //////////////////////////////////////////////////
  MsgHdr->Magic = XDR_encode<uint32_t> (MSG_MAGIC);
  SendingPlayer = m_PlayerList.end();
  CurrentPlayer = m_PlayerList.begin();
  while (CurrentPlayer != m_PlayerList.end())
  {
    //////////////////////////////////////////////////
    //
    //      ignore clients with errors
    //
    //////////////////////////////////////////////////
    if (CurrentPlayer->HasErrors)
    {
      CurrentPlayer++;
      continue;
    }
    //////////////////////////////////////////////////
    //        Sender == CurrentPlayer?
    //////////////////////////////////////////////////
    //  FIXME: if Sender is a Relay,
    //         CurrentPlayer->Address will be
    //         address of Relay and not the client's!
    //         so use a clientID instead
    if (CurrentPlayer->Callsign == MsgHdr->Callsign)
    {
      if (MsgId == POS_DATA_ID)
      {
        CurrentPlayer->LastPos         = SenderPosition;
        CurrentPlayer->LastOrientation = SenderOrientation;
      }
      else
      {
        SenderPosition    = CurrentPlayer->LastPos;
        SenderOrientation = CurrentPlayer->LastOrientation;
      }
      SendingPlayer = CurrentPlayer;
      CurrentPlayer->Timestamp = Timestamp;
      CurrentPlayer->PktsReceivedFrom++;
      CurrentPlayer++;
      continue; // don't send packet back to sender
    }
    //////////////////////////////////////////////////
    //      do not send packets to clients if the
    //      origin is an observer, but do send
    //      chat messages anyway
    //      FIXME: MAGIC = SFGF!
    //////////////////////////////////////////////////
    if ((strncasecmp(MsgHdr->Callsign, "obs", 3) == 0)
    &&  (MsgId != CHAT_MSG_ID))
    {
      return;
    }
    //////////////////////////////////////////////////
    //
    //      do not send packet to clients which
    //      are out of reach.
    //
    //////////////////////////////////////////////////
    if ((Distance (SenderPosition, CurrentPlayer->LastPos) > m_PlayerIsOutOfReach)
    &&  (CurrentPlayer->Callsign.compare (0, 3, "obs", 3) != 0))
    {
      CurrentPlayer++;
      continue;
    }
    //////////////////////////////////////////////////
    //
    //  only send packet to local clients
    //
    //////////////////////////////////////////////////
    if (CurrentPlayer->IsLocal)
    {
      SendChatMessages (CurrentPlayer);
      m_DataSocket->sendto (Msg, Bytes, 0, &CurrentPlayer->Address);
      CurrentPlayer->PktsSentTo++;
      PktsForwarded++;
    }
    CurrentPlayer++;
  }
  if (SendingPlayer == m_PlayerList.end())
  { // player not yet in our list
    // should not happen, but test just in case
    SG_LOG (SG_SYSTEMS, SG_ALERT, "## BAD => "
      << MsgHdr->Callsign << ":" << SenderAddress.getHost()
      << " : " << SenderIsKnown (MsgHdr->Callsign, SenderAddress)
    );
    return;
  }
  DeleteMessageQueue ();
  SendToRelays (Msg, Bytes, SendingPlayer);
} // FG_SERVER::HandlePacket ( char* sMsg[MAX_PACKET_SIZE] )
//////////////////////////////////////////////////////////////////////
void FG_SERVER::Show_Stats(void)
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
    mT_PlayerListIt CurrentPlayer; // get LOCAL pilot count
    pilot_cnt = local_cnt = 0;
    for ( CurrentPlayer = m_PlayerList.begin();
          CurrentPlayer != m_PlayerList.end();
          CurrentPlayer++ )
    {
        pilot_cnt++;
        if ( CurrentPlayer->IsLocal ) {
            local_cnt++;
        }
    }
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "## Pilots: total " << pilot_cnt << ", local " << local_cnt );
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "## Since: Packets " <<
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
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "## Total: Packets " <<
      mT_PacketsReceived << " BL=" <<
      mT_BlackRejected << " INV=" <<
      mT_PacketsInvalid << " UR=" <<
      mT_UnknownRelay << " RD=" <<
      mT_RelayMagic << " PD=" <<
      mT_PositionData << " NP=" <<
      mT_NotPosData <<  " CF=" <<
      mT_CrossFeedSent << "/" << mT_CrossFeedFailed << " TN=" <<
      mT_TelnetReceived << " TC/D/P=" <<
      m_TrackerConnect << "/" << m_TrackerDisconnect << "/" << m_TrackerPostion
    );
    // restart 'since' last stat counter
    m_PacketsReceived = m_BlackRejected = m_PacketsInvalid = 0;
    m_UnknownRelay = m_PositionData = m_TelnetReceived = 0; // reset
    m_RelayMagic = m_NotPosData = 0; // reset
    m_CrossFeedFailed = m_CrossFeedSent = 0;
}

int
FG_SERVER::check_keyboard
()
{
  struct stat buf;
  if (stat(exit_file,&buf) == 0)
  {
    SG_LOG (SG_SYSTEMS, SG_ALERT, "## Got EXIT file : " << exit_file);
    unlink(exit_file);
    if (stat(exit_file,&buf) == 0)
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT, "ERROR: Unable to delete exit file! Doing hard exit...");
      exit(1);
    }
    return 1;
  }
  else if ( stat(reset_file,&buf) == 0)
  {
    SG_LOG (SG_SYSTEMS, SG_ALERT, "## Got RESET file " << reset_file);
    unlink(reset_file);
    if (stat(reset_file,&buf) == 0)
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT, "ERROR: Unable to delete reset file! Doing hard exit...");
      exit(1);
    }
    m_Initialized         = true; // Init() will do it
    m_ReinitData          = true; // init the data port
    m_ReinitTelnet        = true; // init the telnet port
    m_Listening           = false;
    SigHUPHandler ( 0 );
  }
  else if ( stat(stat_file,&buf) == 0)
  {
    SG_LOG (SG_SYSTEMS, SG_ALERT, "## Got STAT file " << stat_file);
    unlink(stat_file);
    if (stat(stat_file,&buf) == 0)
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT, "ERROR: Unable to delete stat file! Doing hard exit...");
      exit(1);
    }
    Show_Stats();
  }
#ifdef _MSC_VER
  if (_kbhit())
  {
    int ch = _getch ();
    if ( ch == 0x1b )
    {
      Show_Stats();   // show stats beofre exit
      printf("Got ESC key to exit...\n");
      return 1;
    }
    else if (( ch == 'R' ) || ( ch == 'r' ))
    {
      printf("Got 'R' - Reset key...\n");
      m_Initialized         = true; // Init() will do it
      m_ReinitData          = true; // init the data port
      m_ReinitTelnet        = true; // init the telnet port
      m_Listening           = false;
      SigHUPHandler ( 0 );
    }
    else if (( ch == 'S' ) || ( ch == 's' ))
    {
      printf("Show stats\n");
      Show_Stats();
    }
    else
    {
      printf("Got UNKNOWN keyboard! %#X - Only ESC, to exit, R/r reset, S/s stats.\n", ch);
    }
  }
#endif
  return 0;
}

//////////////////////////////////////////////////////////////////////
//
//      main loop of the server
//
//////////////////////////////////////////////////////////////////////
int
FG_SERVER::Loop
()
{
  int         Bytes;
  char        Msg[MAX_PACKET_SIZE];
  netAddress  SenderAddress;
  netSocket*  ListenSockets[3 + MAX_TELNETS];
  time_t      tick0;
  time_t      CurrentTime;
  mT_PlayerListIt fg_Player;

  tick0 = time(0);
  m_IsParent = true;
  if (m_Listening == false)
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Loop() - "
    << "not listening on any socket!");
    return (ERROR_NOT_LISTENING);
  }
#ifdef _MSC_VER
  SG_ALERT (SG_SYSTEMS, SG_ALERT,  "ESC key to EXIT (after select " << m_PlayerExpires << " sec timeout)." );
#endif
  //////////////////////////////////////////////////
  //
  //      infinite listening loop
  //
  //////////////////////////////////////////////////
  CreateChatMessage (0, string("server ")+m_ServerName+string(" is online"));
  for (;;)
  {
    CurrentTime = time(0);
    // Update some things every (default) 10 secondes
    if ((CurrentTime - tick0) >= m_UpdateSecs) 
    {
      tick0 = time(0);
      if (m_PlayerList.size()>0)
      {
        //check TTL of the pilots, drop if expired
        fg_Player = m_PlayerList.begin();
        while (fg_Player != m_PlayerList.end())
        {
          //////////////////////////////////////////////////
          //        Drop fg_Player if last sign of
          //        life is older than TTL
          //////////////////////////////////////////////////
          if ( (tick0 - fg_Player->Timestamp) > m_PlayerExpires)
          {
            DropClient (fg_Player);
            continue;
          }
          fg_Player++;
        }
        // updates the position of the users
        // regularly (tracker)
        UpdateTracker (string(""),string(""), string(""),tick0,UPDATE);
      }
      if (check_keyboard())
      {
        break;
      }
    } // position (tracker)
    errno = 0;
    ListenSockets[0] = m_DataSocket;
    ListenSockets[1] = m_TelnetSocket;
    ListenSockets[2] = 0;
    Bytes = m_DataSocket->select (ListenSockets,0,m_PlayerExpires);
    if ( Bytes <= 0 )
    {
      continue;
    }
    if (ListenSockets[0] > 0)
    { // something on the wire (clients)
      Bytes = m_DataSocket->recvfrom(Msg,MAX_PACKET_SIZE, 0, &SenderAddress);
      if (Bytes <= 0) 
      {
        continue;
      }
      m_PacketsReceived++;
      HandlePacket ((char*)&Msg,Bytes,SenderAddress);
    } // DataSocket
    else if (ListenSockets[1] > 0)
    { // something on the wire (telnet)
      netAddress TelnetAddress;
      m_TelnetReceived++;
      int Fd = m_TelnetSocket->accept (&TelnetAddress);
      if (Fd < 0)
      {
        if ((errno != EAGAIN) && (errno != EPIPE))
        {
          SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_SERVER::HandleTelnet() - " << strerror (errno));
        }
        return 0;
      }
      st_telnet* t = new st_telnet;
      t->Instance = this;
      t->Fd       = Fd;
      pthread_t th;
      pthread_create (&th, NULL, &telnet_helper, t);
    } // TelnetSocket
  }
  return (0);
} // FG_SERVER::Loop()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set listening port for incoming clients
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetDataPort
(
  int Port
)
{
  if (Port != m_ListenPort)
  {
    m_ListenPort = Port;
    m_ReinitData = true;
  }
} // FG_SERVER::SetPort ( unsigned int iPort )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set listening port for telnets
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetTelnetPort
(
  int Port
)
{
  if (m_TelnetPort != Port)
  {
    m_TelnetPort = Port;
    m_ReinitTelnet = true;
  }
} // FG_SERVER::SetPort ( unsigned int iPort )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set time in seconds. if no packet arrives from a client
//      within this time, the connection is dropped.
//
//////////////////////////////////////////////////////////////////////
void    
FG_SERVER::SetPlayerExpires
(
  int Seconds
)
{
  m_PlayerExpires = Seconds;
} // FG_SERVER::SetPlayerExpires ( int iSeconds )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set nautical miles two players must be apart to be
//      out of reach
//
//////////////////////////////////////////////////////////////////////
void    
FG_SERVER::SetOutOfReach
(
  int OutOfReach
)
{
  m_PlayerIsOutOfReach = OutOfReach;
} // FG_SERVER::SetOutOfReach ( int iOutOfReach )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set the default loglevel
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetLoglevel
(
  int Loglevel
)
{
  m_Loglevel = (sgDebugPriority) Loglevel;
  sglog().setLogLevels (SG_ALL, m_Loglevel);
} // FG_SERVER::SetLoglevel ( int iLoglevel )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set the logfile
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetLogfile
(
  const std::string &LogfileName
)
{
  if (m_LogFile)
  {
    m_LogFile.close ();
  }
  m_LogFileName = LogfileName;
  m_LogFile.open (m_LogFileName.c_str(), ios::out|ios::app);
  sglog().enable_with_date (true);
  sglog().set_output (m_LogFile);
} // FG_SERVER::SetLogfile ( const std::string &LogfileName )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set if we are running as a Hubserver
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetHub
(
  bool IamHUB
)
{
  m_IamHUB = IamHUB;
} // FG_SERVER::SetLoglevel ( int iLoglevel )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set the server name
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetServerName
(
  const std::string &ServerName
)
{
  m_ServerName = ServerName;
} // FG_SERVER::SetLogfile ( const std::string &LogfileName )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set the address this server listens on
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetBindAddress
(
  const std::string &BindAddress
)
{
  m_BindAddress = BindAddress;
} // FG_SERVER::SetLogfile ( const std::string &LogfileName )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      close sockets, logfile etc.
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::Done
()
{
  if (! m_IsParent)
  {
    return;
  }
  if (m_IsTracked)
  {
#ifdef USE_TRACKER_PORT
    // using a thread - could kill it, but...
#else // #ifdef USE_TRACKER_PORT
    // using fork() - must kill child processes
    pid_t kid = m_TrackerPID;
    if (kill(kid, SIGTERM))
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Done() kill(" << kid << ", SIGKILL)!");
      kill(kid, SIGKILL);
    }
    else
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Done() kill(" << kid << ", SIGTERM) return ok.");
    }
#endif // #ifdef USE_TRACKER_PORT y/n
  }
  SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Done() - exiting");
  m_LogFile.close();
  if (m_Listening == false)
  {
    return;
  }
  if (m_ReinitTelnet && m_TelnetSocket)
  {
    m_TelnetSocket->close();
    delete m_TelnetSocket;
    m_TelnetSocket = 0;
  }
  if (m_ReinitData && m_DataSocket)
  {
    m_DataSocket->close();
    delete m_DataSocket;
    m_DataSocket = 0;
  }
  if (m_IsTracked)
  {
    delete m_Tracker;
    // Remove msg queue
    // msgctl(m_ipcid,IPC_RMID,NULL);
  }
  m_RelayList.clear ();
  m_BlackList.clear ();
  m_Listening = false;
} // FG_SERVER::Done()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      updates the remote tracker web server
//
//////////////////////////////////////////////////////////////////////
int
FG_SERVER::UpdateTracker
(
  string Callsign,
  string Passwd, 
  string Modelname,
  time_t Timestamp,
  int type
)
{
#ifndef NO_TRACKER_PORT
  char            TimeStr[100];
  mT_PlayerListIt CurrentPlayer;
  Point3D         PlayerPosGeod;
  string          Aircraft;
  string          Message;
  tm              *tm;
  FG_TRACKER::m_MsgBuffer buf;

  if (! m_IsTracked || strcmp(Callsign.c_str(),"mpdummy") == 0)
  {
    return (1);
  }
  // Creates the UTC time string
  tm = gmtime (& Timestamp);
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
  size_t Index = Modelname.rfind ("/");
  if (Index != string::npos)
  {
    Aircraft = Modelname.substr (Index + 1);
  }
  else
  {
    Aircraft = Modelname;
  }
  Index = Aircraft.find (".xml");
  if (Index != string::npos)
  {
    Aircraft.erase (Index);
  }
  // Creates the message
  if (type == CONNECT)
  {
    Message  = "CONNECT ";
    Message += Callsign;
    Message += " ";
    Message += Passwd;
    Message += " ";
    Message += Aircraft;
    Message += " ";
    Message += TimeStr;
    // queue the message
    sprintf (buf.mtext, "%s", Message.c_str());
    buf.mtype = 1;
#ifdef USE_TRACKER_PORT
    pthread_mutex_lock( &msg_mutex ); // acquire the lock
    msg_queue.push_back(Message); // queue the message
    pthread_cond_signal( &condition_var );  // wake up the worker
    pthread_mutex_unlock( &msg_mutex ); // give up the lock
#else // !#ifdef USE_TRACKER_PORT
    msgsnd (m_ipcid, &buf, strlen(buf.mtext), IPC_NOWAIT);
#endif // #ifdef USE_TRACKER_PORT y/n
#ifdef ADD_TRACKER_LOG
    write_msg_log(Message.c_str(), Message.size(), (char *)"IN: "); // write message log
#endif // #ifdef ADD_TRACKER_LOG
    m_TrackerConnect++; // count a CONNECT message queued
    return (0);
  }
  else if (type == DISCONNECT)
  {
    Message  = "DISCONNECT ";
    Message += Callsign;
    Message += " ";
    Message += Passwd;
    Message += " ";
    Message += Aircraft;
    Message += " ";
    Message += TimeStr;
    // queue the message
    sprintf (buf.mtext, "%s", Message.c_str());
    buf.mtype = 1;
#ifdef USE_TRACKER_PORT
    pthread_mutex_lock( &msg_mutex ); // acquire the lock
    msg_queue.push_back(Message); // queue the message
    pthread_cond_signal( &condition_var );  // wake up the worker
    pthread_mutex_unlock( &msg_mutex ); // give up the lock
#else // !#ifdef USE_TRACKER_PORT
    msgsnd (m_ipcid, &buf, strlen(buf.mtext), IPC_NOWAIT);
#endif // #ifdef USE_TRACKER_PORT y/n
#ifdef ADD_TRACKER_LOG
    write_msg_log(Message.c_str(), Message.size(),(char *)"IN: "); // write message log
#endif // #ifdef ADD_TRACKER_LOG
    m_TrackerDisconnect++; // count a DISCONNECT message queued
    return (0);
  }
  // we only arrive here if type!=CONNECT and !=DISCONNECT
  CurrentPlayer = m_PlayerList.begin();
  while (CurrentPlayer != m_PlayerList.end())
  {
    if (CurrentPlayer->IsLocal)
    {
      sgCartToGeod (CurrentPlayer->LastPos, PlayerPosGeod);
      Message =  "POSITION ";
      Message += CurrentPlayer->Callsign;
      Message += " ";
      Message += CurrentPlayer->Passwd;
      Message += " ";
      Message += NumToStr (PlayerPosGeod[Lat], 6)+" "; //lat
      Message += NumToStr (PlayerPosGeod[Lon], 6)+" "; //lon
      Message += NumToStr (PlayerPosGeod[Alt], 6)+" "; //alt
      Message += TimeStr;
      // queue the message
      sprintf(buf.mtext,"%s",Message.c_str());
      buf.mtype=1;
#ifdef USE_TRACKER_PORT
      pthread_mutex_lock( &msg_mutex ); // acquire the lock
      msg_queue.push_back(Message); // queue the message
      pthread_cond_signal( &condition_var );  // wake up the worker
      pthread_mutex_unlock( &msg_mutex ); // give up the lock
#else // !#ifdef USE_TRACKER_PORT
      msgsnd(m_ipcid,&buf,strlen(buf.mtext),IPC_NOWAIT);
#endif // #ifdef USE_TRACKER_PORT y/n
      m_TrackerPostion++; // count a POSITION messge queued
    }
    Message.erase(0);
    CurrentPlayer++;
  } // while
#endif // !NO_TRACKER_PORT
  return (0);
} // UpdateTracker (...)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      cleanly closes the tracker
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::CloseTracker
()
{
  if (m_IsTracked)
  {
    if (m_Tracker)
    { // delete only if enabled
      delete m_Tracker;
    }
    m_Tracker = 0;
  }
} // CloseTracker ( )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      Decides whether the relay is interested in full rate updates
//
//////////////////////////////////////////////////////////////////////
bool
FG_SERVER::IsInRange
(
  mT_Relay& Relay,
  FG_Player& SendingPlayer
)
{
  mT_PlayerListIt CurrentPlayer;

  CurrentPlayer = m_PlayerList.begin();
  while (CurrentPlayer != m_PlayerList.end())
  {
    if ((CurrentPlayer->Address == Relay.Address)
    && (Distance (SendingPlayer.LastPos, CurrentPlayer->LastPos) <= m_PlayerIsOutOfReach))
    {
      return true;
    }
    CurrentPlayer++;
  }
  return false;
} // FG_SERVER::IsInRange()
//////////////////////////////////////////////////////////////////////

// vim: ts=2:sw=2:sts=0

