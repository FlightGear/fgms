#include <fg_util.hxx>
#include <stdio.h>

string
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
        return (string)buf;
}

string
timestamp_to_days
(
	time_t Date
)
{
        time_t Diff = time (0) - Date;
        unsigned int temp = 0;

        string Result;
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
}

string
diff_to_days
(
	time_t Date
)
{
        time_t Diff = Date;
        unsigned int Days       = 0;
        unsigned int Hours      = 0;
        unsigned int Minutes    = 0;
        unsigned int Seconds    = 0;

        string Result;
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

string
byte_counter
(
	double bytes
)
{
        double ret_val;
        string ret_str;

        if (bytes > 1125899906842624)
        {
                ret_val = (bytes / 1125899906842624);
                ret_str = NumToStr (ret_val) + " TiB";                                                                       
                return ret_str;
        }
        else if (bytes > 1099511627776)
        {
                ret_val = (bytes / 1099511627776);
                ret_str = NumToStr (ret_val) + " GiB";
                return ret_str;
        }
        else if (bytes > 1073741824)
        {
                ret_val = (bytes / 1073741824);
                ret_str = NumToStr (ret_val) + " MiB";
                return ret_str;
        }
        else if (bytes > 1048576)
        {
                ret_val = (bytes / 1048576);
                ret_str = NumToStr (ret_val) + " KiB";
                return ret_str;
        }
        ret_str = NumToStr (bytes) + " b";
        return ret_str;
}



