/*
	$Id: audio_setup.cpp,v 1.15 2012/09/12 07:25:12 rhabarber1848 Exp $

	audio setup implementation - Neutrino-GUI

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


#include "gui/audio_setup.h"
#include "gui/audiopriority.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>

#include <driver/screen_max.h>

#include <system/debug.h>



CAudioSetup::CAudioSetup()
{
	width = w_max (500, 100);
	selected = -1;
}

CAudioSetup::~CAudioSetup()
{

}

int CAudioSetup::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init audio setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	res = showAudioSetup();
	
	return res;
}


#define AUDIOMENU_ANALOGOUT_OPTION_COUNT 3
const CMenuOptionChooser::keyval AUDIOMENU_ANALOGOUT_OPTIONS[AUDIOMENU_ANALOGOUT_OPTION_COUNT] =
{
	{ 0, LOCALE_AUDIOMENU_STEREO    },
	{ 1, LOCALE_AUDIOMENU_MONOLEFT  },
	{ 2, LOCALE_AUDIOMENU_MONORIGHT }
};

#ifdef HAVE_DBOX_HARDWARE
#ifdef ENABLE_LIRC
#define AUDIOMENU_AVS_CONTROL_OPTION_COUNT 3
#else
#define AUDIOMENU_AVS_CONTROL_OPTION_COUNT 2
#endif
const CMenuOptionChooser::keyval AUDIOMENU_AVS_CONTROL_OPTIONS[AUDIOMENU_AVS_CONTROL_OPTION_COUNT] =
{
	{ CControld::TYPE_OST , LOCALE_AUDIOMENU_OST  },
	{ CControld::TYPE_AVS , LOCALE_AUDIOMENU_AVS  }
#ifdef ENABLE_LIRC
	,{ CControld::TYPE_LIRC, LOCALE_AUDIOMENU_LIRC }
#endif
};
#endif
#ifdef HAVE_TRIPLEDRAGON
#define AUDIOMENU_AVS_CONTROL_OPTION_COUNT 2
const CMenuOptionChooser::keyval AUDIOMENU_AVS_CONTROL_OPTIONS[AUDIOMENU_AVS_CONTROL_OPTION_COUNT] =
{
	{CControld::TYPE_OST, LOCALE_AUDIOMENU_OST},
	{CControld::TYPE_AVS, LOCALE_AUDIOMENU_AVS}
};
#endif

#define AUDIOMENU_LEFT_RIGHT_SELECTABLE_OPTION_COUNT 2
const CMenuOptionChooser::keyval AUDIOMENU_LEFT_RIGHT_SELECTABEL_OPTIONS[AUDIOMENU_LEFT_RIGHT_SELECTABLE_OPTION_COUNT] =
{
	{ 1, LOCALE_OPTIONS_ON  },
	{ 0, LOCALE_OPTIONS_OFF }
};

#define AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_COUNT 2
const CMenuOptionChooser::keyval AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_OPTIONS[AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_COUNT] =
{
	{ 1, LOCALE_OPTIONS_ON  },
	{ 0, LOCALE_OPTIONS_OFF }
};

#define AUDIOMENU_AUDIOPRIORITY_OFF_ON_OPTION_COUNT 2
const CMenuOptionChooser::keyval AUDIOMENU_AUDIOPRIORITY_OFF_ON_OPTIONS[AUDIOMENU_AUDIOPRIORITY_OFF_ON_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF  },
	{ 1, LOCALE_OPTIONS_ON }
};

#define AUDIOMENU_RESYNC_OPTION_COUNT 3
const CMenuOptionChooser::keyval AUDIOMENU_RESYNC_OPTIONS[AUDIOMENU_RESYNC_OPTION_COUNT] =
{
	{ 0, LOCALE_AUDIOMENU_RESYNC_OFF },
	{ 1, LOCALE_AUDIOMENU_RESYNC_ON  },
	{ 2, LOCALE_AUDIOMENU_RESYNC_REPEAT }
};

/* audio settings menu */
int CAudioSetup::showAudioSetup()
{
	CAudioSetupNotifier audioSetupNotifier;

	//menue init
	CMenuWidget* audioSettings = new CMenuWidget(LOCALE_MAINSETTINGS_HEAD, NEUTRINO_ICON_AUDIO, width);
	audioSettings->setPreselected(selected);

	// intros
	audioSettings->addIntroItems(LOCALE_MAINSETTINGS_AUDIO);

	CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_ANALOGOUT, &g_settings.audio_AnalogMode, AUDIOMENU_ANALOGOUT_OPTIONS, AUDIOMENU_ANALOGOUT_OPTION_COUNT, true, &audioSetupNotifier);
	audioSettings->addItem( oj );

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AUDIO_LEFT_RIGHT_SELECTABLE, &g_settings.audio_left_right_selectable, AUDIOMENU_LEFT_RIGHT_SELECTABEL_OPTIONS, AUDIOMENU_LEFT_RIGHT_SELECTABLE_OPTION_COUNT, true);
	audioSettings->addItem( oj );

	audioSettings->addItem(GenericMenuSeparatorLine);

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE, &g_settings.audiochannel_up_down_enable, AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_OPTIONS, AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_COUNT, true);
	audioSettings->addItem( oj );

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_DOLBYDIGITAL, &g_settings.audio_DolbyDigital, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);
	audioSettings->addItem(oj);
	
	audioSettings->addItem(GenericMenuSeparatorLine);

