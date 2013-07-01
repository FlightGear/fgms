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
	static const size_t NONE_EXISTANT;
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
	/** @brief timestamp of last sent packet to this element */
	time_t		LastSent;
	/** @brief Count of packets recieved from client */
	uint64		PktsRcvd;
	/** @brief Count of packets sent to client */
	uint64		PktsSent;
	/** @brief Count of bytes recieved from client */
	uint64		BytesRcvd;
	/** @brief Count of bytes sent to client */
	uint64		BytesSent;
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
 * Player objects are stored in the FG_SERVER::mT_PlayerList
 * They are created and added in FG_SERVER::AddClient
 * They are dropped with FG_SERVER::DropClient after expiry time
 * Clients are added even if they have bad data, see FG_SERVER::AddBadClient
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
	time_t        LastRelayedToInactive;
	FG_Player ();
	FG_Player ( const string& Name );
	FG_Player ( const FG_Player& P);
	~FG_Player ();
	void operator =  ( const FG_Player& P );
	virtual bool operator ==  ( const FG_Player& P );
private:
	void assign ( const FG_Player& P );
}; // FG_Player

template <class T>
class mT_FG_List
{
public:
	typedef vector<T> ListElements;
	typedef typename vector<T>::iterator ListIterator;
	/** @brief maximum entries this list ever had */
	size_t		MaxID;
	mT_FG_List   ( const string& Name );
	~mT_FG_List  ();
	size_t Size  ();
	void   Clear ();
	size_t Add   ( T& Element, time_t TTL );
	ListIterator Delete	( const ListIterator& Element );
	ListIterator Find	( const netAddress& Address, const string& Name = "" );
	ListIterator FindByID( size_t ID );
	ListIterator Begin	();
	ListIterator End	();
	void UpdateSent ( ListIterator& Element, size_t bytes );
	void UpdateRcvd ( ListIterator& Element, size_t bytes );
	void UpdateSent ( size_t bytes );
	void UpdateRcvd ( size_t bytes );
	void Lock();
	void Unlock();
	T operator []( const size_t& Index );
	/** @brief Count of packets recieved from client */
	uint64		PktsRcvd;  
	/** @brief Count of packets sent to client */
	uint64		PktsSent;
	/** @brief Count of bytes recieved from client */
	uint64		BytesRcvd;  
	/** @brief Count of bytes sent to client */
	uint64		BytesSent;        
	string	Name;
private:
	/** @brief mutex for thread safty */
	pthread_mutex_t   m_ListMutex;
	/** @brief timestamp of last cleanup */
	time_t	LastRun;
	/** not defined */
	mT_FG_List ();
	ListElements	Elements;
};

typedef mT_FG_List<FG_ListElement>		FG_List;
typedef mT_FG_List<FG_Player>			PlayerList;
typedef vector<FG_ListElement>::iterator	ItList;
typedef vector<FG_Player>::iterator		PlayerIt;

//////////////////////////////////////////////////////////////////////
template <class T>
mT_FG_List<T>::mT_FG_List
(
	const string& Name
)
{
	pthread_mutex_init ( &m_ListMutex, 0 );
	this->Name	= Name;
	PktsSent	= 0;
	BytesSent	= 0;
	PktsRcvd	= 0;
	BytesRcvd	= 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
mT_FG_List<T>::~mT_FG_List
()
{
	Clear ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
size_t
mT_FG_List<T>::Size
()
{
	int size;
	pthread_mutex_lock   ( & m_ListMutex );
	size = Elements.size ();
	pthread_mutex_unlock ( & m_ListMutex );
	return size;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
size_t
mT_FG_List<T>::Add
(
	T& Element,
	time_t TTL
)
{
	this->MaxID++;
	Element.ID	= this->MaxID;
	Element.Timeout	= TTL;
	pthread_mutex_lock ( & m_ListMutex );
	Elements.push_back ( Element );
	pthread_mutex_unlock ( & m_ListMutex );
	return this->MaxID++;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::Delete
(
	const ListIterator& Element
)
{
	ListIterator E;
	pthread_mutex_lock   ( & m_ListMutex );
	E = Elements.erase   ( Element );
	pthread_mutex_unlock ( & m_ListMutex );
	return (E);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::Find
(
	const netAddress& Address,
	const string& Name
)
{
	ListIterator Element;
	ListIterator RetElem;

	this->LastRun = time (0);
	RetElem = Elements.end();
	Element = Elements.begin();
	while (Element != Elements.end())
	{
		if (Element->Address == Address)
		{
			if (Name != "") 
			{	// additionally look for matching name
				if (Element->Name == Name)
					RetElem = Element;
			}
			else
			{
				RetElem = Element;
			}
		}
		else
		{
			if (Element->Timeout == 0)
			{	// never times out
				Element++;
				continue;
			}
			if ( (this->LastRun - Element->LastSeen) > Element->Timeout )
			{
				SG_LOG ( SG_FGMS, SG_INFO,
				  this->Name << ": TTL exceeded for "
				  << Element->Address.getHost() << " " << Element->Name
				  << " after " << (Element->LastSeen - Element->JoinTime) << " seconds"
				  );
				Element = Elements.erase (Element);
				continue;
			}
		}
		Element++;
	}
	return RetElem;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::FindByID
(
	size_t ID
)
{
	ListIterator Element;
	ListIterator RetElem;
	this->LastRun = time (0);
	RetElem = Elements.end();
	Element = Elements.begin();
	while (Element != Elements.end())
	{
		if (Element->ID == ID)
		{
			RetElem = Element;
			break;
		}
		Element++;
	}
	return RetElem;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::Begin
()
{
	return Elements.begin ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::End
()
{
	return Elements.end ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
void
mT_FG_List<T>::Lock
()
{
	pthread_mutex_lock ( & m_ListMutex );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
void
mT_FG_List<T>::Unlock
()
{
	pthread_mutex_unlock ( & m_ListMutex );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
T
mT_FG_List<T>::operator []
(
	const size_t& Index
)
{
	T RetElem("");
	pthread_mutex_lock ( & m_ListMutex );
	if (Index < Elements.size ())
		RetElem = Elements[Index];
	pthread_mutex_unlock ( & m_ListMutex );
	return RetElem;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
void
mT_FG_List<T>::UpdateSent
(
	ListIterator& Element,
	size_t bytes
)
{
	PktsSent++;
	BytesSent += bytes;
	Element->UpdateSent (bytes);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
void
mT_FG_List<T>::UpdateRcvd
(
	ListIterator& Element,
	size_t bytes
)
{
	PktsRcvd++;
	BytesRcvd += bytes;
	Element->UpdateRcvd (bytes);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
void
mT_FG_List<T>::UpdateSent
(
	size_t bytes
)
{
	PktsSent++;
	BytesSent += bytes;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
void
mT_FG_List<T>::UpdateRcvd
(
	size_t bytes
)
{
	PktsRcvd++;
	BytesRcvd += bytes;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
void
mT_FG_List<T>::Clear
()
{
	Elements.clear ();
}
//////////////////////////////////////////////////////////////////////

#endif
