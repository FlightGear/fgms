// netpacket.hxx -  netpacket is a buffer for network packets
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

#ifndef NET_PACKET_HXX
#define NET_PACKET_HXX
#ifdef _MSC_VER
#pragma warning(disable: 4290)
#pragma warning(disable: 4101)
#endif
#include <stdexcept>
#include <cstring>
#include "encoding.hxx"

/**@ingroup fglib
 * @brief A class for network buffers.
 * It automatically manages the buffer, all you need to do is
 * read/write the data.
 */
class netpacket
{
	/// the internal buffer
	char* m_buffer;
	/// the capacity of the buffer
	uint32_t m_capacity;
	/// already used bytes in the buffer
	uint32_t m_bytes_in_use;
	/// we read at this position in the buffer
	uint32_t m_current_index;
	/// true if we allocated the space of this buffer
	bool m_self_allocated;
public:
	/// buffer encoding styles
	enum class ENCODING_TYPE
	{
		/// use xdr encoding
		XDR,
		/// convert numbers to network byte order
		NET,
		/// no encoding, just raw values
		NONE
	};
	netpacket () = delete;
	netpacket ( const netpacket& ) = delete;
	netpacket ( const uint32_t size );
	~netpacket ();
	void clear ();
	void start ();
	void reset ();
	void copy ( const netpacket& packet );
	bool set_index ( const uint32_t index );
	void set_encoding ( const ENCODING_TYPE encoding );
	ENCODING_TYPE get_encoding() const;
	uint32_t skip ( const uint32_t num_bytes );
	uint32_t available () const;
	uint32_t capacity () const;
	uint32_t bytes_used () const;
	bool is_available  ( const uint32_t size ) const;
	uint32_t remaining_data () const;
	inline uint32_t current_index () const;
	inline const char* buffer() const;
	void set_buffer ( const char* buffer, const uint32_t size );
	void set_used ( const uint32_t UsedBytes );
	bool write_int8   ( const int8_t&   data );
	bool write_uint8  ( const uint8_t&  data );
	bool write_int16  ( const int16_t&  data );
	bool write_uint16 ( const uint16_t& data );
	bool write_int32  ( const int32_t&  data );
	bool write_uint32 ( const uint32_t& data );
	bool write_int64  ( const int64_t&  data );
	bool write_uint64 ( const uint64_t& data );
	bool write_float  ( const float&    data );
	bool write_double ( const double&   data );
	bool write_opaque ( const void* data, const uint32_t size );
	bool write_string ( const std::string& str );
	bool write_c_string ( const char* str, uint32_t size=0 );
	int8_t   read_int8   ();
	uint8_t  read_uint8  ();
	int16_t  read_int16  ();
	uint16_t read_uint16 ();
	int32_t  read_int32  ();
	uint32_t read_uint32 ();
	int64_t  read_int64  ();
	uint64_t read_uint64 ();
	float    read_float  ();
	double   read_double ();
	void read_opaque ( void* buffer, uint32_t& size  );
	std::string read_string ();
	std::string read_c_string ();
	inline int8_t peek ( const int index ) const;
	inline int8_t peek () const;
	void encrypt ( const uint32_t key[4], const uint32_t offset=0 );
	void decrypt ( const uint32_t key[4], const uint32_t offset=0 );
private:
	ENCODING_TYPE m_encoding_type;
	/* read xdr encoded data */
	int8_t   read_xdr_int8   ();
	uint8_t  read_xdr_uint8  ();
	int16_t  read_xdr_int16  ();
	uint16_t read_xdr_uint16 ();
	int32_t  read_xdr_int32  ();
	uint32_t read_xdr_uint32 ();
	int64_t  read_xdr_int64  ();
	uint64_t read_xdr_uint64 ();
	float    read_xdr_float  ();
	double   read_xdr_double ();

	/* read NET encoded data */
	int8_t   read_net_int8   ();
	uint8_t  read_net_uint8  ();
	int16_t  read_net_int16  ();
	uint16_t read_net_uint16 ();
	int32_t  read_net_int32  ();
	uint32_t read_net_uint32 ();
	int64_t  read_net_int64  ();
	uint64_t read_net_uint64 ();
	float    read_net_float  ();
	double   read_net_double ();

	/* read unencoded data */
	int8_t   read_raw_int8   ();
	uint8_t  read_raw_uint8  ();
	int16_t  read_raw_int16  ();
	uint16_t read_raw_uint16 ();
	int32_t  read_raw_int32  ();
	uint32_t read_raw_uint32 ();
	int64_t  read_raw_int64  ();
	uint64_t read_raw_uint64 ();
	float    read_raw_float  ();
	double   read_raw_double ();

	/* write XDR encoded data */
	bool wite_xdr_int8   ( const int8_t&   data );
	bool wite_xdr_uint8  ( const uint8_t&  data );
	bool wite_xdr_int16  ( const int16_t&  data );
	bool wite_xdr_uint16 ( const uint16_t& data );
	bool wite_xdr_int32  ( const int32_t&  data );
	bool wite_xdr_uint32 ( const uint32_t& data );
	bool wite_xdr_int64  ( const int64_t&  data );
	bool wite_xdr_uint64 ( const uint64_t& data );
	bool wite_xdr_float  ( const float&    data );
	bool wite_xdr_double ( const double&   data );

	/* write NET encoded data */
	bool write_net_int8   ( const int8_t&   data );
	bool write_net_uint8  ( const uint8_t&  data );
	bool write_net_int16  ( const int16_t&  data );
	bool write_net_uint16 ( const uint16_t& data );
	bool write_net_int32  ( const int32_t&  data );
	bool write_net_uint32 ( const uint32_t& data );
	bool write_net_int64  ( const int64_t&  data );
	bool write_net_uint64 ( const uint64_t& data );
	bool write_net_float  ( const float&    data );
	bool write_net_double ( const double&   data );

	/* write unencoded data */
	bool write_raw_int8   ( const int8_t&   data );
	bool write_raw_uint8  ( const uint8_t&  data );
	bool write_raw_int16  ( const int16_t&  data );
	bool write_raw_uint16 ( const uint16_t& data );
	bool write_raw_int32  ( const int32_t&  data );
	bool write_raw_uint32 ( const uint32_t& data );
	bool write_raw_int64  ( const int64_t&  data );
	bool write_raw_uint64 ( const uint64_t& data );
	bool write_raw_float  ( const float&    data );
	bool write_raw_double ( const double&   data );

	enum { XTEA_BLOCK_SIZE = 8 };
	/// eXtended Tiny encryption Algorithm
	/// encode 64 data bits (v) with 128 bit key (k)
	void xtea_encipher ( unsigned int num_cycles, uint32_t v[2],
			     uint32_t const k[4]
			   );
	/// eXtended Tiny encryption Algorithm
	/// decode 64 data bits (v) with 128 bit key (k)
	void xtea_decipher ( unsigned int num_cycles, uint32_t v[2],
			     uint32_t const k[4]
			   );
}; // netpacket

/** API method
 * @return
 *   the current index
 */
uint32_t
netpacket::current_index
() const
{
	return ( m_current_index );
} // netpacket::current_index ()

/** API method
 * @return
 *   a pointer to the internal buffer
 */
const char*
netpacket::buffer
() const
{
	return m_buffer;
} // netpacket::buffer ()

/// Get byte at index
int8_t
netpacket::peek
(
	const int index
) const
{
	return m_buffer[index];
}

/// Get byte at current index
int8_t
netpacket::peek
() const
{
	return m_buffer[m_current_index];
}

#endif

