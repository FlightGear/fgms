/**
 * @file fg_util.hxx
 * @author Oliver Schroeder
 *
 */

#ifndef TYPCNVTHDR
#define TYPCNVTHDR 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cstdlib>
#include <ctype.h>              // toupper()
#include <stdint.h>
#include <time.h>

#include <iostream>     // remove after debug

typedef long long t_longlong;

#if __FreeBSD__ || defined(__CYGWIN__)
namespace std {
template < typename T >
T abs ( T x ) {
        return x < 0 ? -x : x;
}
template < typename T >
T llabs ( T x ) {
        return x < 0 ? -x : x;
}
}
#endif

enum NUMERIC_BASE_LIMITS
{
        MIN_BASE = 2,
        MAX_BASE = 36   // 0-9, a-z
};

std::string timestamp_to_datestr ( time_t date );
std::string timestamp_to_days ( time_t date );
std::string diff_to_days ( time_t date );
std::string byte_counter ( double bytes );
bool str_ends_with ( std::string const& value, std::string const& ending );

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Convert string into a integer number
 * @param str_Number    the string representation of a number
 * @param n_Error       if an error (illegal char) occured, n_Error
 *                      points to the position
 * @param n_Base        The base of the string representation
 * @return
 *        - n_Error -1:     empty string
 *        - n_Error -2:     overflow
 *        - n_Error -3:     base out of range
 *        - n_Error >0:     index of non-numeric
 *
 * @note        Overflow detection will only work correctly if 'T' < 64 bit
 *
 */
template < class T >
T StrToInt ( std::string str_Number, int& n_Error, int n_Base = 10 )
{
        if ( ( n_Base < MIN_BASE ) || ( n_Base > MAX_BASE ) )
        {
                n_Error = -3;
                return ( 0 );
        }
        if ( str_Number.size () <= 0 )
        {
                //////////////////////////////////////////////////
                //
                //      string with zero-length -> error
                //
                //////////////////////////////////////////////////
                n_Error = -1;
                return ( 0 );
        }
        //////////////////////////////////////////////////
        //
        //      remember signedness
        //
        //////////////////////////////////////////////////
        bool b_IsNegative = false;
        int n_Index = 0;
        if ( str_Number[n_Index] == '-' )
        {
                b_IsNegative = true;
                n_Index++;
        }
        else if ( str_Number[n_Index] == '+' )
        {
                n_Index++;
        }
        //////////////////////////////////////////////////
        //
        //      walk through the string
        //
        //////////////////////////////////////////////////
        n_Error = 0;
        int n_Length = str_Number.size () - 1;
        int64_t Result = 0;
        while ( ( n_Index <= n_Length ) && ( str_Number[n_Index] != '.' )
                && ( str_Number[n_Index] != ',' ) )
        {
                int CurrentDigit = str_Number[n_Index];
                if ( ( CurrentDigit < '0' ) || ( CurrentDigit > '9' ) )
                {
                        CurrentDigit = toupper ( CurrentDigit );
                        CurrentDigit -= ( 'A' - 10 );
                }
                else
                {
                        CurrentDigit -= '0';
                }
                if ( ( CurrentDigit < 0 ) || ( CurrentDigit > n_Base ) )
                {
                        // character is not a number
                        n_Error = n_Index + 1;
                        return ( Result );
                }
                Result *= n_Base;
                Result += CurrentDigit;
                n_Index++;
        }
        if ( b_IsNegative )
        {
                Result = -Result;
        }
        T real_result = static_cast<T> ( Result );
        if ( Result != real_result )
        {
                // overflow
                n_Error = -2;
        }
        return ( real_result );
} // StrToInt ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief  Convert string into a floating point number
 * @param str_Number    the string representation of a number
 * @param n_Error       if an error (illegal char) occured, n_Error
 *                      point to the position
 * @param n_Base        The base of the string representation
 * @return
 *        - n_Error -1:     empty string
 *        - n_Error -2:     overflow
 *        - n_Error -3:     base out of range
 *        - n_Error >0:     index of non-numeric
 *
 * @bug         This function is broken somehow. Adding 1234.1 and 0.02
 *              gives me 1234.1
 *              I don't get it. But since we don't currently need it I will
 *              leave this fight for another day.
 *
 */
