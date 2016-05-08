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

#include <gui/widget/keychooser.h>

#include <global.h>
#include <neutrino.h>

#include <driver/screen_max.h>

#include <gui/color.h>


class CKeyValue : public CMenuSeparator
{
public:
	int         keyvalue;

	CKeyValue() : CMenuSeparator(CMenuSeparator::STRING, LOCALE_KEYCHOOSERMENU_CURRENTKEY)
		{
		};

	virtual const char * getString(void)
		{
			separator_text  = g_Locale->getText(text);
			separator_text += ": ";
			separator_text += CRCInput::getKeyName(keyvalue);
			return separator_text.c_str();
		};
};



CKeyChooser::CKeyChooser(neutrino_msg_t * const Key, const neutrino_locale_t title, const std::string & Icon) : CMenuWidget(title, Icon)
{
	frameBuffer = CFrameBuffer::getInstance();
	key = Key;
	keyName = CRCInput::getKeyName(*key);
	keyChooser = new CKeyChooserItem(LOCALE_KEYCHOOSER_HEAD, key);
	keyDeleter = new CKeyChooserItemNoKey(key);
	keyChooserForwarder = new CMenuForwarder(LOCALE_KEYCHOOSERMENU_SETNEW, true, NULL, keyChooser);
	keyDeleterForwarder = new CMenuForwarder(LOCALE_KEYCHOOSERMENU_SETNONE, true, NULL, keyDeleter);
	keyDeleterForwarder->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);

	addItem(new CKeyValue());
	addItem(GenericMenuSeparatorLine);
	addItem(GenericMenuBack);
	addItem(GenericMenuSeparatorLine);
	addItem(keyChooserForwarder);
	addItem(keyDeleterForwarder);
}


CKeyChooser::~CKeyChooser()
{
	delete keyChooser;
	delete keyDeleter;
}


void CKeyChooser::paint()
{
	keyName = CRCInput::getKeyName(*key);
	static_cast<CKeyValue*>(items[0])->keyvalue = *key;

	CMenuWidget::paint();
}

//*****************************
CKeyChooserItem::CKeyChooserItem(const neutrino_locale_t Name, neutrino_msg_t * Key)
{
	name = Name;
	key = Key;
	x = y = width = height = 0;
}


int CKeyChooserItem::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	unsigned long long timeoutEnd;

	int res = menu_return::RETURN_REPAINT;

	if (parent)
		parent->hide();

	paint();

	g_RCInput->clearRCMsg();

	timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

 get_Message:
	g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
	/* wait, until all repeat and release events are over... */
	if (msg <= CRCInput::RC_MaxRC && (msg & (CRCInput::RC_Repeat | CRCInput::RC_Release)))
		goto get_Message;
	
	if (msg != CRCInput::RC_timeout)
	{
		if (msg <= CRCInput::RC_MaxRC)
			*key = msg; // repeat bit is already checked above
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			res = menu_return::RETURN_EXIT_ALL;
		else
			goto get_Message;
	}

	hide();
	return res;
}

void CKeyChooserItem::hide()
{
	CFrameBuffer::getInstance()->paintBackgroundBoxRel(x, y, width, height);
}

void CKeyChooserItem::paint()
{
	int hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	int mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();

	width       = w_max(350, 0);
	height      = h_max(hheight + 2 * mheight, 0);
	x           = getScreenStartX(width);
	y           = getScreenStartY(height);

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	int c_rad = RADIUS_MID;
	frameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD_PLUS_0, c_rad, CORNER_TOP);
	frameBuffer->paintBoxRel(x, y + hheight, width, height - hheight, COL_MENUCONTENT_PLUS_0, c_rad, CORNER_BOTTOM );

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + 10, y + hheight + 2, width, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8

	//paint msg...
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, y+ hheight+ mheight, width, g_Locale->getText(LOCALE_KEYCHOOSER_TEXT1), COL_MENUCONTENT, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, y+ hheight+ mheight* 2, width, g_Locale->getText(LOCALE_KEYCHOOSER_TEXT2), COL_MENUCONTENT, 0, true); // UTF-8
}
