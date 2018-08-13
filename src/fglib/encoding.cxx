// encoding.cxx
// implement different encodings for network packets
//
// Copyright (C) 2006  Oliver Schroeder
//
// This file is part of fgms
//
// $Id: encoding.cxx,v 1.1.1.1 2009/10/12 07:24:05 oliver Exp $

#include <fglib/fg_log.hxx>
#include "encoding.hxx"

namespace
{

#if 0
//////////////////////////////////////////////////
//
//  define __BYTE_ORDER
//  define __LITTELE_ENDIAN
//  define __BIG_ENDIAN
//
//////////////////////////////////////////////////
#ifdef __linux__
#   include <endian.h>
#else
#   define __LITTLE_ENDIAN 1234
#   define __BIG_ENDIAN    4321
#endif

#if defined(__alpha) || defined(__alpha__)
#   ifndef __BYTE_ORDER
#       define __BYTE_ORDER __LITTLE_ENDIAN
#   endif

#elif defined __sgi
#   ifndef __BYTE_ORDER
#       define __BYTE_ORDER __BIG_ENDIAN
#   endif

#elif defined(__CYGWIN__) || defined(_WIN32)
#   ifndef __BYTE_ORDER
#       define __BYTE_ORDER __LITTLE_ENDIAN
#   endif

#endif

#if ((__BYTE_ORDER != __LITTLE_ENDIAN) && (__BYTE_ORDER != __BIG_ENDIAN))
#   error "Architecture not supported."
#   error "please define __BYTE_ORDER for this system"
#endif

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#   define LOW  1
#   define HIGH 0
#else
#   define SWAP32(arg) arg
#   define SWAP64(arg) arg
#   define LOW  0
#   define HIGH 1
#endif

// Use native swap functions if available, they are much
// faster.
// Don't use byteswap.h on Alpha machines because its buggy
#if defined(__linux__) && !(defined(__alpha) || defined(__alpha__))
#   include <byteswap.h>
#   define SWAP16(arg) bswap_16(arg)
#   define SWAP32(arg) bswap_32(arg)
#   define SWAP64(arg) bswap_64(arg)
#else
    inline uint16_t sg_bswap_16(uint16_t x) {
        x = (x >> 8) | (x << 8);
        return x;
    }

    inline uint32_t sg_bswap_32(uint32_t x) {
        x = ((x >>  8) & 0x00FF00FFL) | ((x <<  8) & 0xFF00FF00L);
        x = (x >> 16) | (x << 16);
        return x;
    }

    inline uint64_t sg_bswap_64(uint64_t x) {
        x = ((x >>  8) & 0x00FF00FF00FF00FFLL)
          | ((x <<  8) & 0xFF00FF00FF00FF00LL);
        x = ((x >> 16) & 0x0000FFFF0000FFFFLL)
          | ((x << 16) & 0xFFFF0000FFFF0000LL);
        x =  (x >> 32) | (x << 32);
        return x;
    }
#   define SWAP16(arg) sg_bswap_16(arg)
#   define SWAP32(arg) sg_bswap_32(arg)
#   define SWAP64(arg) sg_bswap_64(arg)
#endif
#endif

#if defined(__linux__)
#	include <byteswap.h>
#	define SWAP16(arg) bswap_16(arg)
#	define SWAP32(arg) bswap_32(arg)
#	define SWAP64(arg) bswap_64(arg)
#elif defined _MSC_VER
#	include <stdlib.h>
#	define SWAP16(arg) _byteswap_ushort(arg)
#	define SWAP32(arg) _byteswap_ulong(arg)
#	define SWAP64(arg) _byteswap_uint64(arg)
#else
#   error "Architecture not supported."
#   error "please file a bugreport at https://sourceforge.net/p/fgms/bugs/?source=navbar"
#endif

} // namespace

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

xdr_data_t
xdr_encode_int8
(
	const int8_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		int8_t      raw;
	} tmp;
	tmp.raw = val;
	tmp.encoded = SWAP32 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_int8 ()

//////////////////////////////////////////////////////////////////////

int8_t
xdr_decode_int8
(
	const xdr_data_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		int8_t      raw;
	} tmp;
	tmp.encoded = SWAP32 ( val );
	return ( tmp.raw );
} // xdr_decode_int8 ()

//////////////////////////////////////////////////////////////////////

