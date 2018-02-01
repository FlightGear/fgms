/**
 * @file fg_util.hxx
 * @author Oliver Schroeder
 *
 */

#ifndef TYPCNVTHDR
#define TYPCNVTHDR 1

#include <string>
#include <cstdlib>
#include <ctype.h>		// toupper()
#include <stdint.h>

namespace fgmp
{

typedef long long t_longlong;

template < typename T >
T absolute ( T x )
{
	return x < 0 ? -x : x;
}

enum NUMERIC_BASE_LIMITS
{
	MIN_BASE = 2,
	MAX_BASE = 36	// 0-9, a-z
};

std::string timestamp_to_datestr ( time_t date );
std::string timestamp_to_days ( time_t date );
std::string diff_to_days ( time_t date );
std::string byte_counter ( double bytes );
bool str_ends_with ( std::string const& value, std::string const& ending );

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Convert string into a number
 * @param number	the string representation of a number
 * @param error	if an error (illegal char) occured, error
 * 			point to the position
 * @param base	The base of the string representation
 * @return
 *        - error -1:     empty string
 *        - error -2:     overflow
 *        - error -3:     base out of range
 *        - error >0:     index of non-numeric
 */
template < class T >
T
str_to_num
(
	std::string number,
	int& error,
	int  base = 10
)
{
	int length;
	int str_digit;
	int index		{ 0 };
	bool is_negative	{ false };
	T result		{ 0 };
	T tmp			{ 0 };
	T devisor		{ 1 };
	T exponent		{ 1 };
	T digit;
	if ( ( base < MIN_BASE ) || ( base > MAX_BASE ) )
	{
		error = -3;
		return ( 0 );
	}
	if ( number.size () <= 0 )
	{
		//////////////////////////////////////////////////
		//
		//      string with zero-length -> error
		//
		//////////////////////////////////////////////////
		error = -1;
		return ( 0 );
	}
	//////////////////////////////////////////////////
	//
	//      remember signedness
	//
	//////////////////////////////////////////////////
	if ( number[index] == '-' )
	{
		is_negative = true;
		index++;
	}
	else if ( number[index] == '+' )
	{
		index++;
	}
	//////////////////////////////////////////////////
	//
	//      walk through the string
	//
	//////////////////////////////////////////////////
	error = 0;
	length = number.size () - 1;
	while ( ( index <= length ) && ( number[index] != '.' )
			&& ( number[index] != ',' ) )
	{
		tmp = result;
		str_digit = number[index];
		if ( ( str_digit < '0' ) || ( str_digit > '9' ) )
		{
			str_digit = toupper ( str_digit );
			str_digit -= ( 'A' - 10 );
		}
		else
		{
			str_digit -= '0';
		}
		if ( ( str_digit < 0 ) || ( str_digit > base ) )
		{
			// character is not a number
			error = index + 1;
			return ( result );
		}
		result *= base;
		result += str_digit;
		if ( result < tmp )
		{
			// overflow
			error = -2;
			return ( result );
		}
		index++;
	}
	//////////////////////////////////////////////////
	//
	//	now for the floating point part
	//
	//////////////////////////////////////////////////
	index++;
	devisor  = 1 / ( T ) base;
	exponent = 1 * devisor;
	while ( index <= length )
	{
		tmp = result;
		str_digit = number[index];
		if ( ( str_digit < '0' ) || ( str_digit > '9' ) )
		{
			str_digit = toupper ( str_digit );
			str_digit -= ( 'A' - 10 );
		}
		else
		{
			str_digit -= '0';
		}
		if ( ( str_digit < 0 ) || ( str_digit > base ) )
		{
			// character is not a number
			error = index;
			return ( result );
		}
		digit = str_digit * exponent;
		result += digit;
		exponent *= devisor;
		if ( result < tmp )
		{
			// overflow
			error = -2;
			return ( result );
		}
		index++;
	}
	if ( is_negative )
	{
		result = -result;
	}
	return ( result );
} // str_to_num ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Convert a number to string
 * @param number	the number to convert
 * @param precision	number of digits after the dot
 * @param base	the base of the string representation
 */
template < typename T >
std::string
num_to_str
(
	T number,
	int precision = 0,
	int base = 10
)
{
	const char	digits[] { "0123456789abcdef" };
	std::string	result { "" };
	t_longlong	tmp_number;
	int		factor;
	int		n;
	if ( number == 0 )
	{
		return "0";
	}
	if ( ( base < 2 ) || ( base > 16 ) )
	{
		return ( "0" );
	}
	//////////////////////////////////////////////////
	//
	//	for the floating point part
	//
	//////////////////////////////////////////////////
	if ( precision != 0 )
	{
		factor = 1;
		for ( int i=precision; i>0; i-- )
		{
			factor *= base;
		}
		T tmp;
		tmp = number - ( ( t_longlong ) number );
		tmp_number = ( t_longlong ) ( tmp * factor );
		tmp_number = ( t_longlong ) tmp_number;
		if ( ( tmp_number == 0 ) && ( tmp != 0 ) )
		{
			result = "0" + result;
		}
		else
		{
			n = factor;
			tmp_number = absolute ( ( int ) tmp_number );
			while ( tmp_number >= 0 && n > 1 )
			{
				result = digits[tmp_number % base] + result;
				tmp_number /= base;
				n /= base;
			}
		}
		result = "." + result;
	}
	tmp_number = ( t_longlong ) number;
	if ( tmp_number < 0 )
	{
		tmp_number = -tmp_number;
	}
	if ( tmp_number == 0 )
	{
		result = "0" + result;
	}
	while ( tmp_number > 0 )
	{
		result = digits[tmp_number % base] + result;
		tmp_number /= base;
	}
	if ( number < 0 )
	{
		result = "-" + result;
	}
	return ( result );
} // num_to_str ()
//////////////////////////////////////////////////////////////////////

} // namespace fgmp

#endif
