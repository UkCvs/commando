/*
	$Id: osd_setup.cpp,v 1.22 2012/09/23 08:18:03 rhabarber1848 Exp $

	osd_setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

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


#include "osd_setup.h"
#include "alphasetup.h"
#include "screensetup.h"
#include "osdlang_setup.h"
#include "filebrowser.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/colorchooser.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>

#include <driver/framebuffer.h>
#include <driver/screen_max.h>

#include <xmltree/xmlinterface.h>
#include <system/debug.h>
#include <system/helper.h>
#include <system/httptool.h>

#define NEUTRINO_PICONS_REMOVE_SCRIPT CONFIGDIR "/picons.remove"

const SNeutrinoSettings::FONT_TYPES channellist_font_sizes[5] =
{
	SNeutrinoSettings::FONT_TYPE_CHANNELLIST,
	SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR,
	SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER,
	SNeutrinoSettings::FONT_TYPE_CHANNELLIST_EVENT,
	SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP
};

const SNeutrinoSettings::FONT_TYPES eventlist_font_sizes[4] =
{
	SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE,
	SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE,
	SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL,
	SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME,
};

const SNeutrinoSettings::FONT_TYPES infobar_font_sizes[4] =
{
	SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER,
	SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME,
	SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO,
	SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL
};

const SNeutrinoSettings::FONT_TYPES epg_font_sizes[4] =
{
	SNeutrinoSettings::FONT_TYPE_EPG_TITLE,
	SNeutrinoSettings::FONT_TYPE_EPG_INFO1,
	SNeutrinoSettings::FONT_TYPE_EPG_INFO2,
	SNeutrinoSettings::FONT_TYPE_EPG_DATE
};

const SNeutrinoSettings::FONT_TYPES gamelist_font_sizes[2] =
{
	SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMLARGE,
	SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMSMALL
};

const SNeutrinoSettings::FONT_TYPES other_font_sizes[4] =
{
	SNeutrinoSettings::FONT_TYPE_MENU,
	SNeutrinoSettings::FONT_TYPE_MENU_TITLE,
	SNeutrinoSettings::FONT_TYPE_MENU_INFO,
	SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM
};

font_sizes_groups font_sizes_groups[6] =
{
	{LOCALE_FONTMENU_CHANNELLIST, 5, channellist_font_sizes, "fontsize.dcha"},
	{LOCALE_FONTMENU_EVENTLIST  , 4, eventlist_font_sizes  , "fontsize.deve"},
	{LOCALE_FONTMENU_EPG        , 4, epg_font_sizes        , "fontsize.depg"},
	{LOCALE_FONTMENU_INFOBAR    , 4, infobar_font_sizes    , "fontsize.dinf"},
	{LOCALE_FONTMENU_GAMELIST   , 2, gamelist_font_sizes   , "fontsize.dgam"},
	{NONEXISTANT_LOCALE         , 4, other_font_sizes      , "fontsize.doth"}
};

COsdSetup::COsdSetup(const neutrino_locale_t title, const char * const IconName)
{
#ifdef HAVE_DBOX_HARDWARE
	if (g_info.box_Type == CControld::TUXBOX_MAKER_NOKIA)
		CFrameBuffer::getInstance()->setBlendLevel(g_settings.gtx_alpha1, g_settings.gtx_alpha2);
#endif

	colorSetupNotifier = new CColorSetupNotifier();
	colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);

	fontsizenotifier = new CFontSizeNotifier;

	osd_setup = NULL;

	menue_title = title;
	menue_icon = IconName;

	width = w_max (500, 100);
	selected = -1;
}

COsdSetup::~COsdSetup()
{
	delete colorSetupNotifier;
	delete fontsizenotifier;
}

int COsdSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init osd setup\n");

	if(parent != NULL)
		parent->hide();

	if (actionKey=="show_menue_color_setup")
	{
		int res = showOsdMenueColorSetup();
		return res;
	}
	else if (actionKey=="show_infobar_color_setup")
	{
		int res = showOsdInfobarColorSetup();
		return res;
	}
	else if (actionKey=="show_timeout_setup")
	{
		int res = showOsdTimeoutSetup();
		return res;
	}
	else if (actionKey=="show_infobar_setup")
	{
		int res = showOsdInfobarSetup();
		return res;
	}
	else if (actionKey=="show_channellist_setup")
	{
		int res = showOsdChannelListSetup();
		return res;
	}
	else if (actionKey == "show_language_setup")
	{
		COsdLangSetup osd_lang(menue_title);
		int res = osd_lang.exec(NULL, "");
		return res;
	}
	else if (actionKey=="select_font")
	{
		CFile file;
		file.Name = g_settings.font_file;
		std::string font_Dir = file.getPath();
		CFileBrowser fileBrowser;
		CFileFilter fileFilter;
		fileFilter.addFilter("ttf");
		fileBrowser.Filter = &fileFilter;
		if (fileBrowser.exec(font_Dir.c_str()))
		{
			g_settings.font_file = fileBrowser.getSelectedFile()->Name;
			printf("[neutrino] new font file %s\n", g_settings.font_file.c_str());
			CNeutrinoApp::getInstance()->SetupFonts();
			font_file_name = fileBrowser.getSelectedFile()->getFileName();
		}
		return menu_return::RETURN_REPAINT;
	}
	else if (actionKey=="show_fontsize_setup")
	{
		int res = showOsdFontSizeSetup();
		return res;
	}
	else if (actionKey=="show_timezone_setup")
	{
		showOsdTimeZoneSetup();
		return menu_return::RETURN_REPAINT;
	}
	else if(strncmp(actionKey.c_str(), "fontsize.d", 10) == 0)
	{
		for (int i = 0; i < 6; i++)
		{
			if (actionKey == font_sizes_groups[i].actionkey)
				for (unsigned int j = 0; j < font_sizes_groups[i].count; j++)
				{
					SNeutrinoSettings::FONT_TYPES k = font_sizes_groups[i].content[j];
					CNeutrinoApp::getInstance()->getConfigFile()->setInt32(locale_real_names[neutrino_font[k].name], neutrino_font[k].defaultsize);
				}
		}

		fontsizenotifier->changeNotify(NONEXISTANT_LOCALE, NULL);

		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "channel_logodir")
	{
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.infobar_channel_logodir))
			strncpy(g_settings.infobar_channel_logodir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.infobar_channel_logodir)-1);
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey=="osd.def")
	{
		for (int i = 0; i < TIMING_SETTING_COUNT; i++)
			g_settings.timing[i] = timing_setting[i].default_timing;

		CNeutrinoApp::getInstance()->SetupTiming();
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey=="update_logo")
	{
		CConfigFile config('\t');
		config.loadConfig("/.version");
		const std::string updateURL = config.getString("update",  "http://www.ukcvs.net/C16/");

		CHTTPTool httpTool;
		if (httpTool.downloadFile(updateURL + "icons.zip", "/tmp/icons.zip", 100))
		{
			CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_RESTART_HINT));
			hintBox->paint();
			CNeutrinoApp::getInstance()->exec(NULL, "restart");
			DisplayErrorMessage(g_Locale->getText(LOCALE_SERVICEMENU_RESTART_FAILED));
			hintBox->hide();
			delete hintBox;
		}
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey=="uninstall_logo")
	{
		if (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_OSDSETTINGS_INFOBAR_CHANNELLOGO_UNINSTALL,
			CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_QUESTION, 450,
			g_settings.timing[SNeutrinoSettings::TIMING_MENU], true) == CMessageBox::mbrYes) {
			puts("[osd_setup.cpp] executing " NEUTRINO_PICONS_REMOVE_SCRIPT ".");
			if (my_system(NEUTRINO_PICONS_REMOVE_SCRIPT) != 0)
				perror(NEUTRINO_PICONS_REMOVE_SCRIPT " failed");
		}
		return menu_return::RETURN_REPAINT;
	}
	int res = showOsdSetup();
	
	return res;
}

/* for color settings menu */
#define COLORMENU_CORNERSETTINGS_TYPE_OPTION_COUNT 2
const CMenuOptionChooser::keyval COLORMENU_CORNERSETTINGS_TYPE_OPTIONS[COLORMENU_CORNERSETTINGS_TYPE_OPTION_COUNT] =
{
	{ 0, LOCALE_OSDSETTINGS_ROUNDED_CORNERS_OFF },
	{ 1, LOCALE_OSDSETTINGS_ROUNDED_CORNERS_ON  }
};

