/*
	$Id: record_setup.cpp,v 1.22 2012/09/12 07:25:12 rhabarber1848 Exp $

	record setup implementation - Neutrino-GUI

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


#include "gui/record_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/dirchooser.h>
#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <driver/screen_max.h>

#include <system/debug.h>


CRecordSetup::CRecordSetup()
{
	width = w_max (500, 100);
	selected = -1;
}

CRecordSetup::~CRecordSetup()
{

}

int CRecordSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init record setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	if(actionKey=="recording")
	{
		CNeutrinoApp::getInstance()->setupRecordingDevice();
		return res;
	}
	else if(actionKey == "help_recording")
	{
		ShowLocalizedMessage(LOCALE_SETTINGS_HELP, LOCALE_RECORDINGMENU_HELP, CMessageBox::mbrBack, CMessageBox::mbBack);
		return res;
	}

	if (!CNeutrinoApp::getInstance()->recordingstatus)
		res = showRecordSetup();
	else
		DisplayInfoMessage(g_Locale->getText(LOCALE_RECORDINGMENU_RECORD_IS_RUNNING));
	
	return res;
}

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};

#ifdef HAVE_TRIPLEDRAGON
#define RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT 3
#else
#define RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT 4
#endif
const CMenuOptionChooser::keyval RECORDINGMENU_RECORDING_TYPE_OPTIONS[RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT] =
{
	{ CNeutrinoApp::RECORDING_OFF   , LOCALE_RECORDINGMENU_OFF    },
#ifndef HAVE_TRIPLEDRAGON
	{ CNeutrinoApp::RECORDING_SERVER, LOCALE_RECORDINGMENU_SERVER },
#endif
	{ CNeutrinoApp::RECORDING_VCR   , LOCALE_RECORDINGMENU_VCR    },
	{ CNeutrinoApp::RECORDING_FILE  , LOCALE_RECORDINGMENU_FILE   }
};

#define RECORDINGMENU_STOPSECTIONSD_OPTION_COUNT 3
const CMenuOptionChooser::keyval RECORDINGMENU_STOPSECTIONSD_OPTIONS[RECORDINGMENU_STOPSECTIONSD_OPTION_COUNT] =
{
	{ CNeutrinoApp::SECTIONSD_RUN    , LOCALE_RECORDINGMENU_SECTIONSD_RUN     },
	{ CNeutrinoApp::SECTIONSD_STOP   , LOCALE_RECORDINGMENU_SECTIONSD_STOP    },
	{ CNeutrinoApp::SECTIONSD_RESTART, LOCALE_RECORDINGMENU_SECTIONSD_RESTART }
};

#define RECORDINGMENU_RINGBUFFER_SIZE_COUNT 5
const CMenuOptionChooser::keyval RECORDINGMENU_RINGBUFFER_SIZES[RECORDINGMENU_RINGBUFFER_SIZE_COUNT] =
{
	{ 0, LOCALE_RECORDINGMENU_RINGBUFFERS_05M },
	{ 1, LOCALE_RECORDINGMENU_RINGBUFFERS_1M },
	{ 2, LOCALE_RECORDINGMENU_RINGBUFFERS_2M },
	{ 3, LOCALE_RECORDINGMENU_RINGBUFFERS_4M },
	{ 4, LOCALE_RECORDINGMENU_RINGBUFFERS_8M }
};

int CRecordSetup::showRecordSetup()
{
	// dynamic created objects
	std::vector<CMenuTarget*> toDelete;

	//menue init
	CMenuWidget* recordingSettings = new CMenuWidget(LOCALE_MAINSETTINGS_HEAD, NEUTRINO_ICON_RECORDING, width);
	recordingSettings->setPreselected(selected);

	//prepare input record server ip
	CIPInput recordingSettings_server_ip(LOCALE_RECORDINGMENU_SERVER_IP, g_settings.recording_server_ip);
	//input record server ip
	CMenuForwarder * mf1 = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_IP, (g_settings.recording_type == RECORDING_SERVER), g_settings.recording_server_ip, &recordingSettings_server_ip);

	//prepare input record server port
	CStringInput recordingSettings_server_port(LOCALE_RECORDINGMENU_SERVER_PORT, g_settings.recording_server_port, 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ");
	//input record server port
	CMenuForwarder * mf2 = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_PORT, (g_settings.recording_type == RECORDING_SERVER), g_settings.recording_server_port, &recordingSettings_server_port);

	//prepare input record server mac address
	CMACInput recordingSettings_server_mac(LOCALE_RECORDINGMENU_SERVER_MAC, g_settings.recording_server_mac);
	//input record server mac address
	CMenuForwarder * mf3 = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_MAC, ((g_settings.recording_type == RECORDING_SERVER) && g_settings.recording_server_wakeup==1), g_settings.recording_server_mac, &recordingSettings_server_mac);

	COnOffNotifier wolOnOffNotifier;
	wolOnOffNotifier.addItem(mf3);

	//prepare choose wol
	CMenuOptionChooser * oj2 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_SERVER_WAKEUP, &g_settings.recording_server_wakeup, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER), &wolOnOffNotifier);

	//prepare playback stop
	CMenuOptionChooser* oj3 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STOPPLAYBACK, &g_settings.recording_stopplayback, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE));

	//prepare epg stop
	CMenuOptionChooser* oj4 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_SECTIONSD, &g_settings.recording_stopsectionsd, RECORDINGMENU_STOPSECTIONSD_OPTIONS, RECORDINGMENU_STOPSECTIONSD_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE));

	//prepare zap on announce
	CMenuOptionChooser* oj4b = new CMenuOptionChooser(LOCALE_RECORDINGMENU_ZAP_ON_ANNOUNCE, &g_settings.recording_zap_on_announce, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	//prepare no scart switch
	CMenuOptionChooser* oj5 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_NO_SCART, &g_settings.recording_vcr_no_scart, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_VCR));

	//prepare record in spts mode
	CMenuOptionChooser* oj12 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RECORD_IN_SPTS_MODE, &g_settings.recording_in_spts_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT,(g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE) );

	//prepare record correcture
	int rec_pre,rec_post;
	g_Timerd->getRecordingSafety(rec_pre,rec_post);
	sprintf(record_safety_time_before, "%02d", rec_pre/60);
	sprintf(record_safety_time_after, "%02d", rec_post/60);

	//timersettings submenue
	CMenuWidget *timerRecordingSettings = new CMenuWidget(LOCALE_MAINSETTINGS_RECORDING, NEUTRINO_ICON_TIMER, width);
	toDelete.push_back(timerRecordingSettings);
	CMenuForwarder* mf15 = new CMenuForwarder(LOCALE_TIMERSETTINGS_SEPARATOR ,true, NULL, timerRecordingSettings, NULL, CRCInput::RC_yellow);

	//prepare time before
	CStringInput timerSettings_record_safety_time_before(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE, record_safety_time_before, 2, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE_HINT_1, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE_HINT_2,"0123456789 ", this);
	CMenuForwarder *mf5 = new CMenuForwarder(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE, true, record_safety_time_before, &timerSettings_record_safety_time_before);

	//prepare time after
	CStringInput timerSettings_record_safety_time_after(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER, record_safety_time_after, 2, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER_HINT_1, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER_HINT_2,"0123456789 ", this);
	CMenuForwarder *mf6 = new CMenuForwarder(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER, true, record_safety_time_after, &timerSettings_record_safety_time_after);

	//prepare zap to before
	int zapto_pre;
	g_Timerd->getZaptoSafety(zapto_pre);
	sprintf(zapto_safety_time_before, "%02d", zapto_pre/60);
	CStringInput timerSettings_zapto_safety_time_before(LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE, zapto_safety_time_before, 2, LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE_HINT_1, LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE_HINT_2,"0123456789 ", this);
	CMenuForwarder *mf14 = new CMenuForwarder(LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE, true, zapto_safety_time_before, &timerSettings_zapto_safety_time_before);

	// default recording audio pids
	CMenuWidget *apidRecordingSettings = new CMenuWidget(LOCALE_MAINSETTINGS_RECORDING, NEUTRINO_ICON_AUDIO, width);
	toDelete.push_back(apidRecordingSettings);
	CMenuForwarder* mf13 = new CMenuForwarder(LOCALE_RECORDINGMENU_APIDS, true, NULL, apidRecordingSettings, NULL, CRCInput::RC_blue);

	recording_audio_pids_std = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_STD ) ? 1 : 0 ;
	recording_audio_pids_alt = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_ALT ) ? 1 : 0 ;
	recording_audio_pids_ac3 = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_AC3 ) ? 1 : 0 ;

	//prepare audio pids default
	CMenuOptionChooser* aoj1 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_STD, &recording_audio_pids_std, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, this);
	//prepare audio pids alternate
	CMenuOptionChooser* aoj2 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_ALT, &recording_audio_pids_alt, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, this);
	//prepare audio pids ac3
	CMenuOptionChooser* aoj3 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_AC3, &recording_audio_pids_ac3, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, this);

	apidRecordingSettings->addIntroItems(LOCALE_RECORDINGMENU_APIDS);
	apidRecordingSettings->addItem(aoj1);
	apidRecordingSettings->addItem(aoj2);
	apidRecordingSettings->addItem(aoj3);

	// directory menue for direct recording settings
	CMenuWidget *dirMenu = new CMenuWidget(LOCALE_RECORDINGMENU_FILESETTINGS, NEUTRINO_ICON_RECORDING, width);
	toDelete.push_back(dirMenu);
	dirMenu->addIntroItems(LOCALE_RECORDINGMENU_DEFDIR);
	char temp[10];
	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		// directory menu item
		snprintf(temp, 10, "%d:", i + 1);
		temp[9] = 0; // terminate for sure
		CMenuWidget* dirRecordingSettings = new CMenuWidget(LOCALE_RECORDINGMENU_FILESETTINGS, NEUTRINO_ICON_RECORDING, width);
		toDelete.push_back(dirRecordingSettings);
		dirMenu->addItem(new CMenuForwarder(temp, true, g_settings.recording_dir[i], dirRecordingSettings));

		// subhead
		snprintf(temp, 10, " %d", i + 1);
		temp[9] = 0; // terminate for sure
		CMenuSeparator* dirRecordingSettings_subhead = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING);
		dirRecordingSettings_subhead->setString(std::string(g_Locale->getText(LOCALE_RECORDINGMENU_DEFDIR)) + temp);
		dirRecordingSettings->addItem(dirRecordingSettings_subhead);

		// intro items
		dirRecordingSettings->addIntroItems();

		// directory
		CDirChooser* dirRecordingSettings_dirChooser = new CDirChooser(&g_settings.recording_dir[i]);
		toDelete.push_back(dirRecordingSettings_dirChooser);
		dirRecordingSettings->addItem(new CMenuForwarder(LOCALE_RECORDINGMENU_DIR, true, g_settings.recording_dir[i], dirRecordingSettings_dirChooser));

		// filename template
		CStringInput* dirRecordingSettings_filenameTemplate = new CStringInput(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, &g_settings.recording_filename_template[i], 21, false, LOCALE_RECORDINGMENU_FILENAME_TEMPLATE_HINT, LOCALE_IPSETUP_HINT_2, "%/-_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");
		toDelete.push_back(dirRecordingSettings_filenameTemplate);
		dirRecordingSettings->addItem(new CMenuForwarder(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, true, g_settings.recording_filename_template[i], dirRecordingSettings_filenameTemplate));

		// maximum file size
		CStringInput* dirRecordingSettings_splitsize = new CStringInput(LOCALE_RECORDINGMENU_SPLITSIZE, g_settings.recording_splitsize[i], 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ");
		toDelete.push_back(dirRecordingSettings_splitsize);
		dirRecordingSettings->addItem(new CMenuForwarder(LOCALE_RECORDINGMENU_SPLITSIZE, true, g_settings.recording_splitsize[i], dirRecordingSettings_splitsize));
	}

	// for direct recording
	CMenuWidget *directRecordingSettings = new CMenuWidget(LOCALE_MAINSETTINGS_RECORDING, NEUTRINO_ICON_RECORDING, width);
	toDelete.push_back(directRecordingSettings);
	CMenuForwarder* mf7 = new CMenuForwarder(LOCALE_RECORDINGMENU_FILESETTINGS, (g_settings.recording_type == RECORDING_FILE), NULL, directRecordingSettings, NULL, CRCInput::RC_green);

	CStringInput recordingSettings_splitsize(LOCALE_RECORDINGMENU_SPLITSIZE, g_settings.recording_splitsize_default, 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ");
	CMenuForwarder* mf9 = new CMenuForwarder(LOCALE_RECORDINGMENU_SPLITSIZE, true, g_settings.recording_splitsize_default, &recordingSettings_splitsize);

	CMenuOptionChooser* oj6 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_USE_O_SYNC, &g_settings.recording_use_o_sync, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj7 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_USE_FDATASYNC, &g_settings.recording_use_fdatasync, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj8 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STREAM_VTXT_PID, &g_settings.recording_stream_vtxt_pid, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj9 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STREAM_SUBTITLE_PID, &g_settings.recording_stream_subtitle_pid, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj13 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RINGBUFFERS, &g_settings.recording_ringbuffers, RECORDINGMENU_RINGBUFFER_SIZES, RECORDINGMENU_RINGBUFFER_SIZE_COUNT, true);

	CMenuOptionChooser* oj10 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_CHOOSE_DIRECT_REC_DIR, &g_settings.recording_choose_direct_rec_dir, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj14 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_GEN_PSI, &g_settings.recording_gen_psi, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionNumberChooser* oj15 = new CMenuOptionNumberChooser(LOCALE_RECORDINGMENU_MAX_RECTIME, &g_settings.recording_max_rectime, true, 1, 8);
	oj15->setNumberFormat("%d " + std::string(g_Locale->getText(LOCALE_WORD_HOURS_SHORT)));

	CMenuOptionChooser* oj16 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_NHD_COMPATIBLE_TS, &g_settings.recording_nhd_compatible_ts, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CStringInput recordingSettings_filenameTemplate(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, &g_settings.recording_filename_template_default, 21, false, LOCALE_RECORDINGMENU_FILENAME_TEMPLATE_HINT, LOCALE_IPSETUP_HINT_2, "%/-_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");
	CMenuForwarder* mf11 = new CMenuForwarder(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, true, g_settings.recording_filename_template_default, &recordingSettings_filenameTemplate);

	CStringInput recordingSettings_dirPermissions(LOCALE_RECORDINGMENU_DIR_PERMISSIONS, g_settings.recording_dir_permissions, 3, LOCALE_RECORDINGMENU_DIR_PERMISSIONS_HINT, LOCALE_IPSETUP_HINT_2, "01234567");
	CMenuForwarder* mf12 = new CMenuForwarder(LOCALE_RECORDINGMENU_DIR_PERMISSIONS, true, g_settings.recording_dir_permissions, &recordingSettings_dirPermissions);

	CRecordingNotifier RecordingNotifier(mf1,mf2,oj2,mf3,oj3,oj4,oj5,mf7,oj12);

	//recording type
	CMenuOptionChooser* oj1 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RECORDING_TYPE, &g_settings.recording_type, RECORDINGMENU_RECORDING_TYPE_OPTIONS, RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT, true, &RecordingNotifier);

	//paint menue entries
	//intros
	recordingSettings->addIntroItems(LOCALE_MAINSETTINGS_RECORDING);

	recordingSettings->addItem(new CMenuForwarder(LOCALE_RECORDINGMENU_SETUPNOW, true, NULL, this, "recording", CRCInput::RC_red));
	recordingSettings->addItem(new CMenuForwarder(LOCALE_SETTINGS_HELP, true, NULL, this, "help_recording", CRCInput::RC_help));
	recordingSettings->addItem(GenericMenuSeparatorLine);
	recordingSettings->addItem( oj1); //recording type (off, server, vcr, direct) 
	recordingSettings->addItem(GenericMenuSeparatorLine);
	recordingSettings->addItem( mf7); //direct record settings
#ifndef HAVE_TRIPLEDRAGON
	/* on the TD, we just do not display those items */
	recordingSettings->addItem( mf1); //server ip
	recordingSettings->addItem( mf2); //server port
	recordingSettings->addItem( oj2); //wol
	recordingSettings->addItem( mf3); //mac
