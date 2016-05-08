/*
	$Id: osd_setup.h,v 1.8 2012/09/23 08:16:48 rhabarber1848 Exp $

	osd_setup implementation - Neutrino-GUI

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

#ifndef __osd_setup__
#define __osd_setup__

#include <gui/widget/menue.h>

#include <system/settings.h>

#include "themes.h"

#include <string>

class CFontSizeNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t OptionName, void * data);
};

class COsdSetup : public CMenuTarget, CChangeObserver
{
	private:
		CColorSetupNotifier *colorSetupNotifier;
		CFontSizeNotifier *fontsizenotifier;

		CMenuWidget *osd_setup;

		int width, selected;

		neutrino_locale_t menue_title;
		std::string menue_icon;

		std::string font_file_name;

		int showOsdSetup();
		int showOsdMenueColorSetup();
		int showOsdInfobarColorSetup();
		int showOsdTimeoutSetup();
		int showOsdInfobarSetup();
		int showOsdChannelListSetup();
		int showOsdFontSizeSetup();
		void AddFontSettingItem(CMenuWidget *fontSettings, const SNeutrinoSettings::FONT_TYPES number_of_fontsize_entry);

	public:
		COsdSetup(const neutrino_locale_t title = LOCALE_OSDSETTINGS_COLORMENU_HEAD, const char * const IconName = NEUTRINO_ICON_COLORS);
		~COsdSetup();
		int exec(CMenuTarget* parent, const std::string & actionKey);
		bool changeNotify(const neutrino_locale_t OptionName, void *);

};

#endif
