
/*! \mainpage FlightGear MultiPlayer Server 0.x Production/Stable 
 *
 * \b FGMS is a standalone network server for the <a href="../../flightgear/html">FlightGear</a> flight simulator and licenced under the GPL. 
 * With \b FGMS you can fly with other pilots scattered around the world. So you can assemble experiences in
 *  formation flight and/or build virtual airlines etc.
 * 
 * Note that you don't need to setup your own server in order to participate to flightgears online world. 
 * Just use one of the available public servers.
 * 
 * If you intend to set up your own server, you should use this \b fgms-0 stable series
 *
 * Users interested in \b fgms development can either help to improve the stable version 
 * or help to take the <a href="../../fgms-1/html"><b>fgms-1.x Developer Version</b></a> a step ahead. 
 *
 * 
 * 
 * \section Guides Server Guides
 * 
 *  - \ref install_guide
 *  - \ref ServerOperation
 *  - \ref command_line
 *  - \ref ServerModes
 *  - \ref fgms_conf
 * 
 * \section Essential Essential Docs
 * 
 *  - \ref README_cmake
 *  - \ref INSTALL
 *  - \ref COPYING
 *  - \ref AUTHORS
 *  - \ref fgms_example_conf
 *
 * \section ExtermalLinks Links
 * 
 * - \b Source:  <a href="https://gitorious.org/fgms/fgms-0-x">gitorious.org/fgms/fgms-0-x</a>
 * 
 * - \b Wiki:   <a href="http://wiki.flightgear.org/Fgms">wiki.flightgear.org/Fgms</a>
 * 
 * - \b Home:    <a href="http://fgms.sourceforge.net/index.php">fgms.sourceforge.net</a>
 * 
 * - <b>Chat & Help</b>
 *    - irc.flightgear.org
 *    - /join  \#fgms_example
 * 
 * 
 * \section LinkInfo Info
 * 
 *    @author Oliver Schroeder and Others (see \ref AUTHORS)
 *    @author  Docs Maintainer - pete at freeflightsim.org
 * 
 *  \b License:
 *    -   GNU GENERAL PUBLIC LICENSE
 * 
 * 
 */



/////////////////////////////////////////////////////////////////////////////////////
/*! 
 * 
 * \page INSTALL INSTALL
 *  \include ./INSTALL
 * 
 * \page README_cmake README.cmake
 *  \include ./README.cmake
 * 
 * \page AUTHORS AUTHORS
 *  \include ./AUTHORS
 * 
 * \page COPYING COPYING
 *  \include ./COPYING
 * 
 * \page fgms_example_conf fgms_example.conf
 *  \include ./src/server/fgms_example.conf
 */





/*! \page ServerOperation Server Operation
 *
 * \section Foreword Foreword
 * Over the last years the hosting of public flightgear servers has turned into a task which 
 * sometimes demands skills and time effort by the server hoster. This guide helps 
 * people to set up servers. Feel free to ask if something is in doubt.
 * 
 * \section PublicServers Public Servers
 * - Public servers are meant to run permanently on a host, and let players connect to them.
 * 
 * \section HowTo How to operate a flightgear multiplayer server
 * First of all, you should edit the file \b fgms.conf 
 * (use <a href="fgms_example.conf.xml">fgms_example.conf</a> for the basis). 
 * 
 * 
 * 
 * \page ServerModes Server Modes
 * 
 * @see \ref fgms_conf
 * 
 * \section SecServerModes FGMS Modes
 * \subsection HUB HUB
 * - On the flightgear network we operate the servers in a \b HUB environment. 
 * - This means, that traffic from clients received at 'leave'-servers is 
 *   only relayed to a (one or more) 'HUB'-server(s). 
 * - The HUB then sends the data to all known relay-servers.
 * 
 * \image html FGMS-HUB.png "Above you can see the traffic flow in a HUB environment"
 * 
 * 
 * \subsection NxNMash NxN Mesh
 * 
 * - An alternative configuration is the 'NxN mesh' or 'fully connected mesh' network. 
 * - In this configuration a server receives data from a client and resends it to all other servers in the network. 
 * 
 * \image html FGMS-NxNGrid.png "Above you can see the traffic flow in a fully connected mesh"
 * 
 * The HUB-configuration takes some of the traffic load off leave nodes, as they just relay to one (or more) HUB servers. 
 */