#endif
	recordingSettings->addItem( oj3); //stop playback
	recordingSettings->addItem( oj4); //stop epg
	recordingSettings->addItem( oj4b);//switch on announcement
	recordingSettings->addItem( oj5); //suppress scart switch
#ifdef HAVE_DBOX_HARDWARE
	recordingSettings->addItem(oj12); //use spts
#endif
	recordingSettings->addItem(GenericMenuSeparatorLine);
	recordingSettings->addItem( mf15);//timersettings
		//intros
		timerRecordingSettings->addIntroItems(LOCALE_TIMERSETTINGS_SEPARATOR);
		timerRecordingSettings->addItem( mf5); //start record correcture
		timerRecordingSettings->addItem( mf6); //end record correcture
		timerRecordingSettings->addItem( mf14);//switch correcture
	recordingSettings->addItem( mf13);//audio pid settings
		//intros
		directRecordingSettings->addIntroItems(LOCALE_RECORDINGMENU_FILESETTINGS);
		directRecordingSettings->addItem(new CMenuForwarder(LOCALE_RECORDINGMENU_DEFDIR, true, NULL, dirMenu, NULL, CRCInput::RC_red));
		directRecordingSettings->addItem(oj10);
		directRecordingSettings->addItem(oj15); //max. recording time
		directRecordingSettings->addItem(mf11);
		directRecordingSettings->addItem(mf9);
		directRecordingSettings->addItem(mf12);
		directRecordingSettings->addItem(GenericMenuSeparatorLine);
		directRecordingSettings->addItem(oj13); //ringbuffer
		directRecordingSettings->addItem(oj6);
		directRecordingSettings->addItem(oj7);
		directRecordingSettings->addItem(oj8);
		directRecordingSettings->addItem(oj9);
		directRecordingSettings->addItem(oj14); //gen_psi
		directRecordingSettings->addItem(oj16); //nhd_compatible_ts

	int res = recordingSettings->exec(NULL, "");
	selected = recordingSettings->getSelected();
	delete recordingSettings;

	// delete dynamic created objects
	unsigned int toDeleteSize = toDelete.size();
	for (unsigned int i = 0; i < toDeleteSize; i++)
		delete toDelete[i];

	return res;
}

