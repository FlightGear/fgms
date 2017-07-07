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

#ifndef FG_LIST_HXX
#define FG_LIST_HXX

#include <string>
#include <vector>
#include <list>
#include <pthread.h>
#include "fg_log.hxx"
#include "netaddr.hxx"
#include "fg_geometry.hxx"
#include "fg_thread.hxx"

namespace fgmp
{

//////////////////////////////////////////////////////////////////////
/** 
 * @class ListElement
 * @brief Represent a generic list element
 * 
 */
class ListElement
{
public:
	/** every element has a a name */
	ListElement ( const std::string& Name );
	ListElement ( const ListElement& P );
	~ListElement ();
	/** mark a nonexisting element */
	static const size_t NONE_EXISTANT;
	/** @brief The ID of this entry */
	size_t		ID;
	/** @brief The Timeout of this entry */
	time_t		Timeout;
	/** @brief The callsign (or name) */
	std::string	Name;
	/** @brief The network address of this element */
	netaddr		Address;
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
	void operator =  ( const ListElement& P );
	virtual bool operator ==  ( const ListElement& P );
	void UpdateSent ( size_t bytes );
	void UpdateRcvd ( size_t bytes );
protected:
	ListElement ();
	void assign ( const ListElement& P );
}; // ListElement

/** 
 * @class List
 * @brief a generic list implementation for fgms
 * 
 */
template <class T>
class List : public Lockable
{
public:
	typedef std::vector<T> ListElements;
	typedef typename std::vector<T>::iterator ListIterator;
	/** constructor, must supply a Name */
	List   ( const std::string& Name );
	~List  ();
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
	ListIterator Find	( const netaddr& Address,
		bool ComparePort = false );
	/** find an element by its Name */
	ListIterator FindByName	( const std::string& Name = "" );
	/** find an element by its ID */
	ListIterator FindByID	( size_t ID );
	/** return an iterator of the first element */
	ListIterator Begin	();
	/** return an iterator of the last element */
	ListIterator Last	();
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
	std::string		Name;
private:
	/** @brief timestamp of last cleanup */
	time_t	LastRun;
	/** do not allow standard constructor */
	List ();
	/** the actual storage of elements */
	ListElements	Elements;
};

typedef List<ListElement>		FG_List;
typedef std::vector<ListElement>::iterator	ItList;

//////////////////////////////////////////////////////////////////////
/**
 *
 * construct an element and initialise counters
 */
template <class T>
List<T>::List
(
	const std::string& Name
)
{
	this->Name	= Name;
	PktsSent	= 0;
	BytesSent	= 0;
	PktsRcvd	= 0;
	BytesRcvd	= 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
List<T>::~List
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
List<T>::Size
()
{
	return Elements.size ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** thread safe
 *
 * Add an element to the list
 * @param Element the ListElement to add
 * @param TTL automatic expiry of the element (time-to-live)
 * @see Find()
 */
template <class T>
size_t
List<T>::Add
(
	T& Element,
	time_t TTL
)
{
	this->MaxID++;
	Element.ID	= this->MaxID;
	Element.Timeout	= TTL;
	Elements.push_back   ( Element );
	return this->MaxID;
}
//////////////////////////////////////////////////////////////////////


template <class T>
void 
List<T>::DeleteByPosition
(
	int position
)
{
	LockGuard lg ( & this->m_Mutex );
	ListIterator Element;
	this->LastRun = time (0);
	Element = Elements.begin();
	std::advance (Element,position);
	Elements.erase   ( Element );
}
//////////////////////////////////////////////////////////////////////
/** thread safe
 *
 * Delete an entry from the list
 * @param Element iterator pointing to the element to delete
 */
template <class T>
typename std::vector<T>::iterator
List<T>::Delete
(
	const ListIterator& Element
)
{
	LockGuard lg ( & this->m_Mutex );
	ListIterator E;
	E = Elements.erase   ( Element );
	return (E);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NOT thread safe
 *
 * Find an element by IP-address (and optionally Name). Automatically
 * removes entries which TTL is expired, with the exception of the 'users'
 * list which entries are deleted in FGMS::HandlePacket()
 * @param Address IP address of the element
 * @param ComparePort If true, element matches only if the port also
 *        matches
 * @return iterator pointing to the found element, or End() if element
 *         could not be found
 */
template <class T>
typename std::vector<T>::iterator
List<T>::Find
(
	const netaddr& Address,
	bool ComparePort
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
			if ( ComparePort ) 
			{	// additionally check port
				if ( Element->Address.port() == Address.port() )
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
typename std::vector<T>::iterator
List<T>::FindByName
(
	const std::string& Name
)
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
typename std::vector<T>::iterator
List<T>::FindByID
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
/** NOT thread safe
 *
 * In general iterators are not thread safe. If you use them you have
 * to ensure propper locking youself!
 *
 * @return the first element of the list. If the list is empty this
 *         equals End()
 */
template <class T>
typename std::vector<T>::iterator
List<T>::Begin
()
{
	return Elements.begin ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NOT thread safe
 *
 * In general iterators are not thread safe. If you use them you have
 * to ensure propper locking youself!
 *
 * @return the last element of the list. If the list is empty this
 *         equals End()
 */
template <class T>
typename std::vector<T>::iterator
List<T>::Last
()
{
	ListIterator RetElem = Elements.end ();
	--RetElem;
	return RetElem;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 *
 * @return the end of the list (which is not a valid entry!) 
 */
template <class T>
typename std::vector<T>::iterator
List<T>::End
()
{
	return Elements.end ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** thread safe
 * Check if the elements have TTL expired. 
 * True if not expired/unknown. False if expired.
 */
template <class T>
bool
List<T>::CheckTTL
(
	int position
)
{
	LockGuard lg ( & this->m_Mutex );
	ListIterator Element;
	this->LastRun = time (0);
	Element = Elements.begin();
	std::advance (Element,position);

	if (Element->Timeout == 0)
	{	// never timeouts
		return true;
	}
	if ( (this->LastRun - Element->LastSeen) > Element->Timeout )
	{
		  return false;
	}
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
List<T>::operator []
(
	const size_t& Index
)
{
	T RetElem("");
	if (Index < Elements.size ())
		RetElem = Elements[Index];
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
List<T>::UpdateSent
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
List<T>::UpdateRcvd
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
List<T>::UpdateSent
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
List<T>::UpdateRcvd
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
List<T>::Clear
()
{
	Lock ();
	Elements.clear ();
	Unlock ();
}
//////////////////////////////////////////////////////////////////////

} // namespace fgmp
#endif
