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

/**
 * Add a mutex to an abitrary class.
 *
 * Example:
 * @code
 * class my_class : public fgmp::Lockable
 * {
 * 	...
 * }
 * @endcode
 *
 * will add a mutex to \c my_class. Additionally adds to methods
 * \c Lock() and \c Unlock()
 */
class Lockable
{
public:
	/// initialise the mutex
	Lockable ();
	/// lock the mutex
	void Lock ();
	/// unlock the mutex
	void Unlock ();
protected:
	pthread_mutex_t m_Mutex;
}; // class Lockable

/**
 * Guard a mutex.
 *
 * The constructor locks the provided mutex, the destructor
 * automatically unlocks it.
 *
 * Example:
 * @code
 *
 * pthread_mutex_t m;
 *
 * void my_func ()
 * {
 * 	LockGuard g ( &m );
 *	// do stuff
 * }
 * @endcode
 *
 * The mutex gets automatically unlocked when the my_func() ends.
 *
 */
class LockGuard
{
public:
	LockGuard ( pthread_mutex_t *Mutex );
	~LockGuard ();
protected:
	pthread_mutex_t *m_Mutex;
}; // class LockGuard

template<class T>
class lock_list_t: public std::list<T>, public Lockable
{ }; // class lock_list_t

typedef lock_list_t<std::string>           StrList;
typedef lock_list_t<std::string>::iterator StrIt;

} // namespace fgmp

#endif
