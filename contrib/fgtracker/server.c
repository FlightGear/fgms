/*
 * server.c - the tracker server itself
 *
 *   Author: Gabor Toth <tgbp@freemail.hu>
 *   License: GPL
 *
 *   $Log: server.c,v $
 *   Revision 1.2  2006/05/10 21:22:34  koversrac
 *   	Comment with author and license has been added.
 *   Revision 1.3 2012/07/04 geoff (reports _at_ geoffair _dot_ info)
 *      Add user configuration and help
 *   Revision 1.4 2012/08/06 geoff (reports _at_ geoffair _dot_ info)
 *      Fix missing -D y|n options, and chop PostgreSQL help if not compiled
 *   Revision 1.5 2012/11/09 hazuki (hstsuki_a-ie _at_ yahoo _dot_ com _dot_ hk)
 *      Better resistance on DoS attack. (Still a lot to be done though)
 */

#include "fgt_common.h"
#include "wrappers.h"
#include "fgt_error.h"
#ifndef NO_POSTGRESQL
#include <libpq-fe.h>
#endif // NO_POSTGRESQL
#if defined(_MSC_VER) || defined(USE_PTHREAD)
#include <pthread.h>
#endif // _MSC_VER or USE_PTHREAD

// static int run_as_daemon = RUN_AS_DAEMON;
static int run_as_daemon = 0;

static uint32_t server_port = SERVER_PORT;
static char *server_addr = 0;

/* postgresql default values */
#ifndef DEF_IP_ADDRESS
#define DEF_IP_ADDRESS "192.168.1.105"
#endif

#ifndef DEF_PORT
#define DEF_PORT "5432"
#endif

#ifndef DEF_DATABASE
#define DEF_DATABASE "fgtracker"
#endif

#ifndef DEF_USER_LOGIN
#define DEF_USER_LOGIN "fgtracker"
#endif

#ifndef DEF_USER_PWD
#define DEF_USER_PWD "fgtracker"
#endif

static char *ip_address = (char *)DEF_IP_ADDRESS;
static char *port = (char *)DEF_PORT;
static char *database = (char *)DEF_DATABASE;
static char *user = (char *)DEF_USER_LOGIN;
static char *pwd = (char *)DEF_USER_PWD;

#ifdef NO_POSTGRESQL
typedef struct tagPGconn {
    FILE * fp;
}PGconn;
#else // !NO_POSTGRESQL = use PostreSQL
static char *pgoptions = (char *)"";
static char *pgtty = (char *)"";
#endif // NO_POSTGRESQL

#ifdef _MSC_VER
#define pid_t int
int getpid(void) {
    return (int)GetCurrentThreadId();
}
#define snprintf _snprintf
#define sleep(a) Sleep(a * 1000)
#define usleep(a) uSleep(a)
void uSleep(int waitTime) { 
    LARGE_INTEGER _time1, _time2, _freq;
    _time1.QuadPart = 0;
    _time2.QuadPart = 0;
    _freq.QuadPart  = 0;
    QueryPerformanceCounter(&_time1); 
    QueryPerformanceFrequency(&_freq); 
 
    do { 
        QueryPerformanceCounter(&_time2); 
    } while ( (_time2.QuadPart - _time1.QuadPart) < waitTime ); 
} 
#endif // _MSC_VER


int
daemon_init(void)
{
#ifndef _MSC_VER
	pid_t	pid;

	if ( (pid = fork()) < 0)
		return(-1);
	else if (pid != 0)
		exit(0);	/* parent goes bye-bye */

	/* child continues */
	setsid();		/* become session leader */

	if(chdir("/"))		/* change working directory */
        return(1);

	umask(0);		/* clear our file mode creation mask */
#endif // !_MSC_VER

	return(0);
}

void sigchld_handler(int s)
{
#ifndef _MSC_VER
    pid_t childpid;
    char debugstr[MAXLINE];

    while( (childpid=waitpid(-1, NULL, WNOHANG)) > 0)
    {
		sprintf(debugstr,"Child stopped: %d",childpid);
		debug(1,debugstr);
    }
#endif // !_MSC_VER
}

