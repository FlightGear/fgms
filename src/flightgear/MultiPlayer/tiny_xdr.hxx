/**
 * @file tiny_xdr.hxx
 * @author Oliver Schroeder
 * @brief Tiny XDR implementation for flightgear
 * 
 * - This implementation is not complete, but implements everything we need.
 * - For further reading on XDR read RFC 1832 - http://www.ietf.org/rfc/rfc1832.txt
 */

//////////////////////////////////////////////////////////////////////
//
//      Tiny XDR implementation for flightgear
//      written by Oliver Schroeder
//      released to the puiblic domain
//
//      
//      
//
//      
//
//////////////////////////////////////////////////////////////////////

#ifndef TINY_XDR_HEADER
#define TINY_XDR_HEADER

#ifdef HAVE_CONFIG_H
#       include <config.h>
#endif
#include <simgear/misc/stdint.hxx>
#include <simgear/debug/logstream.hxx>

#define SWAP16(arg) sgIsLittleEndian() ? sg_bswap_16(arg) : arg
#define SWAP32(arg) sgIsLittleEndian() ? sg_bswap_32(arg) : arg
#define SWAP64(arg) sgIsLittleEndian() ? sg_bswap_64(arg) : arg
#define XDR_BYTES_PER_UNIT  4

/** @brief 4 Bytes */
typedef uint32_t    xdr_data_t;      /* 4 Bytes */

/** @brief 8 Bytes */
typedef uint64_t    xdr_data2_t;     /* 8 Bytes */

#ifdef FG_NDEBUG
#       undef FG_TMPDEBUG
#       define FG_NDEBUG
#endif
#define FG_NDEBUG

/**
 * @brief xdr encode 8, 16 and 32 Bit values
 */
template<typename TYPE>
xdr_data_t XDR_encode ( TYPE Val )
{
        union
        {
                xdr_data_t      encoded;
                TYPE            raw;
        } tmp;

        tmp.raw = Val;
        tmp.encoded = SWAP32(tmp.encoded);
        if (sizeof (TYPE) < 4)
        {
                SG_LOG (SG_IO, SG_DEBUG, "XDR_encode ("
                  << (int32_t) Val << ") -> " << (int32_t) tmp.encoded);
        }
        else
        {
                SG_LOG (SG_IO, SG_DEBUG, "XDR_encode ("
                  << (int32_t) Val << ") -> " << tmp.encoded);
        }
        return (tmp.encoded);
}

/**
 * @brief xdr decode 8, 16 and 32 Bit values
 */
template<typename TYPE>
TYPE XDR_decode ( xdr_data_t Val )
{
        union
        {
                xdr_data_t      encoded;
                TYPE            raw;
        } tmp;

        tmp.encoded = SWAP32(Val);
        if (sizeof (TYPE) < 4)
        {
                SG_LOG (SG_IO, SG_DEBUG, "XDR_decode (" << (int32_t) Val
                  << ") -> " << (int32_t) tmp.raw);
        }
        else
        {
                SG_LOG (SG_IO, SG_DEBUG, "XDR_decode (" << (int32_t) Val
                  << ") -> " << tmp.raw);
        }
        return (tmp.raw);
}

/**
 * @brief xdr encode 64 Bit values
 */
template<typename TYPE>
xdr_data2_t XDR_encode64 ( TYPE Val )
{
        union
        {
                xdr_data2_t     encoded;
                TYPE            raw;
        } tmp;

        tmp.raw = Val;
        tmp.encoded = SWAP64(tmp.encoded);
        SG_LOG (SG_IO, SG_DEBUG, "XDR_encode64 (" << (int32_t) Val << ") -> "
          << tmp.encoded);
        return (tmp.encoded);
}

/**
 * @brief xdr decode 64 Bit values
 */
template<typename TYPE>
TYPE XDR_decode64 ( xdr_data2_t Val )
{
        union
        {
                xdr_data2_t     encoded;
                TYPE            raw;
        } tmp;

        tmp.encoded = SWAP64 (Val);
        SG_LOG (SG_IO, SG_DEBUG, "XDR_decode64 (" << (int32_t) Val << ") -> "
          << tmp.raw);
        return (tmp.raw);
}


