#include <fglib/fg_log.hxx>

const char* LOGFILE = "example.log";

int
main
(
	int   argc,
	char* argv[]
)
{
	logger.log ( log::URGENT ) << "Output to console" << log::endl;
	logger << "+ Output to console" << log::endl;
	LOG ( log::DEBUG, "NO Output to console" );
	logger.open ( LOGFILE );
	LOG ( log::HIGH, "this should go into the logfile" );

	fgmp::StrList*	buf = logger.logbuf();
	fgmp::StrIt	it;
	std::cout << "buffer is:" << std::endl;
	for ( it = buf->begin(); it != buf->end(); it++ )
	{
		std::cout << *it;
	}
	std::cout << "end" << std::endl;
}