template < class T >
T StrToFloat ( std::string str_Number, int& n_Error, int n_Base = 10 )
{
        int n_Index = 0;
        if ( ( n_Base < MIN_BASE ) || ( n_Base > MAX_BASE ) )
        {
                n_Error = -3;
                return ( 0 );
        }
        if ( str_Number.size () <= 0 )
        {
                //////////////////////////////////////////////////
                //
                //      string with zero-length -> error
                //
                //////////////////////////////////////////////////
                n_Error = -1;
                return ( 0 );
        }
        //////////////////////////////////////////////////
        //
        //      remember signedness
        //
        //////////////////////////////////////////////////
        bool b_IsNegative = false;
        if ( str_Number[n_Index] == '-' )
        {
                b_IsNegative = true;
                n_Index++;
        }
        else if ( str_Number[n_Index] == '+' )
        {
                n_Index++;
        }
        //////////////////////////////////////////////////
        //
        //      walk through the string
        //
        //////////////////////////////////////////////////
        n_Error = 0;
        int n_Length = str_Number.size () - 1;
        using super_t = T;
        super_t Result = 0.0;
        while ( ( n_Index <= n_Length ) && ( str_Number[n_Index] != '.' )
                && ( str_Number[n_Index] != ',' ) )
        {
                int CurrentDigit = str_Number[n_Index];
                if ( ( CurrentDigit < '0' ) || ( CurrentDigit > '9' ) )
                {
                        CurrentDigit = toupper ( CurrentDigit );
                        CurrentDigit -= ( 'A' - 10 );
                }
                else
                {
                        CurrentDigit -= '0';
                }
                if ( ( CurrentDigit < 0 ) || ( CurrentDigit > n_Base ) )
                {
                        // character is not a number
                        n_Error = n_Index + 1;
                        return ( Result );
                }
//                std::cout << Result << " * 10 = ";
                Result *= n_Base;
//                std::cout << Result << " + " << CurrentDigit;
                Result += CurrentDigit;
//                std::cout << " = " << Result << std::endl;
                n_Index++;
        }
        //////////////////////////////////////////////////
        //
        //      now for the floating point part
        //
        //////////////////////////////////////////////////
        n_Index++;
        super_t Devisor = 1.0 / ( T ) n_Base;
        super_t Exponent = 1.0 * Devisor;
        while ( n_Index <= n_Length )
        {
                int CurrentDigit = str_Number[n_Index];
                if ( ( CurrentDigit < '0' ) || ( CurrentDigit > '9' ) )
                {
                        CurrentDigit = toupper ( CurrentDigit );
                        CurrentDigit -= ( 'A' - 10 );
                }
                else
                {
                        CurrentDigit -= '0';
                }
                if ( ( CurrentDigit < 0 ) || ( CurrentDigit > n_Base ) )
                {
                        // character is not a number
                        n_Error = n_Index;
                        return ( Result );
                }
                super_t T_Current = CurrentDigit * Exponent;
//                std::cout << Result << " + " << T_Current << " (" << Result + T_Current << ") ";
                Result += T_Current;
//                std::cout << " = " << Result << std::endl;
                Exponent *= Devisor;
                n_Index++;
        }
        if ( b_IsNegative )
        {
                Result = -Result;
        }
        T real_result = static_cast<T> ( Result );
        if ( Result != real_result )
        {
                // overflow
                n_Error = -2;
        }
        return ( real_result );
} // StrToFloat ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/**
 * @brief Convert a number to string
 * @param n_Number      the number to convert
 * @param n_Precision   number of digits after the dot
 * @param n_Base        the base of the string representation
 */
template < typename T >
std::string NumToStr ( T n_Number, int n_Precision = 2, int n_Base = 10 )
{
        const char      Numbers[] = "0123456789abcdef";
        std::string     str_Return = "";
        t_longlong      n_WorkNumber;
        int             n_Factor;
        int             n;

        if ( n_Number == 0 )
        {
                return "0";
        }
        if ( ( n_Base < 2 ) || ( n_Base > 16 ) )
        {
                return ( "0" );
        }
        //////////////////////////////////////////////////
        //
        //      for the floating point part
        //
        //////////////////////////////////////////////////
        if ( n_Precision != 0 )
        {
                n_Factor = 1;
                for ( int i = n_Precision; i > 0; i-- )
                {
                        n_Factor *= n_Base;
                }
                T tmp;
                tmp = n_Number - ( ( t_longlong ) n_Number );
                n_WorkNumber = ( t_longlong ) ( tmp * n_Factor );
                n_WorkNumber = ( t_longlong ) n_WorkNumber;
                if ( ( n_WorkNumber == 0 ) && ( tmp != 0 ) )
                {
                        str_Return = "0" + str_Return;
                }
                else
                {
                        n = n_Factor;
#if defined(_MSC_VER) && (!defined(NTDDI_VERSION) || !defined(NTDDI_VISTA) || (NTDDI_VERSION < NTDDI_VISTA))   // if less than VISTA, provide alternative
                        n_WorkNumber = abs ( ( int ) n_WorkNumber );
#else
                        n_WorkNumber = std::llabs ( n_WorkNumber );
#endif
                        while ( n_WorkNumber >= 0 && n > 1 )
                        {
                                str_Return = Numbers[n_WorkNumber % n_Base] + str_Return;
                                n_WorkNumber /= n_Base;
                                n /= n_Base;
                        }
                }
                str_Return = "." + str_Return;
        }
        n_WorkNumber = ( t_longlong ) n_Number;
        if ( n_WorkNumber < 0 )
        {
                n_WorkNumber = -n_WorkNumber;
        }
        if ( n_WorkNumber == 0 )
        {
                str_Return = "0" + str_Return;
        }
        while ( n_WorkNumber > 0 )
        {
                str_Return = Numbers[n_WorkNumber % n_Base] + str_Return;
                n_WorkNumber /= n_Base;
        }
        if ( n_Number < 0 )
        {
                str_Return = "-" + str_Return;
        }
        return ( str_Return );
} // NumToStr ()
//////////////////////////////////////////////////////////////////////

#endif
