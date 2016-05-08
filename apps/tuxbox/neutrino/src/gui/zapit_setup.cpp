/*
	$Id: zapit_setup.cpp,v 1.14 2012/09/23 08:16:48 rhabarber1848 Exp $

	zapit setup menue - Neutrino-GUI

	Copyright (C) 2009/2010 Tuxbox-Team

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

#include "gui/zapit_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/stringinput.h>
#include <gui/widget/dirchooser.h>

#include <driver/screen_max.h>
#include <system/debug.h>
#include <system/setting_helpers.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <errno.h>
#include <libnet.h>
#include <signal.h>
#include <sys/vfs.h> // statfs
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

using namespace std;

CZapitSetup::CZapitSetup(const neutrino_locale_t title, const char * const IconName)
{
	menue_title = title;
	menue_icon = IconName;

	width = w_max (550, 100);
	selected = -1;

	/* These variables need to be defined outside InitZapitSettings,
   	otherwise locale "INTERNAL ERROR - PLEASE REPORT" is displayed
   	instead of the option values */
	remainingChannelsBouquet = 0;
	saveaudiopids = 0;
	savelastchannel = 0;
	uncommitted_switch = 0;
}

CZapitSetup::~CZapitSetup()
{
}

int CZapitSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	if(actionKey == "zapit_starttv")
	{
		res = InitZapitChannelHelper(CZapitClient::MODE_TV);
		return res;
	}
	else if(actionKey == "zapit_startradio")
	{
		res = InitZapitChannelHelper(CZapitClient::MODE_RADIO);
		return res;
	}
	// set new start channel settings for...
	else if (strncmp(actionKey.c_str(), "ZCT:", 4) == 0 || strncmp(actionKey.c_str(), "ZCR:", 4) == 0)
	{
		int delta;
		unsigned int cnr;
		
		sscanf(&(actionKey[4]),"%u" "%n", &cnr, &delta);
		
		if (strncmp(actionKey.c_str(), "ZCT:", 4) == 0)//...tv
		{
			g_Zapit->setStartChannelTV(cnr-1);
			strcpy(CstartChannelTV,g_Zapit->getChannelNrName(g_Zapit->getStartChannelTV(), CZapitClient::MODE_TV).c_str());
		}
		else if (strncmp(actionKey.c_str(), "ZCR:", 4) == 0)//...radio
		{
			g_Zapit->setStartChannelRadio(cnr-1);
			strcpy(CstartChannelRadio,g_Zapit->getChannelNrName(g_Zapit->getStartChannelRadio(), CZapitClient::MODE_RADIO).c_str());
		}
		
		// ...leave bouquet/channel menu and show a refreshed zapit menu with current start channel(s)
		g_RCInput->postMsg(CRCInput::RC_timeout, 0);
		return menu_return::RETURN_EXIT;	
	}

	res = Init();

	return res;
}

// init menue
int CZapitSetup::Init()
{
	printf("[neutrino] init zapit menu\n");
	strcpy(CstartChannelRadio,g_Zapit->getChannelNrName(g_Zapit->getStartChannelRadio(), CZapitClient::MODE_RADIO).c_str());
	strcpy(CstartChannelTV,g_Zapit->getChannelNrName(g_Zapit->getStartChannelTV(), CZapitClient::MODE_TV).c_str());
	int res = showSetup();
	return res;
}


#define ZAPITSETTINGS_UNCOMMITTED_SWITCH_MODE_OPTION_COUNT 3
const CMenuOptionChooser::keyval ZAPITSETTINGS_UNCOMMITTED_SWITCH_MODE_OPTIONS[ZAPITSETTINGS_UNCOMMITTED_SWITCH_MODE_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF                          },
	{ 1, LOCALE_ZAPITCONFIG_UNCOMMITTED_SWITCH_MODE1 },
	{ 2, LOCALE_ZAPITCONFIG_UNCOMMITTED_SWITCH_MODE2 }
};