#define VOLUMEBAR_DISP_POS_OPTIONS_COUNT 7
const CMenuOptionChooser::keyval  VOLUMEBAR_DISP_POS_OPTIONS[VOLUMEBAR_DISP_POS_OPTIONS_COUNT]=
{
	{ CNeutrinoApp::VOLUMEBAR_DISP_POS_TOP_RIGHT     , LOCALE_SETTINGS_POS_TOP_RIGHT      },
	{ CNeutrinoApp::VOLUMEBAR_DISP_POS_TOP_LEFT      , LOCALE_SETTINGS_POS_TOP_LEFT       },
	{ CNeutrinoApp::VOLUMEBAR_DISP_POS_BOTTOM_LEFT   , LOCALE_SETTINGS_POS_BOTTOM_LEFT    },
	{ CNeutrinoApp::VOLUMEBAR_DISP_POS_BOTTOM_RIGHT  , LOCALE_SETTINGS_POS_BOTTOM_RIGHT   },
	{ CNeutrinoApp::VOLUMEBAR_DISP_POS_DEFAULT_CENTER, LOCALE_SETTINGS_POS_DEFAULT_CENTER },
	{ CNeutrinoApp::VOLUMEBAR_DISP_POS_HIGHER_CENTER , LOCALE_SETTINGS_POS_HIGHER_CENTER  },
	{ CNeutrinoApp::VOLUMEBAR_DISP_POS_OFF           , LOCALE_SETTINGS_POS_OFF            }
};

#define SHOW_MUTE_ICON_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval  SHOW_MUTE_ICON_OPTIONS[SHOW_MUTE_ICON_OPTIONS_COUNT]=
{
	{ CNeutrinoApp::SHOW_MUTE_ICON_NO            , LOCALE_MISCSETTINGS_SHOW_MUTE_ICON_NO             },
	{ CNeutrinoApp::SHOW_MUTE_ICON_YES           , LOCALE_MISCSETTINGS_SHOW_MUTE_ICON_YES            },
	{ CNeutrinoApp::SHOW_MUTE_ICON_NOT_IN_AC3MODE, LOCALE_MISCSETTINGS_SHOW_MUTE_ICON_NOT_IN_AC3MODE }
};

