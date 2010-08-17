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
//  chatclient for fgms
//
//////////////////////////////////////////////////////////////////////

#if not defined FG_CHAT_HXX
#define FG_CHAT_HXX

using namespace std;

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <plib/netSocket.h>
#include <flightgear/MultiPlayer/mpmessages.hxx>
#include <flightgear/MultiPlayer/tiny_xdr.hxx>
#include <simgear/debug/logstream.hxx>
#include <server/daemon.hxx>
#include <server/fg_geometry.hxx>

//////////////////////////////////////////////////////////////////////
//
//  the server class
//
//////////////////////////////////////////////////////////////////////
class FG_CHAT
{

public:
  //////////////////////////////////////////////////
  //
  //  internal constants
  //  
  //////////////////////////////////////////////////
  enum FG_CHAT_CONSTANTS
  {
    // return values
    SUCCESS                 = 0,
    ERROR_COMMANDLINE       = 1,
    ERROR_CREATE_SOCKET     = 2,
    ERROR_COULDNT_BIND      = 3,
    ERROR_NOT_LISTENING     = 4,
    ERROR_COULDNT_LISTEN    = 5,
    // other constants
    MAX_PACKET_SIZE         = 1024,
    PLAYER_TTL              = 10
  };
  //////////////////////////////////////////////////
  //
  //  constructors
  //
  //////////////////////////////////////////////////
  FG_CHAT ();
  ~FG_CHAT ();

  //////////////////////////////////////////////////
  //
  //  methods
  //
  //////////////////////////////////////////////////
  int   Init ();
  int   Loop ();
  void  Done ();

private:

  //////////////////////////////////////////////////
  //
  //  type of list of players
  //  
  //////////////////////////////////////////////////
  class mT_Player
  {
    public:
    string        Origin;
    netAddress    Address;
    string        Callsign;
    string        Passwd;
    string        ModelName;
    time_t        JoinTime;
    time_t        Timestamp;
    Point3D       LastPos;
    Point3D       LastOrientation;
    bool          IsLocal;
    string        Error;    // in case of errors
    bool          HasErrors;
    int           ClientID;
    time_t        LastRelayedToInactive;
    unsigned int  PktsReceivedFrom;  // From client
    unsigned int  PktsSentTo;        // Sent to client
    unsigned int  PktsForwarded;     // From client sent to other players/relays
  }; // mT_Player
  //////////////////////////////////////////////////
  //
  //  chat messages from server to clients
  //  
  //////////////////////////////////////////////////
  class mT_ChatMsg
  {
  public:
    mT_ChatMsg (int I, char* M) { Target=I; Msg=M; };
    int   Target; // 0 = all
    char* Msg;
  };
  //////////////////////////////////////////////////
  //
  //  private methods
  //  
  //////////////////////////////////////////////////
  void  AddClient     ( netAddress& Sender, char* Msg );
  void  HandlePacket  ( char *sMsg, int Bytes,
                        netAddress &SenderAdress );
  void  CleanUp ();
  void  CreateChatMessage ( int ID, string Msg );
  void  FG_CHAT::SetLogfile ( const std::string &LogfileName );
  //////////////////////////////////////////////////
  //
  //  private variables
  //  
  //////////////////////////////////////////////////
  typedef std::list<mT_Player>            mT_PlayerList;
  typedef mT_PlayerList::iterator         mT_PlayerListIt;
  typedef std::list<mT_ChatMsg>           mT_MessageList;
  typedef std::list<mT_ChatMsg>::iterator mT_MessageIt;
  bool            m_Initialized;
  bool            m_ReinitData;
  bool            m_Listening;
  int             m_ListenPort;
  int             m_PlayerExpires;
  int             m_PlayerIsOutOfReach;
  sgDebugPriority m_Loglevel;
  ofstream        m_LogFile;
  string          m_LogFileName;
  int             m_NumCurrentClients;
  int             m_NumMaxClients;
  int16_t         m_ProtoMinorVersion;
  int16_t         m_ProtoMajorVersion;
  bool            m_IsParent;
  int             m_MaxClientID;
  string          m_ServerName;
  netSocket*      m_DataSocket;
  mT_PlayerList   m_PlayerList;
  mT_MessageList  m_MessageList;
}; // FG_CHAT

#endif

// vim: ts=2:sw=2:sts=0

