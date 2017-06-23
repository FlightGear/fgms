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
// Copyright (C) 2005  Oliver Schroeder
//

//////////////////////////////////////////////////////////////////////
//
//      Classes for thread safety
//
//////////////////////////////////////////////////////////////////////

#ifndef FG_THREAD_HEADER
#define FG_THREAD_HEADER

#include <pthread.h>

namespace fgmp
{

class Lockable
{
public:
	Lockable ();
	void Lock ();
	void Unlock ();
protected:
	pthread_mutex_t m_Mutex;
}; // class Lockable

class LockGuard
{
public:
	LockGuard ( pthread_mutex_t *Mutex );
	~LockGuard ();
protected:
	pthread_mutex_t *m_Mutex;
}; // class LockGuard

} // namespace fgmp

#endif
