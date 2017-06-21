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

uint32_t
FG_VERSION::num
() const
{
	int* v = (int*) & m_version;
	return *v;
} // FG_VERSION::num ()

//////////////////////////////////////////////////////////////////////

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

bool
FG_VERSION::operator ==
(
	const FG_VERSION& V
) const
{
	if ( (m_version.major == V.m_version.major)
	&&   (m_version.minor == V.m_version.minor)
	&&   (m_version.patch == V.m_version.patch)
	&&   (m_version.dummy == V.m_version.dummy) )
		return true;
	return false;
} // FG_VERSION::operator == 

//////////////////////////////////////////////////////////////////////


bool
FG_VERSION::operator !=
(
	const FG_VERSION& V
) const
{
	if ( (m_version.major != V.m_version.major)
	||   (m_version.minor != V.m_version.minor)
	||   (m_version.patch != V.m_version.patch)
	||   (m_version.dummy != V.m_version.dummy) )
		return true;
	return false;
} // FG_VERSION::operator != 

//////////////////////////////////////////////////////////////////////


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

