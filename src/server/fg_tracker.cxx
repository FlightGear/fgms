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
#include "common.h"
#include "fg_tracker.hxx"
#include "typcnvt.hxx"
#include <simgear/debug/logstream.hxx>
#include "daemon.hxx"

#ifndef DEF_TRACKER_SLEEP
	#define DEF_TRACKER_SLEEP 300   // try to connect each Five minutes
#endif // DEF_TRACKER_SLEEP

#ifdef _MSC_VER
	typedef int pid_t;
	int getpid(void)
	{
		return (int)GetCurrentThreadId();
	}
#define usleep(a) uSleep(a)
	void uSleep(int waitTime)
	{ 
		__int64 time1 = 0;
		time2 = 0;
		freq = 0; 
		QueryPerformanceCounter((LARGE_INTEGER *) &time1); 
		QueryPerformanceFrequency((LARGE_INTEGER *)&freq); 
		do
		{ 
			QueryPerformanceCounter((LARGE_INTEGER *) &time2); 
		} while((time2-time1) < waitTime); 
	} 
#else
	extern  cDaemon Myself;
#endif // !_MSC_VER

#ifndef DEF_DEBUG_OUTPUT
	#define DEF_DEBUG_OUTPUT false
#endif

extern bool RunAsDaemon;
static bool AddDebug = DEF_DEBUG_OUTPUT;

//////////////////////////////////////////////////////////////////////
//
//      Initilize to standard values
//
//////////////////////////////////////////////////////////////////////
FG_TRACKER::FG_TRACKER (int port, string server, int id)
{
    ipcid         = id;
    m_TrackerPort = port;
    strcpy (m_TrackerServer, server.c_str());
    if (!RunAsDaemon || AddDebug) 
        printf("FG_TRACKER::FG_TRACKER: Server: %s, Port: %d\n", m_TrackerServer, m_TrackerPort);
} // FG_TRACKER()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      terminate the tracker
//
//////////////////////////////////////////////////////////////////////
FG_TRACKER::~FG_TRACKER ()
{
} // ~FG_TRACKER()
//////////////////////////////////////////////////////////////////////

#ifdef USE_TRACKER_PORT
void *func_Tracker(void *vp)
{
    FG_TRACKER *pt = (FG_TRACKER *)vp;
    pt->Connect();
    pt->TrackerLoop();
    return ((void *)0xdead);
}

#endif // #ifdef USE_TRACKER_PORT

