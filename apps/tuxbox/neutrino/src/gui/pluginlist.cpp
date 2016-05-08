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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <plugin.h>

#include <gui/pluginlist.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/icons.h>

#include <sstream>
#include <fstream>
#include <iostream>

#include <dirent.h>
#include <dlfcn.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <global.h>
#include <neutrino.h>
#include <plugins.h>
#include <driver/encoding.h>
#include <driver/screen_max.h>

#include <zapit/client/zapittools.h>
#include <daemonc/remotecontrol.h>

CPluginList::CPluginList(const neutrino_locale_t Name, const uint listtype)
{
	frameBuffer = CFrameBuffer::getInstance();
	name = Name;
	pluginlisttype = listtype;
	buttonname = LOCALE_MENU_BACK;
	selected = 0;
	liststart = 0;
	if (listtype == CPlugins::P_TYPE_GAME)
		iconfile = NEUTRINO_ICON_GAMES;
	else if (listtype == CPlugins::P_TYPE_SCRIPT)
		iconfile = NEUTRINO_ICON_SHELL;
	else
		iconfile = "";
	ticonwidth = 0;
	ticonheight = 0;
}

CPluginList::~CPluginList()
{
	for(unsigned int count=0;count<pluginlist.size();count++)
		delete pluginlist[count];
}

int CPluginList::exec(CMenuTarget* parent, const std::string & /*actionKey*/)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	//scan4plugins here!
	for(unsigned int count=0;count<pluginlist.size();count++)
	{
		delete pluginlist[count];
	}
	pluginlist.clear();

	pluginitem* tmp = new pluginitem();
	tmp->name = g_Locale->getText(buttonname);
	pluginlist.push_back(tmp);

	for(unsigned int count=0;count < (unsigned int)g_PluginList->getNumberOfPlugins();count++)
	{
		if ((g_PluginList->getType(count) & pluginlisttype) && !g_PluginList->isHidden(count))
		{
			tmp = new pluginitem();
			tmp->number = count;
			tmp->name = g_PluginList->getName(count);
			tmp->desc = g_PluginList->getDescription(count);
			pluginlist.push_back(tmp);
		}
	}
	if (selected >= pluginlist.size())
		selected = pluginlist.size() - 1;

	paint();

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

		if (msg == CRCInput::RC_timeout || msg == CRCInput::RC_left || msg == g_settings.key_channelList_cancel)
		{
			loop=false;
		}
		else if (msg_repeatok == CRCInput::RC_up || msg_repeatok == g_settings.key_channelList_pageup)
		{
			int step = (msg_repeatok == g_settings.key_channelList_pageup) ? listmaxshow : 1;  // browse or step 1
			int new_selected = selected - step;
			if (new_selected < 0)
				new_selected = pluginlist.size() - 1;
			updateSelection(new_selected);
		}
		else if (msg_repeatok == CRCInput::RC_down || msg_repeatok == g_settings.key_channelList_pagedown)
		{
			unsigned int step = (msg_repeatok == g_settings.key_channelList_pagedown) ? listmaxshow : 1;  // browse or step 1
			unsigned int new_selected = selected + step;
			unsigned int p_size = pluginlist.size();
			if (new_selected >= p_size)
			{
				if ((p_size / listmaxshow + 1) * listmaxshow == p_size + listmaxshow) // last page has full entries
					new_selected = 0;
				else
					new_selected = ((step == listmaxshow) && (new_selected < ((p_size / listmaxshow + 1) * listmaxshow))) ? (p_size - 1) : 0;
			}
			updateSelection(new_selected);
		}
		else if (msg == CRCInput::RC_right || msg == CRCInput::RC_ok)
		{
			if(selected==0)
			{
				if (msg == CRCInput::RC_ok)
					loop = false;
			}
			else
			{//exec the plugin :))
				if (pluginSelected() == close)
				{
					loop=false;
				}
			}
		}
		else if (msg == CRCInput::RC_setup)
		{
			res = menu_return::RETURN_EXIT_ALL;
			loop = false;
		}
		else if( (msg== CRCInput::RC_red) ||
				 (msg==CRCInput::RC_green) ||
				 (msg==CRCInput::RC_yellow) ||
				 (msg==CRCInput::RC_blue)  ||
		         (CRCInput::isNumeric(msg)) )
		{
			g_RCInput->postMsg(msg, data);
			loop=false;
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all )
		{
			loop = false;
			res = menu_return::RETURN_EXIT_ALL;
		}
	}
	hide();
	return res;
}

void CPluginList::hide()
{
	int c_rad_mid = RADIUS_MID;
	frameBuffer->paintBackgroundBoxRel(x, y, width + sb_width + SHADOW_OFFSET, height + (c_rad_mid / 3 * 2) + SHADOW_OFFSET);
}

