/*
	Neutrino-GUI  -   DBoxII-Project

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

	$Id: themes.cpp,v 1.30 2012/09/23 08:18:03 rhabarber1848 Exp $ 

	Copyright (C) 2007, 2008, 2009 (flasher) Frank Liebelt

*/

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <global.h>
#include <neutrino.h>
#include "widget/menue.h"
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/messagebox.h>
#include <driver/framebuffer.h>
#include <driver/screen_max.h>

#include <sys/stat.h>
#include <sys/time.h>

#include "themes.h"

#define THEMEDIR THEMESDIR "/"
#define USERDIR "/var" THEMEDIR
#define FILE_SUFFIX ".theme"

CThemes::CThemes(const neutrino_locale_t title, const char * const IconName)
: themefile('\t')
{
	colorSetupNotifier = new CColorSetupNotifier();

	menue_title = title;
	menue_icon = IconName;

	width = w_max (500, 100);
	selected = -1;

	hasThemeChanged = false;
}

CThemes::~CThemes()
{
	delete colorSetupNotifier;
}

int CThemes::exec(CMenuTarget* parent, const std::string & actionKey)
{
	int res = menu_return::RETURN_REPAINT;

	if( !actionKey.empty() )
	{
		if (actionKey=="theme_neutrino")
		{
			setupDefaultColors();
			colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
		}
		else
		{
			std::string themeFile = actionKey;
			if ( strstr(themeFile.c_str(), "{U}") != 0 ) 
			{
				themeFile.erase(0, 3);
				readFile((char*)((std::string)USERDIR + themeFile + FILE_SUFFIX).c_str());
			} 
			else
				readFile((char*)((std::string)THEMEDIR + themeFile + FILE_SUFFIX).c_str());
		}
		return res;
	}

	if (parent)
		parent->hide();

	if ( !hasThemeChanged )
		rememberOldTheme( true );

	res = Show();
	return res;
}

void CThemes::readThemes(CMenuWidget &themes)
{
	struct dirent **themelist;
	int n;
	const char *pfade[] = {THEMEDIR, USERDIR};
	bool hasCVSThemes, hasUserThemes;
	hasCVSThemes = hasUserThemes = false;
	std::string userThemeFile = "";
	CMenuForwarder* oj;

	for(int p = 0;p < 2;p++)
	{
		n = scandir(pfade[p], &themelist, 0, alphasort);
		if(n < 0)
			perror("loading themes: scandir");
		else
		{
			for(int count=0;count<n;count++)
			{
				char *file = themelist[count]->d_name;
				char *pos = strstr(file, ".theme");
				if(pos != NULL)
				{
					if ( p == 0 && hasCVSThemes == false ) {
						themes.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORTHEMEMENU_SELECT2));
						hasCVSThemes = true;
					} else if ( p == 1 && hasUserThemes == false ) {
						themes.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORTHEMEMENU_SELECT1));
						hasUserThemes = true;
					}
					*pos = '\0';
					if ( p == 1 ) {
						userThemeFile = "{U}" + (std::string)file;
						oj = new CMenuForwarder(file, true, "", this, userThemeFile.c_str());
					} else
						oj = new CMenuForwarder(file, true, "", this, file);
					oj->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);
					themes.addItem( oj );
				}
				free(themelist[count]);
			}
			free(themelist);
		}
	}
}