//show osd setup
int COsdSetup::showOsdSetup()
{
	//osd main settings
	osd_setup = new CMenuWidget(menue_title, menue_icon, width);
	osd_setup->setPreselected(selected);

	//osd settings color sbubmenue
	CMenuWidget *osd_setup_colors 	= new CMenuWidget(menue_title, menue_icon, width);

	// language
	CMenuForwarder *osd_lang_fw = new CMenuForwarder(LOCALE_MAINSETTINGS_LANGUAGE, true, g_settings.language, this, "show_language_setup", CRCInput::RC_red);
	
	//osd color setup forwarder
	CMenuForwarder *osd_setup_color_sub_fw	= new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_HEAD, true, NULL, osd_setup_colors, NULL, CRCInput::RC_green);
		//osd menue colors forwarder
		CMenuForwarder *osd_menucolor_fw = new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_MENUCOLORS, true, NULL, this, "show_menue_color_setup", CRCInput::RC_red);
		//osd infobar setup forwarder
		CMenuForwarder *osd_sbcolor_fw = new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_STATUSBAR, true, NULL, this, "show_infobar_color_setup", CRCInput::RC_green);
		//osd themes setup forwarder
		CThemes *osd_themes = new CThemes(menue_title);
		CMenuForwarder *osd_themes_fw	= new CMenuForwarder(LOCALE_OSDSETTINGS_THEMESELECT, true, NULL, osd_themes, NULL, CRCInput::RC_yellow);
		

	//osd progressbar color
	CMenuOptionChooser *osd_pbcolor_ch = new CMenuOptionChooser(LOCALE_OSDSETTINGS_COLORMENU_PROGRESSBAR, &g_settings.progressbar_color, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);
	//osd fontsize setup
	CMenuForwarder *osd_fontsize_fw = new CMenuForwarder(LOCALE_FONTMENU_HEAD, true, NULL, this, "show_fontsize_setup", CRCInput::RC_yellow);
	//osd timeout setup forwarder
	CMenuForwarder *osd_timeout_fw = new CMenuForwarder(LOCALE_TIMING_HEAD, true, NULL,  this, "show_timeout_setup", CRCInput::RC_blue);
	//osd screen setup
	CScreenSetup *osd_screen = new CScreenSetup();
	CMenuForwarder *osd_screen_fw = new CMenuForwarder(LOCALE_VIDEOMENU_SCREENSETUP, true, NULL, osd_screen, NULL, CRCInput::RC_1);
	//osd infobar setup
	CMenuForwarder *osd_infobar_fw = new CMenuForwarder(LOCALE_OSDSETTINGS_INFOBAR, true, NULL, this, "show_infobar_setup", CRCInput::RC_2);
	//osd channellist setup
	CMenuForwarder *osd_chanlist_fw = new CMenuForwarder(LOCALE_MISCSETTINGS_CHANNELLIST, true, NULL, this, "show_channellist_setup", CRCInput::RC_3);
	//osd timezone setup
	CMenuForwarder *osd_timezone_fw = new CMenuForwarder(LOCALE_TIMEZONEMENU_HEAD, true, NULL, this, "show_timezone_setup", CRCInput::RC_4);

	//osd volumbar position
 	CMenuOptionChooser* osd_volumbarpos_ch = new CMenuOptionChooser(LOCALE_OSDSETTINGS_VOLUMEBAR_DISP_POS, &g_settings.volumebar_disp_pos, VOLUMEBAR_DISP_POS_OPTIONS, VOLUMEBAR_DISP_POS_OPTIONS_COUNT, true);
	//osd mute icon options
 	CMenuOptionChooser* osd_mute_icon_ch = new CMenuOptionChooser(LOCALE_OSDSETTINGS_SHOW_MUTE_ICON, &g_settings.show_mute_icon, SHOW_MUTE_ICON_OPTIONS, SHOW_MUTE_ICON_OPTIONS_COUNT, true);
	//osd corner chooser
	CMenuOptionChooser* osd_corners_ch = new CMenuOptionChooser(LOCALE_OSDSETTINGS_ROUNDED_CORNERS, &g_settings.rounded_corners, COLORMENU_CORNERSETTINGS_TYPE_OPTIONS, COLORMENU_CORNERSETTINGS_TYPE_OPTION_COUNT, true, this);
	//osd menu numbers
	CMenuOptionChooser* osd_menu_numbers_ch = new CMenuOptionChooser(LOCALE_OSDSETTINGS_MENU_NUMBERS_AS_ICONS, &g_settings.menu_numbers_as_icons, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this);

	//osd main settings, intros
	osd_setup->addIntroItems(menue_title != LOCALE_MAINSETTINGS_OSD ? LOCALE_MAINSETTINGS_OSD : NONEXISTANT_LOCALE);
	//--------------------------------------------
	osd_setup->addItem(osd_lang_fw);	//language setup
	osd_setup->addItem(osd_setup_color_sub_fw);	//color setup
		osd_setup_colors->addIntroItems(LOCALE_OSDSETTINGS_COLORMENU_HEAD); //color setup intros
		osd_setup_colors->addItem(osd_menucolor_fw);	//menue colors
		osd_setup_colors->addItem(osd_sbcolor_fw);	//infobar colors
		osd_setup_colors->addItem(osd_pbcolor_ch);	//progressbar colors
		//--------------------------------------------
		osd_setup_colors->addItem(GenericMenuSeparatorLine);
		osd_setup_colors->addItem(osd_themes_fw);	//themes setup
	osd_setup->addItem(osd_fontsize_fw);	//fontsize setup		
	osd_setup->addItem(osd_timeout_fw);	//timeout
	osd_setup->addItem(osd_screen_fw);	//screen setup
	osd_setup->addItem(GenericMenuSeparatorLine);
	//-------------------------------------------
	osd_setup->addItem(osd_infobar_fw);	//infobar setup
	osd_setup->addItem(osd_chanlist_fw);	//channellist setup
	osd_setup->addItem(osd_timezone_fw);	//timezone setup
