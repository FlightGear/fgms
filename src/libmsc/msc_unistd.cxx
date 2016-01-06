/**
 * @file msc_unistd.cxx
 * @brief msc_unistd is for Windows platform only.
 *        It tries to simulate useful function in unistd.h for Linux
 *			Function available:
 *			usleep(microseconds)
 */
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, US
//
#ifdef HAVE_CONFIG_H
#include "config.h" // always config.h first, if there is one...
#endif

#ifdef _MSC_VER
#include "msc_unistd.hxx"

#ifdef USE_WAIT_OBJECT  // User choosable option
////////////////////////////////////////////////////////////////////////////////////////////////////
// A neat alternate WIN32 implementation - uses WaitForSingleObject on a timer handle
// from : http://stackoverflow.com/questions/5801813/c-usleep-is-obsolete-workarounds-for-windows-mingw
int usleep(int usec) 
{ 
    int iret = 0;
    HANDLE timer; 
    LARGE_INTEGER ft; 

    ft.QuadPart = -( 10 * usec ); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL); 
    if (timer) {
        SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
        WaitForSingleObject(timer, INFINITE); 
        CloseHandle(timer); 
    } else
        iret = -1;
    return iret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
#else // !#ifdef USE_WAIT_OBJECT
////////////////////////////////////////////////////////////////////////////////////////////////////

int usleep(int microseconds)
{
    int iret = 0;
    int ms = microseconds / 1000;   // get whole ms, if any
    int micro = microseconds - (ms * 1000);  // any remainder microseconds

    if (ms)
        Sleep(ms);  // sleep quietly for any whole milli-seconds

    if (micro)      // if a microsecond remainder... ahhh...
    {
        LARGE_INTEGER _time1, _time2;
        _time1.QuadPart = 0;
        _time2.QuadPart = 0;
        if (QueryPerformanceCounter(&_time1)) // get mircoseconds now
        {
            do 
	        { 
                // just burn up CPU time, but ONLY for any microseconds remainder
                if (!QueryPerformanceCounter(&_time2))
                {
                    iret = -1;
                    break;
                }
            } while ( (_time2.QuadPart - _time1.QuadPart) < micro ); 
        } else
            iret = -1;
    }

    return iret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // #ifdef USE_WAIT_OBJECT y/n

#endif // _MSC_VER
// eof