int CThemes::Show()
{
	std::string file_name = "";

	CMenuWidget themes(menue_title, menue_icon, width);
	themes.setPreselected(selected);

	//intros
	themes.addIntroItems(menue_title != LOCALE_COLORTHEMEMENU_HEAD2 ? LOCALE_COLORTHEMEMENU_HEAD2 : NONEXISTANT_LOCALE);
	
	//set default theme
	themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_NEUTRINO_THEME, true, NULL, this, "theme_neutrino", CRCInput::RC_red));

	readThemes(themes);

	CStringInputSMS nameInput(LOCALE_COLORTHEMEMENU_NAME, &file_name, 30, false, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789- ");
	CMenuForwarder *m1 = new CMenuForwarder(LOCALE_COLORTHEMEMENU_SAVE, true , NULL, &nameInput);

	// Don't show SAVE if UserDir does'nt exist
	if ( access(USERDIR, F_OK) != 0 ) { // check for existance
	// mkdir must be called for each subdir which does not exist 
	//	mkdir (USERDIR, S_IRUSR | S_IREAD | S_IWUSR | S_IWRITE | S_IXUSR | S_IEXEC) == 0) {
		if (system (((std::string)"mkdir -p " + USERDIR).c_str()) != 0) {
			printf("[neutrino theme] error creating %s\n", USERDIR);
		}
	}
	if (access(USERDIR, F_OK) == 0 ) {
		themes.addItem(GenericMenuSeparatorLine);
		themes.addItem(m1);
	} else {
		delete m1;
		printf("[neutrino theme] error accessing %s\n", USERDIR);
	}

	int res = themes.exec(NULL, "");
	selected = themes.getSelected();

	if (!file_name.empty()) {
		saveFile((char*)((std::string)USERDIR + file_name + FILE_SUFFIX).c_str());
	}

	if (hasThemeChanged) {
		if (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_COLORTHEMEMENU_QUESTION, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, menue_icon.c_str()) != CMessageBox::mbrYes)
			rememberOldTheme( false );
		else
			hasThemeChanged = false;
	}

	return res;
}

