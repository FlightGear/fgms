/**
 * @file msc_unistd.hxx
 * @brief msc_unistd is for Windows platform only.
 *        It tries to simulate useful function in unistd.h for Linux
 *			Function available:
 *			usleep()
 */

#ifdef _MSC_VER
class MSC_unistd
{
	public:
	void usleep (int microseconds);
}
#endif // _MSC_VER