void CPluginList::paintItem(int pos)
{
	int ypos = (y+theight) + pos*fheight;
	int itemheight = fheight;

	int c_rad_small    = 0;
	uint8_t    color   = COL_MENUCONTENT;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;

	if (liststart+pos==selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		c_rad_small = RADIUS_SMALL;
	}

	if(liststart+pos==0)
	{	//back is half-height...
		itemheight = (fheight / 2) + 3;
		frameBuffer->paintBoxRel(x     , ypos + itemheight    , width     , 15, COL_MENUCONTENT_PLUS_0);
		frameBuffer->paintBoxRel(x + 10, ypos + itemheight + 5, width - 20,  1, COL_MENUCONTENT_PLUS_5);
		frameBuffer->paintBoxRel(x + 10, ypos + itemheight + 6, width - 20,  1, COL_MENUCONTENT_PLUS_2);
	}
	else if(liststart==0)
	{
		ypos -= (fheight / 2) - 15;
		if(pos==(int)listmaxshow-1)
			frameBuffer->paintBoxRel(x,ypos+itemheight, width, (fheight / 2)-15, COL_MENUCONTENT_PLUS_0);

	}
	frameBuffer->paintBoxRel(x, ypos, width, itemheight, bgcolor, c_rad_small);


	if(liststart+pos<pluginlist.size())
	{
		pluginitem* actplugin = pluginlist[liststart+pos];
		g_Font[SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMLARGE]->RenderString(x+10, ypos+fheight1+3, width-20, actplugin->name, color, 0, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMSMALL]->RenderString(x+20, ypos+fheight,    width-20, actplugin->desc, color, 0, true); // UTF-8

		if(liststart+pos==selected)
		{
			CLCD::getInstance()->showMenuText(0, actplugin->name.c_str(), -1, true); // UTF-8
			CLCD::getInstance()->showMenuText(1, actplugin->desc.c_str(), -1, true); // UTF-8
		}
	}
}

void CPluginList::paintHead()
{
	frameBuffer->paintBoxRel(x, y, width + sb_width, theight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);

	int iconoffset = 0;
	if (!iconfile.empty())
	{
		frameBuffer->paintIcon(iconfile, x + 8, y + theight / 2 - ticonheight / 2);
		iconoffset = 8 + ticonwidth;
	}
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + iconoffset + 10, y + theight + 2, width - iconoffset - 10, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8		
}

void CPluginList::paint()
{
	int c_rad_mid = RADIUS_MID;

	width = w_max (500, 100);
	height = h_max (526, 50);
	if (!iconfile.empty())
		frameBuffer->getIconSize(iconfile.c_str(), &ticonwidth, &ticonheight);
	theight  = std::max(ticonheight, g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight());
	//
	fheight1 = g_Font[SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMLARGE]->getHeight();
	fheight2 = g_Font[SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMSMALL]->getHeight();
	fheight = fheight1 + fheight2 + 2;
	//
	listmaxshow = (height-theight-0)/fheight;
	if (pluginlist.size() < listmaxshow)
		listmaxshow = pluginlist.size();
	height = theight+0+listmaxshow*fheight; // recalc height
	sb_width = (pluginlist.size() > listmaxshow) ? 15 : 0;
	x = getScreenStartX(width + sb_width);
	y = getScreenStartY(height + (c_rad_mid / 3 * 2));
	liststart = (selected/listmaxshow)*listmaxshow;

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText(name));

	frameBuffer->paintBoxRel(x + SHADOW_OFFSET, y + SHADOW_OFFSET, width + sb_width, height + (c_rad_mid / 3 * 2), COL_MENUCONTENTDARK_PLUS_0, c_rad_mid);
	frameBuffer->paintBoxRel(x, y + height - ((c_rad_mid * 2) + 1) + (c_rad_mid / 3 * 2), width + sb_width, ((c_rad_mid * 2) + 1), COL_MENUCONTENT_PLUS_0, c_rad_mid, CORNER_BOTTOM);

	paintHead();
	paintItems();
}

void CPluginList::paintItems()
{
	if(pluginlist.size() > listmaxshow)
	{
		// Scrollbar
		int nrOfPages = ((pluginlist.size()-1) / listmaxshow)+1; 
		int currPage  = (liststart/listmaxshow) +1;
		frameBuffer->paintBoxRel(x+width, y+theight, 15, height-theight,  COL_MENUCONTENT_PLUS_1);
		frameBuffer->paintBoxRel(x+ width +2, y+theight+2+(currPage-1)*(height-theight-4)/nrOfPages, 11, (height-theight-4)/nrOfPages, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);
	}
	
   for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}
}

void CPluginList::updateSelection(unsigned int newpos)
{
	if (selected != newpos)
	{
		unsigned int prev_selected = selected;
		unsigned int oldliststart = liststart;

		selected = newpos;
		liststart = (selected / listmaxshow) * listmaxshow;
		if (oldliststart != liststart)
			paintItems();
		else
		{
			paintItem(prev_selected - liststart);
			paintItem(selected - liststart);
		}
	}
}

CPluginList::result_ CPluginList::pluginSelected()
{
	g_PluginList->startPlugin(pluginlist[selected]->number);
	hide();
	if (!g_PluginList->getScriptOutput().empty())
	{
		ShowMsgUTF(LOCALE_PLUGINS_RESULT, Latin1_to_UTF8(g_PluginList->getScriptOutput()),
				   CMessageBox::mbrBack,CMessageBox::mbBack,NEUTRINO_ICON_SHELL);
	}
	paint();
	return resume;
}

CPluginChooser::CPluginChooser(const neutrino_locale_t Name, const uint listtype, char* pluginname)
	: CPluginList(Name, listtype), selected_plugin(pluginname)
{
	buttonname = LOCALE_MENU_CANCEL;
}

CPluginList::result_ CPluginChooser::pluginSelected()
{
	strcpy(selected_plugin,g_PluginList->getFileName(pluginlist[selected]->number));
	return CPluginList::close;
}