bool CRecordSetup::changeNotify(const neutrino_locale_t OptionName, void *)
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_RECORDINGMENU_APIDS_STD) ||
	    ARE_LOCALES_EQUAL(OptionName, LOCALE_RECORDINGMENU_APIDS_ALT) ||
	    ARE_LOCALES_EQUAL(OptionName, LOCALE_RECORDINGMENU_APIDS_AC3))
	{
		g_settings.recording_audio_pids_default = ( (recording_audio_pids_std ? TIMERD_APIDS_STD : 0) |
							  (recording_audio_pids_alt ? TIMERD_APIDS_ALT : 0) |
							  (recording_audio_pids_ac3 ? TIMERD_APIDS_AC3 : 0));
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE) ||
	         ARE_LOCALES_EQUAL(OptionName, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER))
	{
		g_Timerd->setRecordingSafety(atoi(record_safety_time_before)*60, atoi(record_safety_time_after)*60);
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE))
	{
		g_Timerd->setZaptoSafety(atoi(zapto_safety_time_before)*60);
	}
	return false;
}

CRecordingNotifier::CRecordingNotifier(CMenuItem* i1 , CMenuItem* i2 , CMenuItem* i3 ,
                                       CMenuItem* i4 , CMenuItem* i5 , CMenuItem* i6 ,
                                       CMenuItem* i7 , CMenuItem* i8 , CMenuItem* i9)
{
	toDisable[ 0] = i1;
	toDisable[ 1] = i2;
	toDisable[ 2] = i3;
	toDisable[ 3] = i4;
	toDisable[ 4] = i5;
	toDisable[ 5] = i6;
	toDisable[ 6] = i7;
	toDisable[ 7] = i8;
	toDisable[ 8] = i9;
}

