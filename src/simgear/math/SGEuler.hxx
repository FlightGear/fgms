/*\
 * SGEuler.cxx
 *
 * Licence: GNU GPL version 2
 *
 * Copyright (c) 2015 - Geoff R. McLane
 * code borrowed from http://pigeond.net/git/?p=flightgear/fgmap.git;a=blob_plain;f=sg_perl/sgmath.cxx;hb=HEAD
 * Copyright (c) 2015 - Pigeond
\*/
#ifndef _SGEULER_HXX_
#define _SGEULER_HXX_

extern void euler_get(float lat, float lon,
                      float ox, float oy, float oz,
                      float *head, float *pitch, float *roll);

#endif // #ifndef _SGEULER_HXX_
/* eof */
