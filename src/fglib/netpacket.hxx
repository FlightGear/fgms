// netpacket.hxx -  NetPacket is a buffer for network packets
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

#ifndef NET_PACKET_HXX
#define NET_PACKET_HXX
#ifdef _MSC_VER
#pragma warning(disable: 4290)
#pragma warning(disable: 4101)
#endif
#include <stdexcept>
#include <cstring>
#include "encoding.hxx"

/** A class for network buffers.
 * It automatically manages the buffer, all you need to do is
 * read/write the data.
 */
class NetPacket
{
	/// the internal buffer
	char* m_Buffer;
	/// the capacity of the buffer
	uint32_t m_Capacity;
	/// already used bytes in the buffer
	uint32_t m_BytesInUse;
	/// we read at this position in the buffer
	uint32_t m_CurrentIndex;
	/// true if we allocated the space of this buffer
	bool m_SelfAllocated;
public:
	/// buffer encoding styles
	enum BUFFER_ENCODING_TYPE
	{
		/// use xdr encoding
		XDR,
		/// convert numbers to network byte order
		NET,
		/// no encoding, just raw values
		NONE
	};
	/**
	 * Create a buffer
	 * @param Size
	 *   number of bytes reserved by this buffer
	 */
	NetPacket ( const uint32_t Size );
	/**
	 * destroy the allocated buffer
	 */
	~NetPacket();
	/**
	 * explicitly clear the buffer, all bytes are set to zero
	 */
	void Clear();
	/**
	 * Set the (internal) index to the start of the buffer.
	 * Call this before reading from the buffer.
	 */
	void Start();
	/**
	 * Set the (internal) index to the start of the buffer.
	 * Assume that there are no bytes used yet.
	 */
	void Reset();
	/**
	 * make *this a copy of @Packet
	 */
	void Copy ( const NetPacket& Packet );
	/**
	 * Set the (internal) index to the specified index. Use this method
	 * to start reading from the buffer at the specified index.
	 * @param Index
	 *  Set internal index to this.
	 * @return true
	 *   Index was successfully set
	 * @return false
	 *   Index was out of range
	 * @see Skip
	 */
	bool SetIndex ( const uint32_t Index );
	/**
	 * set which type of encoding this buffer uses
	 * @see BUFFER_ENCODING_TYPE
	 */
	void SetEncoding ( const BUFFER_ENCODING_TYPE encoding );
	/**
	 * return type of encoding this buffer uses
	 * @see BUFFER_ENCODING_TYPE
	 */
	BUFFER_ENCODING_TYPE GetEncoding() const;
	/**
	 * Use this method to skip the given number of bytes. You can
	 * spare out some bytes, and write/read them later.
	 * @return >0
	 *   Current index, success
	 * @return 0
	 *   Unable to reserve requested number of bytes.
	 * @see SetIndex
	 */
	uint32_t Skip ( const uint32_t NumberofBytes );
	/**
	 * @return the remaining free storage space in bytes
	 */
	uint32_t Available () const;
	/** API method:
	 * @return the overall capacity of this buffer
	 */
	uint32_t Capacity () const;
	/** API method:
	 *  @return the number of bytes used in this buffer
	 */
	uint32_t BytesUsed () const;
	/** API method:
	 * check if the buffer has a specified amount left
	 * free for storage
	 * @param Size
	 *   the number of bytes, which should be free
	 * @return true
	 *   the number of bytes are free for storage
	 * @return false
	 *   not enough bytes left for storage
	 */
	bool isAvailable  ( const uint32_t Size ) const;
	/** API method:
	 * check if there is still data to read
	 * @return
	 *   the number of bytes left for reading
	 */
	uint32_t RemainingData () const;
	/** API method
	 * @return
	 *   the current index
	 */
	inline uint32_t CurrentIndex () const
	{
		return ( m_CurrentIndex );
	};
	/** API method
	 * @return
	 *   a pointer to the internal buffer
	 */
	const char* Buffer() const
	{
		return m_Buffer;
	};
	/** API method
	 * use the specified buffer
	 * @param Buffer
	 *   Pointer to the buffer to use.
	 * @param Size
	 *   The number of (used) bytes in the buffer
	 */
	void SetBuffer ( const char* Buffer, const uint32_t Size );
	/** API method:
	 * Set number of used bytes of this NetPacket. Needed if
	 * the content is set from outside
	 * @param UsedBytes
	 *   the number of bytes already used by this buffer
	 */
	void SetUsed ( const uint32_t UsedBytes );
	/* Write data to the buffer
	 *
	 * Be aware that: \n
	 *   long myvar = 1; \n
	 *   Buffer->Write<long> (myvar); \n
	 * isn't portable, as the type 'long' (as others) may vary in size. \n
	 * Always use uintXX_t! \n
	 * @usage Buffer->Write<uint32_t> (MyVar);
	 */
	/// write a signed 8 bit value to the buffer
	bool Write_int8   ( const int8_t&    Data );
	/// write an unsigned 8 bit value to the buffer
	bool Write_uint8  ( const uint8_t&   Data );
	/// write a signed 16 bit value to the buffer
	bool Write_int16  ( const int16_t&   Data );
	/// write an unsigned 16 bit value to the buffer
	bool Write_uint16 ( const uint16_t& Data );
	/// write a signed 32 bit value to the buffer
	bool Write_int32  ( const int32_t&   Data );
	/// write an unsigned 32 bit value to the buffer
	bool Write_uint32 ( const uint32_t& Data );
	/// write a signed 64 bit value to the buffer
	bool Write_int64  ( const int64_t&   Data );
	/// write an unsigned 64 bit value to the buffer
	bool Write_uint64 ( const uint64_t& Data );
	/// write a float value to the buffer
	bool Write_float  ( const float&     Data );
	/// write a double value to the buffer
	bool Write_double ( const double&    Data );
	/**
	 * copy arbitrary data to the buffer \n
	 * Opaque data has the form |length|data \n
	 * !!! Remember: if you want to store a c-str, write strlen+1 bytes \n
	 * !!! in order to write the \0 character at the end \n
	 * @param Data
	 *   pointer to the data. The data is supposed to be raw, no
	 *   byte ordering is applied to it.
	 * @param Size
	 *   number of bytes to copy
	 * @param AlignBytes
	 *   align data multiples of this
	 */
	bool WriteOpaque ( const void* Data, const uint32_t Size );
	/// Write a string
	bool WriteString ( const std::string& Str );
	/** compatabilty routine
	 *
	 * write a raw c-string to the buffer. if Size!=0
	 * write only Size characters
	 */
	bool WriteCStr ( const char* Str, uint32_t Size=0 );
	/*
	 * Read_methods Read data from the buffer
	 */
	/// Read a signed 8 bit value from the buffer
	int8_t   Read_int8   ();
	/// Read an unsigned 8 bit value from the buffer
	uint8_t  Read_uint8  ();
	/// Read a signed 16 bit value from the buffer
	int16_t  Read_int16  ();
	/// Read an unsigned 8 bit value from the buffer
	uint16_t Read_uint16 ();
	/// Read a signed 32 bit value from the buffer
	int32_t  Read_int32  ();
	/// Read an unsigned 32 bit value from the buffer
	uint32_t Read_uint32 ();
	/// Read a signed 64 bit value from the buffer
	int64_t  Read_int64  ();
	/// Read an unsigned 64 bit value from the buffer
	uint64_t Read_uint64 ();
	/// Read a float value from the buffer
	float    Read_float  ();
	/// Read a double value from the buffer
	double   Read_double ();
	/**
	 * Return a pointer to arbitrary data,
	 * increase internal pointer by 'Size' bytes. \n
	 * Opaque data has the form |length|data
	 * @param Size
	 *   modified by this method. The number of bytes.
	 * @return a pointer to the data.
	 *   The data is supposed to be raw, no byte ordering is
	 *   applied to it.
	 */
	void ReadOpaque ( void* Buffer, uint32_t& Size  );
	/// Read a string from the buffer. \n
	/// Like ReadOpaque(), but automatically inserts the length of the string
	std::string ReadString ();
	/** compatabilty routine
	 *
	 * read a raw c-string from the buffer.
	 */
	std::string ReadCStr ();
	/// Get byte at index
	inline int8_t Peek ( const int Index ) const
	{
		return m_Buffer[Index];
	};
	/// Get byte at current index
	inline int8_t Peek () const
	{
		return m_Buffer[m_CurrentIndex];
	};
	/// en-/decrypt packet with password
	void Encrypt ( const uint32_t Key[4], const uint32_t Offset=0 );
	void Decrypt ( const uint32_t Key[4], const uint32_t Offset=0 );
private:
	BUFFER_ENCODING_TYPE m_EncodingType;
	///  disallow standard constructor
	NetPacket();
	///  disallow copy constructor
	NetPacket ( const NetPacket& Buffer );
	/* read xdr encoded data */
	///
	int8_t   Read_XDR_int8   ();
	///
	uint8_t  Read_XDR_uint8  ();
	///
	int16_t  Read_XDR_int16  ();
	///
	uint16_t Read_XDR_uint16 ();
	///
	int32_t  Read_XDR_int32  ();
	///
	uint32_t Read_XDR_uint32 ();
	///
	int64_t  Read_XDR_int64  ();
	///
	uint64_t Read_XDR_uint64 ();
	///
	float    Read_XDR_float  ();
	///
	double   Read_XDR_double ();

