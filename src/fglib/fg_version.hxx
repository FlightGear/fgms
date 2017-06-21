/**
 * @file fg_version.hxx
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
// Copyright (C) 2017  Oliver Schroeder
//

#ifndef FG_VERSION_HEADER
#define FG_VERSION_HEADER

#include <string>
#include <ostream>
#include <stdint.h>

/**
 * A class to manage the version of a program/library
 */
class FG_VERSION
{
public:
	/** Initialisation via constructor only
	 *
	 * Increment 'patch' if source code has changed at all
	 * (e.g. bugfix).
	 *
	 * If the source has new features but is still
	 * compatible with older versions, increment
	 * 'minor' and set 'patch' to 0.
	 *
	 * If the source gets incompatible with older
	 * versions, increment 'major' and set 'minor'
	 * and 'patch' to 0.
	 *
	 * 'extra' can be used to mark the state of the
	 * source, eg. '-dev' or '-rc1'
	 *
	 */
	FG_VERSION ( char major, char minor, char patch, const char* extra );
	/** initialise all to 0 */
	FG_VERSION ();
	/** return a string representation of the version */
	inline std::string str () const { return m_str_rep; };
	/** return a 32-bit integer representation of the version
	 *  If one of 'major', 'minor' or 'patch' gets over 255
	 *  we need to switch to 64 bit.
	 */
	uint32_t   num () const;
	/** return a 'major' as an int */
	inline int major () const { return m_version.major; };
	/** return a 'minor' as an int */
	inline int minor () const { return m_version.minor; };
	/** return a 'patch' as an int */
	inline int patch () const { return m_version.patch; };
	/** set an uint32_t as version */
	void set_num ( uint32_t v );
	// comparison operator
	bool operator ==  ( const FG_VERSION& V ) const;
	// comparison operator
	bool operator !=  ( const FG_VERSION& V ) const;
	/** output operator */
	friend std::ostream& operator << ( std::ostream& o, const FG_VERSION& V );
private:
	FG_VERSION ( const FG_VERSION& );	// disable
	struct
	{
		char	major;
		char	minor;
		char	patch;
		char	dummy; // align to 32 bit
	} m_version;
	std::string	m_extra;
	std::string	m_str_rep;
	void mk_str_rep ();
}; // class FG_VERSION

#endif

