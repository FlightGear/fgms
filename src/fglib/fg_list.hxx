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
#include "fg_log.hxx"
#include "netaddr.hxx"
#include "fg_geometry.hxx"
#include "fg_thread.hxx"

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

/** 
 * @class list_item
 * @brief Represent a generic list element
 * 
 */
class list_item
{
public:
        list_item ( const std::string& name );
        list_item ( const list_item& P );
        static const size_t NONE_EXISTANT;      ///< mark a nonexisting element
        size_t          id;
        time_t          timeout;
        std::string     name;
        netaddr         address;
        time_t          join_time;
        time_t          last_seen;
        time_t          last_sent;
        uint64_t        pkts_rcvd;
        uint64_t        pkts_sent;
        uint64_t        bytes_rcvd;
        uint64_t        bytes_sent;
        void operator =  ( const list_item& P );
        virtual bool operator ==  ( const list_item& P );
        void update_sent ( size_t bytes );
        void update_rcvd ( size_t bytes );
protected:
        list_item ();
        void assign ( const list_item& P );
}; // list_item

//////////////////////////////////////////////////////////////////////

/** @ingroup fglib
 * @class list
 * @brief a generic list implementation for fgms
 * 
 */
template <class T>
class list : public lockable
{
public:
        using list_items = std::vector<T>;
        using list_it    = typename std::vector<T>::iterator;
        list   ( const std::string& name );
        ~list  ();
        size_t  size  ();
        void    clear ();
        size_t  add   ( T& item, time_t TTL );
        bool    check_ttl (int position);
        void    delete_by_pos (int position);
        list_it erase   ( const list_it& item );
        list_it find    ( const netaddr& address, bool compare_port = false );
        list_it find_by_name    ( const std::string& name = "" );
        list_it find_by_id      ( size_t id );
        list_it begin   ();
        list_it last    ();
        list_it end     ();
        void update_sent ( list_it& item, size_t bytes );
        void update_rcvd ( list_it& item, size_t bytes );
        void update_sent ( size_t bytes );
        void update_rcvd ( size_t bytes );
        T operator []   ( const size_t& Index );
        size_t          max_id		= 0;
        uint64_t        pkts_rcvd		= 0;
        uint64_t        pkts_sent		= 0;
        uint64_t        bytes_rcvd		= 0;
        uint64_t        bytes_sent		= 0;
        std::string     name;
private:
        list ();
        time_t          last_run;
        list_items      items;
};

using fglist   = list<list_item>;
using fglistit = std::vector<list_item>::iterator;

//////////////////////////////////////////////////////////////////////

/**
 * construct an element and initialise counters
 */
template <class T>
list<T>::list
(
        const std::string& name
) : name { name }
{
} // list<T>::list ()

//////////////////////////////////////////////////////////////////////

template <class T>
list<T>::~list
()
{
        clear ();
} // list<T>::~list ()

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
list<T>::size
()
{
        return items.size ();
} // list<T>::size ()

//////////////////////////////////////////////////////////////////////

/** thread safe
 *
 * add an element to the list
 * @param item the list_item to add
 * @param TTL automatic expiry of the element (time-to-live)
 * @see find()
 */
template <class T>
size_t
list<T>::add
(
        T& item,
        time_t TTL
)
{
        this->max_id++;
        item.id = this->max_id;
        item.timeout = TTL;
        items.push_back ( item );
        return this->max_id;
} // list<T>::add ()

//////////////////////////////////////////////////////////////////////

template <class T>
void 
list<T>::delete_by_pos
(
        int position
)
{
        lockguard lg { this->mutex() };

        this->last_run = time (0);
        list_it item;
        item = items.begin();
        std::advance ( item, position );
        items.erase  ( item );
} // list<T>::delete_by_pos ()

//////////////////////////////////////////////////////////////////////

/** thread safe
 *
 * erase an entry from the list
 * @param item iterator pointing to the element to delete
 */
template <class T>
typename std::vector<T>::iterator
list<T>::erase
(
        const list_it& item
)
{
        lockguard lg { this->mutex() };
        return items.erase ( item );
} // list<T>::erase ()

//////////////////////////////////////////////////////////////////////

/** NOT thread safe
 *
 * Find an element by IP-address (and optionally name). Automatically
 * removes entries which TTL is expired, with the exception of the 'users'
 * list which entries are deleted in FGMS::HandlePacket()
 * @param address IP address of the element
 * @param compare_port If true, element matches only if the port also
 *        matches
 * @return iterator pointing to the found element, or end() if element
 *         could not be found
 */