bool CRecordingNotifier::changeNotify(const neutrino_locale_t, void *)
{
	if ((g_settings.recording_type == CNeutrinoApp::RECORDING_OFF) ||
		(g_settings.recording_type == CNeutrinoApp::RECORDING_FILE))
	{
		for(int i = 0; i < 9; i++)
			toDisable[i]->setActive(false);

		if (g_settings.recording_type == CNeutrinoApp::RECORDING_FILE)
		{
			   toDisable[4]->setActive(true);
			   toDisable[5]->setActive(true);
			   toDisable[7]->setActive(true);
			   toDisable[8]->setActive(true);
		}
	}
	else if (g_settings.recording_type == CNeutrinoApp::RECORDING_SERVER)
	{
		toDisable[0]->setActive(true);
		toDisable[1]->setActive(true);
		toDisable[2]->setActive(true);
		toDisable[3]->setActive(g_settings.recording_server_wakeup==1);
		toDisable[4]->setActive(true);
		toDisable[5]->setActive(true);
		toDisable[6]->setActive(false);
		toDisable[7]->setActive(false);
		toDisable[8]->setActive(true);
	}
	else if (g_settings.recording_type == CNeutrinoApp::RECORDING_VCR)
	{
		toDisable[0]->setActive(false);
		toDisable[1]->setActive(false);
		toDisable[2]->setActive(false);
		toDisable[3]->setActive(false);
		toDisable[4]->setActive(false);
		toDisable[5]->setActive(false);
		toDisable[6]->setActive(true);
		toDisable[7]->setActive(false);
		toDisable[8]->setActive(false);
	}

	return false;
}

