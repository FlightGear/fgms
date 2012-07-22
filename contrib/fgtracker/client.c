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

#include "fgt_common.h"
#include "wrappers.h"
#include "fgt_error.h"

static char *server_address = (char *)SERVER_ADDRESS;
static uint32_t server_port = SERVER_PORT;

#ifdef _MSC_VER
#if !defined(NTDDI_VERSION) || !defined(NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA)   // if less than VISTA, provide alternative
#ifndef EAFNOSUPPORT
#define	EAFNOSUPPORT	97	/* not present in errno.h provided with VC */
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

int main (int argc, char **argv)
{
	char msg[MAXLINE];
	int len;
	char *s;
	int sockfd;
	struct sockaddr_in serveraddr;

	sockfd=Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&serveraddr,sizeof(serveraddr));
	printf("fgt_client connecting to %s, on port %u\n", server_address, server_port);
	
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(server_port);
	
 	inet_pton(AF_INET, server_address, &serveraddr.sin_addr);

	Connect(sockfd, (SA *) &serveraddr, sizeof(serveraddr));

	bzero(msg,MAXLINE);

	while ( strncmp(msg,"QUIT\n",MAXLINE) != 0 )
	{
		s = fgets(msg,MAXLINE,stdin);
		len = strlen(msg);
		if (len>0)
		{
			msg[len]='\0';
			if ( SWRITE(sockfd,msg,len) != len ) {
				printf("ERROR: Write socket FAILED!\n");
				break;	/* out of here */
			}
		}
  	}

	Close(sockfd);
	exit (0);
}

