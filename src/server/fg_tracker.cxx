/**
 * @file fg_tracker.cxx
 * @author (c) 2006 Julien Pierru
 * @author (c) 2012 Rob Dosogne ( FreeBSD friendly )
 *
 * @todo Pete To make a links here to the config and explain a bit
 *
 */

//////////////////////////////////////////////////////////////////////
//
//  server tracker for FlightGear
//  (c) 2006 Julien Pierru
//  (c) 2012 Rob Dosogne ( FreeBSD friendly )
//
//  Licenced under GPL
//
//////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <string.h>
#include <sstream>
#ifndef _MSC_VER
	#include <errno.h>
	#include <time.h>
	#include <stdint.h>
	#include <unistd.h>
	#include <sys/ipc.h>
	#include <sys/msg.h>
	#include <sys/types.h>
	#include <signal.h>
	#ifndef __FreeBSD__
		#include <endian.h>
	#endif
#endif
#include <unistd.h>
#include "fg_common.hxx"
#include "fg_tracker.hxx"
#include "fg_util.hxx"
#include <simgear/debug/logstream.hxx>
#include "daemon.hxx"
#include <libcli/debug.hxx>

#ifndef DEF_TRACKER_SLEEP
	// #define DEF_TRACKER_SLEEP 300   // try to connect each Five minutes
	#define DEF_TRACKER_SLEEP 30   // try to connect each Five minutes
#endif // DEF_TRACKER_SLEEP

#ifdef _MSC_VER
	typedef int pid_t;
	int getpid ( void )
	{
		return ( int ) GetCurrentThreadId();
	}
	#define usleep(a) uSleep(a)
	void uSleep ( int waitTime )
	{
		__int64 time1 = 0, time2 = 0, freq = 0;
		QueryPerformanceCounter ( ( LARGE_INTEGER* ) &time1 );
		QueryPerformanceFrequency ( ( LARGE_INTEGER* ) &freq );
		do
		{
			QueryPerformanceCounter ( ( LARGE_INTEGER* ) &time2 );
		}
		while ( ( time2-time1 ) < waitTime );
	}
#endif // !_MSC_VER

logstream* fgms_logstream = NULL;
inline logstream&
tracklog()
{
        if ( fgms_logstream == NULL )
        {
                fgms_logstream = new logstream ( cerr );
        }
        return *fgms_logstream;
}

// #define TRACK_LOG(C,P,M) tracklog()   << loglevel(C,P) << tracklog().datestr() << M << std::endl
#define TRACK_LOG(C,P,M) sglog()   << loglevel(C,P) << sglog().datestr() << M << std::endl

//////////////////////////////////////////////////////////////////////
/**
 * @brief Initialize to standard values
 * @param port
 * @param server ip or domain
 * @param id  what is id? -- todo --
 */
FG_TRACKER::FG_TRACKER ( int port, string server, int id )
{
	ipcid         = id;
	m_TrackerPort = port;
	m_TrackerServer = server;
	m_TrackerSocket = 0;
	TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::FG_TRACKER:"
		<< m_TrackerServer << ", Port: " << m_TrackerPort
	);
} // FG_TRACKER()

//////////////////////////////////////////////////////////////////////
/**
 * @brief xTerminate the tracker
 */
FG_TRACKER::~FG_TRACKER ()
{
} // ~FG_TRACKER()

void* func_Tracker ( void* vp )
{
	FG_TRACKER* pt = ( FG_TRACKER* ) vp;
	pt->TrackerLoop();
	return ( ( void* ) 0xdead );
}

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Initialize the tracker as a new process
 * @param pPIDS -- to do --
 */
int
FG_TRACKER::InitTracker ()
{
	pthread_t thread;
	if ( pthread_create ( &thread, NULL, func_Tracker, ( void* ) this ) )
	{
		TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::InitTracker: can't create thread..." );
		return 1;
	}
	return ( 0 );
} // InitTracker (int port, string server, int id, int pid)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::AddMessage
(
	const string & message
)
{
	pthread_mutex_lock ( &msg_mutex ); // acquire the lock 
	msg_queue.push_back ( message ); // queue the message
	pthread_cond_signal ( &condition_var ); // wake up the worker
	pthread_mutex_unlock ( &msg_mutex ); // give up the lock
} // FG_TRACKER::AddMessage()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Send the messages to the tracker server
 */
