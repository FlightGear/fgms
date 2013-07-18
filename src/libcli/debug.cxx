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
// Copyright (C) 2011  Oliver Schroeder
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <string>
#include "debug.hxx"

using namespace std;

//  #define DEBUG_TRACE

std::string space ( int n )
{
	std::string r;
	for (int i=0; i<n; i++)
		r += " ";
	return (r);
}

int DEBUG::depth = 0;

DEBUG::DEBUG ( const char* function, const char* filename, const int line )
{
	#ifdef DEBUG_TRACE
	this->filename = filename;
	this->function = function;
	this->line = line;
	cerr << space(depth) << filename << ":" << function << ":" << line << " start" << endl;
	depth += 2;
	#endif
}

void DEBUG::trace ( const char* function, const char* filename, const int line )
{
	#ifdef DEBUG_TRACE
	cerr << space(depth) << "# TRACE: "  << filename << ":" << function << ":" << line << endl;
	#endif
}

DEBUG::~DEBUG ()
{
	#ifdef DEBUG_TRACE
	depth -= 2;
	cerr << space(depth) << filename << ":" << function << ":" << line << " end" << endl;
	#endif
}
