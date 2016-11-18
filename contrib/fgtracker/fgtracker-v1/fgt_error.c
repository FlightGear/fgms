/*
 * fgt_error.c - error handling functions
 *
 *   Author: Gabor Toth <tgbp@freemail.hu>
 *   License: GPL
 *
 *   $Log: error.c,v $
 *   Revision 1.2  2006/05/10 21:22:34  koversrac
 *   Comment with author and license has been added.
 *
 */

#include    "fgt_common.h"
#include    "fgt_error.h"
#include    <stdarg.h>          /* ANSI C header file */ 
#ifdef _MSC_VER
#define LOG_INFO 1
#define LOG_ERR  2
#define snprintf _snprintf
#else // !_MSC_VER
#include    <syslog.h>          /* for syslog() */ 
#endif // _MSC_VER y/n

int     daemon_proc = 0;        /* set nonzero by server daemon_init() */ 

void pgm_exit(int val)
{
#if defined(_MSC_VER) && !defined(NDEBUG)
    int c;
    printf("Enter a key to exit...: ");
    c = getchar();
#endif // _MSC_VER an !NDEBUG
    exit(val);
}

#ifdef _MSC_VER
int syslog(int lev, char *fmt, ...) 
{
    char    buf[MAXLINE + 1]; 
    va_list ap; 
    va_start(ap, fmt); 
#ifdef HAVE_VSNPRINTF 
    vsnprintf(buf, MAXLINE, fmt, ap);   * safe */ 
#else 
    vsprintf(buf, fmt, ap);     /* not safe */ 
#endif 
    strcat(buf, "\n"); 
    fflush(stdout);         /* in case stdout and stderr are the same */ 
    fputs(buf, stderr); 
    fflush(stderr); 
    va_end(ap); 
    return lev;
}

// get a message from the system for this error value
char *get_message_text( int err )
{
    LPSTR ptr = 0;
    DWORD fm = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&ptr,
        0,
        NULL );
    if (ptr) {
        size_t len = strlen(ptr);
        while(len--) {
            if (ptr[len] > ' ')
                break;
            ptr[len] = 0;
        }
        if (len)
            return ptr;
        LocalFree(ptr);
    }
    return NULL;
}
#endif // _MSC_VER


/* Nonfatal error related to system call 
 * Print message and return */ 
void 
err_ret(const char *fmt, ...) 
{ 
    va_list ap; 
    va_start(ap, fmt); 
    err_doit(1, LOG_INFO, fmt, ap); 
    va_end(ap); 
    return; 
} 


/* Fatal error related to system call 
 * Print message and terminate */ 
void 
err_sys(const char *fmt, ...) 
{ 
    va_list ap; 
    va_start(ap, fmt); 
    err_doit(1, LOG_ERR, fmt, ap); 
    va_end(ap); 
    pgm_exit(1); 
} 


/* Fatal error related to system call 
 * Print message, dump core, and terminate */ 
void 
err_dump(const char *fmt, ...) 
{ 
    va_list ap; 
    va_start(ap, fmt); 
    err_doit(1, LOG_ERR, fmt, ap); 
    va_end(ap); 
    abort();                    /* dump core and terminate */ 
    exit(1);                    /* shouldn't get here */ 
} 


/* Nonfatal error unrelated to system call 
 * Print message and return */ 
void 
err_msg(const char *fmt, ...) 
{ 
    va_list ap; 
    va_start(ap, fmt); 
    err_doit(0, LOG_INFO, fmt, ap); 
    va_end(ap); 
    return; 
} 


/* Fatal error unrelated to system call 
 * Print message and terminate */ 
void 
err_quit(const char *fmt, ...) 
{ 
    va_list ap; 
    va_start(ap, fmt); 
    err_doit(0, LOG_ERR, fmt, ap); 
    va_end(ap); 
    pgm_exit(1); 
} 


/* Print message and return to caller 
 * Caller specifies "errnoflag" and "level" */ 
void 
err_doit(int errnoflag, int level, const char *fmt, va_list ap) 
{ 
    int     errno_save, n; 
    char    buf[MAXLINE + 1]; 
    errno_save = errno;         /* value caller might want printed */ 
#ifdef HAVE_VSNPRINTF 
    vsnprintf(buf, MAXLINE, fmt, ap);   * safe */ 
#else 
    vsprintf(buf, fmt, ap);     /* not safe */ 
#endif 
    n = strlen(buf); 
    if (errnoflag) {
#ifdef _MSC_VER
        // most likely came from a socket function error
        int err = WSAGetLastError();
        if (err) {  // got a socket error
            char *ptr = get_message_text(err);
            if (ptr) {
                snprintf(buf + n, MAXLINE - n, ": %s", ptr); 
                LocalFree(ptr);
            } else {
                snprintf(buf + n, MAXLINE - n, ": Got socket err %d", err); 
            }
        } else if (errno_save) {
            snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save)); 
        }
#else // _MSC_VER
        snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save)); 
#endif // _MSC_VER y/n
    }
    strcat(buf, "\n"); 
    if (daemon_proc) { 
        syslog(level, "%s", buf); 
    } else { 
        fflush(stdout);         /* in case stdout and stderr are the same */ 
        fputs(buf, stderr); 
        fflush(stderr); 
    } 
    return; 
} 

void
debug(int level,char *str)
{
	char buf[MAXLINE+1];
	if (level<=DEBUG_LEVEL)
	{
		//printf("%s\n",str);
		snprintf(buf,MAXLINE,"%s",str);
		if (daemon_proc)
	       	syslog(level, "%s", buf); 
	   else
				printf("%s\n",str);
	   
	}
}

/* eof - fgt_error.c */

