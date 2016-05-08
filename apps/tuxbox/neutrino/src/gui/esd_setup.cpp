/*
	$Id: esd_setup.cpp,v 1.10 2012/09/12 07:25:12 rhabarber1848 Exp $

	esound setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
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


#include "gui/esd_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include "gui/widget/stringinput.h"

#include "gui/filebrowser.h"

#include <driver/screen_max.h>

#include <system/debug.h>



CEsdSetup::CEsdSetup()
{
	width = w_max (500, 100);
	selected = -1;
}

CEsdSetup::~CEsdSetup()
{

}

int CEsdSetup::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init esd setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	res = showEsdSetup();
	
	return res;
}

int CEsdSetup::showEsdSetup()
/*shows the esd setup menue*/
{
	CMenuWidget* esdSetup = new CMenuWidget(LOCALE_MAINMENU_SETTINGS, NEUTRINO_ICON_AUDIO, width);
	esdSetup->setPreselected(selected);

	// intros
	esdSetup->addIntroItems(LOCALE_ESOUND_NAME);

	// entry
	CStringInput setup_EsoundPort(LOCALE_ESOUND_PORT, g_settings.esound_port, 5, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	esdSetup->addItem(new CMenuForwarder(LOCALE_ESOUND_PORT, true, g_settings.esound_port, &setup_EsoundPort));

	int res = esdSetup->exec(NULL, "");
	selected = esdSetup->getSelected();
	delete esdSetup;

	return res;
}
