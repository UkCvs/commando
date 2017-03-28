/*
	$Id: neutrino_menu.cpp,v 1.142 2012/07/22 06:32:14 rhabarber1848 Exp $
	
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2008, 2009 Stefan Seyfried

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

#include <system/debug.h>
#include <system/helper.h>
#include <unistd.h>

#include <driver/encoding.h>

#include "gui/bedit/bouqueteditor_bouquets.h"
#include "gui/widget/icons.h"

#include "audio_setup.h"
#include "video_setup.h"
#include "audio_select.h"
#ifdef ENABLE_AUDIOPLAYER
#include "audioplayer.h"
#endif
#ifdef ENABLE_ESD
#include "esound.h"
#endif
#ifdef ENABLE_EPGPLUS
#include "epgplus.h"
#endif
#include "favorites.h"
#include "imageinfo.h"
#include "keyhelper.h"
#if defined(ENABLE_AUDIOPLAYER) || defined(ENABLE_INTERNETRADIO) || defined(ENABLE_ESD) || defined(ENABLE_PICTUREVIEWER) || defined (ENABLE_MOVIEPLAYER)
#include "mediaplayer_setup.h"
#endif
#ifdef ENABLE_MOVIEPLAYER
#include "movieplayer.h"
#include "movieplayer_menu.h"
#include "movieplayer_setup.h"
#endif

#include "network_setup.h"
#ifdef ENABLE_GUI_MOUNT
#include "nfs.h"
#endif
#include "parentallock_setup.h"
#include "personalize.h"
#ifdef ENABLE_PICTUREVIEWER
#include "pictureviewer.h"
#endif
#include "pluginlist.h"
#include "record_setup.h"
#include "miscsettings_menu.h"
#include "scan_setup.h"
#include "screensetup.h"
#include "software_update.h"
#include "streaminfo2.h"
#include "sleeptimer.h"
#include "subchannel_select.h"
#include "update.h"
#include "keybind_setup.h"
#include "lcd_setup.h"
#include "driver_boot_setup.h"
#include "osd_setup.h"
#include "rc_lock.h"
#include "mainmenu.h"

#if ENABLE_UPNP
#include "upnpbrowser.h"
#endif

#ifdef _EXPERIMENTAL_SETTINGS_
#include "experimental_menu.h"
#endif

// TODO: k26 support for ENABLE_DRIVE_GUI, it's disabled with -enable-kernel26 yet
#if ENABLE_DRIVE_GUI
#include "drive_setup.h"
#endif /*ENABLE_DRIVE_GUI*/


enum
{
	MENU_MAIN,
	MENU_SETTINGS,
	MENU_SERVICE,
	
	MENU_MAX //3
};
const mn_widget_struct_t menu_widgets[MENU_MAX] =
{
	{LOCALE_MAINMENU_HEAD, 		NEUTRINO_ICON_MAIN, 		400},	/** 0 = MENU_MAIN*/
	{LOCALE_MAINSETTINGS_HEAD, 	NEUTRINO_ICON_SETTINGS, 	400},	/** 1 = MENU_SETTINGS*/
	{LOCALE_SERVICEMENU_HEAD,	NEUTRINO_ICON_SETTINGS, 	400}, 	/** 2 = MENU_SERVICE*/
};

//init all menues
void CNeutrinoApp::InitMenu()
{
	printf("[neutrino] init menus...\n");
	
	CPersonalizeGui::getInstance()->addWidgets(menu_widgets, MENU_MAX);
		
	//needs to run before InitMenuMain() !!
	firstChannel();
	
	InitMenuMain();
	InitMenuSettings();
	InitMenuService();

	CPersonalizeGui::getInstance()->addPersonalizedItems();
}