int CZapitSetup::showSetup()
{
	//init
	CMenuWidget * z = new CMenuWidget(menue_title, menue_icon, width);
	z->setPreselected(selected);

	//save last channel on/off
	savelastchannel = g_Zapit->getSaveLastChannel() ? 1 : 0;

	//last tv-channel
	CMenuForwarder *c1 = new CMenuForwarder(LOCALE_ZAPITCONFIG_START_TV, !savelastchannel, CstartChannelTV, this, "zapit_starttv");
	//last radio-channel
	CMenuForwarder *c2 = new CMenuForwarder(LOCALE_ZAPITCONFIG_START_RADIO, !savelastchannel, CstartChannelRadio, this, "zapit_startradio");

	CZapitSetupNotifier zapitSetupNotifier(c1, c2);
	CMenuOptionChooser *c0 = new CMenuOptionChooser(LOCALE_ZAPITCONFIG_SAVE_LAST_CHANNEL, &savelastchannel, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, &zapitSetupNotifier);

	//save audio pids
	saveaudiopids = g_Zapit->getSaveAudioPIDs() ? 1 : 0;
	CMenuOptionChooser *c4 = new CMenuOptionChooser(LOCALE_ZAPITCONFIG_SAVE_AUDIO_PID, &saveaudiopids, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, &zapitSetupNotifier);

	//bouquet other on/off
	remainingChannelsBouquet = g_Zapit->getRemainingChannelsBouquet() ? 1 : 0;
	CMenuOptionChooser *c5 = new CMenuOptionChooser(LOCALE_ZAPITCONFIG_REMAINING_BOUQUET, &remainingChannelsBouquet, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, &zapitSetupNotifier);

	//uncommitted_switch on/off
	CMenuOptionChooser *c6 = NULL;
	if(g_info.delivery_system == DVB_S)
	{
		uncommitted_switch = g_Zapit->getUncommittedSwitchMode();
		c6 = new CMenuOptionChooser(LOCALE_ZAPITCONFIG_UNCOMMITTED_SWITCH, &uncommitted_switch, ZAPITSETTINGS_UNCOMMITTED_SWITCH_MODE_OPTIONS, ZAPITSETTINGS_UNCOMMITTED_SWITCH_MODE_OPTION_COUNT, true, &zapitSetupNotifier);
	}

	//-----------------------------------
	z->addIntroItems(menue_title != LOCALE_ZAPITCONFIG_HEAD ? LOCALE_ZAPITCONFIG_HEAD : NONEXISTANT_LOCALE);
	//-----------------------------------
	z->addItem(c0);			//save last channel on/off
	z->addItem(c1);			//last tv-channel
	z->addItem(c2);			//last radio-channel
	z->addItem(c4);			//save audio pids
	z->addItem(c5);			//bouquet other on/off
	if(c6 != NULL)
		z->addItem(c6);		//uncommitted_switch on/off

	int res = z->exec(NULL, "");
	selected = z->getSelected();
	delete z;

	return res;
}

int CZapitSetup::InitZapitChannelHelper(CZapitClient::channelsMode mode)
{
	std::vector<CMenuWidget *> toDelete;
	CZapitClient zapit;
	CZapitClient::BouquetList bouquetlist;
	zapit.getBouquets(bouquetlist, false, true, mode); // UTF-8
	CZapitClient::BouquetList::iterator bouquet = bouquetlist.begin();
	CMenuWidget mctv(LOCALE_TIMERLIST_BOUQUETSELECT, NEUTRINO_ICON_SETTINGS, width);
	mctv.addIntroItems(NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, CMenuWidget::BTN_TYPE_CANCEL);

	for(; bouquet != bouquetlist.end(); ++bouquet)
	{
		CMenuWidget* mwtv = new CMenuWidget(LOCALE_TIMERLIST_CHANNELSELECT, NEUTRINO_ICON_SETTINGS, width);
		toDelete.push_back(mwtv);
		CZapitClient::BouquetChannelList channellist;
		zapit.getBouquetChannels(bouquet->bouquet_nr,channellist,mode, true); // UTF-8
		CZapitClient::BouquetChannelList::iterator channel = channellist.begin();
		mwtv->addIntroItems();
		for(; channel != channellist.end(); ++channel)
		{
			char cChannelId[32];
			sprintf(cChannelId,
				"ZC%c:%d,",
				(mode==CZapitClient::MODE_TV)?'T':'R',
				channel->nr);
			CMenuForwarder * chan_item = new CMenuForwarder(channel->name, true, NULL, this, (std::string(cChannelId) + channel->name).c_str());
			chan_item->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);
			mwtv->addItem(chan_item);
		}
		if (!channellist.empty())
			mctv.addItem(new CMenuForwarder(bouquet->name, true, NULL, mwtv));
	}

	int res = mctv.exec(NULL, "");

	// delete dynamic created objects
	for(unsigned int count=0;count<toDelete.size();count++)
		delete toDelete[count];

	return res;
}

CZapitSetupNotifier::CZapitSetupNotifier(CMenuForwarder* i1, CMenuForwarder* i2)
{
	toDisable[0]=i1;
	toDisable[1]=i2;
}

bool CZapitSetupNotifier::changeNotify(const neutrino_locale_t OptionName, void * data)
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_ZAPITCONFIG_REMAINING_BOUQUET))
	{
		g_Zapit->setRemainingChannelsBouquet((*((int *)data)) != 0);
		CNeutrinoApp::getInstance()->exec(NULL, "reloadchannels");
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_ZAPITCONFIG_SAVE_LAST_CHANNEL))
	{
		g_Zapit->setSaveLastChannel((*((int *)data)) != 0);
		if((*((int *)data)) == 0)
		{
			for (int i=0; i<2; i++)
			{
				toDisable[i]->setActive(true);
			}
		}
		else
		{
			for (int i=0; i<2; i++)
			{
				toDisable[i]->setActive(false);
			}
		}
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_ZAPITCONFIG_SAVE_AUDIO_PID))
	{
		g_Zapit->setSaveAudioPIDs((*((int *)data)) != 0);
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_ZAPITCONFIG_UNCOMMITTED_SWITCH))
	{
		g_Zapit->setUncommittedSwitchMode(*(int *)data);
	}
	return false;
}

