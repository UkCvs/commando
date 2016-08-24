/* 
  $Id: settings.h,v 1.247 2012/07/22 06:24:46 rhabarber1848 Exp $
 
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

#ifndef __settings__
#define __settings__

#include <driver/rcinput.h>
#include <system/localize.h>
#include <configfile.h>
#include <zapit/client/zapitclient.h>

#include <string>

struct SNeutrinoSettings
{
	//video
	int video_Format;
	int video_backgroundFormat;
	unsigned char video_csync;

	//misc
	int standby_save_power;
	int shutdown_real;
	int shutdown_real_rcdelay;
	int standby_off_with;
	char shutdown_count[4];
	int sleeptimer_min;
	int volumebar_disp_pos;
	int infobar_sat_display;
	int infobar_subchan_disp_pos;
	int misc_spts;
#ifndef TUXTXT_CFG_STANDALONE
	int tuxtxt_cache;
#endif
	int virtual_zap_mode;
	int progressbar_color;
	int infobar_show;
	int show_mute_icon;
	int bigFonts;
	int channellist_additional;
	int channellist_epgtext_align_right;
	int channellist_extended;
	int channellist_foot;
	char infobar_channel_logodir[100];
	int infobar_show_channellogo;
	int infobar_channellogo_background;
	int startmode;
	int wzap_time;
	int radiotext_enable;

	// EPG
	std::string epg_cache;
	std::string epg_extendedcache;
	std::string epg_old_events;
	std::string epg_max_events;
	std::string epg_dir;
#ifdef ENABLE_FREESATEPG
	int epg_freesat_enabled;
#endif
	// network
	std::string network_ntpserver;
	std::string network_ntprefresh;
	int network_ntpenable;
#ifdef ENABLE_SAMBASERVER
	// samba
	int smb_setup_samba_on_off;
	std::string smb_setup_samba_workgroup;
#endif

	//audio
	int audio_AnalogMode;
	int audio_DolbyDigital;
	int audio_avs_Control;
	int audio_initial_volume;
	int audio_step;
	int audio_PCMOffset;
	int audio_ReSync;
	int audio_ReSync_timer;

	//Audio Priority Pids
#define AUDIO_PRIORITY_NR_OF_ENTRIES 8
	char audio_propids_name[AUDIO_PRIORITY_NR_OF_ENTRIES][8];
	int audio_propids_enabled;

	//vcr
	int vcr_AutoSwitch;

	//language
	char language[25];
	char timezone[150];

	//timing
#define TIMING_SETTING_COUNT 10
	enum TIMING_SETTINGS {
		TIMING_MENU        = 0,
		TIMING_CHANLIST    = 1,
		TIMING_EPG         = 2,
		TIMING_INFOBAR     = 3,
		TIMING_INFOBAR_RADIO = 4,
		TIMING_INFOBAR_MOVIE = 5,
		TIMING_VOLUMEBAR   = 6, 
		TIMING_FILEBROWSER = 7,
		TIMING_NUMERICZAP  = 8,
		TIMING_ZAPHISTORY  = 9
	};

	int  timing       [TIMING_SETTING_COUNT]   ;
	char timing_string[TIMING_SETTING_COUNT][4];

	//widget settings
	int widget_fade;

	//colors
	unsigned char gtx_alpha1;
	unsigned char gtx_alpha2;

	unsigned char menu_Head_alpha;
	unsigned char menu_Head_red;
	unsigned char menu_Head_green;
	unsigned char menu_Head_blue;

	unsigned char menu_Head_Text_alpha;
	unsigned char menu_Head_Text_red;
	unsigned char menu_Head_Text_green;
	unsigned char menu_Head_Text_blue;

	unsigned char menu_Content_alpha;
	unsigned char menu_Content_red;
	unsigned char menu_Content_green;
	unsigned char menu_Content_blue;

	unsigned char menu_Content_Text_alpha;
	unsigned char menu_Content_Text_red;
	unsigned char menu_Content_Text_green;
	unsigned char menu_Content_Text_blue;

	unsigned char menu_Content_Selected_alpha;
	unsigned char menu_Content_Selected_red;
	unsigned char menu_Content_Selected_green;
	unsigned char menu_Content_Selected_blue;

	unsigned char menu_Content_Selected_Text_alpha;
	unsigned char menu_Content_Selected_Text_red;
	unsigned char menu_Content_Selected_Text_green;
	unsigned char menu_Content_Selected_Text_blue;

	unsigned char menu_Content_inactive_alpha;
	unsigned char menu_Content_inactive_red;
	unsigned char menu_Content_inactive_green;
	unsigned char menu_Content_inactive_blue;

	unsigned char menu_Content_inactive_Text_alpha;
	unsigned char menu_Content_inactive_Text_red;
	unsigned char menu_Content_inactive_Text_green;
	unsigned char menu_Content_inactive_Text_blue;

	unsigned char infobar_alpha;
	unsigned char infobar_red;
	unsigned char infobar_green;
	unsigned char infobar_blue;

	unsigned char infobar_Text_alpha;
	unsigned char infobar_Text_red;
	unsigned char infobar_Text_green;
	unsigned char infobar_Text_blue;
		
	//corners
	int rounded_corners;

	//menu numbers
	int	menu_numbers_as_icons;

	//network
#define NETWORK_NFS_NR_OF_ENTRIES 8
struct {
	std::string ip;
	std::string mac;
	std::string local_dir;
	std::string dir;
	int  automount;
	std::string mount_options1;
	std::string mount_options2;
	int  type;
	std::string username;
	std::string password;
} network_nfs[NETWORK_NFS_NR_OF_ENTRIES];

	//personalization
	int personalize_pinstatus;
	int personalize_bluebutton;
	int personalize_redbutton;
	char personalize_pincode[5];

	int personalize_tvmode;
	int personalize_radiomode;
	int personalize_scartmode;
	int personalize_games;
	int personalize_audioplayer;
	int personalize_inetradio;
	int personalize_esound;
	int personalize_movieplayer;
	int personalize_pictureviewer;
	int personalize_upnpbrowser;

	int personalize_scripts;
	int personalize_settings;
	int personalize_service;
	int personalize_sleeptimer;
	int personalize_reboot;
	int personalize_shutdown;

	int personalize_bouqueteditor;
	int personalize_scants;
	int personalize_reload;
	int personalize_getplugins;
	int personalize_restart;
	int personalize_epgrestart;
	int personalize_ucodecheck;
	int personalize_imageinfo;
	int personalize_update;
	int personalize_chan_epg_stat;
#ifdef ENABLE_DRIVE_GUI
	int personalize_drive_setup_stat;
#endif

	int personalize_audio;
	int personalize_video;
	int personalize_youth;
	int personalize_network;
	int personalize_recording;
	int personalize_keybinding;
	int personalize_colors;
	int personalize_lcd;
	int personalize_mediaplayer;
	int personalize_driver;
	int personalize_misc;
	int personalize_true;

	//recording
	int  recording_type;
	int  recording_stopplayback;
	int  recording_stopsectionsd;
	std::string recording_server_ip;
	char recording_server_port[10];
	int  recording_server_wakeup;
	std::string recording_server_mac;
	int  recording_vcr_no_scart;
	int  recording_max_rectime;
	char recording_splitsize_default[10];
	int  recording_use_o_sync;
	int  recording_use_fdatasync;
	unsigned char recording_audio_pids_default;
	int  recording_stream_vtxt_pid;
	int  recording_stream_subtitle_pid;
	int recording_ringbuffers;
	int recording_in_spts_mode;
	int recording_choose_direct_rec_dir;
	std::string recording_filename_template_default;
	char recording_dir_permissions[4];
	int  recording_zap_on_announce;
#define MAX_RECORDING_DIR 10	
	std::string recording_dir[MAX_RECORDING_DIR];
	std::string recording_filename_template[MAX_RECORDING_DIR];
	char recording_splitsize[MAX_RECORDING_DIR][10];
	int recording_gen_psi;
	int recording_nhd_compatible_ts;

	//streaming
	int  streaming_type;
	std::string streaming_server_ip;
	char streaming_server_port[10];
	char streaming_server_cddrive[21];
	char streaming_videorate[6];
	char streaming_audiorate[6];
	char streaming_server_startdir[40];
	int streaming_transcode_audio;
	int streaming_force_avi_rawaudio;
	int streaming_force_transcode_video;
	int streaming_transcode_video_codec;
	int streaming_resolution;
	int streaming_vlc10;
	int streaming_use_buffer;
	int streaming_buffer_segment_size;
	int streaming_stopsectionsd;
	int streaming_show_tv_in_browser;
	int streaming_allow_multiselect;
	int streaming_use_reclength;
	std::string streaming_moviedir;

	int filesystem_is_utf8;
	// default plugin for ts-movieplayer (red button)
	std::string movieplayer_plugin;

	//key configuration
	neutrino_msg_t key_tvradio_mode;

	neutrino_msg_t key_channelList_pageup;
	neutrino_msg_t key_channelList_pagedown;
	neutrino_msg_t key_channelList_cancel;
	neutrino_msg_t key_channelList_sort;
	neutrino_msg_t key_channelList_search;
	neutrino_msg_t key_channelList_addrecord;
	neutrino_msg_t key_channelList_addremind;
	neutrino_msg_t key_channelList_reload;

	neutrino_msg_t key_quickzap_up;
	neutrino_msg_t key_quickzap_down;
	neutrino_msg_t key_volume_up;
	neutrino_msg_t key_volume_down;
	neutrino_msg_t key_bouquet_up;
	neutrino_msg_t key_bouquet_down;
	neutrino_msg_t key_subchannel_up;
	neutrino_msg_t key_subchannel_down;
	neutrino_msg_t key_subchannel_toggle;
	neutrino_msg_t key_zaphistory;
	neutrino_msg_t key_lastchannel;

	neutrino_msg_t key_menu_pageup;
	neutrino_msg_t key_menu_pagedown;

	int repeat_blocker;
	int repeat_genericblocker;
	int audiochannel_up_down_enable;
	int audio_left_right_selectable;

	//screen configuration
	int screen_xres;
	int screen_yres;
	int screen_StartX;
	int screen_StartY;
	int screen_EndX;
	int screen_EndY;

#ifndef DISABLE_INTERNET_UPDATE
	//Software-update
	int softupdate_mode;
	std::string softupdate_url_file;
	std::string softupdate_proxyserver;
	std::string softupdate_proxyusername;
	std::string softupdate_proxypassword;
#endif

	//BouquetHandling
	int bouquetlist_mode;

	// parentallock
	int parentallock_prompt;
	int parentallock_lockage;
	char parentallock_pincode[5];


	// Font sizes
	enum FONT_TYPES {
		FONT_TYPE_MENU =  0,
		FONT_TYPE_MENU_TITLE,
		FONT_TYPE_MENU_INFO,
		FONT_TYPE_EPG_TITLE,
		FONT_TYPE_EPG_INFO1,
		FONT_TYPE_EPG_INFO2,
		FONT_TYPE_EPG_DATE,
		FONT_TYPE_EVENTLIST_TITLE,
		FONT_TYPE_EVENTLIST_ITEMLARGE,
		FONT_TYPE_EVENTLIST_ITEMSMALL,
		FONT_TYPE_EVENTLIST_DATETIME,
		FONT_TYPE_GAMELIST_ITEMLARGE,
		FONT_TYPE_GAMELIST_ITEMSMALL,
		FONT_TYPE_CHANNELLIST,
		FONT_TYPE_CHANNELLIST_DESCR,
		FONT_TYPE_CHANNELLIST_NUMBER,
		FONT_TYPE_CHANNELLIST_EVENT,
		FONT_TYPE_CHANNEL_NUM_ZAP,
		FONT_TYPE_INFOBAR_NUMBER,
		FONT_TYPE_INFOBAR_CHANNAME,
		FONT_TYPE_INFOBAR_INFO,
		FONT_TYPE_INFOBAR_SMALL,
		FONT_TYPE_FILEBROWSER_ITEM,
		FONT_TYPE_COUNT
	};

	// lcdd
	enum LCD_SETTINGS {
		LCD_BRIGHTNESS         = 0,
		LCD_STANDBY_BRIGHTNESS ,
		LCD_CONTRAST           ,
		LCD_POWER              ,
		LCD_INVERSE            ,
		LCD_SHOW_VOLUME        ,
		LCD_AUTODIMM           ,
		LCD_EPGMODE            ,
		LCD_BIAS               ,
		LCD_EPGALIGN           ,

		LCD_SETTING_COUNT
	};
	int lcd_setting[LCD_SETTING_COUNT];

	char lcd_setting_dim_time[4];
	char lcd_setting_dim_brightness[4];

#define FILESYSTEM_ENCODING_TO_UTF8(a) (g_settings.filesystem_is_utf8 ? (a) : ZapitTools::Latin1_to_UTF8(a).c_str())
#define UTF8_TO_FILESYSTEM_ENCODING(a) (g_settings.filesystem_is_utf8 ? (a) : ZapitTools::UTF8_to_Latin1(a).c_str())
#define FILESYSTEM_ENCODING_TO_UTF8_STRING(a) (g_settings.filesystem_is_utf8 ? (a) : Latin1_to_UTF8(a))

#ifdef HAVE_DBOX_HARDWARE
#if HAVE_DVB_API_VERSION == 1
#ifdef ENABLE_RTC
#define DRIVER_SETTING_FILES_COUNT 9
#else /* ENABLE_RTC */
#define DRIVER_SETTING_FILES_COUNT 8
#endif /* ENABLE_RTC */
#else /* HAVE_DVB_API_VERSION == 1 */
#ifdef ENABLE_RTC
#define DRIVER_SETTING_FILES_COUNT 8
#else /* ENABLE_RTC */
#define DRIVER_SETTING_FILES_COUNT 7
#endif /* ENABLE_RTC */
#endif /* HAVE_DVB_API_VERSION == 1 */
#else /* HAVE_DBOX_HARDWARE */
#define DRIVER_SETTING_FILES_COUNT 1
#endif

