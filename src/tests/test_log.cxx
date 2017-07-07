#include <fglib/fg_log.hxx>

const char* LOGFILE = "example.log";

using namespace fgmp;

int
main
(
	int   argc,
	char* argv[]
)
{
	logger.log ( fglog::URGENT ) << "Output to console" << fgmp::endl;
	logger << "+ Output to console" << fgmp::endl;
	LOG ( fglog::DEBUG, "NO Output to console" );
	logger.open ( LOGFILE );
	LOG ( fglog::HIGH, "this should go into the logfile" );

	fgmp::StrList*	buf = logger.logbuf();
	fgmp::StrIt	it;
	std::cout << "buffer is:" << std::endl;
	for ( it = buf->begin(); it != buf->end(); it++ )
	{
		std::cout << *it;
	}
	std::cout << "end" << std::endl;
}
