/**
 * @file fg_tracker.hxx
 * @author (c) 2006 Julien Pierru
 * @author (c) 2015 Hazuki Amamiya
 *
 */

//////////////////////////////////////////////////////////////////////
//
// server tracker for FlightGear
// (c) 2006 Julien Pierru
// (c) 2015 Hazuki Amamiya
// Licenced under GPL
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, US
//////////////////////////////////////////////////////////////////////

#if !defined FG_TRACKER_HPP
#define FG_TRACKER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
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
private:
	//////////////////////////////////////////////////
	//
	//  private variables
	//
	//////////////////////////////////////////////////
	int	m_tracker_port;
	int	m_ping_interval;
	int	m_timeout_stage;
	std::string	m_tracker_server;
	std::string	m_fgms_name;
	std::string	m_domain;
	std::string	m_proto_version;
	bool		m_identified;			// If fgtracker identified this fgms
	std::string	m_version;
	fgmp::netsocket* m_tracker_socket;

	using  vMSG = std::vector<std::string>;	// string vector
	using  VI   = vMSG::iterator;		// string vector iterator
	struct buffsock_t  	 		// socket buffer
	{
		char* buf;
		size_t maxlen;
		size_t curlen;
	};
	pthread_mutex_t msg_mutex;		// message queue mutext
	pthread_mutex_t msg_sent_mutex;		// message queue mutext
	pthread_mutex_t msg_recv_mutex;		// message queue mutext
	vMSG    msg_queue;			// the single message queue
	vMSG    msg_sent_queue;			// the single message queue
	vMSG    msg_recv_queue;			// the single message queue
	bool	want_exit;			// signal to exit
	bool	m_connected;			// If connected to fgtracker
	pthread_cond_t  condition_var;		// message queue condition

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
public:
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
	tracker ( int port, std::string server, std::string m_server_name, std::string m_domain, std::string m_version);
	~tracker ();

	//////////////////////////////////////////////////
	//
	//  public methods
	//
	//////////////////////////////////////////////////
	int	loop ();
	void	add_message ( const std::string& message );

	void set_want_exit ()
	{
		want_exit = true;
	};

	inline bool is_connected ()
	{
		return m_connected;
	};

	pthread_cond_t* get_cond_var ()
	{
		return &condition_var;
	};

	/**
	 * @brief Return the server of the tracker
	 * @retval string Return tracker server as string
	 */
	inline std::string get_server ()
	{
		return m_tracker_server;
	};

	/**
	 * @brief Return the port no of the tracker
	 * @retval int Port Number
	 */
	inline int get_port ()
	{
		return m_tracker_port;
	};
	pthread_t get_thread_id ();

	size_t	queue_size ()
	{
		return msg_queue.size ();
	}

	//////////////////////////////////////////////////
	//	stats
	//////////////////////////////////////////////////
	time_t		last_connected;
	time_t		last_seen;
	time_t		last_sent;
	uint64_t	bytes_sent;
	uint64_t	bytes_rcvd;
	uint64_t	pkts_sent;
	uint64_t	pkts_rcvd;
	size_t		lost_connections;
	pthread_t 	m_thread_id;
};

} // namespace fgmp

#endif

// eof -fg_tracker.hxx
