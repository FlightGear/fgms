#if not defined CLI_DEBUG_H
#define CLI_DEBUG_H

#define CLI_TRACE __FUNCTION__,__FILE__,__LINE__

//  #define DEBUG_OUTPUT
#ifdef DEBUG_OUTPUT
	define DEBUG_OUT(X) printf(X)
#else
	#define DEBUG_OUT(X)
#endif

class DEBUG
{
public:
	DEBUG ( const char* function, const char* filename, const int line );
	void trace ( const char* function, const char* filename, const int line );
	~DEBUG ();
private:
	static int depth;

	const char* filename;
	const char* function;
	int   line;
};

#endif
