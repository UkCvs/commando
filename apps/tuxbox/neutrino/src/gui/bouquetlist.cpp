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

#include <gui/bouquetlist.h>

#include <gui/color.h>
#include <gui/eventlist.h>
#include <gui/infoviewer.h>

#include <gui/widget/menue.h>
#include <gui/widget/icons.h>
#include <gui/widget/buttons.h>

#include <driver/fontrenderer.h>
#include <driver/screen_max.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>
#include <system/helper.h>

#include <global.h>
#include <neutrino.h>

CBouquetList::CBouquetList()
{
	frameBuffer = CFrameBuffer::getInstance();
	selected    = 0;
	liststart   = 0;
}

CBouquetList::~CBouquetList()
{
	for (std::vector<CBouquet *>::iterator it = Bouquets.begin(); it != Bouquets.end(); ++it)
		delete (*it);
}

CBouquet* CBouquetList::addBouquet(const char * const name, int BouquetKey, bool locked)
{
	if ( BouquetKey==-1 )
		BouquetKey= Bouquets.size();

	CBouquet* tmp = new CBouquet( BouquetKey, name, locked );
	Bouquets.push_back(tmp);
	return(tmp);
}

int CBouquetList::getActiveBouquetNumber()
{
	return selected;
}

int CBouquetList::showChannelList( int nBouquet)
{
	if (nBouquet < 0 || nBouquet >= (int)Bouquets.size())
		nBouquet = selected;

	int nNewChannel = Bouquets[nBouquet]->channelList->show();
	if (nNewChannel > -1)
	{
		selected = nBouquet;
		orgChannelList->zapTo(Bouquets[selected]->channelList->getKey(nNewChannel)-1);

		nNewChannel= -2; // exit!
	}

	return nNewChannel;
}

void CBouquetList::adjustToChannel( int nChannelNr)
{
	for (uint i=0; i<Bouquets.size(); i++)
	{
		int nChannelPos = Bouquets[i]->channelList->hasChannel(nChannelNr);
		if (nChannelPos > -1)
		{
			selected = i;
			Bouquets[i]->channelList->setSelected(nChannelPos);
			return;
		}
	}
}


int CBouquetList::activateBouquet( int id, bool bShowChannelList)
{
	int res = menu_return::RETURN_REPAINT;

	if (id >= 0 && id < (int)Bouquets.size())
		selected = id;

	if (bShowChannelList)
	{
		int nNewChannel = Bouquets[selected]->channelList->show();

		if (nNewChannel > -1)
		{
			orgChannelList->zapTo(Bouquets[selected]->channelList->getKey(nNewChannel)-1);
		}
		else if ( nNewChannel == -2 )
		{
			// -2 bedeutet EXIT_ALL
			res = menu_return::RETURN_EXIT_ALL;
		}
	}

	return res;
}

int CBouquetList::exec( bool bShowChannelList)
{
    int res= show();

	if ( res > -1)
	{
		return activateBouquet( selected, bShowChannelList );
	}
	else if ( res == -1)
	{
		// -1 bedeutet nur REPAINT
		return menu_return::RETURN_REPAINT;
	}
	else
	{
		// -2 bedeutet EXIT_ALL
		return menu_return::RETURN_EXIT_ALL;
	}

	return res;
}

int CBouquetList::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	width  = w_max (500, 0);
	height = h_max (465, 40);

	theight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight     = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight();
	footHeight  = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight() + 6;
	listmaxshow = (height - theight - footHeight - 0) / fheight;
	height      = theight + footHeight + listmaxshow * fheight; // recalc height
	x = getScreenStartX (width);
	y = getScreenStartY (height);

	if (Bouquets.empty())
	{
		return res;
	}
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText(LOCALE_BOUQUETLIST_HEAD));

	int digits = 1;
	int i= Bouquets.size();
	while ((i= i/10)!=0)
		digits++;

	paintHead();
	paint();

	int oldselected = selected;
	int firstselected = selected+ 1;
	int zapOnExit = false;

	unsigned int chn= 0;
	int pos = digits;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

		if (msg == CRCInput::RC_timeout || msg == g_settings.key_channelList_cancel)
		{
			selected = oldselected;
			loop=false;
		}
		else if (msg_repeatok == CRCInput::RC_up || msg_repeatok == g_settings.key_channelList_pageup)
		{
			int step = (msg_repeatok == g_settings.key_channelList_pageup) ? listmaxshow : 1;  // browse or step 1
			int new_selected = selected - step;
			if (new_selected < 0)
				new_selected = Bouquets.size() - 1;
			updateSelection(new_selected);
		}
		else if (msg_repeatok == CRCInput::RC_down || msg_repeatok == g_settings.key_channelList_pagedown)
		{
			unsigned int step = (msg_repeatok == g_settings.key_channelList_pagedown) ? listmaxshow : 1;  // browse or step 1
			unsigned int new_selected = selected + step;
			unsigned int b_size = Bouquets.size();
			if (new_selected >= b_size)
			{
				if ((b_size / listmaxshow + 1) * listmaxshow == b_size + listmaxshow) // last page has full entries
					new_selected = 0;
				else
					new_selected = ((step == listmaxshow) && (new_selected < ((b_size / listmaxshow + 1) * listmaxshow))) ? (b_size - 1) : 0;
			}
			updateSelection(new_selected);
		}
		else if ( msg == CRCInput::RC_ok )
		{
			zapOnExit = true;
			loop=false;
		}
		else if (CRCInput::isNumeric(msg))
		{
			if (pos == digits)
			{
				if (msg == CRCInput::RC_0)
				{
					chn = firstselected;
					pos = digits;
				}
				else
				{
					chn = CRCInput::getNumericValue(msg);
					pos = 1;
				}
			}
			else
			{
				chn = chn * 10 + CRCInput::getNumericValue(msg);
				pos++;
			}

			if (chn > Bouquets.size())
			{
				chn = firstselected;
				pos = digits;
			}

			unsigned int new_selected = (chn - 1) % Bouquets.size(); // is % necessary (i.e. can firstselected be > Bouquets.size()) ?
			updateSelection(new_selected);
		}
		else if( ( msg == CRCInput::RC_red ) ||
				 ( msg == CRCInput::RC_green ) ||
				 ( msg == CRCInput::RC_yellow ) ||
				 ( msg == CRCInput::RC_blue ) )
		{
			selected = oldselected;
			g_RCInput->postMsg( msg, data );
			loop=false;
		}
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
				res = -2;
			}
		};
	}
	hide();

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);

	if(zapOnExit)
	{
		return (selected);
	}
	else
	{
		return (res);
	}
}

