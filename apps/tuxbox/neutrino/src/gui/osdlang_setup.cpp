/*
	$Id: osdlang_setup.cpp,v 1.11 2012/09/23 08:26:30 rhabarber1848 Exp $

	OSD-Language Setup  implementation - Neutrino-GUI

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "osdlang_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>

#include <driver/screen_max.h>

#include <system/debug.h>

#include <algorithm>
#include <dirent.h>



COsdLangSetup::COsdLangSetup(const neutrino_locale_t title, const char * const IconName)
{
	menue_title = title;
	menue_icon = IconName;

	width = w_max (500, 100);
	selected = -1;
}

COsdLangSetup::~COsdLangSetup()
{

}


int COsdLangSetup::exec(CMenuTarget* parent, const std::string & actionKey)
{
	dprintf(DEBUG_DEBUG, "init font setup\n");

	if(!actionKey.empty())
	{
		const char * locale = actionKey.c_str();
		strcpy(g_settings.language, locale);

		g_PluginList->loadPlugins();

		int unicode_locale = g_Locale->loadLocale(locale);
		if(CNeutrinoApp::getInstance()->ChangeFonts(unicode_locale))
		{
			parent->hide();
			return menu_return::RETURN_REPAINT;
		}
		else
			return menu_return::RETURN_NONE;
	}

	if(parent != NULL)
		parent->hide();

	int res = showSetup();
	
	return res;
}


int COsdLangSetup::showSetup()
{
	CMenuWidget *osdl_setup = new CMenuWidget(menue_title, menue_icon, width);
	osdl_setup->setPreselected(selected);

	//intros
	osdl_setup->addIntroItems(menue_title != LOCALE_LANGUAGESETUP_HEAD ? LOCALE_LANGUAGESETUP_HEAD : NONEXISTANT_LOCALE);

	//search available languages....

	struct dirent **namelist;
	int n;
	//		printf("scanning locale dir now....(perhaps)\n");

	const char *pfad[] = {DATADIR "/neutrino/locale", CONFIGDIR "/locale"};
	std::string locales = "|";

	for(int p = 0;p < 2;p++)
	{
		n = scandir(pfad[p], &namelist, 0, alphasort);
		if(n < 0)
		{
			perror("loading locales: scandir");
		}
		else
		{
			for(int count=0;count<n;count++)
			{
				char * locale = namelist[count]->d_name;
				char * pos = strstr(locale, ".locale");
				if(pos != NULL)
				{
					*pos = '\0';
					if(locales.find("|" + std::string(locale) + "|") == std::string::npos)
					{
						locales += locale;
						locales += "|";

						CMenuForwarder* oj = new CMenuForwarder(locale, true, NULL, this, locale);
						oj->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);
						osdl_setup->addItem(oj, strcmp(g_settings.language, locale) == 0);
					}
				}
				free(namelist[count]);
			}
			free(namelist);
		}
	}

	int res = osdl_setup->exec(NULL, "");
	selected = osdl_setup->getSelected();
	delete osdl_setup;

	return res;
}

