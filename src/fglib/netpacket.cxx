// netpacket.cxx -  netpacket is a buffer for network packets
//
// This file is part of fgms
//
// copyright (C) 2006  Oliver Schroeder
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

using namespace fgmp;

//////////////////////////////////////////////////////////////////////

/**
 * Create a buffer
 * @param size number of bytes reserved by this buffer
 */
netpacket::netpacket
(
	const uint32_t size
)
{
	if ( size == 0 )
		return;
	m_buffer         = new char[size];
	m_capacity       = size;
	m_current_index  = 0;
	m_bytes_in_use   = 0;
	m_self_allocated = true;
	m_encoding_type  = ENCODING_TYPE::XDR;
	clear();  // FIXME: probably not needed
} // netpacket::netpacket ( uint32_t size )

//////////////////////////////////////////////////////////////////////

/**
 * destroy the allocated buffer
 */
netpacket::~netpacket
()
{
	if ( ( m_buffer ) && ( m_self_allocated ) )
		delete[] m_buffer;
} // netpacket::~netpacket()

//////////////////////////////////////////////////////////////////////

/**
 * explicitly clear the buffer, all bytes are set to zero
 */
void
netpacket::clear
()
{
	if ( m_buffer )
		memset ( m_buffer, 0, m_capacity );
	m_current_index = 0;
	m_bytes_in_use   = 0;
} // netpacket::clear()

//////////////////////////////////////////////////////////////////////

/**
 * Set the (internal) index to the start of the buffer.
 * Call this before reading from the buffer.
 */
void
netpacket::start
()
{
	m_current_index = 0;
} // netpacket::start ()

//////////////////////////////////////////////////////////////////////

/**
 * Set the (internal) index to the start of the buffer.
 * Assume that there are no bytes used yet.
 */
void
netpacket::reset
()
{
	m_current_index = 0;
	m_bytes_in_use  = 0;
} // netpacket::reset ()

//////////////////////////////////////////////////////////////////////

/**
 * make *this a copy of @packet
 */
void
netpacket::copy
(
	const netpacket& packet
)
{
	if ( m_capacity != packet.m_capacity )
	{
		delete m_buffer;
		m_capacity = packet.m_capacity;
		m_buffer  = new char[m_capacity];
	}
	memcpy ( m_buffer, packet.m_buffer, m_capacity );
	m_capacity      = packet.m_capacity;
	m_bytes_in_use  = packet.m_bytes_in_use;
	m_current_index = packet.m_current_index;
} // netpacket::copy ()

//////////////////////////////////////////////////////////////////////

/**
 * Set the (internal) index to the specified index. Use this method
 * to start reading from the buffer at the specified index.
 * @param index
 *  Set internal index to this.
 * @return true
 *   index was successfully set
 * @return false
 *   index was out of range
 * @see skip
 */
bool
netpacket::set_index
(
	const uint32_t index
)
{
	if ( ( ! m_buffer ) || ( index > m_bytes_in_use ) )
		return false;
	m_current_index = index;
	return true;
} // netpacket::set_index ()

//////////////////////////////////////////////////////////////////////

/**
 * set which type of encoding this buffer uses
 * @see ENCODING_TYPE
 */
void
netpacket::set_encoding
(
	const ENCODING_TYPE encoding
)
{
	m_encoding_type = encoding;
} // netpacket::set_encoding()

//////////////////////////////////////////////////////////////////////

/**
 * return type of encoding this buffer uses
 * @see ENCODING_TYPE
 */
netpacket::ENCODING_TYPE
netpacket::get_encoding
() const
{
	return m_encoding_type;
} // netpacket::get_encoding()

//////////////////////////////////////////////////////////////////////

/**
 * Use this method to skip the given number of bytes. You can
 * spare out some bytes, and write/read them later.
 * @return >0 Current index, success
 * @return 0  Unable to reserve requested number of bytes.
 * @see set_index
 */
uint32_t
netpacket::skip
(
	const uint32_t num_bytes
)
{
	uint32_t index = m_current_index + num_bytes;
	if ( ( !m_buffer ) || ( index > m_capacity ) )
		return 0;
	m_current_index = index;
	if ( m_current_index > m_bytes_in_use )
		m_bytes_in_use = m_current_index;
	return m_current_index;
} // netpacket::skip ()

//////////////////////////////////////////////////////////////////////

/**
 * @return the remaining free storage space in bytes
 */
uint32_t
netpacket::available
() const
{
	return m_capacity - m_current_index;
} // netpacket::available ()

//////////////////////////////////////////////////////////////////////

/** API method:
 * @return the overall capacity of this buffer
 */
