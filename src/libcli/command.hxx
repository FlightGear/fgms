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
// derived from libcli by David Parrish (david@dparrish.com)
// Copyright (C) 2011  Oliver Schroeder
//

#ifndef CLI_COMMAND_H
#define CLI_COMMAND_H

#include "common.hxx"

namespace LIBCLI
{

using namespace std;

class CLI;

class CommandBase
{
};

template <class C>
class Command : public CommandBase
{
public:
    friend class CLI;
    typedef int (*c_callback_func)
      (C & obj, char* command, char** argv, int argc);
    typedef int (C::*cpp_callback_func)
      (char* command, char** argv, int argc);
    Command ();
    Command (
        C*              obj,
        const char*     command,
        PRIVLEVEL       level,
        MODE            mode,
        const char*     help
    );
    Command (
        C*              obj,
        const char*     command,
        c_callback_func callback,
        PRIVLEVEL       level,
        MODE            mode,
        const char*     help
    );
    Command (
        C*              obj,
        const char*     command,
        cpp_callback_func callback,
        PRIVLEVEL       level,
        MODE            mode,
        const char*     help
    );
    ~Command ();
    int  exec (char* command, char ** argv, int argc);
    int  exec (C& Instance, char* command, char** argv, int argc);
private:
    char*           command;
    char*           help;
    PRIVLEVEL       privilege;
    MODE            mode;
    unsigned int    unique_len;
    Command*        next;
    Command*        children;
    Command*        parent;
    C*              _obj;
    bool            have_callback;
    c_callback_func     c_callback;
    cpp_callback_func   cpp_callback;
    Command ( const C& obj );
}; // class Command

template <class C>
Command<C>::Command
()
{
    DEBUG d (__FUNCTION__,__FILE__,__LINE__);
    this->parent    = 0;
    this->command   = 0;
    this->help      = 0;
    this->next      = 0;
    this->children  = 0;
    this->privilege = UNPRIVILEGED;
    this->mode      = MODE_ANY;
    this->_obj      = 0;
    this->c_callback    = 0;
    this->cpp_callback  = 0;
    this->have_callback = false;
}

template <class C>
Command<C>::Command
(
    C*              obj,
    const char*     command,
    PRIVLEVEL       level,
    MODE            mode,
    const char*     help
)
{
    DEBUG d (__FUNCTION__,__FILE__,__LINE__);
    if ( (command == 0) || (strlen (command) == 0) )
    {
        throw arg_error ("C1: bad argument");
    }
    if (command)
    {
        this->command   = strdup (command);
    }
    if (help)
    {
        this->help      = strdup (help);
    }
    this->parent    = 0;
    this->privilege = level;
    this->mode      = mode;
    this->next      = 0;
    this->children  = 0;
    this->_obj      = obj;
    this->c_callback    = 0;
    this->cpp_callback  = 0;
    this->have_callback = false;
}

template <class C>
Command<C>::Command
(
    C*              obj,
    const char*     command,
    c_callback_func callback,
    PRIVLEVEL       level,
    MODE            mode,
    const char*     help
)
{
    DEBUG d (__FUNCTION__,__FILE__,__LINE__);
    if ( (command == 0) || (strlen (command) == 0) )
    {
        throw arg_error ("C2: bad argument");
    }
    if (command)
    {
        this->command   = strdup (command);
    }
    if (help)
    {
        this->help      = strdup (help);
    }
    this->parent    = 0;
    this->privilege = level;
    this->mode      = mode;
    this->next      = 0;
    this->children  = 0;
    this->_obj      = obj;
    this->c_callback    = callback;
    this->cpp_callback  = 0;
    if (this->c_callback)
    {
        this->have_callback = true;
    }
    else
    {
        this->have_callback = false;
    }
}

template <class C>
Command<C>::Command
(
    C*              obj,
    const char*     command,
    cpp_callback_func callback,
    PRIVLEVEL       level,
    MODE            mode,
    const char*     help
)
{
    DEBUG d (__FUNCTION__,__FILE__,__LINE__);
    if ( (command == 0) || (strlen (command) == 0) )
    {
        throw arg_error ("C2: bad argument");
    }
    if (command)
    {
        this->command   = strdup (command);
    }
    if (help)
    {
        this->help      = strdup (help);
    }
    this->parent    = 0;
    this->privilege = level;
    this->mode      = mode;
    this->next      = 0;
    this->children  = 0;
    this->_obj      = obj;
    this->c_callback    = 0;
    this->cpp_callback= callback;
    if (this->cpp_callback)
    {
        this->have_callback = true;
    }
    else
    {
        this->have_callback = false;
    }
}

template <class C>
Command<C>::~Command
()
{
    DEBUG d (__FUNCTION__,__FILE__,__LINE__);
    parent = 0;
    if (command)
    {
        free_z (command);
    }
    if (help)
    {
        free_z (help);
    }
    c_callback =  0;
    cpp_callback =  0;
}

template <class C>
int
Command<C>::exec
(
    char*   command,
    char**  argv,
    int     argc
)
{
    DEBUG d (__FUNCTION__,__FILE__,__LINE__);
    if (! have_callback)
    {
        throw arg_error ("Command::exec: no callback!");
    }
    if (c_callback)
    {
        return (this->c_callback (*_obj, command, argv, argc));
    }
    else if (cpp_callback)
    {
        return this->exec (*_obj, command, argv, argc);
    }
    throw arg_error ("BAD: Command::exec: no callback!");
}

template <class C>
int
Command<C>::exec
(
    C& Instance,
    char*   command,
    char**  argv,
    int     argc
)
{
    DEBUG d (__FUNCTION__,__FILE__,__LINE__);
    if (! this->cpp_callback)
    {
        throw arg_error ("Command::exec: no cpp_callback");
    }
    return (CALL_MEMBER_FN (Instance, this->cpp_callback)(command, argv, argc));
}

}; // namespace LIBCLI

#endif