// #define MISC_SETTING_SPTS_MODE 0

	// pictureviewer
	char   picviewer_slide_time[3];
	int    picviewer_scaling;
	std::string picviewer_decode_server_ip;
	char   picviewer_decode_server_port[6];
	char   picviewer_picturedir[100];

	//audioplayer
	int   audioplayer_display;
	int   audioplayer_follow;
	char  audioplayer_screensaver[3];
	int   audioplayer_highprio;
	int   audioplayer_select_title_by_name;
	int   audioplayer_repeat_on;
	int   audioplayer_show_playlist;
	int   audioplayer_enable_sc_metadata;
	char  audioplayer_audioplayerdir[100];

	//Esound
	char  esound_port[6];

	//Filebrowser
	int filebrowser_showrights;
	int filebrowser_sortmethod;
	int filebrowser_denydirectoryleave;

	//uboot
	int	uboot_baudrate;
	int	uboot_dbox_duplex;
	int	uboot_dbox_duplex_bak;
	int	uboot_console;
	int	uboot_console_bak;
	int	uboot_lcd_inverse;
	int	uboot_lcd_contrast;
	int	uboot_lcd_bias;

	//osd
	std::string font_file;

	// USERMENU
	typedef enum
	{
		BUTTON_RED = 0,  // Do not change ordering of members, add new item just before BUTTON_MAX!!!
		BUTTON_GREEN = 1,
		BUTTON_YELLOW = 2,
		BUTTON_BLUE = 3,
		BUTTON_MAX   // MUST be always the last in the list
	} USER_BUTTON;

	typedef enum
	{
		ITEM_NONE = 0, // Do not change ordering of members, add new item just before ITEM_MAX!!!
		ITEM_BAR = 1,
		ITEM_EPG_LIST = 2,
		ITEM_EPG_SUPER = 3,
		ITEM_EPG_INFO = 4,
		ITEM_EPG_MISC = 5,
		ITEM_AUDIO_SELECT = 6,
		ITEM_SUBCHANNEL = 7,
		ITEM_PLUGIN = 8,  
		ITEM_VTXT = 9,
		ITEM_RECORD = 10, 
		ITEM_MOVIEPLAYER_TS = 11,
		ITEM_MOVIEPLAYER_MB = 12,
		ITEM_TIMERLIST = 13,
		ITEM_REMOTE = 14,
		ITEM_FAVORITS = 15,
		ITEM_TECHINFO = 16,
		ITEM_MAX   // MUST be always the last in the list
	} USER_ITEM;

	std::string usermenu_text[BUTTON_MAX];
	int usermenu[BUTTON_MAX][ITEM_MAX];  // (USER_ITEM)  [button][position in Menue] = feature item
};

