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

#ifndef version_HEADER
#define version_HEADER

#include <string>
#include <ostream>
#include <stdint.h>

namespace fgmp
{

/**
 * A class to manage the version of a program/library
 */
class version
{
public:
	version ( char major, char minor, char patch, const char* extra );
	version ();
	inline std::string str () const { return m_str_rep; };
	uint32_t   num () const;
	/** return a 'major' as an int */
	inline int major () const { return m_version.major; };
	/** return a 'minor' as an int */
	inline int minor () const { return m_version.minor; };
	/** return a 'patch' as an int */
	inline int patch () const { return m_version.patch; };
	void set_num ( uint32_t v );
	bool operator ==  ( const version& v ) const;
	bool operator !=  ( const version& v ) const;
	bool operator <   ( const version& v ) const;
	bool operator >   ( const version& v ) const;
	bool operator <=  ( const version& v ) const;
	bool operator >=  ( const version& v ) const;
	bool is_compatible ( const version& v ) const;
	friend std::ostream& operator << ( std::ostream& o, const version& v );
private:
	version ( const version& );	// disable
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
}; // class version

} // namespace fgmp

#endif

