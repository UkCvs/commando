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

	$Id: themes.h,v 1.12 2012/09/23 08:18:03 rhabarber1848 Exp $ 

	Copyright (C) 2007, 2008, 2009 (flasher) Frank Liebelt
*/

#ifndef __cthemes__
#define __cthemes__
#include <string>
#include <configfile.h>

class CColorSetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *);
};

class CThemes : public CMenuTarget
{
	private:
		CConfigFile themefile;
		CColorSetupNotifier *colorSetupNotifier;

		int width, selected;
		int oldThemeValues[40];

		neutrino_locale_t menue_title;
		std::string menue_icon;

		bool hasThemeChanged;

		int Show();
		void readFile(char* themename);
		void saveFile(char* themename);
		void readThemes(CMenuWidget &);
		void rememberOldTheme(bool remember);
		void setupDefaultColors();

	public:
		CThemes(const neutrino_locale_t title = LOCALE_COLORTHEMEMENU_HEAD2, const char * const IconName = NEUTRINO_ICON_COLORS);
		~CThemes();
		int exec(CMenuTarget* parent, const std::string & actionKey);

		static void getColors(CConfigFile &configfile);
		static void setColors(CConfigFile &configfile);
};

#endif
