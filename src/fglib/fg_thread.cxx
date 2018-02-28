/**
 * @file fg_thread.cxx
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
// Copyright (C) 2005  Oliver Schroeder
//

//////////////////////////////////////////////////////////////////////
//
//      Classes for thread safety
//
//////////////////////////////////////////////////////////////////////

#include "fg_thread.hxx"

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

/** Initialise the mutex
 */
lockable::lockable
()
{
//        pthread_mutex_init ( & m_mutex, 0 );
} // lockable::lockable ()

//////////////////////////////////////////////////////////////////////

/** Lock the mutex
 */
void
lockable::lock
()
{
//        pthread_mutex_lock ( & m_mutex );
        m_mutex.lock();
} // lockable::lock ()

//////////////////////////////////////////////////////////////////////

/** Unlock the mutex
 */
void 
lockable::unlock
()
{
        //pthread_mutex_unlock ( & m_mutex );
        m_mutex.unlock();
} // lockable::unlock ()

//////////////////////////////////////////////////////////////////////

#if 0
/** Lock the mutex
 */
lockguard::lockguard
(
        mutex_t *mutex
)
{
        m_mutex = mutex;
        //pthread_mutex_lock ( m_mutex );
        m_mutex->lock ();
} // lockguard::lockguard ()

//////////////////////////////////////////////////////////////////////

/** Unlock the mutex
 */
lockguard::~lockguard
()
{
        //pthread_mutex_unlock ( m_mutex );
        m_mutex->unlock ();
} // lockguard::~lockguard ()

//////////////////////////////////////////////////////////////////////
#endif

} // namespace fgmp

