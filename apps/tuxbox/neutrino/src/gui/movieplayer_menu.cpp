/*
	$Id: movieplayer_menu.cpp,v 1.15 2012/09/12 07:31:21 rhabarber1848 Exp $

	Movieplayer menue - Neutrino-GUI

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


#include "movieplayer_menu.h"
#include "movieplayer_setup.h"

#ifdef ENABLE_GUI_MOUNT
#include "nfs.h"
#endif

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <driver/screen_max.h>

#include <system/debug.h>



CMoviePlayerMenue::CMoviePlayerMenue()
{
	moviePlayerSetup = new CMoviePlayerSetup(&toNotify);

	width = w_max (500, 100);
	selected = -1;
}

CMoviePlayerMenue::~CMoviePlayerMenue()
{
	delete moviePlayerSetup;
}

int CMoviePlayerMenue::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init movieplayer menu\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	res = showMoviePlayerMenue();

	return res;
}

int CMoviePlayerMenue::showMoviePlayerMenue()
{
	CMenuTarget* 	moviePlayerGui = new CMoviePlayerGui();

	//init
	CMenuWidget * mpmenue = new CMenuWidget(LOCALE_MAINMENU_MOVIEPLAYER, NEUTRINO_ICON_EPGINFO, width);
	mpmenue->setPreselected(selected);

	//intros
	mpmenue->addIntroItems();

	//ts playback 
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK, true, NULL, moviePlayerGui, "tsplayback", CRCInput::RC_green));
	//ts playback pin 
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK_PC, true, NULL, moviePlayerGui, "tsplayback_pc", CRCInput::RC_1));

	neutrino_msg_t rc_msg;
#ifdef ENABLE_MOVIEBROWSER
#ifndef ENABLE_MOVIEPLAYER2
	//moviebrowser init via movieplayer 1
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, moviePlayerGui, "tsmoviebrowser", CRCInput::RC_2));
#else
	//moviebrowser init
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, CMovieBrowser::getInstance(), "run", CRCInput::RC_2));
#endif /* ENABLE_MOVIEPLAYER2 */
	rc_msg = CRCInput::RC_3;
#else
	rc_msg = CRCInput::RC_2;
#endif /* ENABLE_MOVIEBROWSER */

	//bookmark
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_BOOKMARK, true, NULL, moviePlayerGui, "bookmarkplayback", rc_msg));

	mpmenue->addItem(GenericMenuSeparatorLine);

	//vlc file play
	toNotify.push_back(new CMenuForwarder(LOCALE_MOVIEPLAYER_FILEPLAYBACK, g_settings.streaming_type == 1, NULL, moviePlayerGui, "fileplayback", CRCInput::RC_red));
	mpmenue->addItem(toNotify.back());
	//vlc dvd play
	toNotify.push_back(new CMenuForwarder(LOCALE_MOVIEPLAYER_DVDPLAYBACK, g_settings.streaming_type == 1, NULL, moviePlayerGui, "dvdplayback", CRCInput::RC_yellow));
	mpmenue->addItem(toNotify.back());
	//vlc vcd play
	toNotify.push_back(new CMenuForwarder(LOCALE_MOVIEPLAYER_VCDPLAYBACK, g_settings.streaming_type == 1, NULL, moviePlayerGui, "vcdplayback", CRCInput::RC_blue));
	mpmenue->addItem(toNotify.back());

	mpmenue->addItem(GenericMenuSeparatorLine);

	//settings
	mpmenue->addItem(new CMenuForwarder(LOCALE_MAINMENU_SETTINGS, true, NULL, moviePlayerSetup, NULL, CRCInput::RC_help));

#ifdef ENABLE_GUI_MOUNT
	//neutrino mount
	CNFSSmallMenu* nfsSmallMenu = new CNFSSmallMenu();
	mpmenue->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_MOUNT, true, NULL, nfsSmallMenu, NULL, CRCInput::RC_setup));
#endif

	int res = mpmenue->exec(NULL, "");
	selected = mpmenue->getSelected();
	delete mpmenue;

	delete moviePlayerGui;
#ifdef ENABLE_GUI_MOUNT
	delete nfsSmallMenu;
#endif
	toNotify.clear();

	return res;
}

