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
/// @file fg_log.cxx
/// flightgear list server
///
/// @author	Oliver Schroeder <fgms@o-schroeder.de>
/// @date	2008-2015
///

#include "fg_log.hxx"

fgmp::fglog logger;

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

/** Create a standard fglog
 *
 * Default priority is \c MEDIUM
 * @see fglog::priority
 * @see fglog::init
 */
fglog::fglog
() : m_logstream(0), m_logfile(0)
{
	init ();
} // fglog::fglog ()

//////////////////////////////////////////////////////////////////////

/** Create a fglog and set the output priority
 *
 * @param p log only messages with priority \c p
 * @see fglog::priority
 * @see fglog::init
 */
fglog::fglog
(
	int p
) : m_logstream(0), m_logfile(0)
{
	init ();
	m_priority = p;
} // fglog::fglog (int)

//////////////////////////////////////////////////////////////////////

/** Create a fglog and log all messages into a file
 *
 * @param name The logfile to use
 * @see fglog::init
 */
fglog::fglog
(
	std::string name
) : m_logstream(0), m_logfile(0)
{
	init ();
	open ( name );
} // fglog::fglog ( name )

//////////////////////////////////////////////////////////////////////

/** Create a fglog and log all messages into a file
 * and use priotity \c p
 *
 * @param name The logfile to use
 * @param p log only messages with priority \c p
 * @see fglog::priority
 * @see fglog::init
 */
fglog::fglog
(
	std::string name,
	int p
) : m_logstream(0), m_logfile(0)
{
	init ();
	m_priority = p;
	open ( name );
} // fglog::fglog ( name, priority )

//////////////////////////////////////////////////////////////////////

/** Close the logstream and all opened files.
 */
fglog::~fglog
()
{
	close ();
} // fglog::~fglog ()

//////////////////////////////////////////////////////////////////////

/** Initialise to default values.
 *
 * defaults are:
 * - log with date
 * - output priority is 'MEDIUM'
 * - use std::cout as output stream
 */
void
fglog::init
()
{
	m_flags		= WITH_DATE;
	m_priority	= MEDIUM;
	m_outprio	= MEDIUM;
	m_logstream	= new std::ostream ( std::cout.rdbuf () );
	m_logstreambuf	= m_logstream->rdbuf ();
	m_logbufsize	= 200;
	m_date		= false;
} // fglog::init()

//////////////////////////////////////////////////////////////////////

/** Open (and use) a logfile
 *
 * @param name The logfile to use
 */
bool
fglog::open
(
	std::string name
)
{
	m_logname = name;
	if ( m_logfile && (*m_logfile) )
	{
		m_logstream->rdbuf ( m_logstreambuf );
		m_logfile->close ();
	}
	m_logstreambuf = m_logstream->rdbuf ();
	if ( ! m_logfile )
	{
		m_logfile = new std::ofstream ( name.c_str(),
		  std::ios::out|std::ios::app );
	}
	else
	{
		m_logfile->open ( name.c_str(), std::ios::out|std::ios::app );
	}
	if ( ! m_logfile->is_open() )
		return false;
	m_logstream->rdbuf ( m_logfile->rdbuf () );
	return true;
} // fglog::open ()

//////////////////////////////////////////////////////////////////////

/** Close all opened logstreams
 */
void
fglog::close
()
{
	if ( m_logfile && (*m_logfile) )
	{
		m_logfile->close ();
	}
	m_logname = "";
	if ( m_logstream )
	{
		m_logstream->rdbuf ( std::cout.rdbuf () );
	}
	if ( m_logstreambuf && m_logstream )
	{
		m_logstreambuf = m_logstream->rdbuf ();
	}
	if ( m_logstream )
	{
		delete m_logstream;
		m_logstream = 0;
	}
	if ( m_logfile )
	{
		delete m_logfile;
		m_logfile = 0;
	}
	m_outprio	= NONE;
} // fglog::close ()

//////////////////////////////////////////////////////////////////////

/** set output priority for following log requests.
 *
 * Only messages with a priority euqal to or higher \c p will be
 * logged.
 */
void
fglog::priority
(
	int p
)
{
	m_priority = p;
} // fglog::priority (p)

//////////////////////////////////////////////////////////////////////

/** return the current priority
 */
int
fglog::priority
() const
{
	return m_priority;
} // fglog::priority (p)

//////////////////////////////////////////////////////////////////////

/** set flags for following log requests.
 * @see fglog::logflags
 */
void
fglog::flags
(
	logflags f
)
{
	m_flags = f;
} // fglog::flags(f)

//////////////////////////////////////////////////////////////////////

/** flush the logstream
 */
void
fglog::flush
()
{
	m_logstream->flush ();
} // fglog::flush()

//////////////////////////////////////////////////////////////////////

/** return the name of the current logfile (if any)
 */
std::string
fglog::get_name
() const
{
	return m_logname;
} // fglog::get_name()

