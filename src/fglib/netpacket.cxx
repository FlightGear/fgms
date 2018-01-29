// netpacket.cxx -  NetPacket is a buffer for network packets
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

// #define SG_DEBUG_ALL
#ifdef HAVE_CONFIG_H
#include "config.h" // for MSVC, always first
#endif

#include <iostream>
#include "fg_log.hxx"
#include "netpacket.hxx"

//////////////////////////////////////////////////////////////////////

NetPacket::NetPacket
(
	const uint32_t Size
)
{
	if ( Size == 0 )
	{
		return;
	}
	m_Buffer        = new char[Size];
	m_Capacity      = Size;
	m_CurrentIndex  = 0;
	m_BytesInUse    = 0;
	m_SelfAllocated = true;
	m_EncodingType  = NetPacket::XDR;
	Clear();  // FIXME: probably not needed
} // NetPacket::NetPacket ( uint32_t Size )

//////////////////////////////////////////////////////////////////////

NetPacket::~NetPacket
()
{
	if ( ( m_Buffer ) && ( m_SelfAllocated ) )
	{
		delete[] m_Buffer;
	}
} // NetPacket::~NetPacket()

//////////////////////////////////////////////////////////////////////

void
NetPacket::Clear
()
{
	if ( m_Buffer )
	{
		memset ( m_Buffer, 0, m_Capacity );
	}
	m_CurrentIndex = 0;
	m_BytesInUse   = 0;
} // NetPacket::Clear()

//////////////////////////////////////////////////////////////////////

void
NetPacket::Start
()
{
	m_CurrentIndex = 0;
} // NetPacket::Start ()

//////////////////////////////////////////////////////////////////////

void
NetPacket::Reset
()
{
	m_CurrentIndex = 0;
	m_BytesInUse   = 0;
} // NetPacket::Reset ()

//////////////////////////////////////////////////////////////////////

void
NetPacket::Copy
(
	const NetPacket& Packet
)
{
	if ( m_Capacity != Packet.m_Capacity )
	{
		delete m_Buffer;
		m_Capacity = Packet.m_Capacity;
		m_Buffer  = new char[m_Capacity];
	}
	memcpy ( m_Buffer, Packet.m_Buffer, m_Capacity );
	m_Capacity      = Packet.m_Capacity;
	m_BytesInUse    = Packet.m_BytesInUse;
	m_CurrentIndex  = Packet.m_CurrentIndex;
} // NetPacket::Copy ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::SetIndex
(
	const uint32_t Index
)
{
	if ( ( !m_Buffer ) || ( Index > m_BytesInUse ) )
	{
		return ( false );
	}
	m_CurrentIndex = Index;
	return ( true );
} // NetPacket::SetIndex ()

//////////////////////////////////////////////////////////////////////

void
NetPacket::SetEncoding
(
	const BUFFER_ENCODING_TYPE encoding
)
{
	m_EncodingType = encoding;
} // NetPacket::SetEncoding()

//////////////////////////////////////////////////////////////////////

NetPacket::BUFFER_ENCODING_TYPE
NetPacket::GetEncoding
() const
{
	return ( m_EncodingType );
} // NetPacket::GetEncoding()

//////////////////////////////////////////////////////////////////////

uint32_t
NetPacket::Skip
(
	const uint32_t NumberofBytes
)
{
	uint32_t Index = m_CurrentIndex + NumberofBytes;
	if ( ( !m_Buffer ) || ( Index > m_Capacity ) )
	{
		return ( 0 );
	}
	m_CurrentIndex = Index;
	if ( m_CurrentIndex > m_BytesInUse )
	{
		m_BytesInUse = m_CurrentIndex;
	}
	return ( m_CurrentIndex );
} // NetPacket::Skip ()

//////////////////////////////////////////////////////////////////////

uint32_t
NetPacket::Available
() const
{
	return ( m_Capacity - m_CurrentIndex );
} // NetPacket::Available ()

//////////////////////////////////////////////////////////////////////

uint32_t
NetPacket::Capacity
() const
{
	return ( m_Capacity );
} // NetPacket::Capacity ()

//////////////////////////////////////////////////////////////////////

