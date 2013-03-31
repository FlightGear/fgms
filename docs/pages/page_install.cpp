
/*! \page install_guide Installation Guide
 *
 * This is a short overview about installing the \ref fgms. 
 * The reader is assumed to be reasonably familiar with working in a Unix/Linux shell environment.
 * 
 * \note First, make sure to read about pre-requisites and bandwidth etc in \ref hosting
 * 
 * The examples below use the following directory structure:
 * \code
 * /home/my/stuff              < working directory
*  /home/my/stuff/fgms         < the git checkout of the source code
*  /home/my/stuff/build_fgms   < the temp build directory
 * \endcode
 * 
 * \section get_bui Getting & Building fgms
 * 
 * \note 
 *      There are currently no 'pre built binary packages' for \ref fgms. Its expected that
 *      you have a server that you are in control of, can install packages and compile
 *      with root permissions if necessary. Also the install is more focused 
 *      on *nix style systems which is the primary development platform.
 * 
 * - You will need the build tools \b cmake and \b make, and also \b g++ to compile the fgms application.
 *    These can usually be found in the package manager for most the common Linux distributions. 
 * -  You will also need the <b>git client</b> to fetch the source code from the repository.
 * - Once the build tools are installed on your machine, create a directory to hold the source code. 
 * \code
 *  mkdir ~stuff
 *  cd ~stuff
 * \endcode
 * - Now clone the source code with git
 * \code 
 *  git clone git://gitorious.org/fgms/fgms-0-x.git
 *  \endcode
 * - Create and enter a temp directory (this is where all the compilers working files will be written to) and 
 *   configure using cmake
 * \code
 *  mkdir build_fgms
 *  cd build_fgms
 *  cmake ../fgms
 *  \endcode
 * - At this point you should have a working \b fgms binary in the build_fgms/ directory.
 * 
 * \subsection set_config Setting up the config file
 * - Create and edit a  \ref fgms_conf  file according to the instructions found in example.
 * \code 
 * cp ../fgms/src/server/fgms_example.conf ./my-test.conf
 * > edit ./my-test,conf
 * \endcode
 * @note  If the server will be offline or for private use (i.e. LAN-only, 
 *        please comment out the relay servers section. This will save bandwidth 
 *        from the server being consumed.)
 *   
 * 
 * \subsection run_fgms_first Run fgms for the first time
 * 
 * \code 
 * ./fgms -c ./my-test.conf
 * \endcode
 * In addition to its configuration file, \b fgms supports a number of configuration parameters that can be
 * passed at startup (and that will by default override any configuration file settings), 
 * to get a summary of supported parameters, you may want to run:
 * \code 
 * ./fgms --help
 * \endcode
 * 
 * 
 * 
 * 
 *  
 * 
 */


