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

#include <gui/bedit/bouqueteditor_channels.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/screen_max.h>
#include <gui/bedit/bouqueteditor_chanselect.h>
#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>

#include <zapit/client/zapitclient.h>


CBEChannelWidget::CBEChannelWidget(const std::string & Caption, unsigned int Bouquet)
{
	frameBuffer = CFrameBuffer::getInstance();

	selected = 0;
	liststart = 0;
	state = beDefault;
	caption = Caption;
	bouquet = Bouquet;
	mode = CZapitClient::MODE_TV;
}

void CBEChannelWidget::paintItem(int pos)
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
	if ((liststart+pos==selected) && (state == beMoving))
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, x + 8, ypos+4);
	}
	if(liststart+pos < Channels.size())
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 5+ numwidth+ 10, ypos+ fheight, width- numwidth- 20- 15, Channels[liststart+pos].name, color);
	}
}

void CBEChannelWidget::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  liststart + listmaxshow;

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

	// paint scrollbar
	int ypos = y+ theight;
	int sb = fheight* listmaxshow;

	frameBuffer->paintBoxRel(x, ypos, width, sb, COL_MENUCONTENT_PLUS_0); //mainframe
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb, COL_MENUCONTENT_PLUS_1);

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int sbc= ((Channels.size()- 1)/ listmaxshow)+ 1;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ sbs*(sb-4)/sbc , 11, (sb-4)/sbc, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);

}

void CBEChannelWidget::paintHead()
{
	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+theight+2, width, caption, COL_MENUHEAD);
}

const struct button_label CBEChannelWidgetButtons[4] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_BOUQUETEDITOR_DELETE     },
	{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_BOUQUETEDITOR_ADD        },
	{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_BOUQUETEDITOR_MOVE       },
	{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_BOUQUETEDITOR_SWITCHMODE }
};

void CBEChannelWidget::paintFoot()
{
	int fy = y + height - ButtonHeight;
	int ButtonWidth = (width - 20) / 4;
	frameBuffer->paintBoxRel(x, fy, width, ButtonHeight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, fy + 2, ButtonWidth, 4, CBEChannelWidgetButtons);
}

void CBEChannelWidget::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height);
}

void CBEChannelWidget::updateSelection(unsigned int newpos)
{
	if (selected != newpos)
	{
		unsigned int prev_selected = selected;
		selected = newpos;

		if (state == beDefault)
		{
			unsigned int oldliststart = liststart;
			liststart = (selected / listmaxshow) * listmaxshow;
			if (oldliststart != liststart)
				paint();
			else
			{
				paintItem(prev_selected - liststart);
				paintItem(selected - liststart);
			}
		}
		else // state == beMoving
		{
			internalMoveChannel(prev_selected, selected);
		}
	}
}