void signal_handler(int s)
{
	#ifndef _MSC_VER
    char debugstr[MAXLINE];
	pid_t mypid = getpid();
	
	switch (s)
	{
		case  1:
			sprintf(debugstr,"[%d] SIGHUP received, exiting...",mypid);
			debug(1,debugstr);
			exit(0);
			break;
		case  2:
			sprintf(debugstr,"[%d] SIGINT received, exiting...",mypid);
			debug(1,debugstr);
			exit(0);
			break;
		case  3:
			sprintf(debugstr,"[%d] SIGQUIT received, exiting...",mypid);
			break;
		case  4:
			sprintf(debugstr,"[%d] SIGILL received",mypid);
			break;
		case  5:
			sprintf(debugstr,"[%d] SIGTRAP received",mypid);
			break;
		case  6:
			sprintf(debugstr,"[%d] SIGABRT received",mypid);
			break;
		case  7:
			sprintf(debugstr,"[%d] SIGBUS received",mypid);
			break;
		case  8:
			sprintf(debugstr,"[%d] SIGFPE received",mypid);
			break;
		case  9:
			sprintf(debugstr,"[%d] SIGKILL received",mypid);
			debug(1,debugstr);
			exit(0);
			break;
		case 10:
			sprintf(debugstr,"[%d] SIGUSR1 received",mypid);
			break;
		case 11:
			sprintf(debugstr,"[%d] SIGSEGV received. Exiting...",mypid);
			exit(1);
			break;
		case 12:
			sprintf(debugstr,"[%d] SIGUSR2 received",mypid);
			break;
		case 13:
			sprintf(debugstr,"[%d] SIGPIPE received. Connection Error. Exiting...",mypid);
			debug(1,debugstr);
			exit(0);
			break;
		case 14:
			sprintf(debugstr,"[%d] SIGALRM received",mypid);
			break;
		case 15:
			sprintf(debugstr,"[%d] SIGTERM received",mypid);
			debug(2,debugstr);
			exit(0);
			break;
		case 16:
			sprintf(debugstr,"[%d] SIGSTKFLT received",mypid);
			break;
		case 17:
			sprintf(debugstr,"[%d] SIGCHLD received",mypid);
			break;
		case 18:
			sprintf(debugstr,"[%d] SIGCONT received",mypid);
			break;
		case 19:
			sprintf(debugstr,"[%d] SIGSTOP received",mypid);
			break;
		case 20: 
			sprintf(debugstr,"[%d] SIGTSTP received",mypid);
			break;
		case 21:
			sprintf(debugstr,"[%d] SIGTTIN received",mypid);
			break;
		case 22:
			sprintf(debugstr,"[%d] SIGTTOU received",mypid);
			break;
		case 23:
			sprintf(debugstr,"[%d] SIGURG received",mypid);
			break;
		case 24:
			sprintf(debugstr,"[%d] SIGXCPU received",mypid);
			break;
		case 25:
			sprintf(debugstr,"[%d] SIGXFSZ received",mypid);
			break;
		case 26:
			sprintf(debugstr,"[%d] SIGVTALRM received",mypid);
			break;
		case 27:
			sprintf(debugstr,"[%d] SIGPROF received",mypid);
			break;
		case 28:
			sprintf(debugstr,"[%d] SIGWINCH received",mypid);
			break;
		case 29: 
			sprintf(debugstr,"[%d] SIGIO received",mypid);
			break;
		case 30:
			sprintf(debugstr,"[%d] SIGPWR received",mypid);
			break;
		default:
			sprintf(debugstr,"[%d] signal %d received",mypid,s);

	}
	debug(2,debugstr);
	#endif // !_MSC_VER
}

#ifdef NO_POSTGRESQL
#define CONNECTION_OK  0
#define CONNECTION_FAILED -1
int PQstatus(PGconn *conn)
{
    if (conn && conn->fp)
        return CONNECTION_OK;
    return CONNECTION_FAILED;
}
int PQfinish( PGconn *conn )
{
    if (conn) {
        if (conn->fp)
            fclose(conn->fp);
        conn->fp = 0;
        free(conn);
    }
    return 0;
}

#ifndef DEF_MESSAGE_LOG
#define DEF_MESSAGE_LOG "fgt_server.log"
#endif

static char *msg_log = (char *)DEF_MESSAGE_LOG;
static FILE *msg_file = NULL;
static void write_message_log(const char *msg, int len)
{
    if (msg_file == NULL) {
        msg_file = fopen(msg_log,"ab");
        if (!msg_file) {
            printf("ERROR: Failed to OPEN/append %s log file!\n", msg_log);
            msg_file = (FILE *)-1;
        }
    }
    if (len && msg_file && (msg_file != (FILE *)-1)) {
        int wtn = (int)fwrite(msg,1,len,msg_file);
        if (wtn != len) {
            fclose(msg_file);
            msg_file = (FILE *)-1;
            printf("ERROR: Failed to WRITE %d != %d to %s log file!\n", wtn, len, msg_log);
        } else {
            if (msg[len-1] != '\n')
                fwrite((char *)"\n",1,1,msg_file);
            fflush(msg_file);   // push it to disk now
        }
    }
}

#endif // NO_POSTGRESQL

/* --------------------------------------
PGconn *PQsetdbLogin(const char *pghost,
                     const char *pgport,
                     const char *pgoptions,
                     const char *pgtty,
                     const char *dbName,
                     const char *login,
                     const char *pwd);
   ------------------------------------- */


int ConnectDB(PGconn **conn)
{
    int iret = 0;   /* assume no error */
    char debugstr[MAXLINE];
#ifdef NO_POSTGRESQL
    FILE *fp;
    PGconn *cp;
    *conn = (PGconn *)malloc(sizeof(PGconn));
    if (!*conn) {
        printf("ERROR: memory allocation FAILED! Aborting...\n");
        exit(1);
    }
    fp = fopen(database,"a");
    cp = *conn;
    if (fp) {
        cp->fp = fp;
    } else {
   	    sprintf(debugstr, "Connection to database failed: %s. NO OPEN/Append",database);
    	debug(1,debugstr);
    	PQfinish(*conn);
        iret = 1;
    }
#else // !NO_POSTGRESQL

    *conn = PQsetdbLogin(ip_address, port, pgoptions, pgtty, database, user, pwd);

	if (PQstatus(*conn) != CONNECTION_OK)
   {
   	sprintf(debugstr, "Connection to database failed: %s",PQerrorMessage(*conn));
    	debug(1,debugstr);
    	PQfinish(*conn);
    	iret = 1;
  	}
#endif // NO_POSTGRESQL y/n
  	return iret;
}

