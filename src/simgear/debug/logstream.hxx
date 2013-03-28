/** 
 * @file logstream.hxx
 * @author  Bernie Bright, 1998
 * @brief Stream based logging mechanism.
 * 
 */

// Written by Bernie Bright, 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id: logstream.hxx,v 1.2 2010/02/15 08:04:17 oliver Exp $

#ifndef _LOGSTREAM_H
#define _LOGSTREAM_H

#include <simgear/compiler.h>

#ifdef _MSC_VER
#  include <windows.h>
#endif

#ifdef SG_HAVE_STD_INCLUDES
# include <streambuf>
# include <iostream>
#else
# include <iostream.h>
# include <simgear/sg_traits.hxx>
#endif

#include <cstdio>
#include <simgear/debug/debug_types.h>

SG_USING_STD(streambuf);
SG_USING_STD(ostream);
SG_USING_STD(cout);
SG_USING_STD(cerr);
SG_USING_STD(endl);
SG_USING_STD(string);

#ifdef __MWERKS__
SG_USING_STD(iostream);
#endif

//
// TODO:
//
// 1. Change output destination. Done.
// 2. Make logbuf thread safe.
// 3. Read environment for default debugClass and debugPriority.
//

/**
 * @brief The user can provide a function which returns a date as a string
 */
typedef string (*DATEFUNC)(void);

/**
 * @class logbuf
 * @brief logbuf is an output-only streambuf 
 * 
 * logbuf is an output-only streambuf with the ability to disable sets of
 * messages at runtime. Only messages with priority >= logbuf::logPriority
 * and debugClass == logbuf::logClass are output.
 */
#ifdef SG_NEED_STREAMBUF_HACK
class logbuf : public __streambuf
#else
class logbuf : public streambuf
#endif
{
public:

#ifndef SG_HAVE_STD_INCLUDES
    typedef char_traits<char>           traits_type;
    typedef char_traits<char>::int_type int_type;
    // typedef char_traits<char>::pos_type pos_type;
    // typedef char_traits<char>::off_type off_type;
#endif
    // logbuf( streambuf* sb ) : sbuf(sb) {}
    /** @brief Constructor */
    logbuf();

    /** @brief  Destructor */
    ~logbuf();

    /**
     * @brief Is logging enabled?
     * @return true or false*/
    bool enabled() { return logging_enabled; }

    /**
     * @brief Set the logging level of subsequent messages.
     * @param c debug class
     * @param p priority
     */
    void set_log_state( sgDebugClass c, sgDebugPriority p );

    /**
     * @brief Set the global logging level.
     * @param c debug class
     * @param p priority
     */
    static void set_log_level( sgDebugClass c, sgDebugPriority p );


    /**
     * @brief Set the allowed logging classes.
     * @param c All enabled logging classes anded together.
     */
    static void set_log_classes (sgDebugClass c);


    /**
     * @brief Get the logging classes currently enabled.
     * @return All enabled debug logging anded together.
     */
    static sgDebugClass get_log_classes ();


    /**
     * @brief Set the logging priority.
     * @param  p The priority cutoff for logging messages.
     */
    static void set_log_priority (sgDebugPriority p);


    /**
     * @brief Get the current logging priority.
     * @return The priority cutoff for logging messages.
     */
    static sgDebugPriority get_log_priority ();


    /**
     * @brief Set the stream buffer
     * @param sb stream buffer
     */
    void set_sb( streambuf* sb );

#ifdef _MSC_VER
    static void has_no_console() { has_console = false; }
#endif

protected:

    /** @brief sync/flush */
    inline virtual int sync();

    /** @brief  overflow */
    int_type overflow( int ch );
    // int xsputn( const char* s, istreamsize n );

private:

    /** @brief  The streambuf used for actual output. Defaults to cerr.rdbuf(). */
    static streambuf* sbuf;

    static bool logging_enabled;
#ifdef _MSC_VER
    static bool has_console;
#endif
    static sgDebugClass logClass;
    static sgDebugPriority logPriority;

private:

    // Not defined.
    logbuf( const logbuf& );
    void operator= ( const logbuf& );
};

inline int
logbuf::sync()
{
#ifdef SG_HAVE_STD_INCLUDES
  return sbuf->pubsync();
#else
  return sbuf->sync();
#endif
}

inline void
logbuf::set_log_state( sgDebugClass c, sgDebugPriority p )
{
    logging_enabled = ((c & logClass) != 0 && p >= logPriority);
}

