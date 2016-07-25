/*
	$Id: miscsettings_menu.cpp,v 1.15 2012/09/23 08:16:48 rhabarber1848 Exp $

	miscsettings_menu implementation - Neutrino-GUI

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

#include "miscsettings_menu.h"
#include "zapit_setup.h"
#include "filebrowser.h"
#include "keybind_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <driver/screen_max.h>

#include <system/debug.h>

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_stop();
extern "C" void tuxtxt_close();
extern "C" int  tuxtxt_init();
extern "C" int  tuxtxt_start(int tpid);
#endif

CMiscMenue::CMiscMenue(const neutrino_locale_t title, const char * const IconName)
{
	menue_title = title;
	menue_icon = IconName;

	width = w_max (500, 100);
	selected = -1;
}

CMiscMenue::~CMiscMenue()
{

}

int CMiscMenue::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init extended settings menue\n");

	if(parent != NULL)
		parent->hide();

	if(actionKey == "epgdir")
	{
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.epg_dir.c_str()))
		{
			if((b.getSelectedFile()->Name) == "/")
			{
				// if selected dir is root -> clear epg_dir
				g_settings.epg_dir = "";
			} else {
				g_settings.epg_dir = b.getSelectedFile()->Name + "/";
			}
			CNeutrinoApp::getInstance()->SendSectionsdConfig(); // update notifier
		}
		return menu_return::RETURN_REPAINT;
	}

	int res = showMenue();
	
	return res;
}

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};

#define OPTIONS_OFF1_ON0_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF1_ON0_OPTIONS[OPTIONS_OFF1_ON0_OPTION_COUNT] =
{
	{ 1, LOCALE_OPTIONS_OFF },
	{ 0, LOCALE_OPTIONS_ON  }
};

#define MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT 2
const CMenuOptionChooser::keyval MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTIONS[MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT] =
{
	{ 0, LOCALE_FILESYSTEM_IS_UTF8_OPTION_ISO8859_1 },
	{ 1, LOCALE_FILESYSTEM_IS_UTF8_OPTION_UTF8      }
};

#define REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS_COUNT 4
const CMenuOptionChooser::keyval  REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS[REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS_COUNT]=
{
	{ CNeutrinoApp::STANDBY_OFF_WITH_POWER        , LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH_POWER         },
	{ CNeutrinoApp::STANDBY_OFF_WITH_POWER_OK     , LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH_POWER_OK      },
	{ CNeutrinoApp::STANDBY_OFF_WITH_POWER_HOME   , LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH_POWER_HOME    },
	{ CNeutrinoApp::STANDBY_OFF_WITH_POWER_HOME_OK, LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH_POWER_HOME_OK }
};

#if !defined(ENABLE_AUDIOPLAYER) && !defined(ENABLE_ESD)
#define MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT 5
#else
#if !defined(ENABLE_AUDIOPLAYER) && defined(ENABLE_ESD)
#define MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT 6
#else
#if defined(ENABLE_AUDIOPLAYER) && !defined(ENABLE_ESD)
#define MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT 7
#else
#define MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT 8
#endif
#endif
#endif
const CMenuOptionChooser::keyval  MISCSETTINGS_STARTMODE_WITH_OPTIONS[MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT]=
{
   { CNeutrinoApp::STARTMODE_RESTORE    , LOCALE_MISCSETTINGS_STARTMODE_RESTORE },
   { CNeutrinoApp::STARTMODE_TV         , LOCALE_MAINMENU_TVMODE                },
   { CNeutrinoApp::STARTMODE_RADIO      , LOCALE_MAINMENU_RADIOMODE             },
   { CNeutrinoApp::STARTMODE_SCART      , LOCALE_MAINMENU_SCARTMODE             },
#ifdef ENABLE_AUDIOPLAYER
   { CNeutrinoApp::STARTMODE_AUDIOPLAYER, LOCALE_MAINMENU_AUDIOPLAYER           },
   { CNeutrinoApp::STARTMODE_INETRADIO  , LOCALE_INETRADIO_NAME                 },
#endif
#ifdef ENABLE_ESD
   { CNeutrinoApp::STARTMODE_ESOUND     , LOCALE_ESOUND_NAME                    },
#endif
   { CNeutrinoApp::STARTMODE_STANDBY    , LOCALE_TIMERLIST_TYPE_STANDBY         }
};

//show misc settings menue
int CMiscMenue::showMenue()
{
	//misc settings
	CMenuWidget *misc_menue 		= new CMenuWidget(menue_title, menue_icon, width);
	misc_menue->setPreselected(selected);

	//general
	CMenuWidget *misc_menue_energy 	= new CMenuWidget(LOCALE_MISCSETTINGS_HEAD, menue_icon, width);
	//epg
	CMenuWidget *misc_menue_epg 		= new CMenuWidget(LOCALE_MISCSETTINGS_HEAD, menue_icon, width);
	//zapit
	CZapitSetup *misc_menue_zapit = new CZapitSetup(LOCALE_MAINSETTINGS_MISC);
	//filebrowser
	CMenuWidget *misc_menue_filebrowser 	= new CMenuWidget(LOCALE_MISCSETTINGS_HEAD, menue_icon, width);

	//osd main settings, intros
	misc_menue->addIntroItems(menue_title != LOCALE_MISCSETTINGS_HEAD ? LOCALE_MISCSETTINGS_HEAD : NONEXISTANT_LOCALE);

	// general
	misc_menue->addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_RC_STANDBY_MODES, true, NULL, misc_menue_energy, NULL,  CRCInput::RC_red));	
	// epg settings
	misc_menue->addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_HEAD, true, NULL, misc_menue_epg, NULL, CRCInput::RC_green));
	// zapit settings
	misc_menue->addItem(new CMenuForwarder(LOCALE_ZAPITCONFIG_HEAD, true, NULL, misc_menue_zapit, NULL, CRCInput::RC_yellow));
	// filebrowser
	misc_menue->addItem(new CMenuForwarder(LOCALE_FILEBROWSER_HEAD, true, NULL, misc_menue_filebrowser, NULL, CRCInput::RC_blue));

	misc_menue->addItem(GenericMenuSeparatorLine);
#ifndef TUXTXT_CFG_STANDALONE
	//tutxt cache
	misc_menue->addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_TUXTXT_CACHE, &g_settings.tuxtxt_cache, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));
#endif
	// startmode
	misc_menue->addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_STARTMODE, &g_settings.startmode, MISCSETTINGS_STARTMODE_WITH_OPTIONS, MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT, true));

	
	/* misc settings sub menues */
	// general
	misc_menue_energy->addIntroItems(LOCALE_MISCSETTINGS_RC_STANDBY_MODES);

	//rc delay
	CMenuOptionChooser *m1 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_SHUTDOWN_REAL_RCDELAY, &g_settings.shutdown_real_rcdelay, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, !g_settings.shutdown_real);
	//standby with...
	CMenuOptionChooser *m5 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH, &g_settings.standby_off_with, REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS, REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS_COUNT, !g_settings.shutdown_real);
	//shutdown count
	CStringInput miscSettings_shutdown_count(LOCALE_MISCSETTINGS_SHUTDOWN_COUNT, g_settings.shutdown_count, 3, LOCALE_MISCSETTINGS_SHUTDOWN_COUNT_HINT1, LOCALE_MISCSETTINGS_SHUTDOWN_COUNT_HINT2, "0123456789 ", this);
	CMenuForwarder *m4 = new CMenuForwarder(LOCALE_MISCSETTINGS_SHUTDOWN_COUNT, !g_settings.shutdown_real, g_settings.shutdown_count, &miscSettings_shutdown_count);
	//sleeptimer minutes
	CIntInput miscSettings_sleeptimer_min(LOCALE_MISCSETTINGS_SLEEPTIMER_MIN, (int&)g_settings.sleeptimer_min, 3, LOCALE_MISCSETTINGS_SLEEPTIMER_MIN_HINT1, LOCALE_MISCSETTINGS_SLEEPTIMER_MIN_HINT2);
	CMenuForwarder *m6 = new CMenuForwarder(LOCALE_MISCSETTINGS_SLEEPTIMER_MIN, true, miscSettings_sleeptimer_min.getValue(), &miscSettings_sleeptimer_min);