//init main menu
void CNeutrinoApp::InitMenuMain()
{
	dprintf(DEBUG_DEBUG, "init mainmenue\n");
	
	CPersonalizeGui *personalize = CPersonalizeGui::getInstance();
	
	///CMenuWidget &menu = personalize->getWidget(MENU_MAIN)/**main**/;
	
	//top
	personalize->addItem(MENU_MAIN, GenericMenuSeparator, NULL, false, CPersonalizeGui::PERSONALIZE_SHOW_NO);

	//tv-mode
	CMenuItem *tvswitch = new CMenuForwarder(LOCALE_MAINMENU_TVMODE, true, NULL, this, "tv", CRCInput::RC_red);
	personalize->addItem(MENU_MAIN, tvswitch, &g_settings.personalize_tvmode, (g_settings.startmode == STARTMODE_TV || firstchannel.mode != 'r' ));
	
	//radio-mode
	CMenuItem *radioswitch = new CMenuForwarder(LOCALE_MAINMENU_RADIOMODE, true, NULL, this, "radio", CRCInput::RC_green);
	personalize->addItem(MENU_MAIN, radioswitch, &g_settings.personalize_radiomode, (g_settings.startmode == STARTMODE_RADIO || (!(g_settings.startmode == STARTMODE_TV) && firstchannel.mode == 'r')));

	//scart
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_SCARTMODE, true, NULL, this, "scart", CRCInput::RC_yellow), &g_settings.personalize_scartmode);

	//games
	menuGamesIsVisible = g_PluginList->hasPlugin(CPlugins::P_TYPE_GAME);
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_GAMES, menuGamesIsVisible, NULL, new CPluginList(LOCALE_MAINMENU_GAMES,CPlugins::P_TYPE_GAME), NULL, CRCInput::RC_blue),
		&g_settings.personalize_games, false, CPersonalizeGui::PERSONALIZE_SHOW_AS_ITEM_OPTION, &menuGamesIsVisible);

	//separator
	personalize->addSeparator(MENU_MAIN);

	// service, also as pin protected option in personalize menu, as a result of parameter value CPersonalizeGui::PERSONALIZE_SHOW_AS_ACCESS_OPTION
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_SERVICE, true, NULL, &personalize->getWidget(MENU_SERVICE)/**service**/), &g_settings.personalize_service, false, CPersonalizeGui::PERSONALIZE_SHOW_AS_ACCESS_OPTION);

	// NOTE: we now manually override the g_settings.personalize_settings for this menu item in neutrino.cpp for box access lockout reasons!
	//       we do not allow users to pin protect settings menu, as we have added root password reset item that we do not want personalized.
	// settings, also as pin protected option in personalize menu, as a result of parameter value CPersonalizeGui::PERSONALIZE_SHOW_AS_ACCESS_OPTION
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_SETTINGS, true, NULL, &personalize->getWidget(MENU_SETTINGS)/**settings**/), &g_settings.personalize_settings, false, CPersonalizeGui::PERSONALIZE_SHOW_AS_ACCESS_OPTION);

	personalize->addSeparator(MENU_MAIN);
	
#ifdef ENABLE_AUDIOPLAYER
	// audioplayer
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_AUDIOPLAYER, true, NULL, new CAudioPlayerGui()), &g_settings.personalize_audioplayer);
	
#ifdef ENABLE_INTERNETRADIO
	// internet player
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_INETRADIO_NAME, true, NULL, new CAudioPlayerGui(true)), &g_settings.personalize_inetradio);
#endif
#endif	

#ifdef ENABLE_ESD
	// esound
	bool show_esd = false;
	if (access("/bin/esd", X_OK) == 0 || access("/var/bin/esd", X_OK) == 0)
	{
		puts("[neutrino] found esound, adding personalized esound entry to mainmenue");
		show_esd = true;
	}
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_ESOUND_NAME, show_esd, NULL, new CEsoundGui()), &g_settings.personalize_esound);

#endif

#ifdef ENABLE_MOVIEPLAYER
	// movieplayer
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_MOVIEPLAYER, true, NULL, new CMoviePlayerMenue()), &g_settings.personalize_movieplayer);
#endif

#ifdef ENABLE_PICTUREVIEWER
	// pictureviewer
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_PICTUREVIEWER, true, NULL, new CPictureViewerGui()), &g_settings.personalize_pictureviewer);
#endif

#if ENABLE_UPNP
	// upnpbrowser
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_UPNPBROWSER, true, NULL, new CUpnpBrowserGui()), &g_settings.personalize_upnpbrowser);
#endif

	// scripts 
	menuScriptsIsVisible = g_PluginList->hasPlugin(CPlugins::P_TYPE_SCRIPT);
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_SCRIPTS, menuScriptsIsVisible, NULL, new CPluginList(LOCALE_MAINMENU_SCRIPTS,CPlugins::P_TYPE_SCRIPT)),
		&g_settings.personalize_scripts, false, CPersonalizeGui::PERSONALIZE_SHOW_AS_ITEM_OPTION, &menuScriptsIsVisible);

