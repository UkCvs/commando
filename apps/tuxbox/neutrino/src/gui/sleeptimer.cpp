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

/*
 * Code Konflikt - will be included in timerdaemon...
 * so this module will be removed...
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/sleeptimer.h>

#include <global.h>

#include <gui/widget/stringinput.h>
#include <system/helper.h>

#include <stdlib.h>

//
// -- Input Widget for setting shutdown time
// -- Menue Handler Interface
// -- to fit the MenueClasses from McClean
//

int CSleepTimerWidget::exec(CMenuTarget* parent, const std::string &)
{
	int    res = menu_return::RETURN_EXIT_ALL;
	CStringInput  *inbox;

	if (parent)
	{
		parent->hide();
	}
   
	shutdown_min = g_Timerd->getSleepTimerRemaining();  // remaining shutdown time?
	sprintf(value, "%03d", shutdown_min);
	if (shutdown_min == 0)  // no timer set
	{
		if (g_settings.sleeptimer_min == 0)
		{
			CSectionsdClient::CurrentNextInfo info_CurrentNext;
			g_InfoViewer->getEPG(g_RemoteControl->current_channel_id, info_CurrentNext);
			if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
			{
				time_t jetzt = time(NULL);
				int current_epg_zeit_dauer_rest = (info_CurrentNext.current_zeit.dauer + 150 - (jetzt - info_CurrentNext.current_zeit.startzeit )) / 60;
				if (current_epg_zeit_dauer_rest > 0 && current_epg_zeit_dauer_rest < 1000)
				{
					sprintf(value, "%03d", current_epg_zeit_dauer_rest);
				}
			}
		}
		else
			sprintf(value, "%03d", g_settings.sleeptimer_min);
	}
	inbox = new CStringInput(LOCALE_SLEEPTIMERBOX_TITLE, value, 3, LOCALE_SLEEPTIMERBOX_HINT1, LOCALE_SLEEPTIMERBOX_HINT2, "0123456789 ", this, NEUTRINO_ICON_TIMER);
	int ret = inbox->exec (NULL, "");

	delete inbox;

	/* exit pressed, cancel timer setup */
	if(ret == menu_return::RETURN_REPAINT)
	{
		return ret;
	}
	return res;
}

bool CSleepTimerWidget::changeNotify(const neutrino_locale_t, void *)
{
	if(shutdown_min!=atoi(value))
	{
		shutdown_min = atoi (value);
		printf("sleeptimer min: %d\n",shutdown_min);
		if (shutdown_min == 0)			// if set to zero remove existing sleeptimer
		{
			int timer_id = g_Timerd->getSleeptimerID();
			if (timer_id > 0)
				g_Timerd->removeTimerEvent(timer_id);
		}
		else							// set the sleeptimer to actual time + shutdown mins and announce 1 min before
		{
			time_t now = time(NULL);
			g_Timerd->setSleeptimer(now + (shutdown_min - 1) * 60, now + shutdown_min * 60);
		}
	}
	return false;
}

const char * CSleepTimerWidget::getTargetValue()
{
#if 0 // socket communication too slow, so sleeptimer item in main menu is painted very slow :-(
	shutdown_min = g_Timerd->getSleepTimerRemaining();
#else
	shutdown_min = 0;
#endif
	if (shutdown_min > 0)
	{
		shutdown_min_string = to_string(shutdown_min) + " " + g_Locale->getText(LOCALE_WORD_MINUTES_SHORT);
		return shutdown_min_string.c_str();
	}
	return NULL;
}
