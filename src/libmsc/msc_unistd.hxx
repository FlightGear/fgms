/**
 * @file msc_unistd.hxx
 * @brief msc_unistd is for Windows platform only.
 *        It tries to simulate useful function in unistd.h for Linux
 *			Function available:
 *			usleep()
 */
#ifndef _MSC_UNISTD_HXX_
#define _MSC_UNISTD_HXX_

#ifdef _MSC_VER

extern int usleep (int microseconds);

#endif // _MSC_VER
#endif // #ifndef _MSC_UNISTD_HXX_
// eof