#ifndef HAVE_TRIPLEDRAGON
	//standby save power
	CMenuOptionChooser *m3 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_STANDBY_SAVE_POWER, &g_settings.standby_save_power, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, !g_settings.shutdown_real);
#endif
	//sutdown real
	COnOffNotifier miscNotifier(1);
	miscNotifier.addItem(m1);
#ifndef HAVE_TRIPLEDRAGON
	miscNotifier.addItem(m3);
#endif
	miscNotifier.addItem(m4);
	miscNotifier.addItem(m5);
	CMenuOptionChooser *m2 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_SHUTDOWN_REAL, &g_settings.shutdown_real, OPTIONS_OFF1_ON0_OPTIONS, OPTIONS_OFF1_ON0_OPTION_COUNT, true, &miscNotifier);

	misc_menue_energy->addItem(m2);
#ifndef HAVE_TRIPLEDRAGON
	/* do not allow TD users to shoot themselves in the foot ;) */
	misc_menue_energy->addItem(m3);
#endif
	misc_menue_energy->addItem(m4);
	misc_menue_energy->addItem(m1);
	misc_menue_energy->addItem(m5);
	misc_menue_energy->addItem(m6);


	//epg settings
	misc_menue_epg->addIntroItems(LOCALE_MISCSETTINGS_EPG_HEAD);

