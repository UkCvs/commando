/*
	$Id: pictureviewer_setup.cpp,v 1.9 2012/09/12 07:25:12 rhabarber1848 Exp $

	pictureviewer setup implementation - Neutrino-GUI

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


#include "gui/pictureviewer_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include "gui/pictureviewer.h"
#include "gui/filebrowser.h"

#include <driver/screen_max.h>

#include <system/debug.h>


CPictureViewerSetup::CPictureViewerSetup()
{
	width = w_max (500, 100);
	selected = -1;
}

CPictureViewerSetup::~CPictureViewerSetup()
{

}

int CPictureViewerSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init pctureviwer setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}


	if(actionKey == "picturedir")
	{
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.picviewer_picturedir))
			strncpy(g_settings.picviewer_picturedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.picviewer_picturedir)-1);
		return res;
	}

	res = showPictureViewerSetup();
	
	return res;
}

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};


#define PICTUREVIEWER_SCALING_OPTION_COUNT 3
const CMenuOptionChooser::keyval PICTUREVIEWER_SCALING_OPTIONS[PICTUREVIEWER_SCALING_OPTION_COUNT] =
{
	{ CPictureViewer::SIMPLE, LOCALE_PICTUREVIEWER_RESIZE_SIMPLE        },
	{ CPictureViewer::COLOR , LOCALE_PICTUREVIEWER_RESIZE_COLOR_AVERAGE },
	{ CPictureViewer::NONE  , LOCALE_PICTUREVIEWER_RESIZE_NONE          }
};


int CPictureViewerSetup::showPictureViewerSetup()
/*shows the setup menue*/
{
	CMenuWidget* picviewsetup = new CMenuWidget(LOCALE_MAINMENU_SETTINGS, NEUTRINO_ICON_VIDEO, width);
	picviewsetup->setPreselected(selected);

	// intros: back ande save
	picviewsetup->addIntroItems(LOCALE_PICTUREVIEWER_HEAD);

	picviewsetup->addItem(new CMenuOptionChooser(LOCALE_PICTUREVIEWER_SCALING  , &g_settings.picviewer_scaling     , PICTUREVIEWER_SCALING_OPTIONS  , PICTUREVIEWER_SCALING_OPTION_COUNT  , true ));

	CStringInput pic_timeout(LOCALE_PICTUREVIEWER_SLIDE_TIME, g_settings.picviewer_slide_time, 2, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	picviewsetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_SLIDE_TIME, true, g_settings.picviewer_slide_time, &pic_timeout));

	picviewsetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_DEFDIR, true, g_settings.picviewer_picturedir, this, "picturedir"));

	CIPInput picViewSettings_DecServerIP(LOCALE_PICTUREVIEWER_DECODE_SERVER_IP, g_settings.picviewer_decode_server_ip);
	picviewsetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_DECODE_SERVER_IP, true, g_settings.picviewer_decode_server_ip, &picViewSettings_DecServerIP));

	CStringInput picViewSettings_DecServerPort(LOCALE_PICTUREVIEWER_DECODE_SERVER_PORT, g_settings.picviewer_decode_server_port, 5, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	picviewsetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_DECODE_SERVER_PORT, true, g_settings.picviewer_decode_server_port, &picViewSettings_DecServerPort));

	int res = picviewsetup->exec(NULL, "");
	selected = picviewsetup->getSelected();
	delete picviewsetup;

	return res;
}