//////////////////////////////////////////////////////////////////////
//
//      Initilize the tracker
//
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::InitTracker ( pid_t *pPIDS )
{
#ifndef NO_TRACKER_PORT
#ifdef USE_TRACKER_PORT
    if (pthread_create( &thread, NULL, func_Tracker, (void*)this ))
    {
        SG_LOG (SG_SYSTEMS, SG_ALERT, "# FG_TRACKER::InitTracker: can't create thread...");
        return 1;
    }
#else // !#ifdef USE_TRACKER_PORT
    pid_t ChildsPID;
    ChildsPID = fork();
    if (ChildsPID < 0)
    {
        SG_LOG (SG_SYSTEMS, SG_ALERT, "# FG_TRACKER::InitTracker: fork() FAILED!");
        return 1;
    }
    else if (ChildsPID == 0)
    {
        usleep(2500);
		#ifndef _MSC_VER
		/*Install signal handler*/
		signal(SIGCHLD, signal_handler);
		signal(SIGHUP, signal_handler);
		signal(SIGINT, signal_handler);
		signal(SIGQUIT, signal_handler);
		signal(SIGILL, signal_handler);
		signal(SIGTRAP, signal_handler);
		signal(SIGABRT, signal_handler);
		signal(SIGBUS, signal_handler);
		signal(SIGFPE, signal_handler);
		signal(SIGKILL, signal_handler);
		signal(SIGUSR1, signal_handler);
		signal(SIGSEGV, signal_handler);
		signal(SIGUSR2, signal_handler);
		signal(SIGPIPE, signal_handler);
		signal(SIGALRM, signal_handler);
		signal(SIGTERM, signal_handler);
		signal(SIGCONT, signal_handler);
		signal(SIGSTOP, signal_handler);
		signal(SIGTSTP, signal_handler);
		#endif // !_MSC_VER
        
        TrackerLoop ();
        exit (0);
    }
    else
    {
        pPIDS[0] = ChildsPID; // parent - store child PID
    }
#endif // #ifdef USE_TRACKER_PORT y/n
#endif // NO_TRACKER_PORT
    return (0);
} // InitTracker (int port, string server, int id, int pid)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  send the messages to the tracker server
//
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::TrackerLoop ()
{
    m_MsgBuffer		buf;
	int 			i=0;
    int				length;
	int				pkt_sent = 0;
	int				max_msg_sent = 25; /*Maximun message sent before receiving reply.*/
    char			res[MSGMAXLINE];	/*Msg from/to server*/
	string 			PINGRPY;
	stringstream out;
	char 			b;
    pid_t			pid = getpid();
    short int		time_out_counter_l=0;
    unsigned int	time_out_counter_u=0;
    short int		time_out_fraction=25; /* 1000000/time_out_fraction must be integer*/
	bool			resentflg = false; /*If ture, resend all message in the msgbuf first*/
	bool			connected = false; /*If connected to fgtracker*/
	bool			sockect_read_completed = false;
	/*Msg structure*/
	struct MSG 
	{
		char msg[MSGMAXLINE];
		struct MSG *next;
	};
	struct MSG		*msgque_head,*msgque_tail,*msgque_new,*msgbuf_head,*msgbuf_tail,*msgbuf_new,*msgbuf_resend;

	
	/*Initalize value*/
    strcpy ( res, "" );
	msgque_head = NULL;
	msgque_tail = NULL;
	msgque_new = NULL;
	msgbuf_head = NULL;
	msgbuf_tail = NULL;
	msgbuf_resend = NULL;

    if (!RunAsDaemon || AddDebug)
        printf("[%d] FG_TRACKER::TrackerLoop entered. Msg structure size: %d\n",pid, (int)sizeof(struct MSG));
    
	connected=Connect();
	
	/*Infinite loop*/
	for ( ; ; )
    {
        length = 0;
			
		/*time-out issue*/
        usleep(1000000/time_out_fraction);
        time_out_counter_l++;
        if (time_out_counter_l==time_out_fraction)
        {
            time_out_counter_u++;
            time_out_counter_l=0;
        }
        if (time_out_counter_u%60==0 && time_out_counter_u >=180 && time_out_counter_l==0)
        {   /*Print warning*/
            if ((!RunAsDaemon || AddDebug) && connected==true)
                printf("[%d] Warning: FG_TRACKER::TrackerLoop No data receive from server for %d seconds\n",pid,time_out_counter_u);
        }
        if (time_out_counter_u%DEF_TRACKER_SLEEP==0 && time_out_counter_l==0)
        {   /*Timed out/Need retry - reconnect*/
			if (connected==true)
			{
				printf("[%d] FG_TRACKER::TrackerLoop Connection timed out...\n",pid);
				SG_LOG (SG_SYSTEMS, SG_ALERT, "["<< pid <<"] FG_TRACKER::TrackerLoop: Connection timed out...");
			}
            connected=Connect();
			msgbuf_resend=NULL;
			pkt_sent=0;
			resentflg = true;
			strcpy ( res, "" );
            time_out_counter_l=1;
            time_out_counter_u=0;
        }
		/*time-out issue End*/
		
		/*Read msg from IPC*/
		#ifndef NO_TRACKER_PORT
		#ifdef USE_TRACKER_PORT
		pthread_mutex_lock( &msg_mutex );   // acquire the lock
		pthread_cond_wait( &condition_var, &msg_mutex );    // go wait for the condition
		VI vi = msg_queue.begin(); // get first message
		if (vi != msg_queue.end())
		{
			std::string s = *vi;
			msg_queue.erase(vi);    // remove from queue
			length = (int)s.size(); // should I worry about LENGTH???
			strcpy( buf.mtext, s.c_str() ); // mtext is 1024 bytes!!!
		}
		pthread_mutex_unlock( &msg_mutex ); // unlock the mutex
		#else // !#ifdef USE_TRACKER_PORT
		length = msgrcv (ipcid, &buf, MAXLINE, 0, MSG_NOERROR | IPC_NOWAIT);
		#endif // #ifdef USE_TRACKER_PORT y/n
		#endif // NO_TRACKER_PORT
		buf.mtext[length] = '\0';
		#ifdef ADD_TRACKER_LOG
		if (length)
			write_msg_log(&buf.mtext[0], length, (char *)"OUT: ");
		#endif // #ifdef ADD_TRACKER_LOG
        if (length>0)
		{
			msgque_new = (struct MSG *) malloc(sizeof(struct MSG));
			if (msgque_new==NULL)
			{
				printf("[%d] FG_TRACKER::TrackerLoop Cannot allocate memory. Force exit...\n",pid);
				exit(0);
			}
			strcpy ( msgque_new->msg, buf.mtext);
			msgque_new->next = NULL;
			
			if (msgque_head == NULL) /*No message in queue at all*/
				msgque_head = msgque_new;
			else
				msgque_tail->next=msgque_new;
			msgque_tail = msgque_new;
		}
		
		length = 0;
		
		/*DO NOT place any stream read/write above this line!*/
		if (connected==false)
			continue;
		/*Place stream read/write below this line.*/
		
		/*Read socket and see if anything arrived*/
		length = strlen(res);
		do
		{	/*Maximun character in a line : MSGMAXLINE-1. res[MSGMAXLINE] = '\0'*/
			if (length==MSGMAXLINE-1)
			{
				sockect_read_completed=true;
				break;
			}
			i = recv( m_TrackerSocket, &b, sizeof( b ), 0 );
			res[length]=b;
			if (b=='\0' && i>=1)
			{
				sockect_read_completed=true;
				break;
			}
			if (i<1)
				break;
		}while(length++);i=0;
		res[length]='\0';
		
        if (length > 0 && sockect_read_completed==true)
        {   /*ACK from server*/
			sockect_read_completed=false;
			//printf("[%d] FG_TRACKER::TrackerLoop pktsent=%d, msgbuf_head to kill %p MsgR: %s.\n",pid,pkt_sent,msgbuf_head,res);
            if ( strncmp( res, "OK", 2 ) == 0 )
            {
                time_out_counter_l=1;
                time_out_counter_u=0;
				if (msgbuf_head==NULL)
					printf("[%d] FG_TRACKER::TrackerLoop Warning! Message count mismatch between fgms and fgtracker. Potential data loss!.\n",pid);
				else
				{
					msgbuf_new=msgbuf_head;
					msgbuf_head=msgbuf_head->next;
					free(msgbuf_new);
					msgbuf_new=NULL;
					pkt_sent--;
				}
            }
            else if ( strncmp( res, "PING", 4 ) == 0 )
            {
                /*reply PONG*/
                time_out_counter_l=1;
                time_out_counter_u=0;
				/*create status report for tracker server*/
				PINGRPY.append("PONG STATUS: pkt_sent=");
				out << pkt_sent;
				PINGRPY.append(out.str());

				if (resentflg==true)
					PINGRPY.append(", resentflg=true, ");
				else
					PINGRPY.append(", resentflg=false, ");

				if (msgque_head==NULL)
					PINGRPY.append( "msgque_head is null, " );
				else
					PINGRPY.append( "msgque_head is NOT null, " );
				
				if (msgbuf_resend ==NULL)
					PINGRPY.append( "msgbuf_resend is null, " );
				else
					PINGRPY.append( "msgbuf_resend is NOT null, " );
				
				if (msgbuf_tail ==NULL)
					PINGRPY.append( "msgbuf_tail is null, " );
				else
					PINGRPY.append( "msgbuf_tail is NOT null, " );
					
				if (msgbuf_tail == msgbuf_resend)
					PINGRPY.append( "msgbuf_tail == msgbuf_resend, " );
				else
					PINGRPY.append( "msgbuf_tail != msgbuf_resend, " );
					
				if (msgbuf_head == NULL)
					PINGRPY.append( "msgbuf_head is null. " );
				else
					PINGRPY.append( "msgbuf_head is NOT null. " );
				
				/*output status to tracker server*/
                if (!RunAsDaemon || AddDebug)
                    printf("[%d] FG_TRACKER::TrackerLoop PING from server received\n",pid);
                SWRITE (m_TrackerSocket,PINGRPY.c_str(),strlen(PINGRPY.c_str())+1);
				PINGRPY.erase();
				out.str("");
            }
            else
            {
                SG_LOG (SG_SYSTEMS, SG_ALERT, "["<< pid <<"] FG_TRACKER::TrackerLoop: Responce not recognized. Msg: " << res << "");
                printf("[%d] FG_TRACKER::TrackerLoop: Responce not recognized. Msg: %s",pid,res);
            }
			strcpy ( res, "" );
        }
		
		length = 0;
		
		/*Send message if necessary*/
        if (pkt_sent<max_msg_sent)
        {   // get message from queue
	
			if (resentflg==true)
			{/*msg from buffer*/
				if (msgbuf_head==NULL)
				{/*All msg sent and well received*/
					printf("[%d] FG_TRACKER::TrackerLoop Resend data completed.\n",pid);
					resentflg=false;
					msgbuf_resend=NULL;
					msgbuf_tail=NULL;
					continue;
				} else if (msgbuf_tail == msgbuf_resend)/*All msg sent, waiting ACK*/
					continue;
				else if (msgbuf_resend==NULL)
				{/*Need to send msg after the connect.*/
					msgbuf_resend=msgbuf_head;
					msgbuf_new=msgbuf_resend;
					length=strlen(msgbuf_new->msg);
				}else
				{/*Need to send further message*/
					msgbuf_resend=msgbuf_resend->next;
					msgbuf_new=msgbuf_resend;
					length=strlen(msgbuf_new->msg);
				}
			}	
			else
			{/*msg from queue*/
				if (msgque_head==NULL) /*No message pending to send at all*/
					continue;
				msgbuf_new=msgque_head;
				msgque_head=msgque_head->next;
				
				if (msgbuf_head == NULL) /*No message in buffer at all*/
					msgbuf_head = msgbuf_new;
				else
					msgbuf_tail->next=msgbuf_new;
				msgbuf_tail = msgbuf_new;
				msgbuf_tail->next = NULL;
				length=strlen(msgbuf_new->msg);
			}
        } else continue; /*buffer full. Don't send msg*/
        if ( length > 0 ) /*confirm if the message length is > 0*/
        {
			length++; /*including the '\0'*/
			
			/*Send message at msgbuf_new via tcp*/
			if (!RunAsDaemon || AddDebug) 
			{
				printf("[%d] FG_TRACKER::TrackerLoop sending msg %d bytes, addr %p, next addr %p\n",pid,length,msgbuf_new,msgbuf_new->next);
				printf("[%d] FG_TRACKER::TrackerLoop Msg: %s\n",pid,msgbuf_new->msg); 
			}
			if (SWRITE (m_TrackerSocket,msgbuf_new->msg,length) < 0)
			{
				printf("[%d] FG_TRACKER::TrackerLoop Can't write to server...\n",pid);
				SG_LOG (SG_SYSTEMS, SG_ALERT, "["<< pid <<"] FG_TRACKER::TrackerLoop: Can't write to server...");
				connected=Connect();
				msgbuf_resend=NULL;
				pkt_sent=0;
				strcpy ( res, "" );
				resentflg = true;
			}
			else	pkt_sent++;
        }
		else
        {	/*should not happen at all!*/
            printf("[%d] FG_TRACKER::TrackerLoop: Internal Error (Msg size <= 0). Contact programmer to fix this issue.\n",pid);
			SG_LOG (SG_SYSTEMS, SG_ALERT, "["<< pid <<"] FG_TRACKER::TrackerLoop: Internal Error (Msg size <= 0). Contact programmer to fix this issue.");
        }
    }
    return (0);
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
    bool connected = false;
    pid_t pid = getpid();
    //////////////////////////////////////////////////
    // close all inherited open sockets, but
    // leave cin, cout, cerr open
    //////////////////////////////////////////////////
#ifndef _MSC_VER
    for (int i=3; i<32; i++)
        SCLOSE (i);
#endif // !_MSC_VER
    if ( m_TrackerSocket > 0 )
        SCLOSE (m_TrackerSocket);
    if (!RunAsDaemon || AddDebug)
        printf("[%d] FG_TRACKER::Connect: Server: %s, Port: %d\n",pid, m_TrackerServer, m_TrackerPort);
    if (connected == false)
    {
        m_TrackerSocket = TcpConnect (m_TrackerServer, m_TrackerPort);
        if (m_TrackerSocket >= 0)
        {
            SG_LOG (SG_SYSTEMS, SG_ALERT, "["<< pid <<"] FG_TRACKER::Connect: success");
            connected = true;
        }
        else
        {
            SG_LOG (SG_SYSTEMS, SG_ALERT, "["<< pid <<"] FG_TRACKER::Connect: Failed to connect to fgtracker! Wait " << DEF_TRACKER_SLEEP << " seconds and retry.");
			printf("[%d] FG_TRACKER::Connect: Failed to connect to fgtracker! Wait %d seconds and retry.\n",pid,DEF_TRACKER_SLEEP);
			return false;
        }
    }
    SWRITE(m_TrackerSocket,"",1);
	sleep(2);
    SWRITE(m_TrackerSocket,"NOWAIT",7);
    if (!RunAsDaemon || AddDebug)
        printf("[%d] FG_TRACKER::Connect: Written 'NOWAIT'\n",pid);
	sleep(1);
    return true;
} // Connect ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  disconnect the tracker from its server
//
//////////////////////////////////////////////////////////////////////
void
FG_TRACKER::Disconnect ()
{
    if ( m_TrackerSocket > 0 )
        SCLOSE (m_TrackerSocket);
    m_TrackerSocket = 0;
} // Disconnect ()
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#if !defined(NTDDI_VERSION) || !defined(NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA)   // if less than VISTA, provide alternative
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT    97      /* not present in errno.h provided with VC */
#endif
int inet_aton(const char *cp, struct in_addr *addr)
{
    addr->s_addr = inet_addr(cp);
    return (addr->s_addr == INADDR_NONE) ? -1 : 0;
}
int inet_pton(int af, const char *src, void *dst)
{
    if (af != AF_INET)
    {
        errno = EAFNOSUPPORT;
        return -1;
    }
    return inet_aton (src, (struct in_addr *)dst);
}
#endif // #if (NTDDI_VERSION < NTDDI_VISTA)
#endif // _MSC_VER