template <class T>
typename std::vector<T>::iterator
list<T>::find
(
        const netaddr& address,
        bool compare_port
)
{
        list_it item { items.begin() };
        list_it ret_item { items.end() };

        this->last_run = time (0);
        while (item != items.end())
        {
                if (item->address == address)
                {
                        if ( compare_port ) 
                        {       // additionally check port
                                if ( item->address.port() == address.port() )
                                {
                                        item->last_seen = this->last_run;
                                        ret_item = item;
                                }
                        }
                        else
                        {
                                item->last_seen = this->last_run;
                                ret_item = item;
                        }
                }
                item++;
        }
        return ret_item;
} // list<T>::find ()

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
list<T>::find_by_name
(
        const std::string& name
)
{
        list_it item { items.begin() };
        list_it ret_item { items.end() };

        this->last_run = time (0);
        while (item != items.end())
        {
                if (item->name == name)
                {
                        item->last_seen = this->last_run;
                        return item;
                }
                item++;
        }
        return ret_item;
} // list<T>::find_by_name ()

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
list<T>::find_by_id
(
        size_t id
)
{
        list_it item { items.begin() };
        list_it ret_item { items.end() };

        this->last_run = time (0);
        while (item != items.end())
        {
                if (item->id == id)
                {
                        ret_item = item;
                        break;
                }
                item++;
        }
        return ret_item;
} // list<T>::find_by_id ()

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
list<T>::begin
()
{
        return items.begin ();
} // list<T>::begin ()

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
list<T>::last
()
{
        list_it ret_item { items.end () };
        --ret_item;
        return ret_item;
} // list<T>::last ()

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 *
 * @return the end of the list (which is not a valid entry!) 
 */
template <class T>
typename std::vector<T>::iterator
list<T>::end
()
{
        return items.end ();
} // list<T>::end ()

//////////////////////////////////////////////////////////////////////

/** thread safe
 * Check if the elements have TTL expired. 
 * True if not expired/unknown. False if expired.
 */
template <class T>
bool
list<T>::check_ttl
(
        int position
)
{
        lockguard lg { this->mutex() };
        list_it item { items.begin() };

        this->last_run = time (0);
        std::advance (item,position);
        if (item->timeout == 0)
        {       // never timeouts
                return true;
        }
        if ( (this->last_run - item->last_seen) > item->timeout )
        {
                  return false;
        }
        return true;
} // list<T>::check_ttl ()

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
list<T>::operator []
(
        const size_t& Index
)
{
        T ret_item {""};
        if (Index < items.size ())
                ret_item = items[Index];
        return ret_item;
} // list<T>::operator []

//////////////////////////////////////////////////////////////////////

/**
 *
 * Update sent-counters of the list and of an element. Every call of
 * this method means we sent a single packet (and the packet counter
 * is increased by 1).
 * @param item The element to which data was sent
 * @param bytes The number of bytes sent
 */
template <class T>
void
list<T>::update_sent
(
        list_it& item,
        size_t bytes
)
{
        pkts_sent++;
        bytes_sent += bytes;
        item->update_sent (bytes);
} // list<T>::update_sent ()

//////////////////////////////////////////////////////////////////////

/**
 *
 * Update receive-counters of the list and of an element. Every call of
 * this method means we received a single packet (and the packet counter
 * is increased by 1).
 * @param item The element from which data was received
 * @param bytes The number of bytes received
 */
template <class T>
void
list<T>::update_rcvd
(
        list_it& item,
        size_t bytes
)
{
        pkts_rcvd++;
        bytes_rcvd += bytes;
        item->update_rcvd (bytes);
} // list<T>::update_rcvd ()

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
list<T>::update_sent
(
        size_t bytes
)
{
        pkts_sent++;
        bytes_sent += bytes;
} // list<T>::update_sent ()

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
list<T>::update_rcvd
(
        size_t bytes
)
{
        pkts_rcvd++;
        bytes_rcvd += bytes;
} // list<T>::update_rcvd ()

//////////////////////////////////////////////////////////////////////

/**
 *
 * erase all entries of the list.
 */
template <class T>
void
list<T>::clear
()
{
        lock ();
        items.clear ();
        unlock ();
} // list<T>::clear()

//////////////////////////////////////////////////////////////////////

} // namespace fgmp
#endif