//////////////////////////////////////////////////////////////////////
//
//      encode to network byte order
//
/////////////////////////////////////////////////////////////////////

/**
 * @brief encode 8-Bit values to network byte order
 *        (actually encodes nothing, just to satisfy the templates)
 */
template<typename TYPE>
uint8_t
NET_encode8 ( TYPE Val )
{
        union
        {
                uint8_t netbyte;
                TYPE    raw;
        } tmp;

        tmp.raw = Val;
        SG_LOG (SG_IO, SG_DEBUG, "NET_encode8 (" << (int32_t) Val << ") -> "
          << (int32_t) tmp.netbyte);
        return (tmp.netbyte);
}

/**
 * @brief  Decode 8-Bit values from network byte order
 *         (actually decodes nothing, just to satisfy the templates)
 */
template<typename TYPE>
TYPE
NET_decode8 ( uint8_t Val )
{
        union
        {
                uint8_t netbyte;
                TYPE    raw;
        } tmp;

        tmp.netbyte = Val;
        SG_LOG (SG_IO, SG_DEBUG, "NET_decode8 (" << (int32_t) Val << ") -> "
          << (int32_t) tmp.raw);
        return (tmp.raw);
}

/**
 * @brief  Encode 16-Bit values to network byte order
 */
template<typename TYPE>
uint16_t
NET_encode16 ( TYPE Val )
{
        union
        {
                uint16_t        netbyte;
                TYPE            raw;
        } tmp;

        tmp.raw = Val;
        tmp.netbyte = SWAP16(tmp.netbyte);
        SG_LOG (SG_IO, SG_DEBUG, "NET_encode16 (" << Val << ") -> "
          << tmp.netbyte);
        return (tmp.netbyte);
}

/**
 * @brief  Decode 16-Bit values from network byte order
 */
template<typename TYPE>
TYPE
NET_decode16 ( uint16_t Val )
{
        union
        {
                uint16_t        netbyte;
                TYPE            raw;
        } tmp;

        tmp.netbyte = SWAP16(Val);
        SG_LOG (SG_IO, SG_DEBUG, "NET_decode16 (" << Val << ") -> "
          << tmp.raw);
        return (tmp.raw);
}

/**
 * @brief  Encode 32-Bit values to network byte order
 */
template<typename TYPE>
uint32_t
NET_encode32 ( TYPE Val )
{
        union
        {
                uint32_t        netbyte;
                TYPE            raw;
        } tmp;

        tmp.raw = Val;
        tmp.netbyte = SWAP32(tmp.netbyte);
        SG_LOG (SG_IO, SG_DEBUG, "NET_encode32 (" << Val << ") -> "
          << tmp.netbyte);
        return (tmp.netbyte);
}

/**
 * @brief  Decode 32-Bit values from network byte order
 */
template<typename TYPE>
TYPE
NET_decode32 ( uint32_t Val )
{
        union
        {
                uint32_t        netbyte;
                TYPE            raw;
        } tmp;

        tmp.netbyte = SWAP32(Val);
        SG_LOG (SG_IO, SG_DEBUG, "NET_decode32 (" << Val << ") -> "
          << tmp.raw);
        return (tmp.raw);
}

/**
 * @brief  Encode 64-Bit values to network byte order
 */
template<typename TYPE>
uint64_t
NET_encode64 ( TYPE Val )
{
        union
        {
                uint64_t        netbyte;
                TYPE            raw;
        } tmp;

        tmp.raw = Val;
        tmp.netbyte = SWAP64(tmp.netbyte);
        SG_LOG (SG_IO, SG_DEBUG, "NET_encode64 (" << Val << ") -> "
          << tmp.netbyte);
        return (tmp.netbyte);
}

/**
 * @brief  Decode 64-Bit values from network byte order
 */
template<typename TYPE>
TYPE
NET_decode64 ( uint64_t Val )
{
        union
        {
                uint64_t        netbyte;
                TYPE            raw;
        } tmp;

        tmp.netbyte = SWAP64(Val);
        SG_LOG (SG_IO, SG_DEBUG, "NET_decode64 (" << Val << ") -> "
          << tmp.raw);
        return (tmp.raw);
}

#endif


