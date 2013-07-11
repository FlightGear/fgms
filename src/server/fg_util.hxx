/**
 * @file fg_util.hxx
 * @author Oliver Schroeder
 * 
 */

#ifndef TYPCNVTHDR
#define TYPCNVTHDR 1

using namespace std;

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <string>
#include <cstdlib>
#include <ctype.h>		// toupper()
#include <stdint.h>

typedef long long t_longlong;

#if __FreeBSD__
	namespace std {
		template < typename T >
		T abs(T x) {
			return x < 0 ? -x : x;
		}
		template < typename T >
		T llabs(T x) {
			return x < 0 ? -x : x;
		}
	}
#endif

enum NUMERIC_BASE_LIMITS
{
	MIN_BASE = 2,
	MAX_BASE = 36	// 0-9, a-z
};

std::string timestamp_to_datestr ( time_t date );
std::string timestamp_to_days ( time_t date );
std::string diff_to_days ( time_t date );
std::string byte_counter ( double bytes );

//////////////////////////////////////////////////////////////////////
/** 
 * @brief  Convert string into a number
 * @param str_Number	the string representation of a number
 * @param n_Error	if an error (illegal char) occured, n_Error
 * 			point to the position
 * @param n_Base	The base of the string representation
 * @return 
 *        - n_Error -1:     empty string
 *        - n_Error -2:     overflow
 *        - n_Error -3:     base out of range
 *        - n_Error >0:     index of non-numeric
 */
template < class T >
T StrToNum (string str_Number, int &n_Error, int n_Base = 10)
{
	int n_Length;
	int n_Current;
	int n_Index		= 0;
	bool b_IsNegative	= false;
	T T_Result		= 0;
	T T_Tmp			= 0;
	T T_Devisor		= 1;
	T T_Exponent		= 1;
	T T_Current;

	if ((n_Base < MIN_BASE) || (n_Base > MAX_BASE))
	{
		n_Error = -3;
		return (0);
	}
	if (str_Number.size () <= 0)
	{
		//////////////////////////////////////////////////
		//
		//      string with zero-length -> error
		//
		//////////////////////////////////////////////////
		n_Error = -1;
		return (0);
	}
	//////////////////////////////////////////////////
	//
	//      remember signedness
	//
	//////////////////////////////////////////////////
	if (str_Number[n_Index] == '-')
	{
		b_IsNegative = true;
		n_Index++;
	}
	else if (str_Number[n_Index] == '+')
	{
		n_Index++;
	}
	//////////////////////////////////////////////////
	//
	//      walk through the string
	//
	//////////////////////////////////////////////////
	n_Error = 0;
	n_Length = str_Number.size () - 1;
	while ((n_Index <= n_Length) && (str_Number[n_Index] != '.')
	       && (str_Number[n_Index] != ','))
	{
		T_Tmp = T_Result;
		n_Current = str_Number[n_Index];
		if ((n_Current < '0') || (n_Current > '9'))
		{
			n_Current = toupper (n_Current);
			n_Current -= ('A' - 10);
		}
		else
		{
			n_Current -= '0';
		}
		if ((n_Current < 0) || (n_Current > n_Base))
		{
			// character is not a number
			n_Error = n_Index + 1;
			return (T_Result);
		}
		T_Result *= n_Base;
		T_Result += n_Current;
		if (T_Result < T_Tmp)
		{
			// overflow
			n_Error = -2;
			return (T_Result);
		}
		n_Index++;
	}
	//////////////////////////////////////////////////
	//
	//	now for the floating point part
	//
	//////////////////////////////////////////////////
	n_Index++;
	T_Devisor  = 1 / (T) n_Base;
	T_Exponent = 1 * T_Devisor;
	while (n_Index <= n_Length)
	{
		T_Tmp = T_Result;
		n_Current = str_Number[n_Index];
		if ((n_Current < '0') || (n_Current > '9'))
		{
			n_Current = toupper (n_Current);
			n_Current -= ('A' - 10);
		}
		else
		{
			n_Current -= '0';
		}
		if ((n_Current < 0) || (n_Current > n_Base))
		{
			// character is not a number
			n_Error = n_Index;
			return (T_Result);
		}
		T_Current = n_Current * T_Exponent;
		T_Result += T_Current;
		T_Exponent *= T_Devisor;
		if (T_Result < T_Tmp)
		{
			// overflow
			n_Error = -2;
			return (T_Result);
		}
		n_Index++;
	}
	if (b_IsNegative)
	{
		T_Result = -T_Result;
	}
	return (T_Result);
} // StrToNum ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Convert a number to string
 * @param n_Number	the number to convert
 * @param n_Precision	number of digits after the dot
 * @param n_Base	the base of the string representation
 */
template < typename T >
string NumToStr ( T n_Number, int n_Precision = 2, int n_Base = 10 )
{
	const char	Numbers[] = "0123456789abcdef";
	string		str_Return = "";	
	t_longlong	n_WorkNumber;
	int		n_Factor;
	int		n;

	if (n_Number == 0)
	{
		return "0";
	}
	if ((n_Base < 2) || (n_Base > 16))
	{
		return ("0");
	}
	//////////////////////////////////////////////////
	//
	//	for the floating point part
	//
	//////////////////////////////////////////////////
	if (n_Precision != 0)
	{
		n_Factor = 1;
		for (int i=n_Precision; i>0; i--)
		{
			n_Factor *= n_Base;
		}
		T tmp;
		tmp = n_Number - ((t_longlong) n_Number);
		n_WorkNumber = (t_longlong) (tmp * n_Factor);
		n_WorkNumber = (t_longlong) n_WorkNumber;
		if ((n_WorkNumber == 0) && (tmp != 0))
		{
			str_Return = "0" + str_Return;
		}
		else
		{
			n = n_Factor;
#if defined(_MSC_VER) && (!defined(NTDDI_VERSION) || !defined(NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA))   // if less than VISTA, provide alternative
			n_WorkNumber = abs((int)n_WorkNumber);
#else
			n_WorkNumber = std::llabs(n_WorkNumber);
#endif
			while (n_WorkNumber >= 0 && n > 1)
			{
				str_Return = Numbers[n_WorkNumber % n_Base] + str_Return;
				n_WorkNumber /= n_Base;
				n /= n_Base;
			}
		}
		str_Return = "." + str_Return;
	}
	n_WorkNumber = (t_longlong) n_Number;
	if (n_WorkNumber < 0)
	{
		n_WorkNumber = -n_WorkNumber;
	}
	if (n_WorkNumber == 0)
	{
		str_Return = "0" + str_Return;
	}
	while (n_WorkNumber > 0)
	{
		str_Return = Numbers[n_WorkNumber % n_Base] + str_Return;
		n_WorkNumber /= n_Base;
	}
	if (n_Number < 0)
	{
		str_Return = "-" + str_Return;
	}
	return (str_Return);
} // NumToStr ()
//////////////////////////////////////////////////////////////////////

#endif