uint32_t
netpacket::capacity
() const
{
	return m_capacity;
} // netpacket::capacity ()

//////////////////////////////////////////////////////////////////////

/** API method:
 *  @return the number of bytes used in this buffer
 */
uint32_t
netpacket::bytes_used
() const
{
	return m_bytes_in_use;
} // netpacket::bytes_used ()

//////////////////////////////////////////////////////////////////////

/** API method:
 * check if the buffer has a specified amount left
 * free for storage
 * @param size
 *   the number of bytes, which should be free
 * @return true
 *   the number of bytes are free for storage
 * @return false
 *   not enough bytes left for storage
 */
bool
netpacket::is_available
(
	const uint32_t size
) const
{
	if ( available() < size )
		return false;
	return true;
} // netpacket::is_available  ( const uint32_t size )

//////////////////////////////////////////////////////////////////////

/** API method:
 * check if there is still data to read
 * @return
 *   the number of bytes left for reading
 */
uint32_t
netpacket::remaining_data
() const
{
	return m_bytes_in_use - m_current_index;
} // netpacket::remaining_data ()

//////////////////////////////////////////////////////////////////////

/** API method
 * use the specified buffer
 * @param buffer
 *   Pointer to the buffer to use.
 * @param size
 *   The number of (used) bytes in the buffer
 */
void
netpacket::set_buffer
(
	const char* buffer, const uint32_t size
)
{
	if ( ( ! buffer ) || ( size == 0 ) )
		return;
	if ( ( m_buffer ) && ( m_self_allocated ) )
		delete[] m_buffer;
	m_self_allocated = false;
	m_buffer	= ( char* ) buffer;
	m_capacity	= size;
	m_current_index	= 0;
	m_bytes_in_use	= size;
} // netpacket::set_buffer ()

//////////////////////////////////////////////////////////////////////

/** API method:
 * Set number of used bytes of this netpacket. Needed if
 * the content is set from outside
 * @param UsedBytes
 *   the number of bytes already used by this buffer
 */
void
netpacket::set_used
(
	const uint32_t UsedBytes
)
{
	m_bytes_in_use = UsedBytes;
} // netpacket::set_used ()

//////////////////////////////////////////////////////////////////////

/** @name Writers
 *
 * Be aware that: \n
 *   long myvar = 1; \n
 *   buffer->Write<long> (myvar); \n
 * isn't portable, as the type 'long' (as others) may vary in size. \n
 * Always use uintXX_t! \n
 * @usage buffer->Write<uint32_t> (MyVar);
 *
 */
/** @{ */

//////////////////////////////////////////////////////////////////////

/**
 * copy arbitrary data to the buffer \n
 * Opaque data has the form |length|data \n
 * !!! Remember: if you want to store a c-str, write strlen+1 bytes \n
 * !!! in order to write the \0 character at the end \n
 * @param data
 *   pointer to the data. The data is supposed to be raw, no
 *   byte ordering is applied to it.
 * @param size
 *   number of bytes to copy
 * @param align_bytes
 *   align data multiples of this
 */
bool
netpacket::write_opaque
(
	const void* data,
	const uint32_t size
)
{
	if ( ! m_buffer )
	{
		// no buffer, so nothing to do but something is wrong
		throw std::runtime_error (
		  "netpacket::write_opaque (): no buffer" );
	}
	if ( ( ! data ) || ( size == 0 ) )
	{
		// no data, so we are done
		return true;
	}
	// round data size to a multiple of XDR datasize
	int rndup = 0;
	if ( m_encoding_type == ENCODING_TYPE::XDR )
	{
		int align_bytes = sizeof ( xdr_data_t );
		rndup = size % align_bytes;
		if ( rndup )
			rndup = align_bytes - rndup;
	}
	if ( available() < size+rndup )
	{
		// not enough space left for data
		return false;
	}
	write_uint32 ( size+rndup );
	char* index = ( char* ) m_buffer + m_current_index;
	memcpy ( index, data, size );
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	if ( rndup )
	{
		memset ( m_buffer+m_current_index, 0, rndup );
		if ( m_current_index == m_bytes_in_use )
		{
			m_current_index += rndup;
			m_bytes_in_use += rndup;
		}
	}
	return true;
} // netpacket::write_opaque ()

//////////////////////////////////////////////////////////////////////

/// Write a string
bool
netpacket::write_string
(
	const std::string& str
)
{
	return write_opaque ( ( const void* ) str.c_str(), str.size() + 1 );
} // write_string ()

//////////////////////////////////////////////////////////////////////

/** compatabilty routine
 *
 * write a raw c-string to the buffer. if size!=0
 * write only size characters
 */