uint32_t
NetPacket::BytesUsed
() const
{
	return ( m_BytesInUse );
} // NetPacket::BytesUsed ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::isAvailable
(
	const uint32_t Size
) const
{
	if ( Available() < Size )
	{
		return ( false );
	}
	return ( true );
} // NetPacket::isAvailable  ( const uint32_t Size )

//////////////////////////////////////////////////////////////////////

void
NetPacket::SetBuffer
(
	const char* Buffer, const uint32_t Size
)
{
	if ( ( !Buffer ) || ( Size == 0 ) )
	{
		return;
	}
	if ( ( m_Buffer ) && ( m_SelfAllocated ) )
	{
		delete[] m_Buffer;
	}
	m_SelfAllocated = false;
	m_Buffer	= ( char* ) Buffer;
	m_Capacity	= Size;
	m_CurrentIndex	= 0;
	m_BytesInUse	= Size;
} // NetPacket::SetBuffer ()

//////////////////////////////////////////////////////////////////////

void
NetPacket::SetUsed
(
	const uint32_t UsedBytes
)
{
	m_BytesInUse = UsedBytes;
} // NetPacket::SetUsed ()

//////////////////////////////////////////////////////////////////////

uint32_t
NetPacket::RemainingData
() const
{
	return ( m_BytesInUse - m_CurrentIndex );
} // NetPacket::RemainingData ()

//////////////////////////////////////////////////////////////////////
//
// eXtended Tiny Encryption Algorithm
// originally developed by David Wheeler and Roger Needham at the Computer
// Laboratory of Cambridge University

void
NetPacket::xtea_encipher
(
	unsigned int num_cycles,
	uint32_t v[2],
	uint32_t const k[4]
)
{
	unsigned int i;
	const uint32_t delta = 0x9E3779B9;
	uint32_t v0 = v[0], v1 = v[1], sum = 0;
	for ( i=0; i < num_cycles; i++ )
	{
		v0 += ( ( ( v1 << 4 ) ^ ( v1 >> 5 ) ) + v1 ) ^ ( sum + k[sum & 3] );
		sum += delta;
		v1 += ( ( ( v0 << 4 ) ^ ( v0 >> 5 ) ) + v0 ) ^ ( sum + k[ ( sum>>11 ) & 3] );
	}
	v[0] = v0;
	v[1] = v1;
} // NetPacket::xtea_encipher ()

//////////////////////////////////////////////////////////////////////

void
NetPacket::xtea_decipher
(
	unsigned int num_cycles,
	uint32_t v[2],
	uint32_t const k[4]
)
{
	unsigned int i;
	const uint32_t delta = 0x9E3779B9;
	uint32_t v0 = v[0], v1 = v[1], sum = delta * num_cycles;
	for ( i=0; i < num_cycles; i++ )
	{
		v1 -= ( ( ( v0 << 4 ) ^ ( v0 >> 5 ) ) + v0 ) ^ ( sum + k[ ( sum>>11 ) & 3] );
		sum -= delta;
		v0 -= ( ( ( v1 << 4 ) ^ ( v1 >> 5 ) ) + v1 ) ^ ( sum + k[sum & 3] );
	}
	v[0] = v0;
	v[1] = v1;
} // NetPacket::xtea_decipher ()

//////////////////////////////////////////////////////////////////////

void
NetPacket::Encrypt
(
	const uint32_t Key[4],
	const uint32_t Offset
)
{
	uint32_t Len = m_BytesInUse;
	uint32_t Mod = Len % XTEA_BLOCK_SIZE;
	if ( ( ! m_Buffer ) || ( m_BytesInUse == 0 ) )
	{
		LOG(log_prio::DEBUG, "NetPacket::Encrypt() - nothing to encrypt!" );
		return;
	}
	if ( m_BytesInUse <= Offset )
	{
		LOG ( log_prio::DEBUG,
		  "NetPacket::Enrypt() - Offset>Used Bytes!" );
		return;
	}
	for ( uint32_t i=Offset; i<=Len-XTEA_BLOCK_SIZE; i+=XTEA_BLOCK_SIZE )
	{
		xtea_encipher ( 32, ( uint32_t* ) &m_Buffer[i], Key );
	}
	if ( Mod )
	{
		uint32_t Off = Len - XTEA_BLOCK_SIZE;
		xtea_encipher ( 32, ( uint32_t* ) &m_Buffer[Off], Key );
	}
} // NetPacket::Encrypt

