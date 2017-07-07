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
/// @copyright	GPLv3
///
/// This is the list server for the flightgear multiplayer
/// network. All servers register at this server. All clients ask fgls
/// which server they should use.
///

#include "fg_log.hxx"

fgmp::fglog logger;

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

fglog::fglog
() : m_logstream(0), m_logfile(0)
{
	init ();
} // fglog::fglog ()

//////////////////////////////////////////////////////////////////////

fglog::fglog
(
	int p
) : m_logstream(0), m_logfile(0)
{
	init ();
	m_priority = p;
} // fglog::fglog (int)

//////////////////////////////////////////////////////////////////////

fglog::fglog
(
	std::string name
) : m_logstream(0), m_logfile(0)
{
	init ();
	open ( name );
} // fglog::fglog ( name )

//////////////////////////////////////////////////////////////////////

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

fglog::~fglog
()
{
	close ();
} // fglog::~fglog ()

//////////////////////////////////////////////////////////////////////

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
	log(p) << logfmt ( p, format,vl ) << endl;
	va_end(vl);
} // fglog::log ( priority, fmt )

//////////////////////////////////////////////////////////////////////

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

