 /**
 * \page the_project The Project
 * 
 * \ref fgms is an UDP packet relay server that powers the \ref FlightGear \ref mp_network.
 *  - its written in c++ 
 *  - is licensed under the <a href="COPYING.html">GPL</a>
 *  - the current production version is the <b>0.x</b> series which is considered stable
 * 
 * \section fgms_overview Overview
 * 
 * \image html fgms.1.png 
 * 
 * A typical \ref fgms servver running will: 
 * * Listen for UDP traffic on port 5000 which are either
 *    - An  \ref fgfs client with a pilot
 *    - Or other fgms servers being relayed to
 *    - Traffic from local pilots is forwarded to the other fgms servers ie \ref conf_relays.
 *    - Traffic from other pilots/relays is forwarded to local pilots, if within range.
 * * UDP packets are forwarded to the \ref conf_crossfeed port
 * * If the tracking is enabled (see \ref tracker_conf)
 *    - A tracker client is started as a a process thread. This forwards information via tcp
 *      to the tracker server
 *    - The tracker server can be either local, or at another location on the internet. It
 *      is a seperate application that has to be run. The server is in the contrib directory.
 * * Port 5001 is a tcp telnet port for getting status information
 * 
 * \section conf_telnet_port Telnet Port
 * The telnet port is a simple way to query the server and get some data including list of pilots. eg
 * \code 
 * > telnet mpserver14.flightgear.org 5000
 * \endcode
 * @see: \ref telnetport setting, FG_SERVER::SetTelnetPort, FG_SERVER::HandleTelnet
 * 
 *
 * 
 * \section the_team The Team
 * 
 * The code is form a variety of sources, with parts taken from \ref simgear, and other sources.
 * Please look at the source code of the file for more details if necessary
 * 
 * * <b>Oliver Schroder</b> 
 *   - Our <a href="http://en.wikipedia.org/wiki/Benevolent_Dictator_for_Life">BDFL</a>
 *   - Oliver is the main developer who looks after the code and the mainhub server
 *   - Based near ?
 *   - irc nick: os
 * * <b>Julien Pierru</b>
 *  - Created the UpdateTracker()
 *  - Based near ?
 * * <b>Anders Gidenstam</b>
 *  - Created the lazy relay 
 *  - Based near ?
 * * <b>Geoff McLane</b>
 *  - Created the windows port, and also generally fixes bugs etc. 
 *  - Based near LFPG
 * * <b>Rob Dosogne</b>
 *  - Created the FreeBSD friendly port
 *  - Based near ?
 * * <b>Hazuki Amamiya</b>
 *  - Created the fg_tracker module
 *  - Based near ?
 *  - irc nick: Hazuki
 * * <b>Pete Morgan</b>
 *  - A ligger who managed to get on this list as docs mainteiner
 *  - Based near EGFF
 *  - irc nick: petefffs
 * 
 * \section the_code Source Code
 * * The source for the project is versioned using git
 * * The repository is hosted at gitorious.org
 * 	- https://gitorious.org/fgms/fgms-0-x
 * 
 * To checkout the source code run
 * \code 
 * 		git clone git://gitorious.org/fgms/fgms-0-x.git 
 * \endcode
 * 
 
 */