/*! \page hosting Hosting
 *
 * \section pre_req Pre-Requisites
 * - <b>A computer running a variant of *nix to compile and host the server</b> 
 *   * Speed of the machine isn't of great consequence as the protocol is a multiplexer which doesn't require much processing grunt.
 * - <b>Direct/physical or remote access to the server i.e. SSH/telnet</b> 
 *  * a conventional web hosting package will usually not be sufficient!) 
 *  * suitable hosting packages are: 
 * 		* dedicated root servers
 * 		* virtual private servers
 * 		* shell servers 
 *  * for a collection of free shell account providers that may be suitable for fgms, 
 *     * see Free Shell Providers http://wiki.flightgear.org/images/cache/d/d0/Free_Shell_Providers.html
 * - If the server is meant to be a public internet server: 
 *    * an <b>internet connection</b>
 *    * <b>sufficient upstream/downstream bandwidth</b> (see \ref bandwidth_req).
 * - <b>Firewall policies</b> will need to be set up to allow 
 *   * for incoming and outgoing UDP traffic for the corresponding ports
 *   * the same applies to the administration port (TCP)
 * - Permission to run unattended background processes 
 *   * (this may only be an issue with certain limited hosting packages)
 * - About <b>5-10 MB of hard disk space</b> (mainly required for building the binary)
 * - A working <b>GNU toolchain</b> including 
 *  * gcc (compiler), gmake & ranlib (to compile the source code)
 * - <b>git</b> to aqquire the fgms source code, 
 
 * 
 * \section traffic Traffic & Bandwidth Considerations
 * \note
 * 		Currently (May 2008), the server code basically boils down to a packet multiplexer 
 * 		(i.e. most data is -unconditionally- transmitted to all 'active' clients), which may thus 
 * 		create considerable amounts of traffic; this ought to be taken into consideration due 
 * 		to bandwidth limitations in most hosting packages (i.e. fgms may create considerable amounts of traffic!). 
 * 
 * * For basic calculations: 
 * 		* inbound traffic is 25 Kilobit per second 
 * 		* outbound is 25 Kbits per second for each plane/client that can 'see' another. (see \ref server_out_of_reach)
 * 		* By default, assuming a 10hz update interval, each active \ref fgfs client will currently 
 * 			cause I/O traffic of approximately 5 kbytes/sec with one update consuming ~500 bytes.
 * 
 * 
 * As client updates have to be propagated to all other active clients by the server, 
 * this number has to be multiplied by the number of active clients -1 
 * (i.e. clients within a configurable range, minus the originating client).
 * In addition, the fgms system allows for traffic updates to be sort of 
 * 'mirrored' on (relayed to) other configurable multiplayer/relay servers.
 * 
 * This feature makes it possible for users/clients to use an arbitrary server 
 * (with acceptable latency), but still see clients/vehicles connected to different servers.
 * Thus, such relay servers may additionally increase inbound/outbound traffic 
 * considerably as all traffic is mirrored/propagated to such relays.
 * 
 * Having more servers should actually decrease the load on each server in high load situations, 
 * i.e. when most of the clients are within range of each other. See this brief note on the 
 * theoretical bandwidth behaviour of fgms.
 * 
 * 
 * 
 * \section reduce_band Reducing bandwidth requirements
 * Total network traffic is mainly determined by the number of active clients that 
 * 'see eachother' and configured mirrors. If traffic considerations are relevant, 
 * the following options exist to reduce overall server/network load:
 * - Configure a relatively low \ref server_out_of_reach value, so that clients 
 *   outside a certain range are not provided with updates (usually about 100 nm on the main server network)
 * - For virtual gatherings (i.e. fly-ins), have clients use airports and places that do not 
 *   have lots of other traffic (i.e. in other words, avoid places like standard airports such as KSFO)
 * - Avoid the use of unnecessary relay servers
 * - If you are not interested in your server being publicly available, 
 *   only share its address with your friends privately
 * - For local testing/development purposes, you may want to run only a private fgms session, 
 *   that may be specific to your LAN or even computer.
 * 
 */