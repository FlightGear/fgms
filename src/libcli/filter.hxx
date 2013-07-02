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
// derived from libcli by David Parrish (david@dparrish.com)
// Copyright (C) 2011  Oliver Schroeder
//


#ifndef CLI_FILTER_H
#define CLI_FILTER_H

namespace LIBCLI
{

struct filter_cmds_t
{
	const char *cmd;
	const char *help;
};

class CLI;
typedef int (CLI::*filter_callback_func) (char *cmd, void *data);

class filter_t
{
public:
	filter_callback_func filter;
	void *data;
	filter_t *next;
	int exec (CLI& Instance, char *cmd);
	int exec (CLI& Instance, char *cmd, void *data);
};


}; // namespace LIBCLI

#endif
