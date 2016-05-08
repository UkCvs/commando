/*
	Neutrino-GUI  -   DBoxII-Project



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

#ifndef __sleeptimer__
#define __sleeptimer__

#include <gui/widget/menue.h>

class CSleepTimerWidget: public CMenuTarget, CChangeObserver
{
 private:
	int shutdown_min;
	std::string shutdown_min_string;
	char value[16];

 public:
	int exec(CMenuTarget* parent, const std::string & actionKey);
	bool changeNotify(const neutrino_locale_t, void *);
	const char * getTargetValue();
};

#endif