int logFlight(PGconn *conn,char *callsign,char *model, char *date, int connect)
{
#ifdef NO_POSTGRESQL
    return 0;
#else // !#ifdef NO_POSTGRESQL
  PGresult		*res;
  char			statement[MAXLINE];
  char			date2[MAXLINE];
  char			callsign2[MAXLINE];
  char			model2[MAXLINE];
  int			error;
  char			debugstr[MAXLINE];

  PQescapeStringConn (conn, date2, date, MAXLINE, &error);
  PQescapeStringConn (conn, callsign2, callsign, MAXLINE, &error);
  PQescapeStringConn (conn, model2, model, MAXLINE, &error);

  sprintf(statement,"UPDATE flights SET status='CLOSED',end_time='%s' WHERE callsign='%s' AND status='OPEN';",date2,callsign2);
  res = PQexec(conn, statement);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    sprintf(debugstr, "Command failed: %s. Force exit.", PQerrorMessage(conn));
    debug(1,debugstr);
    PQclear(res);
	exit (1);
  }

  if (connect)
  {
    sprintf(statement,"INSERT INTO flights (callsign,status,model,start_time) VALUES ('%s','OPEN','%s','%s');",callsign2,model2,date2);
    res = PQexec(conn, statement);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
      sprintf(debugstr, "Command failed: %s. Force exit.", PQerrorMessage(conn));
      debug(1,debugstr);
      PQclear(res);
      exit (1);
    }
  }

  return(0);
#endif // #ifdef NO_POSTGRESQL y/n
 
}

int logPosition(PGconn *conn, char *callsign, char *date, char *lon, char *lat, char *alt)
{
#ifdef NO_POSTGRESQL
    return 0;
#else // !#ifdef NO_POSTGRESQL
  PGresult		*res;
  char			statement[MAXLINE];
  char			date2[MAXLINE];
  char			callsign2[MAXLINE];
  char			lon2[MAXLINE];
  char			lat2[MAXLINE];
  char			alt2[MAXLINE];
  int			error;
  char			debugstr[MAXLINE];

  PQescapeStringConn (conn, date2, date, MAXLINE, &error);
  PQescapeStringConn (conn, callsign2, callsign, MAXLINE, &error);
  PQescapeStringConn (conn, lon2, lon, MAXLINE, &error);
  PQescapeStringConn (conn, lat2, lat, MAXLINE, &error);
  PQescapeStringConn (conn, alt2, alt, MAXLINE, &error);

  sprintf(statement,"	INSERT INTO waypoints "
		    "		(flight_id,time,latitude,longitude,altitude) "
		    "	VALUES ((SELECT id FROM flights WHERE callsign='%s' and status='OPEN'),'%s',%s,%s,%s);",callsign2,date2,lat2,lon2,alt2);

  res = PQexec(conn, statement);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    sprintf(debugstr, "Command failed: %s. Force exit.", PQerrorMessage(conn));
    debug(1,debugstr);
    PQclear(res);
	exit (1);
  }

  return(0);
#endif // #ifdef NO_POSTGRESQL y/n
}

/* =============================================
   FIX20120812 - Due to the fact that at present
   the CALLSIGN sent from fgfs to fgms, and 
   from fgms to fgt_server can contain SPACE
   write a parser to not trip on the space.
   BUT this parser depend on the FACT that 
   at the moment fgms uses ' test ' as the passwd 
   in the ASCII message transferred.
   ============================================= */

// TODO - For Windows must shift out strerror(errno)
// providing and alternative for windows sockets

// establish message types
enum MsgType {
    MT_UNKNOWN,
    MT_REPLY,
    MT_PING,
    MT_CONNECT,
    MT_DISCONNECT,
    MT_POSITION,
	MT_PONG,
	MT_NOWAIT
};

// some easy very specific macros
#define EatSpace    for ( ; i < len; i++ ) { \
                        if (msg[i] > ' ') break; }

#define Collect(a)  off = 0;\
    for ( ; i < len; i++ ) { \
        c = msg[i]; \
        if ( c <= ' ' ) break; \
        a[off++] = (char)c; \
    } \
    a[off] = 0
    
#define ISUPPER(a) (( a >= 'A' )&&( a <= 'Z' ))

int parse_message( char * msg, char *event,
   char *callsign, char *passwd, char *model, 
   char *lat, char *lon, char *alt,
   char *time1, char *time2 )
{
    int iret = MT_UNKNOWN;
    size_t len = strlen(msg);
    size_t i, off, j;
    int c;
    char *cp;

    off = 0;
    // forget some 'bad' message
    if ( !ISUPPER(*msg) )
        return MT_UNKNOWN; // does NOT start with upper

    for ( i = 0; i < len; i++ ) {
        c = msg[i];
        if (!ISUPPER(c))
            break;
        event[off++] = (char)c;
    }
    event[off] = 0; // zero terminate the string

    // deal with some short message types
	if (strcmp(event,"NOWAIT") == 0)
        return MT_NOWAIT;    // all done
    if (strcmp(event,"REPLY") == 0)
        return MT_REPLY;    // all done
    else if (strcmp(event,"PING") == 0)
        return MT_PING; // all done
	else if (strcmp(event,"PONG") == 0)
        return MT_PONG; // all done
    else if (strcmp(event,"CONNECT") == 0)
        iret = MT_CONNECT;
    else if (strcmp(event,"DISCONNECT") == 0)
        iret = MT_DISCONNECT;
    else if (strcmp(event,"POSITION") == 0)
        iret = MT_POSITION;
    else
        return MT_UNKNOWN;  // should not happen, but...

    EatSpace;
    // Expect up to an 8 character CALLSIGN, but 
    // unfortunatley CAN have spaces, so...
    Collect(callsign);
    // it WILL currently be followed by 'test '
    //                   123456
    cp = strstr(&msg[i]," test ");
    if (!cp) {
        // TODO - could return unknown, but
        //return MT_UNKNOWN;
        cp = &msg[i];
    }
    j = (size_t)( cp - &msg[i] );
    // collect BALANCE of the callsign
    // TODO - Maybe here CHANGE spaces to some
    // other character so no trouble with SQL
    // commands as well...
    for ( ; j && (i < len); i++) {
        c = msg[i];
        if (c <= ' ')
            c = '-';
        callsign[off++] = (char)c;
        j--;
    }
    callsign[off] = 0;

    EatSpace;
    Collect(passwd);
    EatSpace;

    // now changes for type
    if (iret == MT_POSITION) {
        Collect(lat);
        EatSpace;
        Collect(lon);
        EatSpace;
        Collect(alt);
        // TODO - Could validate these values
    } else {    // if ((iret == MT_CONNECT)||(iret == MT_DISCONNECT))
        Collect(model);
    }

    // collect time strings - TODO - could add validation
    EatSpace;
    Collect(time1); // date YYYY-MM-DD
    EatSpace;
    Collect(time2); // time HH:MM:SS
    return iret;
}
// remove macros
#undef EatSpace
#undef Collect

