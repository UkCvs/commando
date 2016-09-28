/*
	$Id: neutrino.cpp,v 1.1086 2012/11/01 19:36:26 rhabarber1848 Exp $
	
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2008 Novell, Inc. Author: Stefan Seyfried
	Copyright (C) 2007-2010, 2013 Stefan Seyfried

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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define NEUTRINO_CPP

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>

#include <sys/socket.h>

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>


#include "global.h"
#include "neutrino.h"
#ifndef HAVE_TRIPLEDRAGON
#include <dbox/fp.h>
#endif

#include <daemonc/remotecontrol.h>

#include <driver/encoding.h>
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/stream2file.h>
#include <driver/vcrcontrol.h>
#include <driver/shutdown_count.h>
#ifdef ENABLE_LIRC
#include <irsend/irsend.h>
#endif

#include "gui/widget/dirchooser.h"
#include "gui/filebrowser.h"
#include "gui/widget/hintbox.h"
#include "gui/widget/icons.h"
#include "gui/widget/messagebox.h"
#include "gui/camdmenu.h"

#ifdef ENABLE_AUDIOPLAYER
#include "gui/audioplayer.h"
#endif
#include "gui/bouquetlist.h"
#ifdef ENABLE_MOVIEPLAYER
#include "gui/movieplayer.h"
#endif
#ifdef ENABLE_GUI_MOUNT
#include "gui/nfs.h"
#endif
#include "gui/personalize.h"
#include "gui/scan_setup.h"
#include "gui/esound.h"
#include "gui/osdlang_setup.h"
#ifdef ENABLE_SAMBASERVER
#include "gui/sambaserver_setup.h"
#endif
#include "gui/rc_lock.h"
#include "gui/update.h"
#include "gui/themes.h"

#include <system/setting_helpers.h>
#include <system/settings.h>
#include <system/debug.h>
#include <system/flashtool.h>
#include <system/helper.h>
#ifdef ENABLE_GUI_MOUNT
#include <system/fsmounter.h>
#endif
#include <system/configure_network.h>
#include <timerdclient/timerdmsg.h>

#include <string.h>

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_init();
#endif

CBouquetList    	* bouquetList;
CBouquetList    	* bouquetListTV;
CBouquetList    	* bouquetListRADIO;
CBouquetList    	* bouquetListRecord;

// Globale Variablen - to use import global.h
static char **global_argv;

extern font_sizes_groups font_sizes_groups[];

// USERMENU
const char* usermenu_button_def[SNeutrinoSettings::BUTTON_MAX]={"red","green","yellow","blue"};

CVCRControl::CDevice * recordingdevice = NULL;

#define NEUTRINO_SETTINGS_FILE          CONFIGDIR "/neutrino.conf"
#define NEUTRINO_ZAPTO_TIMER_SCRIPT	CONFIGDIR "/zapto.timer"
#define NEUTRINO_RECORDING_TIMER_SCRIPT CONFIGDIR "/recording.timer"
#define NEUTRINO_RECORDING_START_SCRIPT CONFIGDIR "/recording.start"
#define NEUTRINO_RECORDING_END_SCRIPT CONFIGDIR "/recording.end"
#define NEUTRINO_ENTER_STANDBY_SCRIPT   CONFIGDIR "/standby.on"
#define NEUTRINO_LEAVE_STANDBY_SCRIPT   CONFIGDIR "/standby.off"
#define NEUTRINO_INIT_END_SCRIPT	CONFIGDIR "/init.end"
#define NEUTRINO_SCAN_SETTINGS_FILE     CONFIGDIR "/scan.conf"
#define NEUTRINO_DEFAULTLOCALE_FILE     CONFIGDIR "/defaultlocale"
#define NEUTRINO_PARENTALLOCKED_FILE    DATADIR   "/neutrino/.plocked"

static void initGlobals(void)
{
	g_fontRenderer  = NULL;

	g_RCInput       = NULL;
	g_Controld      = new CControldClient;
	g_Timerd        = NULL;
	g_Zapit         = new CZapitClient;
	g_RemoteControl = NULL;

	g_EpgData       = NULL;
	g_InfoViewer    = NULL;
	g_EventList     = NULL;

#ifdef ENABLE_RADIOTEXT
	g_Radiotext     = NULL;
#endif
	g_Locale        = new CLocaleManager;
	g_PluginList    = NULL;
}

int CNeutrinoApp::execute_sys_command(const char *command)
/* returns exit code after executed system command */
{
	int result = system(command);
	int exit_code = WEXITSTATUS(result);

	if (result !=0)	{
		printf("[neutrino] %s failed with return code = %d\n", command, exit_code);
	}
	return exit_code;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+                                                                                     +
+          CNeutrinoApp - Constructor, initialize g_fontRenderer                      +
+                                                                                     +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
CNeutrinoApp::CNeutrinoApp()
: configfile('\t'),
	recordingstatus(0)
{
	standby_pressed_at.tv_sec = 0;

	frameBuffer = CFrameBuffer::getInstance();
	frameBuffer->setIconBasePath(DATADIR "/neutrino/icons/");

	SetupFrameBuffer();

	mode = mode_unknown;
	channelList       = NULL;
	bouquetList       = NULL;
	channelListTV     = NULL;
	bouquetListTV     = NULL;
	channelListRADIO  = NULL;
	bouquetListRADIO  = NULL;
	channelListRecord = NULL;
	bouquetListRecord = NULL;
	nextRecordingInfo = NULL;
	networksetup      = NULL;
	Timerlist         = NULL;
	skipShutdownTimer = false;
	parentallocked    = false;
	waitforshutdown   = false;
	volumeBarIsVisible	= true;
	wakeupfromScart    = false;
	standbyAfterRecord = false;
	zapto_tv_on_init_done = false;
	zapto_radio_on_init_done = false;
	obeyStartMode   = true;
	menuGamesIsVisible = true;
	menuScriptsIsVisible = true;
	memset(&font, 0, sizeof(neutrino_font_descr_struct));
	memset(&lcd_font, 0, sizeof(lcd_font_descr_struct));
}

/*-------------------------------------------------------------------------------------
-                                                                                     -
-           CNeutrinoApp - Destructor                                                 -
-                                                                                     -
-------------------------------------------------------------------------------------*/
CNeutrinoApp::~CNeutrinoApp()
{
	if (channelListTV)
		delete channelListTV;
	if (channelListRADIO)
		delete channelListRADIO;
	if (channelListRecord)
		delete channelListRecord;
	if (bouquetListTV)
		delete bouquetListTV;
	if (bouquetListRADIO)
		delete bouquetListRADIO;
	if (bouquetListRecord)
		delete bouquetListRecord;
	if (networksetup)
		delete networksetup;
	if (Timerlist)
		delete Timerlist;
}

CNeutrinoApp* CNeutrinoApp::getInstance()
{
	static CNeutrinoApp* neutrinoApp = NULL;

	if(!neutrinoApp)
	{
		neutrinoApp = new CNeutrinoApp();
		dprintf(DEBUG_DEBUG, "NeutrinoApp Instance created\n");
	}
	return neutrinoApp;
}


const font_sizes_struct neutrino_font[SNeutrinoSettings::FONT_TYPE_COUNT] =
{
	{LOCALE_FONTSIZE_MENU               ,  20, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_MENU_TITLE         ,  30, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_MENU_INFO          ,  16, FONT_STYLE_REGULAR, 0},
	{LOCALE_FONTSIZE_EPG_TITLE          ,  25, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_EPG_INFO1          ,  17, FONT_STYLE_ITALIC , 2},
	{LOCALE_FONTSIZE_EPG_INFO2          ,  17, FONT_STYLE_REGULAR, 2},
	{LOCALE_FONTSIZE_EPG_DATE           ,  15, FONT_STYLE_REGULAR, 2},
	{LOCALE_FONTSIZE_EVENTLIST_TITLE    ,  30, FONT_STYLE_REGULAR, 0},
	{LOCALE_FONTSIZE_EVENTLIST_ITEMLARGE,  20, FONT_STYLE_BOLD   , 1},
	{LOCALE_FONTSIZE_EVENTLIST_ITEMSMALL,  14, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_EVENTLIST_DATETIME ,  16, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_GAMELIST_ITEMLARGE ,  20, FONT_STYLE_BOLD   , 1},
	{LOCALE_FONTSIZE_GAMELIST_ITEMSMALL ,  16, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_CHANNELLIST        ,  18, FONT_STYLE_BOLD   , 1},
	{LOCALE_FONTSIZE_CHANNELLIST_DESCR  ,  18, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_CHANNELLIST_NUMBER ,  12, FONT_STYLE_BOLD   , 2},
	{LOCALE_FONTSIZE_CHANNELLIST_EVENT  ,  14, FONT_STYLE_REGULAR, 2},
	{LOCALE_FONTSIZE_CHANNEL_NUM_ZAP    ,  40, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_INFOBAR_NUMBER     ,  50, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_INFOBAR_CHANNAME   ,  30, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_INFOBAR_INFO       ,  20, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_INFOBAR_SMALL      ,  14, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_FILEBROWSER_ITEM   ,  16, FONT_STYLE_BOLD   , 1}
};

typedef struct lcd_setting_t
{
	const char * const name;
	const unsigned int default_value;
} lcd_setting_struct_t;

const lcd_setting_struct_t lcd_setting[SNeutrinoSettings::LCD_SETTING_COUNT] =
{
	{"lcd_brightness"       , DEFAULT_LCD_BRIGHTNESS       },
	{"lcd_standbybrightness", DEFAULT_LCD_STANDBYBRIGHTNESS},
	{"lcd_contrast"         , DEFAULT_LCD_CONTRAST         },
	{"lcd_power"            , DEFAULT_LCD_POWER            },
	{"lcd_inverse"          , DEFAULT_LCD_INVERSE          },
	{"lcd_show_volume"      , DEFAULT_LCD_SHOW_VOLUME      },
	{"lcd_autodimm"         , DEFAULT_LCD_AUTODIMM         },
	{"lcd_epgmode"          , DEFAULT_LCD_EPGMODE          },
	{"lcd_bias"             , DEFAULT_LCD_BIAS             },
	{"lcd_epgalign"         , DEFAULT_LCD_EPGALIGN         }
};


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  loadSetup, load the application-settings                   *
*                                                                                     *
**************************************************************************************/
int CNeutrinoApp::loadSetup()
{
	int erg = 0;

	//settings laden - und dabei Defaults setzen!
	if(!configfile.loadConfig(NEUTRINO_SETTINGS_FILE))
	{
		//file existiert nicht
		erg = 1;
	}
        std::ifstream checkParentallocked(NEUTRINO_PARENTALLOCKED_FILE);
	if(checkParentallocked)
	{
	        parentallocked = true;
	        checkParentallocked.close();
	}

	//video
	g_settings.video_Format = configfile.getInt32("video_Format", CControldClient::VIDEOFORMAT_AUTO);
	g_settings.video_backgroundFormat = configfile.getInt32("video_backgroundFormat", CControldClient::VIDEOFORMAT_16_9);

	g_settings.video_csync = configfile.getInt32( "video_csync", 0 );

	//fb-alphavalues for gtx
	g_settings.gtx_alpha1 = configfile.getInt32( "gtx_alpha1", 0);
	g_settings.gtx_alpha2 = configfile.getInt32( "gtx_alpha2", 1);

	// EPG-Config
	g_settings.epg_cache 		= configfile.getString("epg_cache_time", "2");
	g_settings.epg_extendedcache	= configfile.getString("epg_extendedcache_time", "18");
	g_settings.epg_old_events 	= configfile.getString("epg_old_events", "1");
	g_settings.epg_max_events 	= configfile.getString("epg_max_events", "18000");
	g_settings.epg_dir 		= configfile.getString("epg_dir", "");
#ifdef ENABLE_FREESATEPG
	g_settings.epg_freesat_enabled	= configfile.getBool( "epg_freesat_enabled", false);
#endif
	// EPG-View
	g_settings.bigFonts = configfile.getInt32("bigFonts", 0);

	// NTP-Server for sectionsd
	g_settings.network_ntpserver	= configfile.getString("network_ntpserver", "time.mit.edu");
	g_settings.network_ntprefresh	= configfile.getString("network_ntprefresh", "30" );
	g_settings.network_ntpenable 	= configfile.getBool("network_ntpenable", CNetworkSetup::NETWORK_NTP_OFF);

	//misc
#ifndef HAVE_TRIPLEDRAGON
	g_settings.standby_save_power		= configfile.getBool("standby_save_power"        , false);
#else
	/* on TD this is REQUIRED for proper working standby etc. */
	g_settings.standby_save_power		= true;
#endif
#ifdef HAVE_DBOX_HARDWARE
	g_settings.shutdown_real		= configfile.getBool("shutdown_real"             , true );
	g_settings.shutdown_real_rcdelay	= configfile.getBool("shutdown_real_rcdelay"     , true );
#else
	/* most dreamboxen and the TD cannot do a real shutdown */
	g_settings.shutdown_real		= configfile.getBool("shutdown_real"             , false);
	g_settings.shutdown_real_rcdelay	= configfile.getBool("shutdown_real_rcdelay"     , false);
#endif
	g_settings.standby_off_with		= configfile.getInt32("standby_off_with" , STANDBY_OFF_WITH_POWER );
	strcpy(g_settings.shutdown_count, configfile.getString("shutdown_count","0").c_str());
	g_settings.sleeptimer_min		= configfile.getInt32("sleeptimer_min", 0);
	g_settings.volumebar_disp_pos		= configfile.getInt32("volumebar_disp_pos" , VOLUMEBAR_DISP_POS_DEFAULT_CENTER );
	g_settings.infobar_sat_display		= configfile.getBool("infobar_sat_display"       , false );
	g_settings.infobar_subchan_disp_pos	= configfile.getInt32("infobar_subchan_disp_pos" , CInfoViewer::SUBCHAN_DISP_POS_INFOBAR );
	g_settings.misc_spts			= configfile.getBool("misc_spts"                 , false );
#ifndef TUXTXT_CFG_STANDALONE
	g_settings.tuxtxt_cache			= configfile.getBool("tuxtxt_cache"              , false );
#endif
	g_settings.virtual_zap_mode		= configfile.getBool("virtual_zap_mode"          , true);
	g_settings.progressbar_color		= configfile.getBool("progressbar_color"         , true );
	g_settings.infobar_show			= configfile.getInt32("infobar_show"             , CInfoViewer::EPGINFO_NO_MESSAGE);
	g_settings.show_mute_icon		= configfile.getInt32("show_mute_icon"		, SHOW_MUTE_ICON_YES);
	g_settings.channellist_additional = configfile.getInt32("channellist_additional", CChannelList::ADDITIONAL_MTV);
	g_settings.channellist_epgtext_align_right		= configfile.getBool("channellist_epgtext_align_right"          , false);
	g_settings.channellist_extended		= configfile.getBool("channellist_extended"          , true);
	g_settings.channellist_foot	= configfile.getInt32("channellist_foot"          , CChannelList::FOOT_NEXT);
	strcpy( g_settings.infobar_channel_logodir, configfile.getString( "infobar_channel_logodir", "/var/etc/icons/").c_str()); 
	g_settings.infobar_show_channellogo	= configfile.getInt32("infobar_show_channellogo"		, CInfoViewer::NO_LOGO);
	g_settings.infobar_channellogo_background		= configfile.getInt32("infobar_channellogo_background"		, CInfoViewer::NO_BACKGROUND);
	g_settings.startmode			= configfile.getInt32("startmode" , STARTMODE_TV );
	g_settings.wzap_time			= configfile.getInt32("wzap_time", 3 );
	g_settings.radiotext_enable		= configfile.getBool("radiotext_enable"          , false);

	//audio
	g_settings.audio_AnalogMode 		= configfile.getInt32( "audio_AnalogMode"        , 0 );
	g_settings.audio_DolbyDigital		= configfile.getBool("audio_DolbyDigital"        , false);
	g_settings.audio_initial_volume		= configfile.getInt32( "audio_initial_volume"    , 100 );
#ifdef HAVE_DBOX_HARDWARE
	g_settings.audio_avs_Control 		= configfile.getInt32( "audio_avs_Control", CControld::TYPE_AVS );
	g_settings.audio_step               = configfile.getInt32( "audio_step" , 5 );
#else
	// the dreambox 500 (and the TD) has 32 volume steps, so a stepwidth of 3 matches the hardware better
	g_settings.audio_step               = configfile.getInt32( "audio_step" , 3 );
#ifdef HAVE_TRIPLEDRAGON
	g_settings.audio_avs_Control 		= configfile.getInt32("audio_avs_Control", CControld::TYPE_AVS);
#else
	// dreambox
	g_settings.audio_avs_Control 		= CControld::TYPE_OST;
#endif
#endif
	g_settings.audio_PCMOffset      = configfile.getInt32( "audio_PCMOffset", 0 );
	g_settings.audio_ReSync		= configfile.getInt32( "audio_ReSync"   , 2);
	g_settings.audio_ReSync_timer	= configfile.getInt32( "audio_ReSync_timer" , 40);

	//Audio Priority Pids
	char audio_propids_key[4];
	g_settings.audio_propids_enabled = configfile.getBool("audio_propids_enabled", true);
	strcpy( g_settings.audio_propids_name[0], configfile.getString( "audio_propids_name0", "eng" ).c_str() );
	for(int i=1 ; i < AUDIO_PRIORITY_NR_OF_ENTRIES ; i++)
	{
		sprintf(audio_propids_key, "audio_propids_name%d", i);
		strcpy( g_settings.audio_propids_name[i], configfile.getString( audio_propids_key, "" ).c_str() );
	}

	//vcr
	g_settings.vcr_AutoSwitch		= configfile.getBool("vcr_AutoSwitch"            , true );

	//language
	strcpy(g_settings.language, configfile.getString("language", "").c_str());
	strcpy(g_settings.timezone, configfile.getString("timezone", "London, Dublin (GMT)").c_str());

	//widget settings
	g_settings.widget_fade           	= configfile.getBool("widget_fade"          , true );

	//colors
	CThemes::getColors(configfile);

	//corners
	g_settings.rounded_corners = configfile.getBool("rounded_corners", true);

	//menu numbers
#ifndef HAVE_DBOX_HARDWARE
	g_settings.menu_numbers_as_icons = configfile.getBool("menu_numbers_as_icons", true);
#else
	//we can paint number buttons in menus automatically without parameter, but it isn't a good idea for
	//expensive menu painting with all possible numbers (like in mainmenu) on slower machines like dbox2.
	g_settings.menu_numbers_as_icons = configfile.getBool("menu_numbers_as_icons", false);
#endif

	//network
	for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
	{
		std::string i_str(to_string(i));
		g_settings.network_nfs[i].ip             = configfile.getString("network_nfs_ip_" + i_str, "");
		g_settings.network_nfs[i].dir            = configfile.getString("network_nfs_dir_" + i_str, "");
		g_settings.network_nfs[i].local_dir      = configfile.getString("network_nfs_local_dir_" + i_str, "");
		g_settings.network_nfs[i].automount      = configfile.getInt32 ("network_nfs_automount_" + i_str, 0);
		g_settings.network_nfs[i].type           = configfile.getInt32 ("network_nfs_type_" + i_str, 1);
		g_settings.network_nfs[i].username       = configfile.getString("network_nfs_username_" + i_str, "");
		g_settings.network_nfs[i].password       = configfile.getString("network_nfs_password_" + i_str, "");
		g_settings.network_nfs[i].mount_options1 = configfile.getString("network_nfs_mount_options1_" + i_str, "rw,soft,udp" );
		g_settings.network_nfs[i].mount_options2 = configfile.getString("network_nfs_mount_options2_" + i_str, "nolock,rsize=8192,wsize=8192" );
		g_settings.network_nfs[i].mac            = configfile.getString("network_nfs_mac_" + i_str, "11:22:33:44:55:66");
	}
	g_settings.filesystem_is_utf8              = configfile.getBool("filesystem_is_utf8"                 , true );

#ifdef ENABLE_SAMBASERVER
	// samba
	g_settings.smb_setup_samba_on_off = configfile.getInt32("smb_setup_samba_on_off", CSambaSetup::OFF); // OFF
	g_settings.smb_setup_samba_workgroup = configfile.getString( "smb_setup_samba_workgroup", "WORKGROUP" );
#endif

	// Personalization
	g_settings.personalize_pinstatus = configfile.getInt32("personalize_pinstatus", CPersonalizeGui::PROTECT_MODE_NOT_PROTECTED);
	strcpy( g_settings.personalize_pincode, configfile.getString( "personalize_pincode", "0000" ).c_str() );
	g_settings.personalize_bluebutton = configfile.getInt32("personalize_bluebutton", CPersonalizeGui::PERSONALIZE_MODE_ENABLED); // infobar blue button
	g_settings.personalize_redbutton = configfile.getInt32("personalize_redbutton", CPersonalizeGui::PERSONALIZE_MODE_ENABLED); // infobar red button

	g_settings.personalize_tvmode = configfile.getInt32("personalize_tvmode", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_radiomode = configfile.getInt32("personalize_radiomode", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
#ifdef BOXMODEL_DM500
	// the dm500 has no SCART in and cannot really shut down, so let's hide those entries
	g_settings.personalize_scartmode = configfile.getInt32("personalize_scartmode", CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE);
	g_settings.personalize_shutdown = configfile.getInt32("personalize_shutdown", CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE);
	g_settings.personalize_lcd = configfile.getInt32("personalize_lcd", CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE);
#else
	g_settings.personalize_scartmode = configfile.getInt32("personalize_scartmode", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
#ifdef HAVE_TRIPLEDRAGON
	// no real shutdown, so let's hide it
	g_settings.personalize_shutdown = configfile.getInt32("personalize_shutdown", CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE);
#else
	g_settings.personalize_shutdown = configfile.getInt32("personalize_shutdown", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
#endif
	g_settings.personalize_lcd = configfile.getInt32("personalize_lcd", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
#endif
	g_settings.personalize_sleeptimer = configfile.getInt32("personalize_sleeptimer", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_reboot = configfile.getInt32("personalize_reboot", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);

	g_settings.personalize_games = configfile.getInt32("personalize_games", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_audioplayer = configfile.getInt32("personalize_audioplayer", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_inetradio = configfile.getInt32("personalize_inetradio", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_esound = configfile.getInt32("personalize_esound", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_movieplayer = configfile.getInt32("personalize_movieplayer", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_pictureviewer = configfile.getInt32("personalize_pictureviewer", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
#ifdef ENABLE_UPNP
 	g_settings.personalize_upnpbrowser = configfile.getInt32("personalize_upnpbrowser", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
#endif
	g_settings.personalize_scripts = configfile.getInt32("personalize_scripts", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);

	// do not allow users to pin protect settings menu, we have added root password reset item that we do not want personalized.
	// g_settings.personalize_settings = configfile.getInt32("personalize_settings", CPersonalizeGui::PROTECT_MODE_NOT_PROTECTED);
	g_settings.personalize_settings = CPersonalizeGui::PROTECT_MODE_NOT_PROTECTED;
	// g_settings.personalize_service = configfile.getInt32("personalize_service", CPersonalizeGui::PROTECT_MODE_NOT_PROTECTED);
	g_settings.personalize_service = CPersonalizeGui::PROTECT_MODE_NOT_PROTECTED;

	g_settings.personalize_bouqueteditor = configfile.getInt32("personalize_bouqueteditor", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_scants = configfile.getInt32("personalize_scants", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_reload = configfile.getInt32("personalize_reload", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_getplugins = configfile.getInt32("personalize_getplugins", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_restart = configfile.getInt32("personalize_restart", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_epgrestart = configfile.getInt32("personalize_epgrestart", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_ucodecheck = configfile.getInt32("personalize_ucodecheck", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_imageinfo = configfile.getInt32("personalize_imageinfo", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_update = configfile.getInt32("personalize_update", CPersonalizeGui::PROTECT_MODE_NOT_PROTECTED);
	g_settings.personalize_chan_epg_stat = configfile.getInt32("personalize_chan_epg_stat", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
#ifdef ENABLE_DRIVE_GUI
	g_settings.personalize_drive_setup_stat = configfile.getInt32("personalize_drive_setup_stat", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
#endif

	g_settings.personalize_audio = configfile.getInt32("personalize_audio", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_video = configfile.getInt32("personalize_video", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_youth = configfile.getInt32("personalize_youth", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_network = configfile.getInt32("personalize_network", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_recording = configfile.getInt32("personalize_recording", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_keybinding = configfile.getInt32("personalize_keybinding", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_colors = configfile.getInt32("personalize_colors", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_lcd = configfile.getInt32("personalize_lcd", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_mediaplayer = configfile.getInt32("personalize_mediaplayer", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_driver = configfile.getInt32("personalize_driver", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_misc = configfile.getInt32("personalize_misc", CPersonalizeGui::PERSONALIZE_MODE_VISIBLE);
	g_settings.personalize_true = CPersonalizeGui::PERSONALIZE_MODE_VISIBLE;

	//recording (server + vcr)
	g_settings.recording_type = configfile.getInt32("recording_type", RECORDING_OFF);
	g_settings.recording_stopplayback = configfile.getBool("recording_stopplayback", false);
	g_settings.recording_stopsectionsd = configfile.getInt32("recording_stopsectionsd", SECTIONSD_STOP);
	g_settings.recording_server_ip = configfile.getString("recording_server_ip", "10.10.10.10");
	strcpy( g_settings.recording_server_port, configfile.getString( "recording_server_port", "4000").c_str() );
	g_settings.recording_server_wakeup = configfile.getInt32( "recording_server_wakeup", 0 );
	g_settings.recording_server_mac = configfile.getString("recording_server_mac", "11:22:33:44:55:66");
	g_settings.recording_vcr_no_scart = configfile.getInt32( "recording_vcr_no_scart", false);
	g_settings.recording_max_rectime = configfile.getInt32( "recording_max_rectime", 4 );
	strcpy( g_settings.recording_splitsize_default, configfile.getString( "recording_splitsize_default", "2048").c_str() );
	g_settings.recording_use_o_sync            = configfile.getBool("recordingmenu.use_o_sync"           , false);
	g_settings.recording_use_fdatasync         = configfile.getBool("recordingmenu.use_fdatasync"        , false);
	g_settings.recording_audio_pids_default    = configfile.getInt32("recording_audio_pids_default", TIMERD_APIDS_STD );
	g_settings.recording_stream_vtxt_pid       = configfile.getBool("recordingmenu.stream_vtxt_pid"      , false);
	g_settings.recording_stream_subtitle_pid   = configfile.getBool("recordingmenu.stream_subtitle_pid"  , false);
	g_settings.recording_ringbuffers           = configfile.getInt32("recordingmenu.ringbuffers", 2);
	// compatibility conversion...
	if (g_settings.recording_ringbuffers > 4) {
		/* the "unit" for ringbuffers was 188*362 == 68056, but
		   the ringbuffer code can only take powers of two */
		if (g_settings.recording_ringbuffers > 61)
			g_settings.recording_ringbuffers = 4;	// --> 8MB
		else if (g_settings.recording_ringbuffers > 30)
			g_settings.recording_ringbuffers = 3;	// --> 4MB
		else						// the minimum value was 20
			g_settings.recording_ringbuffers = 2;	// --> 2MB
	}
	g_settings.recording_choose_direct_rec_dir = configfile.getInt32( "recording_choose_direct_rec_dir", 0 );
	g_settings.recording_in_spts_mode          = configfile.getBool("recording_in_spts_mode"         , true);
	g_settings.recording_zap_on_announce       = configfile.getBool("recording_zap_on_announce"      , false);
	g_settings.recording_filename_template_default = configfile.getString("recording_filename_template_default", "%C_%T_%d_%t");
	strncpy(g_settings.recording_dir_permissions, configfile.getString("recording_dir_permissions", "755").c_str(), 3);
	g_settings.recording_dir_permissions[3] = '\0';
	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		std::string i_str(to_string(i));
		g_settings.recording_dir[i] = configfile.getString("recording_dir_" + i_str, "" );
		g_settings.recording_filename_template[i] = configfile.getString("recording_filename_template_" + i_str, "" );
		strcpy(g_settings.recording_splitsize[i], configfile.getString("recording_splitsize_" + i_str, "" ).c_str());
	}
	g_settings.recording_gen_psi           = configfile.getBool("recordingmenu.gen_psi", true);
	g_settings.recording_nhd_compatible_ts = configfile.getBool("recordingmenu.nhd_compatible_ts", true); 

	//streaming (server)
	g_settings.streaming_type = configfile.getInt32( "streaming_type", 0 );
	g_settings.streaming_server_ip = configfile.getString("streaming_server_ip", "10.10.10.10");
	strcpy( g_settings.streaming_server_port, configfile.getString( "streaming_server_port", "8080").c_str() );
	strcpy( g_settings.streaming_server_cddrive, configfile.getString("streaming_server_cddrive", "D:").c_str() );
	strcpy( g_settings.streaming_videorate,  configfile.getString("streaming_videorate", "1000").c_str() );
	strcpy( g_settings.streaming_audiorate, configfile.getString("streaming_audiorate", "192").c_str() );
	strcpy( g_settings.streaming_server_startdir, configfile.getString("streaming_server_startdir", "C:/Movies").c_str() );
	g_settings.streaming_transcode_audio = configfile.getInt32( "streaming_transcode_audio", 0 );
	g_settings.streaming_force_transcode_video = configfile.getInt32( "streaming_force_transcode_video", 0 );
	g_settings.streaming_transcode_video_codec = configfile.getInt32( "streaming_transcode_video_codec", 0 );
	g_settings.streaming_force_avi_rawaudio = configfile.getInt32( "streaming_force_avi_rawaudio", 0 );
#ifdef ENABLE_MOVIEPLAYER
	g_settings.streaming_resolution = configfile.getInt32( "streaming_resolution", CMoviePlayerGui::RES_352X288 );
#else
	g_settings.streaming_resolution = configfile.getInt32( "streaming_resolution", 0 );
#endif
	g_settings.streaming_vlc10 = configfile.getInt32( "streaming_vlc10", 0);
	g_settings.streaming_use_buffer = configfile.getInt32("streaming_use_buffer", 1);
	g_settings.streaming_buffer_segment_size = configfile.getInt32("streaming_buffer_segment_size", 24);
	g_settings.streaming_stopsectionsd = configfile.getInt32("streaming_stopsectionsd", SECTIONSD_STOP);
	g_settings.streaming_show_tv_in_browser = configfile.getInt32("streaming_show_tv_in_browser", 0);
	g_settings.streaming_allow_multiselect = configfile.getBool("streaming_allow_multiselect", false);
	g_settings.streaming_use_reclength = configfile.getBool("streaming_use_reclength", false);
	g_settings.streaming_moviedir = configfile.getString( "streaming_moviedir", "" );

	// default plugin for movieplayer
	g_settings.movieplayer_plugin = configfile.getString( "movieplayer_plugin", "Teletext" );

	//rc-key configuration
#ifdef HAVE_TRIPLEDRAGON
	g_settings.key_tvradio_mode = (neutrino_msg_t)configfile.getInt32("key_tvradio_mode", CRCInput::RC_tv);
	g_settings.key_channelList_pageup = (neutrino_msg_t)configfile.getInt32("key_channelList_pageup", CRCInput::RC_page_up);
	g_settings.key_channelList_pagedown = (neutrino_msg_t)configfile.getInt32("key_channelList_pagedown", CRCInput::RC_page_down);
#else
	g_settings.key_tvradio_mode = (neutrino_msg_t)configfile.getInt32("key_tvradio_mode", CRCInput::RC_nokey);
	g_settings.key_channelList_pageup = (neutrino_msg_t)configfile.getInt32("key_channelList_pageup", CRCInput::RC_minus);
	g_settings.key_channelList_pagedown = (neutrino_msg_t)configfile.getInt32("key_channelList_pagedown", CRCInput::RC_plus);
#endif
	g_settings.key_channelList_cancel = (neutrino_msg_t)configfile.getInt32("key_channelList_cancel", CRCInput::RC_home);
	g_settings.key_channelList_sort = (neutrino_msg_t)configfile.getInt32("key_channelList_sort", CRCInput::RC_blue);
	g_settings.key_channelList_search = (neutrino_msg_t)configfile.getInt32("key_channelList_search", CRCInput::RC_green);
	g_settings.key_channelList_addrecord = (neutrino_msg_t)configfile.getInt32("key_channelList_addrecord", CRCInput::RC_red);
	g_settings.key_channelList_addremind = (neutrino_msg_t)configfile.getInt32("key_channelList_addremind", CRCInput::RC_yellow);
	g_settings.key_channelList_reload = (neutrino_msg_t)configfile.getInt32("key_channelList_reload", CRCInput::RC_setup);

	g_settings.key_quickzap_up = (neutrino_msg_t)configfile.getInt32("key_quickzap_up", CRCInput::RC_up);
	g_settings.key_quickzap_down = (neutrino_msg_t)configfile.getInt32("key_quickzap_down", CRCInput::RC_down);
	g_settings.key_volume_up = (neutrino_msg_t)configfile.getInt32("key_volume_up", CRCInput::RC_plus);
	g_settings.key_volume_down = (neutrino_msg_t)configfile.getInt32("key_volume_down", CRCInput::RC_minus);
	g_settings.key_bouquet_up = (neutrino_msg_t)configfile.getInt32("key_bouquet_up", CRCInput::RC_right);
	g_settings.key_bouquet_down = (neutrino_msg_t)configfile.getInt32("key_bouquet_down", CRCInput::RC_left);
	g_settings.key_subchannel_up = (neutrino_msg_t)configfile.getInt32("key_subchannel_up", CRCInput::RC_right);
	g_settings.key_subchannel_down = (neutrino_msg_t)configfile.getInt32("key_subchannel_down", CRCInput::RC_left);
	g_settings.key_subchannel_toggle = (neutrino_msg_t)configfile.getInt32("key_subchannel_toggle", CRCInput::RC_0);
	g_settings.key_zaphistory = (neutrino_msg_t)configfile.getInt32("key_zaphistory", CRCInput::RC_home);
	g_settings.key_lastchannel = (neutrino_msg_t)configfile.getInt32("key_lastchannel", CRCInput::RC_0);

	g_settings.key_menu_pageup = (neutrino_msg_t)configfile.getInt32("key_menu_pageup", CRCInput::RC_page_up);
	g_settings.key_menu_pagedown = (neutrino_msg_t)configfile.getInt32("key_menu_pagedown", CRCInput::RC_page_down);

#ifdef HAVE_DBOX_HARDWARE
	g_settings.repeat_blocker        = configfile.getInt32("repeat_blocker"       , g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS ? 150 : 25);
	g_settings.repeat_genericblocker = configfile.getInt32("repeat_genericblocker", g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS ?  25 :  0);
#else
	// my dm500s and tripledragon works good with those - seife
	g_settings.repeat_blocker        = configfile.getInt32("repeat_blocker", 300);
	g_settings.repeat_genericblocker = configfile.getInt32("repeat_genericblocker", 100);
#endif
	g_settings.audiochannel_up_down_enable = configfile.getBool("audiochannel_up_down_enable", false);
	g_settings.audio_left_right_selectable = configfile.getBool("audio_left_right_selectable", false);

	//screen configuration
	g_settings.screen_xres = configfile.getInt32("screen_xres", 100);
	g_settings.screen_yres = configfile.getInt32("screen_yres", 100);
	g_settings.screen_StartX = configfile.getInt32( "screen_StartX", 50 );
	g_settings.screen_StartY = configfile.getInt32( "screen_StartY", 20 );
	g_settings.screen_EndX = configfile.getInt32( "screen_EndX", 680 );
	g_settings.screen_EndY = configfile.getInt32( "screen_EndY", 545 );
	frameBuffer->setScreenSize(g_settings.screen_StartX, g_settings.screen_StartY, g_settings.screen_EndX, g_settings.screen_EndY);

#ifndef DISABLE_INTERNET_UPDATE
	//Software-update
	g_settings.softupdate_mode          = configfile.getInt32( "softupdate_mode", CFlashUpdate::UPDATEMODE_INTERNET );
	g_settings.softupdate_url_file      = configfile.getString("softupdate_url_file", "/etc/update.urls" );
	g_settings.softupdate_proxyserver   = configfile.getString("softupdate_proxyserver", "" );
	g_settings.softupdate_proxyusername = configfile.getString("softupdate_proxyusername", "" );
	g_settings.softupdate_proxypassword = configfile.getString("softupdate_proxypassword", "" );
#endif
	// GUI font
	g_settings.font_file = configfile.getString( "font_file", FONTDIR"/LiberationSans-Regular.ttf" );

	//BouquetHandling
	g_settings.bouquetlist_mode = configfile.getInt32( "bouquetlist_mode", bsmChannels );

	// parentallock
	if (!parentallocked)
  	{
	  	g_settings.parentallock_prompt = configfile.getInt32( "parentallock_prompt", 0 );
		g_settings.parentallock_lockage = configfile.getInt32( "parentallock_lockage", 12 );
	}
	else
	{
	        g_settings.parentallock_prompt = 3;
	        g_settings.parentallock_lockage = 18;
	}
	strcpy( g_settings.parentallock_pincode, configfile.getString( "parentallock_pincode", "0000" ).c_str() );

	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		g_settings.timing[i] = configfile.getInt32(locale_real_names[timing_setting[i].name], timing_setting[i].default_timing);

	for (int i = 0; i < SNeutrinoSettings::LCD_SETTING_COUNT; i++)
		g_settings.lcd_setting[i] = configfile.getInt32(lcd_setting[i].name, lcd_setting[i].default_value);
	strcpy(g_settings.lcd_setting_dim_time, configfile.getString("lcd_dim_time","0").c_str());
	strcpy(g_settings.lcd_setting_dim_brightness, configfile.getString("lcd_dim_brightness","0").c_str());

	//Picture-Viewer
	strcpy( g_settings.picviewer_slide_time, configfile.getString( "picviewer_slide_time", "10" ).c_str() );
	g_settings.picviewer_scaling = configfile.getInt32("picviewer_scaling", 1 /*(int)CPictureViewer::SIMPLE*/);
	g_settings.picviewer_decode_server_ip = configfile.getString("picviewer_decode_server_ip", "");
	strcpy(g_settings.picviewer_decode_server_port, configfile.getString("picviewer_decode_server_port", "").c_str());
	strcpy( g_settings.picviewer_picturedir, configfile.getString( "picviewer_picturedir", "" ).c_str() );

	//Audio-Player
#ifdef ENABLE_AUDIOPLAYER
	g_settings.audioplayer_display = configfile.getInt32("audioplayer_display",(int)CAudioPlayerGui::ARTIST_TITLE);
#else
	g_settings.audioplayer_display = configfile.getInt32("audioplayer_display",0);
#endif
	g_settings.audioplayer_follow  = configfile.getInt32("audioplayer_follow",0);
	strcpy( g_settings.audioplayer_screensaver, configfile.getString( "audioplayer_screensaver", "0" ).c_str() );
	g_settings.audioplayer_highprio  = configfile.getInt32("audioplayer_highprio",0);
	g_settings.audioplayer_select_title_by_name = configfile.getInt32("audioplayer_select_title_by_name",0);
	g_settings.audioplayer_repeat_on = configfile.getInt32("audioplayer_repeat_on",0);
	g_settings.audioplayer_show_playlist = configfile.getInt32("audioplayer_show_playlist",1);
	g_settings.audioplayer_enable_sc_metadata = configfile.getInt32("audioplayer_enable_sc_metadata",1);
	strcpy( g_settings.audioplayer_audioplayerdir, configfile.getString( "audioplayer_audioplayerdir", "" ).c_str() );

	//Esound
	strcpy(g_settings.esound_port, configfile.getString("esound_port", "").c_str());

	//Filebrowser
	g_settings.filebrowser_showrights =  configfile.getInt32("filebrowser_showrights", 1);
	g_settings.filebrowser_sortmethod = configfile.getInt32("filebrowser_sortmethod", 0);
	if ((g_settings.filebrowser_sortmethod < 0) || (g_settings.filebrowser_sortmethod >= FILEBROWSER_NUMBER_OF_SORT_VARIANTS))
		g_settings.filebrowser_sortmethod = 0;
	g_settings.filebrowser_denydirectoryleave = configfile.getBool("filebrowser_denydirectoryleave", false);

	// USERMENU
	//-------------------------------------------
	// this is as the current neutrino usermenu
	const char* usermenu_default[SNeutrinoSettings::BUTTON_MAX]={
		"2,3,4,16",			// RED
		"6",				// GREEN
		"3",				// YELLOW
		"9,8,1,15,1,10,11,13,1,14,5"	// BLUE
	};
	char txt1[81];
	std::string txt2;
	const char* txt2ptr;
	for(int button = 0; button < SNeutrinoSettings::BUTTON_MAX; button++)
	{
		snprintf(txt1,80,"usermenu_tv_%s_text",usermenu_button_def[button]);
		txt1[80] = 0; // terminate for sure

		if(usermenu_default[button] == "3")
			g_settings.usermenu_text[button] = configfile.getString(txt1, "Tv Guide" );
		else if(usermenu_default[button] == "2,3,4,16")
			g_settings.usermenu_text[button] = configfile.getString(txt1, "Tv Info" );
		else
			g_settings.usermenu_text[button] = configfile.getString(txt1, "" );

		snprintf(txt1,80,"usermenu_tv_%s",usermenu_button_def[button]);
		txt2 = configfile.getString(txt1,usermenu_default[button]);
		txt2ptr = txt2.c_str();
		for( int pos = 0; pos < SNeutrinoSettings::ITEM_MAX; pos++)
		{
			// find next comma or end of string - if it's not the first round
			if(pos != 0)
			{
				while(*txt2ptr != 0 && *txt2ptr != ',')
					txt2ptr++;
				if(*txt2ptr != 0)
					txt2ptr++;
			}
			if(*txt2ptr != 0)
			{
				g_settings.usermenu[button][pos] = atoi(txt2ptr);  // there is still a string
				if(g_settings.usermenu[button][pos] >= SNeutrinoSettings::ITEM_MAX)
					g_settings.usermenu[button][pos] = 0;
			}
			else
				g_settings.usermenu[button][pos] = 0;     // string empty, fill up with 0
		}
	}
	
	if(configfile.getUnknownKeyQueryedFlag() && (erg==0))
	{
		erg = 2;
	}

	if (!scanSettings.loadSettings(NEUTRINO_SCAN_SETTINGS_FILE, (g_info.delivery_system = g_Zapit->getDeliverySystem())))
	{
		dprintf(DEBUG_NORMAL, "Loading of scan settings failed. Using defaults.\n");
	}

	// uboot config file
	if(fromflash)
	{
		g_settings.uboot_baudrate	= 9600;
		g_settings.uboot_console	= UBOOT_CONSOLE_SERIAL;
		g_settings.uboot_dbox_duplex	= 0;
		g_settings.uboot_lcd_inverse	= -1;
		g_settings.uboot_lcd_contrast	= -1;
		g_settings.uboot_lcd_bias	= 0;

		FILE* fd = fopen("/var/tuxbox/boot/boot.conf", "r");
		if(fd)
		{
			char buffer[100];

			while(fgets(buffer, 99, fd) != NULL)
			{
				if(strncmp(buffer,"console=",8) == 0)
				{
					if(strncmp(&buffer[8], "null", 4)==0)
						g_settings.uboot_console = UBOOT_CONSOLE_NULL;
					else if(strncmp(&buffer[8], "ttyS0", 5)==0)
						g_settings.uboot_console = UBOOT_CONSOLE_SERIAL;
					else if(strncmp(&buffer[8], "tty", 3)==0)
						g_settings.uboot_console = UBOOT_CONSOLE_FB;
				}
				else if(strncmp(buffer,"baudrate=", 9) == 0)
				{
					g_settings.uboot_baudrate = atoi(&buffer[9]);
				}
				else if(strncmp(buffer,"dbox_duplex=", 12) == 0)
				{
					g_settings.uboot_dbox_duplex = atoi(&buffer[12]);
				}
				else if(strncmp(buffer,"lcd_inverse=", 12) == 0)
				{
					g_settings.uboot_lcd_inverse = atoi(&buffer[12]);
				}
				else if(strncmp(buffer,"lcd_contrast=", 13) == 0)
				{
					g_settings.uboot_lcd_contrast = atoi(&buffer[13]);
				}
				else if(strncmp(buffer,"lcd_bias=", 9) == 0)
				{
					g_settings.uboot_lcd_bias = atoi(&buffer[9]);
				}
				else
					printf("[neutrino] unknown entry '%s' found in boot.conf\n", buffer);
			}

			fclose(fd);
		}
		g_settings.uboot_console_bak = g_settings.uboot_console;
		g_settings.uboot_dbox_duplex_bak = g_settings.uboot_dbox_duplex;
	}

	return erg;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  saveSetup, save the application-settings                   *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::saveSetup()
{
	//uboot; write config only on changes
	if (fromflash &&
		((g_settings.uboot_console_bak != g_settings.uboot_console ) ||
		( g_settings.uboot_dbox_duplex_bak != g_settings.uboot_dbox_duplex ) ||
		( g_settings.uboot_lcd_bias  != g_settings.lcd_setting[SNeutrinoSettings::LCD_BIAS] ) ||
		( g_settings.uboot_lcd_inverse  != g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE] ) ||
		( g_settings.uboot_lcd_contrast != g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST] ) ))
	{
		FILE* fd = fopen("/var/tuxbox/boot/boot.conf", "w");

		if(fd != NULL)
		{
			const char * buffer;
			g_settings.uboot_console_bak    = g_settings.uboot_console;
			g_settings.uboot_lcd_inverse	= g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE];
			g_settings.uboot_lcd_contrast	= g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST];
			g_settings.uboot_lcd_bias  	= g_settings.lcd_setting[SNeutrinoSettings::LCD_BIAS];
			g_settings.uboot_dbox_duplex_bak= g_settings.uboot_dbox_duplex;

			switch(g_settings.uboot_console)
			{
			case UBOOT_CONSOLE_SERIAL:
				buffer = "ttyS0";
				break;
			case UBOOT_CONSOLE_FB:
				buffer = "tty";
				break;
			default:
				buffer = "null";
				break;
			}

			fprintf(fd,	"console=%s\n"
					"baudrate=%d\n"
					"dbox_duplex=%d\n"
					"lcd_inverse=%d\n"
					"lcd_contrast=%d\n"
					"lcd_bias=%d\n",
					buffer,
					g_settings.uboot_baudrate,
					g_settings.uboot_dbox_duplex,
					g_settings.uboot_lcd_inverse,
					g_settings.uboot_lcd_contrast,
					g_settings.uboot_lcd_bias);
			fclose(fd);
		}
		else
		{
			fprintf(stderr, "[neutrino] unable to write file /var/tuxbox/boot/boot.conf\n");
		}
	}

	//scan settings
	if(!scanSettings.saveSettings(NEUTRINO_SCAN_SETTINGS_FILE))
	{
		dprintf(DEBUG_NORMAL, "error while saving scan-settings!\n");
	}

	//video
	configfile.setInt32( "video_Format", g_settings.video_Format );
	configfile.setInt32( "video_backgroundFormat", g_settings.video_backgroundFormat );

	configfile.setInt32( "video_csync", g_settings.video_csync );

	//fb-alphavalues for gtx
	configfile.setInt32( "gtx_alpha1", g_settings.gtx_alpha1 );
	configfile.setInt32( "gtx_alpha2", g_settings.gtx_alpha2 );

	// EPG-Config
	configfile.setString("epg_cache_time"          ,g_settings.epg_cache );
	configfile.setString("epg_extendedcache_time"  ,g_settings.epg_extendedcache );
	configfile.setString("epg_old_events"          ,g_settings.epg_old_events );
	configfile.setString("epg_max_events"          ,g_settings.epg_max_events );
	configfile.setString("epg_dir"                 ,g_settings.epg_dir);
#ifdef ENABLE_FREESATEPG
	configfile.setBool("epg_freesat_enabled"       ,g_settings.epg_freesat_enabled );
#endif
	// EPG-View
	configfile.setInt32("bigFonts", g_settings.bigFonts);

	//misc
	configfile.setBool("standby_save_power"        , g_settings.standby_save_power);
	configfile.setBool("shutdown_real"             , g_settings.shutdown_real);
	configfile.setBool("shutdown_real_rcdelay"     , g_settings.shutdown_real_rcdelay);
	configfile.setInt32("standby_off_with"         , g_settings.standby_off_with);
	configfile.setString("shutdown_count"          , g_settings.shutdown_count);
	configfile.setInt32("sleeptimer_min"           , g_settings.sleeptimer_min);
	configfile.setInt32("volumebar_disp_pos" , g_settings.volumebar_disp_pos);
	configfile.setBool("infobar_sat_display"       , g_settings.infobar_sat_display);
	configfile.setInt32("infobar_subchan_disp_pos" , g_settings.infobar_subchan_disp_pos);
	configfile.setBool("misc_spts"                 , g_settings.misc_spts);
#ifndef TUXTXT_CFG_STANDALONE
	configfile.setBool("tuxtxt_cache"              , g_settings.tuxtxt_cache);
#endif
	configfile.setBool("virtual_zap_mode"          , g_settings.virtual_zap_mode);
	configfile.setBool("progressbar_color"		, g_settings.progressbar_color);
	configfile.setInt32("infobar_show"             , g_settings.infobar_show);
	configfile.setInt32("show_mute_icon"			, g_settings.show_mute_icon);
	configfile.setInt32("channellist_additional", g_settings.channellist_additional);
	configfile.setBool("channellist_epgtext_align_right"                 , g_settings.channellist_epgtext_align_right);
	configfile.setBool("channellist_extended"                 , g_settings.channellist_extended);
	configfile.setInt32("channellist_foot"                 , g_settings.channellist_foot);
	configfile.setInt32("infobar_show_channellogo"			, g_settings.infobar_show_channellogo);
	configfile.setString("infobar_channel_logodir"	, g_settings.infobar_channel_logodir);
	configfile.setInt32( "infobar_channellogo_background"	, g_settings.infobar_channellogo_background);
	configfile.setInt32("startmode"                , g_settings.startmode);
	configfile.setInt32("wzap_time"                , g_settings.wzap_time);
	configfile.setBool("radiotext_enable"          , g_settings.radiotext_enable);

	//audio
	configfile.setInt32( "audio_AnalogMode" , g_settings.audio_AnalogMode);
	configfile.setBool("audio_DolbyDigital" , g_settings.audio_DolbyDigital);
	configfile.setInt32( "audio_initial_volume" , g_settings.audio_initial_volume);
	configfile.setInt32( "audio_avs_Control", g_settings.audio_avs_Control);
	configfile.setInt32( "audio_PCMOffset" , g_settings.audio_PCMOffset);
	configfile.setInt32( "audio_step"	, g_settings.audio_step);
	configfile.setInt32( "audio_ReSync" , g_settings.audio_ReSync);
	configfile.setInt32( "audio_ReSync_timer" , g_settings.audio_ReSync_timer);

	//Audio Priority Pids
	char audio_propids_key[4];
	configfile.setBool("audio_propids_enabled", g_settings.audio_propids_enabled);
	for(int i=0 ; i < AUDIO_PRIORITY_NR_OF_ENTRIES ; i++)
	{
		sprintf(audio_propids_key, "audio_propids_name%d", i);
		configfile.setString(audio_propids_key, g_settings.audio_propids_name[i]);
	}

	//vcr
	configfile.setBool("vcr_AutoSwitch"     , g_settings.vcr_AutoSwitch);

	//language
	configfile.setString("language"         , g_settings.language);
	configfile.setString("timezone"		, g_settings.timezone);

	//widget settings
	configfile.setBool("widget_fade"        , g_settings.widget_fade);

	//colors
	CThemes::setColors(configfile);

	//corners
	configfile.setBool( "rounded_corners", g_settings.rounded_corners );

	//menu numbers
	configfile.setBool( "menu_numbers_as_icons", g_settings.menu_numbers_as_icons );

	//network
	for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
	{
		std::string i_str(to_string(i));
		configfile.setString("network_nfs_ip_" + i_str            , g_settings.network_nfs[i].ip );
		configfile.setString("network_nfs_dir_" + i_str           , g_settings.network_nfs[i].dir );
		configfile.setString("network_nfs_local_dir_" + i_str     , g_settings.network_nfs[i].local_dir );
		configfile.setInt32 ("network_nfs_automount_" + i_str     , g_settings.network_nfs[i].automount );
		configfile.setInt32 ("network_nfs_type_" + i_str          , g_settings.network_nfs[i].type );
		configfile.setString("network_nfs_username_" + i_str      , g_settings.network_nfs[i].username );
		configfile.setString("network_nfs_password_" + i_str      , g_settings.network_nfs[i].password );
		configfile.setString("network_nfs_mount_options1_" + i_str, g_settings.network_nfs[i].mount_options1 );
		configfile.setString("network_nfs_mount_options2_" + i_str, g_settings.network_nfs[i].mount_options2 );
		configfile.setString("network_nfs_mac_" + i_str           , g_settings.network_nfs[i].mac );
	}
	configfile.setBool  ( "filesystem_is_utf8" , g_settings.filesystem_is_utf8);
#ifdef ENABLE_SAMBASERVER
	//samba
	configfile.setInt32( "smb_setup_samba_on_off", g_settings.smb_setup_samba_on_off );
	configfile.setString ( "smb_setup_samba_workgroup", g_settings.smb_setup_samba_workgroup);
#endif

	// Personalization
	configfile.setInt32 ( "personalize_pinstatus", g_settings.personalize_pinstatus );
	configfile.setInt32 ( "personalize_bluebutton", g_settings.personalize_bluebutton );
	configfile.setInt32 ( "personalize_redbutton", g_settings.personalize_redbutton );

	configfile.setString( "personalize_pincode", g_settings.personalize_pincode);
	configfile.setInt32 ( "personalize_tvmode", g_settings.personalize_tvmode );
	configfile.setInt32 ( "personalize_radiomode", g_settings.personalize_radiomode );
	configfile.setInt32 ( "personalize_scartmode", g_settings.personalize_scartmode );
	configfile.setInt32 ( "personalize_games", g_settings.personalize_games );
	configfile.setInt32 ( "personalize_audioplayer", g_settings.personalize_audioplayer );
	configfile.setInt32 ( "personalize_inetradio", g_settings.personalize_inetradio );
	configfile.setInt32 ( "personalize_esound", g_settings.personalize_esound );
	configfile.setInt32 ( "personalize_movieplayer", g_settings.personalize_movieplayer );
	configfile.setInt32 ( "personalize_pictureviewer", g_settings.personalize_pictureviewer );
#ifdef ENABLE_UPNP
 	configfile.setInt32 ( "personalize_upnpbrowser", g_settings.personalize_upnpbrowser );
#endif
	configfile.setInt32 ( "personalize_scripts", g_settings.personalize_scripts );

	// do not allow users to pin protect settings menu,
	// we have added a root password reset item that is none personalized.
	// configfile.setInt32 ( "personalize_settings", g_settings.personalize_settings );
	// configfile.setInt32 ( "personalize_service", g_settings.personalize_service );

	configfile.setInt32 ( "personalize_sleeptimer", g_settings.personalize_sleeptimer );
	configfile.setInt32 ( "personalize_reboot", g_settings.personalize_reboot );
	configfile.setInt32 ( "personalize_shutdown", g_settings.personalize_shutdown );

	configfile.setInt32 ( "personalize_bouqueteditor", g_settings.personalize_bouqueteditor );
	configfile.setInt32 ( "personalize_scants", g_settings.personalize_scants );
	configfile.setInt32 ( "personalize_reload", g_settings.personalize_reload );
	configfile.setInt32 ( "personalize_getplugins", g_settings.personalize_getplugins );
	configfile.setInt32 ( "personalize_restart", g_settings.personalize_restart );
	configfile.setInt32 ( "personalize_epgrestart", g_settings.personalize_epgrestart );
	configfile.setInt32 ( "personalize_ucodecheck", g_settings.personalize_ucodecheck );
	configfile.setInt32 ( "personalize_imageinfo", g_settings.personalize_imageinfo );
	configfile.setInt32 ( "personalize_update", g_settings.personalize_update );
	configfile.setInt32 ( "personalize_chan_epg_stat", g_settings.personalize_chan_epg_stat );
#ifdef ENABLE_DRIVE_GUI
	configfile.setInt32 ( "personalize_drive_setup_stat", g_settings.personalize_drive_setup_stat );
#endif

	configfile.setInt32 ( "personalize_audio", g_settings.personalize_audio );
	configfile.setInt32 ( "personalize_video", g_settings.personalize_video );
	configfile.setInt32 ( "personalize_youth", g_settings.personalize_youth );
	configfile.setInt32 ( "personalize_network", g_settings.personalize_network );
	configfile.setInt32 ( "personalize_recording", g_settings.personalize_recording );
	configfile.setInt32 ( "personalize_keybinding", g_settings.personalize_keybinding );
	configfile.setInt32 ( "personalize_colors", g_settings.personalize_colors );
	configfile.setInt32 ( "personalize_lcd", g_settings.personalize_lcd );
	configfile.setInt32 ( "personalize_mediaplayer", g_settings.personalize_mediaplayer );
	configfile.setInt32 ( "personalize_driver", g_settings.personalize_driver );
	configfile.setInt32 ( "personalize_misc", g_settings.personalize_misc );


	// NTP-Server for sectionsd
	configfile.setString( "network_ntpserver", g_settings.network_ntpserver);
	configfile.setString( "network_ntprefresh", g_settings.network_ntprefresh);
	configfile.setBool( "network_ntpenable", g_settings.network_ntpenable);

	//recording (server + vcr)
	configfile.setInt32 ("recording_type",                      g_settings.recording_type);
	configfile.setBool  ("recording_stopplayback"             , g_settings.recording_stopplayback         );
	configfile.setInt32 ("recording_stopsectionsd"            , g_settings.recording_stopsectionsd        );
	configfile.setString("recording_server_ip",                 g_settings.recording_server_ip);
	configfile.setString("recording_server_port",               g_settings.recording_server_port);
	configfile.setInt32 ("recording_server_wakeup",             g_settings.recording_server_wakeup);
	configfile.setString("recording_server_mac",                g_settings.recording_server_mac);
	configfile.setInt32 ("recording_vcr_no_scart",              g_settings.recording_vcr_no_scart);
	configfile.setInt32 ("recording_max_rectime",               g_settings.recording_max_rectime);
	configfile.setString("recording_splitsize_default",         g_settings.recording_splitsize_default);
	configfile.setBool  ("recordingmenu.use_o_sync"           , g_settings.recording_use_o_sync           );
	configfile.setBool  ("recordingmenu.use_fdatasync"        , g_settings.recording_use_fdatasync        );
	configfile.setInt32 ("recording_audio_pids_default"       , g_settings.recording_audio_pids_default);
	configfile.setBool  ("recordingmenu.stream_vtxt_pid"      , g_settings.recording_stream_vtxt_pid      );
	configfile.setBool  ("recordingmenu.stream_subtitle_pid"  , g_settings.recording_stream_subtitle_pid);
	configfile.setInt32 ("recordingmenu.ringbuffers"          , g_settings.recording_ringbuffers);
	configfile.setBool  ("recordingmenu.gen_psi"              , g_settings.recording_gen_psi);
	configfile.setBool  ("recordingmenu.nhd_compatible_ts"    , g_settings.recording_nhd_compatible_ts);
	configfile.setInt32 ("recording_choose_direct_rec_dir"    , g_settings.recording_choose_direct_rec_dir);
	configfile.setBool  ("recording_in_spts_mode"             , g_settings.recording_in_spts_mode         );
	configfile.setBool  ("recording_zap_on_announce"          , g_settings.recording_zap_on_announce      );
	configfile.setString("recording_filename_template_default", g_settings.recording_filename_template_default);
	configfile.setString("recording_dir_permissions"          , g_settings.recording_dir_permissions);
	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		std::string i_str(to_string(i));
		configfile.setString("recording_dir_" + i_str              , g_settings.recording_dir[i] );
		configfile.setString("recording_filename_template_" + i_str, g_settings.recording_filename_template[i] );
		configfile.setString("recording_splitsize_" + i_str        , g_settings.recording_splitsize[i] );
	}

	//streaming
	configfile.setInt32 ( "streaming_type", g_settings.streaming_type );
	configfile.setString( "streaming_server_ip", g_settings.streaming_server_ip );
	configfile.setString( "streaming_server_port", g_settings.streaming_server_port );
	configfile.setString( "streaming_server_cddrive", g_settings.streaming_server_cddrive );
	configfile.setString( "streaming_videorate", g_settings.streaming_videorate );
	configfile.setString( "streaming_audiorate", g_settings.streaming_audiorate );
	configfile.setString( "streaming_server_startdir", g_settings.streaming_server_startdir );
	configfile.setInt32 ( "streaming_transcode_audio", g_settings.streaming_transcode_audio );
	configfile.setInt32 ( "streaming_force_avi_rawaudio", g_settings.streaming_force_avi_rawaudio );
	configfile.setInt32 ( "streaming_force_transcode_video", g_settings.streaming_force_transcode_video );
	configfile.setInt32 ( "streaming_transcode_video_codec", g_settings.streaming_transcode_video_codec );
	configfile.setInt32 ( "streaming_resolution", g_settings.streaming_resolution );
	configfile.setInt32 ( "streaming_vlc10", g_settings.streaming_vlc10 );
	configfile.setInt32 ( "streaming_use_buffer", g_settings.streaming_use_buffer);
	configfile.setInt32 ( "streaming_buffer_segment_size", g_settings.streaming_buffer_segment_size);
	configfile.setInt32 ( "streaming_stopsectionsd", g_settings.streaming_stopsectionsd);
	configfile.setInt32 ( "streaming_show_tv_in_browser", g_settings.streaming_show_tv_in_browser);
	configfile.setBool ("streaming_allow_multiselect", g_settings.streaming_allow_multiselect);
	configfile.setBool ("streaming_use_reclength", g_settings.streaming_use_reclength);
	configfile.setString( "streaming_moviedir", g_settings.streaming_moviedir);

	// default plugin for movieplayer
	configfile.setString( "movieplayer_plugin", g_settings.movieplayer_plugin );

	//rc-key configuration
	configfile.setInt32 ( "key_tvradio_mode", (int)g_settings.key_tvradio_mode );

	configfile.setInt32( "key_channelList_pageup", (int)g_settings.key_channelList_pageup );
	configfile.setInt32( "key_channelList_pagedown", (int)g_settings.key_channelList_pagedown );
	configfile.setInt32( "key_channelList_cancel", (int)g_settings.key_channelList_cancel );
	configfile.setInt32( "key_channelList_sort", (int)g_settings.key_channelList_sort );
	configfile.setInt32( "key_channelList_search", (int)g_settings.key_channelList_search );
	configfile.setInt32( "key_channelList_addrecord", (int)g_settings.key_channelList_addrecord );
	configfile.setInt32( "key_channelList_addremind", (int)g_settings.key_channelList_addremind );
	configfile.setInt32( "key_channelList_reload",  (int)g_settings.key_channelList_reload );

	configfile.setInt32( "key_quickzap_up", (int)g_settings.key_quickzap_up );
	configfile.setInt32( "key_quickzap_down", (int)g_settings.key_quickzap_down );
	configfile.setInt32( "key_volume_up", (int)g_settings.key_volume_up );
	configfile.setInt32( "key_volume_down", (int)g_settings.key_volume_down );
	configfile.setInt32( "key_bouquet_up", (int)g_settings.key_bouquet_up );
	configfile.setInt32( "key_bouquet_down", (int)g_settings.key_bouquet_down );
	configfile.setInt32( "key_subchannel_up", (int)g_settings.key_subchannel_up );
	configfile.setInt32( "key_subchannel_down", (int)g_settings.key_subchannel_down );
	configfile.setInt32( "key_subchannel_toggle", (int)g_settings.key_subchannel_toggle );
	configfile.setInt32( "key_zaphistory", (int)g_settings.key_zaphistory );
	configfile.setInt32( "key_lastchannel", (int)g_settings.key_lastchannel );

	configfile.setInt32( "key_menu_pageup", (int)g_settings.key_menu_pageup );
	configfile.setInt32( "key_menu_pagedown", (int)g_settings.key_menu_pagedown );

	configfile.setInt32( "repeat_blocker", g_settings.repeat_blocker );
	configfile.setInt32( "repeat_genericblocker", g_settings.repeat_genericblocker );
	configfile.setBool  ( "audiochannel_up_down_enable", g_settings.audiochannel_up_down_enable );
	configfile.setBool  ( "audio_left_right_selectable", g_settings.audio_left_right_selectable );

	//screen configuration
	configfile.setInt32( "screen_xres", g_settings.screen_xres);
	configfile.setInt32( "screen_yres", g_settings.screen_yres);
	configfile.setInt32( "screen_StartX", g_settings.screen_StartX );
	configfile.setInt32( "screen_StartY", g_settings.screen_StartY );
	configfile.setInt32( "screen_EndX", g_settings.screen_EndX );
	configfile.setInt32( "screen_EndY", g_settings.screen_EndY );

#ifndef DISABLE_INTERNET_UPDATE
	//Software-update
	configfile.setInt32 ("softupdate_mode"          , g_settings.softupdate_mode          );
	configfile.setString("softupdate_url_file"      , g_settings.softupdate_url_file      );
	configfile.setString("softupdate_proxyserver"   , g_settings.softupdate_proxyserver   );
	configfile.setString("softupdate_proxyusername" , g_settings.softupdate_proxyusername );
	configfile.setString("softupdate_proxypassword" , g_settings.softupdate_proxypassword );
#endif
	// GUI font
	configfile.setString("font_file", g_settings.font_file);

	//BouquetHandling
	configfile.setInt32( "bouquetlist_mode", g_settings.bouquetlist_mode );

	//parentallock
	configfile.setInt32( "parentallock_prompt", g_settings.parentallock_prompt );
	configfile.setInt32( "parentallock_lockage", g_settings.parentallock_lockage );
	configfile.setString( "parentallock_pincode", g_settings.parentallock_pincode );

	//timing
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		configfile.setInt32(locale_real_names[timing_setting[i].name], g_settings.timing[i]);

	for (int i = 0; i < SNeutrinoSettings::LCD_SETTING_COUNT; i++)
		configfile.setInt32(lcd_setting[i].name, g_settings.lcd_setting[i]);
	configfile.setString("lcd_dim_time", g_settings.lcd_setting_dim_time);
	configfile.setString("lcd_dim_brightness", g_settings.lcd_setting_dim_brightness);

	//Picture-Viewer
	configfile.setString( "picviewer_slide_time", g_settings.picviewer_slide_time );
	configfile.setInt32( "picviewer_scaling", g_settings.picviewer_scaling );
	configfile.setString( "picviewer_decode_server_ip", g_settings.picviewer_decode_server_ip );
	configfile.setString( "picviewer_decode_server_port", g_settings.picviewer_decode_server_port);
	configfile.setString( "picviewer_picturedir", g_settings.picviewer_picturedir);

	//Audio-Player
	configfile.setInt32( "audioplayer_display", g_settings.audioplayer_display );
	configfile.setInt32( "audioplayer_follow", g_settings.audioplayer_follow );
	configfile.setString( "audioplayer_screensaver", g_settings.audioplayer_screensaver );
	configfile.setInt32( "audioplayer_highprio", g_settings.audioplayer_highprio );
	configfile.setInt32( "audioplayer_select_title_by_name", g_settings.audioplayer_select_title_by_name );
	configfile.setInt32( "audioplayer_repeat_on", g_settings.audioplayer_repeat_on );
	configfile.setInt32( "audioplayer_show_playlist", g_settings.audioplayer_show_playlist );
	configfile.setInt32( "audioplayer_enable_sc_metadata", g_settings.audioplayer_enable_sc_metadata );
	configfile.setString( "audioplayer_audioplayerdir", g_settings.audioplayer_audioplayerdir);

	//Esound
	configfile.setString( "esound_port", g_settings.esound_port);

	//Filebrowser
	configfile.setInt32("filebrowser_showrights", g_settings.filebrowser_showrights);
	configfile.setInt32("filebrowser_sortmethod", g_settings.filebrowser_sortmethod);
	configfile.setBool("filebrowser_denydirectoryleave", g_settings.filebrowser_denydirectoryleave);

	// USERMENU
	//---------------------------------------
	char txt1[81];
	char txt2[81];
	for(int button = 0; button < SNeutrinoSettings::BUTTON_MAX; button++)
	{
		snprintf(txt1,80,"usermenu_tv_%s_text",usermenu_button_def[button]);
		txt1[80] = 0; // terminate for sure
		configfile.setString(txt1,g_settings.usermenu_text[button]);

		char* txt2ptr = txt2;
		snprintf(txt1,80,"usermenu_tv_%s",usermenu_button_def[button]);
		for(int pos = 0; pos < SNeutrinoSettings::ITEM_MAX; pos++)
		{
			if( g_settings.usermenu[button][pos] != 0)
			{
				if(pos != 0)
					*txt2ptr++ = ',';
				txt2ptr += snprintf(txt2ptr,80,"%d",g_settings.usermenu[button][pos]);
			}
		}
		configfile.setString(txt1,txt2);
	}

	if (configfile.getModifiedFlag())
	{
		dprintf(DEBUG_INFO, "saving neutrino txt-config\n");
		configfile.saveConfig(NEUTRINO_SETTINGS_FILE);
	}

}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  resetPassword, set password back to default                *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::resetPassword()
{
    FILE* fd = fopen("/var/etc/passwd", "w");

    if(fd != NULL)
    {
        fprintf(fd,"root::0:0::/:/bin/sh\n"
                "sshd:*:65532:65534::/:/bin/false\n"
                "ftp:*:65533:65534::/:/bin/false\n"
                "nobody:*:65534:65534::/:/bin/false");
        fclose(fd);
    }
    else
    {
        fprintf(stderr, "[neutrino] unable to write file /var/etc/passwd\n");
    }
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  firstChannel, get the initial channel                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::firstChannel()
{
	g_Zapit->getLastChannel(firstchannel.channelNumber, firstchannel.mode);
	if (!g_Zapit->getSaveLastChannel() && obeyStartMode)
	{
		if (!zapto_tv_on_init_done && firstchannel.mode == 't')
		{
			firstchannel.channelNumber = g_Zapit->getStartChannelTV() + 1;
		}
		else if (!zapto_radio_on_init_done && firstchannel.mode == 'r')
		{
			firstchannel.channelNumber = g_Zapit->getStartChannelRadio() + 1;
		}
	}
}

#ifdef HAVE_DBOX_HARDWARE
/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  ucodes_available, check if ucodes are available            *
*                                                                                     *
**************************************************************************************/

bool CNeutrinoApp::ucodes_available(void)
{
	FILE * fd;

	fd = fopen(UCODEDIR "/avia500.ux", "r");
	if (fd)
		fclose(fd);

	bool ucodes_ok = (fd);

	fd = fopen(UCODEDIR "/avia600.ux", "r");
	if (fd)
		fclose(fd);
	ucodes_ok = ucodes_ok || (fd);

	fd = fopen(UCODEDIR "/cam-alpha.bin", "r");
	if (fd)
		fclose(fd);
	ucodes_ok = ucodes_ok && (fd);

	return ucodes_ok;
}
#endif


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  channelsInit, get the Channellist from daemon              *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::channelsInit(int init_mode, int _mode)
{
	dprintf(DEBUG_DEBUG, "doing channelsInit\n");

	if (init_mode == init_mode_switch) {
		if (mode_radio == _mode) {
			if (channelListRADIO && bouquetListRADIO) {
				channelList = channelListRADIO;
				bouquetList = bouquetListRADIO;
				// otherwise zapit will not tune to the new TP
				channelList->clearTuned();
				return;
			} // else load
		} 
		else if (mode_tv == _mode) {
			if (channelListTV && bouquetListTV) {
				channelList = channelListTV;
				bouquetList = bouquetListTV;
				// otherwise zapit will not tune to the new TP
				channelList->clearTuned();
				return;
			} // else load
		}
	}

	if (mode_unknown == _mode) { // stay in current TV/Radio mode
		if (channelList == channelListTV) {
			_mode = mode_tv;
		} else {
			_mode = mode_radio;
		}
	}

	CZapitClient::BouquetChannelList zapitChannels;
	CZapitClient::BouquetList zapitBouquets;

	//deleting old channelList for mode-switching.
	if (channelListTV)
		delete channelListTV;
	channelListTV = new CChannelList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));

	g_Zapit->getChannels(zapitChannels, CZapitClient::MODE_TV, CZapitClient::SORT_BOUQUET, true); // UTF-8
	for(uint i=0; i<zapitChannels.size(); i++)
	{
		channelListTV->addChannel(zapitChannels[i].nr, zapitChannels[i].nr, zapitChannels[i].name, zapitChannels[i].satellitePosition, zapitChannels[i].channel_id); // UTF-8
	}

	if (bouquetListTV)
		delete bouquetListTV;
	bouquetListTV = new CBouquetList();
	bouquetListTV->orgChannelList = channelListTV;

	/* load non-empty bouquets only */
	g_Zapit->getBouquets(zapitBouquets, false, true, CZapitClient::MODE_TV); // UTF-8
	for(uint i = 0; i < zapitBouquets.size(); i++)
	{
		zapitChannels.clear();

		bouquetListTV->addBouquet(zapitBouquets[i].name, zapitBouquets[i].bouquet_nr, zapitBouquets[i].locked);
		g_Zapit->getBouquetChannels(zapitBouquets[i].bouquet_nr, zapitChannels, CZapitClient::MODE_TV, true); // UTF-8

		for (uint j = 0; j < zapitChannels.size(); j++)
		{
			CChannelList::CChannel* channel = channelListTV->getChannel(zapitChannels[j].nr);
	
			/* observe that "bouquetList->Bouquets[i]" refers to the bouquet we just created using bouquetList->addBouquet */
			bouquetListTV->Bouquets[i]->channelList->addChannel(channel);
			if (zapitBouquets[i].locked)
			{
				channel->bAlwaysLocked = true;

				for (int k=0; k<channelListTV->getSize(); k++)
				{
					if ((*channelListTV)[k]->channel_id == zapitChannels[j].channel_id) {
						(*channelListTV)[k]->bAlwaysLocked = true;
					}
				}
			}
		}
	}

	zapitBouquets.clear();
	zapitChannels.clear();

	// same for the RADIO channels
	if (channelListRADIO)
		delete channelListRADIO;
	channelListRADIO = new CChannelList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));

	g_Zapit->getChannels(zapitChannels, CZapitClient::MODE_RADIO, CZapitClient::SORT_BOUQUET, true); // UTF-8
	for(uint i=0; i<zapitChannels.size(); i++)
	{
		channelListRADIO->addChannel(zapitChannels[i].nr, zapitChannels[i].nr, zapitChannels[i].name, zapitChannels[i].satellitePosition, zapitChannels[i].channel_id); // UTF-8
	}

	if (bouquetListRADIO)
		delete bouquetListRADIO;
	bouquetListRADIO = new CBouquetList();
	bouquetListRADIO->orgChannelList = channelListRADIO;

	/* load non-empty bouquets only */
	g_Zapit->getBouquets(zapitBouquets, false, true, CZapitClient::MODE_RADIO); // UTF-8
	for(uint i = 0; i < zapitBouquets.size(); i++)
	{
		zapitChannels.clear();

		bouquetListRADIO->addBouquet(zapitBouquets[i].name, zapitBouquets[i].bouquet_nr, zapitBouquets[i].locked);
		g_Zapit->getBouquetChannels(zapitBouquets[i].bouquet_nr, zapitChannels, CZapitClient::MODE_RADIO, true); // UTF-8

		for (uint j = 0; j < zapitChannels.size(); j++)
		{
			CChannelList::CChannel* channel = channelListRADIO->getChannel(zapitChannels[j].nr);

			/* observe that "bouquetList->Bouquets[i]" refers to the bouquet we just created using bouquetList->addBouquet */
			bouquetListRADIO->Bouquets[i]->channelList->addChannel(channel);

			if (zapitBouquets[i].locked)
			{
				channel->bAlwaysLocked = true;
			}
		}
	}

	dprintf(DEBUG_DEBUG, "\nAll bouquets-channels received\n");
	if (_mode == mode_radio) {
		if (channelListRADIO && bouquetListRADIO) {
			channelList = channelListRADIO;
			bouquetList = bouquetListRADIO;
			// otherwise zapit will not tune to the new TP
			channelList->clearTuned();
		}
	} 
	else if (_mode == mode_tv) {
		if (channelListTV && bouquetListTV) {
			channelList = channelListTV;
			bouquetList = bouquetListTV;
			// otherwise zapit will not tune to the new TP
			channelList->clearTuned();
		}
	}
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  channelsInit4Record, get the Channellist from daemon       *
*                          during RecordMode                                          *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::channelsInit4Record(void)
{
	dprintf(DEBUG_DEBUG, "doing channelsInit4Record\n");

	CZapitClient::BouquetChannelList zapitChannels;
	CZapitClient::BouquetList zapitBouquets;
	int channel_nr = 0;
	std::map<int, int> channel_nr_map;

	//deleting old channelList for mode-switching.
	if (channelListRecord)
		delete channelListRecord;
	channelListRecord = new CChannelList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));

	g_Zapit->getChannels(zapitChannels, CZapitClient::MODE_CURRENT, CZapitClient::SORT_BOUQUET, true); // UTF-8
	for(uint i=0; i<zapitChannels.size(); i++)
	{
		channel_nr++;
		channel_nr_map[zapitChannels[i].nr] = channel_nr;
		channelListRecord->addChannel(channel_nr, channel_nr, zapitChannels[i].name, zapitChannels[i].satellitePosition, zapitChannels[i].channel_id); // UTF-8
	}

	if (bouquetListRecord)
		delete bouquetListRecord;
	bouquetListRecord = new CBouquetList();
	bouquetListRecord->orgChannelList = channelListRecord;

	/* load non-empty bouquets only */
	g_Zapit->getBouquets(zapitBouquets, false, true); // UTF-8
	for(uint i = 0; i < zapitBouquets.size(); i++)
	{
		zapitChannels.clear();

		bouquetListRecord->addBouquet(zapitBouquets[i].name, zapitBouquets[i].bouquet_nr, zapitBouquets[i].locked);
		g_Zapit->getBouquetChannels(zapitBouquets[i].bouquet_nr, zapitChannels, CZapitClient::MODE_CURRENT, true); // UTF-8

		for (uint j = 0; j < zapitChannels.size(); j++)
		{
			channel_nr = channel_nr_map[zapitChannels[j].nr];
			CChannelList::CChannel* channel = channelListRecord->getChannel(channel_nr);

			/* observe that "bouquetList->Bouquets[i]" refers to the bouquet we just created using bouquetList->addBouquet */
			bouquetListRecord->Bouquets[i]->channelList->addChannel(channel);

			if (zapitBouquets[i].locked)
			{
				channel->bAlwaysLocked = true;
			}
		}
	}

	channelList = channelListRecord;
	bouquetList = bouquetListRecord;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  run, the main runloop                                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::CmdParser(int argc, char **argv)
{
	global_argv = new char *[argc+1];
	for (int i = 0; i < argc; i++)
		global_argv[i] = argv[i];
	global_argv[argc] = NULL;

	softupdate = false;
	fromflash = false;

	font.name = NULL;
	font.filename = NULL;
	lcd_font.filename[0] = NULL;

	for(int x=1; x<argc; x++)
	{
		if ((!strcmp(argv[x], "-u")) || (!strcmp(argv[x], "--enable-update")))
		{
			dprintf(DEBUG_NORMAL, "Software update enabled\n");
			softupdate = true;
		}
		else if ((!strcmp(argv[x], "-f")) || (!strcmp(argv[x], "--enable-flash")))
		{
			dprintf(DEBUG_NORMAL, "enable flash\n");
			fromflash = true;
		}
		else if (!strcmp(argv[x], "--lcdfont"))
		{
			if ((x + 1) < argc)
			{
				lcd_font.filename[0] = argv[x + 1];
				if ((x + 2) < argc)
				{
					lcd_font.filename[1] = argv[x + 2];
					x++;
				}
				else
					lcd_font.filename[1] = NULL;

				if ((x + 2) < argc)
				{
					lcd_font.filename[2] = argv[x + 2];
					x++;
				}
				else
					lcd_font.filename[2] = NULL;
			}
			x++;
		}
		else if (((!strcmp(argv[x], "-v")) || (!strcmp(argv[x], "--verbose"))) && (x+1 < argc))
		{
			int dl = atoi(argv[x+ 1]);
			dprintf(DEBUG_NORMAL, "set debuglevel: %d\n", dl);
			setDebugLevel(dl);
			x++;
		}
		else
		{
			fprintf(stderr, "Usage: neutrino [-u | --enable-update] [-f | --enable-flash] [-v | --verbose 0..3] [--lcdfont /dir/menu.ttf [/dir/time.ttf [/dir/channelname.ttf]]]\n");
			exit(0);
		}
	}
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup the framebuffer                                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::SetupFrameBuffer()
{
	frameBuffer->init();
	if(frameBuffer->setMode(720, 576, 8 * sizeof(fb_pixel_t)))
	{
		dprintf(DEBUG_NORMAL, "Error while setting framebuffer mode\n");
		exit(-1);
	}

	//make 1..8 transparent for dummy painting
	for(int count =0;count<8;count++)
		frameBuffer->paletteSetColor(count, 0x000000, 0xffff);
	frameBuffer->paletteSet();
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup fonts                                                *
*                                                                                     *
**************************************************************************************/

const lcd_font_descr_struct predefined_lcd_font[2] =
{
	{ {FONTDIR "/12.pcf.gz", FONTDIR "/14B.pcf.gz", FONTDIR "/15B.pcf.gz"} },
	{ {FONTDIR "/LiberationSans-Regular.ttf", NULL, NULL} }
};

bool CNeutrinoApp::ChangeFonts(int unicode_locale)
{
	if (unicode_locale == CLocaleManager::NO_SUCH_LOCALE)	/* should not happen, but anyway.. */
		return false;				/* avoid crash from negative array index */
	if (loadLocale_ret != unicode_locale)
	{
		loadLocale_ret = unicode_locale;
		if (unicode_locale == CLocaleManager::UNICODE_FONT &&
		    g_settings.font_file == FONTDIR"/LiberationSans-Regular.ttf")
		{
			g_settings.font_file = FONTDIR"/LiberationSans-Regular.ttf";
			SetupFonts();
		}
		if (lcd_font.filename[0] == NULL)
			CLCD::getInstance()->reinit(predefined_lcd_font[unicode_locale].filename[0],
			                            predefined_lcd_font[unicode_locale].filename[1],
			                            predefined_lcd_font[unicode_locale].filename[2]);
	}
	return true;
}

void CNeutrinoApp::SetupFonts()
{
	const char * style[3];

	if (g_fontRenderer != NULL)
		delete g_fontRenderer;

	g_fontRenderer = new FBFontRenderClass(72 * g_settings.screen_xres / 100, 72 * g_settings.screen_yres / 100);

	if(font.filename != NULL)
		free((void *)font.filename);

	printf("[neutrino] settings font file %s\n", g_settings.font_file.c_str());

	if(access(g_settings.font_file.c_str(), F_OK)) {
		if(!access(FONTDIR"/LiberationSans-Regular.ttf", F_OK)){
			font.filename = strdup(FONTDIR"/LiberationSans-Regular.ttf");
			g_settings.font_file = font.filename;
		}
		else{
			fprintf( stderr,"[neutrino] font file [%s] not found\n neutrino exit\n",FONTDIR"/LiberationSans-Regular.ttf");
			_exit(0);
		}

	}
	else{
		font.filename = strdup(g_settings.font_file.c_str());
	}
	style[FONT_STYLE_REGULAR] = g_fontRenderer->AddFont(font.filename);

	if(font.name != NULL)
		free((void *)font.name);

	font.name = strdup(g_fontRenderer->getFamily(font.filename).c_str());

	printf("[neutrino] font family %s\n", font.name);

	style[FONT_STYLE_BOLD] = "Bold Regular";

	style[FONT_STYLE_ITALIC] = g_fontRenderer->AddFont(font.filename, true);  // make italics

	for (int i = 0; i < SNeutrinoSettings::FONT_TYPE_COUNT; i++)
	{
		delete g_Font[i];
		g_Font[i] = g_fontRenderer->getFont(font.name, style[neutrino_font[i].style], configfile.getInt32(locale_real_names[neutrino_font[i].name], neutrino_font[i].defaultsize)/* + neutrino_font[i].size_offset * font.size_offset*/);
	}

	/* recalculate infobar, EPGData and eventlist position */
	if (g_InfoViewer)
		g_InfoViewer->start();
	if (g_EpgData)
		g_EpgData->start();
	if (g_EventList)
		g_EventList->init();
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup the menu timouts                                     *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::SetupTiming()
{
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		sprintf(g_settings.timing_string[i], "%d", g_settings.timing[i]);
}


bool CNeutrinoApp::doGuiRecord(char * preselectedDir, bool addTimer, char * filename)
{
	CTimerd::RecordingInfo eventinfo;
	bool refreshGui = false;
	/* helpful for debugging...
	if (filename != NULL)
		 printf("CNeutrinoApp::doGuiRecord: filename == '%s'\n",filename);
	 else
		 printf("CNeutrinoApp::doGuiRecord filename == NULL\n");
	 */
	if(CVCRControl::getInstance()->isDeviceRegistered())
	{
		if(recordingstatus == 1)
		{
			puts("[neutrino.cpp] executing " NEUTRINO_RECORDING_START_SCRIPT ".");
			if (my_system(NEUTRINO_RECORDING_START_SCRIPT) != 0)
				perror(NEUTRINO_RECORDING_START_SCRIPT " failed");

			eventinfo.channel_id = g_Zapit->getCurrentServiceID();
			eventinfo.eventRepeat = CTimerd::TIMERREPEAT_ONCE;
			CEPGData epgData;
			if (filename == NULL && g_Sectionsd->getActualEPGServiceKey(g_RemoteControl->current_channel_id, &epgData))
			{
				//printf("CNeutrinoApp::doGuiRecord got info from sectionsd\n");
				eventinfo.epgID = epgData.eventID;
				eventinfo.epg_starttime = epgData.epg_times.startzeit;
				strncpy(eventinfo.epgTitle, epgData.title.c_str(), EPG_TITLE_MAXLEN-1);
				eventinfo.epgTitle[EPG_TITLE_MAXLEN-1]=0;
			}
			else
			{
				eventinfo.epgID = 0;
				eventinfo.epg_starttime = 0;
				strcpy(eventinfo.epgTitle, "");
			}
			if (filename != NULL)
			{
				CTimerd::responseGetTimer timer;
				g_Timerd->getTimer(timer, recording_id);
				eventinfo.apids = timer.apids;
			}
			else
				eventinfo.apids = TIMERD_APIDS_CONF;
			bool doRecord = true;
			if (g_settings.recording_type == RECORDING_FILE)
			{
				std::string recDir = (preselectedDir != NULL) ? preselectedDir : g_settings.recording_dir[0];
				if( preselectedDir == NULL && g_settings.recording_choose_direct_rec_dir)
				{
					CRecDirChooser recDirs(LOCALE_TIMERLIST_RECORDING_DIR, NEUTRINO_ICON_TIMER, NULL, &recDir);
					recDirs.exec(NULL,"");
					refreshGui = true;
					recDir = recDirs.get_selected_dir();
					//printf("dir : %s\n",recDir.c_str());
					if (!recDir.empty())
					{
						int nfs_nr = getNFSIDOfDir(recDir.c_str());
						if(nfs_nr != -1)
						{
							recDir = g_settings.network_nfs[nfs_nr].local_dir;
#ifdef ENABLE_GUI_MOUNT
							if (!CFSMounter::isMounted(g_settings.network_nfs[nfs_nr].local_dir))
							{
				         		printf("not mounted, try to mount: %d\n",nfs_nr);
								CFSMounter::MountRes mres =
									CFSMounter::mount(g_settings.network_nfs[nfs_nr].ip,
											  g_settings.network_nfs[nfs_nr].dir,
											  g_settings.network_nfs[nfs_nr].local_dir,
											  (CFSMounter::FSType) g_settings.network_nfs[nfs_nr].type,
											  g_settings.network_nfs[nfs_nr].username,
											  g_settings.network_nfs[nfs_nr].password,
											  g_settings.network_nfs[nfs_nr].mount_options1,
											  g_settings.network_nfs[nfs_nr].mount_options2);
								if (mres != CFSMounter::MRES_OK)
								{
									doRecord = false;
									std::string msg = mntRes2Str(mres) + "\nDir: " + g_settings.network_nfs[nfs_nr].local_dir;
									ShowMsgUTF(LOCALE_MESSAGEBOX_ERROR, msg.c_str(),
											CMessageBox::mbrBack, CMessageBox::mbBack,NEUTRINO_ICON_ERROR, 450, 10); // UTF-8
								}
							}
#endif
						}
					}
					else
					{
						doRecord = false;
					}
				}
				CVCRControl::CFileDevice *fileDevice;
				if ((fileDevice = dynamic_cast<CVCRControl::CFileDevice*>(recordingdevice)) != NULL)
				{
					if (preselectedDir != NULL) {
						// recording has been interrupted, we are starting again
						// all directories have already been created so we should not create them again
						fileDevice->CreateTemplateDirectories = false;
					}
					fileDevice->Directory = recDir;

					if (filename != NULL) {
						//printf("CNeutrinoApp::doGuiRecord: given filename: %s\n", filename);
						fileDevice->FilenameTemplate = filename;
					}
					else
					{
						fileDevice->FilenameTemplate = g_settings.recording_filename_template_default;
						for (int i = 0; i < MAX_RECORDING_DIR; i++)
						{
							if (!g_settings.recording_filename_template[i].empty() && g_settings.recording_dir[i] == recDir)
							{
								fileDevice->FilenameTemplate = g_settings.recording_filename_template[i];
								break;
							}
						}
					}

					unsigned int splitsize;
					sscanf(g_settings.recording_splitsize_default, "%u", &splitsize);
					for (int i = 0; i < MAX_RECORDING_DIR; i++)
					{
						if (strlen(g_settings.recording_splitsize[i]) > 0 && g_settings.recording_dir[i] == recDir)
						{
							sscanf(g_settings.recording_splitsize[i], "%u", &splitsize);
							break;
						}
					}
					fileDevice->SplitSize = splitsize;
				}
				else
				{
					puts("[neutrino] could not set directory and filename template");
				}
			}
			if(!doRecord || (CVCRControl::getInstance()->Record(&eventinfo)==false))
			{
				recordingstatus=0;
				return refreshGui;
			}
			else if (addTimer)
			{
				time_t now = time(NULL);
				recording_id = g_Timerd->addImmediateRecordTimerEvent(
											eventinfo.channel_id,
											now,
											now + g_settings.recording_max_rectime * 60 * 60,
											eventinfo.epgID,
											eventinfo.epg_starttime,
											eventinfo.apids);
			}
		}
		else
		{
			g_Timerd->stopTimerEvent(recording_id);
			startNextRecording();
		}
		return refreshGui;
	}
	else
		puts("[neutrino.cpp] no recording devices");
	return false;
}

void CNeutrinoApp::SendSectionsdConfig(void)
{
	CSectionsdClient::epg_config config;
	config.scanMode 		= scanSettings.scanSectionsd;
	config.epg_cache 		= atoi(g_settings.epg_cache.c_str());
	config.epg_extendedcache        = atoi(g_settings.epg_extendedcache.c_str());
	config.epg_old_events 		= atoi(g_settings.epg_old_events.c_str());
	config.epg_max_events		= atoi(g_settings.epg_max_events.c_str());
	config.epg_dir			= g_settings.epg_dir;
	config.audio_ReSync		= g_settings.audio_ReSync;
	config.audio_ReSync_timer	= g_settings.audio_ReSync_timer;
	config.network_ntpserver	= g_settings.network_ntpserver;
	config.network_ntprefresh	= atoi(g_settings.network_ntprefresh.c_str());
	config.network_ntpenable	= g_settings.network_ntpenable;
#ifdef ENABLE_FREESATEPG
	config.epg_freesat_enabled	= g_settings.epg_freesat_enabled;
#endif
	g_Sectionsd->setConfig(config);
}

#define LCD_UPDATE_TIME_RADIO_MODE (6 * 1000 * 1000)
#define LCD_UPDATE_TIME_TV_MODE (60 * 1000 * 1000)

void CNeutrinoApp::InitZapper()
{

	g_InfoViewer->start();
	g_EpgData->start();

	firstChannel();

	// EPG-Config
	SendSectionsdConfig();

	if (!g_settings.epg_dir.empty())
		g_Sectionsd->readSIfromXML(g_settings.epg_dir.c_str());

#ifndef TUXTXT_CFG_STANDALONE
	if(g_settings.tuxtxt_cache)
	{
		tuxtxt_init();
	}
#endif

#ifdef HAVE_DBOX_HARDWARE
	// set initial PES/SPTS mode
	if (g_settings.misc_spts != g_Zapit->PlaybackState())
	{
		if (g_settings.misc_spts && firstchannel.mode == 't' )
			g_Zapit->PlaybackSPTS();
		else
			g_Zapit->PlaybackPES();
	}
#endif

	if(
	/* If Neutrino is started at boot time switch to TV */
	(obeyStartMode && (
	/* if the startmode is TV or */
	(g_settings.startmode == STARTMODE_TV) ||
	/* the last zapit mode should be used in case it was TV. */
	(g_settings.startmode == STARTMODE_RESTORE && firstchannel.mode == 't'))) ||
	/* If Neutrino is restarted later switch to firstchannel mode. */
	(!obeyStartMode && firstchannel.mode == 't'))
	{
		channelsInit(init_mode_init, mode_tv);
		tvMode(true);
	}
	else if(
	/* If Neutrino is started at boot time switch to Radio */
	(obeyStartMode && (
	/* if the startmode is Radio or */
	(g_settings.startmode == STARTMODE_RADIO) ||
	/* the last zapit mode should be used in case it was not TV. */
	(g_settings.startmode == STARTMODE_RESTORE && firstchannel.mode != 't'))) ||
	/* If Neutrino is restarted later switch to firstchannel mode. */
	(!obeyStartMode && firstchannel.mode != 't'))
	{
		channelsInit(init_mode_init, mode_radio);
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_RADIO_MODE, false );
		radioMode(true);
	}
	else
	{
		// channel lists need to be initialized, they are needed
		// when switching back from non TV/radio modes
		// use the last known zapit mode for this
		tunerMode = mode = firstchannel.mode == 'r' ? mode_radio : mode_tv;
		channelsInit(init_mode_init, tunerMode);
	}
}

void CNeutrinoApp::setupRecordingDevice(void)
{
	if (g_settings.recording_type == RECORDING_SERVER)
	{
		unsigned int port;
		sscanf(g_settings.recording_server_port, "%u", &port);

		recordingdevice = new CVCRControl::CServerDevice(g_settings.recording_stopplayback, g_settings.recording_stopsectionsd, g_settings.recording_server_ip.c_str(), port);

		CVCRControl::getInstance()->registerDevice(recordingdevice);
	}
	else if (g_settings.recording_type == RECORDING_FILE)
	{
		unsigned int splitsize, ringbuffers;
		sscanf(g_settings.recording_splitsize_default, "%u", &splitsize);
		ringbuffers = g_settings.recording_ringbuffers;

		recordingdevice = new CVCRControl::CFileDevice (g_settings.recording_stopplayback,
														g_settings.recording_stopsectionsd,
														g_settings.recording_dir[0].c_str(),
														splitsize,
														g_settings.recording_use_o_sync,
														g_settings.recording_use_fdatasync,
														g_settings.recording_stream_vtxt_pid,
														g_settings.recording_stream_subtitle_pid,
														ringbuffers, g_settings.recording_gen_psi,
														g_settings.recording_nhd_compatible_ts, true);

		CVCRControl::getInstance()->registerDevice(recordingdevice);
	}
	else if(g_settings.recording_type == RECORDING_VCR)
	{
		recordingdevice = new CVCRControl::CVCRDevice((g_settings.recording_vcr_no_scart == 0));

		CVCRControl::getInstance()->registerDevice(recordingdevice);
	}
	else
	{
		if (CVCRControl::getInstance()->isDeviceRegistered())
			CVCRControl::getInstance()->unregisterDevice();
	}
}

void CNeutrinoApp::prepareEnviroment()
/* use this function for preparing/create/customize 
	some stuff for an optimized enviroment for neutrino, 
	there are certainly more stuff that could be here now and in future */
{	
	/* check for existance of alternate folders for customizing in /var, and create this if not available,
		some folders are necessary, because of default values in neutrino settings exist and 
		there may be happen problems with handling of filebrowser */
	const char * const env_folders[] = 	{NEUTRINO_ICON_VARPATH,	// custom icons, logos
						 PLUGINDIR_VAR,		// custom plugins	
						 LCDDIR_VAR};		// custom lcd icons, logos
											 
	for (unsigned int i=0; i<sizeof(env_folders) / sizeof(env_folders[0]); i++)
	{
		if ( access(env_folders[i], F_OK) != 0 ) { 
			std::string cmd = (std::string)"mkdir -p " + env_folders[i];
			if (!execute_sys_command(cmd.c_str())) 
				printf("[neutrino] error creating %s\n", env_folders[i]);
			else
				printf("[neutrino] alternate folder created in: %s\n", env_folders[i]);
		}
	}
}

int CNeutrinoApp::run(int argc, char **argv)
{
	/* Obey startmode setting for no longer than 120s uptime */
	struct sysinfo info;
	long uptime;
	sysinfo(&info);
	uptime = info.uptime;
	if (uptime > 120)
		obeyStartMode = false;

	CmdParser(argc, argv);

	prepareEnviroment();
	current_muted = false;

	/* if zapit is in standby, loadSetup() will already get wrong info
	   (box type etc) */
	if (g_Zapit->getMode() == CZapitClient::MODE_STANDBY)
		g_Zapit->setStandby(false);

	int loadSettingsErg = loadSetup();

	// init volume
	if (obeyStartMode && g_settings.audio_initial_volume > 0)
		g_Controld->setVolume(g_settings.audio_initial_volume, (CControld::volume_type)g_settings.audio_avs_Control);

	/* load locales before setting up any fonts to determine whether we need a true unicode font */
	bool display_language_selection;
	loadLocale_ret = g_Locale->loadLocale(g_settings.language);
	if (loadLocale_ret == CLocaleManager::NO_SUCH_LOCALE)
	{
		strcpy(g_settings.language, "english");	// Fallback if rest fails
		FILE *f = fopen(NEUTRINO_DEFAULTLOCALE_FILE, "r");
		if (f)
		{
			char loc[100];
			int res = fscanf(f, "%s", loc);
			if (res > 0)
				strcpy(g_settings.language, loc);
			fclose(f);
		}
		loadLocale_ret = g_Locale->loadLocale(g_settings.language);
//		display_language_selection = true;
	}
//	else
		display_language_selection = false;

	unsigned int use_true_unicode_font = (loadLocale_ret == CLocaleManager::ISO_8859_1_FONT) ? 0 : 1;
	if (use_true_unicode_font)
		g_settings.font_file = FONTDIR"/LiberationSans-Regular.ttf";
	if (lcd_font.filename[0] == NULL) /* no lcd font specified in command line */
	{
		CLCD::getInstance()->init(predefined_lcd_font[use_true_unicode_font].filename[0],
		                          predefined_lcd_font[use_true_unicode_font].filename[1],
		                          predefined_lcd_font[use_true_unicode_font].filename[2]);
	}
	else
	{
		CLCD::getInstance()->init(lcd_font.filename[0], lcd_font.filename[1], lcd_font.filename[2]);
	}

	CLCD::getInstance()->showVolume(g_Controld->getVolume((CControld::volume_type)g_settings.audio_avs_Control));
	CLCD::getInstance()->setMuted(g_Controld->getMute((CControld::volume_type)g_settings.audio_avs_Control));

	g_info.box_Type = g_Controld->getBoxType();
	g_info.chip_info = g_Controld->getChipInfo();
	g_info.avia_chip = g_Controld->getAviaChip();

	dprintf( DEBUG_DEBUG, "[neutrino] box_Type: %d\n", g_info.box_Type);
	dprintf( DEBUG_DEBUG, "[neutrino] chip_info: %d\n", g_info.chip_info);
	dprintf( DEBUG_DEBUG, "[neutrino] avia_chip: %d\n", g_info.avia_chip);

	SetupTiming();

	SetupFonts();

	frameBuffer->ClearFrameBuffer();

	g_RCInput 	= new CRCInput;
	g_Sectionsd 	= new CSectionsdClient;
	g_Timerd 	= new CTimerdClient;
	g_RemoteControl = new CRemoteControl;
	g_EpgData 	= new CEpgData;
	g_InfoViewer 	= new CInfoViewer;
	g_EventList 	= new EventList;
	g_PluginList 	= new CPlugins;
	g_PluginList->setPluginDir(PLUGINDIR);

	/* unpause sectionsd, needed if Neutrino restarts after segfault */
	g_Sectionsd->setPauseScanning(false);

#ifdef ENABLE_GUI_MOUNT
	// mount shares before scanning for plugins
	CFSMounter::automount();
#endif

	//load Pluginlist before main menu (only show script menu if at least one
	// script is available
	g_PluginList->loadPlugins();

	networksetup = new CNetworkSetup();

	//USERMENU
	Timerlist			= new CTimerList;

	// setup recording device
	if (g_settings.recording_type != RECORDING_OFF)
		setupRecordingDevice();

	dprintf( DEBUG_NORMAL, "menue setup\n");
	//init Menues
	InitMenu();

	dprintf( DEBUG_NORMAL, "registering as event client\n");

	g_Controld->registerEvent(CControldClient::EVT_MUTECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_VOLUMECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_MODECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_VCRCHANGED, 222, NEUTRINO_UDS_NAME);

	g_Sectionsd->RegisterNeutrino();

#define ZAPIT_EVENT_COUNT 27
	const CZapitClient::events zapit_event[ZAPIT_EVENT_COUNT] =
	{
			CZapitClient::EVT_ZAP_COMPLETE,
			CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD,
			CZapitClient::EVT_ZAP_FAILED,
			CZapitClient::EVT_ZAP_SUB_COMPLETE,
			CZapitClient::EVT_ZAP_SUB_FAILED,
			CZapitClient::EVT_ZAP_MOTOR,
			CZapitClient::EVT_ZAP_CA_CLEAR,
			CZapitClient::EVT_ZAP_CA_LOCK,
			CZapitClient::EVT_ZAP_CA_FTA,
			CZapitClient::EVT_RECORDMODE_ACTIVATED,
			CZapitClient::EVT_RECORDMODE_DEACTIVATED,
			CZapitClient::EVT_SCAN_COMPLETE,
			CZapitClient::EVT_SCAN_FAILED,
			CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
			CZapitClient::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS,
			CZapitClient::EVT_SCAN_REPORT_FREQUENCY,
			CZapitClient::EVT_SCAN_REPORT_FREQUENCYP,
			CZapitClient::EVT_SCAN_SATELLITE,
			CZapitClient::EVT_SCAN_NUM_CHANNELS,
			CZapitClient::EVT_SCAN_PROVIDER,
			CZapitClient::EVT_BOUQUETS_CHANGED,
			CZapitClient::EVT_SCAN_SERVICENAME,
			CZapitClient::EVT_SCAN_FOUND_A_CHAN,
			CZapitClient::EVT_SCAN_FOUND_TV_CHAN,
			CZapitClient::EVT_SCAN_FOUND_RADIO_CHAN,
			CZapitClient::EVT_SCAN_FOUND_DATA_CHAN,
			CZapitClient::EVT_PMT_CHANGED
	};

	for (int i = 0; i < ZAPIT_EVENT_COUNT; i++)
		g_Zapit->registerEvent(zapit_event[i], 222, NEUTRINO_UDS_NAME);

	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_SHUTDOWN, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_SHUTDOWN, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_NEXTPROGRAM, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_NEXTPROGRAM, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_STANDBY_ON, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_STANDBY_OFF, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_RECORD, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_RECORD_START, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_RECORD_STOP, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_ZAPTO, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ZAPTO, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_SLEEPTIMER, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_SLEEPTIMER, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_REMIND, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_EXEC_PLUGIN, 222, NEUTRINO_UDS_NAME);

	//show language settings, if no language is configured
	if (display_language_selection)
	{
		COsdLangSetup languageSettings;
		languageSettings.exec(NULL, "");
	}


#ifdef HAVE_DBOX_HARDWARE
	if (!ucodes_available())
	{
		/* display error message */
		DisplayErrorMessage(g_Locale->getText(LOCALE_UCODES_FAILURE));

		/* show network settings dialog while first booting without ucodes*/
		networksetup->exec(NULL, "");
	}
#endif

	//settings
	if(loadSettingsErg==1)
	{
		dprintf(DEBUG_INFO, "config file missing\n");
		ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_SETTINGS_NOCONFFILE);
		configfile.setModifiedFlag(true);
		saveSetup();
	}
	else if(loadSettingsErg==2)
	{
		dprintf(DEBUG_INFO, "parts of configfile missing\n");
		ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_SETTINGS_MISSINGOPTIONSCONFFILE);
		configfile.setModifiedFlag(true);
		saveSetup();
	}

	//init scan settings
	CScanSetup scansetup;
	scansetup.initScanSettings();

	//init programm
	InitZapper();

	AudioMute( g_Controld->getMute((CControld::volume_type)g_settings.audio_avs_Control), true );

	// shutdown counter
	SHTDCNT::getInstance()->init();

	puts("[neutrino.cpp] executing " NEUTRINO_INIT_END_SCRIPT ".");
	if (my_system(NEUTRINO_INIT_END_SCRIPT) != 0)
		perror(NEUTRINO_INIT_END_SCRIPT " failed");

	RealRun(CPersonalizeGui::getInstance()->getWidget(0)/**main**/);

	ExitRun(true);

	return 0;
}


void CNeutrinoApp::RealRun(CMenuWidget &menu)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	dprintf(DEBUG_NORMAL, "initialized everything\n");
	
#ifdef ENABLE_LIRC
	CIRSend irs("neutrinoon");
	irs.Send();
#endif

#ifdef HAVE_TRIPLEDRAGON
#include <tddevices.h>
#include <avs/avs_inf.h>
#define VCR_DEVICE	"/dev/" DEVICE_NAME_AVS
#define VCR_IOCTL	IOC_AVS_GET_EVENT
/* the TD has 0/1=0V, 2=6V, 3=12V */
#define VCR_0V		1
#else
#define VCR_DEVICE	"/dev/dbox/fp0"
#define VCR_IOCTL	FP_IOCTL_GET_VCR
/* dbox has 0=0V, 1=6V, 2=12V */
#define VCR_0V		0
#endif

	if (g_settings.vcr_AutoSwitch)
	{
		int val = 0;
		int fp = open(VCR_DEVICE ,O_RDWR);
		if (fp >= 0)
		{
			ioctl(fp, VCR_IOCTL, &val);
			close(fp);
			if (val > VCR_0V)
			{
				dprintf(DEBUG_NORMAL, "Switching to scart mode...\n");
				lastMode = mode;
				handleMsg(NeutrinoMessages::EVT_VCRCHANGED, VCR_STATUS_ON);
			}
		}
	}

	if (obeyStartMode)
	{
		if(g_settings.startmode == STARTMODE_SCART)
		{
			lastMode = mode;
			handleMsg(NeutrinoMessages::EVT_VCRCHANGED, VCR_STATUS_ON);
		}
#ifdef ENABLE_AUDIOPLAYER
		else if(g_settings.startmode == STARTMODE_AUDIOPLAYER)
		{
			CAudioPlayerGui tmpAudioPlayerGui;
			tmpAudioPlayerGui.exec(NULL, "");
		}
#ifdef ENABLE_INTERNETRADIO
		else if(g_settings.startmode == STARTMODE_INETRADIO)
		{
			CAudioPlayerGui tmpAudioPlayerGui(true);
			tmpAudioPlayerGui.exec(NULL, "");
		}
#endif
#endif
#ifdef ENABLE_ESD
		else if(g_settings.startmode == STARTMODE_ESOUND)
		{
			CEsoundGui tmpEsoundGui;
			tmpEsoundGui.exec(NULL, "");
		}
#endif
		else if(g_settings.startmode == STARTMODE_STANDBY)
		{
			handleMsg(NeutrinoMessages::STANDBY_ON, 0);
		}
	}

	while( true )
	{
		g_RCInput->getMsg(&msg, &data, 100);	// 10 secs..
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if( ( mode == mode_tv ) || ( ( mode == mode_radio ) ) )
		{
			if( msg == NeutrinoMessages::SHOW_EPG )
			{
				// show EPG

//				g_EpgData->show( channelList->getActiveChannel_ChannelID() );
				g_EpgData->show( g_Zapit->getCurrentServiceID() );

			}
			else if(msg == g_settings.key_tvradio_mode)
			{
				if( mode == mode_tv )
				{
					radioMode();
				}
				else if( mode == mode_radio )
				{
					tvMode();
				}
			}
			else if( msg == CRCInput::RC_setup )
			{
				menu.exec(NULL, "");
			}
			else if(msg == CRCInput::RC_ok)
			{
				int bouqMode = g_settings.bouquetlist_mode;

				if((bouquetList!=NULL) && (bouquetList->Bouquets.empty()))
				{
					dprintf(DEBUG_DEBUG, "bouquets are empty\n");
					bouqMode = bsmAllChannels;
				}
				if((bouquetList!=NULL) && (bouqMode == bsmBouquets))
				{
					bouquetList->exec(true);
				}
				else if((bouquetList!=NULL) && (bouqMode == bsmChannels))
				{
					int nNewChannel = bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->show();
					if(nNewChannel>-1)
					{
						channelList->zapTo(bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->getKey(nNewChannel)-1);
					}
					else
					{
						bouquetList->adjustToChannel(channelList->getActiveChannelNumber());
					}
				}
				else
				{
					dprintf(DEBUG_DEBUG, "using all channels\n");
					channelList->exec();
				}
			}
#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
			// support for additional buttons on Dreambox Remote
			else if( msg == CRCInput::RC_tv )
			{
				if( mode != mode_tv )
				{
					tvMode();
				}
				else if (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_CAMDMENU_CAMDRESET,
					CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo) == CMessageBox::mbrYes) {
					CamdAuswahl * Camd = new CamdAuswahl();
					Camd->CamdReset();
					delete Camd;
				}
			}
			else if( msg == CRCInput::RC_radio )
			{
				if( mode != mode_radio )
				{
					radioMode();
				}
			}
			else if( msg == CRCInput::RC_text )
			{
				g_PluginList->startPlugin("tuxtxt");
			}
			else if(msg == CRCInput::RC_timer)
			{
				Timerlist->exec(NULL, "");
			}
			else if( msg == CRCInput::RC_audio )
			{
				// audio channel selection
				// showUserMenu(SNeutrinoSettings::BUTTON_GREEN);

				// audio button function changed to rezap for audio resync
				ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_ZAPIT_REZAP, 420, 1, NEUTRINO_ICON_INFO);
				channelList->ReZap();
			}
			else if( msg == CRCInput::RC_video )
			{
				// nvod selection
				showUserMenu(SNeutrinoSettings::BUTTON_YELLOW);
			}
#endif
			else if( msg == CRCInput::RC_red )
			{	// eventlist
				if (g_settings.personalize_redbutton == 0)
				{
					// EventList Menu - Personalization Check
					ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_PERSONALIZE_MENUDISABLEDHINT);
				} else {
					showUserMenu(SNeutrinoSettings::BUTTON_RED);  //USERMENU
				}
			}
			else if( msg == CRCInput::RC_blue )
			{	// streaminfo
				if (g_RemoteControl->director_mode)
				{
					g_RemoteControl->setSubChannel(DIRECTORMODE_PORTAL); //Portal RC_blue
					g_InfoViewer->showSubchan();
				}				
				else if (g_settings.personalize_bluebutton == 0)
				{
					// Features Menu - Personalization Check
					ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_PERSONALIZE_MENUDISABLEDHINT);
				}
				else 
					showUserMenu(SNeutrinoSettings::BUTTON_BLUE);
			}
			else if( msg == CRCInput::RC_green )
			{
				showUserMenu(SNeutrinoSettings::BUTTON_GREEN);
				}
			else if( msg == CRCInput::RC_yellow )
			{       // NVODs
				showUserMenu(SNeutrinoSettings::BUTTON_YELLOW);
			}
			else if (CRCInput::isNumeric(msg) && msg != CRCInput::RC_0 && g_RemoteControl->director_mode)
			{
				g_RemoteControl->setSubChannel(CRCInput::getNumericValue(msg));
				g_InfoViewer->showSubchan();
			}
			else if(msg_repeatok == g_settings.key_quickzap_up || msg_repeatok == g_settings.key_quickzap_down)
			{
				//quickzap
				channelList->quickZap(msg_repeatok);
			}
			else if( ( msg == CRCInput::RC_help ) ||
						( msg == NeutrinoMessages::SHOW_INFOBAR ) )
			{
			         // turn on LCD display
				CLCD::getInstance()->wake_up();
				// show Infoviewer
				g_InfoViewer->showTitle(channelList->getActiveChannelNumber(), channelList->getActiveChannelName(), channelList->getActiveSatellitePosition(), channelList->getActiveChannel_ChannelID()); // UTF-8
			}
			else if(msg_repeatok == g_settings.key_subchannel_up)
			{
				g_RemoteControl->subChannelUp();
				g_InfoViewer->showSubchan();
			}
			else if(msg_repeatok == g_settings.key_subchannel_down)
			{
				g_RemoteControl->subChannelDown();
				g_InfoViewer->showSubchan();
			}
			else if(msg == g_settings.key_subchannel_toggle && g_RemoteControl->director_mode)
			{
				g_RemoteControl->toggleSubChannel();
				g_InfoViewer->showSubchan();
			}
			else if(msg == g_settings.key_zaphistory)
			{
				// Zap-History "Bouquet"
				channelList->numericZap( msg );
			}
			else if(msg == g_settings.key_lastchannel)
			{
				// Quick Zap
				channelList->numericZap( msg );
			}
			else if (CRCInput::isNumeric(msg))
			{
				//numeric zap
				channelList->numericZap( msg );
			}
			else if (msg == CRCInput::RC_epg)
			{
				CEventListHandler elist;
				elist.exec(NULL, "");
			}
			else
			{     // turn on LCD display by kicking it
				if (msg == CRCInput::RC_home)
					CLCD::getInstance()->wake_up();
				handleMsg(msg, data);
			}

		}
		else
		{
			// mode == mode_scart
			if( msg == CRCInput::RC_home )
			{
				if( mode == mode_scart )
				{
					//wenn VCR Aufnahme dann stoppen
					if(CVCRControl::getInstance()->isDeviceRegistered())
					{
						if ((CVCRControl::getInstance()->Device->getDeviceType() == CVCRControl::DEVICE_VCR) &&
						    (CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_RECORD ||
						     CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_PAUSE))
						{
							CVCRControl::getInstance()->Stop();
							recordingstatus=0;
							startNextRecording();
						}
					}
					// Scart-Mode verlassen
#ifdef HAVE_TRIPLEDRAGON
					/* on the TD, we switch off power supply of the frontend in standby mode
					   this needs to be switched on again. The easiest way is to let "exit" not
					   return to "on" but fall back to "standby", a regular "leave standby"
					   event will switch on the power again.
					   Just trust me on that ;) */
					if (! wakeupfromScart)
						scartMode(false);
#else
					wakeupfromScart = false;
					scartMode( false );
#endif
				}
				else
					handleMsg(msg, data);
			}
			else
			{
				handleMsg(msg, data);
			}
		}
	}
}

int CNeutrinoApp::handleMsg(const neutrino_msg_t m, neutrino_msg_data_t data)
{
	int res = 0;
	bool repeat = false;
	neutrino_msg_t msg = m;

	if (msg == CRCInput::RC_ignore)
	{
		delete [] (unsigned char*) data;
		return messages_return::handled;
	}

	res = res | g_RemoteControl->handleMsg(msg, data);
	res = res | g_InfoViewer->handleMsg(msg, data);
	res = res | channelList->handleMsg(msg, data);

	if (res != messages_return::unhandled)
	{
		if (msg >= CRCInput::RC_WithData && msg < CRCInput::RC_WithData+ 0x10000000)
			/* BEWARE! the data behind the pointer must have been allocated with
			   "new char[...]", otherwise we'll leak memory.
			   Only debuggable with valgrind */
			delete [] (unsigned char *)data;

		return res & (0xFFFFFFFF - messages_return::unhandled);
	}

	if (msg <= CRCInput::RC_MaxRC)
	{
		if (msg & CRCInput::RC_Repeat)
		{
			repeat = true;
			msg &= ~CRCInput::RC_Repeat;
		}
	}

	if (!waitforshutdown) {
		if (msg == CRCInput::RC_ok &&
		    g_settings.standby_off_with == STANDBY_OFF_WITH_POWER_OK &&
		    mode == mode_standby &&
	        data == 0)
		{
			g_RCInput->postMsg( NeutrinoMessages::STANDBY_OFF, 0 );
		}
		else if (msg == CRCInput::RC_home &&
		         g_settings.standby_off_with == STANDBY_OFF_WITH_POWER_HOME &&
		         mode == mode_standby &&
		         data == 0)
		{
			g_RCInput->postMsg( NeutrinoMessages::STANDBY_OFF, 0 );
		}
		else if ((msg == CRCInput::RC_home || msg == CRCInput::RC_ok) &&
		         g_settings.standby_off_with == STANDBY_OFF_WITH_POWER_HOME_OK &&
		         mode == mode_standby &&
		         data == 0)
		{
			g_RCInput->postMsg( NeutrinoMessages::STANDBY_OFF, 0 );
		}
		else if (msg == CRCInput::RC_standby)
		{
			if (data == 0)
			{
				neutrino_msg_t new_msg;

				/* Note: pressing the power button on the dbox (not the remote control) over 1 second */
				/*       shuts down the system even if !g_settings.shutdown_real_rcdelay (see below)  */
				gettimeofday(&standby_pressed_at, NULL);

				if ((mode != mode_standby) && (g_settings.shutdown_real))
				{
					new_msg = NeutrinoMessages::SHUTDOWN;
				}
				else
				{
					new_msg = (mode == mode_standby) ? NeutrinoMessages::STANDBY_OFF : NeutrinoMessages::STANDBY_ON;

					if ((g_settings.shutdown_real_rcdelay))
					{
						neutrino_msg_t      tmpmsg;
						neutrino_msg_data_t tmpdata;
						struct timeval      endtime;
						time_t              seconds;

						int timeout = g_settings.repeat_blocker;
						int timeout1 = g_settings.repeat_genericblocker;

						if (timeout1 > timeout)
							timeout = timeout1;

						timeout += 500;

						while(true)
						{
							g_RCInput->getMsg_ms(&tmpmsg, &tmpdata, timeout);
							/* if the power key gets released, then get out of here */
							if (tmpmsg == (CRCInput::RC_standby | CRCInput::RC_Release))
								break;

							gettimeofday(&endtime, NULL);
							seconds = endtime.tv_sec - standby_pressed_at.tv_sec;
							if (endtime.tv_usec < standby_pressed_at.tv_usec)
								seconds--;
							if (seconds >= 1)
							{
								new_msg = NeutrinoMessages::SHUTDOWN;
								break;
							}
						}
					}
				}
				g_RCInput->postMsg(new_msg, 0);
				return messages_return::cancel_all | messages_return::handled;
			}
			else                                        /* data == 1: KEY_POWER released                         */
			if (standby_pressed_at.tv_sec != 0) /* check if we received a KEY_POWER pressed event before */
			{                                   /* required for correct handling of KEY_POWER events of  */
				                            /* the power button on the dbox (not the remote control) */
				struct timeval endtime;
				gettimeofday(&endtime, NULL);
				time_t seconds = endtime.tv_sec - standby_pressed_at.tv_sec;
				if (endtime.tv_usec < standby_pressed_at.tv_usec)
					seconds--;
				if (seconds >= 1)
				{
					g_RCInput->postMsg(NeutrinoMessages::SHUTDOWN, 0);
					return messages_return::cancel_all | messages_return::handled;
				}
			}
		}

		if (msg == CRCInput::RC_spkr && !repeat)
		{
			if (mode == mode_standby) //switch lcd off/on
				CLCD::getInstance()->togglePower();
			else if (! doShowMuteIcon()) //mute, but only if volume is not already == 0
				AudioMute(!current_muted);
			return messages_return::handled;
		}

		if (mode == mode_standby && msg <= CRCInput::RC_MaxRC)
			return messages_return::handled; // don't process RC events in standby mode
		else if ((msg == g_settings.key_volume_up) ||
			 (msg == g_settings.key_volume_down) ||
			 (msg == NeutrinoMessages::EVT_VOLCHANGED))
		{
			setVolume(msg, (mode != mode_scart));
			return messages_return::handled;
		}
#ifdef HAVE_TRIPLEDRAGON
		else if(msg == CRCInput::RC_zoomin)
		{
			g_Zapit->setZoom(g_Zapit->getZoom() + 2);
			return messages_return::handled;
		}
		else if(msg == CRCInput::RC_zoomout)
		{
			g_Zapit->setZoom(g_Zapit->getZoom() - 2);
			return messages_return::handled;
		}
		else if(msg == CRCInput::RC_zoomoff)
		{
			g_Zapit->setZoom(100);
			return messages_return::handled;
		}
#endif
		/* HACK: mark all key-repeat and key-release events as "handled" in order not to
		   screw up all the menus and hintboxes that do not yet handle them correctly */
		if (msg <= CRCInput::RC_MaxRC)
			if (repeat || (msg & CRCInput::RC_Release))
				return messages_return::handled;
			else
				return messages_return::unhandled;
		/* no remotecontrol/keyboard events below here... */

		if (msg == NeutrinoMessages::EVT_VCRCHANGED)
		{
			if (g_settings.vcr_AutoSwitch)
			{
				if (data != VCR_STATUS_OFF)
				{
					/* clear remote control queue to avoid changing to soft standby
					   when a device connected to the scart socket is turned on several
					   seconds after Neutrino started */
					g_RCInput->clearRCMsg();

					g_RCInput->postMsg(NeutrinoMessages::VCR_ON, 0);
				}
				else
					g_RCInput->postMsg(NeutrinoMessages::VCR_OFF, 0);
			}
			return messages_return::handled | messages_return::cancel_info;
		}
		else if( msg == NeutrinoMessages::EVT_MUTECHANGED )
		{
			CControldMsg::commandMute* cmd = (CControldMsg::commandMute*) data;
			if(cmd->type == (CControld::volume_type)g_settings.audio_avs_Control)
				AudioMute( cmd->mute, true );
			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::EVT_RECORDMODE )
		{
			dprintf(DEBUG_DEBUG, "neutrino - recordmode %s\n", ( data ) ? "on":"off" );

			recordingstatus = data;

			if( ( !g_InfoViewer->is_visible ) && data )
				g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

			static t_channel_id old_parent_id;
			t_channel_id old_id = g_Zapit->getCurrentServiceID();
			if (data)
			{
				old_parent_id = channelList->getActiveChannel_ChannelID();
				// if record on - get channelList from zapit
				//			channelsInit(init_mode_init);
				channelsInit4Record();
			}
			else
			{
				// if record off - switch channelList to old mode
				if (g_Zapit->isChannelTVChannel(old_parent_id))
					channelsInit(init_mode_switch, mode_tv);
				else 
					channelsInit(init_mode_switch, mode_radio);
			}

			// if a neutrino channel for current channel_id cannot be found (eg tuned to a sub service)
			// adjust to old main channel
			if (!channelList->adjustToChannelID(old_id) && !data)
				channelList->adjustToChannelID(old_parent_id);

			if(old_id == 0)
				channelList->zapTo(0);
			/* if we came from standby to recording => back into standby */
			if (!data &&standbyAfterRecord)
				standbyMode(true);
		}
		else if( msg == NeutrinoMessages::EVT_BOUQUETSCHANGED )			// EVT_BOUQUETSCHANGED: initiated by zapit ; EVT_SERVICESCHANGED: no longer used
		{
			t_channel_id old_id = channelList->getActiveChannel_ChannelID();

			channelsInit(init_mode_init);

			if((old_id == 0) || (!(channelList->adjustToChannelID(old_id))))
				channelList->zapTo(0);

			return messages_return::handled;
		}
		else if (msg == NeutrinoMessages::RECORD_START)
		{
			if (g_settings.standby_save_power && mode == mode_standby)
			{
				standbyAfterRecord = true;
				standbyMode(false);
			}
			puts("[neutrino.cpp] executing " NEUTRINO_RECORDING_START_SCRIPT ".");
			if (my_system(NEUTRINO_RECORDING_START_SCRIPT) != 0)
				perror(NEUTRINO_RECORDING_START_SCRIPT " failed");

			/* set nextRecordingInfo to current event (replace other scheduled recording if available) */

			/*
			 * Note: CTimerd::RecordingInfo is a class!
			 * What a brilliant idea to send classes via the eventserver!
			 * => typecast to avoid destructor call
			 */
			if (nextRecordingInfo != NULL)
				delete [](unsigned char *)nextRecordingInfo;

			nextRecordingInfo = (CTimerd::RecordingInfo *) data;

			startNextRecording();

			return messages_return::handled | messages_return::cancel_all;
		}
		else if( msg == NeutrinoMessages::RECORD_STOP)
		{
			if(((CTimerd::RecordingStopInfo*)data)->eventID==recording_id)
			{ // passendes stop zur Aufnahme
				CVCRControl * vcr_control = CVCRControl::getInstance();
				if (vcr_control->isDeviceRegistered())
				{
					if ((vcr_control->getDeviceState() == CVCRControl::CMD_VCR_RECORD) ||
					    (vcr_control->getDeviceState() == CVCRControl::CMD_VCR_PAUSE ))
					{
						vcr_control->Stop();
						recordingstatus = 0;
					}
					else
						printf("falscher state\n");
				}
				else
					puts("[neutrino.cpp] no recording devices");

				startNextRecording();

				if (recordingstatus == 0)
				{
					puts("[neutrino.cpp] executing " NEUTRINO_RECORDING_END_SCRIPT ".");
					if (my_system(NEUTRINO_RECORDING_END_SCRIPT) != 0)
						perror(NEUTRINO_RECORDING_END_SCRIPT " failed");
				}
			}
			else if(nextRecordingInfo!=NULL)
			{
				if(((CTimerd::RecordingStopInfo*)data)->eventID == nextRecordingInfo->eventID)
				{

					/*
					 * Note: CTimerd::RecordingInfo is a class!
					 * What a brilliant idea to send classes via the eventserver!
					 * => typecast to avoid destructor call
					 */
					delete [](unsigned char *)nextRecordingInfo;

					nextRecordingInfo=NULL;
				}
			}
			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if (msg == NeutrinoMessages::EVT_PMT_CHANGED)
		{
			res = messages_return::handled;
#if 0 // debug
			printf("NeutrinoMessages::EVT_PMT_CHANGED: vpid %x apids(%d): ", g_RemoteControl->current_PIDs.PIDs.vpid, g_RemoteControl->current_PIDs.APIDs.size());
			for(unsigned int i = 0; i < g_RemoteControl->current_PIDs.APIDs.size(); i++)
				printf("%x ", g_RemoteControl->current_PIDs.APIDs[i].pid);
			printf("\n");
#endif
			/******************
				reserved
			 ******************/
			return res;
		}
		else if( msg == NeutrinoMessages::ZAPTO)
		{
			CTimerd::EventInfo * eventinfo;
			eventinfo = (CTimerd::EventInfo *) data;
			if(recordingstatus==0)
			{
				bool isTVMode = g_Zapit->isChannelTVChannel(eventinfo->channel_id);

				if ((!isTVMode) && (mode != mode_radio))
				{
					radioMode(false);
					channelsInit(init_mode_switch, mode_radio);
				}
				else if (isTVMode && (mode != mode_tv))
				{
					tvMode(false);
					channelsInit(init_mode_switch, mode_tv);
				}
				channelList->zapTo_ChannelID(eventinfo->channel_id);
				if (eventinfo->epgID == 1) {
					g_Timerd->resetAdZap_EventID();
				}
			}
			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::ANNOUNCE_ZAPTO)
		{
			CTimerd::RecordingInfo * eventinfo = (CTimerd::RecordingInfo *) data;

			puts("[neutrino.cpp] executing " NEUTRINO_ZAPTO_TIMER_SCRIPT ".");
			if (my_system(NEUTRINO_ZAPTO_TIMER_SCRIPT) != 0)
				perror(NEUTRINO_ZAPTO_TIMER_SCRIPT " failed");

			if( mode == mode_standby )
			{
				// WAKEUP
				standbyMode( false );
			}
			if( mode != mode_scart )
			{
				std::string name = g_Locale->getText(LOCALE_ZAPTOTIMER_ANNOUNCE);
				name += "\n";
				std::string zAddData = "";

				if (g_Zapit->getMode() != CZapitClient::MODE_STANDBY)
					zAddData = g_Zapit->getChannelName(eventinfo->channel_id); // UTF-8
				if (zAddData.empty())
					zAddData = g_Locale->getText(LOCALE_TIMERLIST_PROGRAM_UNKNOWN);

				if (eventinfo->epgID != 0)
				{
					CShortEPGData epgdata;
#warning fixme sectionsd should deliver data in UTF-8 format
					zAddData += " :\n";
					if (g_Sectionsd->getEPGidShort(eventinfo->epgID, &epgdata))
						zAddData += Latin1_to_UTF8(epgdata.title);
					else if (strlen(eventinfo->epgTitle) != 0)
						zAddData += Latin1_to_UTF8(eventinfo->epgTitle);
				}
				else if (strlen(eventinfo->epgTitle) != 0)
				{
					zAddData += " :\n";
					zAddData += Latin1_to_UTF8(eventinfo->epgTitle);
				}

				name += zAddData;
				ShowHintUTF(LOCALE_MESSAGEBOX_INFO, name.c_str());
			}
			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::ANNOUNCE_RECORD)
		{
			CTimerd::RecordingInfo * eventinfo = (CTimerd::RecordingInfo *) data;
			if (g_settings.standby_save_power && mode == mode_standby)
			{
				standbyAfterRecord = true;
				standbyMode(false);
			}
			puts("[neutrino.cpp] executing " NEUTRINO_RECORDING_TIMER_SCRIPT ".");
			if (my_system(NEUTRINO_RECORDING_TIMER_SCRIPT) != 0)
				perror(NEUTRINO_RECORDING_TIMER_SCRIPT " failed");

			if( g_settings.recording_server_wakeup )
			{
				printf("[neutrino] waking up %s (rec-server)\n", g_settings.recording_server_mac.c_str());
				if (my_system(2, "ether-wake", g_settings.recording_server_mac.c_str()) != 0)
					perror("ether-wake failed");
			}
			if (g_settings.recording_type == RECORDING_FILE)
			{
				char * recDir = eventinfo->recordingDir;
				for (int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
				{
					if (g_settings.network_nfs[i].local_dir == recDir)
					{
						printf("[neutrino] waking up %s (%s)\n", g_settings.network_nfs[i].ip.c_str(), recDir);
						if (my_system(2, "ether-wake", g_settings.network_nfs[i].mac.c_str()) != 0)
							perror("ether-wake failed");
						break;
					}
				}
			}
			if( g_settings.recording_zap_on_announce )
			{
				if(recordingstatus==0)
				{
					t_channel_id channel_id = eventinfo->channel_id;
					if(g_Zapit->getCurrentServiceID() != channel_id)
						g_Zapit->zapTo_serviceID_NOWAIT(channel_id);
				}
			}
			if( mode != mode_scart )
			{
				std::string name = g_Locale->getText(LOCALE_RECORDTIMER_ANNOUNCE);
				name += "\n";
				std::string zAddData = "";

				if (g_Zapit->getMode() != CZapitClient::MODE_STANDBY)
					zAddData = g_Zapit->getChannelName(eventinfo->channel_id); // UTF-8
				if (zAddData.empty())
					zAddData = g_Locale->getText(LOCALE_TIMERLIST_PROGRAM_UNKNOWN);

				if (eventinfo->epgID != 0)
				{
					CShortEPGData epgdata;
#warning fixme sectionsd should deliver data in UTF-8 format
					zAddData += " :\n";
					if (g_Sectionsd->getEPGidShort(eventinfo->epgID, &epgdata))
						zAddData += Latin1_to_UTF8(epgdata.title);
					else if (strlen(eventinfo->epgTitle) != 0)
						zAddData += Latin1_to_UTF8(eventinfo->epgTitle);
				}
				else if (strlen(eventinfo->epgTitle) != 0)
				{
					zAddData += " :\n";
					zAddData += Latin1_to_UTF8(eventinfo->epgTitle);
				}

				name += zAddData;
				ShowHintUTF(LOCALE_MESSAGEBOX_INFO, name.c_str());
			}
			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::ANNOUNCE_SLEEPTIMER)
		{
			if( mode != mode_scart )
				if (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_SLEEPTIMERBOX_ANNOUNCE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, 450, 30, true) == CMessageBox::mbrYes)
					g_Timerd->stopTimerEvent(g_Timerd->getSleeptimerID());
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::SLEEPTIMER)
		{
#ifdef ENABLE_LIRC
			CIRSend irs("sleep");
			irs.Send();
#endif

			if(g_settings.shutdown_real)
				ExitRun(true);
			else
				standbyMode( true );
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::STANDBY_TOGGLE )
		{
			standbyMode( !(mode & mode_standby) );
			g_RCInput->clearRCMsg();
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::STANDBY_ON )
		{
			if (g_settings.standby_save_power && recordingstatus != 0)
			{
				DisplayErrorMessage("Nach Ende der Aufnahme wird in Standby geschaltet...");
				standbyAfterRecord = true;
				return messages_return::handled;
			}
			if( mode != mode_standby )
			{
				// noch nicht im Standby-Mode...
				standbyMode( true );
			}
			g_RCInput->clearRCMsg();
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::STANDBY_OFF )
		{
			if( mode == mode_standby )
			{
				// WAKEUP
				standbyMode( false );
			}
			g_RCInput->clearRCMsg();
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::ANNOUNCE_SHUTDOWN)
		{
			if( mode != mode_scart )
				skipShutdownTimer = (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_SHUTDOWNTIMER_ANNOUNCE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, 450, 30, true) == CMessageBox::mbrYes);
		}
		else if( msg == NeutrinoMessages::SHUTDOWN )
		{
			// AUSSCHALTEN...
			if(!skipShutdownTimer)
			{
				ExitRun(true);
			}
			else
			{
				skipShutdownTimer=false;
			}
			return messages_return::handled;
		}
#ifdef ENABLE_ESD
		else if( msg == NeutrinoMessages::ESOUND_ON )
		{
			CEsoundGui tmpEsoundGui;
			tmpEsoundGui.exec(NULL, "");
			g_RCInput->clearRCMsg();
			return messages_return::handled;
		}
#endif
		else if (msg == NeutrinoMessages::EVT_POPUP)
		{
			if (mode != mode_scart)
				ShowHintUTF(LOCALE_MESSAGEBOX_INFO, (const char *) data); // UTF-8
			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if (msg == NeutrinoMessages::EVT_EXTMSG)
		{
			if (mode != mode_scart)
				ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, (const char *) data, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO); // UTF-8
			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if (msg == NeutrinoMessages::EVT_RECORDING_ENDED)
		{
			if (mode != mode_scart)
			{
				neutrino_locale_t msgbody;

				if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_IDLE)
					msgbody = LOCALE_STREAMING_SUCCESS;
				else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_BUFFER_OVERFLOW)
					msgbody = LOCALE_STREAMING_BUFFER_OVERFLOW;
				else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_WRITE_OPEN_FAILURE)
					msgbody = LOCALE_STREAMING_WRITE_ERROR_OPEN;
				else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_WRITE_FAILURE)
					msgbody = LOCALE_STREAMING_WRITE_ERROR;
				else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_RECORDING_THREADS_FAILED)
					msgbody = LOCALE_STREAMING_OUT_OF_MEMORY;
				else
					goto skip_message;

				/*
				 * use a short timeout of only 5 seconds in case it was only a temporary network problem
				 * in case of STREAM2FILE_STATUS_IDLE we might even have to immediately start the next recording
				 */
#warning TODO: it might make some sense to have some log-file (but where do we store this information? nfs/flash/ram?) that collects these messages and maybe a menu-entry to view the lasted XXX messages
				ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, msgbody, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO, 450, 5);

			skip_message:
				;
			}
			if (((* (stream2file_status2_t *) data).status != STREAM2FILE_STATUS_IDLE) && ((* (stream2file_status2_t *) data).status != STREAM2FILE_STATUS_RECORDING_THREADS_FAILED))
			{
#warning TODO: count restart-rate to catch endless loops
				/*
				 * note that changeNotify does not distinguish between LOCALE_MAINMENU_RECORDING_START and LOCALE_MAINMENU_RECORDING_STOP
				 * instead it checks the state of the variable recordingstatus
				 */
				/* restart recording */
				doGuiRecord((*(stream2file_status2_t *) data).dir, false, (*(stream2file_status2_t *) data).filename);
				//changeNotify(LOCALE_MAINMENU_RECORDING_START, data);
			}

			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::REMIND)
		{
			std::string text = Latin1_to_UTF8((char*)data);
			std::string::size_type pos;
			while((pos=text.find('/'))!= std::string::npos)
			{
				text[pos] = '\n';
			}
			if( mode != mode_scart )
				ShowMsgUTF(LOCALE_TIMERLIST_TYPE_REMIND, text, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO); // UTF-8
			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if (msg == NeutrinoMessages::LOCK_RC)
		{
			CRCLock rcLock;
			rcLock.exec(NULL, CRCLock::NO_USER_INPUT);
			return messages_return::handled;
		}
		else if( msg == NeutrinoMessages::CHANGEMODE )
		{
#ifdef ENABLE_RADIOTEXT
			if((data & mode_mask) != mode_radio)
			{
				if (g_Radiotext)
				{
					delete g_Radiotext;
					g_Radiotext = NULL;
				}
			}
#endif
			if((data & mode_mask)== mode_radio)
			{
				if( mode != mode_radio )
				{
					if((data & norezap)==norezap)
						radioMode(false);
					else
						radioMode(true);
#ifdef ENABLE_RADIOTEXT
					if (g_settings.radiotext_enable)
						g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
#endif
				}
			}
			if((data & mode_mask)== mode_tv)
			{
				if( mode != mode_tv )
					if((data & norezap)==norezap)
						tvMode(false);
					else
						tvMode(true);
			}
			if((data &mode_mask)== mode_standby)
			{
				if(mode != mode_standby)
					standbyMode( true );
			}
			if((data &mode_mask)== mode_audio)
			{
				lastMode=mode;
				mode=mode_audio;
			}
			if((data &mode_mask)== mode_pic)
			{
				lastMode=mode;
				mode=mode_pic;
			}
			if((data &mode_mask)== mode_ts)
			{
				lastMode=mode;
				mode=mode_ts;
			}
		}
		else if( msg == NeutrinoMessages::VCR_ON )
		{
			if( mode != mode_scart )
			{
				// noch nicht im Scart-Mode...
				if (mode == mode_standby)				
					wakeupfromScart = true;
				scartMode( true );
			}
			else // sonst nur lcd akt.
				CLCD::getInstance()->setMode(CLCD::MODE_SCART);
		}

		else if( msg == NeutrinoMessages::VCR_OFF )
		{
			if( mode == mode_scart )
			{
				// noch nicht im Scart-Mode...
				scartMode( false );
			}
		}
		else if (msg == NeutrinoMessages::RELOAD_PLUGINS)
		{
			g_PluginList->loadPlugins();
			bool show_games = g_PluginList->hasPlugin(CPlugins::P_TYPE_GAME);
			bool show_scripts = g_PluginList->hasPlugin(CPlugins::P_TYPE_SCRIPT);
			if (show_games != menuGamesIsVisible || show_scripts != menuScriptsIsVisible)
			{
				menuGamesIsVisible = show_games;
				menuScriptsIsVisible = show_scripts;
				CPersonalizeGui::getInstance()->addPersonalizedItems();
			}
			return messages_return::handled;
		}
		else if (msg == NeutrinoMessages::EVT_START_PLUGIN)
		{
			g_PluginList->startPlugin((const char *)data);
			std::string output = g_PluginList->getScriptOutput();
			if (!g_PluginList->getScriptOutput().empty())
			{
				ShowMsgUTF(LOCALE_PLUGINS_RESULT, Latin1_to_UTF8(g_PluginList->getScriptOutput()),
					   CMessageBox::mbrBack,CMessageBox::mbBack,NEUTRINO_ICON_SHELL);
			}

			delete [] (unsigned char*) data;
			return messages_return::handled;
		}
		else if (msg == NeutrinoMessages::EVT_SERVICES_UPD)
		{
			CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO,
							  g_Locale->getText(LOCALE_SERVICEMENU_RELOAD_HINT));
			hintBox->paint();
			g_Zapit->reloadCurrentServices();
			hintBox->hide();
			delete hintBox;
		}
	}
	else
	{
		if (msg == NeutrinoMessages::EVT_SI_FINISHED)
		{
			waitforshutdown = false;
			ExitRun(false);
		}
	}
	if ((msg >= CRCInput::RC_WithData) && (msg < CRCInput::RC_WithData + 0x10000000))
		delete [] (unsigned char*) data;

	return messages_return::unhandled;
}

void CNeutrinoApp::ExitRun(const bool write_si)
{
	  //DisplayErrorMessage(g_Locale->getText(LOCALE_SHUTDOWNERROR_RECODING));
	if ((!recordingstatus ||
	    ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_SHUTDOWN_RECODING_QUERY, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, 450, 30, true) == CMessageBox::mbrYes) && (!waitforshutdown))
	{

		if (write_si) {

			CLCD::getInstance()->setMode(CLCD::MODE_SHUTDOWN);

			dprintf(DEBUG_INFO, "exit\n");
			for(int x=0;x<256;x++)
				frameBuffer->paletteSetColor(x, 0x000000, 0xffff);
			frameBuffer->paletteSet();
/*
			frameBuffer->loadPicture2FrameBuffer("shutdown.raw");
			frameBuffer->loadPal("shutdown.pal");
*/
			saveSetup();

			if (!g_settings.epg_dir.empty()) {
				waitforshutdown = true;
				AudioMute(true);
				g_Sectionsd->setPauseScanning(true);
				g_Sectionsd->writeSI2XML(g_settings.epg_dir.c_str());
			}
			else {
				g_Timerd->shutdown();
				if (g_RCInput != NULL)
					delete g_RCInput;

				if (frameBuffer != NULL)
					delete frameBuffer;
#ifdef ENABLE_LIRC
				CIRSend irs("neutrinooff");
				irs.Send();
#endif

				exit(0);

			}
		} else {
			AudioMute(false);
			g_Timerd->shutdown();
			if (g_RCInput != NULL)
				delete g_RCInput;

			if (frameBuffer != NULL)
				delete frameBuffer;
#ifdef ENABLE_LIRC
			CIRSend irs("neutrinooff");
			irs.Send();
#endif

			exit(0);

		}
	}
}

bool CNeutrinoApp::doShowMuteIcon()
{
	char current_volume;
	if (g_settings.show_mute_icon != SHOW_MUTE_ICON_NO)
	{
		current_volume = g_Controld->getVolume((CControld::volume_type)g_settings.audio_avs_Control);
		if (current_volume == 0)	// show mute icon if volume = 0
		{
			if (g_settings.show_mute_icon == SHOW_MUTE_ICON_YES)
				return true;
			else if (g_settings.show_mute_icon == SHOW_MUTE_ICON_NOT_IN_AC3MODE &&
			         g_settings.audio_DolbyDigital == false)
			{
				// show mute icon if volume = 0 in dependence of enabled or disabled AC3-Mode
				return true;
			}
		}
	}
	return false;
}

void CNeutrinoApp::paintMuteIcon( bool is_visible )
{
	//fprintf(stderr, "%s: %d\n", __PRETTY_FUNCTION__, is_visible);
	frameBuffer->paintMuteIcon(is_visible);
}

void CNeutrinoApp::AudioMute(bool newValue, bool onlyIcon)
{
#ifdef ENABLE_LIRC
	if((CControld::volume_type)g_settings.audio_avs_Control==CControld::TYPE_LIRC) // lirc
	{ // bei LIRC wissen wir nicht wikrlich ob jetzt ge oder entmuted wird, deswegen nix zeigen---
		if (!onlyIcon)
			g_Controld->Mute((CControld::volume_type)g_settings.audio_avs_Control);
	}
	else
#endif
	{
		CLCD::getInstance()->setMuted(newValue);
		if( newValue != current_muted )
		{
			current_muted = newValue;

			if (!onlyIcon)
			{
				if( current_muted )
					g_Controld->Mute((CControld::volume_type)g_settings.audio_avs_Control);
				else
					g_Controld->UnMute((CControld::volume_type)g_settings.audio_avs_Control);
			}

			if (mode != mode_scart && mode != mode_pic)
			{
				// show mute icon ONLY on event or current volume value is 0
				if (current_muted || doShowMuteIcon())
					paintMuteIcon();
				else
					paintMuteIcon(false);
			}
		}
	}
}

void CNeutrinoApp::setVolume(const neutrino_msg_t key, const bool bDoPaint)
{
	neutrino_msg_t msg = key;
	neutrino_msg_t msg_repeatok = key & ~CRCInput::RC_Repeat;

#ifdef ENABLE_LIRC
	const bool lirc = (CControld::volume_type)g_settings.audio_avs_Control==CControld::TYPE_LIRC;
#else
	const bool lirc = false;
#endif
	const int fheight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
	const int dy = std::max(28, fheight); 	// height
	// if you want a non-rounded volumebar, set r=0 here...
	const int r = RADIUS_LARGE;	// radius
	const int b = 4;	// border
	const int w = 200;	// volume bar width
	const int nw = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth("100")+6;// 40;	// numbers width
	const int iw = 24;	// icon width
	const int dx = w + b*2 + iw + nw + r; 	// width + 2*border + icon + numbers + round
	const int bwx = 20;	// border width x from left and right
	const int bwtop = 20; 	// border width y from top
	const int bwbot = 20; 	// border width y from bottom
	int x, y;
	int a_step = g_settings.audio_step;
	volumeBarIsVisible = ((g_settings.volumebar_disp_pos != VOLUMEBAR_DISP_POS_OFF) ? true : false);
	
	if( g_settings.volumebar_disp_pos == VOLUMEBAR_DISP_POS_TOP_RIGHT )
	{
		x = g_settings.screen_EndX - dx - bwx;
		y = g_settings.screen_StartY + bwtop;
	}
	else if( g_settings.volumebar_disp_pos == VOLUMEBAR_DISP_POS_TOP_LEFT )
	{
		x = g_settings.screen_StartX + bwx;
		y = g_settings.screen_StartY + bwtop;
	}
	else if( g_settings.volumebar_disp_pos == VOLUMEBAR_DISP_POS_BOTTOM_LEFT )
	{
		x = g_settings.screen_StartX + bwx;
		y = g_settings.screen_EndY - dy - bwbot;
	}
	else if( g_settings.volumebar_disp_pos == VOLUMEBAR_DISP_POS_BOTTOM_RIGHT )
	{
		x = g_settings.screen_EndX - dx - bwx;
		y = g_settings.screen_EndY - dy - bwbot;
	}
	else if( g_settings.volumebar_disp_pos == VOLUMEBAR_DISP_POS_HIGHER_CENTER )
	{
		x = (((g_settings.screen_EndX- g_settings.screen_StartX)- dx) / 2) + g_settings.screen_StartX;
		y = g_settings.screen_EndY - dy - bwbot - 140;
	}
	else /* if (g_settings.volumebar_disp_pos == VOLUMEBAR_DISP_POS_DEFAULT_CENTER) */
	{
		x = (((g_settings.screen_EndX- g_settings.screen_StartX)- dx) / 2) + g_settings.screen_StartX;
		y = g_settings.screen_EndY - dy - bwbot;
	}

	fb_pixel_t * pixbuf = NULL;

	if (bDoPaint && volumeBarIsVisible && !lirc) // not visible if lirc in use
	{
		pixbuf = new fb_pixel_t[dx * dy];
		if(pixbuf!= NULL)
			frameBuffer->SaveScreen(x, y, dx, dy, pixbuf);
		frameBuffer->paintBoxRel(           x,   y,  dx,     dy, COL_INFOBAR_PLUS_1, r);
		frameBuffer->paintBoxRel(x+iw+r/2-b/2, y+b, w+b, dy-2*b, COL_SILVER);
		int icon_w = 0, icon_h = 0;
		frameBuffer->getIconSize(NEUTRINO_ICON_VOLUME, &icon_w, &icon_h);	
		frameBuffer->paintIcon(NEUTRINO_ICON_VOLUME, x+b+r/2, y+(dy/2)-(icon_h/2), COL_INFOBAR);
	}

	neutrino_msg_data_t data;

	unsigned long long timeoutEnd;

	char current_volume = g_Controld->getVolume((CControld::volume_type)g_settings.audio_avs_Control);
	char last_volume = 101; // make sure the volume bar is shown initially

	do
	{
		if (msg <= CRCInput::RC_MaxRC)
		{
			if (msg_repeatok == g_settings.key_volume_up)
			{
#ifdef ENABLE_LIRC
				if (lirc)
				{
					current_volume = 60; //>50 is plus
				}
				else
#endif
				{
					if (current_volume < 100 - a_step)
						current_volume += a_step;
					else
						current_volume = 100;
				}
			}
			else if (msg_repeatok == g_settings.key_volume_down)
			{
#ifdef ENABLE_LIRC
				if (lirc)
				{
					current_volume = 40; //<40 is minus
				}
				else
#endif
				{
					if (current_volume > a_step)
						current_volume -= a_step;
					else
						current_volume = 0;
				}
			}
			else if (msg != (g_settings.key_volume_down|CRCInput::RC_Release) &&	// ignore release of all the keys
				 msg != (g_settings.key_volume_up|CRCInput::RC_Release) &&	// that have triggered setVolume()
				 msg != (CRCInput::RC_ok|CRCInput::RC_Release))		// "OK" triggers in ost/lirc/avs settings
			{
				g_RCInput->postMsg(msg, data);
				break;
			}

			if (!(msg & CRCInput::RC_Release)) // no need to set on RC_minus release...
				g_Controld->setVolume(current_volume, (CControld::volume_type)g_settings.audio_avs_Control);

			/* the doShowMuteIcon code checks for the real volume,
			   so we must do this after g_Controld->setVolume() */
			if (current_muted && msg_repeatok == g_settings.key_volume_up)
				AudioMute(false); // switch off mute on pressing the plus button
#ifdef ENABLE_LIRC
			if (lirc)
			{
				current_volume = 50;
			}
#endif
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_VOLUMEBAR]);
		}
		else if (msg == NeutrinoMessages::EVT_VOLCHANGED)
		{
			current_volume = g_Controld->getVolume((CControld::volume_type)g_settings.audio_avs_Control);
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_VOLUMEBAR]);
		}
		else if (handleMsg(msg, data) & messages_return::unhandled)
		{
			g_RCInput->postMsg(msg, data);
			break;
		}

		if (bDoPaint && volumeBarIsVisible && current_volume != last_volume && !lirc) //not visible if lirc in use
		{
			last_volume = current_volume;
			int vol = current_volume * 2;
			char p[4]; /* 3 digits + '\0' */
			sprintf(p, "%3d", current_volume);
			/* draw the volume bar */
			frameBuffer->paintBoxRel(x+iw+r/2,     y+b+2, vol  , dy-2*(b+2), COL_INFOBAR_PLUS_3);
			frameBuffer->paintBoxRel(x+iw+vol+r/2, y+b+2, w-vol, dy-2*(b+2), COL_INFOBAR_PLUS_1);
			/* erase the numbers... */
			frameBuffer->paintBoxRel(x+dx-nw-b-r/2, y+b,     nw, dy-2*b,     COL_INFOBAR_PLUS_1);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(x+dx-nw-r/2, y+(dy>>1)+(fheight>>1)+1, nw, p, COL_INFOBAR_PLUS_1);
			if (mode != mode_scart && mode != mode_pic && doShowMuteIcon())
				AudioMute(true, true);
		}
						
		CLCD::getInstance()->showVolume(current_volume);
		if (msg != CRCInput::RC_timeout)
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
			msg_repeatok = msg & ~CRCInput::RC_Repeat;
		}

	}
	while (msg != CRCInput::RC_timeout);

	if (bDoPaint && volumeBarIsVisible && !lirc && (pixbuf != NULL))
	{
		frameBuffer->RestoreScreen(x, y, dx, dy, pixbuf);
		delete [] pixbuf;
	}
}

void CNeutrinoApp::tvMode( bool rezap )
{
	if(mode==mode_radio )
	{
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_TV_MODE, false );

#ifdef ENABLE_RADIOTEXT
		if (g_settings.radiotext_enable && g_Radiotext != NULL)
		{
			delete g_Radiotext;
			g_Radiotext = NULL;
		}
#endif

#ifdef HAVE_DBOX_HARDWARE
		if(g_settings.misc_spts==1)
			g_Zapit->PlaybackSPTS();
#endif
	}

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	if( mode == mode_tv )
	{
		return;
	}
	else if( mode == mode_scart )
	{
		g_Controld->setScartMode( 0 );
	}
	else if( mode == mode_standby )
	{
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);
	}
	
	mode = mode_tv;
	tunerMode = mode_tv;

	if (g_Controld->getVideoFormat() != g_settings.video_Format)
		g_Controld->setVideoFormat(g_settings.video_Format);

	//printf( "tv-mode\n" );

	frameBuffer->useBackground(false);
	frameBuffer->paintBackground();

	g_RemoteControl->tvMode();
	if( rezap )
	{
		firstChannel();
		channelsInit(init_mode_switch, mode_tv);
		channelList->zapTo( firstchannel.channelNumber -1 );
		zapto_tv_on_init_done = true;
	}
}

void CNeutrinoApp::scartMode( bool bOnOff )
{
	//printf( ( bOnOff ) ? "mode: scart on\n" : "mode: scart off\n" );

	if( bOnOff )
	{
		// SCART AN
		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();

		g_Controld->setScartMode( 1 );
		CLCD::getInstance()->setMode(CLCD::MODE_SCART);

		lastMode = mode;
		mode = mode_scart;
	}
	else
	{
		// SCART AUS
		g_Controld->setScartMode( 0 );

		mode = mode_unknown;

		//re-set mode
		if (wakeupfromScart)
		{
			standbyMode(true);
			wakeupfromScart = false;
		}
		else if (tunerMode == mode_radio)
			radioMode(!zapto_radio_on_init_done);
		else
			tvMode(!zapto_tv_on_init_done);
	}
}

void CNeutrinoApp::standbyMode( bool bOnOff )
{
	//printf( ( bOnOff ) ? "mode: standby on\n" : "mode: standby off\n" );

	if( bOnOff )
	{
		// STANDBY AN

		if( mode == mode_scart )
		{
			g_Controld->setScartMode( 0 );
		}

#ifdef ENABLE_RADIOTEXT
		if (mode == mode_radio && g_settings.radiotext_enable && g_Radiotext != NULL)
		{
			delete g_Radiotext;
			g_Radiotext = NULL;
		}
#endif
		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();

		CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
		g_Controld->videoPowerDown(true);

		puts("[neutrino.cpp] executing " NEUTRINO_ENTER_STANDBY_SCRIPT ".");
		if (my_system(NEUTRINO_ENTER_STANDBY_SCRIPT) != 0)
			perror(NEUTRINO_ENTER_STANDBY_SCRIPT " failed");

		if (g_settings.standby_save_power)
		{
			g_Zapit->setStandby(true);
			g_Sectionsd->setPauseScanning(true);
			CLCD::getInstance()->setEPGTitle("");
		}
#ifdef HAVE_TRIPLEDRAGON
		/* this is ugly, and it would belong into zapit.
		   IOC_AVS_STANDBY_ENTER switches off the frontpanel LEDs and
		   it disables the power supply of the frontend, so this needs
		   to come after zapit->standby() and needs to be disabled
		   before zapit is woken up again */
#define IOC_AVS_STANDBY_ENTER _IOW(0x73, 98, unsigned int)
		int fd = open("/dev/stb/tdsystem", O_RDONLY);
		ioctl(fd, IOC_AVS_STANDBY_ENTER);
		close(fd);
#endif

		standbyAfterRecord = false; // reset
		wakeupfromScart = false; //reset
		lastMode = mode;
		mode = mode_standby;

#ifdef ENABLE_LIRC
		//Send ir
		CIRSend irs("sbon");
		irs.Send();
#endif
	}
	else
	{
		// STANDBY AUS
#ifdef HAVE_TRIPLEDRAGON
#define IOC_AVS_STANDBY_LEAVE _IOW(0x73, 99, unsigned int)
		int fd = open("/dev/stb/tdsystem", O_RDONLY);
		ioctl(fd, IOC_AVS_STANDBY_LEAVE);
		close(fd);
#endif

		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		if (g_settings.standby_save_power)
		{
			g_Zapit->setStandby(false);
			g_Sectionsd->setPauseScanning(false);
		}
		g_Controld->videoPowerDown(false);

		puts("[neutrino.cpp] executing " NEUTRINO_LEAVE_STANDBY_SCRIPT ".");
		if (my_system(NEUTRINO_LEAVE_STANDBY_SCRIPT) != 0)
			perror(NEUTRINO_LEAVE_STANDBY_SCRIPT " failed");

#ifdef ENABLE_LIRC
		if (!standbyAfterRecord)
		// execute sboff only if standby was left due to start of timer recording
		{	//Send ir
			CIRSend irs("sboff");
			irs.Send();
		}
#endif

		mode = mode_unknown;

		//re-set mode
		if (tunerMode == mode_radio)
			radioMode(!zapto_radio_on_init_done);
		else
			tvMode(!zapto_tv_on_init_done);
		
		//show mute icon ONLY if muted or current volume value is 0
		if (current_muted || doShowMuteIcon())
			paintMuteIcon();

		/* hack. TODO: why is this needed? */
		g_Sectionsd->setServiceChanged(g_RemoteControl->current_channel_id, false);
	}
}

void CNeutrinoApp::radioMode( bool rezap)
{
	if(mode==mode_tv )
	{
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_RADIO_MODE, false );

#ifdef HAVE_DBOX_HARDWARE
		if(g_settings.misc_spts==1)
			g_Zapit->PlaybackPES();
#endif
	}

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	if( mode==mode_radio )
	{
		return;
	}
	else if( mode == mode_scart )
	{
		g_Controld->setScartMode( 0 );
	}
	else if( mode == mode_standby )
	{
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);
	}
	
	mode = mode_radio;
	tunerMode = mode_radio;

	system("miniops showlogo radio");

	if (g_Controld->getVideoFormat() != g_settings.video_backgroundFormat)
		g_Controld->setVideoFormat(g_settings.video_backgroundFormat);
/*
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->useBackground(frameBuffer->loadBackground(NEUTRINO_ICON_RADIOMODE));// set useBackground true or false
	frameBuffer->paintBackground();
*/
	g_RemoteControl->radioMode();
	if( rezap )
	{
		firstChannel();
		channelsInit(init_mode_switch, mode_radio);
		channelList->zapTo( firstchannel.channelNumber -1 );
		zapto_radio_on_init_done = true;
	}
#ifdef ENABLE_RADIOTEXT
	if (g_settings.radiotext_enable && g_Radiotext == NULL)
	{
		g_Radiotext = new CRadioText;
	}
#endif
}

void CNeutrinoApp::startNextRecording()
{
	if ((recordingstatus   == 0   ) &&
	    (nextRecordingInfo != NULL))
	{
		bool doRecord = true;
		if (CVCRControl::getInstance()->isDeviceRegistered())
		{
			recording_id = nextRecordingInfo->eventID;
			if (g_settings.recording_type == RECORDING_FILE)
			{
				const char *recDir = strlen(nextRecordingInfo->recordingDir) > 0 ?
					nextRecordingInfo->recordingDir : g_settings.recording_dir[0].c_str();

				int free = getFreeDiscSpaceGB(recDir);
				printf("[neutrino.cpp] getFreeDiscSpaceGB %d\n",free);
				if (free < 2)
				{
					int dirid = getFirstFreeRecDirNr(2);
					if (dirid != -1)
						recDir = g_settings.recording_dir[dirid].c_str();
					printf("[neutrino.cpp] getFirstFreeRecDirNr %d\n",dirid);
				}
#ifdef ENABLE_GUI_MOUNT
				if (!CFSMounter::isMounted(recDir))
				{
					printf("[neutrino.cpp] trying to mount %s\n",recDir);
					for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
					{
						if (g_settings.network_nfs[i].local_dir == recDir)
						{
							CFSMounter::MountRes mres =
								CFSMounter::mount(g_settings.network_nfs[i].ip, g_settings.network_nfs[i].dir,
										  g_settings.network_nfs[i].local_dir, (CFSMounter::FSType) g_settings.network_nfs[i].type,
										  g_settings.network_nfs[i].username, g_settings.network_nfs[i].password,
										  g_settings.network_nfs[i].mount_options1, g_settings.network_nfs[i].mount_options2);
							if (mres == CFSMounter::MRES_OK)
							{
								printf("[neutrino.cpp] mount successful\n");
							} else {
								std::string msg = mntRes2Str(mres) + "\nDir: " + nextRecordingInfo->recordingDir;
								ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, msg.c_str()); // UTF-8
								doRecord = false;
							}
							break;
						}
					}
					if (!doRecord)
					{
						// an error occured while mounting recording dir
						// -> try default dir
						recDir = g_settings.recording_dir[0].c_str();
						doRecord = true;
					}
				}
#endif
				if (doRecord)
				{
					printf("[neutrino.cpp] recording to %s\n",recDir);
					CVCRControl::CFileDevice *fileDevice;
					if ((fileDevice = dynamic_cast<CVCRControl::CFileDevice*>(recordingdevice)) != NULL)
					{
						fileDevice->Directory = recDir;

						fileDevice->FilenameTemplate = g_settings.recording_filename_template_default;
						for (int i = 0; i < MAX_RECORDING_DIR; i++)
						{
							if (!g_settings.recording_filename_template[i].empty() && strcmp(g_settings.recording_dir[i].c_str(), recDir) == 0)
							{
								fileDevice->FilenameTemplate = g_settings.recording_filename_template[i];
								break;
							}
						}

						unsigned int splitsize;
						sscanf(g_settings.recording_splitsize_default, "%u", &splitsize);
						for (int i = 0; i < MAX_RECORDING_DIR; i++)
						{
							if (strlen(g_settings.recording_splitsize[i]) > 0 && strcmp(g_settings.recording_dir[i].c_str(), recDir) == 0)
							{
								sscanf(g_settings.recording_splitsize[i], "%u", &splitsize);
								break;
							}
						}
						fileDevice->SplitSize = splitsize;
					} else
					{
						puts("[neutrino] could not set directory and filename template");
					}
				}
			}
			if(doRecord && CVCRControl::getInstance()->Record(nextRecordingInfo))
				recordingstatus = 1;
			else
				recordingstatus = 0;
		}
		else
			puts("[neutrino.cpp] no recording devices");

		/*
		 * Note: CTimerd::RecordingInfo is a class!
		 * What a brilliant idea to send classes via the eventserver!
		 * => typecast to avoid destructor call
		 */
		delete [](unsigned char *)nextRecordingInfo;

		nextRecordingInfo = NULL;
	}
}
/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  exec, menuitem callback (shutdown)                         *
*                                                                                     *
**************************************************************************************/
int CNeutrinoApp::exec(CMenuTarget* /*parent*/, const std::string & actionKey)
{
	//	printf("ac: %s\n", actionKey.c_str());
	int returnval = menu_return::RETURN_NONE;

	if(actionKey=="shutdown")
	{
		ExitRun(true);
	}
	else if(actionKey=="reboot")
	{
		FILE *f = fopen("/tmp/.reboot", "w");
		fclose(f);
		ExitRun(true);
	}
	else if(actionKey=="tv")
	{
		tvMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="radio")
	{
		radioMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="scart")
	{
		g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="savesettings")
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MAINSETTINGS_SAVESETTINGSNOW_HINT)); // UTF-8
		hintBox->paint();

 		g_Controld->saveSettings();

 		saveSetup();

		/* send motor position list to zapit */
		if (scanSettings.diseqcMode == DISEQC_1_2)
		{
			printf("[neutrino] sending motor positions list to zapit...\n");
			CZapitClient::ScanMotorPosList motorPosList;
			CNeutrinoApp::getInstance()->getScanSettings().toMotorPosList(motorPosList);
			g_Zapit->setScanMotorPosList(motorPosList);
		}
		/* set unicable, is only used if diseqcMode == DISEQC_UNICABLE */
		g_Zapit->setUnicableParam(scanSettings.uni_scr, scanSettings.uni_qrg);
		// Houdini set DiseqcType/Repeat so you don't have to reboot for changes to take effect
		/* send diseqc type to zapit */
		g_Zapit->setDiseqcType(CNeutrinoApp::getInstance()->getScanSettings().diseqcMode);
		/* send diseqc repeat to zapit */
		g_Zapit->setDiseqcRepeat(CNeutrinoApp::getInstance()->getScanSettings().diseqcRepeat);
		g_Zapit->saveSettings();

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="resetpassword")
	{
		if (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_MAINSETTINGS_RESETPASSWORD, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo) == CMessageBox::mbrYes)
		{
			resetPassword();
			ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_MAINSETTINGS_RESETPASSWORD_HINT, 420, g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR], NEUTRINO_ICON_INFO);
		}
	}
	else if(actionKey=="reloadchannels")
	{
	 	CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_RELOAD_HINT));
		hintBox->paint();

		g_Zapit->reinitChannels();

		int result = system(mode == mode_radio
				    ? "wget -t 1 -T 5 -q -O /dev/null http://127.0.0.1/control/setmode?radio > /dev/null 2>&1"
				    : "wget -t 1 -T 5 -q -O /dev/null http://127.0.0.1/control/setmode?tv > /dev/null 2>&1");
		if (result)
			perror("Kicking nhttpd failed");

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="reloadplugins")
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_GETPLUGINS_HINT));
		hintBox->paint();

		g_PluginList->loadPlugins();
		bool show_games = g_PluginList->hasPlugin(CPlugins::P_TYPE_GAME);
		bool show_scripts = g_PluginList->hasPlugin(CPlugins::P_TYPE_SCRIPT);
		if (show_games != menuGamesIsVisible || show_scripts != menuScriptsIsVisible)
		{
			menuGamesIsVisible = show_games;
			menuScriptsIsVisible = show_scripts;
			CPersonalizeGui::getInstance()->addPersonalizedItems();
		}

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="restart")
	{
		if (recordingstatus)
			DisplayErrorMessage(g_Locale->getText(LOCALE_SERVICEMENU_RESTART_REFUSED_RECORDING));
		else {
	 		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_RESTART_HINT));
			hintBox->paint();
			execvp(global_argv[0], global_argv); // no return if successful
			DisplayErrorMessage(g_Locale->getText(LOCALE_SERVICEMENU_RESTART_FAILED));

			hintBox->hide();
			delete hintBox;
		}
	}
	else if(actionKey == "clearSectionsd")
	{
		g_Sectionsd->freeMemory();
		returnval = menu_return::RETURN_REPAINT; // just to show that something was done
	}
	else if(actionKey == "EPGrestart")
	{
		g_Sectionsd->Restart();
		g_Sectionsd->RegisterNeutrino();
		g_Sectionsd->setPauseScanning(false);
		g_Sectionsd->setServiceChanged(g_RemoteControl->current_channel_id, false);
		returnval = menu_return::RETURN_REPAINT; // just to show that something was done
	}
