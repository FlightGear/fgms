 /**
 * \page mp_network MP Network
 * 
 * The \ref FlightGear MultiPlayer network of around twenty and counting (circa 2013) server. 
 * * These servers are public internet facing.
 * * Public servers are meant to run permanently on a host, and let players connect to them for free.
 * 
 * \section dns DNS
 * - The mpservers are subdomains of <b>flightgear.org</b> 
 *   * eg: mpserver14.flightgear.org
 * - The DNS entries are maintained by the flightgear-developers
 * - There is no automated or "official" list at present, but the latest servers are expected to be on the FlightGear wiki page 
 *   * http://wiki.flightgear.org/Howto:Multiplayer
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
 * \section hosting_info Hosting and Mp Server
 * If your are interested in hosting a server and joining the network then please read the following guidelines. 
 * However its assumed you are familiar with *nix style systems and can compile code.
 * 
 * * Read the \ref pre_requisites 
 * 
 */