int CBEChannelWidget::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	width  = w_max (550, 0);
	height = h_max (465, 50);
	ButtonHeight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight() + 6;
	theight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight     = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight();
	listmaxshow = (height - theight - ButtonHeight - 0) / fheight;
	height = theight + ButtonHeight + 0 + listmaxshow * fheight; // recalc height
	x = getScreenStartX (width);
	y = getScreenStartY (height);

	if (parent)
	{
		parent->hide();
	}

	Channels.clear();
	g_Zapit->getBouquetChannels( bouquet, Channels, mode);
	paintHead();
	paint();
	paintFoot();

	channelsChanged = false;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);

		if (msg == CRCInput::RC_timeout || msg == g_settings.key_channelList_cancel)
		{
			if (state == beDefault)
			{
				loop = false;
			}
			else if (state == beMoving)
			{
				cancelMoveChannel();
			}
		}
		//
		// -- For more convenience: include browsing of list (paging)  (rasc, 2002-04-02)
		// -- The keys should be configurable. Problem is: red/green key, which is the
		// -- default in neutrino is used as a function key here... so use left/right
		//
		else if (msg_repeatok == CRCInput::RC_up || msg_repeatok == g_settings.key_channelList_pageup)
		{
			if (!(Channels.empty()))
			{
				int step = (msg_repeatok == g_settings.key_channelList_pageup) ? listmaxshow : 1;  // browse or step 1
				int new_selected = selected - step;
				if (new_selected < 0)
					new_selected = Channels.size() - 1;
				updateSelection(new_selected);
			}
		}
		else if (msg_repeatok == CRCInput::RC_down || msg_repeatok == g_settings.key_channelList_pagedown)
		{
			if (!(Channels.empty()))
			{
				unsigned int step = (msg_repeatok == g_settings.key_channelList_pagedown) ? listmaxshow : 1;  // browse or step 1
				unsigned int new_selected = selected + step;
				unsigned int c_size = Channels.size();
				if (new_selected >= c_size)
				{
					if ((c_size / listmaxshow + 1) * listmaxshow == c_size + listmaxshow) // last page has full entries
						new_selected = 0;
					else
						new_selected = ((step == listmaxshow) && (new_selected < ((c_size / listmaxshow + 1) * listmaxshow))) ? (c_size - 1) : 0;
				}
				updateSelection(new_selected);
			}
		}
		else if(msg==CRCInput::RC_red)
		{
			if (state == beDefault)
				deleteChannel();
		}
		else if(msg==CRCInput::RC_green)
		{
			if (state == beDefault)
				addChannel();
		}
		else if(msg==CRCInput::RC_blue)
		{
			if (state == beDefault)
			{
				if (mode == CZapitClient::MODE_TV)
					mode = CZapitClient::MODE_RADIO;
				else
					mode = CZapitClient::MODE_TV;
				Channels.clear();
				g_Zapit->getBouquetChannels( bouquet, Channels, mode);
				selected = 0;
				paint();
			}
		}
		else if(msg==CRCInput::RC_yellow)
		{
			liststart = (selected/listmaxshow)*listmaxshow;
			if (state == beDefault)
				beginMoveChannel();
			paintItem(selected - liststart);
		}
		else if(msg==CRCInput::RC_ok)
		{
			if (state == beDefault)
			{
				if (selected < Channels.size()) /* Channels.size() might be 0 */
					g_Zapit->zapTo_serviceID(Channels[selected].channel_id);

			} else if (state == beMoving) {
				finishMoveChannel();
			}
		}
		else if( CRCInput::isNumeric(msg) )
		{
			if (state == beDefault)
			{
				//kein pushback - wenn man versehentlich wo draufkommt is die edit-arbeit umsonst
				//selected = oldselected;
				//g_RCInput->postMsg( msg, data );
				//loop=false;
			}
			else if (state == beMoving)
			{
				cancelMoveChannel();
			}
		}
		else
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
			// kein canceling...
		}
	}
	hide();
	return res;
}

void CBEChannelWidget::deleteChannel()
{
	if (selected >= Channels.size()) /* Channels.size() might be 0 */
		return;

	g_Zapit->removeChannelFromBouquet(bouquet, Channels[selected].channel_id);
	Channels.clear();
	g_Zapit->getBouquetChannels(bouquet, Channels, mode);
	if (selected >= Channels.size())
		selected = Channels.empty() ? 0 : (Channels.size() - 1);
	channelsChanged = true;
	paint();
}

void CBEChannelWidget::addChannel()
{
	printf("new\n");
	CBEChannelSelectWidget* channelSelectWidget = new CBEChannelSelectWidget(caption, bouquet, mode);
	printf("exec\n");
	channelSelectWidget->exec(this, "");
	printf("exec done\n");
	if (channelSelectWidget->hasChanged())
	{
		channelsChanged = true;
		Channels.clear();
		g_Zapit->getBouquetChannels( bouquet, Channels, mode);
	}
	delete channelSelectWidget;
	paintHead();
	paint();
	paintFoot();
}

void CBEChannelWidget::beginMoveChannel()
{
	state = beMoving;
	origPosition = selected;
	newPosition = selected;
}

void CBEChannelWidget::finishMoveChannel()
{
	state = beDefault;
	if (newPosition != origPosition)
	{
		g_Zapit->moveChannel( bouquet, origPosition+1, newPosition+1, mode);
		Channels.clear();
		g_Zapit->getBouquetChannels( bouquet, Channels, mode);
		channelsChanged = true;
	}
	paint();
}

void CBEChannelWidget::cancelMoveChannel()
{
	state = beDefault;
	internalMoveChannel( newPosition, origPosition);
}

void CBEChannelWidget::internalMoveChannel( unsigned int fromPosition, unsigned int toPosition)
{
	if ( (int) toPosition == -1 ) return;
	if ( toPosition == Channels.size()) return;

	CZapitClient::responseGetBouquetChannels Channel = Channels[fromPosition];
	if (fromPosition < toPosition)
	{
		for (unsigned int i=fromPosition; i<toPosition; i++)
			Channels[i] = Channels[i+1];
	}
	else if (fromPosition > toPosition)
	{
		for (unsigned int i=fromPosition; i>toPosition; i--)
			Channels[i] = Channels[i-1];
	}
	Channels[toPosition] = Channel;
	selected = toPosition;
	newPosition = toPosition;
	paint();
}

bool CBEChannelWidget::hasChanged()
{
	return (channelsChanged);
}