void doit(int fd)
{
    // put these BIG buffers outside the function stack
	static char debugstr[MAXLINE];
    static char msg[MSGMAXLINE];
    static char event[MAXLINE];
    static char callsign[MAXLINE];
    static char passwd[MAXLINE];
    static char model[MAXLINE];
    static char time1[MAXLINE];
    static char time2[MAXLINE];
    static char time[MAXLINE];
    static char lon[MAXLINE];
    static char lat[MAXLINE];
    static char alt[MAXLINE];
	static char *clientip;
	char b;
    uint16_t clientport =1;
	int i=0;
	int pg_reconn_counter;
	short int time_out_counter_l=1;
	unsigned int time_out_counter_u=0;
	short int time_out_fraction=20; /* 1000000/time_out_fraction must be integer*/
    int len;
    pid_t mypid;
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen;
    PGconn *conn = NULL;
    short int reply = 0;
    short int nowait = 0;
	short int sockect_read_completed =0;
    int res, sendok;
	unsigned long no_of_line=0;
	
#ifdef _MSC_VER
    u_long opts = 1;
    int status = ioctlsocket(fd, FIONBIO, &opts);
    mypid = getpid();
    if (SERROR(status)) {
		sprintf(debugstr,"[%d] FAILED to set the socket to non-blocking mode. Exiting...",mypid);
		exit (0);
    } else {
		sprintf(debugstr,"[%d] Socket set to non-blocking mode. %d scans per second",mypid,time_out_fraction);
    }
#else // !_MSC_VER
    mypid = getpid();
	if(fcntl(fd, F_GETFL) & O_NONBLOCK) 
	{
		// socket is non-blocking
		sprintf(debugstr,"[%d] Socket is in non-blocking mode",mypid);
	} else
	{
		sprintf(debugstr,"[%d] Socket is in blocking mode",mypid);
		debug(2,debugstr);
		if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) < 0)
		{
			sprintf(debugstr,"[%d] FAILED to set the socket to non-blocking mode. Exiting...",mypid);
			exit (0);
		}	
		else
			sprintf(debugstr,"[%d] Socket set to non-blocking mode. %d scans per second",mypid,time_out_fraction);

	}