//////////////////////////////////////////////////////////////////////

void
NetPacket::Decrypt
(
	const uint32_t Key[4],
	const uint32_t Offset
)
{
	uint32_t Len = m_BytesInUse;
	uint32_t Mod = Len % XTEA_BLOCK_SIZE;
	if ( ( ! m_Buffer ) || ( m_BytesInUse == 0 ) )
	{
		LOG ( log_prio::DEBUG,
		  "NetPacket::Decrypt() - nothing to encrypt!" );
		return;
	}
	if ( m_BytesInUse <= Offset )
	{
		LOG ( log_prio::DEBUG,
		  "NetPacket::Decrypt() - Offset>Used Bytes!" );
		return;
	}
	if ( Mod )
	{
		uint32_t Off = Len - XTEA_BLOCK_SIZE;
		xtea_decipher ( 32, ( uint32_t* ) &m_Buffer[Off], Key );
	}
	for ( uint32_t i=Offset; i<=Len-XTEA_BLOCK_SIZE; i+=XTEA_BLOCK_SIZE )
	{
		xtea_decipher ( 32, ( uint32_t* ) &m_Buffer[i], Key );
	}
} // NetPacket::Decrypt

//////////////////////////////////////////////////////////////////////

bool
NetPacket::WriteOpaque
(
	const void* Data,
	const uint32_t Size
)
{
	if ( ! m_Buffer )
	{
		// no buffer, so nothing to do but something is wrong
		throw ( std::runtime_error (
		  "NetPacket::WriteOpaque (): no buffer" )
		);
	}
	if ( ( ! Data ) || ( Size == 0 ) )
	{
		// no data, so we are done
		return ( true );
	}
	// round data size to a multiple of XDR datasize
	int rndup = 0;
	if ( m_EncodingType == NetPacket::XDR )
	{
		int AlignBytes = sizeof ( xdr_data_t );
		rndup = Size % AlignBytes;
		if ( rndup )
		{
			rndup = AlignBytes - rndup;
		}
	}
	if ( Available() < Size+rndup )
	{
		// not enough space left for data
		return ( false );
	}
	Write_uint32 ( Size+rndup );
	char* Index = ( char* ) m_Buffer + m_CurrentIndex;
	memcpy ( Index, Data, Size );
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += Size;
		m_BytesInUse += Size;
	}
	if ( rndup )
	{
		memset ( m_Buffer+m_CurrentIndex, 0, rndup );
		if ( m_CurrentIndex == m_BytesInUse )
		{
			m_CurrentIndex += rndup;
			m_BytesInUse += rndup;
		}
	}
	return ( true );
} // NetPacket::WriteOpaque ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::WriteString
(
	const std::string& Str
)
{
	return ( WriteOpaque ( ( const void* ) Str.c_str(), Str.size() +1 ) );
} // WriteString ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::WriteCStr
(
	const char* Str,
	uint32_t Size
)
{
	if ( ! m_Buffer )
	{
		// no buffer, so nothing to do but something is wrong
		throw std::runtime_error (
			"NetPacket::WriteCStr (): no buffer" );
	}
	if ( ( ! Str ) || ( Size == 0 ) )
	{
		// no data, so we are done
		return ( true );
	}
	uint32_t len = Size;
	if ( Size == 0 )
	{
		len = strlen ( Str );
	}
	if ( len == 0 )
	{
		// no data, so we are done
		return ( true );
	}
	if ( Available() < len )
	{
		// not enough space left for data
		return ( false );
	}
	char* Index = ( char* ) m_Buffer + m_CurrentIndex;
	memcpy ( Index, Str, len );
	Index += len;
	*Index = '0';
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += len;
		m_BytesInUse += len;
	}
	return ( true );
} // NetPacket::WriteCStr ()

//////////////////////////////////////////////////////////////////////

void
NetPacket::ReadOpaque
(
	void* Buffer,
	uint32_t& Size
)
{
	uint32_t data_available = RemainingData();
	if ( ( ! m_Buffer ) || ( data_available == 0 ) )
	{
		throw std::runtime_error (
			"NetPacket::ReadOpaque (): no more data to read" );
	}
	Size = Read_uint32 ();
	if ( data_available < Size )
	{
		throw std::runtime_error (
			"NetPacket::ReadOpaque (): invalid length" );
	}
	char* ret = ( char* )  &m_Buffer[m_CurrentIndex];
	memcpy ( Buffer, ret, Size );
	m_CurrentIndex += Size;
} // NetPacket::ReadOpaque ()

