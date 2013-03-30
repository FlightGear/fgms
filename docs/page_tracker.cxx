 /**
 * \page tracker The Tracker
 * 
 * The \ref FlightGear Tracker tracks flights performed on multiplayer server.
 * 
 * - Flights are indexed and stored by their Callsign
 * - The following data is logged
 *     - Start Time
 *     - End Time
 *     - Max altitude
 *     - Aircraft
 *     - Duration
 *     - Total distance
 *     - Max ground speed
 *     - Max Mach
 * 
 * The Tracker comprised two seperate parts:
 * * <b>\ref tracker_server</b> - a standalone application
 * * <b>\ref tracker_client</b> - embedded in fgms
 *
 * \section tracker_server Tracker Server
 *  - The Tracker server is run as a standalone application, seperate from \ref fgms
 *  - It could be located on another machine on the internet
 *  - It can track more than one mpserver
 * 
 * 
 * The data is stored is a postgres database
 * 
 * \subsection tracket_server_dev Dev Info
 * The source code for the tracker server is in the <b>contrib/fgtracker/</b> directory and
 * the application is written is c. (see server.c).
 * 
 * 
 * 
 * \section tracker_client Tracker Client
 * This client is embedded in the \ref fgms itelf (see FG_TRACKER by Julien Pierru)
 * The tracker actually forks iteslf as another application:
 * 
 */