/*
	NeutrinoNG  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef __neutrino_helper__
#define __neutrino_helper__

#include <string>
#include <sstream>

void StrSearchReplace( std::string &s, const std::string &to_find, const std::string& repl_with );

int my_system(const char * cmd);
int my_system(int argc, const char *arg, ...); /* argc is number of arguments including command */

bool file_exists(const char *filename);

std::string find_executable(const char *name);

std::string to_string(int);
std::string to_string(unsigned int);
std::string to_string(long);
std::string to_string(unsigned long);
std::string to_string(long long);
std::string to_string(unsigned long long);
#endif