//////////////////////////////////////////////////////////////////////

std::string
NetPacket::ReadString
()
{
	uint32_t data_available = RemainingData();
	if ( ( ! m_Buffer ) || ( data_available == 0 ) )
	{
		throw std::runtime_error (
			"NetPacket::ReadString (): no more data to read" );
	}
	uint32_t Size = Read_uint32 ();
	if ( data_available < Size )
	{
		throw std::runtime_error (
			"NetPacket::ReadString (): invalid length" );
	}
	std::string ret = ( char* )  &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += Size;
	return ( ret );
} // NetPacket::ReadString ()

//////////////////////////////////////////////////////////////////////

std::string
NetPacket::ReadCStr
()
{
	if ( ( ! m_Buffer ) || ( RemainingData() == 0 ) )
	{
		throw std::runtime_error (
			"NetPacket::ReadCStr (): no more data to read" );
	}
	std::string ret = ( char* )  &m_Buffer[m_CurrentIndex];
	int len = ret.size () + 1;
	m_CurrentIndex += len;
	return ( ret );
} // NetPacket::ReadCStr ()

//////////////////////////////////////////////////////////////////////

int8_t
NetPacket::Read_XDR_int8
()
{
	uint32_t Data;
	try
	{
		Data = Read_NONE_int32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_int8 ( Data ) );
} // NetPacket::Read_XDR_int8 ()

//////////////////////////////////////////////////////////////////////

uint8_t
NetPacket::Read_XDR_uint8
()
{
	uint32_t Data;
	try
	{
		Data = Read_NONE_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_uint8 ( Data ) );
} // NetPacket::Read_XDR_uint8 ()

//////////////////////////////////////////////////////////////////////

int16_t
NetPacket::Read_XDR_int16
()
{
	uint32_t Data;
	try
	{
		Data = Read_NONE_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_int16 ( Data ) );
} // NetPacket::Read_XDR_int16 ()

//////////////////////////////////////////////////////////////////////

uint16_t
NetPacket::Read_XDR_uint16
()
{
	uint32_t Data;
	try
	{
		Data = Read_NONE_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_uint16 ( Data ) );
} // NetPacket::Read_XDR_uint16 ()

//////////////////////////////////////////////////////////////////////

int32_t
NetPacket::Read_XDR_int32
()
{
	uint32_t Data;
	try
	{
		Data = Read_NONE_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_int32 ( Data ) );
} // NetPacket::Read_XDR_int32 ()

//////////////////////////////////////////////////////////////////////

uint32_t
NetPacket::Read_XDR_uint32
()
{
	uint32_t Data;
	try
	{
		Data = Read_NONE_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_uint32 ( Data ) );
} // NetPacket::Read_XDR_uint32 ()

//////////////////////////////////////////////////////////////////////

int64_t
NetPacket::Read_XDR_int64
()
{
	uint64_t Data;
	try
	{
		Data = Read_NONE_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_int64 ( Data ) );
} // NetPacket::Read_XDR_int64 ()

//////////////////////////////////////////////////////////////////////

uint64_t
NetPacket::Read_XDR_uint64
()
{
	uint64_t Data;
	try
	{
		Data = Read_NONE_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_uint64 ( Data ) );
} // NetPacket::Read_XDR_uint64 ()

//////////////////////////////////////////////////////////////////////

float
NetPacket::Read_XDR_float
()
{
	uint32_t Data;
	try
	{
		Data = Read_NONE_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_float ( Data ) );
} // NetPacket::Read_XDR_float ()

//////////////////////////////////////////////////////////////////////