#ifdef ENABLE_FREESATEPG
	//Freesat epg
	misc_menue_epg->addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_EPG_FREESAT, &g_settings.epg_freesat_enabled, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));
#endif
	//epg cache ??is this really usefull??
	CStringInput miscSettings_epg_cache(LOCALE_MISCSETTINGS_EPG_CACHE, &g_settings.epg_cache, 2, false, LOCALE_MISCSETTINGS_EPG_CACHE_HINT1, LOCALE_MISCSETTINGS_EPG_CACHE_HINT2, "0123456789 ", this);
	misc_menue_epg->addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_CACHE, true, g_settings.epg_cache, &miscSettings_epg_cache));
	//extended epg cache
	CStringInput miscSettings_epg_extendedcache(LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE, &g_settings.epg_extendedcache, 2, false, LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE_HINT1, LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE_HINT2, "0123456789 ", this);
	misc_menue_epg->addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE, true, g_settings.epg_extendedcache, &miscSettings_epg_extendedcache));
	//old events
	CStringInput miscSettings_epg_old_events(LOCALE_MISCSETTINGS_EPG_OLD_EVENTS, &g_settings.epg_old_events, 2, false, LOCALE_MISCSETTINGS_EPG_OLD_EVENTS_HINT1, LOCALE_MISCSETTINGS_EPG_OLD_EVENTS_HINT2, "0123456789 ", this);
	misc_menue_epg->addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_OLD_EVENTS, true, g_settings.epg_old_events, &miscSettings_epg_old_events));
	//max epg events
	CStringInput miscSettings_epg_max_events(LOCALE_MISCSETTINGS_EPG_MAX_EVENTS, &g_settings.epg_max_events, 5, false, LOCALE_MISCSETTINGS_EPG_MAX_EVENTS_HINT1, LOCALE_MISCSETTINGS_EPG_MAX_EVENTS_HINT2, "0123456789 ", this);
	misc_menue_epg->addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_MAX_EVENTS, true, g_settings.epg_max_events, &miscSettings_epg_max_events));
	misc_menue_epg->addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_DIR, true, g_settings.epg_dir, this, "epgdir"));


	//filebrowser
	misc_menue_filebrowser->addIntroItems(LOCALE_FILEBROWSER_HEAD);

	misc_menue_filebrowser->addItem(new CMenuOptionChooser(LOCALE_FILESYSTEM_IS_UTF8            , &g_settings.filesystem_is_utf8            , MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTIONS, MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT, true ));
	misc_menue_filebrowser->addItem(new CMenuOptionChooser(LOCALE_FILEBROWSER_SHOWRIGHTS        , &g_settings.filebrowser_showrights        , MESSAGEBOX_NO_YES_OPTIONS              , MESSAGEBOX_NO_YES_OPTION_COUNT              , true ));
	misc_menue_filebrowser->addItem(new CMenuOptionChooser(LOCALE_FILEBROWSER_DENYDIRECTORYLEAVE, &g_settings.filebrowser_denydirectoryleave, MESSAGEBOX_NO_YES_OPTIONS              , MESSAGEBOX_NO_YES_OPTION_COUNT              , true ));


	int res = misc_menue->exec(NULL, "");
	selected = misc_menue->getSelected();
	delete misc_menue;

	delete misc_menue_energy;
	delete misc_menue_epg;
	delete misc_menue_zapit;
	delete misc_menue_filebrowser;

	return res;
}

bool CMiscMenue::changeNotify(const neutrino_locale_t OptionName, void *)
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_MISCSETTINGS_SHUTDOWN_COUNT))
	{
		printf("[neutrino] shutdown counter changed to %d minutes\n", atoi(g_settings.shutdown_count));
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_MISCSETTINGS_EPG_CACHE) ||
	         ARE_LOCALES_EQUAL(OptionName, LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE) ||
	         ARE_LOCALES_EQUAL(OptionName, LOCALE_MISCSETTINGS_EPG_OLD_EVENTS) ||
#ifdef ENABLE_FREESATEPG
	         ARE_LOCALES_EQUAL(OptionName, LOCALE_MISCSETTINGS_EPG_FREESAT) ||
#endif
	         ARE_LOCALES_EQUAL(OptionName, LOCALE_MISCSETTINGS_EPG_MAX_EVENTS))
	{
		CNeutrinoApp::getInstance()->SendSectionsdConfig();
	}
#ifndef TUXTXT_CFG_STANDALONE
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_MISCSETTINGS_TUXTXT_CACHE))
	{
		int vtpid = g_RemoteControl->current_PIDs.PIDs.vtxtpid;

		if (g_settings.tuxtxt_cache)
		{
			tuxtxt_init();
			if (vtpid)
				tuxtxt_start(vtpid);
		}
		else
		{
			tuxtxt_stop();
			tuxtxt_close();
		}
	}
#endif
	return false;
}

