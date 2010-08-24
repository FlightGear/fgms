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

//////////////////////////////////////////////////////////////////////
//
//      Server for FlightGear
//      (c) 2005-2010 Oliver Schroeder
//      (c) 2006 Julien Pierru ( UpdateTracker() )
//      (c) 2007-2010 Anders Gidenstam ( LazyRelay )
//
//////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#include <endian.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <string>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <netinet/in.h>
/* From netSocket.cxx */
#ifndef INADDR_NONE
  #define INADDR_NONE ((unsigned long)-1)
#endif

#include "fg_server.hxx"
#include "typcnvt.hxx"

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#   define LOW  1
#   define HIGH 0
#else
#   define LOW  0
#   define HIGH 1
#endif

bool    RunAsDaemon = true;
cDaemon Myself;

//////////////////////////////////////////////////////////////////////
//
//      Initilize to standard values
//
//////////////////////////////////////////////////////////////////////
FG_SERVER::FG_SERVER ()
{
  m_Initialized         = true; // Init() will do it
  m_ReinitData          = true; // init the data port
  m_ReinitTelnet        = true; // init the telnet port
  m_ListenPort          = 5000; // port for client connections
  m_PlayerExpires       = 10;   // standard expiration period
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
  int16_t *ver          = (int16_t*) & PROTO_VER;
  m_ProtoMinorVersion   = ver[HIGH];
  m_ProtoMajorVersion   = ver[LOW];
  m_LogFileName         = "fg_server.log";
  //wp                  = fopen("wp.txt", "w");
  m_BlackList           = map<uint32_t, bool>::map();
} // FG_SERVER::FG_SERVER()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      standard destructor
//
//////////////////////////////////////////////////////////////////////
FG_SERVER::~FG_SERVER ()
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
FG_SERVER::Init ()
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
    if (m_DataSocket->bind ("", m_ListenPort) != 0)
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
    m_TelnetSocket = new netSocket;
    if (m_TelnetSocket->open (true) == 0)   // TCP-Socket
    {
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Init() - "
        << "failed to create telnet socket");
      return (ERROR_CREATE_SOCKET);
    }
    m_TelnetSocket->setBlocking (false);
    if (m_TelnetSocket->bind ("", m_TelnetPort) != 0)
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
    m_ReinitTelnet = false;
  }
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# This is " << m_ServerName);
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# FlightGear Multiplayer Server v"
    << VERSION << " started");
  SG_ALERT (SG_SYSTEMS, SG_ALERT, "# using protocol version v"
    << m_ProtoMajorVersion << "." << m_ProtoMinorVersion
    << " (LazyRelay enabled)");
  SG_ALERT (SG_SYSTEMS, SG_ALERT,"# listening to port " << m_ListenPort);
  SG_ALERT (SG_SYSTEMS, SG_ALERT,"# telnet port " << m_TelnetPort);
  SG_ALERT (SG_SYSTEMS, SG_ALERT,"# using logfile " << m_LogFileName);
  if (true == m_IamHUB)
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "# I am a HUB Server");
  }
  if (m_IsTracked)
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "# tracked to "
      << m_Tracker->GetTrackerServer ()
      << ":" << m_Tracker->GetTrackerPort ());
    if (m_Tracker->InitTracker() == ERROR_CREATE_SOCKET)
    {
      /* This is in the child */
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_TRACKER::InitTracker() - "
        << "failed to create TCP socket");
      m_IsTracked = false;
      // Remove msg queue
      //msgctl (m_ipcid, IPC_RMID, NULL);
      exit (ERROR_CREATE_SOCKET);
    }
  }
  SG_ALERT (SG_SYSTEMS, SG_ALERT,
    "# I have " << m_RelayList.size() << " relays");
  mT_RelayListIt CurrentRelay = m_RelayList.begin();
  while (CurrentRelay != m_RelayList.end())
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "# relay " << CurrentRelay->Name);
    CurrentRelay++;
  }
  SG_ALERT (SG_SYSTEMS, SG_ALERT,
    "# I have " << m_CrossfeedList.size() << " crossfeeds");
  mT_RelayListIt CurrentCrossfeed = m_CrossfeedList.begin();
  while (CurrentCrossfeed != m_CrossfeedList.end())
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "# crossfeed " << CurrentCrossfeed->Name
              << ":" << CurrentCrossfeed->Address.getPort());
    CurrentCrossfeed++;
  }
  SG_ALERT (SG_SYSTEMS, SG_ALERT,
    "# I have " << m_BlackList.size() << " blacklisted IPs");