#endif // _MSC_VER y/n
	debug(2,debugstr);
	
	clientaddrlen = sizeof(struct sockaddr_in);
    while (getpeername(fd, (SA *) &clientaddr, &clientaddrlen)==-1)
	{
		sprintf(debugstr,"[%d] Failed to get IP address of client! 1 sec to retry.",mypid);
		debug(2,debugstr);
		if (i>10)
		{
			sprintf(debugstr,"[%d] Stop retrying to get the client address.",mypid);
			debug(2,debugstr);
			clientip = "Unknown";
			break;
		}	
		sleep(1);
		i++;
	}
	if (i<=10)
	{
		sprintf(debugstr,"[%d] Got IP address of client. CIP %s POR %d",mypid,inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));	
		debug(2,debugstr);
		clientip = inet_ntoa(clientaddr.sin_addr);	
		clientport = ntohs(clientaddr.sin_port);
	}
	i=0;

    while (1)
    {
        i=0;
		len = strlen(msg);
		
		if (nowait==0)
		{
			len = SREAD(fd, msg, MSGMAXLINE-1);
			if (len>0)
				sockect_read_completed=1;
		}
		else
		{
			do
			{	/*Maximun character in a line : MSGMAXLINE-1. msg[MSGMAXLINE] = '\0'*/
				if (len==MSGMAXLINE-1)
				{
					sockect_read_completed=1;
					break;
				}
				i = recv( fd, &b, sizeof( b ), 0 );
				msg[len]=b;
				if (b=='\0' && i>=1)
				{
					sockect_read_completed=1;
					break;
				}
				if (i<1)
					break;
			}while(len++);
		}
		i=0;
		msg[len]='\0';
		
		if (sockect_read_completed==0)
		{
			usleep(1000000/time_out_fraction);
			time_out_counter_l++;
			if (time_out_counter_l==time_out_fraction)
			{
				time_out_counter_u++;
				time_out_counter_l=0;
			}
			if (time_out_counter_u%60==0 && time_out_counter_u >=120 && time_out_counter_l==0)
			{	/*Print warning*/
				snprintf(debugstr,MAXLINE,"[%d] %s:%d: Warning: No data receive from client for %d seconds",mypid,clientip,clientport,time_out_counter_u);
				debug(1,debugstr);
			}
			if (time_out_counter_u%120==0 && time_out_counter_l==0) 
			{	/*Send PING*/
				if (SWRITE(fd,"PING",5) != 5)
				{
					sprintf(debugstr,"[%d] %s:%d: Write PING failed! - %s", mypid, clientip,clientport,strerror(errno));
					sprintf(debugstr,"[%d] %s:%d: Connection lost. Exiting...", mypid, clientip,clientport);
					debug(1,debugstr);
					exit(0);
				}
				else
					sprintf(debugstr,"[%d] %s:%d: Wrote PING. Waiting reply", mypid, clientip,clientport);
				debug(1,debugstr);
			}
			if (time_out_counter_u%300==0 && time_out_counter_l==0)/*Exit*/
				break;
			continue;
		}
		sockect_read_completed=0;
		time_out_counter_l=1;
		time_out_counter_u=0;
		no_of_line++;
        snprintf(debugstr,MAXLINE,"[%d] %s:%d: Read %d bytes",mypid,clientip,clientport,len);
        debug(3,debugstr);

        // no need to clear the WHOLE buffer - just first byte
        event[0] = 0;
        callsign[0] = 0;
        passwd[0] = 0;
        model[0] = 0;
        time1[0] = 0;
        time2[0] = 0;
        time[0] = 0;
        lon[0] = 0;
        lat[0] = 0;
        alt[0] = 0;

        // parse the message
        res = parse_message( msg, event, callsign, passwd, model, lat, lon, alt, time1, time2 );
        sprintf(debugstr,"[%d] %s:%d: msg: %s",mypid,clientip,clientport,msg);
        debug(3,debugstr);

        pg_reconn_counter=0;
#ifdef NO_POSTGRESQL
        write_message_log(msg,strlen(msg));
#else // !#ifdef NO_POSTGRESQL
        while ( (PQstatus(conn) != CONNECTION_OK) && (pg_reconn_counter<120) ) 
		{
            sprintf(debugstr,"[%d] %s:%d: (Re)Connnet loop - %d",mypid,clientip,clientport,pg_reconn_counter);
            debug(3,debugstr);
            pg_reconn_counter++;
            PQfinish(conn);
            ConnectDB(&conn);
            sleep(1);
            if (PQstatus(conn)==CONNECTION_OK)
            {
                sprintf(debugstr,"[%d] %s:%d: (Re)Connected to PQ successful after %d tries",mypid,clientip,clientport,pg_reconn_counter);
                debug(1,debugstr);
            }
            else
            {
                if (pg_reconn_counter%10==0)
                {
                    sprintf(debugstr,"[%d] %s:%d: Reconnected to PQ failed after %d tries",mypid,clientip,clientport,pg_reconn_counter);
                    debug(1,debugstr);
                }
            }
        }
#endif // #ifdef NO_POSTGRESQL
        pg_reconn_counter=0;

        sendok = 0; // some messages require an 'OK' reply, if a 'REPLY' command was received

		switch (res) 
		{
			case MT_NOWAIT: /*starting from 0.10.22 */
				reply = 1;  // Client want REPLY to each message
				nowait = 1;// Client will not wait ACK before sending another message
				sprintf(debugstr,"[%d] %s:%d: NOWAIT mode is ON", mypid, clientip,clientport);
				debug(1,debugstr);
				break;
			case MT_REPLY: /*0.10.21 and below*/
				reply = 1;  // Client want REPLY to each message
				sprintf(debugstr,"[%d] %s:%d: REPLY mode is ON", mypid, clientip,clientport);
				debug(1,debugstr);
				break;
			case MT_UNKNOWN:
			        if (no_of_line!=1)
					{ 	//first line from fgms will be ignored.
						sprintf(debugstr,"[%d] %s:%d: Unknown msg received", mypid, clientip,clientport);
						debug(1,debugstr);
					}
				break;
			case MT_PING:
				if (SWRITE(fd,"PONG",5) != 5) {
					sprintf(debugstr,"[%d] %s:%d: write PONG failed! - %s", mypid, clientip,clientport,strerror(errno));
					debug(1,debugstr);
				}
				break;
			case MT_PONG:
					sprintf(debugstr,"[%d] %s:%d: PONG Received", mypid, clientip,clientport);
					debug(1,debugstr);
				break;
			case MT_CONNECT:
				sprintf(time,"%s %s Z",time1,time2);
				if ((strncmp("mpdummy",callsign,7) != 0) && (strncmp("obscam",callsign,6) != 0))
					logFlight(conn,callsign,model,time,1);
				sendok = 1;
				break;
			case MT_DISCONNECT:
				sprintf(time,"%s %s Z",time1,time2);
				if ((strncmp("mpdummy",callsign,7) != 0) && (strncmp("obscam",callsign,6) != 0))
					logFlight(conn,callsign,model,time,0);
				sendok = 1;
				break;
			case MT_POSITION:
				sprintf(time,"%s %s Z",time1,time2);
				if (strncmp("mpdummy",callsign,7) != 0 && strncmp("obscam",callsign,6) != 0)
					logPosition(conn,callsign,time,lon,lat,alt);
				sendok = 1;
				break;
			default:
				sprintf(debugstr,"[%d] %s:%d: Uncased message received!",mypid,clientip,clientport);
				debug(1,debugstr);
				sendok = 1; // not a valid message but server wating reply, so...
				break;
        }
        if (sendok && reply) {
            if (SWRITE(fd,"OK",3) != 3) {
                sprintf(debugstr,"[%d] %s:%d: Write OK failed - %s", mypid, clientip,clientport,strerror(errno));
                sprintf(debugstr,"[%d] %s:%d: Connection lost. Exiting...", mypid, clientip,clientport);
				debug(1,debugstr);
				exit(0);
            } else {
                sprintf(debugstr,"[%d] %s:%d: Reply sent.", mypid,clientip,clientport);
                debug(3,debugstr);
            }
        }
		strcpy(msg,""); /*clear message*/
    }
    sprintf(debugstr,"[%d] %s:%d: Connection timeout after %d seconds. Exiting...", mypid, clientip,clientport,time_out_counter_u);
    debug(1,debugstr);
}