// corners
#define CORNER_RADIUS_LARGE		12
#define CORNER_RADIUS_MID		9
#define CORNER_RADIUS_SMALL		4

// convenience macros
#define RADIUS_LARGE	(g_settings.rounded_corners ? CORNER_RADIUS_LARGE : 0)
#define RADIUS_MID	(g_settings.rounded_corners ? CORNER_RADIUS_MID : 0)
#define RADIUS_SMALL	(g_settings.rounded_corners ? CORNER_RADIUS_SMALL : 0)

// shadow
#define SHADOW_OFFSET			4

/* some default Values */

typedef struct time_settings_t
{
	const int default_timing;
	const neutrino_locale_t name;
} time_settings_struct_t;

const time_settings_struct_t timing_setting[TIMING_SETTING_COUNT] =
{
	{ 60,   LOCALE_TIMING_MENU        },
	{ 60,   LOCALE_TIMING_CHANLIST    },
	{ 240,  LOCALE_TIMING_EPG         },
	{ 6,    LOCALE_TIMING_INFOBAR     },
	{ 10,   LOCALE_TIMING_INFOBAR_RADIO },
	{ 6,    LOCALE_TIMING_INFOBAR_MOVIEPLAYER},
	{ 3,    LOCALE_TIMING_VOLUMEBAR   },
	{ 60,   LOCALE_TIMING_FILEBROWSER },
	{ 3,    LOCALE_TIMING_NUMERICZAP  },
	{ 3,    LOCALE_TIMING_ZAPHISTORY  }
};

