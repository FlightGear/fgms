 /**
 * \page mp_network MP Network
 * 
 * The \ref FlightGear MultiPlayer network consists of around 20++ (circa 2013) running servers. 
 * * These servers are public internet facing contributed by various parties.
 * * Public servers are meant to run permanently on a host, and let players connect to them for free
 * * Typically there are around 10 to 60 players on the network circa 2013.
 * 
 * The main Hub Server (\ref server_is_hub ) is currently <b>mpserver01</b>, hosted by \ref oliver
 * 
 * \section dns DNS
 * - The mpservers are subdomains of <b>flightgear.org</b> 
 *   * eg: mpserver14.flightgear.org
 * - The DNS entries are maintained by the flightgear-developers
 * - There is no automated or "official" list at present, but the latest servers are expected to be on the FlightGear wiki page 
 *   * http://wiki.flightgear.org/Howto:Multiplayer
 * 
 * \note To make any changes, or to be added to DNS contact the \ref FlightGear developers
 *      on the \ref fg_mailing_list
 * 
 * \section HUB Hub 
 * - On the FlightGear Mp network operates the servers in a \b HUB environment. 
 * - This means, that traffic from clients received at 'leave'-servers is 
 *   only relayed to a (one or more) 'HUB'-server(s). 
 * - The HUB then sends the data to all known relay servers.
 * - The HUB-configuration takes some of the traffic load off leave nodes, as they just relay to one (or more) HUB servers. 
 * - Currently mpserver01.flightgear.org is the only hub.
 * 
 * \image html FGMS-HUB.png "Above you can see the traffic flow in a HUB environment"
 * 
 * 
 * 
 * \subsection next_dd Next >
 * *  \ref hosting
 * 
 */