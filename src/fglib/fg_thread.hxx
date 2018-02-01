/**
 * @file fg_thread.hxx
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
// Copyright (C) 2017  Oliver Schroeder
//

//////////////////////////////////////////////////////////////////////
//
//      Classes for thread safety
//
//////////////////////////////////////////////////////////////////////

#ifndef FG_THREAD_HEADER
#define FG_THREAD_HEADER

#include <list>
#include <string>
#include <pthread.h>

namespace fgmp
{

using mutex_t = pthread_mutex_t;

/**
 * @brief Add a mutex to an abitrary class.
 *
 * Example:
 * @code
 * class my_class : public fgmp::lockable
 * {
 * 	...
 * }
 * @endcode
 *
 * will add a mutex to \c my_class. Additionally adds two methods
 * \c lock() and \c unlock()
 */
class lockable
{
public:
	lockable ();
	void lock ();
	void unlock ();
	inline mutex_t* mutex () { return & m_mutex; };
private:
	mutex_t m_mutex;
}; // class lockable

/**
 * @brief Guard a mutex.
 *
 * The constructor locks the provided mutex, the destructor
 * automatically unlocks it.
 *
 * Example:
 * @code
 *
 * mutex_t m;
 *
 * void my_func ()
 * {
 * 	lockguard g ( &m );
 *	// do stuff
 * }
 * @endcode
 *
 * The mutex gets automatically unlocked when the my_func() ends.
 *
 * @note The mutex is expected to be already initialised.
 */
class lockguard
{
public:
	lockguard ( mutex_t *mutex );
	~lockguard ();
protected:
	mutex_t *m_mutex;
}; // class lockguard

/**
 * @brief a std::list<> protected by lockable
 */
template<class T>
class lock_list_t: public std::list<T>, public lockable
{ }; // class lock_list_t

/**
 * @brief a lock_list<string> 
 */
using str_list = lock_list_t<std::string>;
/**
 * @brief an iterator for a lock_list_t<std::string>
 */
using str_it = lock_list_t<std::string>::iterator;

inline str_list::iterator begin ( str_list * l ) { return l->begin(); }
inline str_list::iterator end   ( str_list * l ) { return l->end(); }

} // namespace fgmp

#endif