#if 0 // code to be removed when replacement found to be working

void doit2(int fd)
{
  int pg_reconn_counter;
  int len;
  char debugstr[MAXLINE];
  char msg[MAXLINE];
  char event[MAXLINE];
  char callsign[MAXLINE];
  char passwd[MAXLINE];
  char model[MAXLINE];
  char time1[MAXLINE];
  char time2[MAXLINE];
  char time[MAXLINE];
  char lon[MAXLINE];
  char lat[MAXLINE];
  char alt[MAXLINE];
  pid_t mypid;
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen;
  PGconn *conn=NULL;
  int reply=0;

  mypid=getpid();

  getpeername(fd, (SA *) &clientaddr, &clientaddrlen);

#if defined(_MSC_VER) && (!defined(NTDDI_VERSION) || !defined(NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA))   // if less than VISTA, need alternative
  sprintf(debugstr,"connection on port %d", ntohs(clientaddr.sin_port));
#else
  sprintf(debugstr,"connection from %s, port %d", inet_ntop(AF_INET, &clientaddr.sin_addr, msg, sizeof(msg)), ntohs(clientaddr.sin_port));
#endif
  debug(2,debugstr);

  while ( (len = SREAD(fd, msg, MAXLINE)) >0)
  {
       	msg[len]='\0';

	snprintf(debugstr,MAXLINE,"Children [%5d] read %d bytes\n",mypid,len);

	debug(3,debugstr);

  	bzero(event,MAXLINE);
  	bzero(callsign,MAXLINE);
  	bzero(passwd,MAXLINE);
  	bzero(model,MAXLINE);
  	bzero(time1,MAXLINE);
  	bzero(time2,MAXLINE);
  	bzero(time,MAXLINE);
  	bzero(lon,MAXLINE);
  	bzero(lat,MAXLINE);
  	bzero(alt,MAXLINE);

	if (len>0)
	{	

		sscanf(msg,"%s ",event);
		sprintf(debugstr,"msg: %s",msg);
		debug(3,debugstr);

		pg_reconn_counter=0;
#ifdef NO_POSTGRESQL
        write_message_log(msg,len);
#else // !#ifdef NO_POSTGRESQL
		while ( (PQstatus(conn)!=CONNECTION_OK) && (pg_reconn_counter<120) )
                {
			sprintf(debugstr,"Reconn loop %d",pg_reconn_counter);
                        debug(3,debugstr);

			pg_reconn_counter++;
			PQfinish(conn);
			ConnectDB(&conn);
			sleep(1);
			if (PQstatus(conn)==CONNECTION_OK)
			{
				sprintf(debugstr,"Reconnected to PQ successful after %d tries",pg_reconn_counter);
				debug(1,debugstr);
			}
			else
			{
				if (pg_reconn_counter%10==0)
				{
					sprintf(debugstr,"Reconnected to PQ failed after %d tries",pg_reconn_counter);
					debug(1,debugstr);
				}
			}

                }
#endif // #ifdef NO_POSTGRESQL
		pg_reconn_counter=0;

		if (strncmp("REPLY",event,5)==0)
		{
			
			reply=1;
		}

		if (strncmp("PING",event,4)==0)
		{
			if (SWRITE(fd,"PONG",5) != 5)
                debug(1,"write PONG failed");
		}

		if (strncmp("CONNECT",event,7)==0)
		{
       			sscanf(msg,"%s %s %s %s %s %s",event,callsign,passwd,model,time1,time2);
       			sprintf(time,"%s %s Z",time1,time2);
       			if (strncmp("mpdummy",callsign,7)!=0 && strncmp("obscam",callsign,6)!=0) logFlight(conn,callsign,model,time,1);
			if (reply)
			{
				if (SWRITE(fd,"OK",3) != 3)
                    debug(1,"write OK (CONNECT) failed");
                else
                    debug(3,"reply sent");
			}
		}
		else if (strncmp("DISCONNECT",event,10)==0)
		{
       			sscanf(msg,"%s %s %s %s %s %s",event,callsign,passwd,model,time1,time2);
       			sprintf(time,"%s %s Z",time1,time2);
       			if (strncmp("mpdummy",callsign,7)!=0 && strncmp("obscam",callsign,6)!=0) logFlight(conn,callsign,model,time,0);
			if (reply)
			{
				if (SWRITE(fd,"OK",3) != 3)
                    debug(1,"write OK (DISCONNECT) failed");
                else
                    debug(3,"reply sent");
			}
		}
       		else if (strncmp("POSITION",event,8)==0)
		{
       			sscanf(msg,"%s %s %s %s %s %s %s %s",event,callsign,passwd,lat,lon,alt,time1,time2);
       			sprintf(time,"%s %s Z",time1,time2);
       			if (strncmp("mpdummy",callsign,7)!=0 && strncmp("obscam",callsign,6)!=0) logPosition(conn,callsign,time,lon,lat,alt);
			if (reply)
			{
				if (SWRITE(fd,"OK",3) != 3)
                    debug(1,"write OK (POSITION) failed");
                else
                    debug(3,"reply sent");
			}
		}
	}
  }
}

#endif // 0 - code to be removed when replacement tested

