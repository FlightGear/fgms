
/*! \page install_guide Installation Guide
 *
 * This page is provides a short overview about installing the 
 * <b>FlightGear Multiplayer Server (fgms)</b>. The reader is assumed to be reasonably familiar with working in a Unix/Linux shell environment.
 * 
 * \section preq Pre-Requisites
 * - A computer running a variant of *nix to compile and host the server. Speed of the machine isn't of great consequence as the protocol is a multiplexer which doesn't require much processing grunt.
 * - Direct/physical or remote access to the server (i.e. SSH/telnet, a conventional web hosting package will usually not be sufficient!)-suitable hosting packages are: dedicated root servers, virtual private servers, shell servers - for a collection of free shell account providers that may be suitable for fgms, see Free Shell Providers (you may want to check this out if you are interested in running fgms but don't have hosting yet)
 * - If the server is meant to be a public internet server: an internet connection, featuring sufficient upstream/downstream capabilities (see below for details concerning bandwidth requirements).
 * - Firewall policies will need to be set up to allow for incoming and outgoing UDP traffic for the corresponding ports, the same applies to the administration port (TCP)
 * - Permission to run unattended background processes (this may only be an issue with certain limited hosting packages)
 * A working GNU toolchain including gcc (compiler), gmake & ranlib (to compile the source code)
 * \b fgms source code, 
 * About 5-10 MB of hard disk space (mainly required for building the binary)
 * 
 * \section traffic Traffic & Bandwidth Considerations
 * Note: Currently (May 2008), the server code basically boils down to a packet multiplexer 
 * (i.e. most data is -unconditionally- transmitted to all 'active' clients), which may thus 
 * create considerable amounts of traffic; this ought to be taken into consideration due 
 * to bandwidth limitations in most hosting packages 
 * (i.e. fgms may create considerable amounts of traffic!). For basic calculations: inbound traffic is 25
 * Kilobit per second while outbound is 25 Kbits per second for each plane/client that can 'see' another.
 * 
 * 
 * By default, assuming a 10hz update interval, each active fgfs client will currently 
 * cause I/O traffic of approximately 5 kbytes/sec (one update consuming ~500 bytes) [1].
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
 * - Configure a relatively low "server.out_of_reach" value, so that clients 
 *   outside a certain range are not provided with updates (usually about 100 nm on the main server network)
 * - For virtual gatherings (i.e. fly-ins), have clients use airports and places that do not 
 *   have lots of other traffic (i.e. in other words, avoid places like standard airports such as KSFO)
 * - Avoid the use of unnecessary relay servers
 * - If you are not interested in your server being publicly available, 
 *   only share its address with your friends privately
 * - For local testing/development purposes, you may want to run only a private fgms session, 
 *   that may be specific to your LAN or even computer.
 * 
 * \section get_bui Getting & Building fgms
 * 
 * - You will need the build tools cmake and make, and also g++ to compile the fgms application.
 *   These can usually be found in the package manager for most the common Linux distributions. 
 *   You will also need the git client to fetch the source code from the repository, if 
 *   you plan to download it from the command line interface (see below).
 * - Once the build tools are installed on your machine, create a directory to hold the source code. 
 *   This could be named anything, such as fgms. Create it in your user home directory. 
 *   \note Note that this WILL NOT be the directory where the program will be compiled.
 * - Now \b cd into the directory you just made, where you will run the command to 
 *   fetch the source code from the git repository (see below).
 * - To download the latest stable version via HTTP, you can use direct your browse to the URL
 *   http://gitorious.org/fgms/fgms-0-x/. 
 *   Download and unzip the source code, and place it in the directory you created above.
 * - To download a file directly to your server from an ssh session, 
 *   you can use git tools with the following command:
 *   \code 
 *   git clone git://gitorious.org/fgms/fgms-0-x.git
 *   \endcode
 * 
 * \section set_config Setting up the config file
 * 
 * At this point you should have a working \b fgms binary in the build-fgms directory. 
 * - Edit the \ref fgms_conf  file according to the instructions found in example.
 * @note  If the server will be offline or for private use (i.e. LAN-only, 
 *        please comment out the relay servers section. This will save bandwidth 
 *        from the server being consumed.
 *   
 * 
 * \section run_fgms Running FGMS for the first time
 * 
 * In addition to its configuration file, \b fgms supports a number of configuration parameters that can be
 * passed at startup (and that will by default override any configuration file settings), 
 * to get a summary of supported parameters, you may want to run:
 * \code 
 * ./fgms --help
 * \endcode
 * Which should present you with help output.
 * 
 * @see \ref command_line
 *  
 * \page command_line Command Line Options
 * 
 * To show help run
 * \code 
 * ./fgms --help
 * \endcode
 * 
 * \section options Options
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
 * @see \ref fgms_conf
 * 
 */


