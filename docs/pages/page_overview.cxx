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
 * * If configured, <b>all</b> UDP packets are forwarded to the \ref conf_crossfeed port
 * * If the tracking is enabled (see \ref tracker)
 *    - A \ref tracker_client is started as a a process thread. This forwards information via tcp
 *      to the \ref tracker_server
 *    - The \ref tracker_server can be either local, or at another location on the internet. It
 *      is a seperate application that has to be run.
 * * Port 5001 is a tcp \ref telnet_port for getting status information
 * \note 
 *       And important aspect is missing from the above.
 *        - Packets are only forwarded to players who are within range of each other in the simulator
 *        - eg A pilot physically in London and another in Tokyo are flying above Paris and see each other
 *        - This substantially reduces traffic, see \ref server_out_of_reach
 * 
 * 
 * 
 * 
 * \section paris_1 1: Between FGFS and FGMS - UDP:5000
 * 
 * - An \ref fgfs  sim session can choose to connect to ANY instance of FGMS on the \ref mp_network
 * - Below is one connection, in this case to a FGMS which has the \ref server_is_hub attribute true
 * - but can be to ANY instance of FGMS.
\code
	/----\                        /----\
	|    |  Flight Data - 10 Hz   |    |
	|    | >>>>>>>>>>>>>>>>>>>>>> |    |
	|FGFS| Nearby Flights - 10 Hz |FGMS|
	|    | <<<<<<<<<<<<<<<<<<<<<< |*HUB|
	|    |                        |    |
	\----/                        \----/
\endcode
 *
 * * These are raw packets of \ref xdr encoded binary data consisting of a 
 *   header, positions data, and various property tree values, including the CHAT messages.
 * * A typical packet is about 800 bytes long, with a maximum packet length (I think) of 1200 bytes.
 * * As recently indicated now that FGFS has extrapolation code to continue the aircraft
 *   movement even in the absence of position packets
 * 
 * \note Nominal 10 Hz is too frequent and 2 to 4 hz is sufficient
 *
 * @see FG_SERVER::SetHub(), \ref server_is_hub 
 * 
 * \section paris_2 2: Between two instances of FGMS - UDP:5000
 * 
 * Packets between FGMS and configured FGMS \ref conf_relays
 * - in this case to a RELAY that has the HUB attribute set.
 * - LOCAL means a FGFS instances connected to that particular FGMS.
\code
     /----\                        /----\
     |    |  LOCAL Flights - 10 Hz |    |
     |    | >>>>>>>>>>>>>>>>>>>>>> |    |
     |FGMS| Nearby Flights - 10 Hz |FGMS|
     |    | <<<<<<<<<<<<<<<<<<<<<< |*HUB|
     |    |  All Flights - 1 Hz    |    |
     \----/ <<<<<<<<<<<<<<<<<<<<<< \----/
             (Lazy Relay)
\endcode
 * If ALL other FGMS instances RELAY to the FGMS with HUB attribute, then they need
 * not RELAY to other instances, and will receive ALL Flight information 
 * at a slower rate, 1 Hz.
 * 
 * @see \ref conf_relays, FG_SERVER::AddRelay(), FG_SERVER::SendToRelays()
 * 
 * \section paris_3 3: Other connections supported by FGMS
 * 
 * 
 * \subsection paris_crossfeed (a) CROSSFEED Port - UDP:3333
 *
\code
     /----\                        /-----\
     |    |                        |     |
     |    |                        |     |
     |FGMS| All Flights - as recd  |CROSS|
     |    | >>>>>>>>>>>>>>>>>>>>>  |FEED |
     |    |                        |     |
     \----/                       -\-----/
\endcode
 * * Since CROSSFEED receives ALL raw packets as
 *   received by that instance of FGMS it has ALL
 *   Flight information, and can offer various
 *   forms of output of that data, filtered or unfiltered.
 * * One offered presently on a HTTP port is a
 *   json string reply. Since REMOTE packets only
 *   arrive at 1 Hz, this sting is only updated
 *   each second.
  * * CROSSFEED can also filter these packets for
 *   'movement' - positions, altitude, heading,
 *   speed change, and write the results to
 *   a PostgreSQL database. That is 'replace'
 *   the following 'tracker' port.
 * 
 * @see \ref conf_crossfeed, FG_SERVER::AddCrossfeed(), FG_SERVER::SendToCrossfeed()
 * 
 * \subsection paris_tracker (b) TRACKER Port - UDP:8000
 * 
 \code 
    ---------------------------

     /----\                        /-----\
     |    |                        |     |
     |    | LOCAL Flights as recd. |     |
     |FGMS| >>>>>>>>>>>>>>>>>>>>>> |TRACK|
     |    | PING/PONG - alive msg  |SERV.|
     |    | <<<<<<<<<<<<>>>>>>>>>> |     |
     \----/                       -\-----/
 \endcode 
 * Inside FGMS these messages are transferred
 * from the FG_SERVER module to the FG_TRACKER
 * module via an IPC.
 *
 * The unfiltered LOCAL Flight messages are
 * of three types -
 *   CONNECT    - When a new FGFS connects to FGMS
 *   POSITION   - Messages as the flight continues.
 *   DISCONNECT - Final message when flight expires.
 *
 *   The Tracker Server posts these (unfiltered)
 *   messages to a PostgreSQL database.
 *
 *   No protocol other than the PING/PONG - are
 *   you alive - messages. All data is in form of an
 *   ASCII string.
 *
 *   This requires each FGMS to actively connect
 *   to the tracking server.
 * 
 * @see \ref tracker, \ref conf_tracker, FG_SERVER::AddTracker()
 * 
 * 
 * \subsection paris_telnet (c) TELNET Port - TCP:5001
 * This control port is presently only used to
 * provide a list of current flights.
\code

     /----\                        /-----\
     |    |                        |     |
     |PC  | Flight List            |     |
     \----/ <<<<<<<<<<<<<<<<<<<<<< |FGMS |
     |....| (LOCAL and REMOTE)     |     |
     \----/                        |     |
                                   \-----/
\endcode
 * Provides a list of all flights, marked as
 * LOCAL or REMOTE (with server name).
 * It can be a heavy load on the FGMS instance
 * if this is done frequently. For each request
 * a fork() (or thread) process is started to
 * attend to the reply.
 * 
 * No protocol, other than the single shot
 * telnet connection. Data flows only from FGMS
 * to the Telnet establisher.
 * 
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