xdr_data_t
xdr_encode_uint8
(
	const uint8_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		uint8_t     raw;
	} tmp;
	tmp.raw = val;
	tmp.encoded = SWAP32 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_uint8 ()

//////////////////////////////////////////////////////////////////////

uint8_t
xdr_decode_uint8
(
	const xdr_data_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		uint8_t     raw;
	} tmp;
	tmp.encoded = SWAP32 ( val );
	return ( tmp.raw );
} // xdr_decode_uint8 ()

//////////////////////////////////////////////////////////////////////

xdr_data_t
xdr_encode_int16
(
	const int16_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		int16_t     raw;
	} tmp;
	tmp.raw = val;
	tmp.encoded = SWAP32 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_int16 ()

//////////////////////////////////////////////////////////////////////

int16_t
xdr_decode_int16
(
	const xdr_data_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		int16_t     raw;
	} tmp;
	tmp.encoded = SWAP32 ( val );
	return ( tmp.raw );
} // xdr_decode_int16 ()

//////////////////////////////////////////////////////////////////////

xdr_data_t
xdr_encode_uint16
(
	const uint16_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		uint16_t    raw;
	} tmp;
	tmp.raw = val;
	tmp.encoded = SWAP32 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_uint16 ()

//////////////////////////////////////////////////////////////////////

uint16_t
xdr_decode_uint16
(
	const xdr_data_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		uint16_t    raw;
	} tmp;
	tmp.encoded = SWAP32 ( val );
	return ( tmp.raw );
} // xdr_decode_uint16 ()

//////////////////////////////////////////////////////////////////////

xdr_data_t
xdr_encode_int32
(
	const int32_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		int32_t     raw;
	} tmp;
	tmp.raw = val;
	tmp.encoded = SWAP32 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_int32 ()

//////////////////////////////////////////////////////////////////////

int32_t
xdr_decode_int32
(
	const xdr_data_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		int32_t     raw;
	} tmp;
	tmp.encoded = SWAP32 ( val );
	return ( tmp.raw );
} // xdr_decode_int32 ()

//////////////////////////////////////////////////////////////////////

xdr_data_t
xdr_encode_uint32
(
	const uint32_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		uint32_t    raw;
	} tmp;
	tmp.raw = val;
	tmp.encoded = SWAP32 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_uint32 ()

//////////////////////////////////////////////////////////////////////

uint32_t
xdr_decode_uint32
(
	const xdr_data_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		uint32_t    raw;
	} tmp;
	tmp.encoded = SWAP32 ( val );
	return ( tmp.raw );
} // xdr_decode_uint32 ()

//////////////////////////////////////////////////////////////////////

