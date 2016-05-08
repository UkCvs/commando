/*
	$Id: osdlang_setup.h,v 1.9 2012/09/23 08:18:03 rhabarber1848 Exp $

	OSD-Language Setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2010 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

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

#ifndef __osdlang_setup__
#define __osdlang_setup__

#include <global.h>
#include <neutrino.h>

#include <gui/widget/menue.h>

#include <string>

class COsdLangSetup : public CMenuTarget
{
	private:
		int width, selected;

		neutrino_locale_t menue_title;
		std::string menue_icon;

		int showSetup();

	public:
		COsdLangSetup(const neutrino_locale_t title = LOCALE_LANGUAGESETUP_HEAD, const char * const IconName = NEUTRINO_ICON_LANGUAGE);
		~COsdLangSetup();
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

#endif
