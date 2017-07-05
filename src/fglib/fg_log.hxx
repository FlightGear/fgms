//
// Copyright (C) Oliver Schroeder <fgms@postrobot.de>
//
// This file is part of fgms, the flightgear multiplayer server
//
// fgms is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// fgms is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fgms.  If not, see <http://www.gnu.org/licenses/>
//

///
/// @file fg_log.hxx
/// fgms logging utility
///
/// @author	Oliver Schroeder <fgms@o-schroeder.de>
/// @date	2003
/// @copyright	GPLv3
///

#ifndef FG_LOGOBJECT
#define FG_LOGOBJECT

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdarg.h>
#include "fg_thread.hxx"

namespace log
{

enum priority
{
	NONE,
	HIGH,		// error messages
	MEDIUM,		// standard messages
	DEBUG,		// debug messages
	URGENT	= 64,	// log always, regardless of logging level
	CONSOLE	= 128,	// log to terminal (too)
	ERROR	= 192	// = URGENT | CONSOLE
};
enum logflags
{
	NO_DATE,
	WITH_DATE
};

class logobject : public fgmp::Lockable
{
public:
	logobject ();
	logobject ( int p );
	logobject ( std::string name );
	logobject ( std::string name, int p );
	~logobject ();
	template <class any_type> logobject &operator << (any_type var);
	logobject& operator << (logobject& (*pf) (logobject&))
	  { return ((*pf)(*this)); };
	operator void* () const { return (m_logfile? (void*) 1: (void*) 0); };
	bool open ( std::string name );
	void close ();
	inline void priority ( int p ) { m_priority = p; };
	inline int  priority () { return m_priority; };
	inline void flags ( logflags f ) { m_flags = f; };
	inline void flush () { m_logstream->flush (); };
	inline std::string get_name () { return m_logname; };
	inline bool is_open () const
	   { return m_logfile ? m_logfile->is_open() : false; };
	logobject& log ();
	logobject& log ( int p );
	void log ( int p, const char *format, ... );
	void commit ();
	// make sure to lock the logbuf before using it
	fgmp::StrList* logbuf() { return &m_logbuf; };
	inline void setbufsize ( size_t size ) { m_logbufsize = size; };
	friend logobject& endl ( logobject& l );
private:
	logobject ( const logobject& X ); // not available
	void init ();
	std::string logfmt ( int p,const char *format, va_list ap );
	std::string datestr ( void );
	int		m_priority;	// we want to see this prio in our logs
	int		m_outprio;	// current output uses this prio
	logflags	m_flags;
	std::ostream*	m_logstream;
	std::ofstream*	m_logfile;
	std::streambuf*	m_logstreambuf;
	std::string	m_logname;
	std::stringstream m_logline;
	fgmp::StrList	m_logbuf;
	size_t		m_logbufsize;
	bool		m_date;	// true when date is to be put on stream
}; // class logobject

template <class any_type>
logobject&
logobject::operator << ( any_type v )
{
	int p_copy = m_outprio;

	if ( p_copy & CONSOLE )	// print on screen
	{
		p_copy &= ~CONSOLE;
		if ( ! m_date )
			std::cout << v;
		if ( p_copy == 0 )
		{
			return *this;
		}
	}
	if ( p_copy & URGENT )	// print on screen
	{
		p_copy = m_priority;
	}
	if ( p_copy > m_priority )
	{
		return *this;	// do not log anything
	}
	if ( ! is_open() )
		return *this;
	if ( m_logstream && (*m_logstream) )
	{
		(*m_logstream) << v;
		m_logline << v;
	}
	return *this;
}

logobject& endl ( logobject& l );

} // namespace log

/// define a global logobject
extern log::logobject logger; // logobject.cxx

#define LOG(P,M) { \
	logger.Lock (); \
	logger.log (P) << M << log::endl; \
	logger.Unlock (); \
}

#endif