	/* read NET encoded data */
	///
	int8_t   Read_NET_int8   ();
	///
	uint8_t  Read_NET_uint8  ();
	///
	int16_t  Read_NET_int16  ();
	///
	uint16_t Read_NET_uint16 ();
	///
	int32_t  Read_NET_int32  ();
	///
	uint32_t Read_NET_uint32 ();
	///
	int64_t  Read_NET_int64  ();
	///
	uint64_t Read_NET_uint64 ();
	///
	float    Read_NET_float  ();
	///
	double   Read_NET_double ();

	/* read unecoded data */
	///
	int8_t   Read_NONE_int8   ();
	///
	uint8_t  Read_NONE_uint8  ();
	///
	int16_t  Read_NONE_int16  ();
	///
	uint16_t Read_NONE_uint16 ();
	///
	int32_t  Read_NONE_int32  ();
	///
	uint32_t Read_NONE_uint32 ();
	///
	int64_t  Read_NONE_int64  ();
	///
	uint64_t Read_NONE_uint64 ();
	///
	float    Read_NONE_float  ();
	///
	double   Read_NONE_double ();

	/* write XDR encoded data */
	///
	bool Write_XDR_int8   ( const int8_t&    Data );
	///
	bool Write_XDR_uint8  ( const uint8_t&   Data );
	///
	bool Write_XDR_int16  ( const int16_t&   Data );
	///
	bool Write_XDR_uint16 ( const uint16_t& Data );
	///
	bool Write_XDR_int32  ( const int32_t&   Data );
	///
	bool Write_XDR_uint32 ( const uint32_t& Data );
	///
	bool Write_XDR_int64  ( const int64_t&   Data );
	///
	bool Write_XDR_uint64 ( const uint64_t& Data );
	///
	bool Write_XDR_float  ( const float&     Data );
	///
	bool Write_XDR_double ( const double&    Data );