#ifdef HAVE_DBOX_HARDWARE
	CMenuOptionNumberChooser *mf = new CMenuOptionNumberChooser(LOCALE_AUDIOMENU_PCMOFFSET, &g_settings.audio_PCMOffset, (g_settings.audio_avs_Control == CControld::TYPE_LIRC), 0, 99, 0, 0, NONEXISTANT_LOCALE, NULL, &audioSetupNotifier, CRCInput::RC_nokey, "", true);
	CAudioSetupNotifier2 audioSetupNotifier2(mf);

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AVS_CONTROL, &g_settings.audio_avs_Control, AUDIOMENU_AVS_CONTROL_OPTIONS, AUDIOMENU_AVS_CONTROL_OPTION_COUNT, true, &audioSetupNotifier2);
	audioSettings->addItem(oj);
	audioSettings->addItem(mf);
#endif
#ifdef HAVE_TRIPLEDRAGON
	CAudioSetupNotifier2 audioSetupNotifier2(NULL);
	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AVS_CONTROL, &g_settings.audio_avs_Control, AUDIOMENU_AVS_CONTROL_OPTIONS, AUDIOMENU_AVS_CONTROL_OPTION_COUNT, true, &audioSetupNotifier2);
	audioSettings->addItem(oj);
#endif
	
	// volume bar steps
	CMenuOptionNumberChooser *as = new CMenuOptionNumberChooser(LOCALE_AUDIOMENU_VOLUMEBAR_AUDIOSTEPS, &g_settings.audio_step, true, 0, 25, 0, 0, NONEXISTANT_LOCALE, NULL, NULL, CRCInput::RC_nokey, "", true);
	audioSettings->addItem(as);

	// initial volume
	CMenuOptionNumberChooser *iv = new CMenuOptionNumberChooser(LOCALE_AUDIOMENU_INITIAL_VOLUME, &g_settings.audio_initial_volume, true, 0, 100, 0, 0, LOCALE_AUDIOMENU_INITIAL_VOLUME_RESTORE, NULL, NULL, CRCInput::RC_nokey, "", true);
	iv->setNumberFormat("%d%%");
	audioSettings->addItem(iv);

	audioSettings->addItem(GenericMenuSeparatorLine);

	// Audio auto AV ReSync
	CMenuOptionNumberChooser *rt = new CMenuOptionNumberChooser(LOCALE_AUDIOMENU_RESYNC_TIMER, &g_settings.audio_ReSync_timer, (g_settings.audio_ReSync == 2), 5, 180, 0, 0, NONEXISTANT_LOCALE, NULL, &audioSetupNotifier, CRCInput::RC_nokey, "", true);
	CAudioSetupNotifier4 audioSetupNotifier4(rt);
	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_RESYNC, &g_settings.audio_ReSync, AUDIOMENU_RESYNC_OPTIONS, AUDIOMENU_RESYNC_OPTION_COUNT, true, &audioSetupNotifier4);
	audioSettings->addItem(oj);
	audioSettings->addItem(rt);

	// Audio Priority Pids
	CMenuForwarder *ap = new CMenuForwarder(LOCALE_AUDIOPRIORITY_SETTINGS, g_settings.audio_propids_enabled, NULL, new audioprioritySettingsMenu());
	CAudioSetupNotifier3 audioSetupNotifier3(ap);
	oj = new CMenuOptionChooser(LOCALE_AUDIOPRIORITY_ENABLED, &g_settings.audio_propids_enabled, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, &audioSetupNotifier3);
	audioSettings->addItem(oj);
	audioSettings->addItem(ap);

	int res = audioSettings->exec(NULL, "");
	selected = audioSettings->getSelected();
	delete audioSettings;

	return res;
}

bool CAudioSetupNotifier::changeNotify(const neutrino_locale_t OptionName, void *)
{
	//printf("notify: %d\n", OptionName);

	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_PCMOFFSET))
	{
		if (g_settings.audio_avs_Control == CControld::TYPE_LIRC)
			g_Controld->setVolume(100 - g_settings.audio_PCMOffset, CControld::TYPE_OST);
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_ANALOGOUT))
	{
		g_Zapit->setAudioMode(g_settings.audio_AnalogMode);
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_RESYNC_TIMER))
	{
		CNeutrinoApp::getInstance()->SendSectionsdConfig();
	}
	return false;
}

CAudioSetupNotifier2::CAudioSetupNotifier2( CMenuItem* i1)
{
	toDisable[0]=i1;
}

bool CAudioSetupNotifier2::changeNotify(const neutrino_locale_t, void *)
{
	if (toDisable[0])
		toDisable[0]->setActive(g_settings.audio_avs_Control == CControld::TYPE_LIRC);

	if (g_settings.audio_avs_Control == CControld::TYPE_LIRC)
		g_Controld->setVolume(100 - g_settings.audio_PCMOffset, CControld::TYPE_OST);
	// tell controld the new volume_type
	g_Controld->setVolume(g_Controld->getVolume((CControld::volume_type)g_settings.audio_avs_Control),
									 (CControld::volume_type)g_settings.audio_avs_Control);
	return false;
}

CAudioSetupNotifier3::CAudioSetupNotifier3( CMenuItem* i1)
{
	toDisable[0]=i1;
}

bool CAudioSetupNotifier3::changeNotify(const neutrino_locale_t, void *)
{
	if (toDisable[0])
		toDisable[0]->setActive(g_settings.audio_propids_enabled == true);

	return false;
}

CAudioSetupNotifier4::CAudioSetupNotifier4( CMenuItem* i1)
{
	toDisable[0]=i1;
}

bool CAudioSetupNotifier4::changeNotify(const neutrino_locale_t, void *)
{
	if (toDisable[0])
		toDisable[0]->setActive(g_settings.audio_ReSync == 2);

	CNeutrinoApp::getInstance()->SendSectionsdConfig();

	return false;
}

