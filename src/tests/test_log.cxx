#include <fglib/fg_log.hxx>

const char* LOGFILE = "example.log";

int
main
(
        int   argc,
        char* argv[]
)
{
        using namespace fgmp;

        logger.log ( fglog::prio::URGENT ) << "Output to console" << fglog::endl;
        logger << "+ Output to console" << fglog::endl;
        LOG ( fglog::prio::DEBUG, "NO Output to console" );
        logger.open ( LOGFILE );
        LOG ( fglog::prio::HIGH, "this should go into the logfile" );

        fgmp::str_list* buf = logger.logbuf();
        std::cout << "buffer is:" << std::endl;
        for ( auto it : buf )
        {
                std::cout << it;
        }
        std::cout << "end" << std::endl;
}
