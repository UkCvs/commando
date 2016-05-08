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

#include <gui/widget/messagebox.h>

#include <gui/widget/icons.h>
#include <driver/framebuffer.h>
#include <global.h>
#include <neutrino.h>


CMessageBox::CMessageBox(const neutrino_locale_t Caption, const char * const Text, const int Width, const char * const Icon, const CMessageBox::result_ &Default, const uint ShowButtons) : CHintBoxExt(Caption, Text, Width, Icon)
{
	returnDefaultOnTimeout = false;

	m_height += (m_fheight << 1);

	result = Default;

	showbuttons = ShowButtons;
}

CMessageBox::CMessageBox(const neutrino_locale_t Caption, ContentLines& Lines, const int Width, const char * const Icon, const CMessageBox::result_ &Default, const uint ShowButtons) : CHintBoxExt(Caption, Lines, Width, Icon)
{
	returnDefaultOnTimeout = false;

	m_height += (m_fheight << 1);

	result = Default;

	showbuttons = ShowButtons;
}

void CMessageBox::returnDefaultValueOnTimeout(bool returnDefault)
{
	returnDefaultOnTimeout = returnDefault;
}

void CMessageBox::paintButtons()
{
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	uint8_t    color;
	fb_pixel_t bgcolor;
	int iconh = 0, iconw = 0;
	int offset = 0;

	m_window->paintBoxRel(0, m_height - (m_fheight << 1), m_width, (m_fheight << 1), (CFBWindow::color_t)COL_MENUCONTENT_PLUS_0, RADIUS_MID, CORNER_BOTTOM);

	//irgendwann alle vergleichen - aber cancel ist sicher der längste
	int MaxButtonTextWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(g_Locale->getText(LOCALE_MESSAGEBOX_CANCEL), true); // UTF-8

	int ButtonWidth = 20 + 33 + MaxButtonTextWidth;

//	int ButtonSpacing = 40;
//	int startpos = (m_width - ((ButtonWidth*3)+(ButtonSpacing*2))) / 2;

	int ButtonSpacing = (m_width- 20- (ButtonWidth*3) ) / 2;

	int xpos = 10;
	int ypos = m_height - m_fheight - 20;
	
	int buttonY_mid = ypos + (m_fheight / 2);
	int ytext_start = buttonY_mid + (g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight() / 2) + 1;
	if (m_fheight & 1) { offset = 1; } // Y-offset for icon

	if (showbuttons & mbYes)
	{
		if (result == mbrYes)
		{
			color   = COL_MENUCONTENTSELECTED;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
		else
		{
			color   = COL_INFOBAR_SHADOW;
			bgcolor = COL_INFOBAR_SHADOW_PLUS_0;
		}
		// get height/width of icon
		frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_RED, &iconw, &iconh);

		m_window->paintBoxRel(xpos, ypos, ButtonWidth, m_fheight, (CFBWindow::color_t)bgcolor, RADIUS_SMALL);
		m_window->paintIcon(NEUTRINO_ICON_BUTTON_RED, xpos + 22 - (iconw / 2), buttonY_mid - (iconh / 2) +  offset);
		m_window->RenderString(g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], xpos + 43, ytext_start, ButtonWidth- 53, g_Locale->getText(LOCALE_MESSAGEBOX_YES), (CFBWindow::color_t)color, 0, true); // UTF-8
	}

	xpos += ButtonWidth + ButtonSpacing;

	if (showbuttons & mbNo)
	{
		if (result == mbrNo)
		{
			color   = COL_MENUCONTENTSELECTED;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
		else
		{
			color   = COL_INFOBAR_SHADOW;
			bgcolor = COL_INFOBAR_SHADOW_PLUS_0;
		}
		// get height/width of icon
		frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_GREEN, &iconw, &iconh);

		m_window->paintBoxRel(xpos, ypos, ButtonWidth, m_fheight, (CFBWindow::color_t)bgcolor, RADIUS_SMALL);
		m_window->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, xpos + 22 - (iconw / 2), buttonY_mid - (iconh / 2) +  offset);
		m_window->RenderString(g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], xpos + 43, ytext_start, ButtonWidth- 53, g_Locale->getText(LOCALE_MESSAGEBOX_NO), (CFBWindow::color_t)color, 0, true); // UTF-8
	}

	xpos += ButtonWidth + ButtonSpacing;

	if (showbuttons & (mbCancel | mbBack))
	{
		if (result >= mbrCancel)
		{
			color   = COL_MENUCONTENTSELECTED;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
		else
		{
			color   = COL_INFOBAR_SHADOW;
			bgcolor = COL_INFOBAR_SHADOW_PLUS_0;
		}
		// get height/width of icon
		frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_HOME, &iconw, &iconh);

		m_window->paintBoxRel(xpos, ypos, ButtonWidth, m_fheight, (CFBWindow::color_t)bgcolor, RADIUS_SMALL);
		m_window->paintIcon(NEUTRINO_ICON_BUTTON_HOME, xpos + 22 - (iconw / 2), buttonY_mid - (iconh / 2) +  offset);
		m_window->RenderString(g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], xpos + 43, ytext_start, ButtonWidth- 53, g_Locale->getText((showbuttons & mbCancel) ? LOCALE_MESSAGEBOX_CANCEL : LOCALE_MESSAGEBOX_BACK), (CFBWindow::color_t)color, 0, true); // UTF-8

	}
}

