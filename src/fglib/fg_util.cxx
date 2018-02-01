//
// util functions
//
#ifdef HAVE_CONFIG_H
#include "config.h" // for MSVC, always first
#endif

#include <stdio.h>
#include <sstream>
#include <iomanip>
#include "fg_util.hxx"

namespace fgmp
{

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
	struct tm* tmr { localtime ( &date ) };
	std::stringstream s;

	s << std::setfill ('0') << std::setw(2)
		<< tmr->tm_mday << "."
		<< tmr->tm_mon+1 << "."
		<< std::setw(4) << tmr->tm_year+1900 << " "
		<< std::setw(2) << tmr->tm_hour << ":"
		<< tmr->tm_min << ":"
		<< tmr->tm_sec << " ";
	return s.str();
} // timestamp_to_datestr()

//////////////////////////////////////////////////////////////////////

/**
 * @brief calculate the difference of a timestamp to now and convert
 * the difference to a string representation of the form "3 days 2 hours"
 */
std::string
timestamp_to_days
(
	time_t date
)
{
	time_t diff = time ( 0 ) - date;
	return diff_to_days ( diff );
} // timestamp_to_days

//////////////////////////////////////////////////////////////////////

/**
 * @brief convert a time duration expressed in seconds to a string
 * representation of the form "3 days 2 hours"
 */
std::string
diff_to_days
(
	time_t date
)
{
	time_t diff { date };
	unsigned int temp { 0 };
	std::string result;

	if ( diff > 31536000 )	// years
	{
		temp = diff / 31536000;
		result += num_to_str ( temp, 2 ) + " years";
		return result;
	}
	if ( diff > 86400 )	// days
	{
		temp = diff / 86400;
		result += num_to_str ( temp ) + " days";
		return result;
	}
	if ( diff > 3600 )	// hours
	{
		temp = diff / 3600;
		result += num_to_str ( temp ) + " hours";
		return result;
	}
	if ( diff > 60 )		// minutes
	{
		temp = diff / 60;
		result += num_to_str ( temp ) + " minutes";
		return result;
	}
	temp = diff;
	result += num_to_str ( temp ) + " seconds";
	return result;
} // diff_to_days

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
	if ( bytes > 1099511627776. )
	{
		ret_val = ( bytes / 1099511627776. );
		ret_str = num_to_str ( ret_val, 2 ) + " TiB";
		return ret_str;
	}
	else if ( bytes > 1073741824. )
	{
		ret_val = ( bytes / 1073741824. );
		ret_str = num_to_str ( ret_val, 2 ) + " GiB";
		return ret_str;
	}
	else if ( bytes > 1048576 )
	{
		ret_val = ( bytes / 1048576 );
		ret_str = num_to_str ( ret_val, 2 ) + " MiB";
		return ret_str;
	}
	else if ( bytes > 1024 )
	{
		ret_val = ( bytes / 1024 );
		ret_str = num_to_str ( ret_val, 2 ) + " KiB";
		return ret_str;
	}
	ret_str = num_to_str ( bytes, 2 ) + " b";
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
	std::string const& value,
	std::string const& ending
)
{
	if ( ending.size() > value.size() )
	{
		return false;
	}
	return std::equal ( ending.rbegin(), ending.rend(), value.rbegin() );
} // str_ends_with ()
//////////////////////////////////////////////////////////////////////

} // namespace fgmp

