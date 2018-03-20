//
// This file is part of fgms, the flightgear multiplayer server
// https://sourceforge.net/projects/fgms/
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
// along with this program; if not see <http://www.gnu.org/licenses/>
//

/**
 * @file fg_tracker.hxx
 *
 * @author (c) 2006 Julien Pierru
 * @author (c) 2015 Hazuki Amamiya
 * @author 2006-2018 Oliver Schroeder <fgms@o-schroeder.de>
 */

#ifndef _fg_tracker_hxx
#define _fg_tracker_hxx

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <fglib/netsocket.hxx>
#include <fglib/fg_geometry.hxx>

namespace fgmp
{

//////////////////////////////////////////////////////////////////////
/**
 * @class tracker
 * @brief The tracker class
 */
class tracker
{

public:
	// FIXME: only used in fgms.cxx
	enum
	{
		CONNECT,
		DISCONNECT,
		UPDATE
	};

	//////////////////////////////////////////////////
	//
	//  constructors
	//
	//////////////////////////////////////////////////
	tracker (
		int port,
		std::string server,
		std::string m_server_name,
		std::string m_domain,
		std::string m_version
	);
	~tracker ();

	//////////////////////////////////////////////////
	//
	//  public methods
	//
	//////////////////////////////////////////////////
	int	loop ();
	void	add_message ( const std::string& message );

	inline void	set_want_exit ();
	inline bool	is_connected ();
	inline int	get_port ();
	inline size_t	queue_size ();
	inline std::condition_variable& get_cond_var ();
	inline std::string get_server ();

	//////////////////////////////////////////////////
	//	stats
	//////////////////////////////////////////////////
	time_t		last_connected	= 0;
	time_t		last_seen	= 0;
	time_t		last_sent	= 0;
	uint64_t	bytes_sent	= 0;
	uint64_t	bytes_rcvd	= 0;
	uint64_t	pkts_sent	= 0;
	uint64_t	pkts_rcvd	= 0;
	size_t		lost_connections	= 0;

private:
	//////////////////////////////////////////////////
	//
	//  private variables
	//
	//////////////////////////////////////////////////
	int	m_tracker_port;
	int	m_ping_interval		= 55;
	int	m_timeout_stage		= 0;
	std::string	m_tracker_server;
	std::string	m_fgms_name;
	std::string	m_domain;
	std::string	m_proto_version;
	bool		m_identified;	// If fgtracker identified this fgms
	std::string	m_version;
	fgmp::netsocket* m_tracker_socket	= 0;
	using  vMSG = std::vector<std::string>;	// string vector
	using  VI   = vMSG::iterator;		// string vector iterator
	struct buffsock_t  	 		// socket buffer
	{
		char* buf;
		size_t maxlen;
		size_t curlen;
	};
	std::mutex msg_mutex;		// message queue mutext
	std::mutex msg_sent_mutex;		// message queue mutext
	std::mutex msg_recv_mutex;		// message queue mutext
	vMSG    msg_queue;			// the single message queue
	vMSG    msg_sent_queue;			// the single message queue
	vMSG    msg_recv_queue;			// the single message queue
	bool	want_exit = false;		// signal to exit
	bool	m_connected = false;		// If connected to fgtracker
	std::condition_variable  condition_var;		// message queue condition
	//////////////////////////////////////////////////
	//
	//  private methods
	//
	//////////////////////////////////////////////////
	bool 	connect ();
	void	write_queue ();
	void	read_queue ();
	void 	requeue_msg ();
	void 	buffsock_free ( buffsock_t* bs );
	void	check_timeout ();
	int	tracker_write ( const std::string& str );
	void 	tracker_read ( buffsock_t* bs );
	void 	reply_from_server ();
}; // class tracker

//////////////////////////////////////////////////////////////////////
//
//	inline methods
//
//////////////////////////////////////////////////////////////////////

void
tracker::set_want_exit
()
{
	want_exit = true;
}

//////////////////////////////////////////////////////////////////////

bool
tracker::is_connected
()
{
	return m_connected;
}

//////////////////////////////////////////////////////////////////////

std::condition_variable&
tracker::get_cond_var
()
{
	return condition_var;
}

//////////////////////////////////////////////////////////////////////

/**
 * @brief Return the server of the tracker
 * @retval string Return tracker server as string
 */
std::string
tracker::get_server
()
{
	return m_tracker_server;
}

//////////////////////////////////////////////////////////////////////

/**
 * @brief Return the port no of the tracker
 * @retval int Port Number
 */
int
tracker::get_port
()
{
	return m_tracker_port;
}

//////////////////////////////////////////////////////////////////////

size_t
tracker::queue_size
()
{
	return msg_queue.size ();
}

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

#endif

// eof -fg_tracker.hxx
