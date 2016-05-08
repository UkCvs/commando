/*
	NeutrinoNG  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/
	
	$Id: helper.cpp,v 1.4 2009/02/24 19:27:59 seife Exp $
	
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


#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdarg.h>

#include "helper.h"

void StrSearchReplace( std::string &s, const std::string &to_find, const std::string& repl_with )
{
	std::string::size_type location = s.find(to_find);
	if ( location == std::string::npos )
	{
		return;
	}
	while ( location != std::string::npos )
	{
		s.erase(location,to_find.size());
		s.insert(location,repl_with);
		location = s.find(to_find, location);
	}
}

bool file_exists(const char *filename)
{
	struct stat stat_buf;
	if(::stat(filename, &stat_buf) == 0)
	{
		return true;
	} else
	{
		return false;
	}
}

//use for script with full path
int my_system(const char * cmd)
{
	if (!file_exists(cmd))
		return -1;

	return my_system(1, cmd);
}

int my_system(int argc, const char *arg, ...)
{
	int i = 0, ret, childExit = 0;
#define ARGV_MAX 64
	/* static right now but could be made dynamic if necessary */
	int argv_max = ARGV_MAX;
	const char *argv[ARGV_MAX];
	va_list args;
	argv[0] = arg;
	va_start(args, arg);

	while(++i < argc)
	{
		if (i == argv_max)
		{
			fprintf(stderr, "my_system: too many arguments!\n");
			va_end(args);
			return -1;
		}
		argv[i] = va_arg(args, const char *);
	}
	argv[i] = NULL; /* sentinel */
	//fprintf(stderr,"%s:", __func__);for(i=0;argv[i];i++)fprintf(stderr," '%s'",argv[i]);fprintf(stderr,"\n");

	pid_t pid;
	int maxfd = getdtablesize();// sysconf(_SC_OPEN_MAX);
	switch (pid = vfork())
	{
		case -1: /* can't vfork */
			perror("vfork");
			ret = -errno;
			break;
		case 0: /* child process */
			ret = 0;
			for(i = 3; i < maxfd; i++)
				close(i);
			if (setsid() == -1)
				perror("my_system setsid");
			if (execvp(argv[0], (char * const *)argv))
			{
				ret = -errno;
				if (errno != ENOENT) /* don't complain if argv[0] only does not exist */
					fprintf(stderr, "ERROR: my_system \"%s\": %m\n", argv[0]);
			}
			_exit(ret); // terminate c h i l d proces s only
		default: /* parent returns to calling process */
			ret = 0;
			waitpid(pid, &childExit, 0);
			if (WEXITSTATUS(childExit) != 0)
				ret = (signed char)WEXITSTATUS(childExit);
			break;
	}
	va_end(args);
	return ret;
}

std::string find_executable(const char *name)
{
	struct stat s;
	char *tmpPath = getenv("PATH");
	char *p, *n, *path;
	if (tmpPath)
		path = strdupa(tmpPath);
	else
		path = strdupa("/bin:/var/bin:/sbin:/var/sbin");
	if (name[0] == '/') { /* full path given */
		if (!access(name, X_OK) && !stat(name, &s) && S_ISREG(s.st_mode))
			return std::string(name);
		return "";
	}

	p = path;
	while (p) {
		n = strchr(p, ':');
		if (n)
			*n++ = '\0';
		if (*p != '\0') {
			std::string tmp = std::string(p) + "/" + std::string(name);
			const char *f = tmp.c_str();
			if (!access(f, X_OK) && !stat(f, &s) && S_ISREG(s.st_mode))
				return tmp;
		}
		p = n;
	}
	return "";
}

std::string to_string(int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(unsigned int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(long i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(unsigned long i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(long long i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(unsigned long long i)
{
	std::stringstream s;
	s << i;
	return s.str();
}