xdr_data2_t
xdr_encode_int64
(
	const int64_t& val
)
{
	union
	{
		xdr_data2_t encoded;
		int64_t     raw;
	} tmp;
	tmp.raw = val;
	tmp.encoded = SWAP64 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_int64 ()

//////////////////////////////////////////////////////////////////////

int64_t
xdr_decode_int64
(
	const xdr_data2_t& val
)
{
	union
	{
		xdr_data2_t encoded;
		int64_t     raw;
	} tmp;
	tmp.encoded = SWAP64 ( val );
	return ( tmp.raw );
} // xdr_decode_int64 ()

//////////////////////////////////////////////////////////////////////

xdr_data2_t
xdr_encode_uint64
(
	const uint64_t& val
)
{
	union
	{
		xdr_data2_t encoded;
		uint64_t    raw;
	} tmp;
	tmp.raw = val;
	tmp.encoded = SWAP64 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_uint64 ()

//////////////////////////////////////////////////////////////////////

uint64_t
xdr_decode_uint64
(
	const xdr_data2_t& val
)
{
	union
	{
		xdr_data2_t encoded;
		uint64_t    raw;
	} tmp;
	tmp.encoded = SWAP64 ( val );
	return ( tmp.raw );
} // xdr_decode_uint64 ()

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
xdr_data_t
xdr_encode_float
(
	const float& val
)
{
	union
	{
		xdr_data_t  encoded;
		float       raw;
	} tmp;
	if ( sizeof ( float ) != sizeof ( xdr_data_t ) )
	{
		LOG ( fglog::prio::DEBUG,  "xdr_encode_float: "
		  << "sizeof (float) != sizeof (xdr_data_t) !!!" );
	}
	tmp.raw = val;
	tmp.encoded = SWAP32 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_float ()

//////////////////////////////////////////////////////////////////////

float
xdr_decode_float
(
	const xdr_data_t& val
)
{
	union
	{
		xdr_data_t  encoded;
		float       raw;
	} tmp;
	if ( sizeof ( float ) != sizeof ( xdr_data_t ) )
	{
		LOG ( fglog::prio::DEBUG, "xdr_decode_float: "
		  << "sizeof (float) != sizeof (xdr_data_t) !!!" );
	}
	tmp.encoded = SWAP32 ( val );
	return ( tmp.raw );
} // xdr_decode_float ()

//////////////////////////////////////////////////////////////////////

xdr_data2_t
xdr_encode_double
(
	const double& val
)
{
	union
	{
		xdr_data2_t encoded;
		double      raw;
	} tmp;
	if ( sizeof ( double ) != sizeof ( xdr_data2_t ) )
	{
		LOG ( fglog::prio::DEBUG, "xdr_encode_double: "
		  << "sizeof (double) != sizeof (xdr_data2_t) !!!" );
	}
	tmp.raw = val;
	tmp.encoded = SWAP64 ( tmp.encoded );
	return ( tmp.encoded );
} // xdr_encode_double ()

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
double
xdr_decode_double
(
	const xdr_data2_t& val
)
{
	union
	{
		xdr_data2_t encoded;
		double      raw;
	} tmp;
	if ( sizeof ( double ) != sizeof ( xdr_data2_t ) )
	{
		LOG ( fglog::prio::DEBUG, "xdr_decode_double: "
		  << "sizeof (double) != sizeof (xdr_data2_t) !!!" );
	}
	tmp.encoded = SWAP64 ( val );
	return ( tmp.raw );
} // xdr_decode_double ()

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//      encode to network byte order
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

int8_t
net_encode_int8
(
	const int8_t& val
)
{
	return ( val );
} // net_encode_int8 ()

/////////////////////////////////////////////////////////////////////

int8_t
net_decode_int8
(
	const int8_t& val
)
{
	return ( val );
} // net_decode_int8 ()

/////////////////////////////////////////////////////////////////////

uint8_t
net_encode_uint8
(
	const uint8_t& val
)
{
	return ( val );
} // net_encode_uint8 ()

/////////////////////////////////////////////////////////////////////

uint8_t
net_decode_uint8
(
	const uint8_t& val
)
{
	return ( val );
} // net_decode_uint8 ()

/////////////////////////////////////////////////////////////////////

int16_t
net_encode_int16
(
	const int16_t& val
)
{
	union
	{
		int16_t  net;
		int16_t  raw;
	} tmp;
	tmp.raw = val;
	tmp.net = SWAP16 ( tmp.net );
	return ( tmp.net );
} // net_encode_int16 ()

/////////////////////////////////////////////////////////////////////

int16_t
net_decode_int16
(
	const int16_t& val
)
{
	union
	{
		int16_t  net;
		int16_t  raw;
	} tmp;
	tmp.net = SWAP16 ( val );
	return ( tmp.raw );
} // net_decode_int16 ()

/////////////////////////////////////////////////////////////////////

uint16_t
net_encode_uint16
(
	const uint16_t& val
)
{
	union
	{
		uint16_t  net;
		uint16_t  raw;
	} tmp;
	tmp.raw = val;
	tmp.net = SWAP16 ( tmp.net );
	return ( tmp.net );
} // net_encode_uint16 ()

/////////////////////////////////////////////////////////////////////

uint16_t
net_decode_uint16
(
	const uint16_t& val
)
{
	union
	{
		uint16_t  net;
		uint16_t  raw;
	} tmp;
	tmp.net = SWAP16 ( val );
	return ( tmp.raw );
} // net_decode_uint16 ()

/////////////////////////////////////////////////////////////////////

int32_t
net_encode_int32
(
	const int32_t& val
)
{
	union
	{
		int32_t  net;
		int32_t  raw;
	} tmp;
	tmp.raw = val;
	tmp.net = SWAP32 ( tmp.net );
	return ( tmp.net );
} // net_encode_int32 ()

/////////////////////////////////////////////////////////////////////

int32_t
net_decode_int32
(
	const int32_t& val
)
{
	union
	{
		int32_t  net;
		int32_t  raw;
	} tmp;
	tmp.net = SWAP32 ( val );
	return ( tmp.raw );
} // net_decode_int32 ()

/////////////////////////////////////////////////////////////////////

uint32_t
net_encode_uint32
(
	const uint32_t& val
)
{
	union
	{
		uint32_t  net;
		uint32_t  raw;
	} tmp;
	tmp.raw = val;
	tmp.net = SWAP32 ( tmp.net );
	return ( tmp.net );
} // net_encode_uint32 ()

/////////////////////////////////////////////////////////////////////

uint32_t
net_decode_uint32
(
	const uint32_t& val
)
{
	union
	{
		uint32_t  net;
		uint32_t  raw;
	} tmp;
	tmp.net = SWAP32 ( val );
	return ( tmp.raw );
} // net_decode_uint32 ()

/////////////////////////////////////////////////////////////////////

int64_t
net_encode_int64
(
	const int64_t& val
)
{
	union
	{
		int64_t  net;
		int64_t  raw;
	} tmp;
	tmp.raw = val;
	tmp.net = SWAP64 ( tmp.net );
	return ( tmp.net );
} // net_encode_int64 ()

/////////////////////////////////////////////////////////////////////

int64_t
net_decode_int64
(
	const int64_t& val
)
{
	union
	{
		int64_t  net;
		int64_t  raw;
	} tmp;
	tmp.net = SWAP64 ( val );
	return ( tmp.raw );
} // net_decode_int64 ()

/////////////////////////////////////////////////////////////////////

uint64_t
net_encode_uint64
(
	const uint64_t& val
)
{
	union
	{
		uint64_t  net;
		uint64_t  raw;
	} tmp;
	tmp.raw = val;
	tmp.net = SWAP64 ( tmp.net );
	return ( tmp.net );
} // net_encode_uint64 ()

/////////////////////////////////////////////////////////////////////

uint64_t
net_decode_uint64
(
	const uint64_t& val
)
{
	union
	{
		uint64_t  net;
		uint64_t  raw;
	} tmp;
	tmp.net = SWAP64 ( val );
	return ( tmp.raw );
} // net_decode_uint64 ()

/////////////////////////////////////////////////////////////////////

uint32_t
net_encode_float
(
	const float& val
)
{
	union
	{
		uint32_t  net;
		float     raw;
	} tmp;
	if ( sizeof ( float ) != sizeof ( uint32_t ) )
	{
		LOG ( fglog::prio::DEBUG, "net_encode_float: "
		  << "sizeof (float) != 4 !!!" );
	}
	tmp.raw = val;
	tmp.net = SWAP32 ( tmp.net );
	return ( tmp.net );
} // net_encode_float ()

/////////////////////////////////////////////////////////////////////

float
net_decode_float
(
	const uint32_t& val
)
{
	union
	{
		uint32_t  net;
		float     raw;
	} tmp;
	if ( sizeof ( float ) != sizeof ( uint32_t ) )
	{
		LOG ( fglog::prio::DEBUG, "net_decode_float: "
		  << "sizeof (float) != 4 !!!" );
	}
	tmp.net = SWAP32 ( val );
	return ( tmp.raw );
} // net_decode_float ()

/////////////////////////////////////////////////////////////////////

uint64_t
net_encode_double
(
	const double& val
)
{
	union
	{
		uint64_t  net;
		double    raw;
	} tmp;
	if ( sizeof ( double ) != sizeof ( uint64_t ) )
	{
		LOG ( fglog::prio::DEBUG, "net_encode_double: "
		  << "sizeof (double) != sizeof (uint64_t) !!!" );
	}
	tmp.raw = val;
	tmp.net = SWAP64 ( tmp.net );
	return ( tmp.net );
} // net_encode_double ()

/////////////////////////////////////////////////////////////////////

double
net_decode_double
(
	const uint64_t& val
)
{
	union
	{
		uint64_t  net;
		double    raw;
	} tmp;
	if ( sizeof ( double ) != sizeof ( uint64_t ) )
	{
		LOG ( fglog::prio::DEBUG, "net_decode_double: "
		  << "sizeof (double) != sizeof (uint64_t) !!!" );
	}
	tmp.net = SWAP64 ( val );
	return ( tmp.raw );
} // net_decode_double ()

/////////////////////////////////////////////////////////////////////

} // namespace fgmp