#ifdef HAVE_DBOX_HARDWARE
	CAlphaSetup* osd_alpha = NULL;
	if ((g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS) || (g_info.box_Type == CControld::TUXBOX_MAKER_SAGEM))
	{	
		// eNX
		osd_setup->addItem(new CMenuOptionChooser(LOCALE_OSDSETTINGS_COLORMENU_FADE, &g_settings.widget_fade, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true )); //fade

	}
	else 
	{
		//GTX
		osd_alpha = new CAlphaSetup(LOCALE_OSDSETTINGS_COLORMENU_GTX_ALPHA);
		osd_setup->addItem(new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_GTX_ALPHA, true, NULL, osd_alpha, NULL, CRCInput::RC_5));
	}
#else 	
	//Dream and TD
	osd_setup->addItem(new CMenuOptionChooser(LOCALE_OSDSETTINGS_COLORMENU_FADE, &g_settings.widget_fade, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true )); //fade
#endif
	osd_setup->addItem(GenericMenuSeparatorLine);
	//-------------------------------------------
	osd_setup->addItem(osd_corners_ch); //corner style
	osd_setup->addItem(osd_volumbarpos_ch);	//volumebar
	osd_setup->addItem(osd_mute_icon_ch);	//mute icon
	osd_setup->addItem(osd_menu_numbers_ch);	//menu numbers

	int res = osd_setup->exec(NULL, "");
	selected = osd_setup->getSelected();
	delete osd_setup;

	delete osd_setup_colors;
	delete osd_themes;
	delete osd_screen;
#ifdef HAVE_DBOX_HARDWARE
	delete osd_alpha;
#endif

	return res;
}


//show menue colorsetup
int COsdSetup::showOsdMenueColorSetup()
{
	CMenuWidget * ocs = new CMenuWidget(menue_title, menue_icon, width);

	CColorChooser chHeadcolor(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND_HEAD, &g_settings.menu_Head_red, &g_settings.menu_Head_green, &g_settings.menu_Head_blue,  &g_settings.menu_Head_alpha, colorSetupNotifier);
	CColorChooser chHeadTextcolor(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR_HEAD, &g_settings.menu_Head_Text_red, &g_settings.menu_Head_Text_green, &g_settings.menu_Head_Text_blue, NULL, colorSetupNotifier);
	CColorChooser chContentcolor(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND_HEAD, &g_settings.menu_Content_red, &g_settings.menu_Content_green, &g_settings.menu_Content_blue,&g_settings.menu_Content_alpha, colorSetupNotifier);
	CColorChooser chContentTextcolor(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR_HEAD, &g_settings.menu_Content_Text_red, &g_settings.menu_Content_Text_green, &g_settings.menu_Content_Text_blue, NULL, colorSetupNotifier);
	CColorChooser chContentSelectedcolor(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND_HEAD, &g_settings.menu_Content_Selected_red, &g_settings.menu_Content_Selected_green, &g_settings.menu_Content_Selected_blue,&g_settings.menu_Content_Selected_alpha, colorSetupNotifier);
	CColorChooser chContentSelectedTextcolor(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR_HEAD, &g_settings.menu_Content_Selected_Text_red, &g_settings.menu_Content_Selected_Text_green, &g_settings.menu_Content_Selected_Text_blue,NULL, colorSetupNotifier);
	CColorChooser chContentInactivecolor(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND_HEAD, &g_settings.menu_Content_inactive_red, &g_settings.menu_Content_inactive_green, &g_settings.menu_Content_inactive_blue, &g_settings.menu_Content_inactive_alpha, colorSetupNotifier);
	CColorChooser chContentInactiveTextcolor(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR_HEAD, &g_settings.menu_Content_inactive_Text_red, &g_settings.menu_Content_inactive_Text_green, &g_settings.menu_Content_inactive_Text_blue,NULL, colorSetupNotifier);

	ocs->addIntroItems(LOCALE_OSDSETTINGS_COLORMENU_MENUCOLORS, LOCALE_COLORMENUSETUP_MENUHEAD);
	ocs->addItem(new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND, true, NULL, &chHeadcolor));
	ocs->addItem(new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR, true, NULL, &chHeadTextcolor));
	ocs->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORMENUSETUP_MENUCONTENT));
	ocs->addItem(new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND, true, NULL, &chContentcolor));
	ocs->addItem(new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR, true, NULL, &chContentTextcolor));
	ocs->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORMENUSETUP_MENUCONTENT_INACTIVE));
	ocs->addItem(new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND, true, NULL, &chContentInactivecolor));
	ocs->addItem(new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR, true, NULL, &chContentInactiveTextcolor));
	ocs->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORMENUSETUP_MENUCONTENT_SELECTED));
	ocs->addItem(new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND, true, NULL, &chContentSelectedcolor));
	ocs->addItem(new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR, true, NULL, &chContentSelectedTextcolor));

	int res = ocs->exec(NULL, "");
	delete ocs;

	return res;
}


/* infobar colors */
int COsdSetup::showOsdInfobarColorSetup()
{
	CMenuWidget * ois = new CMenuWidget(menue_title, menue_icon, width);

	CColorChooser chInfobarcolor(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND_HEAD, &g_settings.infobar_red, &g_settings.infobar_green, &g_settings.infobar_blue,  &g_settings.infobar_alpha, colorSetupNotifier);
	CColorChooser chInfobarTextcolor_head(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR_HEAD, &g_settings.infobar_Text_red, &g_settings.infobar_Text_green, &g_settings.infobar_Text_blue, NULL, colorSetupNotifier);
	CMenuForwarder *fwInfobarBackground = new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_BACKGROUND, true, NULL, &chInfobarcolor);
	CMenuForwarder *fwInfobarTextcolor = new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_TEXTCOLOR, true, NULL, &chInfobarTextcolor_head);

	ois->addIntroItems(LOCALE_COLORSTATUSBAR_TEXT);
	//-----------------------------------------------
	ois->addItem(fwInfobarBackground);
	ois->addItem(fwInfobarTextcolor);

	int res = ois->exec(NULL, "");
	delete ois;

	return res;
}


