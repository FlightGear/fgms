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
 * * UDP packets are forwarded to the \ref conf_crossfeed port
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
 * > telnet mpserver14.flightgear.org 5000
 * \endcode
 * @see: \ref telnetport setting, FG_SERVER::SetTelnetPort, FG_SERVER::HandleTelnet
 * 
 *
 * 
 * 
 * \section the_code_intro Source Code
 * * The source for the project is versioned using git
 * * The repository is hosted at gitorious.org
 * 	- https://gitorious.org/fgms/fgms-0-x
 * 
 * To checkout the source code run
 * \code 
 * 		git clone git://gitorious.org/fgms/fgms-0-x.git 
 * \endcode
 * 
 * 
 * 
 * \section the_team The Team
 * 
 * The code is form a variety of sources, with parts taken from \ref simgear, and other sources.
 * Please look at the source code of the file for more details if necessary
 * 
 *  \subsection oliver Oliver
 *  * <b>Oliver Schroder</b> 
 *   - Our <a href="http://en.wikipedia.org/wiki/Benevolent_Dictator_for_Life">BDFL</a>
 *   - Oliver is the main developer who looks after the code and the mainhub server mpserver01
 *   - Based near ?
 *   - irc nick: os
 * 
 *  \subsection julien Julien
 * * <b>Julien Pierru</b>
 *  - Created the UpdateTracker()
 *  - Based near ?
 *
 *  \subsection anders Anders
 * * <b>Anders Gidenstam</b>
 *  - Created the lazy relay 
 *  - Based near ?
 * 
 *  \subsection geoff Geoff
 * * <b>Geoff McLane</b>
 *  - Created the windows port, and also generally fixes bugs etc. Host mpserver14
 *  - Based near LFPG
 * 
 *   \subsection rob Rob
 * * <b>Rob Dosogne</b>
 *  - Created the FreeBSD friendly port
 *  - Based near ?
 * 
 *   \subsection hazuki Hazuki
 * * <b>Hazuki Amamiya</b>
 *  - Created the FG_TRACKER module, and also hosts the \ref tracker.
 *  - Based near ?
 *  - irc nick: Hazuki
 * 
 *   \subsection pete Pete
 * * <b>Pete Morgan</b>
 *  - A ligger who managed to get on this list as docs maintainer
 *  - Based near EGFF
 *  - irc nick: petefffs
 */