void CBouquetList::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height);
}

void CBouquetList::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int c_rad_small;

	uint8_t    color;
	fb_pixel_t bgcolor;
	if (liststart + pos == selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		c_rad_small = RADIUS_SMALL;
	}
	else
	{
		color   = COL_MENUCONTENT;
		bgcolor = COL_MENUCONTENT_PLUS_0;
		c_rad_small = 0;
	}

	frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, bgcolor, c_rad_small);
	if(liststart+pos<Bouquets.size())
	{
		CBouquet* bouq = Bouquets[liststart+pos];
		//number - for direct jump
		std::string tmp = to_string(liststart+pos+ 1);

		if (liststart + pos == selected)
			CLCD::getInstance()->showMenuText(0, bouq->channelList->getName(), -1, true); // UTF-8

		int numpos = x+5+numwidth- g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(tmp);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(numpos,ypos+fheight, numwidth+5, tmp, color, fheight);

		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, bouq->channelList->getName(), color, 0, true); // UTF-8
	}
}

void CBouquetList::paintHead()
{
	frameBuffer->paintBoxRel(x, y, width, theight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+theight+2, width, g_Locale->getText(LOCALE_BOUQUETLIST_HEAD), COL_MENUHEAD, 0, true); // UTF-8
}

struct button_label CBouquetListButtons[] =
{
	{ NEUTRINO_ICON_BUTTON_OKAY,	LOCALE_BOUQUETLIST_BOUQUETSELECT },
	{ NEUTRINO_ICON_BUTTON_HOME, 	LOCALE_LISTSCROLL_EXIT },
	{ NEUTRINO_ICON_BUTTON_TOP, 	LOCALE_GENERIC_EMPTY },
	{ NEUTRINO_ICON_BUTTON_PLUS, 	LOCALE_GENERIC_EMPTY  },
	{ NEUTRINO_ICON_BUTTON_DOWN, 	LOCALE_GENERIC_EMPTY  },
	{ NEUTRINO_ICON_BUTTON_MINUS, 	LOCALE_GENERIC_EMPTY  }
};

void CBouquetList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  liststart + listmaxshow;
	int ypos = y+ theight;
	int sb = fheight* listmaxshow;

	frameBuffer->paintBoxRel(x, ypos, width, sb, COL_MENUCONTENT_PLUS_0); //mainframe
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb, COL_MENUCONTENT_PLUS_1); //scrollbar

	if(lastnum<10)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0");
	else if(lastnum<100)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("00");
	else if(lastnum<1000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("000");
	else if(lastnum<10000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0000");
	else // if(lastnum<100000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("00000");

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int sbc= ((Bouquets.size()- 1)/ listmaxshow)+ 1;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ sbs*(sb-4)/sbc, 11, (sb-4)/sbc, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);

	//footbar
	int fy = y + height - footHeight;
	int ButtonWith = (width - 8) / 3;
	frameBuffer->paintBoxRel(x, fy, width, footHeight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 4, fy + 2, ButtonWith, 6, CBouquetListButtons);
}

void CBouquetList::updateSelection(unsigned int newpos)
{
	if (selected != newpos)
	{
		unsigned int prev_selected = selected;
		unsigned int oldliststart = liststart;

		selected = newpos;
		liststart = (selected / listmaxshow) * listmaxshow;
		if (oldliststart != liststart)
			paint();
		else
		{
			paintItem(prev_selected - liststart);
			paintItem(selected - liststart);
		}
	}
}