inline logbuf::int_type
logbuf::overflow( int c )
{
#ifdef _MSC_VER
    if ( logging_enabled ) {
        if ( !has_console ) {
            AllocConsole();
            freopen("conin$", "r", stdin);
            freopen("conout$", "w", stdout);
            freopen("conout$", "w", stderr);
            has_console = true;
        }
        return sbuf->sputc(c);
    }
    else
        return EOF == 0 ? 1: 0;
#else
    return logging_enabled ? sbuf->sputc(c) : (EOF == 0 ? 1: 0);
#endif
}

/**
 * @struct loglevel
 * @brief logstream manipulator for setting the log level of a message.
 */
struct loglevel
{
    loglevel( sgDebugClass c, sgDebugPriority p )
  : logClass(c), logPriority(p) {}

    sgDebugClass logClass;
    sgDebugPriority logPriority;
};

/**
 * @struct logstream_base 
 * @brief A helper class 
 * 
 * Ensures a streambuf and ostream are constructed and
 * destroyed in the correct order.  The streambuf must be created before the
 * ostream but bases are constructed before members.  Thus, making this class
 * a private base of logstream, declared to the left of ostream, we ensure the
 * correct order of construction and destruction.
 */
struct logstream_base
{
    // logstream_base( streambuf* sb ) : lbuf(sb) {}
    logstream_base() {}

    logbuf lbuf;
};

/**
 * @class logstream
 * @brief Class to manage the debug logging stream.
 */
class logstream : private logstream_base, public ostream
{
public:
    /**
     * @brief The default is to send messages to cerr.
     * @param out output stream
     */
    logstream( ostream& out )
    // : logstream_base(out.rdbuf()),
    : logstream_base(), ostream(&lbuf)
    {
      with_date = false;
      userdatestr = 0;
      lbuf.set_sb(out.rdbuf());
    }

    /**
     * @brief Set the output stream
     * @param out output stream
     */
    void set_output( ostream& out ) { lbuf.set_sb( out.rdbuf() ); }

    /**
     * @brief Set the global log class and priority level.
     * @param c debug class
     * @param p priority
     */
    void setLogLevels( sgDebugClass c, sgDebugPriority p );

    /**
     * @brief Output operator to capture the debug level and priority of a message.
     * @param l log level
     */
    inline ostream& operator<< ( const loglevel& l );
	
    /**
     * @brief Set a user provided function, which returns a date as a string
     */
    inline void setuserdatestr ( DATEFUNC udsf ) { userdatestr=udsf; };
	
    /**
     * @brief Return a date as a string, standard format
     */
    string datestr ( void );
	
    /**
     * @brief Enable output of date
     */
    inline void enable_with_date ( bool enable ) { with_date = enable; };
    bool        with_date;
private:
    DATEFUNC    userdatestr;
};

inline ostream&
logstream::operator<< ( const loglevel& l )
{
    lbuf.set_log_state( l.logClass, l.logPriority );
    return *this;
}

extern logstream *global_logstream;

/**
 * @relates logstream
 * @brief Return the one and only logstream instance.
 * 
 * We use a function instead of a global object so we are assured that cerr
 * has been initialised.
 * @return current logstream
 */
inline logstream&
sglog()
{
  if (global_logstream == NULL) {

#ifdef __APPLE__
    /**
     * There appears to be a bug in the C++ runtime in Mac OS X that
     * will crash if certain funtions are called (in this case
     * cerr.rdbuf()) during static initialization of a class. This
     * print statement is hack to kick the library in the pants so it
     * won't crash when cerr.rdbuf() is first called -DW 
     **/
    cout << "Using Mac OS X hack for initializing C++ stdio..." << endl;
#endif    
    global_logstream = new logstream (cerr);
  }
  return *global_logstream;
}


/** \def SG_LOG(C,P,M)
 * Log a message.
 * @param C debug class
 * @param P priority
 * @param M message
 */
#ifdef FG_NDEBUG
# define SG_LOG(C,P,M)
# define SG_ALERT(C,P,M)
#elif defined( __MWERKS__ )
# define SG_LOG(C,P,M) ::sglog() << ::loglevel(C,P) << M << std::endl
# define SG_ALERT(C,P,M)::sglog() << ::loglevel(C,P) << M << std::endl;  std::cerr << M << std::endl
#else
# define SG_LOG(C,P,M) sglog()   << loglevel(C,P) << sglog().datestr() << M << std::endl
# define SG_ALERT(C,P,M) sglog() << loglevel(C,P) << sglog().datestr() << M << endl; std::cerr << M << std::endl
#endif

#endif // _LOGSTREAM_H

