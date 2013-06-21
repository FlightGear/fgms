#if not defined CLI_DEBUG_H
#define CLI_DEBUG_H

#define CLI_TRACE __FUNCTION__,__FILE__,__LINE__

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
