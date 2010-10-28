/*
 * server.c - the tracker server itself
 *
 *   Author: Gabor Toth <tgbp@freemail.hu>
 *   License: GPL
 *
 *   $Log: server.c,v $
 *   Revision 1.2  2006/05/10 21:22:34  koversrac
 *   Comment with author and license has been added.
 *
 */

#include "common.h"
#include "wrappers.h"
#include "error.h"
#include <libpq-fe.h>

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

	chdir("/");		/* change working directory */

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

void ConnectDB(PGconn **conn)
{
  char debugstr[MAXLINE];
  *conn=PQsetdbLogin("62.112.194.20", "5432", "", "", "FlightGear-log", "tgbp", "password");

  if (PQstatus(*conn) != CONNECTION_OK)
  {
    sprintf(debugstr, "Connection to database failed: %s",PQerrorMessage(*conn));
    debug(1,debugstr);
    PQfinish(*conn);
  }
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
			write(fd,"PONG",5);
		}

		if (strncmp("CONNECT",event,7)==0)
		{
       			sscanf(msg,"%s %s %s %s %s %s",event,callsign,passwd,model,time1,time2);
       			sprintf(time,"%s %s Z",time1,time2);
       			if (strncmp("mpdummy",callsign,7)!=0 && strncmp("obscam",callsign,6)!=0) logFlight(conn,callsign,model,time,1);
			if (reply)
			{
				write(fd,"OK",2);
				//write(stdout,"OK",2);
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
				write(fd,"OK",2);
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
				write(fd,"OK",2);
				debug(3,"reply sent");
			}
		}
	}
  }
}

int main (int argc, char **argv)
{
	pid_t pid,childpid;
	int listenfd, connfd;
	struct sigaction sig_child;
	char debugstr[MAXLINE];

	struct sockaddr_in serveraddr,clientaddr;
	char buff[MAXLINE];
	time_t ticks;
	socklen_t clientaddrlen;

	openlog("fgtracker-server",LOG_PID,LOG_LOCAL1);
	daemon_proc=daemon_init();

	debug(1,"FlightGear tracker started!");
	
	listenfd=Socket(AF_INET,SOCK_STREAM,0);

	bzero(&serveraddr,sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);

	Bind(listenfd,(SA *)&serveraddr,sizeof(serveraddr));

	Listen(listenfd,SERVER_LISTENQ);


  	sig_child.sa_handler = sigchld_handler;
  	sigemptyset(&sig_child.sa_mask);
  	sig_child.sa_flags = SA_RESTART;
  	if (sigaction(SIGCHLD, &sig_child, NULL) < 0 ) 
	{
		err_sys("sigaction error");
      		exit(1);
	}
	
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

