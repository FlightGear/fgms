/**                                                                                                                          
 * @file fg_common.cxx
 * @author Oliver Schroeder
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, U$
//
// Copyright (C) 2013  Oliver Schroeder
//

#include <typcnvt.hxx>
#include <fg_common.hxx>

std::string
timestamp_to_datestr ( time_t date )
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
}

std::string
timestamp_to_days ( time_t Date )
{
	time_t Diff = time (0) - Date;
	unsigned int Days	= 0;
	unsigned int Hours	= 0;
	unsigned int Minutes	= 0;
	unsigned int Seconds	= 0;
	std::string Result;

	if (Diff > 86400)
	{
		Days = Diff / 86400;
		Diff -= (Days * 86400);
		Result += NumToStr (Days, 0) + " days ";
	}
	if (Diff > 3600)
	{
		Hours = Diff / 3600;
		Diff -= (Hours * 3600);
		Result += NumToStr (Hours, 0) + " hours ";
	}
	if (Diff > 60)
	{
		Minutes = Diff / 60;
		Diff -= (Minutes * 60);
		Result += NumToStr (Minutes, 0) + " minutes ";
	}
	Seconds = Diff;
	Result += NumToStr (Seconds, 0) + " seconds ";
        return Result;
}

std::string
byte_counter ( uint64 bytes )
{
        double ret_val;
        std::string ret_str;

        if (bytes > 1125899906842624)
        {
                ret_val = ((double) bytes / 1125899906842624);
                ret_str = NumToStr (ret_val) + " TiB";
                return ret_str;
        }
        else if (bytes > 1099511627776)
        {
                ret_val = ((double) bytes / 1099511627776);
                ret_str = NumToStr (ret_val) + " GiB";
                return ret_str;
        }
        else if (bytes > 1073741824)
        {
                ret_val = ((double) bytes / 1073741824);
                ret_str = NumToStr (ret_val) + " MiB";
                return ret_str;
        }
        else if (bytes > 1048576)
        {
                ret_val = ((double) bytes / 1048576);
                ret_str = NumToStr (ret_val) + " KiB";
                return ret_str;
        }
        ret_str = NumToStr (bytes) + " b";
        return ret_str;
}