// lcdd
#define DEFAULT_LCD_BRIGHTNESS			0xff
#define DEFAULT_LCD_STANDBYBRIGHTNESS		0xaa
#define DEFAULT_LCD_CONTRAST			0x0F
#define DEFAULT_LCD_POWER			0x01
#define DEFAULT_LCD_INVERSE			0x00
#define DEFAULT_LCD_AUTODIMM			0x00
#define DEFAULT_LCD_SHOW_VOLUME			CLCD::STATUSLINE_VOLUME
#define DEFAULT_LCD_EPGMODE			CLCD::EPG_NAME
#define DEFAULT_LCD_BIAS			0x00	/* 0x0 = default for philips rev 2, 0x1 = default for all others */
#define DEFAULT_LCD_EPGALIGN		CLCD::EPGALIGN_LEFT

/* end default values */

struct SglobalInfo
{
	unsigned char     box_Type;
	unsigned char     chip_info;
	unsigned char     avia_chip;
	delivery_system_t delivery_system;
};

const int RECORDING_OFF    = 0;
const int RECORDING_SERVER = 1;
const int RECORDING_VCR    = 2;
const int RECORDING_FILE   = 3;

const int PARENTALLOCK_PROMPT_NEVER          = 0;
const int PARENTALLOCK_PROMPT_ONSTART        = 1;
const int PARENTALLOCK_PROMPT_CHANGETOLOCKED = 2;
const int PARENTALLOCK_PROMPT_ONSIGNAL       = 3;