char *get_base_name(char *name)
{
    char *bn = strdup(name);
    size_t len = strlen(bn);
    size_t i, off;
    int c;
    off = 0;
    for (i = 0; i < len; i++) {
        c = bn[i];
        if (( c == '/' )||( c == '\\' ))
            off = i + 1;
    }
    return &bn[off];
}

void give_help(char *name)
{
    char *bn = get_base_name(name);
    printf("%s - version 1.3, compiled %s, at %s\n", bn, __DATE__, __TIME__);
#ifndef NO_POSTGRESQL
    printf("PostgreSQL Database Information\n");
    printf(" --db database (-d) = Set the database name. (def=%s)\n",database);
    printf(" --ip addr     (-i) = Set the IP address of the postgresql server. (def=%s)\n",ip_address);
    printf(" --port val    (-p) = Set the port of the postgreql server. (def=%s)\n",port);
    printf(" --user name   (-u) = Set the user name. (def=%s)\n",user);
    printf(" --word pwd    (-w) = Set the password for the above user. (def=%s)\n",pwd);
#endif
    printf("fgms connection\n");
    printf(" --IP addr     (-I) = Set IP address to connect to fgms. (def=IPADDR_ANY)\n");
    printf(" --PORT val    (-P) = Set PORT address to connect to fgms. (dep=%d)\n", server_port);
    printf("General Options\n");
    printf(" --help    (-h. -?) = This help, and exit(0).\n");
    printf(" --version     (-v) = This help, and exit(0).\n");
#ifndef _MSC_VER
    printf(" --DAEMON y|n  (-D) = Run as daemon yes or no. (def=%s).\n", (run_as_daemon ? "y" : "n"));
#endif
    printf(" %s will connect to an instance of 'fgms', receive flight and position messages.\n",bn);
#ifdef NO_POSTGRESQL
    printf(" These will be written to a message log [%s].\n", msg_log ); 
#else
    printf(" These will be stored in a PostgreSQL database, for later 'tracker' display.\n");
#endif
}

