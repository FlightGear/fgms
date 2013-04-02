 /**
 * \page developers_guide Developers Guide
 * 
 * 
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
 * 
 * 
 * \section code_layout Code Layout
 * * <b>src/</b>
 *   - <b>server/</b> - the source code for \ref fgms
 *   - <b>flightgear/</b>  - see \ref flightgear_inc below
 *   - <b>plib/</b>  - see \ref plib below
 *   - <b>simgear/</b>  - see \ref simgear below
 * * <b>contrib/</b>
 *   - <b>fgracker/</b> - the \ref tracker_server
 *   - <b>mpstatus/</b>  - a www page and perl script to query mp statuses
 *   - <b>plib/</b>  - see \ref plib below
 *   - <b>tools/</b>  - contains check_fgms.sh, a script to check fgms status
 * * <b>docs/</b> - stuff to do with documentation in this page
 * 
 * \section other_libs Other Libs
 * The fgms repository contains all the libs required to install. However some parts were
 * copied, borrowed etc from other sources:
 * 
 * \subsection simgear simgear
 *   -  Low level maths, geospatial calculations, etc
 *   - fgms used the constants, logging and some structures
 *   - simgear and flightear source are very closely related
 *   - Info: http://wiki.flightgear.org/SimGear
 *   - Code: https://gitorious.org/fg/simgear
 *   - parts of simgear  have been copied into <b>src/simgear</b>
 * 
 * \subsection plib plib
 *   -  PLIB - A Suite of Portable Game Libraries
 *   - fgms used the network sockets (see netSocket.hxx and netSocket.cxx)
 *   - Info: http://plib.sourceforge.net
 *   - parts of plib have been copied into <b>src/plib/</b>
 * 
 * \subsection flightgear_inc flightgear 
 *   - fgms uses parts of the mpmessages and \ref xdr code. (see mpmessages.hxx and tiny_xdr.hxx)
 *   - Info: \ref FlightGear page
 *   - parts of flightgear have been copied into <b>src/flightgear/</b>
 * 
 * 
 * \section mp_overview MultiPlayer Overview
 * This is an overview about how the communicatinons works between \ref fgfs instances via an \ref fgms. 
 * * The table below shows communications with three players over a network
 * * Messages are sent a few times a second 
 * 
 * <table>
 * <tr><th>fgms - me</th><th>packet</th><th>fgms</th><th>packet</th><th>fgms - others</th></tr>
 * <tr><td>me</td><td>> send ></td><td>> relay ></td><td>> recv ></td><td>other 1</td></tr>
 * <tr><td> </td><td> </td><td> </td><td>> recv ></td><td>other 2</td></tr>
 * <tr><td>me</td><td>< recv <</td><td>< relay <</td><td>< send <</td><td>other 1</td></tr>
 * <tr><td></td><td> </td><td>> relay ></td><td>> recv ></td><td>other 2</td></tr>
 * </table>
 * 
 * \section date_interchange Data Interchange
 *  - <b>Send</b> 
 *    - packets are assembled on an \ref fgfs instance, values such as callsign, position, speed etc 
 *    - the values are packaged into an \ref xdr encoded binary blob
 *    - the packet is transmitted via udp out to an mp server
 *    - eg --multiplayer=out,15,mpserver14.flightgear.org,5000
 * 
 *  - <b>MpServer</b> 
 *    -  an \ref fgms server will recieve this as a "LOCAL" connection, ie a direct connection (FG_SERVER::HandlePacket)
 *    - fgms will then relay the packet to other local players, and to the relay servers (FG_SERVER::SendToRelays)
 *  - <b>Reciever</b>
 *    -  a \ref fgms client the other ends will recive the packet 
 *    - decode the data encoded in \ref xdr
 *    - then update the sim with this data
 *  - <b>And < vice > versa</b>
 * 
 * 
 * 
 * 
 * \section mp_messages Data Packets
 *  - The data encoded into a sequence of bytes using \ref xdr.
 *  - Every packet starts with 'Header' bytes which define whats in the rest of the packet.
 *  <table>
 *  <tr><th colspan="2">An Mp Message Packet</th></tr>
 *  <tr><td>Header</td><td>Other data</td></tr>
 *  </table>
 * 
 * \subsection p_header Header
 *  - The T_MsgHdr is the c struct used to define the header.
 *  <table>
 *  <tr><th>Name</th><th>Comment</th></tr>
 *  <tr><td>Magic</td><td>The "magic" that must start each packet, see ::MSG_MAGIC</td></tr>
 *  <tr><td>ProtoVer</td><td>The protocol version which must match - see ::PROTO_VER </td></tr>
 *  <tr><td>MsgId</td><td>the MsgId is the "type" of message that is sent</td></tr>
 *  <tr><td>MsgLen</td><td>The length of this message</td></tr>
 *  <tr><td>ReplyAddr</td><td>--not-used--</td></tr>
 *  <tr><td>ReplyPort</td><td>--not-used--</td></tr>
 *  <tr><td>Callsign</td><td>Length is ::MAX_CALLSIGN_LEN abc123</td></tr>
  *  </table>
 *   -  T_MsgHdr::MsgId - the MsgId is  one of:
 *      - ::POS_DATA_ID - a position message and the most trafficked
 *      - ::RESET_DATA_ID - to reset ?
 *      - ::CHAT_MSG_ID - for mp chat BUT
 *        \warning ::CHAT_MSG_ID is not used in current implementation. Instead chat is packaged into properties.
 * 
 * \subsection p_position Position Message
 *  - T_PositionMsg is the c struct used to define a position
 * <table>
 * <tr><th>Name</th><th>Comment</th></tr>
 * <tr><td>Model</td><td>Length  is ::MAX_MODEL_NAME_LEN eg /model/foo/aero.xml</td></tr>
 * <tr><td>time</td><td>epoch time</td></tr>
 * <tr><td>lag</td><td>lag ??</td></tr>
 * <tr><td>position</td><td>double[3]</td></tr>
 * <tr><td>orientation</td><td>float[3]</td></tr>
 * <tr><td>linearVel</td><td>float[3]</td></tr>
 * <tr><td>angularVel</td><td>float[3]</td></tr>
 * <tr><td>linearAccel</td><td>float[3]</td></tr>
 * <tr><td>angularAccel</td><td>float[3]</td></tr>
 * </table>
 * 
 * \section xdr 
 *  * External Data Representation 
 * 		- Info: http://en.wikipedia.org/wiki/External_Data_Representation
 * 		- Spec: http://www.ietf.org/rfc/rfc1832.txt
 *  
 * <i>XDR is a standard data serialization format. It allows data to be transferred between different kinds of computer systems. Converting from the local representation to XDR is called encoding. Converting from XDR to the local representation is called decoding. XDR is implemented as a software library of functions which is portable between different operating systems and is also independent of the transport layer.</i>
 *  
 * fgms uses a small subset of xdr necessary for multiplayer. This is defined in tiny_xdr.hxx
 * 
 * \section contribute Contribute 
 * You are more than welcome to contribute to the project. 
 *  * If you have a patch, or a bug then contact one of the \ref developers or the \ref fg_mailing_list
 *  * Looks at the \ref bugs
 * 
 * 
 * 
 * \section Documentation
 * The documentation is created using doxygen and is generated from source.
 * * Here for some formating info 
 *   - http://www.stack.nl/~dimitri/doxygen/manual/commands.html
 * * Private variables/function are documented
 * * Pages, ie stuff not related to source such as this page are in <b>/doc/pages/</b>
 * * The doxygen tag file is located at <b>fgms.tag</b>
 * * Doxygen 1.8+ is required for building the documents.
 */