#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
#define MAX_SATELLITES 100
#else
#define MAX_SATELLITES 64
#endif
class CScanSettings
{
 public:
	CConfigFile               configfile;
	CZapitClient::bouquetMode bouquetMode;
	CZapitClient::scanType scanType;
	diseqc_t                  diseqcMode;
	uint32_t                  diseqcRepeat;
	int			  satCount;
	char                      satNameNoDiseqc[30];
	int                       satDiseqc[MAX_SATELLITES];
	int	                  satMotorPos[MAX_SATELLITES];
	t_satellite_position	  satPosition[MAX_SATELLITES];
	char	                  satName[MAX_SATELLITES][30];
	delivery_system_t         delivery_system;
	int                       uni_qrg;
	int                       uni_scr;

	int		scanSectionsd;

	int motorRotationSpeed;
	int useGotoXX;
	double gotoXXLatitude;
	double gotoXXLongitude;
	int gotoXXLaDirection;
	int gotoXXLoDirection;

	int		scan_mode;
	int		TP_scan;
	int		TP_fec;
	int		TP_pol;
	int		TP_mod;
	char		TP_freq[10];
	char		TP_rate[9];
	char		TP_satname[30];
	int		TP_diseqc;

	CScanSettings();

	int* diseqscOfSat( char* satname);
	int* motorPosOfSat( char* satname);
	char* satOfDiseqc(int diseqc) const;
	char* satOfMotorPos(int32_t motorPos) const;
	void toSatList( CZapitClient::ScanSatelliteList& ) const;
	void toMotorPosList( CZapitClient::ScanMotorPosList& ) const;

	bool loadSettings(const char * const fileName, const delivery_system_t dsys);
	bool saveSettings(const char * const fileName);
};


#endif
