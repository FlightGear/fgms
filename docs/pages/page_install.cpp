
/*! \page install_guide Installation Guide
 *
 * This is a short overview about installing the \ref fgms. 
 * The reader is assumed to be reasonably familiar with working in a Unix/Linux shell environment.
 * 
 * * Make sure to read about bandwidth etc first in \ref hosting
 * 
 * \note 
 * 		There are currently no "pre build binary packages" for \ref fgms. Its expected that
 *      you have a server that you are in control of, can install packages and compile
 *      with root permissions if necessary. Also the install is more focused 
 *      on *nix style systems which is the primary development platform.
 * 
 * \todo Include windows install (geoff)
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
 * Which will present you with help output.
 * 
 * @see \ref fgms
 *  
 * 
 * 
 */


