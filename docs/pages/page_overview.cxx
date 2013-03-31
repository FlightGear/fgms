 /**
 * \page overview Overview
 * 
 * \ref fgms is an UDP packet relay server that powers the \ref FlightGear \ref mp_network.
 *  - its written in c++ 
 *  - is licensed under the <a href="COPYING.html">GPL</a>
 *  - the current production version is the <b>0.x</b> series which is considered stable
 * 
 * 
 * \image html fgms.1.png 
 * 
 * A typical \ref fgms server running will: 
 * * Listen for UDP traffic on port 5000 which are either
 *    - An  \ref fgfs client with a pilot
 *    - Or other fgms servers in a relay relationship
 *    - Traffic from local pilots is forwarded to the other fgms servers ie \ref conf_relays.
 *    - Traffic from other pilots/relays is forwarded to local pilots, if within range.
 * * If configured, <b>all</a> UDP packets are forwarded to the \ref conf_crossfeed port
 * * If the tracking is enabled (see \ref tracker)
 *    - A \ref tracker_client is started as a a process thread. This forwards information via tcp
 *      to the \ref tracker_server
 *    - The \ref tracker_server can be either local, or at another location on the internet. It
 *      is a seperate application that has to be run.
 * * Port 5001 is a tcp \ref telnet_port for getting status information
 * 
 * \section telnet_port Telnet Port
 * The telnet port is a simple way to query the server and get some data including list of pilots. eg
 * \code 
 * > telnet mpserver14.flightgear.org 5001
 * \endcode
 * @see \ref telnetport setting, FG_SERVER::SetTelnetPort, FG_SERVER::HandleTelnet
 * 
 * 
 * \subsection next_dd Next >
 * *  The \ref mp_network
 * 
 */