/* OSD timeouts */
int COsdSetup::showOsdTimeoutSetup()
{
	/* note: SetupTiming() is already called in CNeutrinoApp::run */

	// dynamic created objects
	std::vector<CMenuTarget*> toDelete;

	CMenuWidget * ots = new CMenuWidget(menue_title, menue_icon, width);

	ots->addIntroItems(LOCALE_TIMING_HEAD);

	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
	{
		CStringInput * colorSettings_timing_item = new CStringInput(timing_setting[i].name, g_settings.timing_string[i], 3, LOCALE_TIMING_HINT_1, LOCALE_TIMING_HINT_2, "0123456789 ", this);
		toDelete.push_back(colorSettings_timing_item);
		ots->addItem(new CMenuForwarder(timing_setting[i].name, true, g_settings.timing_string[i], colorSettings_timing_item));
	}

	ots->addItem(GenericMenuSeparatorLine);
	CMenuForwarder * ots_defaults_fw = new CMenuForwarder(LOCALE_OPTIONS_DEFAULT, true, NULL, this, "osd.def");
	ots_defaults_fw->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);
	ots->addItem(ots_defaults_fw);

	int res = ots->exec(NULL, "");
	delete ots;

	// delete dynamic created objects
	unsigned int toDeleteSize = toDelete.size();
	for (unsigned int i = 0; i < toDeleteSize; i++)
		delete toDelete[i];

	return res;
}

#define INFOBAR_EPG_SHOW_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval  INFOBAR_EPG_SHOW_OPTIONS[INFOBAR_EPG_SHOW_OPTIONS_COUNT]=
{
	{ CInfoViewer::EPGINFO_NO_MESSAGE     , LOCALE_OPTIONS_OFF                          },
	{ CInfoViewer::EPGINFO_SIMPLE_MESSAGE , LOCALE_INFOVIEWER_EPGINFO_SIMPLE_MESSAGE    },
	{ CInfoViewer::EPGINFO_COMPLEX_MESSAGE, LOCALE_INFOVIEWER_EPGINFO_EXPENSIVE_MESSAGE }
};

#define INFOBAR_CHANNELLOGO_SHOW_OPTIONS_COUNT 4
const CMenuOptionChooser::keyval  INFOBAR_CHANNELLOGO_SHOW_OPTIONS[INFOBAR_CHANNELLOGO_SHOW_OPTIONS_COUNT]=
{
	{ CInfoViewer::NO_LOGO                , LOCALE_INFOVIEWER_CHANNELLOGO_OFF                     },
	{ CInfoViewer::LOGO_AS_CHANNELNUM     , LOCALE_INFOVIEWER_CHANNELLOGO_SHOW_IN_NUMBERBOX       },
	{ CInfoViewer::LOGO_AS_CHANNELNAME    , LOCALE_INFOVIEWER_CHANNELLOGO_SHOW_AS_CHANNELNAME     },
	{ CInfoViewer::LOGO_BESIDE_CHANNELNAME, LOCALE_INFOVIEWER_CHANNELLOGO_SHOW_BESIDE_CHANNELNAME }
};

#define INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval  INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS[INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS_COUNT]=
{
	{ CInfoViewer::NO_BACKGROUND, LOCALE_INFOVIEWER_CHANNELLOGO_BACKGROUND_OFF    },
	{ CInfoViewer::LOGO_FRAMED  , LOCALE_INFOVIEWER_CHANNELLOGO_BACKGROUND_FRAMED },
	{ CInfoViewer::LOGO_SHADED  , LOCALE_INFOVIEWER_CHANNELLOGO_BACKGROUND_SHADED }
};

#define INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT 5
const CMenuOptionChooser::keyval  INFOBAR_SUBCHAN_DISP_POS_OPTIONS[INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT]=
{
	{ CInfoViewer::SUBCHAN_DISP_POS_TOP_RIGHT   , LOCALE_SETTINGS_POS_TOP_RIGHT    },
	{ CInfoViewer::SUBCHAN_DISP_POS_TOP_LEFT    , LOCALE_SETTINGS_POS_TOP_LEFT     },
	{ CInfoViewer::SUBCHAN_DISP_POS_BOTTOM_LEFT , LOCALE_SETTINGS_POS_BOTTOM_LEFT  },
	{ CInfoViewer::SUBCHAN_DISP_POS_BOTTOM_RIGHT, LOCALE_SETTINGS_POS_BOTTOM_RIGHT },
	{ CInfoViewer::SUBCHAN_DISP_POS_INFOBAR     , LOCALE_SETTINGS_POS_INFOBAR      }
};

