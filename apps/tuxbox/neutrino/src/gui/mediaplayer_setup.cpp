/*
	$Id: mediaplayer_setup.cpp,v 1.11 2012/09/12 07:25:12 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	media player setup implementation - Neutrino-GUI

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


#include "gui/mediaplayer_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include "gui/widget/stringinput.h"

#ifdef ENABLE_AUDIOPLAYER
#include "gui/audioplayer_setup.h"
#endif

#ifdef ENABLE_PICTUREVIEWER
#include "gui/pictureviewer_setup.h"
#endif

#ifdef ENABLE_ESD
#include "gui/esd_setup.h"
#endif

#ifdef ENABLE_MOVIEPLAYER
#include "gui/movieplayer_setup.h"
#endif


#include <driver/screen_max.h>

#include <system/debug.h>
#include <unistd.h>



CMediaPlayerSetup::CMediaPlayerSetup()
{
	width = w_max (500, 100);
	selected = -1;
}

CMediaPlayerSetup::~CMediaPlayerSetup()
{

}

int CMediaPlayerSetup::exec(CMenuTarget* parent, const std::string & /*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init mediaplayer setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	res = showMediaPlayerSetup();
	
	return res;
}

int CMediaPlayerSetup::showMediaPlayerSetup()
/*shows media setup menue entries*/
{
	CMenuWidget* mediaSetup = new CMenuWidget(LOCALE_MAINMENU_SETTINGS, NEUTRINO_ICON_SETTINGS, width);
	mediaSetup->setPreselected(selected);

	// intros
	mediaSetup->addIntroItems(LOCALE_MEDIAPLAYERSETTINGS_GENERAL);

	// entries
#ifdef ENABLE_AUDIOPLAYER
	// audioplayer
	CAudioPlayerSetup* audioPlayerSetup = new CAudioPlayerSetup();
	mediaSetup->addItem(new CMenuForwarder(LOCALE_MAINMENU_AUDIOPLAYER, true, NULL, audioPlayerSetup, NULL, CRCInput::RC_red));
#endif
#ifdef ENABLE_ESD
	// esound
	CEsdSetup* esdSetup = NULL;
	if (access("/bin/esd", X_OK) == 0 || access("/var/bin/esd", X_OK) == 0)
	{
		esdSetup = new CEsdSetup();
		mediaSetup->addItem(new CMenuForwarder(LOCALE_ESOUND_NAME, true, NULL, esdSetup, NULL, CRCInput::RC_green));
	}
#endif
#ifdef ENABLE_MOVIEPLAYER
	// movieplayer
	CMoviePlayerSetup* moviePlayerSetup = new CMoviePlayerSetup();
	mediaSetup->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_STREAMING, true, NULL, moviePlayerSetup, NULL, CRCInput::RC_yellow));
#endif
#ifdef ENABLE_PICTUREVIEWER
	// pictureviewer
	CPictureViewerSetup* pictureViewerSetup = new CPictureViewerSetup();
	mediaSetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_HEAD, true, NULL, pictureViewerSetup, NULL, CRCInput::RC_blue));
#endif

	int res = mediaSetup->exec(NULL, "");
	selected = mediaSetup->getSelected();
	delete mediaSetup;

#ifdef ENABLE_AUDIOPLAYER
	delete audioPlayerSetup;
#endif
#ifdef ENABLE_ESD
	delete esdSetup;
#endif
#ifdef ENABLE_MOVIEPLAYER
	delete moviePlayerSetup;
#endif
#ifdef ENABLE_PICTUREVIEWER
	delete pictureViewerSetup;
#endif

	return res;
}
