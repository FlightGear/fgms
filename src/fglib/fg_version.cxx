/**
 * @file fg_version.cxx
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

#include "fg_version.hxx"
#include "fg_util.hxx"

//////////////////////////////////////////////////////////////////////

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
FG_VERSION::FG_VERSION
(
	char major,
	char minor,
	char patch,
	const char* extra
)
{
	m_version.major = major;
	m_version.minor = minor;
	m_version.patch = patch;
	m_version.dummy = 0;
	m_extra = extra;
	mk_str_rep ();
} // FG_VERSION::FG_VERSION ()

//////////////////////////////////////////////////////////////////////

/** initialise all to 0
 */
FG_VERSION::FG_VERSION
()
{
	m_version.major = 0;
	m_version.minor = 0;
	m_version.patch = 0;
	m_version.dummy = 0;
	m_extra = "";
	mk_str_rep ();
} // FG_VERSION::FG_VERSION ()

//////////////////////////////////////////////////////////////////////

/** return a string representation of the version
 */
void
FG_VERSION::mk_str_rep
()
{
	m_str_rep =  NumToStr (m_version.major, 0) + ".";
	m_str_rep += NumToStr (m_version.minor, 0) + ".";
	m_str_rep += NumToStr (m_version.patch, 0);
	m_str_rep += m_extra;
} // FG_VERSION::mk_str_rep ()

//////////////////////////////////////////////////////////////////////

/** return a 32-bit integer representation of the version
 *
 *  If one of 'major', 'minor' or 'patch' gets over 255
 *  we need to switch to 64 bit.
 */
uint32_t
FG_VERSION::num
() const
{
	int* v = (int*) & m_version;
	return *v;
} // FG_VERSION::num ()

//////////////////////////////////////////////////////////////////////

/** set an uint32_t as version
 */
void
FG_VERSION::set_num
(
	uint32_t v
)
{
	int *i = (int*) & m_version;
	*i = v;
	mk_str_rep ();
} // FG_VERSION::set_num ()

//////////////////////////////////////////////////////////////////////

/** Compare two versions
 *
 * return true if they are equal.
 */
bool
FG_VERSION::operator ==
(
	const FG_VERSION& v
) const
{
	if ( (m_version.major == v.m_version.major)
	&&   (m_version.minor == v.m_version.minor)
	&&   (m_version.patch == v.m_version.patch)
	&&   (m_version.dummy == v.m_version.dummy) )
		return true;
	return false;
} // FG_VERSION::operator == 

//////////////////////////////////////////////////////////////////////

/** Compare two versions
 *
 * return true if they are unequal.
 */
bool
FG_VERSION::operator !=
(
	const FG_VERSION& v
) const
{
	if ( (m_version.major != v.m_version.major)
	||   (m_version.minor != v.m_version.minor)
	||   (m_version.patch != v.m_version.patch)
	||   (m_version.dummy != v.m_version.dummy) )
		return true;
	return false;
} // FG_VERSION::operator != 

//////////////////////////////////////////////////////////////////////

/** Compare two versions
 *
 * return true if left is less than right
 */
bool
FG_VERSION::operator <
(
	const FG_VERSION& v
) const
{
	int *left = (int*)  & m_version;
	int *right = (int*) & v.m_version;
	if ( *left < *right )
		return true;
	return false;
} // FG_VERSION::operator <

//////////////////////////////////////////////////////////////////////

/** Compare two versions
 *
 * return true if left is greater than right
 */
bool
FG_VERSION::operator >
(
	const FG_VERSION& v
) const
{
	int *left = (int*)  & m_version;
	int *right = (int*) & v.m_version;
	if ( *left > *right )
		return true;
	return false;
} // FG_VERSION::operator >

//////////////////////////////////////////////////////////////////////

/** Compare two versions
 *
 * return true if left is less or equal to right
 */
bool
FG_VERSION::operator <=
(
	const FG_VERSION& v
) const
{
	int *left = (int*)  & m_version;
	int *right = (int*) & v.m_version;
	if ( *left <= *right )
		return true;
	return false;
} // FG_VERSION::operator >=

//////////////////////////////////////////////////////////////////////

/** Compare two versions
 *
 * return true if left is greater or equal to right
 */
bool
FG_VERSION::operator >=
(
	const FG_VERSION& v
) const
{
	int *left = (int*)  & m_version;
	int *right = (int*) & v.m_version;
	if ( *left >= *right )
		return true;
	return false;
} // FG_VERSION::operator >=

//////////////////////////////////////////////////////////////////////

/** Output operator
 *
 * Put this version on a stream.
 */
std::ostream&
operator <<
(
	std::ostream& o,
	const FG_VERSION& V
)
{
	o << V.str();
	return o;
} // ostream << FG_VERSION

//////////////////////////////////////////////////////////////////////

