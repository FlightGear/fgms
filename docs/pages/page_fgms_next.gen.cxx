 /**
 * \page fgms_next_gen Next Gen
 * 
 * \warning Please note that this proposal is work in progress!
 * 
 * \section fgms_1_x fgms-1-x
 *  There is another project started <b>fgms-1-x</b> to create the
 *  next generation of server. The code is in a seperate repository:
 * * https://gitorious.org/fgms/fgms-1-x
 * 
 * \section next_protocol Proposal
 * * A new protocol for flightgear multiplayer comunication
 * * The new protocol will bring several substantial changes to the multiplayer code of \ref FlightGear adn \ref fgms.
 * 
 * \section new_classes New Classes
 * First of three new classes will be introduced:
 *
 * \subsection NetBuffer
 * This class manages a buffer, a range of memory, we can write data to and read data 
 * from. It automatically takes care about encoding of data and we can read/write any type of data.
 * 
 * \subsection NetBufferPool
 * This class manages a pool of NetBuffers. We can request new buffers from it, and give back no longer used buffers.
 *
 * \subsection NetPeer 
 * This class manages peers (ie. clients). Every peer has a socket and a \ref NetBufferPool 
 * associated with it. The application (like \ref fgsf) simply writes data to the netpeer. 
 * The class netpeer automagically fills in header information and requests a 
 * new buffer from the pool in case the current buffer is full.
 * 
 * 
 * \section overall_2 Data Schematic
 * Over all, the data flow in the multiplayer network will look something like:
 * \image html flightgear_mp_flow.jpg
 * 
 * So, the server will receive data packets from client 1, and queue those packets for all other clients in their send queue.
 * 
 * A client will put the data it wants to send into its send queue and will receive data 
 * for other clients in its receive queue. The received packets will be processed and the 
 * data handled over to the object representing the client.
 *
 * \section new_packets Packets
 * All packets will be transmitted as UDP datagrams and will have a well defined header which looks like this:
 * 
 * <table>
 * <tr><td>SIZE</td>
 * <td>\ref ng_Magic</td>
 * <td>\ref ng_ProtoVersion</td>
 * <td>\ref ng_Flags</td>
 * <td>\ref ng_MsgCounter</td>
 * <td>\ref ng_Timestamp</td>
 * <td>[\ref ng_Data]</td></tr>
 * </table>
 * 
 * \subsection ng_Magic Magic 
 *  would be one of
 * - \b FGNC - FlightGear Network Client
 * - \b FGNS - FlightGear Network Server (needed or at least usefull for relaying data between servers.
 * - \b FGNO - FlightGear Network Observer 
 *    - ie a client which sends no data, eg a radar station 
 * 
 * @see Current ::MSG_MAGIC
 * 
 * 
 * 
 * \subsection ng_ProtoVersion ProtoVersion 
 *  - a number representing the protocol version
 * 
 * @see Current ::PROTO_VER
 * 
 * 
 * 
 * \subsection ng_Flags Flags 
 * - \b RELIABLE - the sender repeats resending the packet until a confirmation is 
 *      received (or the receiver is assumed down)
 * - \b UNRELIABLE - fire and forget (default) If the application writes data 
 *       to a netpeer marked as \b RELIABLE, the whole packet becomes \b RELIABLE.
 * 
 * 
 * \subsection ng_MsgCounter MsgCounter
 * - Every packet send to one client has its unique packet count number
 * 
 * 
 * 
 * \subsection ng_Timestamp Timestamp
 * - Timestamp representing the simulation time at which the packet was sent
 * 
 * 
 * 
 * \subsection ng_Data Data
 *  - \b PROPERTY-ID / \b VALUE pair
 *  - Property/Value pairs are not only meant to represent properties of \ref FlightFear, but 
 *    eg. CHAT|MSG or ACK|MsgID too.
 * 
 * 
 * 
 * \section ng_config Configuration
 * The mapping of property-id to properties within flightgear should be configurable via xml-files. 
 * On the other hand, external applications (like the server or are radar station) 
 * need to know at least some of those properties (eg. locaton/speed). 
 * Maybe we need the xml configuration at the server side, too. 
 * But I don't want to require a user to install \ref plib / \ref simgear in order to install the server.
 * 
 * Some properties get only sent once, during the initialisation phase. 
 * Those properties do not change during a lifetime of a session, like callsign, 
 * used aitcraft etc. 
 * 
 * Other properties get sent in every packet, like position, speed etc. 
 * And others get only sent sometimes, therefor reliably, like changes to flaps, gear etc.
 * 
 * \section xmldef XML Definition
 *
 * The multiplayer protocol definition file is defined in the
 * <a target="_blank"
 *   href="https://gitorious.org/fgms/fgms-1-x/blobs/master/mp-proto-spec.xml"><b>mp-proto-spec.xml</b></a> file.
 *
 * This file will be used within <b>fgfs _and_ fgms</b>, so ensure that both versions match!
 *
 * The xml defines the static map of all id|properties which can be seen
 * in an mp-packet sent over the network. eg
 * \code
    <!--
        ACKnowledge a received packet
        ACK|PACKET-ID
    -->
    <prop type="int" sendovermp="always">
        <id>2</id>
        <path>multiplayer/system/ACK</path>
    </prop>
    <prop type="float" sendovermp="onchange">
        <id>700073</id>
        <path>sim/hitches/aerotow/tow/brake-force</path>
    </prop>
    <prop type="float" sendovermp="onchange">
        <id>700090</id>
        <path>sim/multiplay/chat</path>
    </prop>
    \endcode
 *
 * Although the protocol can handle unknown properties (see comment
 * of IDs below), you should generally include all properties which
 * can be sent in this list.
 *
 * \section ng_ids ID's
 * The IDs of the properties are not coincidental. The value encodes
 * the type (and length) of the corresponding data!
 *
 * For example:
 * -  \b 000000 + \b ID - is reserved for internal networking layer 
 * -  \b 1100000 + \b ID - is for length encoded properties (mainly strings)
 * -  \b 900000 + \b ID - is for FLOAT -> 32-bit properties (4 byte)
 *
 * These are summarised in this table below:
 * \code


    TypeID      starting ID     Bytes
    ------------------------------------------------
      int8          1           100000          4
     uint8          2           200000          4
     int16          3           300000          4
    uint16          4           400000          4
     int32          5           500000          4
    uint32          6           600000          4
     float          9           700000          4
      bool          13          800000          4

    double          10          900000          8
     int64          7           1000000         8
    uint64          8           1100000         8

    string          11          1200000         L
    opaque          12          1300000         L
    \endcode
 *  
 * 
 * \section ng_prop_types Property Types
 * 
 * The data types in the property map fall into the following types:
 * 
 * \subsection ngp_special Special purpose properties
 * * These are internal to the networking layer, and not saved to the property tree
 * * ID is 000000 + id
 * * see https://gitorious.org/fgms/fgms-1-x/blobs/master/mp-proto-spec.xml#line52
 * 
 * 
 * \subsection ngp_len Length encoded properties
 * * mainly string
 * * sent as ID | LENGTH | DATA
 * * ID is 1100000 + id
 * * see https://gitorious.org/fgms/fgms-1-x/blobs/master/mp-proto-spec.xml#line94
 * 
 * 
 * \subsection ngp_bool BOOL
 * * 32-bit properties (4 byte)
 * * sent as ID | DATA
 * * ID is 1300000 + id
 * * see https://gitorious.org/fgms/fgms-1-x/blobs/master/mp-proto-spec.xml#line511
 * 
 * 
 * \subsection ngp_uint UNSIGNED INTEGERS
 * * 32-bit properties (4 byte)
 * * sent as ID | DATA
 * * ID is 600000 + id
 * * see https://gitorious.org/fgms/fgms-1-x/blobs/master/mp-proto-spec.xml#line154
 * 
 * 
 * \subsection ngp_float FLOAT 
 * * 32-bit properties (4 byte)
 * * sent as ID | DATA
 * * ID is 900000 + id
 * * see https://gitorious.org/fgms/fgms-1-x/blobs/master/mp-proto-spec.xml#line202
 * 
 * 
 * \subsection ngp_double DOUBLE
 * * 64-bit properties (8 byte)
 * * sent as ID | DATA
 * * ID is 1000000 + id
 * * see https://gitorious.org/fgms/fgms-1-x/blobs/master/mp-proto-spec.xml#line531
 * 
 * 
 */