bool
netpacket::write_c_string
(
	const char* str,
	uint32_t size
)
{
	if ( ! m_buffer )
	{
		// no buffer, so nothing to do but something is wrong
		throw std::runtime_error (
		  "netpacket::write_c_string (): no buffer" );
	}
	if ( ( ! str ) || ( size == 0 ) )
		return true; // no data, so we are done
	uint32_t len = size;
	if ( size == 0 )
		len = strlen ( str );
	if ( len == 0 )
		return true; // no data, so we are done
	if ( available() < len )
		return false; // not enough space left for data
	char* index = ( char* ) m_buffer + m_current_index;
	memcpy ( index, str, len );
	index += len;
	*index = '0';
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += len;
		m_bytes_in_use += len;
	}
	return true;
} // netpacket::write_c_string ()

//////////////////////////////////////////////////////////////////////

/// write a signed 8 bit value to the buffer
bool
netpacket::write_int8
(
	const int8_t& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_int8 ( data );
		case ENCODING_TYPE::NET:
			return write_net_int8 ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_int8 ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; // never reached
} // netpacket::write_int8 ()

//////////////////////////////////////////////////////////////////////

/// write an unsigned 8 bit value to the buffer
bool
netpacket::write_uint8
(
	const uint8_t& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_uint8 ( data );
		case ENCODING_TYPE::NET:
			return write_net_uint8 ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_uint8 ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; // never reached
} // netpacket::write_uint8 ()

//////////////////////////////////////////////////////////////////////

/// write a signed 16 bit value to the buffer
bool
netpacket::write_int16
(
	const int16_t& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_int16 ( data );
		case ENCODING_TYPE::NET:
			return write_net_int16 ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_int16 ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; //  never reached
} // netpacket::write_int16 ()

//////////////////////////////////////////////////////////////////////

/// write an unsigned 16 bit value to the buffer
bool
netpacket::write_uint16
(
	const uint16_t& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_uint16 ( data );
		case ENCODING_TYPE::NET:
			return write_net_uint16 ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_uint16 ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; // never reached
} // netpacket::write_uint16 ()

//////////////////////////////////////////////////////////////////////

/// write a signed 32 bit value to the buffer
bool
netpacket::write_int32
(
	const int32_t& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_int32 ( data );
		case ENCODING_TYPE::NET:
			return write_net_int32 ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_int32 ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; // never reached
} // netpacket::write_int32 ()

//////////////////////////////////////////////////////////////////////

/// write an unsigned 32 bit value to the buffer
bool
netpacket::write_uint32
(
	const uint32_t& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_uint32 ( data );
		case ENCODING_TYPE::NET:
			return write_net_uint32 ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_uint32 ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; // never reached
} // netpacket::write_uint32 ()

//////////////////////////////////////////////////////////////////////

/// write a signed 64 bit value to the buffer
bool
netpacket::write_int64
(
	const int64_t& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_int64 ( data );
		case ENCODING_TYPE::NET:
			return write_net_int64 ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_int64 ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; // never reached
} // netpacket::write_int64 ()

//////////////////////////////////////////////////////////////////////

/// write an unsigned 64 bit value to the buffer
bool
netpacket::write_uint64
(
	const uint64_t& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_uint64 ( data );
		case ENCODING_TYPE::NET:
			return write_net_uint64 ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_uint64 ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; // never reached
} // netpacket::write_uint64 ()

//////////////////////////////////////////////////////////////////////

/// write a float value to the buffer
bool
netpacket::write_float
(
	const float& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_float ( data );
		case ENCODING_TYPE::NET:
			return write_net_float ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_float ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; // never reached
} // netpacket::write_float ()

//////////////////////////////////////////////////////////////////////

/// write a double value to the buffer
bool
netpacket::write_double
(
	const double& data
)
{
	if ( ! m_buffer )
		return false;
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return wite_xdr_double ( data );
		case ENCODING_TYPE::NET:
			return write_net_double ( data );
		case ENCODING_TYPE::NONE:
			return write_raw_double ( data );
		default:
			return false;
		}
	}
	catch ( std::runtime_error& ex )
	{
		return false;
	}
	return false; // never reached
} // netpacket::write_double ()

//////////////////////////////////////////////////////////////////////

/** @} */
/** @name Readers
 *
 * Be aware that: \n
 *   long myvar = 1; \n
 *   buffer->Write<long> (myvar); \n
 * isn't portable, as the type 'long' (as others) may vary in size. \n
 * Always use uintXX_t! \n
 * @usage buffer->Write<uint32_t> (MyVar);
 *
 */
/** @{ */