double
NetPacket::Read_XDR_double
()
{
	uint64_t Data;
	try
	{
		Data = Read_NONE_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( XDR_decode_double ( Data ) );
} // NetPacket::Read_XDR_double ()

//////////////////////////////////////////////////////////////////////

int8_t
NetPacket::Read_NET_int8
()
{
	int8_t Data;
	try
	{
		Data = Read_NONE_int8 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_uint8 ( Data ) );
} // NetPacket::Read_NET_int8 ()

//////////////////////////////////////////////////////////////////////

uint8_t
NetPacket::Read_NET_uint8
()
{
	uint8_t Data;
	try
	{
		Data = Read_NONE_uint8 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_uint8 ( Data ) );
} // NetPacket::Read_NET_uint8 ()

//////////////////////////////////////////////////////////////////////

int16_t
NetPacket::Read_NET_int16
()
{
	int16_t Data;
	try
	{
		Data = Read_NONE_uint16 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_int16 ( Data ) );
} // NetPacket::Read_NET_int16 ()

//////////////////////////////////////////////////////////////////////

uint16_t
NetPacket::Read_NET_uint16
()
{
	uint16_t Data;
	try
	{
		Data = Read_NONE_uint16 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_uint16 ( Data ) );
} // NetPacket::Read_NET_uint16 ()

//////////////////////////////////////////////////////////////////////

int32_t
NetPacket::Read_NET_int32
()
{
	int32_t Data;
	try
	{
		Data = Read_NONE_int32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_int32 ( Data ) );
} // NetPacket::Read_NET_int32 ()

//////////////////////////////////////////////////////////////////////

uint32_t
NetPacket::Read_NET_uint32
()
{
	uint32_t Data;
	try
	{
		Data = Read_NONE_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_uint32 ( Data ) );
} // NetPacket::Read_NET_uint32 ()

//////////////////////////////////////////////////////////////////////

int64_t
NetPacket::Read_NET_int64
()
{
	int64_t Data;
	try
	{
		Data = Read_NONE_int64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_int64 ( Data ) );
} // NetPacket::Read_NET_int64 ()

//////////////////////////////////////////////////////////////////////

uint64_t
NetPacket::Read_NET_uint64
()
{
	uint64_t Data;
	try
	{
		Data = Read_NONE_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_uint64 ( Data ) );
} // NetPacket::Read_NET_int64 ()

//////////////////////////////////////////////////////////////////////

float
NetPacket::Read_NET_float
()
{
	uint32_t Data;
	try
	{
		Data = Read_NONE_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_float ( Data ) );
} // NetPacket::Read_NET_float ()

//////////////////////////////////////////////////////////////////////

double
NetPacket::Read_NET_double
()
{
	uint64_t Data;
	try
	{
		Data = Read_NONE_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( NET_decode_double ( Data ) );
} // NetPacket::Read_NET_double ()

//////////////////////////////////////////////////////////////////////

int8_t
NetPacket::Read_NONE_int8
()
{
	unsigned int size;
	size = sizeof ( int8_t );
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_int8 (): no more data to read" );
	}
	int8_t* Data = ( int8_t* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( * Data );
} // NetPacket::Read_NONE_int8 ()

//////////////////////////////////////////////////////////////////////

uint8_t
NetPacket::Read_NONE_uint8
()
{
	unsigned int size;
	size = sizeof ( uint8_t );
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_uint8 (): no more data to read" );
	}
	uint8_t* Data = ( uint8_t* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( *Data );
} // NetPacket::Read_NONE_uint8 ()

//////////////////////////////////////////////////////////////////////

int16_t
NetPacket::Read_NONE_int16
()
{
	unsigned int size;
	size = sizeof ( int16_t );
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_int16 (): no more data to read" );
	}
	int16_t* Data = ( int16_t* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( *Data );
} // NetPacket::Read_NONE_int16 ()

//////////////////////////////////////////////////////////////////////

uint16_t
NetPacket::Read_NONE_uint16
()
{
	unsigned int size;
	size = sizeof ( uint16_t );
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_uint16 (): no more data to read" );
	}
	uint16_t* Data = ( uint16_t* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( *Data );
} // NetPacket::Read_NONE_uint16 ()

//////////////////////////////////////////////////////////////////////

int32_t
NetPacket::Read_NONE_int32
()
{
	unsigned int size;
	size = sizeof ( int32_t );
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_int32 (): no more data to read" );
	}
	int32_t* Data = ( int32_t* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( *Data );
} // NetPacket::Read_NONE_int32  ()

//////////////////////////////////////////////////////////////////////

uint32_t
NetPacket::Read_NONE_uint32
()
{
	unsigned int size;
	size = sizeof ( uint32_t );
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_uint32 (): no more data to read" );
	}
	uint32_t* Data = ( uint32_t* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( *Data );
} // NetPacket::Read_NONE_uint32  ()

//////////////////////////////////////////////////////////////////////

int64_t
NetPacket::Read_NONE_int64
()
{
	unsigned int size;
	size = sizeof ( int64_t );
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_int64 (): no more data to read" );
	}
	int64_t* Data = ( int64_t* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( *Data );
} // NetPacket::Read_NONE_int64  ()

//////////////////////////////////////////////////////////////////////

uint64_t
NetPacket::Read_NONE_uint64
()
{
	unsigned int size;
	size = sizeof ( uint64_t );
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_uint64 (): no more data to read" );
	}
	uint64_t* Data = ( uint64_t* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( *Data );
} // NetPacket::Read_NONE_uint64  ()

//////////////////////////////////////////////////////////////////////

float
NetPacket::Read_NONE_float
()
{
	unsigned int size;
	size = sizeof ( float );
	if ( size > sizeof ( xdr_data_t ) )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_float (): wrong size of float" );
	}
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_float (): no more data to read" );
	}
	float* Data = ( float* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( *Data );
} // NetPacket::Read_NONE_float  ()

//////////////////////////////////////////////////////////////////////

double
NetPacket::Read_NONE_double
()
{
	unsigned int size;
	size = sizeof ( double );
	if ( size != sizeof ( xdr_data2_t ) )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_double (): wrong size of double" );
	}
	if ( size > RemainingData () )
	{
		throw std::runtime_error (
			"NetPacket::Read_NONE_double (): no more data to read" );
	}
	double* Data = ( double* ) &m_Buffer[m_CurrentIndex];
	m_CurrentIndex += size;
	return ( *Data );
} // NetPacket::Read_NONE_double  ()

