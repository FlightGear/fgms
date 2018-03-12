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

#if !defined CLI_DEBUG_H
#define CLI_DEBUG_H

#include <iostream>

#ifdef DEF_ENABLE_DEBUG
	#define DEBUG_OUT(X) std::cerr << DEBUG::space() << X << std::endl;
	#define DEBUG_TRACE  DEBUG __debug ( __FUNCTION__,__FILE__,__LINE__ );
	#define DEBUG_LINE   std::cerr << DEBUG::space() << __FILE__ << " " << __LINE__ << std::endl;
#else
	#define DEBUG_OUT(X)
	#define DEBUG_TRACE
	#define DEBUG_LINE
#endif

/** @ingroup fglib
 * @breif support for easy debugging
 */
class DEBUG
{
public:
	DEBUG ( const char* function, const char* filename, const int line );
	void trace (const char* function,const char* filename,const int line );
	static std::string space ();
	static void dump ( void* data, int len );
	~DEBUG ();
private:
	static int depth;

	const char* filename;
	const char* function;
	int   line;
};

#endif