//////////////////////////////////////////////////////////////////////

/**
 * Return a pointer to arbitrary data,
 * increase internal pointer by 'size' bytes. \n
 * Opaque data has the form |length|data
 * @param size
 *   modified by this method. The number of bytes.
 * @return a pointer to the data.
 *   The data is supposed to be raw, no byte ordering is
 *   applied to it.
 */
void
netpacket::read_opaque
(
	void* buffer,
	uint32_t& size
)
{
	uint32_t data_available = remaining_data();
	if ( ( ! m_buffer ) || ( data_available == 0 ) )
	{
		throw std::runtime_error (
		  "netpacket::read_opaque (): no more data to read" );
	}
	size = read_uint32 ();
	if ( data_available < size )
	{
		throw std::runtime_error (
		  "netpacket::read_opaque (): invalid length" );
	}
	char* ret = ( char* )  &m_buffer[m_current_index];
	memcpy ( buffer, ret, size );
	m_current_index += size;
} // netpacket::read_opaque ()

//////////////////////////////////////////////////////////////////////

/// Read a string from the buffer. \n
/// Like read_opaque(), but automatically inserts the length of the string
std::string
netpacket::read_string
()
{
	uint32_t data_available = remaining_data();
	if ( ( ! m_buffer ) || ( data_available == 0 ) )
	{
		throw std::runtime_error (
		  "netpacket::read_string (): no more data to read" );
	}
	uint32_t size = read_uint32 ();
	if ( data_available < size )
	{
		throw std::runtime_error (
		  "netpacket::read_string (): invalid length" );
	}
	std::string ret = ( char* )  &m_buffer[m_current_index];
	m_current_index += size;
	return ret;
} // netpacket::read_string ()

//////////////////////////////////////////////////////////////////////

/** compatabilty routine
 *
 * read a raw c-string from the buffer.
 */
std::string
netpacket::read_c_string
()
{
	if ( ( ! m_buffer ) || ( remaining_data() == 0 ) )
	{
		throw std::runtime_error (
		  "netpacket::read_c_string (): no more data to read" );
	}
	std::string ret = ( char* )  &m_buffer[m_current_index];
	int len = ret.size () + 1;
	m_current_index += len;
	return ret;
} // netpacket::read_c_string ()

//////////////////////////////////////////////////////////////////////

