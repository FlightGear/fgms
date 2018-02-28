/**
 * @file getopt.hxx
 * @brief getopt is for Windows platform only.
 *        It tries to simulate gnu getopt
 */
#ifndef _msc__getopt_header
#define _msc_getopt_header

#ifdef _MSC_VER

int getopt ( int argc, char* argv[], char* args );

#endif // _MSC_VER
#endif // #ifndef _msc_getopt_header
// eof
