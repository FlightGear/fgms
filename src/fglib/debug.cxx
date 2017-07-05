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
// Copyright (C) 2011  Oliver Schroeder
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include "debug.hxx"

using namespace std;

int DEBUG::depth = 0;

//////////////////////////////////////////////////////////////////////

DEBUG::DEBUG
(
	const char* function,
	const char* filename,
	const int line
)
{
#ifdef DEF_ENABLE_TRACE
	this->filename = filename;
	this->function = function;
	this->line = line;
	cerr	<< space() << filename << ":"
		<< function << ":" << line << " start" << endl;
	depth += 2;
#endif
} // DEBUG::DEBUG()

//////////////////////////////////////////////////////////////////////

std::string
DEBUG::space
()
{
	std::string r;
	for (int i=0; i<depth; i++)
		r += " ";
	return (r);
} // DEBUG::space()

//////////////////////////////////////////////////////////////////////

void
DEBUG::trace
(
	const char* function,
	const char* filename,
	const int line
)
{
#ifdef DEF_ENABLE_TRACE
	cerr	<< space() << "# TRACE: "  << filename << ":"
		<< function << ":" << line << endl;
#endif
} // DEBUG::trace()

//////////////////////////////////////////////////////////////////////

void
DEBUG::dump
(
	void* data,
	int len
)
{
	const char hex [] = "0123456789ABCDEF";
	uint8_t* d = (uint8_t*) data;

	std::cerr << std::endl;
	std::cerr << "----------------------------------" << std::endl;
	for ( int i = 0; i < len; i++ )
	{
		std::cerr
			<< hex [*d >> 4]
			<< hex [*d & ~240]
			<< " ";
		if ( (i+1) % 16 == 0 )
			std::cerr << std::endl;
		d++;
	}
	std::cerr << std::endl;
	std::cerr << "----------------------------------" << std::endl;
} // Debug::dump()

//////////////////////////////////////////////////////////////////////

DEBUG::~DEBUG
()
{
#ifdef DEF_ENABLE_TRACE
	depth -= 2;
	cerr	<< space() << filename << ":"
		<< function << ":" << line << " end" << endl;
#endif
} // DEBUG::~DEBUG()

//////////////////////////////////////////////////////////////////////