#if defined(ENABLE_AUDIOPLAYER) || defined(ENABLE_INTERNETRADIO) || defined(ENABLE_ESD) || defined(ENABLE_MOVIEPLAYER) || defined(ENABLE_PICTUREVIEWER) || defined(ENABLE_UPNP)
	personalize->addSeparator(MENU_MAIN, NONEXISTANT_LOCALE, CPersonalizeGui::PERSONALIZE_SHOW_NO); //don't show this separator in personal menu
#endif

	personalize->addSeparator(MENU_MAIN);

	//10. -- only 10 shortcuts (1-9, 0), the next could be the last also!(10. => 0)
	//sleeptimer
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_SLEEPTIMER, true, NULL, new CSleepTimerWidget), &g_settings.personalize_sleeptimer);

	// reboot
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_REBOOT, true, NULL, this, "reboot"), &g_settings.personalize_reboot);

	// shutdown
	personalize->addItem(MENU_MAIN, new CMenuForwarder(LOCALE_MAINMENU_SHUTDOWN, true, NULL, this, "shutdown", CRCInput::RC_standby), &g_settings.personalize_shutdown);
}

//settings menue
void CNeutrinoApp::InitMenuSettings()
{
	dprintf(DEBUG_DEBUG, "init settings menue...\n");
	
	CPersonalizeGui *personalize = CPersonalizeGui::getInstance();
	
	///CMenuWidget &menu = personalize->getWidget(MENU_SETTINGS)/**settings**/;
	
	// back button, no personalized
	personalize->addIntroItems(MENU_SETTINGS);
	
	// save
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "savesettings", CRCInput::RC_red), &g_settings.personalize_true);
	
	// separator line
	personalize->addItem(MENU_SETTINGS, GenericMenuSeparatorLine, NULL, false, CPersonalizeGui::PERSONALIZE_SHOW_NO);
	
	// video.
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_VIDEO, true, NULL, new CVideoSetup()), &g_settings.personalize_video);	

	// audio
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_AUDIO, true, NULL, new CAudioSetup()), &g_settings.personalize_audio);
	
	// parental lock
	personalize->addItem(MENU_SETTINGS, new CLockedMenuForwarder(LOCALE_PARENTALLOCK_PARENTALLOCK, g_settings.parentallock_pincode, g_settings.parentallock_prompt, true, NULL, new CParentalSetup()), &g_settings.personalize_youth);

	// network
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_NETWORK, true, NULL, networksetup), &g_settings.personalize_network);

	// record settings
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_RECORDING, true, NULL, new CRecordSetup()), &g_settings.personalize_recording);

	// osd
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_OSD, true, NULL, new COsdSetup(LOCALE_MAINMENU_SETTINGS)), &g_settings.personalize_colors);
	
	// lcd.
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_LCD, true, NULL, new CLcdSetup(LOCALE_MAINMENU_SETTINGS)), &g_settings.personalize_lcd);
	
	//10. -- only 10 shortcuts (1-9, 0), the next could be the last also!(10. => 0)
	//keybindings
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_KEYBINDING, true, NULL, new CKeybindSetup(LOCALE_MAINMENU_SETTINGS)), &g_settings.personalize_keybinding);

	//resetpassword
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_RESETPASSWORD, true, NULL, this, "resetpassword", CRCInput::RC_help, NEUTRINO_ICON_BUTTON_HELP), NULL, false, CPersonalizeGui::PERSONALIZE_SHOW_NO);

#ifdef ENABLE_DRIVE_GUI
	// ide, hdd, mmc setup
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_DRIVE_SETUP_HEAD, true, NULL, CDriveSetup::getInstance()), &g_settings.personalize_drive_setup_stat); 
#endif /*ENABLE_DRIVE_GUI*/
	
	//blue (audioplayer, pictureviewer, esd, mediaplayer)
#if defined(ENABLE_AUDIOPLAYER) || defined(ENABLE_PICTUREVIEWER) || defined(ENABLE_ESD) || defined(ENABLE_MOVIEPLAYER)
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MEDIAPLAYERSETTINGS_GENERAL, true, NULL, new CMediaPlayerSetup, NULL, CRCInput::RC_blue), &g_settings.personalize_mediaplayer);
#endif

	//green (driver/boot settings)
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_DRIVER, true, NULL, new CDriverBootSetup(LOCALE_MAINMENU_SETTINGS), NULL, CRCInput::RC_green), &g_settings.personalize_driver);
	
	//yellow (miscSettings)
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_MAINSETTINGS_MISC, true, NULL, new CMiscMenue(LOCALE_MAINMENU_SETTINGS), NULL, CRCInput::RC_yellow), &g_settings.personalize_misc);

