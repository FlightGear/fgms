 /**
 * \page fgms fgms
 * 
 * \section fgms_cmd fgms command line
 * To show help run
 * \code 
 * fgms --help
 * \endcode
 * 
 * \subsection command_line_options Options
 * \code
 * -h            print this help screen
 * -a PORT       listen to PORT for telnet
 * -c config     read 'config' as configuration file
 * -p PORT       listen to PORT
 * -t TTL        Time a client is active while not sending packets
 * -o OOR        nautical miles two players must be apart to be out of reach
 * -l LOGFILE    Log to LOGFILE
 * -v LEVEL      verbosity (loglevel) in range 1 (few) and 5 (much)
 * -d            do _not_ run as a daemon (stay in foreground)
 * -D            do run as a daemon
 * \endcode
 * 
 * @see \ref fgms_conf, \ref ParseParams
 * 
 * \section fgms_infoo Other Info
 * 
 * 
 * \subsection bugs Bugs
 * At present fgms does not have its own bug tracker, instead were using:
 * * Using the \ref fg_mailing_list 
 * * and the \ref FlightGear bug tracker
 *   - Visit > <a href="https://code.google.com/p/flightgear-bugs/issues/list?q=label:fgms">code.google.com/p/flightgear-bugs</a>
 *   - use the <b>fgms</b> tag
 * 
 * \subsection support Support
 * - <b>IRC</b>
 *    - <b>#fgms</b> on <b>irc.flightgear.org</b>
 *    - Link > <a href="irc://irc.flightgear.org#fgms">irc.flightgear.org#fgms</a>
 *    - See \ref developers for more detail and IRC nicks
 * 
 * 
 * 
 * \subsection ExtermalLinks Links
 * 
 * - \b Source:  <a href="https://git.code.sf.net/p/fgms">https://git.code.sf.net/p/fgms</a>
 * - \b Home:    <a href="http://fgms.freeflightsim.org/">fgms.freeflightsim.org</a>
 * - \b Wiki:   <a href="http://wiki.flightgear.org/Fgms">wiki.flightgear.org/Fgms</a>
 */