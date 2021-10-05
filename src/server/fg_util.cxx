#include <fg_util.hxx>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
/**
 * @brief convert a unix timestamp to a string
 * representation of a date
 */
std::string
timestamp_to_datestr
(
	time_t date
)
{
        struct tm *tmr;
        char buf[64];
        tmr = localtime(&date);
        sprintf (buf, "%02d.%02d.%04d %02d:%02d:%02d ",
                tmr->tm_mday,
                tmr->tm_mon+1,
                tmr->tm_year+1900,
                tmr->tm_hour,
                tmr->tm_min,
                tmr->tm_sec);
        return (std::string)buf;
} // timestamp_to_datestr()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief calculate the difference of a timestamp to now and convert
 * the difference to a string representation of the form "3 days 2 hours"
 */
std::string
timestamp_to_days
(
	time_t Date
)
{
        time_t Diff = time (0) - Date;
	return diff_to_days (Diff);
} // timestamp_to_days
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief convert a time duration expressed in seconds to a string
 * representation of the form "3 days 2 hours"
 */
std::string
diff_to_days
(
	time_t Date
)
{
        time_t Diff = Date;
        unsigned int temp = 0;
        std::string Result;

        if (Diff > 31536000)	// years
        {
                temp = Diff / 31536000;
                Result += NumToStr (temp, 0) + " years";
		return Result;
        }
        if (Diff > 86400)	// days
        {
                temp = Diff / 86400;
                Result += NumToStr (temp, 0) + " days";
		return Result;
        }
        if (Diff > 3600)	// hours
        {
                temp = Diff / 3600;
                Result += NumToStr (temp, 0) + " hours";
		return Result;
        }
        if (Diff > 60)		// minutes
        {
                temp = Diff / 60;
                Result += NumToStr (temp, 0) + " minutes";
		return Result;
        }
        temp = Diff;
        Result += NumToStr (temp, 0) + " seconds";
        return Result;

} // diff_to_days
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * Convert a byte counter to a string representation of the form
 * "3.7 Gib" (3.7 Gigibit).
 * The units conform to IEC,
 * see http://physics.nist.gov/cuu/Units/binary.html
 */
std::string
byte_counter
(
	double bytes
)
{
        double ret_val;
        std::string ret_str;

        if (bytes > 1099511627776.)
        {
                ret_val = (bytes / 1099511627776.);
                ret_str = NumToStr (ret_val) + " TiB";                                                                       
                return ret_str;
        }
        else if (bytes > 1073741824.)
        {
                ret_val = (bytes / 1073741824.);
                ret_str = NumToStr (ret_val) + " GiB";
                return ret_str;
        }
        else if (bytes > 1048576)
        {
                ret_val = (bytes / 1048576);
                ret_str = NumToStr (ret_val) + " MiB";
                return ret_str;
        }
        else if (bytes > 1024)
        {
                ret_val = (bytes / 1024);
                ret_str = NumToStr (ret_val) + " KiB";
                return ret_str;
        }
        ret_str = NumToStr (bytes) + " b";
        return ret_str;
} // byte_counter()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @return true	'value' ends with 'ending'
 * @return false else
 */
bool
str_ends_with 
(
	std::string const & value,
	std::string const & ending
)
{
	if ( ending.size() > value.size() )
		return false;
	return std::equal( ending.rbegin(), ending.rend(), value.rbegin() );
} // str_ends_with ()
//////////////////////////////////////////////////////////////////////