#if 0
  mT_BlackListIt CurrentBlack = m_BlackList.begin();
  while (CurrentBlack != m_BlackList.end())
  {
    char* buf = inet_ntoa ( (in_addr) CurrentBlack->first ) ;
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "# blacklisted: "
      << buf);
    CurrentBlack++;
  }
#endif
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
FG_SERVER::PrepareInit ()
{
  SG_LOG (SG_SYSTEMS, SG_ALERT, "# caught SIGHUP, doing reinit!");
  m_RelayList.clear ();
  m_CrossfeedList.clear ();
} // FG_SERVER::PrepareInit ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      handle a telnet session
//      if a telnet connection is opened, this method outputs a list
//      of all known clients.
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::HandleTelnet ( netSocket* Telnet )
{
  pid_t           Pid;
  netSocket       NewTelnet;
  int             Fd;
  string          Message;
  Point3D         PlayerPosGeod;  // Geodetic Coordinates
  netAddress      TelnetAddress;
  mT_PlayerListIt CurrentPlayer;

  errno = 0;
  Fd = Telnet->accept (&TelnetAddress);
  if (Fd < 0)
  {
    if (errno != EAGAIN)
    {
      SG_LOG (SG_SYSTEMS, SG_ALERT,
        "FG_SERVER::HandleTelnet() - " << strerror (errno));
    }
    return;
  }
  NewTelnet.setHandle (Fd);
  //////////////////////////////////////////////////
  //
  //      fork a new process,
  //      creating a new thread migth be better
  //
  //////////////////////////////////////////////////
  if ( (Pid = fork ()) < 0)
  {
    SG_LOG (SG_SYSTEMS, SG_ALERT,
      "FG_SERVER::HandleTelnet() - couldn't fork!");
    return; // something went wrong!
  }
  if (Pid > 0)
  { // parent
    return;
  }
  m_IsParent = false;
  m_DataSocket->close ();
  Telnet->close();
  //////////////////////////////////////////////////
  //
  //      create the output message
  //      header
  //
  //////////////////////////////////////////////////
  Message  = "# This is " + m_ServerName;
  Message += "\n";
  Message += "# FlightGear Multiplayer Server v" + string(VERSION);
  Message += " using protocol version v";
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
  NewTelnet.send (Message.c_str(),Message.size(),0);
  Message  = "# "+ NumToStr (m_PlayerList.size(), 0);
  if ( (m_PlayerList.size(), 0) == 1 )
    Message += " pilot online\n";
  else
    Message += " pilots online\n";
  NewTelnet.send (Message.c_str(), Message.size(), 0);
  //////////////////////////////////////////////////
  //
  //      create list of players
  //
  //////////////////////////////////////////////////
  for (CurrentPlayer = m_PlayerList.begin();
       CurrentPlayer != m_PlayerList.end();
       CurrentPlayer++)
  {
    sgCartToGeod (CurrentPlayer->LastPos, PlayerPosGeod);
    Message = "";
    if (CurrentPlayer->Callsign.compare(0, 3, "obs", 3) == 0)
    {
      Message = "# ";
    }
    Message += CurrentPlayer->Callsign + "@";
    if (CurrentPlayer->IsLocal)
    {
      Message += "LOCAL: ";
    }
    else
    {
      Message += CurrentPlayer->Origin + ": ";
    }
    if (CurrentPlayer->Error != "")
    {
      Message += CurrentPlayer->Error + " ";
    }
    Message += NumToStr (CurrentPlayer->LastPos[X], 6)+" ";
    Message += NumToStr (CurrentPlayer->LastPos[Y], 6)+" ";
    Message += NumToStr (CurrentPlayer->LastPos[Z], 6)+" ";
    Message += NumToStr (PlayerPosGeod[Lat], 6)+" ";
    Message += NumToStr (PlayerPosGeod[Lon], 6)+" ";
    Message += NumToStr (PlayerPosGeod[Alt], 6)+" ";
    Message += NumToStr (CurrentPlayer->LastOrientation[X], 6)+" ";
    Message += NumToStr (CurrentPlayer->LastOrientation[Y], 6)+" ";
    Message += NumToStr (CurrentPlayer->LastOrientation[Z], 6)+" ";
    Message += CurrentPlayer->ModelName;
    Message += "\n";
    NewTelnet.send (Message.c_str(), Message.size(), 0);
  }
  NewTelnet.close ();
  exit (0);
} // FG_SERVER::HandleTelnet ( netAddress& Sender )
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
  netAddress& Sender,
  string & ErrorMsg,
  bool IsLocal
  )
{
  bool                    AlreadyThere;
  string                  Message;
  mT_Player               NewPlayer;
  mT_PlayerListIt         CurrentPlayer;

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
  NewPlayer.LastRelayedToInactive = 0;
  NewPlayer.PktsReceivedFrom      = 0;
  NewPlayer.PktsSentTo            = 0;
  NewPlayer.PktsForwarded         = 0;
  NewPlayer.LastRelayedToInactive = 0;
  AlreadyThere  = false;
  CurrentPlayer = m_PlayerList.begin();
  //////////////////////////////////////////////////
  //      see, if we already know the client
  //////////////////////////////////////////////////
  while (CurrentPlayer != m_PlayerList.end())
  {
    if (CurrentPlayer->Address.getIP() == Sender.getIP())
    {
      AlreadyThere = true;
    }
    CurrentPlayer++;
  }
  //////////////////////////////////////////////////
  //      new client, send an error message
  //////////////////////////////////////////////////
  if (! AlreadyThere)
  {
    SG_LOG (SG_SYSTEMS, SG_ALERT,
      "FG_SERVER::AddBadClient() - " << ErrorMsg);
    Message = "bad client connected: ";
    Message += Sender.getHost() + string(": ");
    Message += ErrorMsg;
    CreateChatMessage (NewPlayer.ClientID, Message);
    m_PlayerList.push_back (NewPlayer);
    m_NumCurrentClients++;
  }
} // FG_SERVER::AddBadClient ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      insert a new client into internal list
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::AddClient ( netAddress& Sender, char* Msg, bool IsLocal )
{
  time_t          Timestamp;
  uint32_t        MsgLen;
  uint32_t        MsgId;
  uint32_t        MsgMagic;
  string          Message;
  T_MsgHdr*       MsgHdr;
  T_PositionMsg*  PosMsg;
  mT_Player       NewPlayer;

  Timestamp           = time(0);
  MsgHdr              = (T_MsgHdr *) Msg;
  PosMsg              = (T_PositionMsg *) (Msg + sizeof(T_MsgHdr));
  MsgId               = XDR_decode<uint32_t> (MsgHdr->MsgId);
  MsgLen              = XDR_decode<uint32_t> (MsgHdr->MsgLen);
  MsgMagic            = XDR_decode<uint32_t> (MsgHdr->Magic);
  if (MsgId != POS_DATA_ID)
    return;
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
  NewPlayer.LastRelayedToInactive = 0;
  NewPlayer.PktsReceivedFrom = 0;
  NewPlayer.PktsSentTo       = 0;
  NewPlayer.PktsForwarded    = 0;
  NewPlayer.LastRelayedToInactive = 0;
  NewPlayer.LastPos.Set (
      XDR_decode64<double> (PosMsg->position[X]),
      XDR_decode64<double> (PosMsg->position[Y]),
      XDR_decode64<double> (PosMsg->position[Z]));
  NewPlayer.LastOrientation.Set (
      XDR_decode<float> (PosMsg->orientation[X]),
      XDR_decode<float> (PosMsg->orientation[Y]),
      XDR_decode<float> (PosMsg->orientation[Z]));
  NewPlayer.ModelName = PosMsg->Model;
  m_MaxClientID++;
  NewPlayer.ClientID = m_MaxClientID;
  m_PlayerList.push_back (NewPlayer);
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
  if (IsLocal)
  {
    Message = "New LOCAL Client: ";
  }
  else
  {
    Message = "New REMOTE Client: ";
  }
  SG_LOG (SG_SYSTEMS, SG_INFO, Message
    << NewPlayer.Callsign << " "
    << Sender.getHost() << ":" << Sender.getPort()
    << " (" << NewPlayer.ModelName << ")");
  SG_LOG (SG_SYSTEMS, SG_INFO, "current clients: "
    << m_NumCurrentClients << " max: " << m_NumMaxClients);
} // FG_SERVER::AddClient()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      insert a new relay server into internal list
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::AddRelay ( const string & Server, int Port )
{
  mT_Relay        NewRelay;
  unsigned int    IP;

  NewRelay.Name = Server;
  NewRelay.Address.set ((char*) Server.c_str(), Port);
  IP = NewRelay.Address.getIP();
  if ( IP != INADDR_ANY && IP != INADDR_NONE )
  {
    NewRelay.Timestamp = time(0);
    NewRelay.Active = false;
    m_RelayList.push_back (NewRelay);
  }
} // FG_SERVER::AddRelay()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      insert a new crossfeed server into internal list
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::AddCrossfeed ( const string & Server, int Port )
{
  mT_Relay        NewRelay;
  unsigned int    IP;

  NewRelay.Name = Server;
  NewRelay.Address.set ((char*) Server.c_str(), Port);
  IP = NewRelay.Address.getIP();
  if ( IP != INADDR_ANY && IP != INADDR_NONE )
  {
    NewRelay.Timestamp = time(0);
    NewRelay.Active = false;
    m_CrossfeedList.push_back (NewRelay);
  }
} // FG_SERVER::AddCrossfeed()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      adds a tracking server
//
//////////////////////////////////////////////////////////////////////
int
FG_SERVER::AddTracker ( const string & Server, int Port, bool IsTracked )
{
  m_IsTracked     = IsTracked;
  if ( m_Tracker )
  {
    msgctl(m_ipcid,IPC_RMID,NULL);
    delete m_Tracker;
  }
  m_ipcid         = msgget(IPCKEY,IPCPERMS|IPC_CREAT);
  m_Tracker = new FG_TRACKER(Port,Server,m_ipcid);
  return (SUCCESS);
} // FG_SERVER::AddTracker()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      Add an IP to the blacklist
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::AddBlacklist ( const string& FourDottedIP )
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
FG_SERVER::DropClient ( mT_PlayerListIt& CurrentPlayer )
{
  if (CurrentPlayer->IsLocal) 
  {
    UpdateTracker (CurrentPlayer->Callsign,
      CurrentPlayer->Passwd, 
      CurrentPlayer->ModelName,
      CurrentPlayer->Timestamp,
      DISCONNECT);
  }
  SG_LOG (SG_SYSTEMS, SG_INFO, "FG_SERVER::DropClient() - "
    << "TTL exeeded, dropping pilot "
    << CurrentPlayer->Callsign
    << "  after " << time(0)-CurrentPlayer->JoinTime << " seconds."
    << "  Usage #packets in: " << CurrentPlayer->PktsReceivedFrom
    << " forwarded: " << CurrentPlayer->PktsForwarded
    << " out: " << CurrentPlayer->PktsSentTo);
  m_NumCurrentClients--;
  SG_LOG (SG_SYSTEMS, SG_INFO, "current clients: "
    << m_NumCurrentClients << " max: " << m_NumMaxClients);
  if (m_NumCurrentClients > 0)
  {
    string Message = "client '";
    Message += CurrentPlayer->Callsign;
    Message += "' has left";
    CreateChatMessage (0, Message);
  }
  CurrentPlayer = m_PlayerList.erase (CurrentPlayer);
} // FG_SERVER::DropClient ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      clean up player list
//      check for expired players
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::CleanUp ()
{
  time_t          Timestamp;
  mT_PlayerListIt CurrentPlayer;
  mT_RelayListIt  CurrentRelay;

  Timestamp = time(0);
  CurrentPlayer = m_PlayerList.begin();
  while (CurrentPlayer != m_PlayerList.end())
  {
    if ((Timestamp-CurrentPlayer->Timestamp) > PLAYER_TTL)
    {
      DropClient (CurrentPlayer);
      continue;
    }
    CurrentPlayer++;
  }
  CurrentRelay = m_RelayList.begin();
  while (CurrentRelay != m_RelayList.end())
  {
    if (((Timestamp-CurrentRelay->Timestamp) > RELAY_TTL)
    && CurrentRelay->Active)
    {
      CurrentRelay->Active = false;
      SG_LOG (SG_SYSTEMS, SG_ALERT, "Deactivating relay "
        << CurrentRelay->Name);
    }
    CurrentRelay++;
  }
} // FG_SERVER::CleanUp ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  create a chat message and put it into the internal message queue
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::CreateChatMessage ( int ID, string Msg )
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
  // strncpy(MsgHdr.Callsign, m_ServerName.c_str(), MAX_CALLSIGN_LEN);
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
FG_SERVER::IsBlackListed ( netAddress &SenderAddress )
{
  string          ErrorMsg;
  if (m_BlackList.find(SenderAddress.getIP()) != m_BlackList.end())
  {
    ErrorMsg  = SenderAddress.getHost();
    ErrorMsg += " banned!";
    //AddBadClient (SenderAddress, ErrorMsg, true);
    SG_LOG (SG_SYSTEMS, SG_ALERT, "BLACKLISTED: " << ErrorMsg);
    return (true);
  }
  return (false);
} // FG_SERVER::IsBlackListed ()
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
  netAddress &SenderAddress
  )
{
  uint32_t        MsgMagic;
  uint32_t        MsgLen;
  uint32_t        MsgId;
  string          ErrorMsg;

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
    ErrorMsg  = SenderAddress.getHost();
    ErrorMsg += " illegal magic number!";
    AddBadClient (SenderAddress, ErrorMsg, true);
    return (false);
  }
  if (XDR_decode<uint32_t> (MsgHdr->Version) != PROTO_VER)
  {
    MsgHdr->Version = XDR_decode<uint32_t> (MsgHdr->Version);
    ErrorMsg  = SenderAddress.getHost();
    ErrorMsg += " illegal protocol version! Should be ";
    int16_t *ver = (int16_t*) & PROTO_VER;
    ErrorMsg += NumToStr (ver[LOW], 0);
    ErrorMsg += "." + NumToStr (ver[HIGH], 0);
    ErrorMsg += " but is ";
    ver = (int16_t*) & MsgHdr->Version;
    ErrorMsg += NumToStr (ver[LOW], 0);
    ErrorMsg += "." + NumToStr (ver[HIGH], 0);
    AddBadClient (SenderAddress, ErrorMsg, true);
    return (false);
  }
  if (MsgId == POS_DATA_ID) 
  {
    if (MsgLen < sizeof(T_MsgHdr) + sizeof(T_PositionMsg))
    {
      ErrorMsg  = SenderAddress.getHost();
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
FG_SERVER::SendChatMessages ( mT_PlayerListIt& CurrentPlayer )
{
  T_ChatMsg*    ChatMsg;
  mT_MessageIt  CurrentMessage;

  if ((CurrentPlayer->IsLocal) && (m_MessageList.size()))
  {
    CurrentMessage = m_MessageList.begin();
    while (CurrentMessage != m_MessageList.end())
    {
      if ((CurrentMessage->Target == 0)
      ||  (CurrentMessage->Target == CurrentPlayer->ClientID))
      {
        ChatMsg =(T_ChatMsg*)(CurrentMessage->Msg+sizeof(T_MsgHdr));
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
FG_SERVER::DeleteMessageQueue ()
{
  mT_MessageIt    CurrentMessage;
  if (m_MessageList.size())
  {
    CurrentMessage = m_MessageList.begin();
    while (CurrentMessage != m_MessageList.end())
    {
      delete[] CurrentMessage->Msg;
      CurrentMessage = m_MessageList.erase (CurrentMessage);
    }
  }
} // FG_SERVER::DeleteMessageQueue ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      handle client connections
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::HandlePacket ( char * Msg, int Bytes, netAddress &SenderAddress )
{
  T_MsgHdr*       MsgHdr;
  T_PositionMsg*  PosMsg;
  uint32_t        MsgId;
  uint32_t        MsgMagic;
  time_t          Timestamp;
  bool            PlayerInList;
  bool            PacketFromLocalClient;
  Point3D         SenderPosition;
  Point3D         SenderOrientation;
  Point3D         PlayerPosGeod;
  mT_PlayerListIt CurrentPlayer;
  mT_PlayerListIt SendingPlayer = m_PlayerList.end();
  mT_RelayListIt  CurrentRelay;
  unsigned int    PktsForwarded = 0;

  MsgHdr    = (T_MsgHdr *) Msg;
  Timestamp = time(0);
  MsgMagic  = XDR_decode<uint32_t> (MsgHdr->Magic);
  MsgId     = XDR_decode<uint32_t> (MsgHdr->MsgId);
  PacketFromLocalClient = true;  // assume client to be local
  if (true == IsBlackListed (SenderAddress))
  {
    return;
  }
  if (false == PacketIsValid (Bytes, MsgHdr, SenderAddress))
  {
    return;
  }
  if (MsgMagic == RELAY_MAGIC) // not a local client
  {
    PacketFromLocalClient = false;
    MsgHdr->Magic = XDR_encode<uint32_t> (MSG_MAGIC);
  }
  //////////////////////////////////////////////////
  //
  //    Store senders position
  //
  //////////////////////////////////////////////////
  if (MsgId == POS_DATA_ID)
  {
    PosMsg = (T_PositionMsg *) (Msg + sizeof(T_MsgHdr));
    SenderPosition.Set (
      XDR_decode64<double> (PosMsg->position[X]),
      XDR_decode64<double> (PosMsg->position[Y]),
      XDR_decode64<double> (PosMsg->position[Z]));
    SenderOrientation.Set (
      XDR_decode<float> (PosMsg->orientation[X]),
      XDR_decode<float> (PosMsg->orientation[Y]),
      XDR_decode<float> (PosMsg->orientation[Z]));
  }
  //////////////////////////////////////////
  //
  //      send the packet back to all clients.
  //      since we are walking through the list,
  //      we look for the sending client, too. if it
  //      is not already there, add it to the list
  //
  //////////////////////////////////////////////////
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
    SendChatMessages (CurrentPlayer);
    //////////////////////////////////////////////////
    //        Sender == CurrentPlayer?
    //////////////////////////////////////////////////
    //  FIXME: if Sender is a Relay,
    //         CurrentPlayer->lRealAddress will be
    //         address of Relay and not the client's!
    //         so use a clientID instead
    if ((CurrentPlayer->Callsign == MsgHdr->Callsign)
    && (CurrentPlayer->Address.getIP() == SenderAddress.getIP()))
    {
      PlayerInList = true;
      CurrentPlayer->Timestamp = Timestamp;
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
      CurrentPlayer->PktsReceivedFrom++;
      CurrentPlayer++;
      continue; // don't send packet back to sender
    }
    else if (CurrentPlayer->Callsign == MsgHdr->Callsign)
    {
      // Same callsign, but different IP. 
      // Quietly ignore this packet. 
      return;
    } 
    //////////////////////////////////////////////////
    //        Drop CurrentPlayer if last sign of
    //        life is older then TTL
    //////////////////////////////////////////////////
    if ((Timestamp - CurrentPlayer->Timestamp) > PLAYER_TTL)
    {
      DropClient (CurrentPlayer);
      continue;
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
    //      FIXME: check for all message types
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
      m_DataSocket->sendto (Msg, Bytes, 0, &CurrentPlayer->Address);
      CurrentPlayer->PktsSentTo++;
      PktsForwarded++;
    }
    CurrentPlayer++;
  }
  if (false == PlayerInList)
  {
    if (MsgId != POS_DATA_ID)
    {
      // ignore clients until we have a valid position
      return;
    }
    AddClient (SenderAddress, Msg, PacketFromLocalClient);
    CurrentPlayer = m_PlayerList.end();
    CurrentPlayer--;
    SendingPlayer = CurrentPlayer;
  }
  DeleteMessageQueue ();
  //////////////////////////////////////////////////
  //
  //        send packet to all Relays,
  //        change Magic to RELAY_MAGIC, so the
  //        receiving Relay will not resend
  //        packets back for this client.
  //        do not send to oberserver, if origin is
  //        a Relay
  //
  //////////////////////////////////////////////////
  if (SendingPlayer == m_PlayerList.end())
  {
    // The sending player was not found in the player list.
    // FIXME: This happens although it seems like it should be impossible.
    SG_LOG (SG_SYSTEMS, SG_ALERT,
      "fgms: Sending player not found in m_PlayerList in HandlePacket()!");
    return;
  }
  bool UpdateInactive = Timestamp -
    SendingPlayer->LastRelayedToInactive > UPDATE_INACTIVE_PERIOD;
  if (true == UpdateInactive)
  {
    SendingPlayer->LastRelayedToInactive = Timestamp;
  }
  if (true == PacketFromLocalClient)
  {
    MsgHdr->Magic = XDR_encode<uint32_t> (RELAY_MAGIC);
    CurrentRelay = m_RelayList.begin();
    while (CurrentRelay != m_RelayList.end())
    {
      if ( (CurrentRelay->Active && IsInRange(*CurrentRelay, *SendingPlayer))
      || (true == UpdateInactive)
      || (true == m_IamHUB))
      {
        if (CurrentRelay->Address.getIP() != SenderAddress.getIP())
        {
          m_DataSocket->sendto(Msg, Bytes, 0, &CurrentRelay->Address);
          PktsForwarded++;
        }
      }
      CurrentRelay++;
    }
  }
  else 
  {
    // Renew timestamp of sending relay.
    mT_RelayListIt CurrentRelay = m_RelayList.begin();
    while (CurrentRelay != m_RelayList.end())
    {
      if (CurrentRelay->Address.getIP() == SenderAddress.getIP())
      {
        if (!CurrentRelay->Active)
        {
          SG_LOG (SG_SYSTEMS, SG_ALERT, "Activating relay "
            << CurrentRelay->Name);
          CurrentRelay->Active = true;
        }
        CurrentRelay->Timestamp = Timestamp;
        break;
      }
      CurrentRelay++;
    }
  }
  //////////////////////////////////////////////////
  //
  //  Send the packet to the crossfed servers.
  //
  //////////////////////////////////////////////////
  MsgHdr->Magic = XDR_encode<uint32_t> (RELAY_MAGIC);
  mT_RelayListIt CurrentCrossfeed = m_CrossfeedList.begin();
  while (CurrentCrossfeed != m_CrossfeedList.end())
  {
    if (CurrentCrossfeed->Address.getIP() != SenderAddress.getIP())
    {
      m_DataSocket->sendto(Msg, Bytes, 0, &CurrentCrossfeed->Address);
    }
    CurrentCrossfeed++;
  }
  SendingPlayer->PktsForwarded += PktsForwarded;
} // FG_SERVER::HandlePacket ( char* sMsg[MAX_PACKET_SIZE] )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      main loop of the server
//
//////////////////////////////////////////////////////////////////////
int
FG_SERVER::Loop ()
{
  int         Bytes;
  char        Msg[MAX_PACKET_SIZE];
  netAddress  SenderAddress;
  netSocket*  ListenSockets[3 + MAX_TELNETS];
  time_t      tick0,tick1;
  time_t      ticksum;
  time_t      CurrentTime, LastCleanUp;

  tick0 = time(0);
  m_IsParent = true;
  if (m_Listening == false)
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Loop() - "
      << "not listening on any socket!");
    return (ERROR_NOT_LISTENING);
  }
  //////////////////////////////////////////////////
  //
  //      infinite listening loop
  //
  //////////////////////////////////////////////////
  CreateChatMessage (0,
    string("server ")+m_ServerName+string(" is online"));
  LastCleanUp = time(0);
  for (;;)
  {
    CurrentTime = time(0);
    tick1 = time(0);
    ticksum = tick1-tick0;
    // Update tracker every 10 secondes
    if ((float)ticksum >= 10.0) 
    {
      tick0 = time(0);
      if (m_PlayerList.size()>0)
      { // updates the position of the users
        // regularly (tracker)
        UpdateTracker (string(""),string(""), string(""),tick0,UPDATE);
      }
    } // position (tracker)
    errno = 0;
    ListenSockets[0] = m_DataSocket;
    ListenSockets[1] = m_TelnetSocket;
    ListenSockets[2] = 0;
    Bytes = m_DataSocket->select (ListenSockets,0,m_PlayerExpires);
    if (Bytes == -2)
    { // timeout, no packets received
      CleanUp ();
      LastCleanUp = CurrentTime;
      continue;
    }
    if (! Bytes)
    {
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Loop() - Bytes <= 0!");
      continue;
    }
    if (ListenSockets[0] != 0)
    { // something on the wire (clients)
      Bytes = m_DataSocket->recvfrom(Msg,MAX_PACKET_SIZE, 0, &SenderAddress);
      if (Bytes <= 0) 
      {
        if (errno == EAGAIN)
          continue;
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Loop() - "
          << "recv(client) <= 0!");
        // FIXME: reason to quit?
        continue;
      }
      HandlePacket ((char*)&Msg,Bytes,SenderAddress);
    } // DataSocket
    else if (ListenSockets[1] != 0)
    { // something on the wire (telnet)
      HandleTelnet (m_TelnetSocket);
      CleanUp ();
      LastCleanUp = CurrentTime;
    } // TelnetSocket
    // Do periodic CleanUp.
    if ((CurrentTime-LastCleanUp > PLAYER_TTL)
    || (CurrentTime-LastCleanUp > RELAY_TTL))
    {
      CleanUp ();
      LastCleanUp = CurrentTime;
    }
  }
} // FG_SERVER::Loop()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set listening port for incoming clients
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetDataPort ( int Port )
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
FG_SERVER::SetTelnetPort ( int Port )
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
FG_SERVER::SetPlayerExpires ( int Seconds )
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
FG_SERVER::SetOutOfReach ( int OutOfReach )
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
FG_SERVER::SetLoglevel ( int Loglevel )
{
  m_Loglevel = (sgDebugPriority) Loglevel;
  sglog().setLogLevels (SG_ALL, m_Loglevel);
} // FG_SERVER::SetLoglevel ( int iLoglevel )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set if we are running as a Hubserver
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetHub ( bool IamHUB )
{
  m_IamHUB = IamHUB;
} // FG_SERVER::SetLoglevel ( int iLoglevel )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set the logfile
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetLogfile ( const std::string &LogfileName )
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
//      set the server name
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::SetServerName ( const std::string &ServerName )
{
  m_ServerName = ServerName;
} // FG_SERVER::SetLogfile ( const std::string &LogfileName )
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//
//      close sockets, logfile etc.
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::Done ()
{
  if (m_IsParent)
  {
    SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_SERVER::Done() - exiting");
  }
  if (m_LogFile)
  {
    m_LogFile.close();
  }
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
  char            TimeStr[100];
  mT_PlayerListIt CurrentPlayer;
  Point3D         PlayerPosGeod;
  string          Aircraft;
  string          Message;
  tm              *tm;
  struct msgbuffer buf;

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
    msgsnd (m_ipcid, &buf, strlen(buf.mtext), IPC_NOWAIT);
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
    msgsnd (m_ipcid, &buf, strlen(buf.mtext), IPC_NOWAIT);
    return (0);
  }
  // we only arrive here if type!=CONNECT and !=DISCONNECT
  CurrentPlayer = m_PlayerList.begin();
  while (CurrentPlayer != m_PlayerList.end())
  {
    if ((CurrentPlayer->IsLocal)
    ||   (true == m_IamHUB))
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
      msgsnd(m_ipcid,&buf,strlen(buf.mtext),IPC_NOWAIT);
    }
    Message.erase(0);
    CurrentPlayer++;
  } // while
  return (0);
} // UpdateTracker (...)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      cleanly closes the tracker
//
//////////////////////////////////////////////////////////////////////
void
FG_SERVER::CloseTracker()
{
  if (m_IsTracked)
  {
    delete m_Tracker;
    m_Tracker = 0;

    // Remove msg queue
    //msgctl(m_ipcid,IPC_RMID,NULL);
  }
} // CloseTracker ( )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      Decides whether the relay is interested in full rate updates
//
//////////////////////////////////////////////////////////////////////
bool
FG_SERVER::IsInRange( mT_Relay& Relay,  mT_Player& SendingPlayer )
{
  mT_PlayerListIt         CurrentPlayer;

  CurrentPlayer = m_PlayerList.begin();
  while (CurrentPlayer != m_PlayerList.end())
  {
    if ((CurrentPlayer->Address.getIP() == Relay.Address.getIP())
    && (Distance (SendingPlayer.LastPos, CurrentPlayer->LastPos)
        <= m_PlayerIsOutOfReach))
    {
      return true;
    }
    CurrentPlayer++;
  }
  return false;
} // FG_SERVER::IsInRange()
//////////////////////////////////////////////////////////////////////

// vim: ts=2:sw=2:sts=0

