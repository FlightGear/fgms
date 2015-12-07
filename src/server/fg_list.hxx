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

/**
 * @file fg_list.hxx
 * @author Oliver Schroeder
 *
 * Implementation of a thread-safe list.
 */

#if !defined FG_LIST_HXX
#define FG_LIST_HXX

#include <string>
#include <vector>
#include <plib/netSocket.h>
#include <pthread.h>
#include <fg_geometry.hxx>
#include <fg_common.hxx>


//////////////////////////////////////////////////////////////////////
/** 
 * @class FG_ListElement
 * @brief Represent a generic list element
 * 
 */
class FG_ListElement
{
public:
	/** every element has a a name */
	FG_ListElement ( const string& Name );
	FG_ListElement ( const FG_ListElement& P );
	~FG_ListElement ();
	/** mark a nonexisting element */
	static const size_t NONE_EXISTANT;
	/** @brief The ID of this entry */
	size_t		ID;
	/** @brief The Timeout of this entry */
	time_t		Timeout;
	/** @brief The callsign (or name) */
	string		Name;
	/** @brief The network address of this element */
	netAddress	Address;
	/** @brief The time this entry was added to the list */
	time_t		JoinTime;
	/** @brief timestamp of last seen packet from this element */
	time_t		LastSeen;
	/** @brief timestamp of last sent packet to this element */
	time_t		LastSent;
	/** @brief Count of packets recieved from client */
	uint64_t	PktsRcvd;
	/** @brief Count of packets sent to client */
	uint64_t	PktsSent;
	/** @brief Count of bytes recieved from client */
	uint64_t	BytesRcvd;
	/** @brief Count of bytes sent to client */
	uint64_t	BytesSent;
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
 * Player objects are stored in the FG_SERVER::m_PlayerList
 * They are created and added in FG_SERVER::AddClient
 * They are dropped with FG_SERVER::DropClient after expiry time
 * Clients are added even if they have bad data, see FG_SERVER::AddBadClient
 */
class FG_Player : public FG_ListElement
{
public:
	string	Origin;
	/** @brief The password 
	 *  @warning This is not currently used
	 */
	string	Passwd;
	/** @brief The model name */
	string	ModelName;
	/** @brief The last recorded position */
	Point3D	LastPos;
	/** @brief The last recorded orientation */
	Point3D	LastOrientation;
	/** @brief \b true is this client is directly connected to this \ref fgms instance */
	bool	IsLocal;
	/** @brief in case of errors the reason is stored here 
	 * @see FG_SERVER::AddBadClient
	 */
	string	Error;    // in case of errors
	/** @brief \b true if this client has errors
	 * @see FG_SERVER::AddBadClient
	 */
	bool	HasErrors;
	/** when did we sent updates of this player to inactive relays */
	time_t	LastRelayedToInactive;
	/** \b true if we need to send updates to inactive relays */
	bool	DoUpdate;
	FG_Player ();
	FG_Player ( const string& Name );
	FG_Player ( const FG_Player& P);
	~FG_Player ();
	void operator =  ( const FG_Player& P );
	virtual bool operator ==  ( const FG_Player& P );
private:
	void assign ( const FG_Player& P );
}; // FG_Player

/** 
 * @class mT_FG_List
 * @brief a generic list implementation for fgms
 * 
 */
template <class T>
class mT_FG_List
{
public:
	typedef vector<T> ListElements;
	typedef typename vector<T>::iterator ListIterator;
	/** constructor, must supply a Name */
	mT_FG_List   ( const string& Name );
	~mT_FG_List  ();
	/** return the number of elements in this list */
	size_t Size  ();
	/** delete all elements of this list */
	void   Clear ();
	/** add an element to this list */
	size_t Add   ( T& Element, time_t TTL );
	/** Check if the entry TTL expired*/
	bool CheckTTL (int position);
	/** delete an element of this list (by position) */
	void DeleteByPosition (int position);
	/** delete an element of this list */
	ListIterator Delete	( const ListIterator& Element );
	/** find an element by its IP address */
	ListIterator Find	( const netAddress& Address, const string& Name = "" );
	/** find an element by its Name */
	ListIterator FindByName	( const string& Name = "" );
	/** find an element by its ID */
	ListIterator FindByID	( size_t ID );
	/** return an iterator of the first element */
	ListIterator Begin	();
	/** iterator of the end of the list */
	ListIterator End	();
	/** update sent counter of an element and of the list */
	void UpdateSent ( ListIterator& Element, size_t bytes );
	/** update receive counter of an element and of the list */
	void UpdateRcvd ( ListIterator& Element, size_t bytes );
	/** update sent counter of the list */
	void UpdateSent ( size_t bytes );
	/** update receive counter of the list */
	void UpdateRcvd ( size_t bytes );
	/** thread lock the list */
	void Lock();
	/** thread unlock the list */
	void Unlock();
	/** return a copy of an element at position x (thread safe) */
	T operator []( const size_t& Index );
	/** @brief maximum entries this list ever had */
	size_t		MaxID;
	/** @brief Count of packets recieved from client */
	uint64_t	PktsRcvd;  
	/** @brief Count of packets sent to client */
	uint64_t	PktsSent;
	/** @brief Count of bytes recieved from client */
	uint64_t	BytesRcvd;  
	/** @brief Count of bytes sent to client */
	uint64_t	BytesSent;        
	/** the name (or description) of this element */
	string		Name;
private:
	/** @brief mutex for thread safty */
	pthread_mutex_t   m_ListMutex;
	/** @brief timestamp of last cleanup */
	time_t	LastRun;
	/** do not allow standard constructor */
	mT_FG_List ();
	/** the actual storage of elements */
	ListElements	Elements;
};

typedef mT_FG_List<FG_ListElement>		FG_List;
typedef mT_FG_List<FG_Player>			PlayerList;
typedef vector<FG_ListElement>::iterator	ItList;
typedef vector<FG_Player>::iterator		PlayerIt;

//////////////////////////////////////////////////////////////////////
/**
 *
 * construct an element and initialise counters
 */
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
/** NOT thread safe
 *
 * Return the number of elements of this list.
 * If working with threads make sure to * use Lock() and Unlock()
 * @see Lock()
 * @see Unlock()
 */
template <class T>
size_t
mT_FG_List<T>::Size()
{
	return Elements.size ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** thread safe
 *
 * Add an element to the list
 * @param Element the FG_ListElement to add
 * @param TTL automatic expiry of the element (time-to-live)
 * @see Find()
 */
template <class T>
size_t
mT_FG_List<T>::Add( T& Element, time_t TTL)
{
	this->MaxID++;
	Element.ID	= this->MaxID;
	Element.Timeout	= TTL;
//	pthread_mutex_lock   ( & m_ListMutex );
	Elements.push_back   ( Element );
//	pthread_mutex_unlock ( & m_ListMutex );
	return this->MaxID;
}
//////////////////////////////////////////////////////////////////////


template <class T>
void 
mT_FG_List<T>::DeleteByPosition (int position)
{
	pthread_mutex_lock ( & m_ListMutex );
	ListIterator Element;
	this->LastRun = time (0);
	Element = Elements.begin();
	std::advance (Element,position);
	Elements.erase   ( Element );
	pthread_mutex_unlock ( & m_ListMutex );
}
//////////////////////////////////////////////////////////////////////
/** thread safe
 *
 * Delete an entry from the list
 * @param Element iterator pointing to the element to delete
 */
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::Delete( const ListIterator& Element)
{
	ListIterator E;
	pthread_mutex_lock   ( & m_ListMutex );
	E = Elements.erase   ( Element );
	pthread_mutex_unlock ( & m_ListMutex );
	return (E);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NOT thread safe
 *
 * Find an element by IP-address (and optionally Name). Automatically
 * removes entries which TTL is expired, with the exception of the 'users'
 * list which entries are deleted in FG_SERVER::HandlePacket()
 * @param Address IP address of the element
 * @param Name The name (or description) of the element
 * @return iterator pointing to the found element, or End() if element
 *         could not be found
 */
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::Find( const netAddress& Address, const string& Name)
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
				{
					Element->LastSeen = this->LastRun;
					RetElem = Element;
				}
			}
			else
			{
				Element->LastSeen = this->LastRun;
				RetElem = Element;
			}
		}
		else
		{
			if ((Element->Timeout == 0) || (this->Name == "Users"))
			{	// never times out
				Element++;
				continue;
			}
			if ( (this->LastRun - Element->LastSeen) > Element->Timeout )
			{
				SG_LOG ( SG_FGMS, SG_INFO,
				  this->Name << ": TTL exceeded for "
				  << Element->Name << " "
				  << Element->Address.getHost() << " "
				  << "after " << diff_to_days (Element->LastSeen - Element->JoinTime)
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
/** NOT thread safe
 *
 * Find an element by Name.
 * @param Name The name (or description) of the element
 * @return iterator pointing to the found element, or End() if element
 *         could not be found
 */
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::FindByName( const string& Name)
{
	ListIterator Element;
	ListIterator RetElem;

	this->LastRun = time (0);
	RetElem = Elements.end();
	Element = Elements.begin();
	while (Element != Elements.end())
	{
		if (Element->Name == Name)
		{
			Element->LastSeen = this->LastRun;
			return Element;
		}
		Element++;
	}
	return RetElem;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NOT thread safe
 *
 * Find an element by ID.
 * @param ID The ID of the element
 * @return iterator pointing to the found element, or End() if element
 *         could not be found
 */
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::FindByID
( size_t ID )
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
/** NOT thread safe
 *
 * In general iterators are not thread safe. If you use them you have
 * to ensure propper locking youself!
 *
 * @return the first element of the list. If the list is empty this
 *         equals End()
 */
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::Begin()
{
	return Elements.begin ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 *
 * @return the end of the list (which is not a valid entry!) 
 */
template <class T>
typename vector<T>::iterator
mT_FG_List<T>::End()
{
	return Elements.end ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 *
 * Set a mutex lock on the list, so concurrent operations are blocked
 * until the lock is released.
 * @see Unlock()
 */
template <class T>
void
mT_FG_List<T>::Lock()
{
	pthread_mutex_lock ( & m_ListMutex );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 *
 * Release a mutex lock
 */
template <class T>
void
mT_FG_List<T>::Unlock()
{
	pthread_mutex_unlock ( & m_ListMutex );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** thread safe
 * Check if the elements have TTL expired. 
 * True if not expired/unknown. False if expired.
 */
template <class T>
bool
mT_FG_List<T>::CheckTTL( int position )
{
	pthread_mutex_lock ( & m_ListMutex );
	ListIterator Element;
	this->LastRun = time (0);
	Element = Elements.begin();
	std::advance (Element,position);

	if (Element->Timeout == 0)
	{	// never timeouts
		pthread_mutex_unlock ( & m_ListMutex );
		return true;
	}
	if ( (this->LastRun - Element->LastSeen) > Element->Timeout )
	{
		SG_LOG ( SG_FGMS, SG_INFO,
		  this->Name << ": TTL exceeded for "
		  << Element->Name << " "
		  << Element->Address.getHost() << " "
		  << "after " << diff_to_days (Element->LastSeen - Element->JoinTime)
		  );
		  pthread_mutex_unlock ( & m_ListMutex );
		  return false;
	}
	pthread_mutex_unlock ( & m_ListMutex );
	return true;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** thread safe
 * Use this for element access whenever possible. However, if you 
 * need to modify the element you have to use iterators which are not
 * thread safe and make sure to use Lock() and Unlock() yourself.
 * @param Index index of the element
 * @return a copy the the element at index Index
 */
template <class T>
T
mT_FG_List<T>::operator []( const size_t& Index )
{
	T RetElem("");
//	pthread_mutex_lock ( & m_ListMutex );
	if (Index < Elements.size ())
		RetElem = Elements[Index];
//	pthread_mutex_unlock ( & m_ListMutex );
	return RetElem;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 *
 * Update sent-counters of the list and of an element. Every call of
 * this method means we sent a single packet (and the packet counter
 * is increased by 1).
 * @param Element The element to which data was sent
 * @param bytes The number of bytes sent
 */
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
/**
 *
 * Update receive-counters of the list and of an element. Every call of
 * this method means we received a single packet (and the packet counter
 * is increased by 1).
 * @param Element The element from which data was received
 * @param bytes The number of bytes received
 */
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
/**
 *
 * Update sent-counters of the list. Every call of
 * this method means we sent a single packet (and the packet counter
 * is increased by 1).
 * @param bytes The number of bytes sent
 */
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
/**
 *
 * Update received-counters of the list. Every call of
 * this method means we received a single packet (and the packet counter
 * is increased by 1).
 * @param bytes The number of bytes received.
 */
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
/**
 *
 * Delete all entries of the list.
 */
template <class T>
void
mT_FG_List<T>::Clear()
{
	Lock ();
	Elements.clear ();
	Unlock ();
}
//////////////////////////////////////////////////////////////////////

#endif