//infobar settings
int COsdSetup::showOsdInfobarSetup()
{
	CMenuWidget * oibs = new CMenuWidget(menue_title, menue_icon, width);

	//prepare items
	CMenuOptionChooser *oibs_sat_display_ch = new CMenuOptionChooser(LOCALE_OSDSETTINGS_INFOBAR_SAT_DISPLAY, &g_settings.infobar_sat_display, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);
	CMenuOptionChooser *oibs_subchanpos_ch 	= new CMenuOptionChooser(LOCALE_OSDSETTINGS_INFOVIEWER_SUBCHAN_DISP_POS, &g_settings.infobar_subchan_disp_pos, INFOBAR_SUBCHAN_DISP_POS_OPTIONS, INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT, true);
	CMenuOptionChooser *oibs_vzap_ch 	= new CMenuOptionChooser(LOCALE_OSDSETTINGS_INFOBAR_VIRTUAL_ZAP_MODE, &g_settings.virtual_zap_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);
	CMenuOptionChooser *oibs_epgshow_ch 	= new CMenuOptionChooser(LOCALE_OSDSETTINGS_INFOBAR_SHOW, &g_settings.infobar_show, INFOBAR_EPG_SHOW_OPTIONS, INFOBAR_EPG_SHOW_OPTIONS_COUNT, true);

#ifdef ENABLE_RADIOTEXT
	CMenuOptionChooser *oibs_radiotext_ch 	= new CMenuOptionChooser(LOCALE_OSDSETTINGS_INFOVIEWER_RADIOTEXT, &g_settings.radiotext_enable, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this);
#endif
	CMenuSeparator     *oibs_chanlogo_sep 	= new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_OSDSETTINGS_INFOBAR_CHANNELLOGO);
	
	
	//channel logo
	bool activ_logo_opts = g_settings.infobar_show_channellogo != CInfoViewer::NO_LOGO ? true : false;
	CMenuForwarder 	   *oibs_chanlogo_fw 	= new CMenuForwarder(LOCALE_OSDSETTINGS_INFOBAR_CHANNELLOGO_LOGODIR, activ_logo_opts, g_settings.infobar_channel_logodir, this, "channel_logodir");
	CMenuOptionChooser *oibs_chanlogo_bg_ch = new CMenuOptionChooser(LOCALE_OSDSETTINGS_INFOBAR_CHANNELLOGO_BACKGROUND, &g_settings.infobar_channellogo_background, INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS, INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS_COUNT, activ_logo_opts);
	CMenuForwarder 	   *oibs_updatelogo_fw = new CMenuForwarder(LOCALE_OSDSETTINGS_INFOBAR_CHANNELLOGO_UPDATE, activ_logo_opts, NULL, this, "update_logo");
	CMenuForwarder 	   *oibs_uninstalllogo_fw = new CMenuForwarder(LOCALE_OSDSETTINGS_INFOBAR_CHANNELLOGO_UNINSTALL, activ_logo_opts, NULL, this, "uninstall_logo");

	COnOffNotifier channelLogoNotifier;
	channelLogoNotifier.addItem(oibs_chanlogo_fw);
	channelLogoNotifier.addItem(oibs_chanlogo_bg_ch);
	channelLogoNotifier.addItem(oibs_updatelogo_fw);
	channelLogoNotifier.addItem(oibs_uninstalllogo_fw);
	CMenuOptionChooser *oibs_chanlogo_ch 	= new CMenuOptionChooser(LOCALE_OSDSETTINGS_INFOBAR_CHANNELLOGO_SHOW, &g_settings.infobar_show_channellogo, INFOBAR_CHANNELLOGO_SHOW_OPTIONS, INFOBAR_CHANNELLOGO_SHOW_OPTIONS_COUNT, true, &channelLogoNotifier);
	


	//show items
	oibs->addIntroItems(LOCALE_OSDSETTINGS_INFOBAR);
	//-------------------------------------------------
	oibs->addItem(oibs_sat_display_ch);
	oibs->addItem(oibs_subchanpos_ch);
	oibs->addItem(oibs_vzap_ch);
	oibs->addItem(oibs_epgshow_ch);
#ifdef ENABLE_RADIOTEXT
	oibs->addItem(oibs_radiotext_ch);
#endif
	oibs->addItem(oibs_chanlogo_sep);
	//-------------------------------------------------	
	oibs->addItem(oibs_chanlogo_ch);
	oibs->addItem(oibs_chanlogo_fw);
	oibs->addItem(oibs_chanlogo_bg_ch);
	oibs->addItem(oibs_updatelogo_fw);
	oibs->addItem(oibs_uninstalllogo_fw);

	int res = oibs->exec(NULL, "");
	delete oibs;

	return res;
}

#define CHANNELLIST_ADDITIONAL_OPTION_COUNT 3
const CMenuOptionChooser::keyval CHANNELLIST_ADDITIONAL_OPTIONS[CHANNELLIST_ADDITIONAL_OPTION_COUNT] =
{
	{ CChannelList::ADDITIONAL_OFF, LOCALE_CHANNELLIST_ADDITIONAL_OFF       },
	{ CChannelList::ADDITIONAL_ON , LOCALE_CHANNELLIST_ADDITIONAL_ON        },
	{ CChannelList::ADDITIONAL_MTV, LOCALE_CHANNELLIST_ADDITIONAL_ON_MINITV }
};

#define CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS_COUNT 2
const CMenuOptionChooser::keyval  CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS[CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS_COUNT]=
{
	{ 0 , LOCALE_CHANNELLIST_EPGTEXT_ALIGN_LEFT },
	{ 1 , LOCALE_CHANNELLIST_EPGTEXT_ALIGN_RIGHT }
};

#define CHANNELLIST_FOOT_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval  CHANNELLIST_FOOT_OPTIONS[CHANNELLIST_FOOT_OPTIONS_COUNT]=
{
	{ CChannelList::FOOT_FREQ, LOCALE_CHANNELLIST_FOOT_FREQ },
	{ CChannelList::FOOT_NEXT, LOCALE_CHANNELLIST_FOOT_NEXT },
	{ CChannelList::FOOT_OFF , LOCALE_CHANNELLIST_FOOT_OFF  }
};