int CMessageBox::exec(int timeout)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	CHintBoxExt::paint();

	if (m_window == NULL)
	{
		return res; /* out of memory */
	}

	paintButtons();

	if ( timeout == -1 )
		timeout = g_settings.timing[SNeutrinoSettings::TIMING_EPG];

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	bool loop=true;
	while (loop)
	{

		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		if (msg == CRCInput::RC_timeout && returnDefaultOnTimeout)
		{
			// return default 
			loop = false;
		}
		else if (((msg == CRCInput::RC_timeout) ||
			  (msg == g_settings.key_channelList_cancel)) &&
			 (showbuttons & (mbCancel | mbBack)))
		{
			result = (showbuttons & mbCancel) ? mbrCancel : mbrBack;
			loop   = false;
		}
		else if ((msg == CRCInput::RC_green) && (showbuttons & mbNo))
		{
			result = mbrNo;
			loop   = false;
		}
		else if ((msg == CRCInput::RC_red) && (showbuttons & mbYes))
		{
			result = mbrYes;
			loop   = false;
		}
		else if(msg==CRCInput::RC_right)
		{
			bool ok = false;
			while (!ok)
			{
				result = (CMessageBox::result_)((result + 1) & 3);
				ok = showbuttons & (1 << result);
			}

			paintButtons();
		}
		else if (has_scrollbar() && ((msg == CRCInput::RC_up) || (msg == CRCInput::RC_down)))
		{
			if (msg == CRCInput::RC_up)
				scroll_up();
			else
				scroll_down();
				
			paintButtons();
		}
		else if(msg==CRCInput::RC_left)
		{
			bool ok = false;
			while (!ok)
			{
				result = (CMessageBox::result_)((result - 1) & 3);
				ok = showbuttons & (1 << result);
			}

			paintButtons();

		}
		else if(msg == CRCInput::RC_ok)
		{
			loop = false;
		}
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
		{
			res  = menu_return::RETURN_EXIT_ALL;
			loop = false;
		}

	}

	hide();
	
	return res;
}

int ShowMsgUTF(const neutrino_locale_t Caption, const char * const Text, const CMessageBox::result_ &Default, const uint ShowButtons, const char * const Icon, const int Width, const int timeout, bool returnDefaultOnTimeout)
{
   	CMessageBox* messageBox = new CMessageBox(Caption, Text, Width, Icon, Default, ShowButtons);
	messageBox->returnDefaultValueOnTimeout(returnDefaultOnTimeout);
	messageBox->exec(timeout);
	int res = messageBox->result;
	delete messageBox;
	
	return res;
}

int ShowLocalizedMessage(const neutrino_locale_t Caption, const neutrino_locale_t Text, const CMessageBox::result_ &Default, const uint ShowButtons, const char * const Icon, const int Width, const int timeout, bool returnDefaultOnTimeout)
{
	return ShowMsgUTF(Caption, g_Locale->getText(Text), Default, ShowButtons, Icon, Width, timeout,returnDefaultOnTimeout);
}

int ShowMsgUTF(const neutrino_locale_t Caption, const std::string & Text, const CMessageBox::result_ &Default, const uint ShowButtons, const char * const Icon, const int Width, const int timeout, bool returnDefaultOnTimeout)
{
	return ShowMsgUTF(Caption, Text.c_str(), Default, ShowButtons, Icon, Width, timeout,returnDefaultOnTimeout);
}

void DisplayErrorMessage(const char * const ErrorMsg)
{
	ShowMsgUTF(LOCALE_MESSAGEBOX_ERROR, ErrorMsg, CMessageBox::mbrCancel, CMessageBox::mbCancel, NEUTRINO_ICON_ERROR);
}

void DisplayInfoMessage(const char * const ErrorMsg)
{
	ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, ErrorMsg, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);
}
