#include <fglib/fg_log.hxx>

const char* LOGFILE = "example.log";

int
main
(
	int   argc,
	char* argv[]
)
{
	using fgmp::log_prio;

	logger.log ( log_prio::URGENT ) << "Output to console" << fgmp::endl;
	logger << "+ Output to console" << fgmp::endl;
	LOG ( log_prio::DEBUG, "NO Output to console" );
	logger.open ( LOGFILE );
	LOG ( log_prio::HIGH, "this should go into the logfile" );

	fgmp::str_list*	buf = logger.logbuf();
	std::cout << "buffer is:" << std::endl;
	for ( auto it : buf )
	{
		std::cout << it;
	}
	std::cout << "end" << std::endl;
}