//////////////////////////////////////////////////////////////////////

int8_t
NetPacket::Read_int8
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_int8 (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_int8 () );
		case NetPacket::NET:
			return ( Read_NET_int8 () );
		case NetPacket::NONE:
			return ( Read_NONE_int8 () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_int8 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_int8 ()

//////////////////////////////////////////////////////////////////////

uint8_t
NetPacket::Read_uint8
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_uint8 (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_uint8 () );
		case NetPacket::NET:
			return ( Read_NET_uint8 () );
		case NetPacket::NONE:
			return ( Read_NONE_uint8 () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_uint8 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_uint8 ()

//////////////////////////////////////////////////////////////////////

int16_t
NetPacket::Read_int16
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_int16 (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_int16 () );
		case NetPacket::NET:
			return ( Read_NET_int16 () );
		case NetPacket::NONE:
			return ( Read_NONE_int16 () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_int16 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_int16 ()

//////////////////////////////////////////////////////////////////////

uint16_t
NetPacket::Read_uint16
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_uint16 (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_uint16 () );
		case NetPacket::NET:
			return ( Read_NET_uint16 () );
		case NetPacket::NONE:
			return ( Read_NONE_uint16 () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_uint16 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_uint16 ()

//////////////////////////////////////////////////////////////////////

int32_t
NetPacket::Read_int32
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_int32 (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_int32 () );
		case NetPacket::NET:
			return ( Read_NET_int32 () );
		case NetPacket::NONE:
			return ( Read_NONE_int32 () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_int32 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_int32 ()

//////////////////////////////////////////////////////////////////////

uint32_t
NetPacket::Read_uint32
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_uint32 (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_uint32 () );
		case NetPacket::NET:
			return ( Read_NET_uint32 () );
		case NetPacket::NONE:
			return ( Read_NONE_uint32 () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_uint32 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_uint32 ()

//////////////////////////////////////////////////////////////////////

int64_t
NetPacket::Read_int64
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_int64 (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_int64 () );
		case NetPacket::NET:
			return ( Read_NET_int64 () );
		case NetPacket::NONE:
			return ( Read_NONE_int64 () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_int64 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_int64 ()

//////////////////////////////////////////////////////////////////////

uint64_t
NetPacket::Read_uint64
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_uint64 (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_uint64 () );
		case NetPacket::NET:
			return ( Read_NET_uint64 () );
		case NetPacket::NONE:
			return ( Read_NONE_uint64 () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_uint64 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_uint64 ()

//////////////////////////////////////////////////////////////////////

float
NetPacket::Read_float
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_float (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_float () );
		case NetPacket::NET:
			return ( Read_NET_float () );
		case NetPacket::NONE:
			return ( Read_NONE_float () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_float (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_float ()

//////////////////////////////////////////////////////////////////////

double
NetPacket::Read_double
()
{
	if ( ! m_Buffer )
	{
		throw std::runtime_error (
			"NetPacket::Read_double (): no buffer" );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Read_XDR_double () );
		case NetPacket::NET:
			return ( Read_NET_double () );
		case NetPacket::NONE:
			return ( Read_NONE_double () );
		default:
			throw std::runtime_error (
				"NetPacket::Read_double (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( 0 ); // keep compilers happy
} // Read_double ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_int8
(
	const int8_t& Data
)
{
	try
	{
		Write_NONE_uint32 ( XDR_encode_int8 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_int8 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_uint8
(
	const uint8_t& Data
)
{
	try
	{
		Write_NONE_uint32 ( XDR_encode_uint8 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_uint8 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_int16
(
	const int16_t& Data
)
{
	try
	{
		Write_NONE_uint32 ( XDR_encode_int16 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_int16 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_uint16
(
	const uint16_t& Data
)
{
	try
	{
		Write_NONE_uint32 ( XDR_encode_uint16 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_uint16 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_int32
(
	const int32_t& Data
)
{
	try
	{
		Write_NONE_uint32 ( XDR_encode_int32 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_int32 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_uint32
(
	const uint32_t& Data
)
{
	try
	{
		Write_NONE_uint32 ( XDR_encode_uint32 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_uint32 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_int64
(
	const int64_t& Data
)
{
	try
	{
		Write_NONE_uint64 ( XDR_encode_int64 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_int64 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_uint64
(
	const uint64_t& Data
)
{
	try
	{
		Write_NONE_uint64 ( XDR_encode_uint64 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_uint64 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_float
(
	const float& Data
)
{
	unsigned int size;
	size = sizeof ( xdr_data_t );
	if ( size != sizeof ( float ) )
	{
		throw std::runtime_error (
			"NetPacket::Write_XDR_float (): wrong size of float" );
	}
	try
	{
		Write_NONE_uint32 ( XDR_encode_float ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_float ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_XDR_double
(
	const double& Data
)
{
	unsigned int size;
	size = sizeof ( xdr_data2_t );
	if ( size != sizeof ( double ) )
	{
		throw std::runtime_error (
			"NetPacket::Write_XDR_double (): wrong size of double" );
	}
	try
	{
		Write_NONE_uint64 ( XDR_encode_double ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_XDR_double ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_int8
(
	const int8_t& Data
)
{
	try
	{
		Write_NONE_int8 ( NET_encode_int8 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_int8 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_uint8
(
	const uint8_t& Data
)
{
	try
	{
		Write_NONE_uint8 ( NET_encode_uint8 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_uint8 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_int16
(
	const int16_t& Data
)
{
	try
	{
		Write_NONE_int16 ( NET_encode_int16 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_int16 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_uint16
(
	const uint16_t& Data
)
{
	try
	{
		Write_NONE_uint16 ( NET_encode_uint16 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_uint16 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_int32
(
	const int32_t& Data
)
{
	try
	{
		Write_NONE_int32 ( NET_encode_int32 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_int32 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_uint32
(
	const uint32_t& Data
)
{
	try
	{
		Write_NONE_uint32 ( NET_encode_uint32 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_uint32 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_int64
(
	const int64_t& Data
)
{
	try
	{
		Write_NONE_int64 ( NET_encode_int64 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_int64 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_uint64
(
	const uint64_t& Data
)
{
	try
	{
		Write_NONE_uint64 ( NET_encode_uint64 ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_uint64 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_float
(
	const float& Data
)
{
	try
	{
		Write_NONE_uint32 ( NET_encode_float ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_float ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NET_double
(
	const double& Data
)
{
	try
	{
		Write_NONE_uint64 ( NET_encode_double ( Data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ( ex );
	}
	return ( true );
} // NetPacket::Write_NET_double ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_int8
(
	const int8_t& Data
)
{
	unsigned int size;
	size = sizeof ( int8_t );
	if ( Available() < size )
	{
		return ( false );
	}
	int8_t* Index = ( int8_t* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_int8 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_uint8
(
	const uint8_t& Data
)
{
	unsigned int size;
	size = sizeof ( uint8_t );
	if ( Available() < size )
	{
		return ( false );
	}
	uint8_t* Index = ( uint8_t* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_uint8 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_int16
(
	const int16_t& Data
)
{
	unsigned int size;
	size = sizeof ( int16_t );
	if ( Available() < size )
	{
		return ( false );
	}
	int16_t* Index = ( int16_t* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_int16 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_uint16
(
	const uint16_t& Data
)
{
	unsigned int size;
	size = sizeof ( uint16_t );
	if ( Available() < size )
	{
		return ( false );
	}
	uint16_t* Index = ( uint16_t* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_uint16 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_int32
(
	const int32_t& Data
)
{
	unsigned int size;
	size = sizeof ( int32_t );
	if ( Available() < size )
	{
		return ( false );
	}
	int32_t* Index = ( int32_t* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_int32 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_uint32
(
	const uint32_t& Data
)
{
	unsigned int size;
	size = sizeof ( uint32_t );
	if ( Available() < size )
	{
		return ( false );
	}
	uint32_t* Index = ( uint32_t* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_uint32 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_int64
(
	const int64_t& Data
)
{
	unsigned int size;
	size = sizeof ( int64_t );
	if ( Available() < size )
	{
		return ( false );
	}
	int64_t* Index = ( int64_t* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_int64 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_uint64
(
	const uint64_t& Data
)
{
	unsigned int size;
	size = sizeof ( uint64_t );
	if ( Available() < size )
	{
		return ( false );
	}
	uint64_t* Index = ( uint64_t* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_uint64 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_float
(
	const float& Data
)
{
	unsigned int size;
	size = sizeof ( float );
	if ( Available() < size )
	{
		return ( false );
	}
	float* Index = ( float* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_float ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_NONE_double
(
	const double& Data
)
{
	unsigned int size;
	size = sizeof ( double );
	if ( Available() < size )
	{
		return ( false );
	}
	double* Index = ( double* ) &m_Buffer[m_CurrentIndex];
	*Index = Data;
	if ( m_CurrentIndex == m_BytesInUse )
	{
		m_CurrentIndex += size;
		m_BytesInUse += size;
	}
	return true;
} // NetPacket::Write_NONE_double ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_int8
(
	const int8_t& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_int8 ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_int8 ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_int8 ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); // never reached
} // NetPacket::Write_int8 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_uint8
(
	const uint8_t& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_uint8 ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_uint8 ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_uint8 ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); // never reached
} // NetPacket::Write_uint8 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_int16
(
	const int16_t& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_int16 ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_int16 ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_int16 ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); //  never reached
} // NetPacket::Write_int16 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_uint16
(
	const uint16_t& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_uint16 ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_uint16 ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_uint16 ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); // never reached
} // NetPacket::Write_uint16 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_int32
(
	const int32_t& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_int32 ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_int32 ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_int32 ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); // never reached
} // NetPacket::Write_int32 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_uint32
(
	const uint32_t& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_uint32 ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_uint32 ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_uint32 ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); // never reached
} // NetPacket::Write_uint32 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_int64
(
	const int64_t& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_int64 ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_int64 ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_int64 ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); // never reached
} // NetPacket::Write_int64 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_uint64
(
	const uint64_t& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_uint64 ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_uint64 ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_uint64 ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); // never reached
} // NetPacket::Write_uint64 ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_float
(
	const float& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_float ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_float ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_float ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); // never reached
} // NetPacket::Write_float ()

//////////////////////////////////////////////////////////////////////

bool
NetPacket::Write_double
(
	const double& Data
)
{
	if ( ! m_Buffer )
	{
		return ( false );
	}
	try
	{
		switch ( m_EncodingType )
		{
		case NetPacket::XDR:
			return ( Write_XDR_double ( Data ) );
		case NetPacket::NET:
			return ( Write_NET_double ( Data ) );
		case NetPacket::NONE:
			return ( Write_NONE_double ( Data ) );
		default:
			return ( false );
		}
	}
	catch ( std::runtime_error& ex )
	{
		return ( false );
	}
	return ( false ); // never reached
} // NetPacket::Write_double ()

//////////////////////////////////////////////////////////////////////