#ifdef ENABLE_MOVIEPLAYER2
	else if (actionKey.find("mb.file://") == 0)
	{
		CMenuTarget* mp = new CMoviePlayerGui();
		returnval = mp->exec(NULL, actionKey.substr(3));
		delete mp;
	}
#endif

	return returnval;
}

/**************************************************************************************
*                                                                                     *
*          changeNotify - features menu recording start / stop                        *
*                                                                                     *
**************************************************************************************/
bool CNeutrinoApp::changeNotify(const neutrino_locale_t OptionName, void * Data)
{
	if ((ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_RECORDING_START)) || (ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_RECORDING)))
	{
		return doGuiRecord(NULL,ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_RECORDING));
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_PAUSESECTIONSD))
	{
		g_Sectionsd->setPauseScanning((*((int *)Data)) == 0);
	}
	return false;
}

/**************************************************************************************
*                                                                                     *
*          Main programm - no function here                                           *
*                                                                                     *
**************************************************************************************/
int main(int argc, char **argv)
{
	setDebugLevel(DEBUG_NORMAL);

	tzset();
	initGlobals();

	char * buf = (char*)malloc(64);
	int count;
	if (buf && (count = readlink("/proc/self/exe", buf, 63)) >= 0) {
		buf[count] = '\0';
		printf("starting %s\n", buf);
		free(buf);
	}
	for (int i = 3; i < 256; i++)
		close(i);

	return CNeutrinoApp::getInstance()->run(argc, argv);
}