#ifdef _EXPERIMENTAL_SETTINGS_
	//experimental settings
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_EXPERIMENTALSETTINGS, true, NULL, new CExperimentalSettingsMenuHandler()), NULL, false, CPersonalizeGui::PERSONALIZE_SHOW_NO);
#endif

	//separator
 	personalize->addSeparator(MENU_SETTINGS);
	
	//personalize
	personalize->addItem(MENU_SETTINGS, new CMenuForwarder(LOCALE_PERSONALIZE_HEAD, true, NULL, personalize), &g_settings.personalize_pinstatus, false, CPersonalizeGui::PERSONALIZE_SHOW_AS_ACCESS_OPTION);
}

const CMenuOptionChooser::keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF },
	{ 1, LOCALE_OPTIONS_ON  }
};

/* service menu*/
void CNeutrinoApp::InitMenuService()
{
	dprintf(DEBUG_DEBUG, "init service menu...\n");
	
	CPersonalizeGui *personalize = CPersonalizeGui::getInstance();

	///CMenuWidget &menu = personalize->getWidget(MENU_SERVICE)/**service**/;
	
	// back button, no personalized
	personalize->addIntroItems(MENU_SERVICE);

	// bouquets
	personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_BOUQUETEDITOR_NAME, true, NULL, new CBEBouquetWidget(), NULL, CRCInput::RC_red), &g_settings.personalize_bouqueteditor);

	// channel scan
	personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_SERVICEMENU_SCANTS, true, NULL, new CScanSetup, NULL, CRCInput::RC_green), &g_settings.personalize_scants);

	// separator
	personalize->addSeparator(MENU_SERVICE); 

	// reload channels
	personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_SERVICEMENU_RELOAD, true, NULL, this, "reloadchannels"), &g_settings.personalize_reload);

	// reload plugins
	personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_SERVICEMENU_GETPLUGINS, true, NULL, this, "reloadplugins"), &g_settings.personalize_getplugins);
	
	// restart neutrino
	personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_SERVICEMENU_RESTART, true, NULL, this, "restart"), &g_settings.personalize_restart);
	
	// epg restart
	personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_SERVICEMENU_EPGRESTART, true, NULL, this, "EPGrestart"), &g_settings.personalize_epgrestart);
	
#ifdef HAVE_DBOX_HARDWARE
	// ucode check
	personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_SERVICEMENU_UCODECHECK, true, NULL, new CUCodeCheckExec), &g_settings.personalize_ucodecheck);
#endif

	// epg status
	personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_SERVICEMENU_CHAN_EPG_STAT, true, NULL, new CDVBInfoExec), &g_settings.personalize_chan_epg_stat);

	// separator
	personalize->addSeparator(MENU_SERVICE); 

	//yellow imageinfo
	personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_SERVICEMENU_IMAGEINFO, true, NULL, new CImageInfo(), NULL, CRCInput::RC_yellow), &g_settings.personalize_imageinfo);

	//softupdate
	if(softupdate)
	{
		// blue software update
		personalize->addItem(MENU_SERVICE, new CMenuForwarder(LOCALE_SERVICEMENU_UPDATE, true, NULL, new CSoftwareUpdate(), NULL, CRCInput::RC_blue), &g_settings.personalize_update, false, CPersonalizeGui::PERSONALIZE_SHOW_AS_ACCESS_OPTION);
 	}

}


#define MAINMENU_RECORDING_OPTION_COUNT 2
const CMenuOptionChooser::keyval MAINMENU_RECORDING_OPTIONS[MAINMENU_RECORDING_OPTION_COUNT] =
{
	{ 0, LOCALE_MAINMENU_RECORDING_START },
	{ 1, LOCALE_MAINMENU_RECORDING_STOP  }
};


