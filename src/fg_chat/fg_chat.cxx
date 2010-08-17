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
//      Chatclient for fgms
//      (c) 2005-2010 Oliver Schroeder
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

#include <server/typcnvt.hxx>
#include "fg_chat.hxx"

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
FG_CHAT::FG_CHAT ()
{
  m_Initialized         = true; // Init() will do it
  m_ReinitData          = true; // init the data port
  m_ListenPort          = 5000; // port for client connections
  m_PlayerExpires       = 10;   // standard expiration period
  m_Listening           = false;
  m_Loglevel            = SG_INFO;
  m_DataSocket          = 0;
  m_NumMaxClients       = 0;
  m_PlayerIsOutOfReach  = 100;  // standard 100 nm
  m_NumCurrentClients   = 0;
  m_IsParent            = false;
  m_MaxClientID         = 0;
  m_ServerName          = "* Server *";
  int16_t *ver          = (int16_t*) & PROTO_VER;
  m_ProtoMinorVersion   = ver[HIGH];
  m_ProtoMajorVersion   = ver[LOW];
  m_LogFileName         = "chat.log";
} // FG_CHAT::FG_CHAT()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      standard destructor
//
//////////////////////////////////////////////////////////////////////
FG_CHAT::~FG_CHAT ()
{
  Done();
} // FG_CHAT::~FG_CHAT()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      basic initialization
//
//////////////////////////////////////////////////////////////////////
int
FG_CHAT::Init ()
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
  if (m_ReinitData)
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
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_CHAT::Init() - "
        << "failed to create listener socket");
      return (ERROR_CREATE_SOCKET);
    }
    m_DataSocket->setBlocking (false);
    if (m_DataSocket->bind ("", m_ListenPort) != 0)
    {
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_CHAT::Init() - "
        << "failed to bind to port " << m_ListenPort);
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "already in use?");
      return (ERROR_COULDNT_BIND);
    }
    m_ReinitData = false;
  }
  m_Listening = true;
  return (SUCCESS);
} // FG_CHAT::Init()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      insert a new client into internal list
//
//////////////////////////////////////////////////////////////////////
void
FG_CHAT::AddClient ( netAddress& Sender, char* Msg )
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
  NewPlayer.Callsign  = MsgHdr->Callsign;
  NewPlayer.Passwd    = "test"; //MsgHdr->Passwd;
  NewPlayer.ModelName = "* unknown *";
  NewPlayer.Timestamp = Timestamp;
  NewPlayer.JoinTime  = Timestamp;
  NewPlayer.Origin    = Sender.getHost ();
  NewPlayer.HasErrors = false;
  NewPlayer.Address   = Sender;
  NewPlayer.LastPos.clear();
  NewPlayer.LastOrientation.clear();
  NewPlayer.LastRelayedToInactive = 0;
  NewPlayer.PktsReceivedFrom = 0;
  NewPlayer.PktsSentTo       = 0;
  NewPlayer.PktsForwarded    = 0;
  NewPlayer.LastRelayedToInactive = 0;
  if (MsgId == CHAT_MSG_ID)
  { // don't add to local list
    return;
  }
  if (MsgId == POS_DATA_ID) 
  {
    NewPlayer.LastPos.Set (
      XDR_decode64<double> (PosMsg->position[X]),
      XDR_decode64<double> (PosMsg->position[Y]),
      XDR_decode64<double> (PosMsg->position[Z]));
    NewPlayer.LastOrientation.Set (
      XDR_decode<float> (PosMsg->orientation[X]),
      XDR_decode<float> (PosMsg->orientation[Y]),
      XDR_decode<float> (PosMsg->orientation[Z]));
    NewPlayer.ModelName = PosMsg->Model;
  }
  m_MaxClientID++;
  NewPlayer.ClientID = m_MaxClientID;
  m_PlayerList.push_back (NewPlayer);
  m_NumCurrentClients++;
  if (m_NumCurrentClients > m_NumMaxClients)
  {
    m_NumMaxClients = m_NumCurrentClients;
  }
  cout << endl
       << "New LOCAL Client: "
       << NewPlayer.Callsign << " "
       << Sender.getHost() << ":" << Sender.getPort()
       << " (" << NewPlayer.ModelName << ")"
       << endl;
  cout << "current clients: "
       << m_NumCurrentClients << " max: " << m_NumMaxClients
       << endl;
} // FG_CHAT::AddClient()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      clean up player list
//      check for expired players
//
//////////////////////////////////////////////////////////////////////
void
FG_CHAT::CleanUp ()
{
  time_t          Timestamp;
  mT_PlayerListIt CurrentPlayer;

  Timestamp = time(0);
  CurrentPlayer = m_PlayerList.begin();
  while (CurrentPlayer != m_PlayerList.end())
  {
    if ((Timestamp - CurrentPlayer->Timestamp) > PLAYER_TTL)
    {
      cout
        << "TTL exeeded, dropping pilot " << CurrentPlayer->Callsign
        << "  after " << Timestamp-CurrentPlayer->JoinTime << " seconds."
        << "  Usage in: " << CurrentPlayer->PktsReceivedFrom
        << " forwarded: " << CurrentPlayer->PktsForwarded
        << " out: " << CurrentPlayer->PktsSentTo
        << endl;
      m_NumCurrentClients--;
      cout
        << "current clients: "
        << m_NumCurrentClients << " max: " << m_NumMaxClients
        << endl;
      mT_PlayerListIt P;
      P = CurrentPlayer;
      CurrentPlayer = m_PlayerList.erase (P);
      continue;
    }
    CurrentPlayer++;
  }
} // FG_CHAT::CleanUp ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  create a chat message and put it into the internal message queue
//
//////////////////////////////////////////////////////////////////////
void
FG_CHAT::CreateChatMessage ( int ID, string Msg )
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
} // FG_CHAT::CreateChatMessage ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      handle client connections
//
//////////////////////////////////////////////////////////////////////
void
FG_CHAT::HandlePacket ( char * Msg, int Bytes, netAddress &SenderAddress )
{
  T_MsgHdr*       MsgHdr;
  T_PositionMsg*  PosMsg;
  T_ChatMsg*      ChatMsg;
  uint32_t        MsgLen;
  uint32_t        MsgId;
  uint32_t        MsgMagic;
  time_t          Timestamp;
  bool            PlayerInList;
  string          ErrorMsg;
  Point3D         SenderPosition;
  Point3D         SenderOrientation;
  Point3D         PlayerPosGeod;
  mT_PlayerListIt CurrentPlayer;
  mT_PlayerListIt SendingPlayer;
  mT_MessageIt    CurrentMessage;
  unsigned int    PktsForwarded = 0;

  MsgHdr = (T_MsgHdr *) Msg;
  Timestamp = time(0);
  //////////////////////////////////////////////////
  //
  //      check if the received packet is valid
  //
  //////////////////////////////////////////////////
  if (Bytes < (int)sizeof(MsgHdr))
  {
    ErrorMsg  = SenderAddress.getHost();
    ErrorMsg += " packet size is too small!";
    cout << ErrorMsg << endl;
    return;
  }
  MsgMagic = XDR_decode<uint32_t> (MsgHdr->Magic);
  if (MsgMagic != MSG_MAGIC)
  {
    ErrorMsg  = SenderAddress.getHost();
    ErrorMsg += " illegal magic number!";
    cout << ErrorMsg << endl;
    return;
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
    cout << ErrorMsg << endl;
    return;
  }
  MsgId  = XDR_decode<uint32_t> (MsgHdr->MsgId);
  MsgLen = XDR_decode<uint32_t> (MsgHdr->MsgLen);
  if (MsgId == POS_DATA_ID) 
  {
    if (MsgLen < sizeof(T_MsgHdr) + sizeof(T_PositionMsg))
    {
      ErrorMsg  = SenderAddress.getHost();
      ErrorMsg += " Client sends insufficient position data, ";
      ErrorMsg += "should be ";
      ErrorMsg += NumToStr (sizeof(T_MsgHdr)+sizeof(T_PositionMsg));
      ErrorMsg += " is: " + NumToStr (MsgHdr->MsgLen);
      cout << ErrorMsg << endl;
      return;
    }
  }
  //////////////////////////////////////////////////
  //
  //      check for senders position
  //      we have to walk through to list until
  //      we find the sender, so we walk through
  //      the list twice.
  //      FIXME: can be done better?
  //
  //////////////////////////////////////////////////
  PlayerInList = false;
  CurrentPlayer = m_PlayerList.begin();
  while ((CurrentPlayer != m_PlayerList.end()) && (PlayerInList == false))
  {
    if ((CurrentPlayer->Callsign == MsgHdr->Callsign)
    && (CurrentPlayer->Address.getIP() == SenderAddress.getIP()))
    {
      PlayerInList = true;
      CurrentPlayer->Timestamp = Timestamp;
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
        CurrentPlayer->LastPos = SenderPosition;
        CurrentPlayer->LastOrientation = SenderOrientation;
      }
      else
      {
        SenderPosition = CurrentPlayer->LastPos;
        SenderOrientation = CurrentPlayer->LastOrientation;
      }
    } else if (CurrentPlayer->Callsign == MsgHdr->Callsign) {
      cout << "double! " << CurrentPlayer->Callsign
           << " reconnected from " << SenderAddress.getHost()
           << endl;
      // Quietly ignore this packet. 
      return;
    } 
    CurrentPlayer++;
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
    //////////////////////////////////////////////////
    //
    //  send any message in m_MessageList to all 
    //  clients
    //  FIXME: send to server, not to clients
    //
    //////////////////////////////////////////////////
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
    //////////////////////////////////////////////////
    //        Sender == CurrentPlayer?
    //////////////////////////////////////////////////
    if ((CurrentPlayer->Callsign == MsgHdr->Callsign)
    && (CurrentPlayer->Address.getIP() == SenderAddress.getIP()))
    {
      SendingPlayer = CurrentPlayer;
      SendingPlayer->PktsReceivedFrom++;
      CurrentPlayer++;
      continue; // don't send packet back to sender
    }
    //////////////////////////////////////////////////
    //        Drop CurrentPlayer if last sign of
    //        life is older then TTL
    //////////////////////////////////////////////////
    if ((Timestamp - CurrentPlayer->Timestamp) > PLAYER_TTL)
    {
      cout
        << "TTL exeeded, dropping pilot "
        << CurrentPlayer->Callsign
        << "  after " << Timestamp-CurrentPlayer->JoinTime << " seconds."
        << "  Usage #packets in: " << CurrentPlayer->PktsReceivedFrom
        << " forwarded: " << CurrentPlayer->PktsForwarded
        << " out: " << CurrentPlayer->PktsSentTo
        << endl;
      m_NumCurrentClients--;
      cout << "current clients: "
        << m_NumCurrentClients << " max: " << m_NumMaxClients
        << endl;
      mT_PlayerListIt P;
      P = CurrentPlayer;
      CurrentPlayer = m_PlayerList.erase (P);
      continue;
    }
    //////////////////////////////////////////////////
    //
    //      do not send packet to clients which
    //      are out of reach.
    //      FIXME: check for all message types
    //
    //////////////////////////////////////////////////
    if (MsgId == POS_DATA_ID)
    if ((Distance
         (SenderPosition, CurrentPlayer->LastPos) > m_PlayerIsOutOfReach)
    &&  (CurrentPlayer->Callsign.compare(0, 3, "obs", 3) != 0) )
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
  if (PlayerInList == false)
  {
    AddClient (SenderAddress, Msg);
  }
  //////////////////////////////////////////////////
  //
  //      delete internal chat message queue
  //
  //////////////////////////////////////////////////
  if (m_MessageList.size())
  {
    mT_MessageIt P;
    CurrentMessage = m_MessageList.begin();
    while (CurrentMessage != m_MessageList.end())
    {
      P = CurrentMessage;
      delete[] P->Msg;
      m_MessageList.erase (P);
      CurrentMessage = m_MessageList.begin();
    }
  }
} // FG_CHAT::HandlePacket ( char* sMsg[MAX_PACKET_SIZE] )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      main loop of the server
//
//////////////////////////////////////////////////////////////////////
int
FG_CHAT::Loop ()
{
  int         Bytes;
  char        Msg[MAX_PACKET_SIZE];
  netAddress  SenderAddress;
  netSocket*  ListenSockets[3];
  time_t      tick0,tick1;
  time_t      ticksum;
  time_t      CurrentTime, LastCleanUp;

  tick0 = time(0);
  m_IsParent = true;
  if (m_Listening == false)
  {
    SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_CHAT::Loop() - "
      << "not listening on any socket!");
    return (ERROR_NOT_LISTENING);
  }
  //////////////////////////////////////////////////
  //
  //      infinite listening loop
  //
  //////////////////////////////////////////////////
  LastCleanUp = time(0);
  for (;;)
  {
    CurrentTime = time(0);
    tick1 = time(0);
    ticksum = tick1-tick0;
    errno = 0;
    ListenSockets[0] = m_DataSocket;
    ListenSockets[1] = stdin;
    ListenSockets[2] = 0;
    Bytes = m_DataSocket->select (ListenSockets, 0, m_PlayerExpires);
    if (Bytes == -2)
    { // timeout, no packets received
      CleanUp ();
      LastCleanUp = CurrentTime;
      continue;
    }
    if (! Bytes)
    {
      SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_CHAT::Loop() - Bytes <= 0!");
      continue;
    }
    if (ListenSockets[0] != 0)
    { // something on the wire (clients)
      Bytes = m_DataSocket->recvfrom(Msg,MAX_PACKET_SIZE, 0, &SenderAddress);
      if (Bytes <= 0) 
      {
        if (errno == EAGAIN)
          continue;
        SG_ALERT (SG_SYSTEMS, SG_ALERT, "FG_CHAT::Loop() - "
          << "recv(client) <= 0!");
        // FIXME: reason to quit?
        continue;
      }
      HandlePacket ((char*)&Msg,Bytes,SenderAddress);
    } // DataSocket
    else if (ListenSockets[1] != 0)
    { // user input
      // FIXME: cin >> buffer;
      CleanUp ();
      LastCleanUp = CurrentTime;
    } // TelnetSocket
    // Do periodic CleanUp.
    if (CurrentTime-LastCleanUp > PLAYER_TTL)
    {
      CleanUp ();
      LastCleanUp = CurrentTime;
    }
  }
} // FG_CHAT::Loop()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      set the logfile
//
//////////////////////////////////////////////////////////////////////
void
FG_CHAT::SetLogfile ( const std::string &LogfileName )
{
  if (m_LogFile)
  {
    m_LogFile.close ();
  }
  m_LogFileName = LogfileName;
  m_LogFile.open (m_LogFileName.c_str(), ios::out|ios::app);
  sglog().enable_with_date (true);
  sglog().set_output (m_LogFile);
} // FG_CHAT::SetLogfile ( const std::string &LogfileName )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      close sockets, logfile etc.
//
//////////////////////////////////////////////////////////////////////
void
FG_CHAT::Done ()
{
  if (m_IsParent)
  {
    SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_CHAT::Done() - exiting");
  }
  if (m_LogFile)
  {
    m_LogFile.close();
  }
  if (m_Listening == false)
  {
    return;
  }
  if (m_ReinitData && m_DataSocket)
  {
    m_DataSocket->close();
    delete m_DataSocket;
    m_DataSocket = 0;
  }
  m_Listening = false;
} // FG_CHAT::Done()
//////////////////////////////////////////////////////////////////////

// vim: ts=2:sw=2:sts=0