int
FG_TRACKER::TrackerLoop ()
{
	/*Msg structure*/
	class MSG
	{
	public:
		string msg;
		MSG* next;
	};
	m_MsgBuffer	buf;
	int 		i=0;
	size_t		length;
	int		pkt_sent = 0;
	int		max_msg_sent = 25; /*Maximun message sent before receiving reply.*/
	char		res[MSGMAXLINE];	/*Msg from/to server*/
	string 		PINGRPY;
	stringstream	out;
	pid_t		pid = getpid();
	short int	time_out_counter_l=0;
	unsigned int	time_out_counter_u=0;
	short int	time_out_fraction=25; /* 1000000/time_out_fraction must be integer*/
	bool		resentflg = false; /*If ture, resend all message in the msgbuf first*/
	bool		sockect_read_completed = false;
	MSG*		msgque_head;
	MSG*		msgque_tail;
	MSG*		msgque_new;
	MSG*		msgbuf_head;
	MSG*		msgbuf_tail;
	MSG*		msgbuf_new;
	MSG*		msgbuf_resend;
	string		CurrentMessage; // received from FG_SERVER
	/*Initalize value*/
	strcpy ( res, "" );
	msgque_head = NULL;
	msgque_tail = NULL;
	msgque_new = NULL;
	msgbuf_head = NULL;
	msgbuf_tail = NULL;
	msgbuf_resend = NULL;
	pthread_mutex_init ( &msg_mutex, 0 ); 
	pthread_cond_init  ( &condition_var, 0 ); 
	sleep (2);
	// tracklog().setLogLevels ( SG_FGTRACKER, SG_INFO );
	TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
		<< "trying to connect"
	);
	m_connected = Connect();
	TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
		<< "Msg structure size: " << sizeof ( class MSG )
	);
	/*Infinite loop*/
	for ( ; ; )
	{
		while (! m_connected)
		{
			TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
				<< "not connected, will slepp for " << DEF_TRACKER_SLEEP << " seconds"
			);
			sleep (DEF_TRACKER_SLEEP);
			m_connected = Connect();
		}
		length = 0;
		/*time-out issue*/
		usleep ( 1000000/time_out_fraction );
		time_out_counter_l++;
		if ( time_out_counter_l==time_out_fraction )
		{
			time_out_counter_u++;
			time_out_counter_l=0;
		}
		if ( time_out_counter_u%60==0 && time_out_counter_u >=180 && time_out_counter_l==0 )
		{
			/*Print warning*/
			if ( m_connected==true )
			{
				TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
					<< "Warning: FG_TRACKER::TrackerLoop No data receive from server for "
					<< time_out_counter_u << "seconds"
				);
			}
		}
		if ( time_out_counter_u%DEF_TRACKER_SLEEP==0 && time_out_counter_l==0 )
		{
			/*Timed out/Need retry - reconnect*/
			if ( m_connected==true )
			{
				TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
					<< "Connection timed out..."
				);
			}
			m_connected = false;
			pkt_sent  = 0;
			resentflg = true;
			msgbuf_resend=NULL;
			strcpy ( res, "" );
			time_out_counter_l = 1;
			time_out_counter_u = 0;
			continue;
		}
		/*time-out issue End*/
		/*Read msg from IPC*/
		pthread_mutex_lock ( &msg_mutex );
		pthread_cond_wait ( &condition_var, &msg_mutex );   // go wait for the condition
		VI vi = msg_queue.begin(); // get first message
		if ( vi != msg_queue.end() )
		{
			CurrentMessage = (*vi).c_str();
			msg_queue.erase ( vi ); // remove from queue
			length = CurrentMessage.size(); // should I worry about LENGTH???
		}
		pthread_mutex_unlock ( &msg_mutex ); // unlock the mutex
		if ( length )
		{
#ifdef ADD_TRACKER_LOG
			write_msg_log ( CurrentMessage.c_str(), length, ( char* ) "OUT: " );
#endif // #ifdef ADD_TRACKER_LOG
			msgque_new = new MSG;
			if ( msgque_new == NULL )
			{
				TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
					"Cannot allocate memory. Force exit..."
				);
				exit ( 0 );
			}
			msgque_new->msg = CurrentMessage;
			msgque_new->next = NULL;
			if ( msgque_head == NULL ) /*No message in queue at all*/
			{
				msgque_head = msgque_new;
			}
			else
			{
				msgque_tail->next=msgque_new;
			}
			msgque_tail = msgque_new;
		}
		else
		{
			CurrentMessage = "length=0!";
		}
		length = 0;
		/*DO NOT place any stream read/write above this line!*/
		if ( m_connected == false )
		{
			cout << "TrackerLoop::m_connected == false!" << endl;
			continue;
		}
		/*Place stream read/write below this line.*/
		/*Read socket and see if anything arrived*/
		/*Maximun character in a line : MSGMAXLINE-1. res[MSGMAXLINE] = '\0'*/
		length = 0;
		i = m_TrackerSocket->recv (res, MSGMAXLINE, 0);
		if (i >= 0)
		{
			sockect_read_completed = true;
			length = i;
			res[length]='\0';
		}
		i=0;
		//////////////////////////////////////////////////
		//	received data from tracking server
		//////////////////////////////////////////////////
		if ( (length > 0) && (sockect_read_completed == true) )
		{
			/*ACK from server*/
			sockect_read_completed=false;
			if ( strncmp ( res, "OK", 2 ) == 0 )
			{
				time_out_counter_l=1;
				time_out_counter_u=0;
				if ( msgbuf_head==NULL )
				{
					TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
						<< "Warning! Message count mismatch between fgms "
						<< "and fgtracker. Potential data loss!"
					);
				}
				else
				{
					msgbuf_new=msgbuf_head;
					msgbuf_head=msgbuf_head->next;
					free ( msgbuf_new );
					msgbuf_new=NULL;
					pkt_sent--;
				}
			}
			else if ( strncmp ( res, "PING", 4 ) == 0 )
			{
				/*reply PONG*/
				time_out_counter_l=1;
				time_out_counter_u=0;
				/*create status report for tracker server*/
				PINGRPY.append ( "PONG STATUS: pkt_sent=" );
				out << pkt_sent;
				PINGRPY.append ( out.str() );
				if ( resentflg==true )
				{
					PINGRPY.append ( ", resentflg=true, " );
				}
				else
				{
					PINGRPY.append ( ", resentflg=false, " );
				}
				if ( msgque_head==NULL )
				{
					PINGRPY.append ( "msgque_head is null, " );
				}
				else
				{
					PINGRPY.append ( "msgque_head is NOT null, " );
				}
				if ( msgbuf_resend ==NULL )
				{
					PINGRPY.append ( "msgbuf_resend is null, " );
				}
				else
				{
					PINGRPY.append ( "msgbuf_resend is NOT null, " );
				}
				if ( msgbuf_tail ==NULL )
				{
					PINGRPY.append ( "msgbuf_tail is null, " );
				}
				else
				{
					PINGRPY.append ( "msgbuf_tail is NOT null, " );
				}
				if ( msgbuf_tail == msgbuf_resend )
				{
					PINGRPY.append ( "msgbuf_tail == msgbuf_resend, " );
				}
				else
				{
					PINGRPY.append ( "msgbuf_tail != msgbuf_resend, " );
				}
				if ( msgbuf_head == NULL )
				{
					PINGRPY.append ( "msgbuf_head is null. " );
				}
				else
				{
					PINGRPY.append ( "msgbuf_head is NOT null. " );
				}
				/*output status to tracker server*/
				TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
					<< "PING from server received"
				);
				if (m_TrackerSocket->write_str (PINGRPY) < 0)
				{
					m_connected = false;
					TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
						<< "lost connection to server"
					);
					continue;

				}
				PINGRPY.erase();
				out.str ( "" );
			}
			else
			{
				TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
					<< "Responce not recognized. Msg: '" << res << "' length: " << length
				);
			}
			strcpy ( res, "" );
		}
		length = 0;
		/*Send message if necessary*/
		if ( pkt_sent<max_msg_sent )
		{
			// get message from queue
			if ( resentflg==true )
			{
				/*msg from buffer*/
				if ( msgbuf_head==NULL )
				{
					/*All msg sent and well received*/
					TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
						<< "Resend data completed."
					);
					resentflg     = false;
					msgbuf_resend = NULL;
					msgbuf_tail   = NULL;
					continue;
				}
				else if ( msgbuf_tail == msgbuf_resend ) /*All msg sent, waiting ACK*/
				{
					continue;
				}
				else if ( msgbuf_resend == NULL )
				{
					/*Need to send msg after the connect.*/
					msgbuf_resend = msgbuf_head;
					msgbuf_new    = msgbuf_resend;
					length = CurrentMessage.size();
					// strlen ( msgbuf_new->msg );
				}
				else
				{
					/*Need to send further message*/
					msgbuf_resend = msgbuf_resend->next;
					msgbuf_new    = msgbuf_resend;
					length        = CurrentMessage.size();
					// length=strlen ( msgbuf_new->msg );
				}
			}
			else
			{
				/*msg from queue*/
				if ( msgque_head == NULL ) /*No message pending to send at all*/
				{
					continue;
				}
				msgbuf_new  = msgque_head;
				msgque_head = msgque_head->next;
				if ( msgbuf_head == NULL ) /*No message in buffer at all*/
				{
					msgbuf_head = msgbuf_new;
				}
				else
				{
					msgbuf_tail->next = msgbuf_new;
				}
				msgbuf_tail = msgbuf_new;
				msgbuf_tail->next = NULL;
				length = CurrentMessage.size();
				// length=strlen ( msgbuf_new->msg );
			}
		}
		else
		{
			continue;        /*buffer full. Don't send msg*/
		}
		if ( length > 0 ) /*confirm if the message length is > 0*/
		{
			length++; /*including the '\0'*/
			/*Send message at msgbuf_new via tcp*/
			TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
				<< "sending msg " << length << "  bytes, addr " << msgbuf_new
				<< "next addr " << msgbuf_new->next
			);
			TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
				<< "Msg: " << msgbuf_new->msg
			);
			if ( m_TrackerSocket->write_str (msgbuf_new->msg) < 0 )
			{
				TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
					<< "Can't write to server..."
				);
				m_connected = false;
				pkt_sent  = 0;
				resentflg = true;
				msgbuf_resend=NULL;
				strcpy ( res, "" );
				continue;
			}
			else
			{
				pkt_sent++;
			}
		}
		else
		{
			/*should not happen at all!*/
			TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::TrackerLoop [" << pid << "]: "
				<< "Internal Error (Msg size <= 0). Contact programmer to fix this issue."
			);
		}
	}
	return ( 0 );
} // TrackerLoop ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  (Re)connect the tracker to its server
//	RETURN: true = success, false = failed
//
//////////////////////////////////////////////////////////////////////
bool
FG_TRACKER::Connect()
{
	pid_t pid = getpid();

	if ( m_TrackerSocket )
	{
		m_TrackerSocket->close ();
		delete m_TrackerSocket;
	}
	m_TrackerSocket = new netSocket();
	TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::Connect [" << pid << "]: "
		<< "Server: " << m_TrackerServer << ", Port: " << m_TrackerPort);
	if ( m_TrackerSocket->open (true) == false)
	{
		TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Connect [" << pid << "]: "
			<< "Can't get socket..."
		);
		return false;
	}
	if ( m_TrackerSocket->connect ( m_TrackerServer.c_str(), m_TrackerPort ) < 0 )
	{
		TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Connect [" << pid << "]: "
			<< "Connect failed!"
		);
		return false;
	}
	m_TrackerSocket->setBlocking ( false );
	TRACK_LOG ( SG_FGTRACKER, SG_ALERT, "# FG_TRACKER::Connect [" << pid << "]: "
		<< "success"
	);
	m_TrackerSocket->write_char ('\0');
	sleep ( 2 );
	m_TrackerSocket->write_str ("NOWAIT");
	TRACK_LOG ( SG_FGTRACKER, SG_DEBUG, "# FG_TRACKER::Connect [" << pid << "]: "
		<< "Written 'NOWAIT'"
		);
	sleep ( 1 );
	return true;
} // Connect ()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Disconnect the tracker from its server
 */
