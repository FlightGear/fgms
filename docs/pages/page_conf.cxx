/**
 * \page fgms_conf Configuration
 * 
 * \ref fgms is configured via a config file, see \ref fgms_example_conf
 * - by default on *nix systems the file is located at:
 *   - <b>/usr/local/etc/fgms.conf</b>
 * - Parts of the configuration can be ovverriden with the startup command line - see \ref fgms
 * 
 * TIP: Add a # to comment out a line in a config file
 * \code 
 * #foo = 5000  < this line has been commented out
 * foo = 5001
 * \endcode
 * 
 * @see \ref reduce_bandwidth - optimizing the configuration
 * @see FG_CONFIG class - used to load and query
 * 
 * \section conf_details Server Config
 * 
 * 
 *  \subsection servername server.name
 * \code 
 * server.name = albert
 * \endcode
 *  - This name is used in server chat messages and when queried via telnet
 * 
 * 
 * \subsection serveraddress server.address
 * \code 
 * # only listen on this address
 * server.address = 123.123.456.456
 * \endcode
 * - If you want to receive data over the internet and have only one IP configured on your server, 
 *  just comment out the line (prepend it with a '#' character)
 * - If you have several IPs configured, set this to the IP your server should listen and send data on.
 * - If you only want to handle local traffic, set this to <code>server.address = 127.0.0.1</code>
 * \note If you are part of the \ref mp_network then this IP must match the \ref dns entry.
 * \subsection serverport server.port
 * \code 
 * # listening port for FlightGear Sim client
 * server.port = 5000
 * \endcode
 * -  For public servers this should be \b 5000
 * 
 * \subsection server_daemon server.daemon
 * \code 
 * server.daemon = false
 * \endcode
 * - If set to \b true, the server will run in the background, otherwise foreground
 * 
 * \subsection server_is_hub server.is_hub
 * \code 
 * server.is_hub = false
 * \endcode
 * - If set to \b true, fgms will act as a \b hub server
 * - a HUB server will <b>resend packets received from relays to all other relays</b>.
 * @warning<b>Only set to true if you know what you are doing!!</b>
 * 
 * @see FG_SERVER::SetHub and \ref mp_network
 * 
* 
 * \subsection server_logfile server.logfile
 * \code 
 * server.logfile = fgms.log
 * \endcode
 * - Write logs to this file
 * 
 * 
 * \subsection telnetport server.telnet_port
 * \code 
 * server.telnet_port = 5001
 * \endcode
 * - port for telnet server
 * - set to 0 (zero) to disable telnet
 * - note however, for public servers this should be \b 5001
 * 
 * \section conf_player Player/Client Config
 * \subsection server_out_of_reach server.out_of_reach
 * \code 
 * server.out_of_reach = 100
 * \endcode
 * - Distance in nautical miles 
 * - Only forward data to clients which are really nearby the sender (in virtual space)
 * @see FG_SERVER::SetOutOfReach and FG_SERVER::IsInRange
 * 
 * \subsection server_playerexpires server.playerexpires
 * \code 
 * server.playerexpires = 10
 * \endcode
 * - Time to keep client information in list without updates in seconds
 * @see FG_SERVER::SetPlayerExpires
 * 
 * 
 * 
 * 
 * \section conf_relays Relays
 * Here you configure to which servers you want your server
 * to send data of local clients to. Remember that those 
 * servers should be configured so that those will sent their client data to your server, too!
 * 
 * \subsection relays relay.host / relay.port
 * \code
 * relay.host = mpserver14.flightgear.org
 * relay.port = 5000
 * # relay.host = mpserver99.flightgear.org
 * # relay.port = 5000
 * \endcode
 * - If you want to interconnect several servers, this option is for you. All other should leave these
 * options out of the configuration. If you want to connect servers, you should add all 
 * servers to the configuration you want to send traffic to.
 * 
 * \note The relay servers you send traffic to must be configured accordingly to relay to you, or your traffic will be ignored!
 * 
 * @see ::FG_SERVER::AddRelay
 * 
 * 
 * 
 * 
 * \section conf_crossfeed Crossfeed 
 * 
 * \subsection crossfeed_set crossfeed.host / crossfeed.port
 * A list of one or more host/port to forward UDP packets to. 
 * \code
 * crossfeed.host = localhost
 * crossfeed.port = 5002
 * # crossfeed.host = other.server.com
 * # crossfeed.port = 5444
 * \endcode
 * - These servers will received all <b>locally received packets</b> without any condition
 * - i.e. all local packets are mirrored to them
 * 
 * @see FG_SERVER::AddCrossfeed and FG_SERVER::SendToCrossfeed
 * 
 * \section client_blacklistsss Client BlackList
 * 
 * \subsection blacklist_conf blacklist
 * List of blacklisted client IPs. Set these to <b>block specific client IPs</b>.
 * \code
 * blacklist = 123.123.123.123
 * blacklist = 12.12.12.12
 * \endcode
 * - Blacklisted IPs will ignore all traffic comming from the given IP.
 * 
 * @see ::FG_SERVER::AddBlacklist
 * 
 * 
 * 
 * \section tracker_conf Tracker Config
 * 
 * \subsection servertracked  server.tracked
 * \code 
 * server.tracked = false
 * \endcode
 * - In most cases you should set this to \b false. You can however, run the tracking server 
 * (part of the source tree, under /crontrib) and point the tracking to your server.
 * If your server is not part of the public network and you are not running a 
 * tracking server you should set this to \b false.
 * 
 * \subsection tracking_server server.tracking_server
 * \code
 * server.tracking_server = 62.112.194.20
 * \endcode
 * - Enter the IP address of the tracking server
 * 
 * \subsection tracking_port server.tracking_port
 * \code
 * server.tracking_port = 8000
 * \endcode
 * - Enter the port number of the tracking server
 * @see ::FG_SERVER::AddTracker
 * 
 * 
 */