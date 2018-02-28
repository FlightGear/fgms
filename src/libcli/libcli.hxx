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

#ifndef _libcli_header
#define _libcli_header

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <stdarg.h>
#ifndef _MSC_VER
        #include <termios.h>
#endif
#include "common.hxx"
#include "command.hxx"
#include "cli_client.hxx"
#include "cli_line.hxx"

namespace libcli
{

/**
 * The main cli class, which draws together all parts.
 */
class cli
{
public:
        using userlist = std::map<std::string,std::string>;
        using auth_func = std::function
          <int ( const std::string&, const std::string& )>;
        using enable_func = std::function<int ( const std::string& )>;
        using filterlist  = std::vector<filter>;

        cli ( int fd );
        ~cli ();
        void    register_command ( command* cmd, command* parent = 0 );
        void    unregister_command ( const std::string & name );
        RESULT  run_command ( const std::string& line );
        void    loop ();
        RESULT  file ( const std::string& filename, int privilege, int mode );
        void    allow_user ( const std::string& username,
                             const std::string& password );
        void    deny_user ( const std::string& username );
        void    regular ( int ( *callback ) () );
        void    show_help ( const std::string& arg, const std::string& desc );
        int     set_privilege ( int privilege );
        int     set_configmode ( int mode, const std::string& config_desc );
        inline void set_prompt   ( const std::string& prompt );
        inline void set_hostname ( const std::string& hostname );
        inline void set_modestr  ( const std::string& modestring );
        inline void set_auth_callback ( auth_func callback );
        inline void set_enable_callback ( enable_func callback );
        inline void set_enable_password ( const std::string& password );
        inline void set_banner ( const std::string& banner );
        inline void set_compare_case ( bool cmpcase );

protected:
        /// The different states of the cli
        enum class CLI_STATE
        {
                LOGIN,                  ///< ask for login name
                PASSWORD,               ///< ask for password
                NORMAL,                 ///< in normal operation mode
                QUIT                    ///< user requested to quit the cli
        };
        cli_client m_client;
        void    build_prompt ();
        bool    compare ( const std::string& s1, const std::string& s2,
                          const size_t len = 0 );
        int     compare_len ( const std::string& s1, const std::string& s2 );
        void    build_shortest ( command::cmdlist& commands );
        RESULT  parse_line   ( const std::string& line, strvec & commands,
                               strvec & filters );
        bool    command_available ( const command::command_p cmd ) const;
        RESULT  install_filter ( const strvec & filter_cmds );
        RESULT  exec_command ( const command::cmdlist & cmds,
                               const strvec & words, size_t & start_word );
        void    get_completions ( const command::cmdlist & cmds,
                                  const strvec & words, const size_t start_word,
                                  char lastchar );
        void    get_filter_completions ( const strvec & words,
                                         size_t start_word, char lastchar );
        bool    pass_matches ( const std::string& pass,
                               const std::string& tried_pass );
        void    show_prompt ();
        RESULT no_more_args     ( const strvec& args, size_t first_arg );
        RESULT internal_enable
          ( const std::string& command, const strvec& args, size_t first_arg );
        RESULT internal_disable
          ( const std::string& command, const strvec& args, size_t first_arg );
        RESULT internal_help
          ( const std::string& command, const strvec& args, size_t first_arg );
        RESULT internal_whoami
          ( const std::string& command, const strvec& args, size_t first_arg );
        RESULT internal_history
          ( const std::string& command, const strvec& args, size_t first_arg );
        RESULT internal_quit
          ( const std::string& command, const strvec& args, size_t first_arg );
        RESULT internal_exit
          ( const std::string& command, const strvec& args, size_t first_arg );
        RESULT internal_dump
          ( const std::string& command, const strvec& args, size_t first_arg );
        RESULT internal_pager
          ( const std::string& command, const strvec& args, size_t first_arg );
        RESULT internal_configure
          ( const std::string& command, const strvec& args, size_t first_arg );

        int    check_enable ( const std::string& pass );
        int    check_user_auth ( const std::string& username,
                                 const std::string& password );
        void    leave_config_mode ();
        void    list_completions ();
        bool    wants_help ( const std::string& arg );
        bool    authenticate_user ();
        line_editor m_editor;   ///< internal line editor
        std::string m_username; ///< login name of user
        bool    m_compare_case; ///< compare commands case sensitive?
        std::string m_prompt;   ///< current prompt
        std::string m_hostname; ///< part of the prompt
        std::string m_modestr;  ///< part of the prompt
        std::string m_banner;   ///< presented when connecting to the cli
        std::string m_enable_password;
        int             m_privilege;    ///< current privilege level of the user
        int             m_mode;         ///< current 'mode' of the cli
        CLI_STATE       m_state;        ///< current state of the cli
        userlist        m_users;        ///< internally known users
        auth_func       m_auth_callback;
        enable_func     m_enable_callback;
        command::cmdlist m_commands;    ///< list of known commands
};

//////////////////////////////////////////////////////////////////////

/**
 * Set the base prompt.
 * @see build_prompt
 */
void
cli::set_prompt
(
        const std::string& prompt
)
{
        m_prompt = prompt;
        build_prompt ();
}

//////////////////////////////////////////////////////////////////////

/**
 * Set the hostname.
 * @see build_prompt
 */
void
cli::set_hostname
(
        const std::string& hostname
)
{
        m_hostname = hostname;
        build_prompt ();
}

//////////////////////////////////////////////////////////////////////

/**
 * Set the modestring (eg. '-config'.
 * @see build_prompt
 */
void
cli::set_modestr
(
        const std::string& modestr
)
{
        m_modestr = modestr;
        build_prompt ();
}

//////////////////////////////////////////////////////////////////////

/**
 * Set an authentication callback. Can be used to authenticate via
 * RADIUS or TACACS.
 */
void
cli::set_auth_callback
(
        auth_func callback
)
{
        m_auth_callback = callback;
} // cli::set_auth_callback ()

//////////////////////////////////////////////////////////////////////

/**
 * Set an authentication callback for the enable password.
 * Can be used to authenticate via RADIUS or TACACS.
 */
void
cli::set_enable_callback
(
        enable_func callback
)
{
        m_enable_callback = callback;
} // cli::set_enable_callback ()


//////////////////////////////////////////////////////////////////////

void
cli::set_enable_password
(
        const std::string& password
)
{
        m_enable_password = password;
} // cli::set_enable_password ()

//////////////////////////////////////////////////////////////////////

/**
 * Set a banner. The banner is presented to the user when connecting
 * to the cli.
 */
void
cli::set_banner
(
        const std::string& banner
)
{
        m_banner = banner;
} // cli::set_banner ()

//////////////////////////////////////////////////////////////////////

/**
 * When set to 'false' commands are case insensitive.<br>
 * When set to 'true' commands are case sensitive.
 */
void
cli::set_compare_case
(
        bool cmpcase
)
{
        m_compare_case = cmpcase;
} // cli::set_compare_case ()

//////////////////////////////////////////////////////////////////////

} // namespace libcli

#endif
