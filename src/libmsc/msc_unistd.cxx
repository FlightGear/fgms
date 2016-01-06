/**
 * @file msc_unistd.cxx
 * @brief msc_unistd is for Windows platform only.
 *        It tries to simulate useful function in unistd.h for Linux
 *			Function available:
 *			usleep()
 */
 
#ifdef _MSC_VER
#include "msc_unistd.hxx"

using namespace std;

void MSC_unistd::uSleep(int microseconds) 
{
    LARGE_INTEGER _time1, _time2, _freq;
    _time1.QuadPart = 0;
    _time2.QuadPart = 0;
    _freq.QuadPart  = 0;
    QueryPerformanceCounter(&_time1); 
    QueryPerformanceFrequency(&_freq); 
 
    do 
	{ 
        QueryPerformanceCounter(&_time2); 
    } while ( (_time2.QuadPart - _time1.QuadPart) < microseconds ); 
}
#endif // _MSC_VER