int parse_commands( int argc, char **argv )
{
    int i, i2;
    char *arg;
    char *sarg;
    
    for ( i = 1; i < argc; i++ ) {
        i2 = i + 1;
        arg = argv[i];
        sarg = arg;
        if ((strcmp(arg,"--help") == 0) || (strcmp(arg,"-h") == 0) ||
            (strcmp(arg,"-?") == 0) || (strcmp(arg,"--version") == 0)||
            (strcmp(arg,"-v") == 0)) {
            give_help(argv[0]);
            exit(0);
        } else if (*sarg == '-') {
            sarg++;
            while (*sarg == '-') sarg++;
            switch (*sarg) 
            {
            case 'd':
                if (i2 < argc) {
                    sarg = argv[i2];
                    database = strdup(sarg);
                    i++;
                } else {
                    printf("database name must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'i':
                if (i2 < argc) {
                    sarg = argv[i2];
                    ip_address = strdup(sarg);
                    i++;
                } else {
                    printf("IP address must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'p':
                if (i2 < argc) {
                    sarg = argv[i2];
                    port = strdup(sarg);
                    i++;
                } else {
                    printf("port value must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'u':
                if (i2 < argc) {
                    sarg = argv[i2];
                    user = strdup(sarg);
                    i++;
                } else {
                    printf("user name must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'w':
                if (i2 < argc) {
                    sarg = argv[i2];
                    pwd = strdup(sarg);
                    i++;
                } else {
                    printf("password must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'I':
                if (i2 < argc) {
                    sarg = argv[i2];
                    server_addr = strdup(sarg);
                    i++;
                } else {
                    printf("fgms server IP address must follow!\n");
                    goto Bad_ARG;
                }
                break;
            case 'P':
                if (i2 < argc) {
                    sarg = argv[i2];
                    server_port = atoi(sarg);
                    i++;
                } else {
                    printf("fgms server PORT value must follow!\n");
                    goto Bad_ARG;
                }
                break;
#ifndef _MSC_VER
            case 'D':
                if (i2 < argc) {
                    sarg = argv[i2];
                    if (strcmp(sarg,"y") == 0) {
                        run_as_daemon = 1;
                    } else if (strcmp(sarg,"n") == 0) {
                        run_as_daemon = 0;
                    } else {
                        printf("Only 'y' or 'n' can follow -D!\n");
                        goto Bad_ARG;
                    }
                    i++;    // skip following argument
                } else {
                    printf("either 'y' or 'n' must follow!\n");
                    goto Bad_ARG;
                }
                break;
#endif
            default:
                goto Bad_ARG;
            }
        } else {
Bad_ARG:
            printf("ERROR: Unknown argument [%s]! Try -?\n",arg);
            exit(1);
        }
    }
    return 0;
}

#define PQ_EXEC_SUCCESS(res) ((PQresultStatus(res) == PGRES_COMMAND_OK)||(PQresultStatus(res) == PGRES_TUPLES_OK))
int check_tables(PGconn *conn)
{
#ifdef NO_POSTGRESQL
    return 0;
#else // !NO_POSTGRESQL
    PGresult *res;
    char buff[MAXLINE];
    char *cp = buff;
    char *val;
    int i, j, nFields, nRows;
    int got_flights = 0;
    int got_waypts = 0;
    if (PQstatus(conn) == CONNECTION_OK) {
        res = PQexec(conn,"BEGIN");
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            PQclear(res);
            return 1;
        }
        /* should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
        PQclear(res);
        strcpy(cp,"SELECT table_name FROM information_schema.tables WHERE table_schema = 'public';");
        res = PQexec(conn, cp);
        if (PQ_EXEC_SUCCESS(res)) {
            nFields = PQnfields(res);
            nRows = PQntuples(res);
            for (j = 0; j < nFields; j++) {
                for (i = 0; i < nRows; i++) {
                    val = PQgetvalue(res, i, j);
                    if (val) {
                        if (strcmp(val,"flights") == 0)
                            got_flights = 1;
                        else if (strcmp(val,"waypoints") == 0)
                            got_waypts = 1;
                    }
                }
            }
       } else {
            return 1;
       }
       PQclear(res);
       /* end the transaction */
       res = PQexec(conn, "END");
       PQclear(res);
    } else {
        return 1;
    }
    return ((got_flights && got_waypts) ? 0 : 1);
#endif // NO_POSTGRESQL y/n
}

int test_db_connection()
{
    int iret = 1;
    PGconn *conn = NULL;
    if (ConnectDB(&conn)) {
        return 1;   /* FAILED - conn closed */
    }
    iret = check_tables(conn);
    PQfinish(conn);
    return iret;
}

static void net_exit ( void )
{
#ifdef _MSC_VER
	/* Clean up windows networking */
	if ( WSACleanup() == SOCKET_ERROR ) {
		if ( WSAGetLastError() == WSAEINPROGRESS ) {
			WSACancelBlockingCall();
			WSACleanup();
		}
	}
#endif
}


int net_init ()
{
#ifdef _MSC_VER
	/* Start up the windows networking */
	WORD version_wanted = MAKEWORD(1,1);
	WSADATA wsaData;

	if ( WSAStartup(version_wanted, &wsaData) != 0 ) {
		printf("Couldn't initialize Winsock 1.1\n");
		return(1);
	}
#endif

    atexit( net_exit ) ;
	return(0);
}

static pid_t pid,childpid;
static int listenfd, connfd;

#if defined(_MSC_VER) || defined(USE_PTHREAD)
static pthread_t thread;
void *func_child(void *vp)
{
	static char debugstr2[MAXLINE];

	childpid=getpid();

	sprintf(debugstr2,"CLIENT[%5d] started",childpid);
	debug(2,debugstr2);

	doit(connfd);
			
	Close(connfd);
	sprintf(debugstr2,"CLIENT[%5d] stopped",childpid);
	debug(2,debugstr2);

    return ((void *)0xdead);
}
#endif // _MSC_VER || USE_PTHREAD

int main (int argc, char **argv)
{
	char debugstr[MAXLINE];
	struct sockaddr_in serveraddr,clientaddr;
	socklen_t clientaddrlen;

    parse_commands(argc,argv);  /* parse user command line - no return if error */

    if (test_db_connection()) 
	{
        printf("PostgreSQL connection FAILED on [%s], port [%s], database [%s], user [%s], pwd [%s]. Aborting...\n",
            ip_address, port, database, user, pwd );
        return 1;
    }
	printf("PostgreSQL connection test finished.\n");

    if ( net_init() ) {
        return 1;
    }

#ifndef _MSC_VER
    openlog("fgtracker-server",LOG_PID,LOG_LOCAL1);
#endif

	daemon_proc = 0;
	
	if (run_as_daemon)
		daemon_proc = daemon_init();

	listenfd=Socket(AF_INET,SOCK_STREAM,0);

	bzero(&serveraddr,sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	if (server_addr) {
        serveraddr.sin_addr.s_addr = inet_addr(server_addr);
        if (serveraddr.sin_addr.s_addr == INADDR_NONE) {
            struct hostent *hp = gethostbyname(server_addr);
            if (hp == NULL) {
                sprintf(debugstr,"FlightGear tracker unable to resolve address %s, reverting to INADDR_ANY!",server_addr);
                debug(1,debugstr);
                serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
                server_addr = 0;
            } else {
                memcpy((char *)&serveraddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
            }
        }
	} else {
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

	serveraddr.sin_port = htons(server_port);

	Bind(listenfd,(SA *)&serveraddr,sizeof(serveraddr));

    if (server_addr)
        sprintf(debugstr,"FlightGear tracker started listening on %s, port %d!",server_addr,server_port);
    else
        sprintf(debugstr,"FlightGear tracker started listening on INADDR_ANY, port %d!",server_port);
	debug(1,debugstr);
	
	Listen(listenfd,SERVER_LISTENQ);

#ifndef _MSC_VER
	/*Installing signal handler*/
	signal(SIGCHLD, sigchld_handler);
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

	debug(1,"FlightGear tracker initialized. Waiting connection...");
	for ( ; ; )
	{
		clientaddrlen=sizeof(clientaddr);

		connfd=Accept(listenfd,(SA *) &clientaddr, &clientaddrlen);

#if defined(_MSC_VER) || defined(USE_PTHREAD)
        // use a thread
        if (pthread_create( &thread, NULL, func_child, (void*)0 )) {
        	debug(1,"ERROR: Failed to create child thread!");
        }
#else // !_MSC_VER
		if ( (pid = Fork()) == 0 )
		{	/* child */
			Close(listenfd);

			childpid=getpid();

			sprintf(debugstr,"CLIENT[%5d] started",childpid);
			debug(2,debugstr);
			doit(connfd);
			Close(connfd);
			sprintf(debugstr,"CLIENT[%5d] stopped",childpid);
			debug(2,debugstr);
			exit(0);
		}
		Close(connfd);
#endif // _MSC_VER or USE_PTHREAD y/n
	}
#ifndef _MSC_VER
	closelog();
#endif // !_MSC_VER
}

/* eof - server.c */