void CThemes::rememberOldTheme(bool remember)
{
	if ( remember ) {
		oldThemeValues[0]  = g_settings.menu_Head_alpha;
		oldThemeValues[1]  = g_settings.menu_Head_red;
		oldThemeValues[2]  = g_settings.menu_Head_green;
		oldThemeValues[3]  = g_settings.menu_Head_blue;
		oldThemeValues[4]  = g_settings.menu_Head_Text_alpha;
		oldThemeValues[5]  = g_settings.menu_Head_Text_red;
		oldThemeValues[6]  = g_settings.menu_Head_Text_green;
		oldThemeValues[7]  = g_settings.menu_Head_Text_blue;
		oldThemeValues[8]  = g_settings.menu_Content_alpha;
		oldThemeValues[9]  = g_settings.menu_Content_red;
		oldThemeValues[10] = g_settings.menu_Content_green;
		oldThemeValues[11] = g_settings.menu_Content_blue;
		oldThemeValues[12] = g_settings.menu_Content_Text_alpha;
		oldThemeValues[13] = g_settings.menu_Content_Text_red;
		oldThemeValues[14] = g_settings.menu_Content_Text_green;
		oldThemeValues[15] = g_settings.menu_Content_Text_blue;
		oldThemeValues[16] = g_settings.menu_Content_Selected_alpha;
		oldThemeValues[17] = g_settings.menu_Content_Selected_red;
		oldThemeValues[18] = g_settings.menu_Content_Selected_green;
		oldThemeValues[19] = g_settings.menu_Content_Selected_blue;
		oldThemeValues[20] = g_settings.menu_Content_Selected_Text_alpha;
		oldThemeValues[21] = g_settings.menu_Content_Selected_Text_red;
		oldThemeValues[22] = g_settings.menu_Content_Selected_Text_green;
		oldThemeValues[23] = g_settings.menu_Content_Selected_Text_blue;
		oldThemeValues[24] = g_settings.menu_Content_inactive_alpha;
		oldThemeValues[25] = g_settings.menu_Content_inactive_red;
		oldThemeValues[26] = g_settings.menu_Content_inactive_green;
		oldThemeValues[27] = g_settings.menu_Content_inactive_blue;
		oldThemeValues[28] = g_settings.menu_Content_inactive_Text_alpha;
		oldThemeValues[29] = g_settings.menu_Content_inactive_Text_red;
		oldThemeValues[30] = g_settings.menu_Content_inactive_Text_green;
		oldThemeValues[31] = g_settings.menu_Content_inactive_Text_blue;
		oldThemeValues[32] = g_settings.infobar_alpha;
		oldThemeValues[33] = g_settings.infobar_red;
		oldThemeValues[34] = g_settings.infobar_green;
		oldThemeValues[35] = g_settings.infobar_blue;
		oldThemeValues[36] = g_settings.infobar_Text_alpha;
		oldThemeValues[37] = g_settings.infobar_Text_red;
		oldThemeValues[38] = g_settings.infobar_Text_green;
		oldThemeValues[39] = g_settings.infobar_Text_blue;
	} else {
		g_settings.menu_Head_alpha 			= oldThemeValues[0];
		g_settings.menu_Head_red 			= oldThemeValues[1];
		g_settings.menu_Head_green 			= oldThemeValues[2];
		g_settings.menu_Head_blue 			= oldThemeValues[3];
		g_settings.menu_Head_Text_alpha 		= oldThemeValues[4];
		g_settings.menu_Head_Text_red 			= oldThemeValues[5];
		g_settings.menu_Head_Text_green 		= oldThemeValues[6];
		g_settings.menu_Head_Text_blue 			= oldThemeValues[7];
		g_settings.menu_Content_alpha 			= oldThemeValues[8];
		g_settings.menu_Content_red 			= oldThemeValues[9];
		g_settings.menu_Content_green 			= oldThemeValues[10];
		g_settings.menu_Content_blue 			= oldThemeValues[11];
		g_settings.menu_Content_Text_alpha 		= oldThemeValues[12];
		g_settings.menu_Content_Text_red 		= oldThemeValues[13];
		g_settings.menu_Content_Text_green 		= oldThemeValues[14];
		g_settings.menu_Content_Text_blue 		= oldThemeValues[15];
		g_settings.menu_Content_Selected_alpha 		= oldThemeValues[16];
		g_settings.menu_Content_Selected_red 		= oldThemeValues[17];
		g_settings.menu_Content_Selected_green 		= oldThemeValues[18];
		g_settings.menu_Content_Selected_blue 		= oldThemeValues[19];
		g_settings.menu_Content_Selected_Text_alpha 	= oldThemeValues[20];
		g_settings.menu_Content_Selected_Text_red 	= oldThemeValues[21];
		g_settings.menu_Content_Selected_Text_green 	= oldThemeValues[22];
		g_settings.menu_Content_Selected_Text_blue 	= oldThemeValues[23];
		g_settings.menu_Content_inactive_alpha 		= oldThemeValues[24];
		g_settings.menu_Content_inactive_red 		= oldThemeValues[25];
		g_settings.menu_Content_inactive_green 		= oldThemeValues[26];
		g_settings.menu_Content_inactive_blue		= oldThemeValues[27];
		g_settings.menu_Content_inactive_Text_alpha 	= oldThemeValues[28];
		g_settings.menu_Content_inactive_Text_red 	= oldThemeValues[29];
		g_settings.menu_Content_inactive_Text_green 	= oldThemeValues[30];
		g_settings.menu_Content_inactive_Text_blue 	= oldThemeValues[31];
		g_settings.infobar_alpha 			= oldThemeValues[32];
		g_settings.infobar_red 				= oldThemeValues[33];
		g_settings.infobar_green 			= oldThemeValues[34];
		g_settings.infobar_blue 			= oldThemeValues[35];
		g_settings.infobar_Text_alpha 			= oldThemeValues[36];
		g_settings.infobar_Text_red 			= oldThemeValues[37];
		g_settings.infobar_Text_green 			= oldThemeValues[38];
		g_settings.infobar_Text_blue 			= oldThemeValues[39];

		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
		hasThemeChanged = false;
	}
}

void CThemes::readFile(char* themename)
{
	if(themefile.loadConfig(themename))
	{
		getColors(themefile);
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
		hasThemeChanged = true;
	}
	else
		printf("[neutrino theme] %s not found\n", themename);
}

