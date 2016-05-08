/*
	$Id: user_menue_setup.cpp,v 1.12 2012/09/12 07:25:12 rhabarber1848 Exp $

	user_menue setup implementation - Neutrino-GUI
	based up implementation by Günther

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


#include "gui/user_menue_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/keychooser.h>
#include <system/helper.h>

#include <driver/screen_max.h>

#include <system/debug.h>

CUserMenuSetup::CUserMenuSetup(neutrino_locale_t menue_title, int menue_button)
{
	local = menue_title;
	button = menue_button;

	width = w_max (500, 100);
	selected = -1;
}

CUserMenuSetup::~CUserMenuSetup()
{

}

#define USERMENU_ITEM_OPTION_COUNT SNeutrinoSettings::ITEM_MAX
const CMenuOptionChooser::keyval USERMENU_ITEM_OPTIONS[USERMENU_ITEM_OPTION_COUNT] =
{
	{SNeutrinoSettings::ITEM_NONE, LOCALE_USERMENU_ITEM_NONE} ,
	{SNeutrinoSettings::ITEM_BAR, LOCALE_USERMENU_ITEM_BAR} ,
	{SNeutrinoSettings::ITEM_EPG_LIST, LOCALE_EPGMENU_EVENTLIST} ,
	{SNeutrinoSettings::ITEM_EPG_SUPER, LOCALE_EPGMENU_EPGPLUS} ,
	{SNeutrinoSettings::ITEM_EPG_INFO, LOCALE_EPGMENU_EVENTINFO} ,
	{SNeutrinoSettings::ITEM_EPG_MISC, LOCALE_USERMENU_ITEM_EPG_MISC} ,
	{SNeutrinoSettings::ITEM_AUDIO_SELECT, LOCALE_APIDSELECTOR_HEAD} ,
	{SNeutrinoSettings::ITEM_SUBCHANNEL, LOCALE_INFOVIEWER_SUBSERVICE} ,
	{SNeutrinoSettings::ITEM_PLUGIN, LOCALE_TIMERLIST_PLUGIN} ,
	{SNeutrinoSettings::ITEM_VTXT, LOCALE_USERMENU_ITEM_VTXT} ,
	{SNeutrinoSettings::ITEM_RECORD, LOCALE_TIMERLIST_TYPE_RECORD} ,
	{SNeutrinoSettings::ITEM_MOVIEPLAYER_TS, LOCALE_MAINMENU_MOVIEPLAYER} ,
	{SNeutrinoSettings::ITEM_MOVIEPLAYER_MB, LOCALE_MOVIEBROWSER_HEAD} ,
	{SNeutrinoSettings::ITEM_TIMERLIST, LOCALE_TIMERLIST_NAME} ,
	{SNeutrinoSettings::ITEM_REMOTE, LOCALE_RCLOCK_MENUEADD} ,
	{SNeutrinoSettings::ITEM_FAVORITS, LOCALE_FAVORITES_MENUEADD} ,
	{SNeutrinoSettings::ITEM_TECHINFO, LOCALE_EPGMENU_STREAMINFO}
};

int CUserMenuSetup::exec(CMenuTarget* parent, const std::string &)
{
	if(parent != NULL)
		parent->hide();

	int res = showSetup();

	return res;
}

int CUserMenuSetup::showSetup()
{
	CMenuWidget * ums = new CMenuWidget(LOCALE_PERSONALIZE_HEAD, NEUTRINO_ICON_PROTECTING, width);
	ums->setPreselected(selected);

	CStringInputSMS name(LOCALE_USERMENU_NAME, &g_settings.usermenu_text[button], 11, true, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz\xE4\xF6\xFC\xDF/- ");
	CMenuForwarder *mf = new CMenuForwarder(LOCALE_USERMENU_NAME, true, g_settings.usermenu_text[button], &name);

	//-------------------------------------
	ums->addIntroItems(local);
	//-------------------------------------
	ums->addItem(mf);
	ums->addItem(GenericMenuSeparatorLine);
	//---------------------------
	for(int item = 1; item < SNeutrinoSettings::ITEM_MAX && item <14; item++) // Do not show more than 13 items
	{
		ums->addItem( new CMenuOptionChooser((to_string(item)+":").c_str(), &g_settings.usermenu[button][item], USERMENU_ITEM_OPTIONS, USERMENU_ITEM_OPTION_COUNT, true, NULL, CRCInput::RC_nokey, "", true, true));
	}

	int res = ums->exec(NULL, "");
	selected = ums->getSelected();
	delete ums;

	return res;
}

