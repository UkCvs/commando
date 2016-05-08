/*
	$Id: subchannel_select.cpp,v 1.3 2012/09/12 07:25:12 rhabarber1848 Exp $

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

#include <global.h>
#include <neutrino.h>
#include <driver/encoding.h>
#include <gui/widget/icons.h>

#include <gui/subchannel_select.h>

int CSubChannelSelectMenu::exec(CMenuTarget* parent, const std::string &actionKey)
{
	int res = menu_return::RETURN_EXIT_ALL;

	if (parent)
		parent->hide();

	if (actionKey != "-1")
	{
		unsigned sel = atoi(actionKey.c_str());
		g_RemoteControl->setSubChannel(sel);
		g_InfoViewer->showSubchan();
		return res;
	}

	res = doMenu();
	return res;
}

int CSubChannelSelectMenu::doMenu()
{
	CMenuWidget SubChannelSelector(g_RemoteControl->are_subchannels ? LOCALE_NVODSELECTOR_SUBSERVICE : LOCALE_NVODSELECTOR_HEAD, NEUTRINO_ICON_VIDEO, 350);
	SubChannelSelector.addIntroItems(NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, CMenuWidget::BTN_TYPE_CANCEL);

	int count = 0;
	char nvod_id[5];

	for (CSubServiceListSorted::iterator e = g_RemoteControl->subChannels.begin(); e != g_RemoteControl->subChannels.end(); ++e)
	{
		sprintf(nvod_id, "%d", count);

		if (!g_RemoteControl->are_subchannels)
		{
			char nvod_time_a[50], nvod_time_e[50], nvod_time_x[50];
			char nvod_s[100];
			struct tm tmZeit;

			localtime_r(&e->startzeit, &tmZeit);
			sprintf(nvod_time_a, "%02d:%02d", tmZeit.tm_hour, tmZeit.tm_min);

			time_t endtime = e->startzeit + e->dauer;
			localtime_r(&endtime, &tmZeit);
			sprintf(nvod_time_e, "%02d:%02d", tmZeit.tm_hour, tmZeit.tm_min);

			time_t jetzt = time(NULL);
			if (e->startzeit > jetzt)
			{
				int mins=(e->startzeit- jetzt)/ 60;
				sprintf(nvod_time_x, g_Locale->getText(LOCALE_NVOD_STARTING), mins);
			}
			else if (e->startzeit <= jetzt && jetzt < endtime)
			{
				int proz = (jetzt - e->startzeit) * 100 / e->dauer;
				sprintf(nvod_time_x, g_Locale->getText(LOCALE_NVOD_PERCENTAGE), proz);
			}
			else
				nvod_time_x[0] = 0;

			sprintf(nvod_s, "%s - %s %s", nvod_time_a, nvod_time_e, nvod_time_x);
			SubChannelSelector.addItem(new CMenuForwarder(nvod_s, true, NULL, this, nvod_id), count == g_RemoteControl->selected_subchannel);
		}
		else
		{
			if (count == 0)
				SubChannelSelector.addItem(new CMenuForwarder(Latin1_to_UTF8(e->subservice_name).c_str(), true, NULL, this, nvod_id, CRCInput::RC_blue));
			else
				SubChannelSelector.addItem(new CMenuForwarder(Latin1_to_UTF8(e->subservice_name).c_str(), true, NULL, this, nvod_id, CRCInput::convertDigitToKey(count)), count == g_RemoteControl->selected_subchannel);
		}

		count++;
	}

	if (g_RemoteControl->are_subchannels)
	{
		SubChannelSelector.addItem(GenericMenuSeparatorLine);
		SubChannelSelector.addItem(new CMenuOptionChooser(LOCALE_NVODSELECTOR_DIRECTORMODE, &g_RemoteControl->director_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, NULL, CRCInput::RC_yellow));
	}

	return SubChannelSelector.exec(NULL, "");
}