void CThemes::saveFile(char * themename)
{
	setColors(themefile);
	if (!themefile.saveConfig(themename))
		printf("[neutrino theme] %s write error\n", themename);
}

// setup default Color Sheme (Neutrino)
void CThemes::setupDefaultColors()
{
	CConfigFile empty(':');
	getColors(empty);
}

void CThemes::getColors(CConfigFile &configfile)
{
	g_settings.menu_Head_alpha = configfile.getInt32( "menu_Head_alpha", 0x00 );
	g_settings.menu_Head_red = configfile.getInt32( "menu_Head_red", 0x00 );
	g_settings.menu_Head_green = configfile.getInt32( "menu_Head_green", 0x0A );
	g_settings.menu_Head_blue = configfile.getInt32( "menu_Head_blue", 0x19 );
	g_settings.menu_Head_Text_alpha = configfile.getInt32( "menu_Head_Text_alpha", 0x00 );
	g_settings.menu_Head_Text_red = configfile.getInt32( "menu_Head_Text_red", 0x5F );
	g_settings.menu_Head_Text_green = configfile.getInt32( "menu_Head_Text_green", 0x46 );
	g_settings.menu_Head_Text_blue = configfile.getInt32( "menu_Head_Text_blue", 0x00 );
	g_settings.menu_Content_alpha = configfile.getInt32( "menu_Content_alpha", 0x00 );
	g_settings.menu_Content_red = configfile.getInt32( "menu_Content_red", 0x00 );
	g_settings.menu_Content_green = configfile.getInt32( "menu_Content_green", 0x0F );
	g_settings.menu_Content_blue = configfile.getInt32( "menu_Content_blue", 0x1E );
	g_settings.menu_Content_Text_alpha = configfile.getInt32( "menu_Content_Text_alpha", 0x00 );
	g_settings.menu_Content_Text_red = configfile.getInt32( "menu_Content_Text_red", 0x64 );
	g_settings.menu_Content_Text_green = configfile.getInt32( "menu_Content_Text_green", 0x64 );
	g_settings.menu_Content_Text_blue = configfile.getInt32( "menu_Content_Text_blue", 0x64 );
	g_settings.menu_Content_Selected_alpha = configfile.getInt32( "menu_Content_Selected_alpha", 0x00 );
	g_settings.menu_Content_Selected_red = configfile.getInt32( "menu_Content_Selected_red", 0x5A );
	g_settings.menu_Content_Selected_green = configfile.getInt32( "menu_Content_Selected_green", 0x5A );
	g_settings.menu_Content_Selected_blue = configfile.getInt32( "menu_Content_Selected_blue", 0x00 );
	g_settings.menu_Content_Selected_Text_alpha = configfile.getInt32( "menu_Content_Selected_Text_alpha", 0x00 );
	g_settings.menu_Content_Selected_Text_red = configfile.getInt32( "menu_Content_Selected_Text_red", 0x00 );
	g_settings.menu_Content_Selected_Text_green = configfile.getInt32( "menu_Content_Selected_Text_green", 0x00 );
	g_settings.menu_Content_Selected_Text_blue = configfile.getInt32( "menu_Content_Selected_Text_blue", 0x00 );
	g_settings.menu_Content_inactive_alpha = configfile.getInt32( "menu_Content_inactive_alpha", 0x00 );
	g_settings.menu_Content_inactive_red = configfile.getInt32( "menu_Content_inactive_red", 0x00 );
	g_settings.menu_Content_inactive_green = configfile.getInt32( "menu_Content_inactive_green", 0x0F );
	g_settings.menu_Content_inactive_blue = configfile.getInt32( "menu_Content_inactive_blue", 0x1E );
	g_settings.menu_Content_inactive_Text_alpha = configfile.getInt32( "menu_Content_inactive_Text_alpha", 0x00 );
	g_settings.menu_Content_inactive_Text_red = configfile.getInt32( "menu_Content_inactive_Text_red", 0x5F );
	g_settings.menu_Content_inactive_Text_green = configfile.getInt32( "menu_Content_inactive_Text_green", 0x46 );
	g_settings.menu_Content_inactive_Text_blue = configfile.getInt32( "menu_Content_inactive_Text_blue", 0x28 );
	g_settings.infobar_alpha = configfile.getInt32( "infobar_alpha", 0x00 );
	g_settings.infobar_red = configfile.getInt32( "infobar_red", 0x00 );
	g_settings.infobar_green = configfile.getInt32( "infobar_green", 0x00 );
	g_settings.infobar_blue = configfile.getInt32( "infobar_blue", 0x19 );
	g_settings.infobar_Text_alpha = configfile.getInt32( "infobar_Text_alpha", 0x00 );
	g_settings.infobar_Text_red = configfile.getInt32( "infobar_Text_red", 0x64 );
	g_settings.infobar_Text_green = configfile.getInt32( "infobar_Text_green", 0x64 );
	g_settings.infobar_Text_blue = configfile.getInt32( "infobar_Text_blue", 0x64 );
}