	/* write NET encoded data */
	///
	bool Write_NET_int8   ( const int8_t&    Data );
	///
	bool Write_NET_uint8  ( const uint8_t&   Data );
	///
	bool Write_NET_int16  ( const int16_t&   Data );
	///
	bool Write_NET_uint16 ( const uint16_t& Data );
	///
	bool Write_NET_int32  ( const int32_t&   Data );
	///
	bool Write_NET_uint32 ( const uint32_t& Data );
	///
	bool Write_NET_int64  ( const int64_t&   Data );
	///
	bool Write_NET_uint64 ( const uint64_t& Data );
	///
	bool Write_NET_float  ( const float&     Data );
	///
	bool Write_NET_double ( const double&    Data );

	/* write unencoded data */
	///
	bool Write_NONE_int8   ( const int8_t&    Data );
	///
	bool Write_NONE_uint8  ( const uint8_t&   Data );
	///
	bool Write_NONE_int16  ( const int16_t&   Data );
	///
	bool Write_NONE_uint16 ( const uint16_t& Data );
	///
	bool Write_NONE_int32  ( const int32_t&   Data );
	///
	bool Write_NONE_uint32 ( const uint32_t& Data );
	///
	bool Write_NONE_int64  ( const int64_t&   Data );
	///
	bool Write_NONE_uint64 ( const uint64_t& Data );
	///
	bool Write_NONE_float  ( const float&     Data );
	///
	bool Write_NONE_double ( const double&    Data );
	///
	enum { XTEA_BLOCK_SIZE = 8 };

	/// eXtended Tiny Encryption Algorithm
	/// encode 64 data bits (v) with 128 bit key (k)
	void xtea_encipher ( unsigned int num_cycles, uint32_t v[2],
			     uint32_t const k[4]
			   );
	/// eXtended Tiny Encryption Algorithm
	/// decode 64 data bits (v) with 128 bit key (k)
	void xtea_decipher ( unsigned int num_cycles, uint32_t v[2],
			     uint32_t const k[4]
			   );
}; // NetPacket

#endif