//////////////////////////////////////////////////////////////////////
//
//  creates a TCP connection
//
//////////////////////////////////////////////////////////////////////
int
FG_TRACKER::TcpConnect (char *server_address,int server_port)
{
    struct sockaddr_in serveraddr;
    int sockfd;
    pid_t pid = getpid();
	
	sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if (SERROR(sockfd))
    {
		printf( "[%d] FG_TRACKER::TcpConnect: Can't get socket...\n",pid);
        return -1;
    }
    bzero(&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(server_port);
#ifdef _MSC_VER
    if ( inet_pton(AF_INET, server_address, &serveraddr.sin_addr) == -1 )
    {
        printf( "[%d] FG_TRACKER::TcpConnect: inet_pton failed!\n",pid);
        if (!SERROR(sockfd))
            SCLOSE(sockfd);
        return -1;
    }
#else
    inet_pton(AF_INET, server_address, &serveraddr.sin_addr);
#endif

    if (connect(sockfd, (SA *) &serveraddr, sizeof(serveraddr))<0 )
    {
        if (!SERROR(sockfd))
            SCLOSE(sockfd); /*close the socket*/
		printf( "[%d] FG_TRACKER::TcpConnect: Connect failed!\n",pid);
        return -1;
    }
    else
    {	
#ifdef _MSC_VER
        u_long opt = 1;
        ioctlsocket(sockfd,FIONBIO,&opt);
#else // !_MSC_VER
		if(fcntl(sockfd, F_GETFL) & O_NONBLOCK) 
		{
			// socket is non-blocking
			SG_LOG (SG_SYSTEMS, SG_ALERT, "["<< pid <<"] FG_TRACKER::TcpConnect: Socket is in non-blocking mode");
			printf( "[%d] FG_TRACKER::TcpConnect: Socket is in non-blocking mode\n",pid);
		}
		else
		{
			SG_LOG (SG_SYSTEMS, SG_ALERT, "["<< pid <<"] FG_TRACKER::TcpConnect: Socket is in blocking mode");
			printf( "[%d] FG_TRACKER::TcpConnect: Socket is in blocking mode\n",pid);
			if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK) < 0)
			{
				printf( "[%d] FG_TRACKER::TcpConnect: FAILED to set the socket to non-blocking mode\n",pid);
			}
			else
				printf( "[%d] FG_TRACKER::TcpConnect: Set the socket to non-blocking mode\n",pid);
		}
#endif // MSC_VER y/n
		return (sockfd);
	}
}  // TcpConnect  ()

