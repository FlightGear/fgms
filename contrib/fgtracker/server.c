/*
 * server.c - the tracker server itself
 *
 *   Author: Gabor Toth <tgbp@freemail.hu>
 *   License: GPL
 *
 *   $Log: server.c,v $
 *   Revision 1.2  2006/05/10 21:22:34  koversrac
 *   Comment with author and license has been added.
 *   Revision 1.3 2012/07/04 geoff (reports _at_ geoffair _dot_ info)
 *   Add user configuration and help
 *
 */

#include "common.h"
#include "wrappers.h"
#include "error.h"
#include <libpq-fe.h>

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
static char *pgoptions = (char *)"";
static char *pgtty = (char *)"";

int
daemon_init(void)
{
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

	return(0);
}

void sigchld_handler(int s)
{
    pid_t childpid;
    char debugstr[MAXLINE];

    while( (childpid=waitpid(-1, NULL, WNOHANG)) > 0)
    {
	sprintf(debugstr,"Child stopped: %d",childpid);
	debug(2,debugstr);
    }
}

void sighup_handler(int s)
{
    exit(0);
}

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

    *conn = PQsetdbLogin(ip_address, port, pgoptions, pgtty, database, user, pwd);

	if (PQstatus(*conn) != CONNECTION_OK)
   {
   	sprintf(debugstr, "Connection to database failed: %s",PQerrorMessage(*conn));
    	debug(1,debugstr);
    	PQfinish(*conn);
    	iret = 1;
  	}
  	return iret;
}

int logFlight(PGconn *conn,char *callsign,char *model, char *date, int connect)
{
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
    sprintf(debugstr, "Command failed: %s", PQerrorMessage(conn));
    debug(1,debugstr);
    PQclear(res);
  }

  if (connect)
  {
    sprintf(statement,"INSERT INTO flights (callsign,status,model,start_time) VALUES ('%s','OPEN','%s','%s');",callsign2,model2,date2);
    res = PQexec(conn, statement);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
      sprintf(debugstr, "Command failed: %s", PQerrorMessage(conn));
      debug(1,debugstr);
      PQclear(res);
      return(1);
    }
  }

  return(0);
  
}

int logPosition(PGconn *conn, char *callsign, char *date, char *lon, char *lat, char *alt)
{
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
    sprintf(debugstr, "Command failed: %s", PQerrorMessage(conn));
    debug(1,debugstr);
    PQclear(res);
  }

  return(0);

}

void doit(int fd)
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

  sprintf(debugstr,"connection from %s, port %d", inet_ntop(AF_INET, &clientaddr.sin_addr, msg, sizeof(msg)), ntohs(clientaddr.sin_port));
  debug(2,debugstr);

  while ( (len = read(fd, msg, MAXLINE)) >0)
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
		pg_reconn_counter=0;

		if (strncmp("REPLY",event,5)==0)
		{
			
			reply=1;
		}

		if (strncmp("PING",event,4)==0)
		{
			if (write(fd,"PONG",5) != 5)
                debug(1,"write PONG failed");
		}

		if (strncmp("CONNECT",event,7)==0)
		{
       			sscanf(msg,"%s %s %s %s %s %s",event,callsign,passwd,model,time1,time2);
       			sprintf(time,"%s %s Z",time1,time2);
       			if (strncmp("mpdummy",callsign,7)!=0 && strncmp("obscam",callsign,6)!=0) logFlight(conn,callsign,model,time,1);
			if (reply)
			{
				if (write(fd,"OK",2) != 2)
                    debug(1,"write OK1 failed");
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
				if (write(fd,"OK",2) != 2)
                    debug(1,"write OK2 failed");
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
				if (write(fd,"OK",2) != 2)
                    debug(1,"write OK2 failed");
                else
                    debug(3,"reply sent");
			}
		}
	}
  }
}

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
    printf("PostgreSQL Database Information\n");
    printf(" --db database (-d) = Set the database name. (def=%s)\n",database);
    printf(" --ip addr     (-i) = Set the IP address of the postgresql server. (def=%s)\n",ip_address);
    printf(" --port val    (-p) = Set the port of the postgreql server. (def=%s)\n",port);
    printf(" --user name   (-u) = Set the user name. (def=%s)\n",user);
    printf(" --word pwd    (-w) = Set the password for the above user. (def=%s)\n",pwd);
    printf("fgms connection\n");
    printf(" --IP addr     (-I) = Set IP address to connect to fgms. (def=IPADDR_ANY)\n");
    printf(" --PORT val    (-P) = Set PORT address to connect to fgms. (dep=%d)\n", server_port);
    printf("General Options\n");
    printf(" --help    (-h. -?) = This help, and exit(0).\n");
    printf(" --version     (-v) = This help, and exit(0).\n");
    printf(" --DAEMON y|n  (-D) = Run as daemon yes or no. (def=%s).\n", (run_as_daemon ? "y" : "n"));
    printf(" %s will connect to an instance of 'fgms', receive and add flight and position\n",bn);
    printf(" messages to the PostgreSQL database, for later 'tracker' display.\n");
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
            (strcmp(arg,"-?") == 0) || (strcmp(arg,"--version") == 0)) {
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
    PGresult *res;
    char buff[MAXLINE];
    char *cp = buff;
    char *val;
    int i, j, i2, nFields, nRows;
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
                    i2 = i + 1;
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
}

int test_db_connection()
{
    int iret;
    PGconn *conn = NULL;
    if (ConnectDB(&conn)) {
        return 1;   /* FAILED - conn closed */
    }
    iret = check_tables(conn);
    PQfinish(conn);
    return iret;
}

int main (int argc, char **argv)
{
	pid_t pid,childpid;
	int listenfd, connfd;
	struct sigaction sig_child;
	char debugstr[MAXLINE];
	struct sockaddr_in serveraddr,clientaddr;
	socklen_t clientaddrlen;

    parse_commands(argc,argv);  /* parse user command line - no return if error */

    if (test_db_connection()) {
        printf("PostgreSQL connection FAILED on [%s], port [%s], database [%s], user [%s], pwd [%s]. Aborting...\n",
            ip_address, port, database, user, pwd );
        return 1;
    }

    openlog("fgtracker-server",LOG_PID,LOG_LOCAL1);

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


  	sig_child.sa_handler = sigchld_handler;
  	sigemptyset(&sig_child.sa_mask);
  	sig_child.sa_flags = SA_RESTART;
  	if (sigaction(SIGCHLD, &sig_child, NULL) < 0 ) 
	{
		err_sys("sigaction error");
      		exit(1);
	}
	
	debug(1,"FlightGear tracker, got connection...");
	for ( ; ; )
	{
		clientaddrlen=sizeof(clientaddr);

		connfd=Accept(listenfd,(SA *) &clientaddr, &clientaddrlen);

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
	}

	closelog();
}

/* eof - server.c */

