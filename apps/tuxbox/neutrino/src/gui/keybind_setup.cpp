/*
	$Id: keybind_setup.cpp,v 1.19 2012/09/23 08:16:48 rhabarber1848 Exp $

	keybindings setup implementation - Neutrino-GUI

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


#include "gui/keybind_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/bouquetlist.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/keychooser.h>

#include <driver/screen_max.h>

#include <system/debug.h>


CKeybindSetup::CKeybindSetup(const neutrino_locale_t title, const char * const IconName)
{
	changeNotify(LOCALE_KEYBINDINGMENU_REPEATBLOCK, NULL);

	menue_title = title;
	menue_icon = IconName;

	width = w_max (500, 100);
	selected = -1;
}

CKeybindSetup::~CKeybindSetup()
{

}

int CKeybindSetup::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init keybindings setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	res = showSetup();
	
	return res;
}

const neutrino_locale_t keydescription_head[] =
{
	LOCALE_KEYBINDINGMENU_TVRADIOMODE_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEUP_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_CANCEL_HEAD,
	LOCALE_KEYBINDINGMENU_SORT_HEAD,
	LOCALE_EVENTFINDER_HEAD,
	LOCALE_KEYBINDINGMENU_ADDRECORD_HEAD,
	LOCALE_KEYBINDINGMENU_ADDREMIND_HEAD,
	LOCALE_KEYBINDINGMENU_RELOAD_HEAD,
	LOCALE_KEYBINDINGMENU_CHANNELUP_HEAD,
	LOCALE_KEYBINDINGMENU_CHANNELDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_VOLUMEUP_HEAD,
	LOCALE_KEYBINDINGMENU_VOLUMEDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_BOUQUETUP_HEAD,
	LOCALE_KEYBINDINGMENU_BOUQUETDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELUP_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELTOGGLE_HEAD,
	LOCALE_KEYBINDINGMENU_ZAPHISTORY_HEAD,
	LOCALE_KEYBINDINGMENU_LASTCHANNEL_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEUP_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEDOWN_HEAD
};

const neutrino_locale_t keydescription[] =
{
	LOCALE_KEYBINDINGMENU_TVRADIOMODE,
	LOCALE_KEYBINDINGMENU_PAGEUP,
	LOCALE_KEYBINDINGMENU_PAGEDOWN,
	LOCALE_KEYBINDINGMENU_CANCEL,
	LOCALE_KEYBINDINGMENU_SORT,
	LOCALE_EVENTFINDER_HEAD,
	LOCALE_KEYBINDINGMENU_ADDRECORD,
	LOCALE_KEYBINDINGMENU_ADDREMIND,
	LOCALE_KEYBINDINGMENU_RELOAD,
	LOCALE_KEYBINDINGMENU_CHANNELUP,
	LOCALE_KEYBINDINGMENU_CHANNELDOWN,
	LOCALE_KEYBINDINGMENU_VOLUMEUP,
	LOCALE_KEYBINDINGMENU_VOLUMEDOWN,
	LOCALE_KEYBINDINGMENU_BOUQUETUP,
	LOCALE_KEYBINDINGMENU_BOUQUETDOWN,
	LOCALE_KEYBINDINGMENU_SUBCHANNELUP,
	LOCALE_KEYBINDINGMENU_SUBCHANNELDOWN,
	LOCALE_KEYBINDINGMENU_SUBCHANNELTOGGLE,
	LOCALE_KEYBINDINGMENU_ZAPHISTORY,
	LOCALE_KEYBINDINGMENU_LASTCHANNEL,
	LOCALE_KEYBINDINGMENU_PAGEUP,
	LOCALE_KEYBINDINGMENU_PAGEDOWN
};

#define KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT 3
const CMenuOptionChooser::keyval KEYBINDINGMENU_BOUQUETHANDLING_OPTIONS[KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT] =
{
	{ bsmChannels   , LOCALE_KEYBINDINGMENU_BOUQUETCHANNELS_ON_OK },
	{ bsmBouquets   , LOCALE_KEYBINDINGMENU_BOUQUETLIST_ON_OK     },
	{ bsmAllChannels, LOCALE_KEYBINDINGMENU_ALLCHANNELS_ON_OK     }
};

int CKeybindSetup::showSetup()
{
	CMenuWidget * ks = new CMenuWidget(menue_title, menue_icon, width);
	ks->setPreselected(selected);

	neutrino_msg_t * keyvalue_p[] =
		{
			&g_settings.key_tvradio_mode,
			&g_settings.key_channelList_pageup,
			&g_settings.key_channelList_pagedown,
			&g_settings.key_channelList_cancel,
			&g_settings.key_channelList_sort,
			&g_settings.key_channelList_search,
			&g_settings.key_channelList_addrecord,
			&g_settings.key_channelList_addremind,
			&g_settings.key_channelList_reload,
			&g_settings.key_quickzap_up,
			&g_settings.key_quickzap_down,
			&g_settings.key_volume_up,
			&g_settings.key_volume_down,
			&g_settings.key_bouquet_up,
			&g_settings.key_bouquet_down,
			&g_settings.key_subchannel_up,
			&g_settings.key_subchannel_down,
			&g_settings.key_subchannel_toggle,
			&g_settings.key_zaphistory,
			&g_settings.key_lastchannel,
			&g_settings.key_menu_pageup,
			&g_settings.key_menu_pagedown
		};

	CKeyChooser * keychooser[MAX_NUM_KEYNAMES];
	for (int i = 0; i < MAX_NUM_KEYNAMES; i++)
		keychooser[i] = new CKeyChooser(keyvalue_p[i], keydescription_head[i], NEUTRINO_ICON_KEYBINDING);


	//remote control
	CMenuWidget * ks_rc 		= new CMenuWidget(menue_title, menue_icon, width);
	CMenuForwarder *ks_rc_fw 	= new CMenuForwarder(LOCALE_KEYBINDINGMENU, true, NULL, ks_rc, NULL, CRCInput::RC_red);

	std::string ms_number_format("%d ");
	ms_number_format += g_Locale->getText(LOCALE_WORD_MILLISECONDS_SHORT);
	CMenuOptionNumberChooser *ks_rc_repeat_fw         = new CMenuOptionNumberChooser(LOCALE_KEYBINDINGMENU_REPEATBLOCK       , &g_settings.repeat_blocker       , true, 0, 999, 0, 0, NONEXISTANT_LOCALE, NULL, this, CRCInput::RC_nokey, "", true);
	ks_rc_repeat_fw->setNumberFormat(ms_number_format);
	CMenuOptionNumberChooser *ks_rc_repeat_generic_fw = new CMenuOptionNumberChooser(LOCALE_KEYBINDINGMENU_REPEATBLOCKGENERIC, &g_settings.repeat_genericblocker, true, 0, 999, 0, 0, NONEXISTANT_LOCALE, NULL, this, CRCInput::RC_nokey, "", true);
	ks_rc_repeat_generic_fw->setNumberFormat(ms_number_format);

	//mode change
	CMenuForwarder * ks_mc_fw = new CMenuForwarder(keydescription[VIRTUALKEY_TV_RADIO_MODE], true, keychooser[VIRTUALKEY_TV_RADIO_MODE]->getKeyName(), keychooser[VIRTUALKEY_TV_RADIO_MODE]);

	//channellist
	CMenuSeparator * ks_cl_sep = new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_CHANNELLIST);
	CMenuOptionChooser *ks_cl_oj = new CMenuOptionChooser(LOCALE_KEYBINDINGMENU_BOUQUETHANDLING, &g_settings.bouquetlist_mode, KEYBINDINGMENU_BOUQUETHANDLING_OPTIONS, KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT, true );

	//quickzap
	CMenuSeparator * ks_qz_sep = new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_QUICKZAP);

	//menu navigation
	CMenuSeparator * ks_mn_sep = new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_MENU);
	

	//paint items
	//remote control
	ks->addIntroItems(menue_title != LOCALE_MAINSETTINGS_KEYBINDING ? LOCALE_MAINSETTINGS_KEYBINDING : NONEXISTANT_LOCALE, LOCALE_KEYBINDINGMENU_RC);
	ks->addItem(ks_rc_repeat_fw);
	ks->addItem(ks_rc_repeat_generic_fw);
	//----------------------------------
	//keysetup
	ks->addItem(GenericMenuSeparatorLine);
	ks->addItem(ks_rc_fw);
	//----------------------------------
		//show mode change item
		ks_rc->addIntroItems(LOCALE_KEYBINDINGMENU, LOCALE_KEYBINDINGMENU_MODECHANGE);
		ks_rc->addItem(ks_mc_fw);
		//----------------------------------
		//show channellist items
		ks_rc->addItem(ks_cl_sep);
		ks_rc->addItem(ks_cl_oj);
		for (int i = VIRTUALKEY_PAGE_UP; i <= VIRTUALKEY_RELOAD; i++)
			ks_rc->addItem(new CMenuForwarder(keydescription[i], true, keychooser[i]->getKeyName(), keychooser[i]));
		//----------------------------------
		//show quickzap items
		ks_rc->addItem(ks_qz_sep);
		for (int i = VIRTUALKEY_CHANNEL_UP; i <= VIRTUALKEY_LASTCHANNEL; i++)
			ks_rc->addItem(new CMenuForwarder(keydescription[i], true, keychooser[i]->getKeyName(), keychooser[i]));
		//----------------------------------
		//show menu navigation items
		ks_rc->addItem(ks_mn_sep);
		for (int i = VIRTUALKEY_MENU_PAGE_UP; i <= VIRTUALKEY_MENU_PAGE_DOWN; i++)
			ks_rc->addItem(new CMenuForwarder(keydescription[i], true, keychooser[i]->getKeyName(), keychooser[i]));

	int res = ks->exec(NULL, "");
	selected = ks->getSelected();
	delete ks;

	delete ks_rc;
	for (int i = 0; i < MAX_NUM_KEYNAMES; i++)
		delete keychooser[i];

	return res;
}

bool CKeybindSetup::changeNotify(const neutrino_locale_t OptionName, void *)
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_KEYBINDINGMENU_REPEATBLOCK) ||
	    ARE_LOCALES_EQUAL(OptionName, LOCALE_KEYBINDINGMENU_REPEATBLOCKGENERIC))
	{
		g_RCInput->setRepeat(g_settings.repeat_blocker, g_settings.repeat_genericblocker);
	}
	return false;
}
