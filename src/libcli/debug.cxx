#include <iostream>
#include <string>
#include "debug.hxx"

using namespace std;

//  #define DEBUG_TRACE

std::string space ( int n )
{
	std::string r;
	for (int i=0; i<n; i++)
		r += " ";
	return (r);
}

int DEBUG::depth = 0;

DEBUG::DEBUG ( const char* function, const char* filename, const int line )
{
	#ifdef DEBUG_TRACE
	this->filename = filename;
	this->function = function;
	this->line = line;
	cerr << space(depth) << filename << ":" << function << ":" << line << " start" << endl;
	depth += 2;
	#endif
}

void DEBUG::trace ( const char* function, const char* filename, const int line )
{
	#ifdef DEBUG_TRACE
	cerr << space(depth) << "# TRACE: "  << filename << ":" << function << ":" << line << endl;
	#endif
}

DEBUG::~DEBUG ()
{
	#ifdef DEBUG_TRACE
	depth -= 2;
	cerr << space(depth) << filename << ":" << function << ":" << line << " end" << endl;
	#endif
}