void CThemes::setColors(CConfigFile &configfile)
{
	configfile.setInt32( "menu_Head_alpha", g_settings.menu_Head_alpha );
	configfile.setInt32( "menu_Head_red", g_settings.menu_Head_red );
	configfile.setInt32( "menu_Head_green", g_settings.menu_Head_green );
	configfile.setInt32( "menu_Head_blue", g_settings.menu_Head_blue );
	configfile.setInt32( "menu_Head_Text_alpha", g_settings.menu_Head_Text_alpha );
	configfile.setInt32( "menu_Head_Text_red", g_settings.menu_Head_Text_red );
	configfile.setInt32( "menu_Head_Text_green", g_settings.menu_Head_Text_green );
	configfile.setInt32( "menu_Head_Text_blue", g_settings.menu_Head_Text_blue );
	configfile.setInt32( "menu_Content_alpha", g_settings.menu_Content_alpha );
	configfile.setInt32( "menu_Content_red", g_settings.menu_Content_red );
	configfile.setInt32( "menu_Content_green", g_settings.menu_Content_green );
	configfile.setInt32( "menu_Content_blue", g_settings.menu_Content_blue );
	configfile.setInt32( "menu_Content_Text_alpha", g_settings.menu_Content_Text_alpha );
	configfile.setInt32( "menu_Content_Text_red", g_settings.menu_Content_Text_red );
	configfile.setInt32( "menu_Content_Text_green", g_settings.menu_Content_Text_green );
	configfile.setInt32( "menu_Content_Text_blue", g_settings.menu_Content_Text_blue );
	configfile.setInt32( "menu_Content_Selected_alpha", g_settings.menu_Content_Selected_alpha );
	configfile.setInt32( "menu_Content_Selected_red", g_settings.menu_Content_Selected_red );
	configfile.setInt32( "menu_Content_Selected_green", g_settings.menu_Content_Selected_green );
	configfile.setInt32( "menu_Content_Selected_blue", g_settings.menu_Content_Selected_blue );
	configfile.setInt32( "menu_Content_Selected_Text_alpha", g_settings.menu_Content_Selected_Text_alpha );
	configfile.setInt32( "menu_Content_Selected_Text_red", g_settings.menu_Content_Selected_Text_red );
	configfile.setInt32( "menu_Content_Selected_Text_green", g_settings.menu_Content_Selected_Text_green );
	configfile.setInt32( "menu_Content_Selected_Text_blue", g_settings.menu_Content_Selected_Text_blue );
	configfile.setInt32( "menu_Content_inactive_alpha", g_settings.menu_Content_inactive_alpha );
	configfile.setInt32( "menu_Content_inactive_red", g_settings.menu_Content_inactive_red );
	configfile.setInt32( "menu_Content_inactive_green", g_settings.menu_Content_inactive_green );
	configfile.setInt32( "menu_Content_inactive_blue", g_settings.menu_Content_inactive_blue );
	configfile.setInt32( "menu_Content_inactive_Text_alpha", g_settings.menu_Content_inactive_Text_alpha );
	configfile.setInt32( "menu_Content_inactive_Text_red", g_settings.menu_Content_inactive_Text_red );
	configfile.setInt32( "menu_Content_inactive_Text_green", g_settings.menu_Content_inactive_Text_green );
	configfile.setInt32( "menu_Content_inactive_Text_blue", g_settings.menu_Content_inactive_Text_blue );
	configfile.setInt32( "infobar_alpha", g_settings.infobar_alpha );
	configfile.setInt32( "infobar_red", g_settings.infobar_red );
	configfile.setInt32( "infobar_green", g_settings.infobar_green );
	configfile.setInt32( "infobar_blue", g_settings.infobar_blue );
	configfile.setInt32( "infobar_Text_alpha", g_settings.infobar_Text_alpha );
	configfile.setInt32( "infobar_Text_red", g_settings.infobar_Text_red );
	configfile.setInt32( "infobar_Text_green", g_settings.infobar_Text_green );
	configfile.setInt32( "infobar_Text_blue", g_settings.infobar_Text_blue );
}

