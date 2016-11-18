/*
 * wrappers.h - headers for wrapper functions
 *
 *   Author: Gabor Toth <tgbp@freemail.hu>
 *   License: GPL
 *
 *   $Log: wrappers.h,v $
 *   Revision 1.2  2006/05/10 21:22:34  koversrac
 *   Comment with author and license has been added.
 *
 */

#ifndef __wrappers_h
#define __wrappers_h
#include "fgt_common.h"
#include "fgt_error.h"


int Accept(int sockfd,struct sockaddr *cliaddr, socklen_t *addrlen) ;

int Bind(int sockfd,const struct sockaddr *serveraddr, socklen_t addrlen) ;

int Close(int sockfd);

int Connect(int sockfd, const struct sockaddr *serveraddr, socklen_t addrlen) ;

int Fork();

int Listen(int sockfd,int backlog);

int Socket(int family, int type, int protocol) ;

#endif
