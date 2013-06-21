#ifndef CLI_COMMON_H
#define CLI_COMMON_H

#ifdef __GNUC__
#       define UNUSED(d) d __attribute__ ((unused))
#else
#       define UNUSED(d) d
#endif

#include <string.h>
#include <stdlib.h>
#include <exception>
#include <iostream>
#include "debug.hxx"

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember)) 
#define free_z(p) do { if (p) { free (p); (p) = 0; } } while (0)

namespace LIBCLI
{

enum RETURN_CODES
{
	OK              = 0,
	ERROR           = -1,
	QUIT            = -2,
	ERROR_ARG       = -3
};

enum PRIVLEVEL
{
	UNPRIVILEGED    = 0,
	PRIVILEGED      = 15
};

enum MODE
{
	MODE_ANY        = -1,
	MODE_EXEC       = 0,
	MODE_CONFIG     = 1
};

enum PRINT_MODE
{
	PRINT_PLAIN     = 0x00,
	PRINT_FILTERED  = 0x01,
	PRINT_BUFFERED  = 0x02
};

enum MATCH_MODE
{
	MATCH_REGEX     = 1,
	MATCH_INVERT    = 2
};

enum CLI_STATES
{
	STATE_LOGIN,
	STATE_PASSWORD,
	STATE_NORMAL,
	STATE_ENABLE_PASSWORD,
	STATE_ENABLE
};

const int MAX_HISTORY   = 256;

class arg_error : public std::exception
{
public:
	arg_error ( const char* r ) { reason = r; };
	virtual const char* what() const throw()
	{
		return reason;
	}
	const char*	reason;
};

class mem_error : public std::exception
{
public:
	virtual const char* what() const throw()
	{
		return "could not allocate memory";
	}
};

}; // namespace LIBCLI

#endif
