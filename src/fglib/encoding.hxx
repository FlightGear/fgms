// encoding.hxx
// provide encodings for different data types in network packets
//
// This file is part of fgms
//
// Copyright (C) 2006  Oliver Schroeder
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see
// <http://www.gnu.org/licenses/>.
//


#ifndef ENCODING_HXX
#define ENCODING_HXX

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdint.h>

namespace fgmp
{

// #define XDR_BYTES_PER_UNIT  4

/*
 * Data types used in XDR de-/encoding
 */
/** @{ */
using xdr_data_t  = uint32_t;	///< 4 bytes
using xdr_data2_t = uint64_t;	///< 8 bytes
/** @} */

/*
 * XDR encoding/decoding routines.
 * For further reading on XDR read RFC 1832.
 */

/*
 * encoding routines for 8-bit values
 */
/// XDR encode a signed 8 bit value
xdr_data_t  XDR_encode_int8   ( const int8_t&   Val );
/// XDR decode a signed 8 bit value
int8_t      XDR_decode_int8   ( const xdr_data_t& Val );
/// XDR encode an unsigned 8 bit value
xdr_data_t  XDR_encode_uint8  ( const uint8_t& Val );
/// XDR decode an unsigned 8 bit value
uint8_t     XDR_decode_uint8  ( const xdr_data_t& Val );

/*
 * encoding routines for 16-bit values
 */
/// XDR encode a signed 16 bit value
xdr_data_t  XDR_encode_int16  ( const int16_t& Val );
/// XDR decode a signed 16 bit value
int16_t     XDR_decode_int16  ( const xdr_data_t& Val );
/// XDR encode an unsigned 16 bit value
xdr_data_t  XDR_encode_uint16 ( const uint16_t& Val );
/// XDR decode an unsigned 16 bit value
uint16_t    XDR_decode_uint16 ( const xdr_data_t& Val );

/*
 * encoding routines for 32-bit values
 */
/// XDR encode a signed 32 bit value
xdr_data_t  XDR_encode_int32  ( const int32_t& Val );
/// XDR decode a signed 32 bit value
int32_t     XDR_decode_int32  ( const xdr_data_t& Val );
/// XDR encode an unsigned 32 bit value
xdr_data_t  XDR_encode_uint32 ( const uint32_t& Val );
/// XDR decode an unsigned 32 bit value
uint32_t    XDR_decode_uint32 ( const xdr_data_t& Val );

/*
 * encoding routines for 64-bit values
 */
/// XDR encode a signed 64 bit value
xdr_data2_t XDR_encode_int64  ( const int64_t& Val );
/// XDR decode a signed 64 bit value
int64_t     XDR_decode_int64  ( const xdr_data2_t& Val );
/// XDR encode an unsigned 64 bit value
xdr_data2_t XDR_encode_uint64 ( const uint64_t& Val );
/// XDR decode an unsigned 64 bit value
uint64_t    XDR_decode_uint64 ( const xdr_data2_t& Val );

/*
 * encoding routines for floating point values
 * XDR_deencoding
 *
 *  @bug #1 these funtions must be fixed for
 *         none IEEE-encoding architecturs
 *         (eg. vax, big suns etc)
 *  @bug #2 some compilers return 'double'
 *         regardless of return-type 'float'
 *         this must be fixed, too
 *  @bug #3 some machines may need to use a
 *         different endianess for floats!
 */
/// XDR encode a float value
xdr_data_t  XDR_encode_float  ( const float& Val );
/// XDR decode a float value
float       XDR_decode_float  ( const xdr_data_t& Val );
/// XDR encode a double value
xdr_data2_t XDR_encode_double ( const double& Val );
/// XDR decode a double value
double      XDR_decode_double ( const xdr_data2_t& Val );

/*
 * encode to/from network byte order
 * These functions encode values to network byte order.
 */

/*
 * encoding routines for 8-bit values
 */
/// NET encode a signed 8 bit value
int8_t    NET_encode_int8   ( const int8_t& Val );
/// NET decode a signed 8 bit value
int8_t    NET_decode_int8   ( const int8_t& Val );
/// NET encode an unsigned 8 bit value
uint8_t   NET_encode_uint8  ( const uint8_t& Val );
/// NET decode an unsigned 8 bit value
uint8_t   NET_decode_uint8  ( const uint8_t& Val );

/*
 * encoding routines for 16-bit values
 */
/// NET encode a signed 16 bit value
int16_t   NET_encode_int16  ( const int16_t& Val );
/// NET decode a signed 16 bit value
int16_t   NET_decode_int16  ( const int16_t& Val );
/// NET encode an unsigned 16 bit value
uint16_t  NET_encode_uint16 ( const uint16_t& Val );
/// NET decode an unsigned 16 bit value
uint16_t  NET_decode_uint16 ( const uint16_t& Val );

/*
 * NET_encode_32bit encoding routines for 32-bit values
 */
/// NET encode a signed 32 bit value
int32_t   NET_encode_int32  ( const int32_t& Val );
/// NET decode a signed 32 bit value
int32_t   NET_decode_int32  ( const int32_t& Val );
/// NET encode an unsigned 32 bit value
uint32_t  NET_encode_uint32 ( const uint32_t& Val );
/// NET decode an unsigned 32 bit value
uint32_t  NET_decode_uint32 ( const uint32_t& Val );

/*
 * encoding routines for 64-bit values
 */
/// NET encode a signed 64 bit value
int64_t   NET_encode_int64  ( const int64_t& Val );
/// NET decode a signed 64 bit value
int64_t   NET_decode_int64  ( const int64_t& Val );
/// NET encode an unsigned 64 bit value
uint64_t  NET_encode_uint64 ( const uint64_t& Val );
/// NET decode an unsigned 64 bit value
uint64_t  NET_decode_uint64 ( const uint64_t& Val );

/*
 * encoding routines for floating point values
 */
/// NET encode a float value
uint32_t  NET_encode_float  ( const float& Val );
/// NET decode a float value
float     NET_decode_float  ( const uint32_t& Val );
/// NET encode a double value
uint64_t  NET_encode_double ( const double& Val );
/// NET decode a double value
double    NET_decode_double ( const uint64_t& Val );

} // namespace fgmp

#endif