void
FG_TRACKER::Disconnect ()
{
	if ( m_TrackerSocket )
	{
		m_TrackerSocket->close ();
		delete m_TrackerSocket;
	}
} // Disconnect ()
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#if !defined(NTDDI_VERSION) || !defined(NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA)   // if less than VISTA, provide alternative
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT    97      /* not present in errno.h provided with VC */
#endif
int inet_aton ( const char* cp, struct in_addr* addr )
{
	addr->s_addr = inet_addr ( cp );
	return ( addr->s_addr == INADDR_NONE ) ? -1 : 0;
}
int inet_pton ( int af, const char* src, void* dst )
{
	if ( af != AF_INET )
	{
		errno = EAFNOSUPPORT;
		return -1;
	}
	return inet_aton ( src, ( struct in_addr* ) dst );
}
#endif // #if (NTDDI_VERSION < NTDDI_VISTA)
#endif // _MSC_VER

//////////////////////////////////////////////////////////////////////
/**
 * @brief Signal handling
 * @param s int with the signal
 */
void
signal_handler ( int s )
{
#ifndef _MSC_VER
	pid_t mypid = getpid();
	switch ( s )
	{
	case  1:
		printf ( "[%d] SIGHUP received, exiting...\n",mypid );
		exit ( 0 );
		break;
	case  2:
		printf ( "[%d] SIGINT received, exiting...\n",mypid );
		exit ( 0 );
		break;
	case  3:
		printf ( "[%d] SIGQUIT received, exiting...\n",mypid );
		break;
	case  4:
		printf ( "[%d] SIGILL received\n",mypid );
		break;
	case  5:
		printf ( "[%d] SIGTRAP received\n",mypid );
		break;
	case  6:
		printf ( "[%d] SIGABRT received\n",mypid );
		break;
	case  7:
		printf ( "[%d] SIGBUS received\n",mypid );
		break;
	case  8:
		printf ( "[%d] SIGFPE received\n",mypid );
		break;
	case  9:
		printf ( "[%d] SIGKILL received\n",mypid );
		exit ( 0 );
		break;
	case 10:
		printf ( "[%d] SIGUSR1 received\n",mypid );
		break;
	case 11:
		printf ( "[%d] SIGSEGV received. Exiting...\n",mypid );
		exit ( 1 );
		break;
	case 12:
		printf ( "[%d] SIGUSR2 received\n",mypid );
		break;
	case 13:
		printf ( "[%d] SIGPIPE received. Connection Error.\n",mypid );
		break;
	case 14:
		printf ( "[%d] SIGALRM received\n",mypid );
		break;
	case 15:
		printf ( "[%d] SIGTERM received\n",mypid );
		exit ( 0 );
		break;
	case 16:
		printf ( "[%d] SIGSTKFLT received\n",mypid );
		break;
	case 17:
		printf ( "[%d] SIGCHLD received\n",mypid );
		break;
	case 18:
		printf ( "[%d] SIGCONT received\n",mypid );
		break;
	case 19:
		printf ( "[%d] SIGSTOP received\n",mypid );
		break;
	case 20:
		printf ( "[%d] SIGTSTP received\n",mypid );
		break;
	case 21:
		printf ( "[%d] SIGTTIN received\n",mypid );
		break;
	case 22:
		printf ( "[%d] SIGTTOU received\n",mypid );
		break;
	case 23:
		printf ( "[%d] SIGURG received\n",mypid );
		break;
	case 24:
		printf ( "[%d] SIGXCPU received\n",mypid );
		break;
	case 25:
		printf ( "[%d] SIGXFSZ received\n",mypid );
		break;
	case 26:
		printf ( "[%d] SIGVTALRM received\n",mypid );
		break;
	case 27:
		printf ( "[%d] SIGPROF received\n",mypid );
		break;
	case 28:
		printf ( "[%d] SIGWINCH received\n",mypid );
		break;
	case 29:
		printf ( "[%d] SIGIO received\n",mypid );
		break;
	case 30:
		printf ( "[%d] SIGPWR received\n",mypid );
		break;
	default:
		printf ( "[%d] signal %d received\n",mypid,s );
	}
#endif
}

// eof - fg_tracker.cxx
//////////////////////////////////////////////////////////////////////