bool CColorSetupNotifier::changeNotify(const neutrino_locale_t, void *)
{
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();
//	unsigned char r,g,b;
	//setting colors-..
	frameBuffer->paletteGenFade(COL_MENUHEAD,
	                              convertSetupColor2RGB(g_settings.menu_Head_red, g_settings.menu_Head_green, g_settings.menu_Head_blue),
	                              convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue),
	                              8, convertSetupAlpha2Alpha( g_settings.menu_Head_alpha ) );

	frameBuffer->paletteGenFade(COL_MENUCONTENT,
	                              convertSetupColor2RGB(g_settings.menu_Content_red, g_settings.menu_Content_green, g_settings.menu_Content_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );


	frameBuffer->paletteGenFade(COL_MENUCONTENTDARK,
	                              convertSetupColor2RGB(int(g_settings.menu_Content_red*0.6), int(g_settings.menu_Content_green*0.6), int(g_settings.menu_Content_blue*0.6)),
	                              convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );

	frameBuffer->paletteGenFade(COL_MENUCONTENTSELECTED,
	                              convertSetupColor2RGB(g_settings.menu_Content_Selected_red, g_settings.menu_Content_Selected_green, g_settings.menu_Content_Selected_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_Selected_Text_red, g_settings.menu_Content_Selected_Text_green, g_settings.menu_Content_Selected_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_Selected_alpha) );

	frameBuffer->paletteGenFade(COL_MENUCONTENTINACTIVE,
	                              convertSetupColor2RGB(g_settings.menu_Content_inactive_red, g_settings.menu_Content_inactive_green, g_settings.menu_Content_inactive_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_inactive_Text_red, g_settings.menu_Content_inactive_Text_green, g_settings.menu_Content_inactive_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_inactive_alpha) );

	frameBuffer->paletteGenFade(COL_INFOBAR,
	                              convertSetupColor2RGB(g_settings.infobar_red, g_settings.infobar_green, g_settings.infobar_blue),
	                              convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

/*	frameBuffer->paletteSetColor( COL_INFOBAR_SHADOW,
	                                convertSetupColor2RGB(
	                                    int(g_settings.infobar_red*0.4),
	                                    int(g_settings.infobar_green*0.4),
	                                    int(g_settings.infobar_blue*0.4)),
	                                g_settings.infobar_alpha);
*/
	frameBuffer->paletteGenFade(COL_INFOBAR_SHADOW,
	                              convertSetupColor2RGB(int(g_settings.infobar_red*0.4), int(g_settings.infobar_green*0.4), int(g_settings.infobar_blue*0.4)),
	                              convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
	
	frameBuffer->paletteSet();
	return false;
}

