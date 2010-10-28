/*
 * wrappers.c - wrapper functions for various system calls
 *
 *   Author: Gabor Toth <tgbp@freemail.hu>
 *   License: GPL
 *
 *   $Log: wrappers.c,v $
 *   Revision 1.2  2006/05/10 21:22:34  koversrac
 *   Comment with author and license has been added.
 *
 */

#include "wrappers.h"


int
Accept(int sockfd,struct sockaddr *cliaddr, socklen_t *addrlen) 
{ 
    int     n; 
    if ( (n = accept(sockfd,cliaddr,addrlen)) < 0) 
        err_sys("accept error"); 
    return (n); 
} 

int
Bind(int sockfd,const struct sockaddr *serveraddr, socklen_t addrlen) 
{ 
    int     n; 
    if ( (n = bind(sockfd,serveraddr,addrlen)) < 0) 
        err_sys("bind error"); 
    return (n); 
} 

int
Close(int sockfd)
{
    int     n; 
    if ( (n = close(sockfd)) < 0) 
        err_sys("close error"); 
    return (n); 
}

int
Connect(int sockfd, const struct sockaddr *serveraddr, socklen_t addrlen) 
{ 
    int     n; 
    if ( (n = connect(sockfd,serveraddr,addrlen)) < 0) 
        err_sys("connect error"); 
    return (n); 
} 

int
Fork()
{
    int    n;
    if ( (n = fork()) <0 )
	err_sys("fork error");
    return (n);
}

int
Listen(int sockfd,int backlog)
{ 
    int     n; 
    if ( (n = listen(sockfd,backlog)) < 0) 
        err_sys("listen error"); 
    return (n); 
} 

int
Socket(int family, int type, int protocol) 
{ 
    int     n; 
    if ( (n = socket(family, type, protocol)) < 0) 
        err_sys("socket error"); 
    return (n); 
} 


