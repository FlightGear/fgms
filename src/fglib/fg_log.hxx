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
/// @author     Oliver Schroeder <fgms@o-schroeder.de>
/// @date       2003
///

#ifndef FG_LOGOBJECT
#define FG_LOGOBJECT

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdarg.h>
#include <cstdint>
#include "fg_thread.hxx"

namespace fgmp
{

/**@ingroup fglib
 * @brief Provide a logging class for fgms
 *
 * The purpose is to have logging priorities and only really log messages
 * of the priority we want to see. The 'output' priority can be changed
 * at runtime.
 * Additionally all logged messages are stored in a buffer, which can be
 * accessed from the outside.
 */
class fglog : public fgmp::lockable
{
public:
        /// The available priorities
        enum class prio
        {
                NONE,           ///< do not log at all
                HIGH,           ///< error messages
                MEDIUM,         ///< standard messages
                DEBUG,          ///< debug messages
                URGENT  = 64,   ///< log always, regardless of logging level
                CONSOLE = 128,  ///< log to terminal (too)
                EMIT    = 192   ///< = URGENT | CONSOLE
        };
        /// Define whether logged lines should be prefixed with a date or not.
        /// Default is with date.
        enum class flags
        {
                NO_DATE,
                WITH_DATE
        };

        fglog ();
        fglog ( const fglog& ) = delete; // disable copy constructor
        fglog ( prio p );
        fglog ( std::string name );
        fglog ( std::string name, prio p );
        ~fglog ();
        template <class any_type> fglog &operator << (any_type var);
        // black c++ magic
        fglog& operator << ( fglog& (*pf) ( fglog& ))
          { return ((*pf)(*this)); };
        operator void* () const { return (m_logfile? (void*) 1: (void*) 0); };
        bool open ( std::string name );
        void close ();
        void priority ( prio p );
        int  priority () const;
        void set_flags ( flags f );
        void flush ();
        std::string get_name () const;
        bool is_open () const;
        fgmp::str_list* logbuf();
        void setbufsize ( size_t size );
        fglog& log ();
        fglog& log ( prio p );
        void log ( prio p, const char *format, ... );
        static fglog& endl ( fglog& l );
private:
        void init ();
        std::string logfmt ( prio p,const char *format, va_list ap );
        std::string datestr ( void );
        prio    m_priority;     // we want to see this prio in our logs
        prio    m_outprio;      // current output uses this prio
        fglog::flags    m_flags;
        std::ostream*   m_logstream;
        std::ofstream*  m_logfile;
        std::streambuf* m_logstreambuf;
        std::string     m_logname;
        std::stringstream m_logline;
        fgmp::str_list  m_logbuf;
        size_t          m_logbufsize;
        bool            m_date; // true when date is to be put on stream
}; // class fglog

fglog::prio make_prio ( int i );

/**
 * @brief Output operator for log
 *
 * This method evaluates if the output is really put on the stream,
 * depending on the current output priority and the output priority
 * defined by the application.
 */
template <class any_type>
fglog&
fglog::operator << ( any_type v )
{
        int p_copy = int(m_outprio);


        if ( ( m_logfile != nullptr )           // don't have a logfile
        &&   ( p_copy & int(prio::CONSOLE) ) )  // print on screen
        {
                p_copy &= ~int (prio::CONSOLE);
                if ( ! m_date )
                        std::cout << v;
                if ( p_copy == 0 )
                {
                        return *this;
                }
        }
        if ( p_copy & int(prio::URGENT) )       // print on screen
        {
                p_copy = int(m_priority);
        }
        if ( p_copy > int(m_priority) )
        {
                return *this;   // do not log anything
        }
        if ( m_logstream && (*m_logstream) )
        {
                (*m_logstream) << v;
                m_logline << v;
        }
        return *this;
}

} // namespace fgmp

/**
 * @brief define a global log
 *
 * This object is globally accessible by all. If you need a second
 * logfile, use a second fglog::fglog
 */
extern fgmp::fglog logger; // log.cxx

/**
 * @brief Define a logging macro for convinience
 *
 * Example:
 * @code
 * LOG ( fglog::DEBUG,  "log me" );
 * @endcode
 */
#define LOG(P,M) { \
        logger.lock (); \
        logger.log (P) << M << fgmp::fglog::endl; \
        logger.unlock (); \
}

#endif