//channellist
int COsdSetup::showOsdChannelListSetup()
{
	CMenuWidget * ocls = new CMenuWidget(menue_title, menue_icon, width);

	// channellist additional
	CMenuOptionChooser *ocls_additional	= new CMenuOptionChooser(LOCALE_CHANNELLIST_ADDITIONAL, &g_settings.channellist_additional, CHANNELLIST_ADDITIONAL_OPTIONS, CHANNELLIST_ADDITIONAL_OPTION_COUNT, true);
	// epg align
	CMenuOptionChooser *ocls_align_ch 	= new CMenuOptionChooser(LOCALE_MISCSETTINGS_CHANNELLIST_EPGTEXT_ALIGN, &g_settings.channellist_epgtext_align_right, CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS, CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS_COUNT, true);
	// extended channel list
	CMenuOptionChooser *ocls_ext_ch 	= new CMenuOptionChooser(LOCALE_CHANNELLIST_EXTENDED, &g_settings.channellist_extended, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);
	// foot
	CMenuOptionChooser *ocls_foot		= new CMenuOptionChooser(LOCALE_CHANNELLIST_FOOT, &g_settings.channellist_foot, CHANNELLIST_FOOT_OPTIONS, CHANNELLIST_FOOT_OPTIONS_COUNT, true);


	//show items
	ocls->addIntroItems(LOCALE_MISCSETTINGS_CHANNELLIST);
	//-------------------------------------------------
	ocls->addItem(ocls_additional);
	ocls->addItem(ocls_align_ch);
	ocls->addItem(ocls_ext_ch);
	ocls->addItem(ocls_foot);

	int res = ocls->exec(NULL, "");
	delete ocls;

	return res;
}


/* for font size setup */
class CMenuNumberInput : public CMenuForwarder, CMenuTarget, CChangeObserver
{
private:
	CChangeObserver * observer;
	CConfigFile     * configfile;
	int32_t           defaultvalue;
	char              value[11];

protected:

	virtual const char * getOption(void)
		{
			sprintf(value, "%u", configfile->getInt32(locale_real_names[text], defaultvalue));
			return value;
		}

	virtual bool changeNotify(const neutrino_locale_t OptionName, void * Data)
		{
			configfile->setInt32(locale_real_names[text], atoi(value));
			return observer->changeNotify(OptionName, Data);
		}


public:
	CMenuNumberInput(const neutrino_locale_t Text, const int32_t DefaultValue, CChangeObserver * const Observer, CConfigFile * const Configfile) : CMenuForwarder(Text, true, NULL, this)
		{
			observer     = Observer;
			configfile   = Configfile;
			defaultvalue = DefaultValue;
		}

	int exec(CMenuTarget * parent, const std::string & action_Key)
		{
			CStringInput input(text, (char *)getOption(), 3, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ", this);
			return input.exec(parent, action_Key);
		}
};

void COsdSetup::AddFontSettingItem(CMenuWidget *fontSettings, const SNeutrinoSettings::FONT_TYPES number_of_fontsize_entry)
{
	fontSettings->addItem(new CMenuNumberInput(neutrino_font[number_of_fontsize_entry].name, neutrino_font[number_of_fontsize_entry].defaultsize, fontsizenotifier, CNeutrinoApp::getInstance()->getConfigFile()));
}

/* font settings  */
int COsdSetup::showOsdFontSizeSetup()
{
	char val_x[4] = {0};
	char val_y[4] = {0};
	snprintf(val_x,sizeof(val_x), "%03d",g_settings.screen_xres);
	snprintf(val_y,sizeof(val_y), "%03d",g_settings.screen_yres);

	CMenuWidget * fontSettings = new CMenuWidget(menue_title, menue_icon, width);
	fontSettings->addIntroItems(LOCALE_FONTMENU_HEAD);

	// select gui font file
	CFile font_file;
	font_file.Name = g_settings.font_file;
	font_file_name = font_file.getFileName();
	CMenuForwarder *mf = new CMenuForwarder(LOCALE_OSDSETTINGS_COLORMENU_FONT, true, font_file_name, this, "select_font", CRCInput::RC_red);
	fontSettings->addItem(mf);
	fontSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_FONTMENU_SIZES));

	// submenu font scaling
	CMenuWidget * fontscale = new CMenuWidget(LOCALE_FONTMENU_HEAD, menue_icon, width);
	fontscale->addIntroItems(LOCALE_FONTMENU_SCALING);

	CStringInput xres_count(LOCALE_FONTMENU_SCALING_X, val_x,/*50,200,*/ 3, LOCALE_FONTMENU_SCALING, LOCALE_FONTMENU_SCALING_X_HINT2, "0123456789 ", fontsizenotifier);
	CMenuForwarder *m_x = new CMenuForwarder(LOCALE_FONTMENU_SCALING_X, true, val_x, &xres_count);

	CStringInput yres_count(LOCALE_FONTMENU_SCALING_Y, val_y,/*50,200,*/ 3, LOCALE_FONTMENU_SCALING, LOCALE_FONTMENU_SCALING_Y_HINT2, "0123456789 ", fontsizenotifier);
	CMenuForwarder *m_y = new CMenuForwarder(LOCALE_FONTMENU_SCALING_Y, true, val_y, &yres_count);

	fontscale->addItem(m_x);
	fontscale->addItem(m_y);
	fontSettings->addItem(new CMenuForwarder(LOCALE_FONTMENU_SCALING, true, NULL, fontscale)); //OK

	AddFontSettingItem(fontSettings, SNeutrinoSettings::FONT_TYPE_MENU_TITLE);
	AddFontSettingItem(fontSettings, SNeutrinoSettings::FONT_TYPE_MENU);
	AddFontSettingItem(fontSettings, SNeutrinoSettings::FONT_TYPE_MENU_INFO);

	fontSettings->addItem(GenericMenuSeparatorLine);

	// dynamic created objects
	std::vector<CMenuTarget*> toDelete;

	for (int i = 0; i < 5; i++)
	{
		CMenuWidget * fontSettingsSubMenu = new CMenuWidget(font_sizes_groups[i].groupname, menue_icon, width);
		toDelete.push_back(fontSettingsSubMenu);
		fontSettingsSubMenu->addIntroItems();
		for (unsigned int j = 0; j < font_sizes_groups[i].count; j++)
		{
			AddFontSettingItem(fontSettingsSubMenu, font_sizes_groups[i].content[j]);
		}
		fontSettingsSubMenu->addItem(GenericMenuSeparatorLine);
		CMenuForwarder * fontSettingsSubMenu_loadDefaults = new CMenuForwarder(LOCALE_OPTIONS_DEFAULT, true, NULL, this, font_sizes_groups[i].actionkey);
		fontSettingsSubMenu_loadDefaults->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);
		fontSettingsSubMenu->addItem(fontSettingsSubMenu_loadDefaults);

		fontSettings->addItem(new CMenuForwarder(font_sizes_groups[i].groupname, true, NULL, fontSettingsSubMenu));
	}

	AddFontSettingItem(fontSettings, SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM);
	fontSettings->addItem(GenericMenuSeparatorLine);
	CMenuForwarder * fontSettings_loadDefaults = new CMenuForwarder(LOCALE_OPTIONS_DEFAULT, true, NULL, this, font_sizes_groups[5].actionkey);
	fontSettings_loadDefaults->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);
	fontSettings->addItem(fontSettings_loadDefaults);

	int res = fontSettings->exec(NULL, "");
	delete fontSettings;
	delete fontscale;

	// delete dynamic created objects
	unsigned int toDeleteSize = toDelete.size();
	for (unsigned int i = 0; i < toDeleteSize; i++)
		delete toDelete[i];

	return res;
}

