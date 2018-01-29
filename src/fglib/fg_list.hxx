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
	ListElement ( const std::string& name );
	ListElement ( const ListElement& P );
	virtual ~ListElement ();
	/** mark a nonexisting element */
	static const size_t NONE_EXISTANT;
	/** @brief The id of this entry */
	size_t		id;
	/** @brief The timeout of this entry */
	time_t		timeout;
	/** @brief The callsign (or name) */
	std::string	name;
	/** @brief The network address of this element */
	netaddr		address;
	/** @brief The time this entry was added to the list */
	time_t		join_time;
	/** @brief timestamp of last seen packet from this element */
	time_t		last_seen;
	/** @brief timestamp of last sent packet to this element */
	time_t		last_sent;
	/** @brief Count of packets recieved from client */
	uint64_t	pkts_rcvd;
	/** @brief Count of packets sent to client */
	uint64_t	pkts_sent;
	/** @brief Count of bytes recieved from client */
	uint64_t	bytes_rcvd;
	/** @brief Count of bytes sent to client */
	uint64_t	bytes_sent;
	void operator =  ( const ListElement& P );
	virtual bool operator ==  ( const ListElement& P );
	void update_sent ( size_t bytes );
	void update_rcvd ( size_t bytes );
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
	/** constructor, must supply a name */
	List   ( const std::string& name );
	~List  ();
	/** return the number of elements in this list */
	size_t size  ();
	/** delete all elements of this list */
	void   clear ();
	/** add an element to this list */
	size_t add   ( T& Element, time_t TTL );
	/** Check if the entry TTL expired*/
	bool check_ttl (int position);
	/** delete an element of this list (by position) */
	void delete_by_pos (int position);
	/** delete an element of this list */
	ListIterator erase	( const ListIterator& Element );
	/** find an element by its IP address */
	ListIterator find	( const netaddr& address,
		bool ComparePort = false );
	/** find an element by its name */
	ListIterator find_by_name	( const std::string& name = "" );
	/** find an element by its id */
	ListIterator find_by_id	( size_t id );
	/** return an iterator of the first element */
	ListIterator begin	();
	/** return an iterator of the last element */
	ListIterator last	();
	/** iterator of the end of the list */
	ListIterator end	();
	/** update sent counter of an element and of the list */
	void update_sent ( ListIterator& Element, size_t bytes );
	/** update receive counter of an element and of the list */
	void update_rcvd ( ListIterator& Element, size_t bytes );
	/** update sent counter of the list */
	void update_sent ( size_t bytes );
	/** update receive counter of the list */
	void update_rcvd ( size_t bytes );
	/** return a copy of an element at position x (thread safe) */
	T operator []( const size_t& Index );
	/** @brief maximum entries this list ever had */
	size_t		max_id;
	/** @brief Count of packets recieved from client */
	uint64_t	pkts_rcvd;  
	/** @brief Count of packets sent to client */
	uint64_t	pkts_sent;
	/** @brief Count of bytes recieved from client */
	uint64_t	bytes_rcvd;  
	/** @brief Count of bytes sent to client */
	uint64_t	bytes_sent;        
	/** the name (or description) of this element */
	std::string	name;
