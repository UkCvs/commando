/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

#include <sstream>

#include <global.h>
#include <neutrino.h>
#include <gui/widget/icons.h>
#include <gui/widget/menue.h>

#include <gui/audio_select.h>
#include <gui/audio_setup.h>



//
//  -- AUDIO Selector Menue Handler Class
//  -- to be used for calls from Menue
//  -- (2005-08-31 rasc)
// 



// -- this is a copy from neutrino.cpp!!
#define AUDIOMENU_ANALOGOUT_OPTION_COUNT 3
const CMenuOptionChooser::keyval AUDIOMENU_ANALOGOUT_OPTIONS[AUDIOMENU_ANALOGOUT_OPTION_COUNT] =
{
	{ 0, LOCALE_AUDIOMENU_STEREO    },
	{ 1, LOCALE_AUDIOMENU_MONOLEFT  },
	{ 2, LOCALE_AUDIOMENU_MONORIGHT }
};



int CAudioSelectMenuHandler::exec(CMenuTarget* parent, const std::string &actionKey)
{
	int           res = menu_return::RETURN_EXIT_ALL;

	if (parent) {
		parent->hide();
	}

	if (actionKey != "-1")
	{
		std::istringstream iss(actionKey);
		std::string type = "";
		iss >> type;

		if (type == "AUD:")
		{
			unsigned int sel = 0;
			iss >> sel;
			if (g_RemoteControl->current_PIDs.PIDs.selected_apid != sel)
				g_RemoteControl->setAPID(sel);
		}
		else if (type == "TTX:")
		{
			int vtxtpage = 0;
			iss >> std::hex >> vtxtpage;
			g_PluginList->startPlugin("tuxtxt", 0, vtxtpage);
		}
		else if (type == "DVB:")
		{
			int subpid = 0;
			iss >> subpid;
			CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
			g_PluginList->startPlugin("dvbsub", subpid);
		}

		return res;
	}

	res = doMenu();
	return res;
}



int CAudioSelectMenuHandler::doMenu()
{
	CMenuWidget AudioSelector(LOCALE_APIDSELECTOR_HEAD, NEUTRINO_ICON_AUDIO, 360);
	AudioSelector.addItem(GenericMenuSeparator);
	AudioSelector.addItem(GenericMenuCancel);

	unsigned int numberOfAPIDs = g_RemoteControl->current_PIDs.APIDs.size();
	unsigned int numberOfSubPIDs = g_RemoteControl->current_PIDs.SubPIDs.size();
	unsigned int digit = 0;

	// -- setup menue due to Audio PIDs
	if (numberOfAPIDs > 1) 
	{
		AudioSelector.addItem(GenericMenuSeparatorLine);

		for (unsigned int i = 0; i < numberOfAPIDs; i++) 
		{
			std::ostringstream actionKey;
			actionKey << "AUD: " << i;

			CMenuForwarder* fw = new CMenuForwarder(
					g_RemoteControl->current_PIDs.APIDs[i].desc,
					true, NULL, this, actionKey.str().c_str(),
					CRCInput::convertDigitToKey(++digit));
			fw->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);

			AudioSelector.addItem(fw, (i == g_RemoteControl->current_PIDs.PIDs.selected_apid));
		}
	}

	// -- setup menue for to Dual Channel Stereo
	CAudioSetupNotifier* audioSetupNotifier = NULL;
	if (g_settings.audio_left_right_selectable) {

	   AudioSelector.addItem(GenericMenuSeparatorLine);

	   audioSetupNotifier = new CAudioSetupNotifier();
	   CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_ANALOGOUT,
				&g_settings.audio_AnalogMode,
				AUDIOMENU_ANALOGOUT_OPTIONS, AUDIOMENU_ANALOGOUT_OPTION_COUNT,
				true, audioSetupNotifier);

	   AudioSelector.addItem( oj );

	}

	// -- setup menue due to Subtitle PIDs
	if (numberOfSubPIDs > 0)
	{
		AudioSelector.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_SUBTITLES_HEAD));

		bool hasTuxtxtPlugin = g_PluginList->hasPlugin("tuxtxt");
		bool hasDvbsubPlugin = g_PluginList->hasPlugin("dvbsub");

		std::string text = "";
		bool active = false;

		for (unsigned int i = 0; i < numberOfSubPIDs; i++)
		{
			std::ostringstream actionKey;

			if (g_RemoteControl->current_PIDs.SubPIDs[i].pid == g_RemoteControl->current_PIDs.PIDs.vtxtpid)
			{
				text.assign("TTX: ");
				active = hasTuxtxtPlugin;
				actionKey << "TTX: " << g_RemoteControl->current_PIDs.SubPIDs[i].composition_page;
			}
			else
			{
				text.assign("DVB: ");
				active = hasDvbsubPlugin;
				actionKey << "DVB: " << g_RemoteControl->current_PIDs.SubPIDs[i].pid;
			}
			text.append(getISO639Description(g_RemoteControl->current_PIDs.SubPIDs[i].desc));

			CMenuForwarder* fw = new CMenuForwarder(text.c_str(),
					active, NULL, this, actionKey.str().c_str(),
					CRCInput::convertDigitToKey(++digit));
			fw->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);

			AudioSelector.addItem(fw);
		}
	}

	int res = AudioSelector.exec(NULL, "");
	delete audioSetupNotifier;
	return res;
}



