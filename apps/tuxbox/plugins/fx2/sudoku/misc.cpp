/*
	Sudoku
	Copyright (C) 2006-2009 Martin Schlosser
	email: sudoku@software-schlosser.de
	www:   http://www.software-schlosser.de

	Sudoku is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Sudoku is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Sudoku in a file named COPYING.
	If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/time.h>

// ----------------------------------------------------------------------------
// Rand
// ----------------------------------------------------------------------------
int Rand(void)
{
//	return rand();
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (int) tv.tv_usec;
}

// ----------------------------------------------------------------------------
// Sleep
// ----------------------------------------------------------------------------
void Sleep(unsigned int usec)
{
	struct timeval tv;
	tv.tv_sec		= 0;
	tv.tv_usec	= usec;
	select(0, 0, 0, 0, &tv);
}