private:
	/** @brief timestamp of last cleanup */
	time_t	last_run;
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
	const std::string& name
)
{
	this->name	= name;
	pkts_sent	= 0;
	bytes_sent	= 0;
	pkts_rcvd	= 0;
	bytes_rcvd	= 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
template <class T>
List<T>::~List
()
{
	clear ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NOT thread safe
 *
 * Return the number of elements of this list.
 * If working with threads make sure to * use lock() and unlock()
 * @see lock()
 * @see unlock()
 */
template <class T>
size_t
List<T>::size
()
{
	return Elements.size ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** thread safe
 *
 * add an element to the list
 * @param Element the ListElement to add
 * @param TTL automatic expiry of the element (time-to-live)
 * @see find()
 */
template <class T>
size_t
List<T>::add
(
	T& Element,
	time_t TTL
)
{
	this->max_id++;
	Element.id	= this->max_id;
	Element.timeout	= TTL;
	Elements.push_back   ( Element );
	return this->max_id;
}
//////////////////////////////////////////////////////////////////////


template <class T>
void 
List<T>::delete_by_pos
(
	int position
)
{
	LockGuard lg ( & this->m_mutex );
	ListIterator Element;
	this->last_run = time (0);
	Element = Elements.begin();
	std::advance (Element,position);
	Elements.erase   ( Element );
}
//////////////////////////////////////////////////////////////////////
/** thread safe
 *
 * erase an entry from the list
 * @param Element iterator pointing to the element to delete
 */
template <class T>
typename std::vector<T>::iterator
List<T>::erase
(
	const ListIterator& Element
)
{
	LockGuard lg ( & this->m_mutex );
	ListIterator E;
	E = Elements.erase   ( Element );
	return (E);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/** NOT thread safe
 *
 * Find an element by IP-address (and optionally name). Automatically
 * removes entries which TTL is expired, with the exception of the 'users'
 * list which entries are deleted in FGMS::HandlePacket()
 * @param address IP address of the element
 * @param ComparePort If true, element matches only if the port also
 *        matches
 * @return iterator pointing to the found element, or end() if element
 *         could not be found
 */
template <class T>
typename std::vector<T>::iterator
List<T>::find
(
	const netaddr& address,
	bool ComparePort
)
{
	ListIterator Element;
	ListIterator RetElem;

	this->last_run = time (0);
	RetElem = Elements.end();
	Element = Elements.begin();
	while (Element != Elements.end())
	{
		if (Element->address == address)
		{
			if ( ComparePort ) 
			{	// additionally check port
				if ( Element->address.port() == address.port() )
				{
					Element->last_seen = this->last_run;
					RetElem = Element;
				}
			}
			else
			{
				Element->last_seen = this->last_run;
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
 * Find an element by name.
 * @param name The name (or description) of the element
 * @return iterator pointing to the found element, or end() if element
 *         could not be found
 */
template <class T>
typename std::vector<T>::iterator
List<T>::find_by_name
(
	const std::string& name
)
{
	ListIterator Element;
	ListIterator RetElem;

	this->last_run = time (0);
	RetElem = Elements.end();
	Element = Elements.begin();
	while (Element != Elements.end())
	{
		if (Element->name == name)
		{
			Element->last_seen = this->last_run;
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
 * Find an element by id.
 * @param id The id of the element
 * @return iterator pointing to the found element, or end() if element
 *         could not be found
 */
template <class T>
typename std::vector<T>::iterator
List<T>::find_by_id
(
	size_t id
)
{
	ListIterator Element;
	ListIterator RetElem;
	this->last_run = time (0);
	RetElem = Elements.end();
	Element = Elements.begin();
	while (Element != Elements.end())
	{
		if (Element->id == id)
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
 *         equals end()
 */
template <class T>
typename std::vector<T>::iterator
List<T>::begin
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
 *         equals end()
 */
template <class T>
typename std::vector<T>::iterator
List<T>::last
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
List<T>::end
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
List<T>::check_ttl
(
	int position
)
{
	LockGuard lg ( & this->m_mutex );
	ListIterator Element;
	this->last_run = time (0);
	Element = Elements.begin();
	std::advance (Element,position);

	if (Element->timeout == 0)
	{	// never timeouts
		return true;
	}
	if ( (this->last_run - Element->last_seen) > Element->timeout )
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
 * thread safe and make sure to use lock() and unlock() yourself.
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
List<T>::update_sent
(
	ListIterator& Element,
	size_t bytes
)
{
	pkts_sent++;
	bytes_sent += bytes;
	Element->update_sent (bytes);
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
List<T>::update_rcvd
(
	ListIterator& Element,
	size_t bytes
)
{
	pkts_rcvd++;
	bytes_rcvd += bytes;
	Element->update_rcvd (bytes);
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
List<T>::update_sent
(
	size_t bytes
)
{
	pkts_sent++;
	bytes_sent += bytes;
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
List<T>::update_rcvd
(
	size_t bytes
)
{
	pkts_rcvd++;
	bytes_rcvd += bytes;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 *
 * erase all entries of the list.
 */
template <class T>
void
List<T>::clear
()
{
	lock ();
	Elements.clear ();
	unlock ();
}
//////////////////////////////////////////////////////////////////////

} // namespace fgmp
#endif