/* timezone settings  */
void COsdSetup::showOsdTimeZoneSetup()
{
	CMenuWidget * timezoneSettings = new CMenuWidget(menue_title, menue_icon, width);
	CMenuSeparator * timezoneSettings_subhead = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_TIMEZONEMENU_HEAD);

	timezoneSettings->addItem(timezoneSettings_subhead);
	timezoneSettings->addItem(GenericMenuSeparator);
	timezoneSettings->addItem(GenericMenuBack);
	timezoneSettings->addItem(GenericMenuSeparatorLine);

	xmlDocPtr parser = parseXmlFile("/etc/timezone.xml");

	CMenuOptionStringChooser* tzSelect;

	if (parser != NULL)
	{
		tzSelect = new CMenuOptionStringChooser(LOCALE_TIMEZONEMENU_OPTION, g_settings.timezone, true, new CTZChangeNotifier());
		xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
		bool found = false;

		while (search)
		{
			if (!strcmp(xmlGetName(search), "zone"))
			{
				std::string name = xmlGetAttribute(search, "name");
				tzSelect->addOption(name.c_str());
				found = true;
			}
			search = search->xmlNextNode;
		}

		if (found)
			timezoneSettings->addItem(tzSelect);
		else
		{
			delete tzSelect;
			tzSelect = NULL;
		}

		xmlFreeDoc(parser);
	}

	timezoneSettings->exec(NULL, "");
	timezoneSettings->hide();
	delete timezoneSettings;
}

bool CFontSizeNotifier::changeNotify(const neutrino_locale_t OptionName, void * data)
{
	if (data != NULL) {
		int xre = g_settings.screen_xres;
		int yre = g_settings.screen_yres;
		char dat[4];
		char val[4];
		sscanf((char*) data, "%hhu", &dat[0]);
		sprintf(val, "%hhu", dat[0]);

		if (ARE_LOCALES_EQUAL(OptionName, LOCALE_FONTMENU_SCALING_X))
		{
			xre = atoi(val);
			//fallback for min/max bugs ;)
			if( xre < 50 || xre > 200 ) {
				xre = g_settings.screen_xres;
				snprintf((char *)data,sizeof(data), "%03d",g_settings.screen_xres);
			}
		}
		else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_FONTMENU_SCALING_Y))
		{
			yre = atoi(val);
			if( yre < 50 || yre > 200 ) {
				yre = g_settings.screen_yres;
				snprintf((char *)data,sizeof(data), "%03d",g_settings.screen_yres);
			}
		}

		if (xre != g_settings.screen_xres || yre != g_settings.screen_yres) {
			printf("[neutrino] new font scale settings x: %d%% y: %d%%\n", xre, yre);
			g_settings.screen_xres = xre;
			g_settings.screen_yres = yre;
		}
	}

	CHintBox hintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_FONTSIZE_HINT)); // UTF-8
	hintBox.paint();

	CNeutrinoApp::getInstance()->SetupFonts();
	hintBox.hide();

	return true;
}


bool COsdSetup::changeNotify(const neutrino_locale_t OptionName, void *)
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_OSDSETTINGS_ROUNDED_CORNERS) ||
	    ARE_LOCALES_EQUAL(OptionName, LOCALE_OSDSETTINGS_MENU_NUMBERS_AS_ICONS))
	{
		osd_setup->hide();
		return true;
	}
#ifdef ENABLE_RADIOTEXT
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_OSDSETTINGS_INFOVIEWER_RADIOTEXT))
	{
		if (g_settings.radiotext_enable)
		{
			if (g_Radiotext == NULL)
				g_Radiotext = new CRadioText;
			if (g_Radiotext && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio))
				g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
		}
		else
		{
			// stop radiotext PES decoding
			if (g_Radiotext)
				g_Radiotext->radiotext_stop();
			delete g_Radiotext;
			g_Radiotext = NULL;
		}
		return false;
	}
#endif

	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
	{
		if (ARE_LOCALES_EQUAL(OptionName, timing_setting[i].name))
		{
			g_settings.timing[i] = atoi(g_settings.timing_string[i]);
			return true;
		}
	}

	return false;
}