int8_t
netpacket::read_int8
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_int8 (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_int8 ();
		case ENCODING_TYPE::NET:
			return read_net_int8 ();
		case ENCODING_TYPE::NONE:
			return read_raw_int8 ();
		default:
			throw std::runtime_error (
			  "netpacket::read_int8 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_int8 ()

//////////////////////////////////////////////////////////////////////

uint8_t
netpacket::read_uint8
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_uint8 (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_uint8 ();
		case ENCODING_TYPE::NET:
			return read_net_uint8 ();
		case ENCODING_TYPE::NONE:
			return read_raw_uint8 ();
		default:
			throw std::runtime_error (
			  "netpacket::read_uint8 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_uint8 ()

//////////////////////////////////////////////////////////////////////

int16_t
netpacket::read_int16
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_int16 (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_int16 ();
		case ENCODING_TYPE::NET:
			return read_net_int16 ();
		case ENCODING_TYPE::NONE:
			return read_raw_int16 ();
		default:
			throw std::runtime_error (
			  "netpacket::read_int16 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_int16 ()

//////////////////////////////////////////////////////////////////////

uint16_t
netpacket::read_uint16
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_uint16 (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_uint16 ();
		case ENCODING_TYPE::NET:
			return read_net_uint16 ();
		case ENCODING_TYPE::NONE:
			return read_raw_uint16 ();
		default:
			throw std::runtime_error (
			  "netpacket::read_uint16 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_uint16 ()

//////////////////////////////////////////////////////////////////////

int32_t
netpacket::read_int32
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_int32 (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_int32 ();
		case ENCODING_TYPE::NET:
			return read_net_int32 ();
		case ENCODING_TYPE::NONE:
			return read_raw_int32 ();
		default:
			throw std::runtime_error (
			  "netpacket::read_int32 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_int32 ()

//////////////////////////////////////////////////////////////////////

uint32_t
netpacket::read_uint32
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_uint32 (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_uint32 ();
		case ENCODING_TYPE::NET:
			return read_net_uint32 ();
		case ENCODING_TYPE::NONE:
			return read_raw_uint32 ();
		default:
			throw std::runtime_error (
			  "netpacket::read_uint32 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_uint32 ()

//////////////////////////////////////////////////////////////////////

int64_t
netpacket::read_int64
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_int64 (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_int64 ();
		case ENCODING_TYPE::NET:
			return read_net_int64 ();
		case ENCODING_TYPE::NONE:
			return read_raw_int64 ();
		default:
			throw std::runtime_error (
			  "netpacket::read_int64 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_int64 ()

//////////////////////////////////////////////////////////////////////

uint64_t
netpacket::read_uint64
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_uint64 (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_uint64 ();
		case ENCODING_TYPE::NET:
			return read_net_uint64 ();
		case ENCODING_TYPE::NONE:
			return read_raw_uint64 ();
		default:
			throw std::runtime_error (
			  "netpacket::read_uint64 (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_uint64 ()

//////////////////////////////////////////////////////////////////////

float
netpacket::read_float
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_float (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_float ();
		case ENCODING_TYPE::NET:
			return read_net_float ();
		case ENCODING_TYPE::NONE:
			return read_raw_float ();
		default:
			throw std::runtime_error (
			  "netpacket::read_float (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_float ()

//////////////////////////////////////////////////////////////////////

double
netpacket::read_double
()
{
	if ( ! m_buffer )
	{
		throw std::runtime_error (
		  "netpacket::read_double (): no buffer" );
	}
	try
	{
		switch ( m_encoding_type )
		{
		case ENCODING_TYPE::XDR:
			return read_xdr_double ();
		case ENCODING_TYPE::NET:
			return read_net_double ();
		case ENCODING_TYPE::NONE:
			return read_raw_double ();
		default:
			throw std::runtime_error (
			  "netpacket::read_double (): unknown encoding type" );
		}
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return 0; // keep compilers happy
} // read_double ()

//////////////////////////////////////////////////////////////////////

/** @} */

//////////////////////////////////////////////////////////////////////

/// Read a signed 8 bit value from the buffer
int8_t
netpacket::read_xdr_int8
()
{
	uint32_t data;
	try
	{
		data = read_raw_int32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_int8 ( data );
} // netpacket::read_xdr_int8 ()

//////////////////////////////////////////////////////////////////////

/// Read an unsigned 8 bit value from the buffer
uint8_t
netpacket::read_xdr_uint8
()
{
	uint32_t data;
	try
	{
		data = read_raw_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_uint8 ( data );
} // netpacket::read_xdr_uint8 ()

//////////////////////////////////////////////////////////////////////

/// Read a signed 16 bit value from the buffer
int16_t
netpacket::read_xdr_int16
()
{
	uint32_t data;
	try
	{
		data = read_raw_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_int16 ( data );
} // netpacket::read_xdr_int16 ()

//////////////////////////////////////////////////////////////////////

/// Read an unsigned 16 bit value from the buffer
uint16_t
netpacket::read_xdr_uint16
()
{
	uint32_t data;
	try
	{
		data = read_raw_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_uint16 ( data );
} // netpacket::read_xdr_uint16 ()

//////////////////////////////////////////////////////////////////////

/// Read a signed 32 bit value from the buffer
int32_t
netpacket::read_xdr_int32
()
{
	uint32_t data;
	try
	{
		data = read_raw_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_int32 ( data );
} // netpacket::read_xdr_int32 ()

//////////////////////////////////////////////////////////////////////

/// Read an unsigned 32 bit value from the buffer
uint32_t
netpacket::read_xdr_uint32
()
{
	uint32_t data;
	try
	{
		data = read_raw_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_uint32 ( data );
} // netpacket::read_xdr_uint32 ()

//////////////////////////////////////////////////////////////////////

/// Read a signed 64 bit value from the buffer
int64_t
netpacket::read_xdr_int64
()
{
	uint64_t data;
	try
	{
		data = read_raw_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_int64 ( data );
} // netpacket::read_xdr_int64 ()

//////////////////////////////////////////////////////////////////////

/// Read an unsigned 64 bit value from the buffer
uint64_t
netpacket::read_xdr_uint64
()
{
	uint64_t data;
	try
	{
		data = read_raw_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_uint64 ( data );
} // netpacket::read_xdr_uint64 ()

//////////////////////////////////////////////////////////////////////

/// Read a float value from the buffer
float
netpacket::read_xdr_float
()
{
	uint32_t data;
	try
	{
		data = read_raw_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_float ( data );
} // netpacket::read_xdr_float ()

//////////////////////////////////////////////////////////////////////

/// Read a double value from the buffer
double
netpacket::read_xdr_double
()
{
	uint64_t data;
	try
	{
		data = read_raw_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return xdr_decode_double ( data );
} // netpacket::read_xdr_double ()

//////////////////////////////////////////////////////////////////////

int8_t
netpacket::read_net_int8
()
{
	int8_t data;
	try
	{
		data = read_raw_int8 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_uint8 ( data );
} // netpacket::read_net_int8 ()

//////////////////////////////////////////////////////////////////////

uint8_t
netpacket::read_net_uint8
()
{
	uint8_t data;
	try
	{
		data = read_raw_uint8 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_uint8 ( data );
} // netpacket::read_net_uint8 ()

//////////////////////////////////////////////////////////////////////

int16_t
netpacket::read_net_int16
()
{
	int16_t data;
	try
	{
		data = read_raw_uint16 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_int16 ( data );
} // netpacket::read_net_int16 ()

//////////////////////////////////////////////////////////////////////

uint16_t
netpacket::read_net_uint16
()
{
	uint16_t data;
	try
	{
		data = read_raw_uint16 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_uint16 ( data );
} // netpacket::read_net_uint16 ()

//////////////////////////////////////////////////////////////////////

int32_t
netpacket::read_net_int32
()
{
	int32_t data;
	try
	{
		data = read_raw_int32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_int32 ( data );
} // netpacket::read_net_int32 ()

//////////////////////////////////////////////////////////////////////

uint32_t
netpacket::read_net_uint32
()
{
	uint32_t data;
	try
	{
		data = read_raw_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_uint32 ( data );
} // netpacket::read_net_uint32 ()

//////////////////////////////////////////////////////////////////////

int64_t
netpacket::read_net_int64
()
{
	int64_t data;
	try
	{
		data = read_raw_int64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_int64 ( data );
} // netpacket::read_net_int64 ()

//////////////////////////////////////////////////////////////////////

uint64_t
netpacket::read_net_uint64
()
{
	uint64_t data;
	try
	{
		data = read_raw_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_uint64 ( data );
} // netpacket::read_net_int64 ()

//////////////////////////////////////////////////////////////////////

float
netpacket::read_net_float
()
{
	uint32_t data;
	try
	{
		data = read_raw_uint32 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_float ( data );
} // netpacket::read_net_float ()

//////////////////////////////////////////////////////////////////////

double
netpacket::read_net_double
()
{
	uint64_t data;
	try
	{
		data = read_raw_uint64 ();
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return net_decode_double ( data );
} // netpacket::read_net_double ()

//////////////////////////////////////////////////////////////////////

int8_t
netpacket::read_raw_int8
()
{
	unsigned int size;
	size = sizeof ( int8_t );
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_int8 (): no more data to read" );
	}
	int8_t* data = ( int8_t* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_int8 ()

//////////////////////////////////////////////////////////////////////

uint8_t
netpacket::read_raw_uint8
()
{
	unsigned int size;
	size = sizeof ( uint8_t );
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_uint8 (): no more data to read" );
	}
	uint8_t* data = ( uint8_t* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_uint8 ()

//////////////////////////////////////////////////////////////////////

int16_t
netpacket::read_raw_int16
()
{
	unsigned int size;
	size = sizeof ( int16_t );
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_int16 (): no more data to read" );
	}
	int16_t* data = ( int16_t* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_int16 ()

//////////////////////////////////////////////////////////////////////

uint16_t
netpacket::read_raw_uint16
()
{
	unsigned int size;
	size = sizeof ( uint16_t );
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_uint16 (): no more data to read" );
	}
	uint16_t* data = ( uint16_t* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_uint16 ()

//////////////////////////////////////////////////////////////////////

int32_t
netpacket::read_raw_int32
()
{
	unsigned int size;
	size = sizeof ( int32_t );
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_int32 (): no more data to read" );
	}
	int32_t* data = ( int32_t* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_int32  ()

//////////////////////////////////////////////////////////////////////

uint32_t
netpacket::read_raw_uint32
()
{
	unsigned int size;
	size = sizeof ( uint32_t );
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_uint32 (): no more data to read" );
	}
	uint32_t* data = ( uint32_t* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_uint32  ()

//////////////////////////////////////////////////////////////////////

int64_t
netpacket::read_raw_int64
()
{
	unsigned int size;
	size = sizeof ( int64_t );
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_int64 (): no more data to read" );
	}
	int64_t* data = ( int64_t* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_int64  ()

//////////////////////////////////////////////////////////////////////

uint64_t
netpacket::read_raw_uint64
()
{
	unsigned int size;
	size = sizeof ( uint64_t );
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_uint64 (): no more data to read" );
	}
	uint64_t* data = ( uint64_t* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_uint64  ()

//////////////////////////////////////////////////////////////////////

float
netpacket::read_raw_float
()
{
	unsigned int size;
	size = sizeof ( float );
	if ( size > sizeof ( xdr_data_t ) )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_float (): wrong size of float" );
	}
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_float (): no more data to read" );
	}
	float* data = ( float* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_float  ()

//////////////////////////////////////////////////////////////////////

double
netpacket::read_raw_double
()
{
	unsigned int size;
	size = sizeof ( double );
	if ( size != sizeof ( xdr_data2_t ) )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_double (): wrong size of double" );
	}
	if ( size > remaining_data () )
	{
		throw std::runtime_error (
		  "netpacket::read_raw_double (): no more data to read" );
	}
	double* data = ( double* ) &m_buffer[m_current_index];
	m_current_index += size;
	return *data;
} // netpacket::read_raw_double  ()

//////////////////////////////////////////////////////////////////////
bool
netpacket::wite_xdr_int8
(
	const int8_t& data
)
{
	try
	{
		write_raw_uint32 ( xdr_encode_int8 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_int8 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::wite_xdr_uint8
(
	const uint8_t& data
)
{
	try
	{
		write_raw_uint32 ( xdr_encode_uint8 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_uint8 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::wite_xdr_int16
(
	const int16_t& data
)
{
	try
	{
		write_raw_uint32 ( xdr_encode_int16 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_int16 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::wite_xdr_uint16
(
	const uint16_t& data
)
{
	try
	{
		write_raw_uint32 ( xdr_encode_uint16 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_uint16 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::wite_xdr_int32
(
	const int32_t& data
)
{
	try
	{
		write_raw_uint32 ( xdr_encode_int32 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_int32 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::wite_xdr_uint32
(
	const uint32_t& data
)
{
	try
	{
		write_raw_uint32 ( xdr_encode_uint32 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_uint32 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::wite_xdr_int64
(
	const int64_t& data
)
{
	try
	{
		write_raw_uint64 ( xdr_encode_int64 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_int64 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::wite_xdr_uint64
(
	const uint64_t& data
)
{
	try
	{
		write_raw_uint64 ( xdr_encode_uint64 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_uint64 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::wite_xdr_float
(
	const float& data
)
{
	unsigned int size;
	size = sizeof ( xdr_data_t );
	if ( size != sizeof ( float ) )
	{
		throw std::runtime_error (
		  "netpacket::wite_xdr_float (): wrong size of float" );
	}
	try
	{
		write_raw_uint32 ( xdr_encode_float ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_float ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::wite_xdr_double
(
	const double& data
)
{
	unsigned int size;
	size = sizeof ( xdr_data2_t );
	if ( size != sizeof ( double ) )
	{
		throw std::runtime_error (
		  "netpacket::wite_xdr_double (): wrong size of double" );
	}
	try
	{
		write_raw_uint64 ( xdr_encode_double ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::wite_xdr_double ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_int8
(
	const int8_t& data
)
{
	try
	{
		write_raw_int8 ( net_encode_int8 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_int8 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_uint8
(
	const uint8_t& data
)
{
	try
	{
		write_raw_uint8 ( net_encode_uint8 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_uint8 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_int16
(
	const int16_t& data
)
{
	try
	{
		write_raw_int16 ( net_encode_int16 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_int16 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_uint16
(
	const uint16_t& data
)
{
	try
	{
		write_raw_uint16 ( net_encode_uint16 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_uint16 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_int32
(
	const int32_t& data
)
{
	try
	{
		write_raw_int32 ( net_encode_int32 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_int32 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_uint32
(
	const uint32_t& data
)
{
	try
	{
		write_raw_uint32 ( net_encode_uint32 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_uint32 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_int64
(
	const int64_t& data
)
{
	try
	{
		write_raw_int64 ( net_encode_int64 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_int64 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_uint64
(
	const uint64_t& data
)
{
	try
	{
		write_raw_uint64 ( net_encode_uint64 ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_uint64 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_float
(
	const float& data
)
{
	try
	{
		write_raw_uint32 ( net_encode_float ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_float ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_net_double
(
	const double& data
)
{
	try
	{
		write_raw_uint64 ( net_encode_double ( data ) );
	}
	catch ( std::runtime_error& ex )
	{
		throw ex;
	}
	return true;
} // netpacket::write_net_double ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_int8
(
	const int8_t& data
)
{
	unsigned int size;
	size = sizeof ( int8_t );
	if ( available() < size )
		return false;
	int8_t* index = ( int8_t* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_int8 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_uint8
(
	const uint8_t& data
)
{
	unsigned int size;
	size = sizeof ( uint8_t );
	if ( available() < size )
		return false;
	uint8_t* index = ( uint8_t* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_uint8 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_int16
(
	const int16_t& data
)
{
	unsigned int size;
	size = sizeof ( int16_t );
	if ( available() < size )
		return false;
	int16_t* index = ( int16_t* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_int16 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_uint16
(
	const uint16_t& data
)
{
	unsigned int size;
	size = sizeof ( uint16_t );
	if ( available() < size )
		return false;
	uint16_t* index = ( uint16_t* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_uint16 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_int32
(
	const int32_t& data
)
{
	unsigned int size;
	size = sizeof ( int32_t );
	if ( available() < size )
		return false;
	int32_t* index = ( int32_t* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_int32 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_uint32
(
	const uint32_t& data
)
{
	unsigned int size;
	size = sizeof ( uint32_t );
	if ( available() < size )
		return false;
	uint32_t* index = ( uint32_t* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_uint32 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_int64
(
	const int64_t& data
)
{
	unsigned int size;
	size = sizeof ( int64_t );
	if ( available() < size )
		return false;
	int64_t* index = ( int64_t* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_int64 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_uint64
(
	const uint64_t& data
)
{
	unsigned int size;
	size = sizeof ( uint64_t );
	if ( available() < size )
		return false;
	uint64_t* index = ( uint64_t* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_uint64 ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_float
(
	const float& data
)
{
	unsigned int size;
	size = sizeof ( float );
	if ( available() < size )
		return false;
	float* index = ( float* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_float ()

//////////////////////////////////////////////////////////////////////

bool
netpacket::write_raw_double
(
	const double& data
)
{
	unsigned int size;
	size = sizeof ( double );
	if ( available() < size )
		return false;
	double* index = ( double* ) &m_buffer[m_current_index];
	*index = data;
	if ( m_current_index == m_bytes_in_use )
	{
		m_current_index += size;
		m_bytes_in_use += size;
	}
	return true;
} // netpacket::write_raw_double ()

//////////////////////////////////////////////////////////////////////

void
netpacket::encrypt
(
	const uint32_t key[4],
	const uint32_t offset
)
{
	uint32_t Len = m_bytes_in_use;
	uint32_t Mod = Len % XTEA_BLOCK_SIZE;
	if ( ( ! m_buffer ) || ( m_bytes_in_use == 0 ) )
	{
		LOG(fglog::prio::DEBUG,
		  "netpacket::encrypt() - nothing to encrypt!" );
		return;
	}
	if ( m_bytes_in_use <= offset )
	{
		LOG ( fglog::prio::DEBUG,
		  "netpacket::Enrypt() - offset>Used Bytes!" );
		return;
	}
	for ( uint32_t i=offset; i<=Len-XTEA_BLOCK_SIZE; i+=XTEA_BLOCK_SIZE )
		xtea_encipher ( 32, ( uint32_t* ) &m_buffer[i], key );
	if ( Mod )
	{
		uint32_t Off = Len - XTEA_BLOCK_SIZE;
		xtea_encipher ( 32, ( uint32_t* ) &m_buffer[Off], key );
	}
} // netpacket::encrypt

//////////////////////////////////////////////////////////////////////

void
netpacket::decrypt
(
	const uint32_t key[4],
	const uint32_t offset
)
{
	uint32_t Len = m_bytes_in_use;
	uint32_t Mod = Len % XTEA_BLOCK_SIZE;
	if ( ( ! m_buffer ) || ( m_bytes_in_use == 0 ) )
	{
		LOG ( fglog::prio::DEBUG,
		  "netpacket::decrypt() - nothing to encrypt!" );
		return;
	}
	if ( m_bytes_in_use <= offset )
	{
		LOG ( fglog::prio::DEBUG,
		  "netpacket::decrypt() - offset>Used Bytes!" );
		return;
	}
	if ( Mod )
	{
		uint32_t Off = Len - XTEA_BLOCK_SIZE;
		xtea_decipher ( 32, ( uint32_t* ) &m_buffer[Off], key );
	}
	for ( uint32_t i=offset; i<=Len-XTEA_BLOCK_SIZE; i+=XTEA_BLOCK_SIZE )
		xtea_decipher ( 32, ( uint32_t* ) &m_buffer[i], key );
} // netpacket::decrypt

//////////////////////////////////////////////////////////////////////

//
// eXtended Tiny encryption Algorithm
// originally developed by David Wheeler and Roger Needham at the Computer
// Laboratory of Cambridge University
void
netpacket::xtea_encipher
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
} // netpacket::xtea_encipher ()

//////////////////////////////////////////////////////////////////////

void
netpacket::xtea_decipher
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
} // netpacket::xtea_decipher ()

//////////////////////////////////////////////////////////////////////

