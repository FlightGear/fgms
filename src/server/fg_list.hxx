/**
 * @file fg_list.hxx
 * @author Oliver Schroeder
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, U$
//
// Copyright (C) 2005-2013  Oliver Schroeder
//

//////////////////////////////////////////////////////////////////////
/** 
 * @class FG_ListElement
 * @brief Represent a Player
 * 
 * * Player objects are stored in the FG_SERVER::mT_PlayerList
 * * They are created and added in FG_SERVER::AddClient
 * * They are dropped with FG_SERVER::DropClient after expiry time
 * * Clients are added even if they have bad data, see FG_SERVER::AddBadClient
 */

#if !defined FG_LIST_HXX
#define FG_LIST_HXX

#include <string>
#include <vector>
#include <plib/netSocket.h>
#include <pthread.h>
#include <fg_geometry.hxx>
#include <fg_common.hxx>

class FG_ListElement
{
public:
	/** @brief The ID of this entry */
	size_t		ID;
	/** @brief The Timeout of this entry */
	time_t		Timeout;
	/** @brief The callsign (or name) */
	string		Name;
	/** @brief The network address of this element */
	netAddress	Address;
	/** @brief The time this player joined the sessin in utc */
	time_t		JoinTime;
	/** @brief timestamp of last seen packet from this element */
	time_t		LastSeen;
	/** @brief Count of packets recieved from client */
	uint64		PktsRcvdFrom;  
	/** @brief Count of packets sent to client */
	uint64		PktsSentTo;        
	/** @brief Count of bytes recieved from client */
	uint64		BytesRcvdFrom;  
	/** @brief Count of bytes sent to client */
	uint64		BytesSentTo;        
	FG_ListElement ( const string& Name );
	FG_ListElement ( const FG_ListElement& P );
	~FG_ListElement ();
	void operator =  ( const FG_ListElement& P );
	virtual bool operator ==  ( const FG_ListElement& P );
	void UpdateSent ( size_t bytes );
	void UpdateRcvd ( size_t bytes );
protected:
	FG_ListElement ();
	void assign ( const FG_ListElement& P );
}; // FG_ListElement

//////////////////////////////////////////////////////////////////////
/** 
 * @class FG_Player
 * @brief Represent a Player
 * 
 * * Player objects are stored in the FG_SERVER::mT_PlayerList
 * * They are created and added in FG_SERVER::AddClient
 * * They are dropped with FG_SERVER::DropClient after expiry time
 * * Clients are added even if they have bad data, see FG_SERVER::AddBadClient
 */

class FG_Player : public FG_ListElement
{
public:
	string        Origin;
	/** @brief The password 
		@warning This is not currently used
	 */
	string        Passwd;
	/** @brief The model name */
	string        ModelName;
	/** @brief The last recorded position */
	Point3D       LastPos;
	/** @brief The last recorded orientation */
	Point3D       LastOrientation;
	/** @brief \b true is this client is directly connected to this \ref fgms instance */
	bool          IsLocal;
	/** @brief The last error message is any 
	 * @see FG_SERVER::AddBadClient
	 */
	string        Error;    // in case of errors
	/** @brief \b true if this client has errors
	 * @see FG_SERVER::AddBadClient
	 */
	bool          HasErrors;
	/** @brief This client id
	 * @see FG_SERVER::m_MaxClientID
	 */
	int           ClientID;
	time_t        LastRelayedToInactive;
	FG_Player ();
	FG_Player ( const FG_Player& P);
	~FG_Player ();
	void operator =  ( const FG_Player& P );
	virtual bool operator ==  ( const FG_Player& P );
private:
	void assign ( const FG_Player& P );
}; // FG_Player

typedef vector<FG_ListElement>	ElementList;
typedef ElementList::iterator	ItList;

class FG_List
{
public:
	/** @brief maximum entries this list ever had */
	size_t		MaxID;
	/** @brief look for expirering entries every so often */
	ElementList	Elements;
	FG_List	( const string& Name );
	~FG_List ();
	size_t Size ();
	size_t Add	( FG_ListElement& Element, time_t TTL );
	ItList Delete	( const ItList& Element );
	void Clear	();
	ItList Find	( const netAddress& Address, const string& Name );
	ItList FindByID	( size_t ID );
	ItList End	();
	FG_ListElement operator []( const size_t& Index );
	void UpdateSent ( ItList& Element, size_t bytes );
	void UpdateRcvd ( ItList& Element, size_t bytes );
	/** @brief Count of packets recieved from client */
	uint64		PktsRcvd;  
	/** @brief Count of packets sent to client */
	uint64		PktsSent;
	/** @brief Count of bytes recieved from client */
	uint64		BytesRcvd;  
	/** @brief Count of bytes sent to client */
	uint64		BytesSent;        
private:
	/** @brief mutex for thread safty */
	pthread_mutex_t   m_ListMutex;
	/** @brief timestamp of last cleanup */
	time_t	LastRun;
	string	Name;
	/** not defined */
	FG_List	();
};

#endif