// USERMENU
bool CNeutrinoApp::showUserMenu(int button)
{
	if(button < 0 || button >= SNeutrinoSettings::BUTTON_MAX) 
		return false;
		
	CMenuItem* menu_item = NULL;
	CKeyHelper keyhelper;
	neutrino_msg_t key = CRCInput::RC_nokey;
	const char * icon = NULL; 
	
	int menu_items = 0;
	int menu_prev = -1;
	int cnt = 0;
	int dummy = 0;

	// define classes
	CFavorites* tmpFavorites				= NULL;
	CAudioSelectMenuHandler* tmpAudioSelectMenuHandler 	= NULL;
	CSubChannelSelectMenu* tmpSubChannelSelectMenu		= NULL;
	CStreamInfo2Handler*	tmpStreamInfo2Handler 		= NULL;
	CEventListHandler* tmpEventListHandler			= NULL;
#ifdef ENABLE_EPGPLUS
	CEPGplusHandler* tmpEPGplusHandler			= NULL;
#endif
	CEPGDataHandler* tmpEPGDataHandler			= NULL;
#ifdef ENABLE_MOVIEPLAYER
	CMoviePlayerGui* tmpMoviePlayerGui			= NULL;
#endif
	CRCLock* rcLock						= NULL;
	CStreamFeaturesChangeExec* StreamFeaturesChanger	= NULL;
	
	std::string txt = g_settings.usermenu_text[button];
	if (button == SNeutrinoSettings::BUTTON_RED)
	{
		if( txt.empty() )
			txt = g_Locale->getText(LOCALE_INFOVIEWER_EVENTLIST);
	}
	else if( button == SNeutrinoSettings::BUTTON_GREEN)
	{
		if( txt.empty() )
			txt = g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES);
	}
	else if( button == SNeutrinoSettings::BUTTON_YELLOW)
	{
		if( txt.empty() ) 
			txt = g_Locale->getText((g_RemoteControl->are_subchannels) ? LOCALE_INFOVIEWER_SUBSERVICE : LOCALE_INFOVIEWER_SELECTTIME);
			//txt = g_Locale->getText(LOCALE_NVODSELECTOR_DIRECTORMODE);
	}
	else if( button == SNeutrinoSettings::BUTTON_BLUE)
	{	
		if( txt.empty() )
			txt = g_Locale->getText(LOCALE_INFOVIEWER_STREAMINFO);
	}
	CMenuWidget *menu = new CMenuWidget(txt.c_str() , NEUTRINO_ICON_FEATURES, 400);
	if (menu == NULL) 
		return 0;
	menu->addItem(GenericMenuSeparator);

	//ExtrasMenu
	if (button == SNeutrinoSettings::BUTTON_BLUE)
	{
		menu->addItem(new CMenuForwarder(LOCALE_MAINMENU_EXTRASMENU, true, NULL, new MainMenu(), NULL, CRCInput::RC_setup, NEUTRINO_ICON_BUTTON_MENU));
		menu->addItem(GenericMenuSeparatorLine);
	}

	// go through any postition number
	for(int pos = 0; pos < SNeutrinoSettings::ITEM_MAX ; pos++)
	{
		// now compare pos with the position of any item. Add this item if position is the same 
		switch(g_settings.usermenu[button][pos])
		{
			case SNeutrinoSettings::ITEM_NONE: 
				// do nothing 
				break;

			case SNeutrinoSettings::ITEM_BAR: 
				if(menu_prev == -1 && menu_prev == SNeutrinoSettings::ITEM_BAR )
					break;
					
				menu->addItem(GenericMenuSeparatorLine);
				menu_prev = SNeutrinoSettings::ITEM_BAR;
				break;

			case SNeutrinoSettings::ITEM_VTXT:
				for(unsigned int count = 0; count < (unsigned int) g_PluginList->getNumberOfPlugins(); count++)
				{
					std::string tmp = g_PluginList->getName(count);
					if (g_PluginList->getType(count)== CPlugins::P_TYPE_TOOL && !g_PluginList->isHidden(count) && tmp.find("Teletext") != std::string::npos)
					{
						menu_items++;
						menu_prev = SNeutrinoSettings::ITEM_VTXT;
						if (StreamFeaturesChanger == NULL)
							StreamFeaturesChanger = new CStreamFeaturesChangeExec();
						keyhelper.get(&key, &icon, CRCInput::RC_nokey);
						menu_item = new CMenuForwarder(g_PluginList->getName(count), true, NULL, StreamFeaturesChanger, to_string(count).c_str(), key, icon);
						menu->addItem(menu_item, (cnt == 0));
						cnt++;
					}
				}
				break;

			case SNeutrinoSettings::ITEM_PLUGIN:
				for(unsigned int count = 0; count < (unsigned int) g_PluginList->getNumberOfPlugins(); count++)
				{
					std::string tmp = g_PluginList->getName(count);
					if (g_PluginList->getType(count)== CPlugins::P_TYPE_TOOL && !g_PluginList->isHidden(count) && tmp.find("Teletext") == std::string::npos)
					{
						menu_items++;
						menu_prev = SNeutrinoSettings::ITEM_PLUGIN;
						if (StreamFeaturesChanger == NULL)
							StreamFeaturesChanger = new CStreamFeaturesChangeExec();
						// (test by AL 28 Mar 2017)
						keyhelper.get(&key, &icon, cnt == 0 ? CRCInput::RC_blue : CRCInput::RC_nokey);
//						keyhelper.get(&key, &icon, tmp.find("Commando Toolbox") != std::string::npos ? CRCInput::RC_blue : CRCInput::RC_nokey);
						menu_item = new CMenuForwarder(g_PluginList->getName(count), true, NULL, StreamFeaturesChanger, to_string(count).c_str(), key, icon);
						menu->addItem(menu_item, (cnt == 0));
						cnt++;
					}
				}
				break;
				
			case SNeutrinoSettings::ITEM_FAVORITS: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_FAVORITS;
				if (tmpFavorites == NULL)
					tmpFavorites = new CFavorites();
				keyhelper.get(&key,&icon);
				menu_item = new CMenuForwarder(LOCALE_FAVORITES_MENUEADD, true, NULL, tmpFavorites, "-1", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_RECORD: 
				if(g_settings.recording_type == RECORDING_OFF) 
					break;
				
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_RECORD;
				keyhelper.get(&key,&icon,CRCInput::RC_red);
				menu_item = new CMenuOptionChooser(LOCALE_MAINMENU_RECORDING, &recordingstatus, MAINMENU_RECORDING_OPTIONS, MAINMENU_RECORDING_OPTION_COUNT, true, this, key, icon);
				menu->addItem(menu_item, false);
				break;

#ifdef ENABLE_MOVIEPLAYER
			case SNeutrinoSettings::ITEM_MOVIEPLAYER_TS: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_MOVIEPLAYER_TS;
				if (tmpMoviePlayerGui == NULL)
					tmpMoviePlayerGui = new CMoviePlayerGui();
				keyhelper.get(&key,&icon,CRCInput::RC_green);
				menu_item = new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK, true, NULL, tmpMoviePlayerGui, "tsplayback", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_MOVIEPLAYER_MB: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_MOVIEPLAYER_MB;
				keyhelper.get(&key,&icon,CRCInput::RC_green);
#ifndef ENABLE_MOVIEPLAYER2
				if (tmpMoviePlayerGui == NULL)
					tmpMoviePlayerGui = new CMoviePlayerGui();
				menu_item = new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, tmpMoviePlayerGui, "tsmoviebrowser", key, icon);
#else
				menu_item = new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, CMovieBrowser::getInstance(), "run", key, icon);
#endif
				menu->addItem(menu_item, false);
				break;
#endif

			case SNeutrinoSettings::ITEM_TIMERLIST: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_TIMERLIST;
				keyhelper.get(&key,&icon,CRCInput::RC_yellow);
				menu_item = new CMenuForwarder(LOCALE_TIMERLIST_NAME, true, NULL, Timerlist, "-1", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_REMOTE: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_REMOTE;
				if (rcLock == NULL)
					rcLock = new CRCLock();
				keyhelper.get(&key,&icon);
				menu_item = new CMenuForwarder(LOCALE_RCLOCK_MENUEADD, true, NULL, rcLock, "-1" , key, icon );
				menu->addItem(menu_item, false);
				break;

#ifdef ENABLE_EPGPLUS
			case SNeutrinoSettings::ITEM_EPG_SUPER: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_EPG_SUPER;
				if (tmpEPGplusHandler == NULL)
					tmpEPGplusHandler = new CEPGplusHandler();
				keyhelper.get(&key,&icon,CRCInput::RC_green);
				menu_item = new CMenuForwarder(LOCALE_EPGMENU_EPGPLUS   , true, NULL, tmpEPGplusHandler  ,  "-1", key, icon);
				menu->addItem(menu_item, false);
				break;
#endif

			case SNeutrinoSettings::ITEM_EPG_LIST: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_EPG_LIST;
				if (tmpEventListHandler == NULL)
					tmpEventListHandler = new CEventListHandler();
				keyhelper.get(&key,&icon,CRCInput::RC_red);
				menu_item = new CMenuForwarder(LOCALE_EPGMENU_EVENTLIST , true, NULL, tmpEventListHandler,  "-1", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_EPG_INFO: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_EPG_INFO;
				if (tmpEPGDataHandler == NULL)
					tmpEPGDataHandler = new CEPGDataHandler();
				keyhelper.get(&key,&icon,CRCInput::RC_yellow);
				menu_item = new CMenuForwarder(LOCALE_EPGMENU_EVENTINFO , true, NULL, tmpEPGDataHandler ,  "-1", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_EPG_MISC: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_EPG_MISC;
				dummy = g_Sectionsd->getIsScanningActive();
				keyhelper.get(&key,&icon);
				menu_item = new CMenuOptionChooser(LOCALE_MAINMENU_PAUSESECTIONSD, &dummy, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this, key, icon );
				menu->addItem(menu_item, false);
				menu_items++;
				keyhelper.get(&key,&icon);
				menu_item = new CMenuForwarder(LOCALE_MAINMENU_CLEARSECTIONSD, true, NULL, this, "clearSectionsd", key,icon);
				menu->addItem(menu_item, false);
				break;
				
			case SNeutrinoSettings::ITEM_AUDIO_SELECT: 
				// -- new Audio Selector Menu (rasc 2005-08-30)
				if (g_settings.audio_left_right_selectable ||
				    g_RemoteControl->current_PIDs.APIDs.size() > 1 ||
				    !g_RemoteControl->current_PIDs.SubPIDs.empty())
				{
					menu_items++;
					menu_prev = SNeutrinoSettings::ITEM_AUDIO_SELECT;
					if (tmpAudioSelectMenuHandler == NULL)
						tmpAudioSelectMenuHandler = new CAudioSelectMenuHandler();
					keyhelper.get(&key,&icon);
					menu_item = new CMenuForwarder(LOCALE_APIDSELECTOR_HEAD, true, NULL, tmpAudioSelectMenuHandler, "-1", key,icon);
					menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_SUBCHANNEL: 
				if (!(g_RemoteControl->subChannels.empty()))
				{
					// NVOD/SubService- Kanal!
					menu_items++;
					menu_prev = SNeutrinoSettings::ITEM_SUBCHANNEL;
					if (tmpSubChannelSelectMenu == NULL)
						tmpSubChannelSelectMenu = new CSubChannelSelectMenu();
					keyhelper.get(&key,&icon);
					menu_item = new CMenuForwarder(g_RemoteControl->are_subchannels ? LOCALE_NVODSELECTOR_SUBSERVICE : LOCALE_NVODSELECTOR_HEAD, true, NULL, tmpSubChannelSelectMenu, "-1", key, icon);
					menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_TECHINFO: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_TECHINFO;
				if (tmpStreamInfo2Handler == NULL)
					tmpStreamInfo2Handler = new CStreamInfo2Handler();
				keyhelper.get(&key,&icon,CRCInput::RC_blue);
				menu_item = new CMenuForwarder(LOCALE_EPGMENU_STREAMINFO, true, NULL, tmpStreamInfo2Handler    , "-1", key, icon );
				menu->addItem(menu_item, false);
				break;

			default: 
				printf("[neutrino] WARNING! menu wrong item!!\n");
				break;
		}
	} 

	if(menu_items >	1 ) 				// show menu if there are more than 2 items only
	{
		menu->exec(NULL,"");
	}
	else if (menu_item != NULL)			// otherwise, we start the item directly (must be the last one)
	{
		menu_item->exec( NULL );
	}									// neither nor, we do nothing

	// clear the heap
	delete menu;
	delete tmpFavorites;
	delete tmpAudioSelectMenuHandler;
	delete tmpSubChannelSelectMenu;
	delete tmpStreamInfo2Handler;
	delete tmpEventListHandler;
#ifdef ENABLE_EPGPLUS
	delete tmpEPGplusHandler;
#endif
	delete tmpEPGDataHandler;
#ifdef ENABLE_MOVIEPLAYER
	delete tmpMoviePlayerGui;
#endif
	delete rcLock;
	delete StreamFeaturesChanger;

	return true;
}

