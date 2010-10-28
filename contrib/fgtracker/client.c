/*
 * client.c - a test client
 *
 *   Author: Gabor Toth <tgbp@freemail.hu>
 *   License: GPL
 *
 *   $Log: client.c,v $
 *   Revision 1.2  2006/05/10 21:22:34  koversrac
 *   Comment with author and license has been added.
 *
 */

#include "common.h"
#include "wrappers.h"
#include "error.h"

int main (int argc, char **argv)
{
	char msg[MAXLINE];
	int len;

	int sockfd;
	struct sockaddr_in serveraddr;

	sockfd=Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&serveraddr,sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVER_PORT);
	
 	inet_pton(AF_INET, SERVER_ADDRESS, &serveraddr.sin_addr);

	Connect(sockfd, (SA *) &serveraddr, sizeof(serveraddr));

	bzero(msg,MAXLINE);

	while ( strncmp(msg,"QUIT\n",MAXLINE)!=0 )
	{
		fgets(msg,MAXLINE,stdin);
		len=strlen(msg);
		if (len>0)
		{
			msg[len]='\0';
			write(sockfd,msg,len);
		}
  	}

	Close(sockfd);
	exit (0);
}