//////////////////////////////////////////////////////////////////////

/** return true if there is a logfile in use
 */
bool
fglog::is_open
() const
{
	return m_logfile ? m_logfile->is_open() : false;
} // fglog::is_open()

//////////////////////////////////////////////////////////////////////

/** Return the internal buffer  of logged messages.
 *
 * All logged messages are stored in an internal buffer which can
 * be accessed using this method.
 * Used in *_cli to be able to view the log messages within the cli.
 *
 * Example:
 * @code
 * fgmp::StrList*  buf = logger.logbuf();
 * fgmp::StrIt     it;
 * buf->Lock ();	// lock the buffer !
 * for ( it = buf->begin(); it != buf->end(); it++ )
 * {
 * 	...
 * }
 * buf->Unlock ();
 * @endcode
 *
 * @note make sure to lock the logbuf before using it, to ensure no lines
 * are deleted while accessing it.
 */
fgmp::StrList*
fglog::logbuf
()
{
	return &m_logbuf;
} // fglog::logbuf()

//////////////////////////////////////////////////////////////////////

/** Set the size of our internal buffer.
 *
 * @param size	lines to keep in the internal buffer.
 */
void
fglog::setbufsize
(
	size_t size
)
{
	m_logbufsize = size;
} // fglog::setbufsize(s)

//////////////////////////////////////////////////////////////////////

/** log something to the logstream
 *
 * The message is logged with the last priority used.
 *
 * Example:
 *
 * @code
 * fgmp::fglog log;
 *
 * log.log ( fglog::HIGH ) << "Output with priority HIGH" << fgmp::endl;
 * log.log () << "Output with priority HIGH, too" << fgmp::endl;
 * @endcode
 */
fglog&
fglog::log
()
{
	m_outprio = m_priority;
	if ( m_flags & WITH_DATE )
	{
		m_date = true;
		(*this) << datestr ();
		m_date = false;
	}
	return *this;
} // fglog::log ()

//////////////////////////////////////////////////////////////////////

/** log something to the logstream
 *
 * The message is logged with the priority provided.
 *
 * @param p the priority to use
 *
 * Example:
 *
 * @code
 * fgmp::fglog log;
 *
 * log.log ( fglog::HIGH ) << "Output with priority HIGH" << fgmp::endl;
 * @endcode
 */
fglog&
fglog::log
(
	int p
)
{
	m_outprio = p;
	if ( m_flags & WITH_DATE )
	{
		m_date = true;
		(*this) << datestr ();
		m_date = false;
	}
	return *this;
} // fglog::log ( priority )

//////////////////////////////////////////////////////////////////////

/** log something to the logstream
 *
 * The message is provided in a format string and logged with the
 * priority provided. endl is automatically appended.
 *
 * @param p the priority to use
 *
 * Example:
 *
 * @code
 * fgmp::fglog log;
 *
 * log.log ( fglog::HIGH, "temperature: %d", temperature );
 * @endcode
 *
 */
void
fglog::log
(
	int p,
	const char* format,
	...
)
{
	if ( ! format )
	{
		return;
	}
	va_list vl;
	va_start ( vl,format );
	log(p) << logfmt ( p, format,vl ) << fgmp::endl;
	va_end(vl);
} // fglog::log ( priority, fmt )

//////////////////////////////////////////////////////////////////////

/** internal, handling of format strings
 */
std::string
fglog::logfmt
(
	int p,
	const char* format,
	va_list vl
)
{
	char buf[5000];
	m_outprio = p;
	vsprintf ( buf, format, vl );
	return static_cast<std::string> (buf);
} // fglog::logfmt ()

//////////////////////////////////////////////////////////////////////

/** internal, generate a string representing the current time and date.
 */
std::string
fglog::datestr
()
{
	time_t		t;
	struct tm*	tmr;
	char		buf[41];

	t = time ( 0 );
	tmr = localtime ( &t );
	sprintf (buf, "%02d.%02d.%04d %02d:%02d:%02d ",
		tmr->tm_mday,
		tmr->tm_mon+1,
		tmr->tm_year+1900,
		tmr->tm_hour,
		tmr->tm_min,
		tmr->tm_sec);
	return static_cast<std::string> (buf);
}

//////////////////////////////////////////////////////////////////////

/** internal, put the logged message into our internal buffer of
 * log messages.
 */
void
fglog::commit
()
{
	while ( m_logbuf.size() > m_logbufsize )
	{
		m_logbuf.pop_front ();
	}
	std::string s = m_logline.str ();
	m_logbuf.push_back ( s );
	m_logline.str( "" );
	if ( m_logstream )
		m_logstream->flush();
}

//////////////////////////////////////////////////////////////////////

/** put a newline character on the stream and the complete string into
 * the internal buffer of log messages.
 */
fglog&
endl
(
	fglog& l
)
{
	l << "\r\n";
	l.commit ();
	return l;
} // endl ()

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