//////////////////////////////////////////////////////////////////////
//
//  Signal handling
//
//////////////////////////////////////////////////////////////////////
void
signal_handler(int s)
{
#ifndef _MSC_VER
	pid_t mypid = getpid();
	
	switch (s)
	{
		case  1:
			printf("[%d] SIGHUP received, exiting...\n",mypid);
			exit(0);
			break;
		case  2:
			printf("[%d] SIGINT received, exiting...\n",mypid);
			exit(0);
			break;
		case  3:
			printf("[%d] SIGQUIT received, exiting...\n",mypid);
			break;
		case  4:
			printf("[%d] SIGILL received\n",mypid);
			break;
		case  5:
			printf("[%d] SIGTRAP received\n",mypid);
			break;
		case  6:
			printf("[%d] SIGABRT received\n",mypid);
			break;
		case  7:
			printf("[%d] SIGBUS received\n",mypid);
			break;
		case  8:
			printf("[%d] SIGFPE received\n",mypid);
			break;
		case  9:
			printf("[%d] SIGKILL received\n",mypid);
			exit(0);
			break;
		case 10:
			printf("[%d] SIGUSR1 received\n",mypid);
			break;
		case 11:
			printf("[%d] SIGSEGV received. Exiting...\n",mypid);
			exit(1);
			break;
		case 12:
			printf("[%d] SIGUSR2 received\n",mypid);
			break;
		case 13:
			printf("[%d] SIGPIPE received. Connection Error.\n",mypid);
			break;
		case 14:
			printf("[%d] SIGALRM received\n",mypid);
			break;
		case 15:
			printf("[%d] SIGTERM received\n",mypid);
			exit(0);
			break;
		case 16:
			printf("[%d] SIGSTKFLT received\n",mypid);
			break;
		case 17:
			printf("[%d] SIGCHLD received\n",mypid);
			break;
		case 18:
			printf("[%d] SIGCONT received\n",mypid);
			break;
		case 19:
			printf("[%d] SIGSTOP received\n",mypid);
			break;
		case 20: 
			printf("[%d] SIGTSTP received\n",mypid);
			break;
		case 21:
			printf("[%d] SIGTTIN received\n",mypid);
			break;
		case 22:
			printf("[%d] SIGTTOU received\n",mypid);
			break;
		case 23:
			printf("[%d] SIGURG received\n",mypid);
			break;
		case 24:
			printf("[%d] SIGXCPU received\n",mypid);
			break;
		case 25:
			printf("[%d] SIGXFSZ received\n",mypid);
			break;
		case 26:
			printf("[%d] SIGVTALRM received\n",mypid);
			break;
		case 27:
			printf("[%d] SIGPROF received\n",mypid);
			break;
		case 28:
			printf("[%d] SIGWINCH received\n",mypid);
			break;
		case 29: 
			printf("[%d] SIGIO received\n",mypid);
			break;
		case 30:
			printf("[%d] SIGPWR received\n",mypid);
			break;
		default:
			printf("[%d] signal %d received\n",mypid,s);

	}
#endif
}

