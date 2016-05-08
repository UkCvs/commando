/*
	$Id: menue.cpp,v 1.206 2012/11/01 19:30:05 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	(C) 2008, 2009 Stefan Seyfried

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

#include <gui/widget/menue.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>

#include <gui/color.h>

#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <system/helper.h>

#include <global.h>
#include <neutrino.h>

#include <cctype>
#include <algorithm>

/* the following generic menu items are integrated into multiple menus at the same time */
CMenuSeparator CGenericMenuSeparator;
CMenuSeparator CGenericMenuSeparatorLine(CMenuSeparator::LINE);
CMenuForwarder CGenericMenuBack(LOCALE_MENU_BACK, true, NULL, NULL, NULL, CRCInput::RC_nokey, NEUTRINO_ICON_BUTTON_LEFT);
CMenuForwarder CGenericMenuCancel(LOCALE_MENU_CANCEL, true, NULL, NULL, NULL, CRCInput::RC_nokey, NEUTRINO_ICON_BUTTON_HOME);
CMenuSeparator * const GenericMenuSeparator = &CGenericMenuSeparator;
CMenuSeparator * const GenericMenuSeparatorLine = &CGenericMenuSeparatorLine;
CMenuForwarder * const GenericMenuBack = &CGenericMenuBack;
CMenuForwarder * const GenericMenuCancel = &CGenericMenuCancel;




void CMenuItem::init(const int X, const int Y, const int DX, const int OFFX)
{
	x    = X;
	y    = Y;
	dx   = DX;
	offx = OFFX;
}

void CMenuItem::setActive(const bool Active)
{
	active = Active;
	/* used gets set by the addItem() function. This is for disabling
	   machine-specific options by just not calling the addItem() function.
	   Without this, the changeNotifiers would become machine-dependent. */
	if (used && x != -1)
		paint();
}

void CMenuItem::setItemButton(const std::string& icon_Name, const bool is_select_button)
{
	if (is_select_button)
		selected_iconName = icon_Name;
	else
		iconName = icon_Name;
}

void CMenuItem::initItemColors(const bool select_mode)
{
	if (select_mode)
	{
		item_color   = COL_MENUCONTENTSELECTED;
		item_bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	else if (!active)
	{
		item_color   = COL_MENUCONTENTINACTIVE;
		item_bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}
	else
	{
		item_color   = COL_MENUCONTENT;
		item_bgcolor = COL_MENUCONTENT_PLUS_0;
	}
}

void CMenuItem::paintItemBackground (const bool /*select_mode*/, const int &item_height)
{
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();
#if 0
	//FIXME what select_mode change here ??
	if (select_mode)
		frameBuffer->paintBoxRel(x, y, dx, item_height, item_bgcolor, RADIUS_SMALL);
	else
#endif
		frameBuffer->paintBoxRel(x, y, dx, item_height, item_bgcolor, RADIUS_SMALL);
}

void CMenuItem::paintItemCaption(const bool select_mode, const int &item_height, const char * left_text, const char * right_text)
{
	bool right_text_is_utf8 = (right_text != NULL) ? isUTF8(right_text, strlen(right_text)) : false;

	if (select_mode)
	{
		CLCD::getInstance()->showMenuText(0, left_text, -1, true); // UTF-8
		if (right_text != NULL)
			CLCD::getInstance()->showMenuText(1, right_text, -1, right_text_is_utf8);
		else
			CLCD::getInstance()->showMenuText(1, "");
	}

	//left text
	int stringstartposName = x + offx + 10;
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName, y + item_height, dx - (stringstartposName - x) - 10, left_text, item_color, 0, true); // UTF-8

	//right text
	if (right_text != NULL)
	{
		int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(right_text, right_text_is_utf8);
		int stringstartposOption = std::max(stringstartposName + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(left_text, true) + 10,
				x + dx - stringwidth - 10); //+ offx
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + item_height, dx - (stringstartposOption - x) - 10, right_text, item_color, 0, right_text_is_utf8);
	}
}

void CMenuItem::prepareItem(const bool select_mode, const int &item_height)
{
	//set colors
	initItemColors(select_mode);

	//paint item background
	paintItemBackground(select_mode, item_height);
}

void CMenuItem::paintItemButton(const bool select_mode, const int &item_height, const std::string &icon_Name)
{
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();
	bool selected = select_mode;
	int height = item_height;
	bool icon_painted = false;

	std::string icon_name = getIconName();
	int icon_w = 0;
	int icon_h = 0;

	if (selected && offx > 0)
	{
		if (!selected_iconName.empty())
			icon_name = selected_iconName;
		else if (icon_name.empty() && !CRCInput::isNumeric(directKey))
			icon_name = icon_Name;
	}
	
	//paint icon
	//get data for marker icon
	int m_icon_w = 0, m_icon_h = 0;
	frameBuffer->getIconSize(NEUTRINO_ICON_RIGHT_MARKER, &m_icon_w, &m_icon_h);
	
	int icon_x = 0;
	int icon_space = x + (offx+10)/2;

	if (!icon_name.empty())
	{
		frameBuffer->getIconSize(icon_name.c_str(), &icon_w, &icon_h);
		
		if (active  && icon_w>0 && icon_h>0)
		{
			icon_x = icon_space - ((icon_w+m_icon_w)/2);
			icon_painted = 	frameBuffer->paintIcon(icon_name, icon_x, y+ ((height/2- icon_h/2)) );
		}
	}

	//paint number 
	int number_w = 0;
	int number_x = 0;
	
	if (CRCInput::isNumeric(directKey) && !icon_painted)
	{
		unsigned char color   = COL_MENUCONTENT;
		if (selected)
			color   = COL_MENUCONTENTSELECTED;
		if (!active)
			color   = COL_MENUCONTENTINACTIVE;

		number_w = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(CRCInput::getKeyName(directKey));
		
		number_x = icon_space - ((number_w+m_icon_w)/2);
		
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(number_x/*x + 15*/, y+ height, height, CRCInput::getKeyName(directKey), color, height);
	}
	
	//paint marker
	if  (selected && directKey != CRCInput::RC_nokey && selected_iconName.empty())
	{
		int icon_offset = 2;
		if (icon_painted)
			icon_offset += icon_x+icon_w;
		else
			icon_offset += number_x + number_w;
		
		frameBuffer->paintIcon(NEUTRINO_ICON_RIGHT_MARKER, icon_offset, y+ ((height/2- m_icon_h/2)) );
	}
}

std::string CMenuItem::getIconName()
{
	std::string icon_name;

	if (!iconName.empty())
	{
		icon_name = iconName;
	}
	else if (CRCInput::isNumeric(directKey))
	{
		if (g_settings.menu_numbers_as_icons)
		{
			icon_name = CRCInput::getKeyName(directKey);
			icon_name += ".raw";
		}
	}
	else
	{
		switch (directKey)
		{
			case CRCInput::RC_red:
				icon_name = NEUTRINO_ICON_BUTTON_RED;
				break;
			case CRCInput::RC_green:
				icon_name = NEUTRINO_ICON_BUTTON_GREEN;
				break;
			case CRCInput::RC_yellow:
				icon_name = NEUTRINO_ICON_BUTTON_YELLOW;
				break;
			case CRCInput::RC_blue:
				icon_name = NEUTRINO_ICON_BUTTON_BLUE;
				break;
			case CRCInput::RC_standby:
				icon_name = NEUTRINO_ICON_BUTTON_POWER;
				break;
			case CRCInput::RC_setup:
				icon_name = NEUTRINO_ICON_BUTTON_DBOX;
				break;
			case CRCInput::RC_help:
				icon_name = NEUTRINO_ICON_BUTTON_HELP;
				break;
		}
	}

	return icon_name;
}


CMenuWidget::CMenuWidget(const neutrino_locale_t Name, const std::string & Icon, const int mwidth, const int mheight)
{
	frameBuffer = CFrameBuffer::getInstance();
	name = Name;
	nameString = g_Locale->getText(Name);
	iconfile = Icon;
	preselected = -1;
	selected = preselected;
	width = w_max(mwidth, 0);
	height = mheight; // height(menu_title)+10+...
	wanted_height=mheight;
	current_page=0;
}

CMenuWidget::CMenuWidget(const char* Name, const std::string & Icon, const int mwidth, const int mheight)
{
	frameBuffer = CFrameBuffer::getInstance();
	name = NONEXISTANT_LOCALE;
	nameString = Name;
	iconfile = Icon;
	preselected = -1;
	selected = preselected;
	width = w_max(mwidth, 0);
	height = mheight; // height(menu_title)+10+...
	wanted_height=mheight;
	current_page=0;
}

CMenuWidget::~CMenuWidget()
{
	for(unsigned int count=0;count<items.size();count++)
	{
		CMenuItem * item = items[count];
		if ((item != GenericMenuSeparator) &&
		    (item != GenericMenuSeparatorLine) &&
		    (item != GenericMenuBack)&&
		    (item != GenericMenuCancel))
			delete item;
	}
}

void CMenuWidget::addItem(CMenuItem* menuItem, const bool defaultselected)
{
	if (menuItem->isSelectable())
	{
		bool isSelected = defaultselected;

		if (preselected != -1)
			isSelected = (preselected == (int)items.size());

		if (isSelected)
			selected = items.size();
	}

	menuItem->isUsed();
	items.push_back(menuItem);
}

void CMenuWidget::resetWidget()
{
	items.clear();
	page_start.clear();
	selected=-1;
}

void CMenuWidget::insertItem(const uint& item_id, CMenuItem* menuItem)
{
	items.insert(items.begin()+item_id, menuItem);
}

void CMenuWidget::removeItem(const uint& item_id)
{
	items.erase(items.begin()+item_id);
}

bool CMenuWidget::hasItem()
{
	return !items.empty();
}

int CMenuWidget::getItemId(CMenuItem* menuItem)
{
	for (uint i= 0; i< items.size(); i++)
	{
		if (items[i] == menuItem)
			return i;
	}
	
	return 0;
}

CMenuItem* CMenuWidget::getItem(const uint& item_id)
{
	for (uint i= 0; i< items.size(); i++)
	{
		if (i == item_id)
			return items[i];
	}
	
	return NULL;
}

std::string CMenuWidget::getName()
{
	if (name != NONEXISTANT_LOCALE)
		nameString = g_Locale->getText(name);
	return nameString;
}

int CMenuWidget::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	// paint() changes the mode...
	CLCD::MODES oldLcdMode = CLCD::getInstance()->getMode();
	std::string oldLcdMenutitle = CLCD::getInstance()->getMenutitle();

	int pos;

	if (parent)
		parent->hide();

	paint();
	int retval = menu_return::RETURN_REPAINT;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

	do {
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

		if (msg <= CRCInput::RC_MaxRC)
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

		bool handled = false;

		for (unsigned int i = 0; i < items.size(); i++)
		{
			CMenuItem* titem = items[i];
			if ((titem->directKey != CRCInput::RC_nokey) &&
			    (titem->directKey == msg))
			{
				if (titem->isSelectable())
				{
					items[selected]->paint(false);
					titem->paint(true);
					selected = i;
					msg = CRCInput::RC_ok;
				}
				else
					// swallow-key...
					handled = true;
				break;
			}
		}

		if (!handled)
		{
			switch (msg)
			{

				case CRCInput::RC_up:
				case CRCInput::RC_up|CRCInput::RC_Repeat:
				case CRCInput::RC_down:
				case CRCInput::RC_down|CRCInput::RC_Repeat:
				{
					//search next / prev selectable item
					for (unsigned int count = 1; count < items.size(); count++)
					{
						if ((msg & ~CRCInput::RC_Repeat) == CRCInput::RC_up)
						{
							pos = selected - count;
							if (pos < 0)
								pos += items.size();
						}
						else
							pos = (selected + count) % items.size();

						if (updateSelection(pos))
							break;
					}
					break;
				}

				case CRCInput::RC_right:
				{
					CMenuItem* sel_item = items[selected];
					if(sel_item == GenericMenuBack || sel_item == GenericMenuCancel)
						break;
				}
				case CRCInput::RC_ok:
					//exec this item...
					if (hasItem())
					{
						CMenuItem* item = items[selected];
						int rv = item->exec(this);
						switch (rv)
						{
							case menu_return::RETURN_EXIT_ALL:
								retval = menu_return::RETURN_EXIT_ALL;
								// TODO: is this fallthrough intentional? --seife
							case menu_return::RETURN_EXIT:
								msg = CRCInput::RC_timeout;
								break;
							case menu_return::RETURN_REPAINT:
								paint();
								break;
						}
					}
					else
						msg = CRCInput::RC_timeout;
					break;

				case CRCInput::RC_left:
				case CRCInput::RC_home:
					msg = CRCInput::RC_timeout;
					break;

				case CRCInput::RC_timeout:
					break;

				//close any menue on dbox-key
				case CRCInput::RC_setup:
					msg = CRCInput::RC_timeout;
					retval = menu_return::RETURN_EXIT_ALL;
					break;

				default:
					// handle configurable keys
					if (msg == g_settings.key_menu_pageup)
					{
						if (current_page)
						{
							pos = page_start[current_page] - 1;
							for (unsigned int count = pos; count > 0; count--)
							{
								if (updateSelection(pos))
									break;
								pos--;
							}
						}
						else
						{
							pos = 0;
							for (unsigned int count = 0; count < items.size(); count++)
							{
								if (updateSelection(pos))
									break;
								pos++;
							}
						}
					}
					else if (msg == g_settings.key_menu_pagedown)
					{
						pos = page_start[current_page + 1];
						if (pos >= (int)items.size())
							pos = items.size() - 1;
						for (unsigned int count = pos; count < items.size(); count++)
						{
							if (updateSelection(pos))
								break;
							pos++;
						}
					}
					else
					{
						if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
						{
							retval = menu_return::RETURN_EXIT_ALL;
							msg = CRCInput::RC_timeout;
						}
					}
			}

			if (msg <= CRCInput::RC_MaxRC)
			{
				// recalculate timeout for RC-keys
				timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);
			}
		}

	}
	while (msg != CRCInput::RC_timeout);

	hide();
	if (CLCD::getInstance()->getMode() != CLCD::MODE_STANDBY)
		CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());

	return retval;
}

void CMenuWidget::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width + sb_width + SHADOW_OFFSET, height + (CORNER_RADIUS_MID / 3 * 2) + SHADOW_OFFSET);

	/* x = -1 is a marker which prevents the item from being painted on setActive changes */
	for (unsigned int count = 0; count < items.size(); count++)
		items[count]->init(-1, 0, 0, 0);
}

void CMenuWidget::paint()
{
	if (name != NONEXISTANT_LOCALE)
		nameString = g_Locale->getText(name);

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, nameString.c_str());

	height = h_max(wanted_height, 0);

	int neededWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(nameString, true); // UTF-8
	if (neededWidth > width - 48)
		width = w_max(neededWidth + 49, 0);

	int hiconheight = 0, hiconoffset = 0;
	if (!iconfile.empty())
	{
		int iconw;
		frameBuffer->getIconSize(iconfile.c_str(), &iconw, &hiconheight);
		hiconoffset = 8 + iconw;
	}

	int hheight = std::max(hiconheight, g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight());

	int itemHeightTotal=0;
	int heightCurrPage=0;
	page_start.clear();
	page_start.push_back(0);
	total_pages=1;
	for (unsigned int i= 0; i< items.size(); i++)
	{
		int item_height=items[i]->getHeight();
		itemHeightTotal+=item_height;
		heightCurrPage+=item_height;
		if(heightCurrPage > (height-hheight))
		{
			page_start.push_back(i);
			total_pages++;
			heightCurrPage=item_height;
		}
	}
	page_start.push_back(items.size());

	iconOffset= 0;
	for (unsigned int i= 0; i< items.size(); i++)
	{
		if ((!(items[i]->iconName.empty())) || CRCInput::isNumeric(items[i]->directKey))
		{
			int iconw, iconh;

			frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_RIGHT, &iconw, &iconh);
			iconOffset += iconw;

			frameBuffer->getIconSize(NEUTRINO_ICON_RIGHT_MARKER, &iconw, &iconh);
			iconOffset += iconw;

			iconOffset += 10;
			break;
		}
	}

	// shrink menu if less items
	if(hheight+itemHeightTotal < height)
		height=hheight+itemHeightTotal;

	sb_width = (total_pages > 1) ? 15 : 0;

	int c_rad_mid = RADIUS_MID;

	x = getScreenStartX(width + sb_width);
	y = getScreenStartY(height + (c_rad_mid / 3 * 2));

	frameBuffer->paintBoxRel(x + SHADOW_OFFSET, y + SHADOW_OFFSET, width + sb_width, height + (c_rad_mid / 3 * 2), COL_MENUCONTENTDARK_PLUS_0, c_rad_mid);
	frameBuffer->paintBoxRel(x, y + height - ((c_rad_mid * 2) + 1) + (c_rad_mid / 3 * 2), width + sb_width, ((c_rad_mid * 2) + 1), COL_MENUCONTENT_PLUS_0, c_rad_mid, CORNER_BOTTOM);
	frameBuffer->paintBoxRel(x, y, width + sb_width, hheight, COL_MENUHEAD_PLUS_0, c_rad_mid, CORNER_TOP);

	if (!iconfile.empty())
		frameBuffer->paintIcon(iconfile, x + 8, y + hheight / 2 - hiconheight / 2);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + hiconoffset + 10, y + hheight + 2, width - hiconoffset - 10, nameString, COL_MENUHEAD, 0, true); // UTF-8

	item_start_y = y+hheight;
	paintItems();
}

void CMenuWidget::paintItems()
{
	int item_height=height-(item_start_y-y);

	//Item not currently on screen
	if (selected >= 0)
	{
		while(selected < page_start[current_page])
			current_page--;
		while(selected >= page_start[current_page + 1])
			current_page++;
	}

	// Scrollbar
	if(total_pages>1)
	{
		frameBuffer->paintBoxRel(x+ width,item_start_y, 15, item_height, COL_MENUCONTENT_PLUS_1);
		frameBuffer->paintBoxRel(x+ width +2, item_start_y+ 2+ current_page*(item_height-4)/total_pages, 11, (item_height-4)/total_pages, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);
	}
	frameBuffer->paintBoxRel(x,item_start_y, width,item_height, COL_MENUCONTENT_PLUS_0);
	int ypos=item_start_y;
	for (int count = 0; count < (int)items.size(); count++)
	{
		CMenuItem* item = items[count];
	
		if ((count >= page_start[current_page]) &&
		    (count < page_start[current_page + 1]))
		{
			item->init(x, ypos, width, iconOffset);
			if( (item->isSelectable()) && (selected==-1) )
			{
				ypos = item->paint(true);
				selected = count;
			}
			else
			{
				ypos = item->paint(selected == count);
			}
		}
		else
		{
			/* x = -1 is a marker which prevents the item from being painted on setActive changes */
			item->init(-1, 0, 0, 0);
		}
	}
}

bool CMenuWidget::updateSelection(int pos)
{
	CMenuItem* item = items[pos];
	if (item->isSelectable())
	{
		if ((pos < page_start[current_page + 1]) &&
		    (pos >= page_start[current_page]))
		{ // Item is currently on screen
			//clear prev. selected
			items[selected]->paint(false);
			//select new
			item->paint(true);
			selected = pos;
		}
		else
		{
			selected = pos;
			paintItems();
		}
		return true;
	}
	return false;
}

/*adds the typical menu intro with optional subhead, separator, back button and separatorline to menu*/
void CMenuWidget::addIntroItems(neutrino_locale_t subhead_text, neutrino_locale_t section_text, int buttontype)
{
	if (subhead_text != NONEXISTANT_LOCALE)
		addItem(new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, subhead_text));

	addItem(GenericMenuSeparator);

	switch (buttontype)
	{
		case BTN_TYPE_BACK:
			addItem(GenericMenuBack);
			break;
		case BTN_TYPE_CANCEL:
			addItem(GenericMenuCancel);
			break;
		default:
			break;
	}

	if (section_text != NONEXISTANT_LOCALE)
		addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, section_text));
	else if (buttontype != BTN_TYPE_NO)
		addItem(GenericMenuSeparatorLine);
}

//-------------------------------------------------------------------------------------------------------------------------------
CMenuOptionNumberChooser::CMenuOptionNumberChooser(const neutrino_locale_t name, int * const OptionValue, const bool Active, const int min_value, const int max_value, const int print_offset, const int special_value, const neutrino_locale_t special_value_name, const char * non_localized_name, CChangeObserver * const Observ, const neutrino_msg_t DirectKey, const std::string & IconName, bool NumericInput)
{
	optionName           = name;
	active               = Active;
	optionValue          = OptionValue;

	lower_bound          = min_value;
	upper_bound          = max_value;

	display_offset       = print_offset;

	localized_value      = special_value;
	localized_value_name = special_value_name;

	optionString         = non_localized_name;

	observ               = Observ;
	directKey            = DirectKey;
	iconName             = IconName;

	numeric_input        = NumericInput;
	numberFormat         = "%d";
}

void CMenuOptionNumberChooser::setNumberFormat(const std::string &format)
{
	numberFormat = format;
}

int CMenuOptionNumberChooser::getHeight(void) const
{
	return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

int CMenuOptionNumberChooser::exec(CMenuTarget* parent)
{
	bool optionValueNotChanged = false;
	bool wantsRepaint = false;
	int ret = menu_return::RETURN_NONE;

	if (numeric_input)
	{
		int size = 0;
		int b = lower_bound;
		if (b < 0)
		{
			size++;
			b = -b;
		}
		if (b < upper_bound)
			b = upper_bound;
		for (; b; b /= 10, size++);
		CIntInput cii(optionName, *optionValue, size, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, NULL, &optionValueNotChanged);
		ret = cii.exec(parent, "");
		if (!optionValueNotChanged)
		{
			if (*optionValue > upper_bound)
				*optionValue = upper_bound;
			else if (*optionValue < lower_bound)
				*optionValue = lower_bound;
		}
	}
	else
	{
		if (((*optionValue) >= upper_bound) || ((*optionValue) < lower_bound))
			*optionValue = lower_bound;
		else
			(*optionValue)++;
		paint(true);
	}

	if (observ && !optionValueNotChanged)
		wantsRepaint = observ->changeNotify(optionName, optionValue);

	if (wantsRepaint)
		ret = menu_return::RETURN_REPAINT;

	return ret;
}

int CMenuOptionNumberChooser::paint(bool selected)
{
	int height = getHeight();

	const char * l_option;
	char option_value[21];
	if ((localized_value_name == NONEXISTANT_LOCALE) || ((*optionValue) != localized_value))
	{
		snprintf(option_value, 21, numberFormat.c_str(), ((*optionValue) + display_offset));
		l_option = option_value;
	}
	else
		l_option = g_Locale->getText(localized_value_name);

	const char * l_optionName = (optionString != NULL) ? optionString : g_Locale->getText(optionName);

	//paint item
	prepareItem(selected, height);

	//paint item icon
	paintItemButton(selected, height, NEUTRINO_ICON_BUTTON_OKAY);

	//paint text
	paintItemCaption(selected, height, l_optionName, l_option);

	return y+height;
}




CMenuOptionChooser::CMenuOptionChooser(const neutrino_locale_t OptionName, int * const OptionValue, const struct keyval * const Options, const unsigned Number_Of_Options, const bool Active, CChangeObserver * const Observ, const neutrino_msg_t DirectKey, const std::string & IconName, bool Pulldown, bool OptionsSort)
{
	optionNameString  = g_Locale->getText(OptionName);
	optionName        = OptionName;
	active            = Active;
	optionValue       = OptionValue;
	number_of_options = Number_Of_Options;
	observ            = Observ;
	directKey         = DirectKey;
	iconName          = IconName;
	pulldown          = Pulldown;
	optionsSort       = OptionsSort;
	for (unsigned int i = 0; i < number_of_options; i++)
		options.push_back(&Options[i]);
}

CMenuOptionChooser::CMenuOptionChooser(const char* OptionName, int * const OptionValue, const struct keyval * const Options, const unsigned Number_Of_Options, const bool Active, CChangeObserver * const Observ, const neutrino_msg_t DirectKey, const std::string & IconName, bool Pulldown, bool OptionsSort)
{
	optionNameString  = OptionName;
	optionName        = NONEXISTANT_LOCALE;
	active            = Active;
	optionValue       = OptionValue;
	number_of_options = Number_Of_Options;
	observ            = Observ;
	directKey         = DirectKey;
	iconName          = IconName;
	pulldown          = Pulldown;
	optionsSort       = OptionsSort;
	for (unsigned int i = 0; i < number_of_options; i++)
		options.push_back(&Options[i]);
}

int CMenuOptionChooser::getHeight(void) const
{
	return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

void CMenuOptionChooser::setOptionValue(const int newvalue)
{
	*optionValue = newvalue;
}

int CMenuOptionChooser::getOptionValue(void) const
{
	return *optionValue;
}

static bool sortByOptionValue(const CMenuOptionChooser::keyval * a, const CMenuOptionChooser::keyval * b)
{
	return (strcasecmp(g_Locale->getText(a->value), g_Locale->getText(b->value)) < 0);
}

int CMenuOptionChooser::exec(CMenuTarget* parent)
{
	bool optionValueChanged = true;
	bool wantsRepaint = false;
	int ret = menu_return::RETURN_NONE;

	if (optionsSort)
		sort(options.begin(), options.end(), sortByOptionValue);

	if (pulldown)
	{
		int select = -1;
		CMenuWidget* menu = new CMenuWidget(optionNameString.c_str(), "", dx);
		menu->addIntroItems(NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, CMenuWidget::BTN_TYPE_CANCEL);
		CMenuSelectorTarget * selector = new CMenuSelectorTarget(&select);
		for(unsigned int count = 0; count < number_of_options; count++)
		{
			bool selected = false;
			if (options[count]->key == (*optionValue))
				selected = true;
			CMenuForwarder *mn_option = new CMenuForwarder(options[count]->value, true, NULL, selector, to_string(count).c_str());
			mn_option->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true /*for selected item*/);
			menu->addItem(mn_option, selected);
		}
		ret = menu->exec(parent, "");
		if (select >= 0)
			*optionValue = options[select]->key;
		else
			optionValueChanged = false;
		delete menu;
		delete selector;
	}
	else
	{
		//select next value
		unsigned int count;
		for(count = 0; count < number_of_options; count++)
		{
			if (options[count]->key == (*optionValue))
			{
				*optionValue = options[(count+1) % number_of_options]->key;
				break;
			}
		}
		// if options are removed optionValue may not exist anymore -> use 1st available option
		if ((count == number_of_options) && number_of_options)
			*optionValue = options[0]->key;
		paint(true);
	}

	if (observ && optionValueChanged)
		wantsRepaint = observ->changeNotify(optionName, optionValue);

	if (wantsRepaint)
		ret = menu_return::RETURN_REPAINT;

	return ret;
}

int CMenuOptionChooser::paint( bool selected )
{
	int height = getHeight();

	if (optionName != NONEXISTANT_LOCALE)
		optionNameString = g_Locale->getText(optionName);

	neutrino_locale_t option = NONEXISTANT_LOCALE;
	for(unsigned int count = 0 ; count < number_of_options; count++)
	{
		if (options[count]->key == *optionValue)
		{
			option = options[count]->value;
			break;
		}
	}
	const char * l_option = g_Locale->getText(option);

	//paint item
	prepareItem(selected, height);

	//paint item icon
	paintItemButton(selected, height, NEUTRINO_ICON_BUTTON_OKAY);

	//paint text
	paintItemCaption(selected, height, optionNameString.c_str(), l_option);

	return y+height;
}


//-------------------------------------------------------------------------------------------------------------------------------

CMenuOptionStringChooser::CMenuOptionStringChooser(const neutrino_locale_t OptionName, char* OptionValue, bool Active, CChangeObserver* Observ, const neutrino_msg_t DirectKey, const std::string & IconName, bool Pulldown, bool OptionsSort)
{
	optionName  = OptionName;
	active      = Active;
	optionValue = OptionValue;
	observ      = Observ;
	directKey   = DirectKey;
	iconName    = IconName;
	pulldown    = Pulldown;
	optionsSort = OptionsSort;
}

int CMenuOptionStringChooser::getHeight(void) const
{
	return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

void CMenuOptionStringChooser::removeOptions()
{
	options.clear();
}

CMenuOptionStringChooser::~CMenuOptionStringChooser()
{

}

void CMenuOptionStringChooser::addOption(const char * const value)
{
	options.push_back(std::string(value));
}

static bool sortByOptionStringValue(const std::string & a, const std::string & b)
{
	return (strcasecmp(a.c_str(), b.c_str()) < 0);
}

int CMenuOptionStringChooser::exec(CMenuTarget* parent)
{
	bool optionValueChanged = true;
	bool wantsRepaint = false;
	int ret = menu_return::RETURN_NONE;

	if (optionsSort)
		sort(options.begin(), options.end(), sortByOptionStringValue);

	if (pulldown)
	{
		int select = -1;
		CMenuWidget* menu = new CMenuWidget(optionName, "", dx);
		menu->addIntroItems(NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, CMenuWidget::BTN_TYPE_CANCEL);
		CMenuSelectorTarget * selector = new CMenuSelectorTarget(&select);
		for (unsigned int count = 0; count < options.size(); count++)
		{
			bool selected = false;
			if (strcmp(options[count].c_str(), optionValue) == 0)
				selected = true;
			CMenuForwarder *mn_option = new CMenuForwarder(options[count].c_str(), true, NULL, selector, to_string(count).c_str());
			mn_option->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true /*for selected item*/);
			menu->addItem(mn_option, selected);
		}
		ret = menu->exec(parent, "");
		if (select >= 0)
			strcpy(optionValue, options[select].c_str());
		else
			optionValueChanged = false;
		delete menu;
		delete selector;
	}
	else
	{
		//select next value
		unsigned int count;
		for(count = 0; count < options.size(); count++)
		{
			if ((strcmp(options[count].c_str(), optionValue) == 0) || (optionValue[0] == '\0'))
			{
				strcpy(optionValue, options[(count + 1) % options.size()].c_str());
				break;
			}
		}
		// if options are removed optionValue may not exist anymore -> use 1st available option
		if ((count == options.size()) && !options.empty())
			strcpy(optionValue, options[0].c_str());
		paint(true);
	}

	if (observ && optionValueChanged)
		wantsRepaint = observ->changeNotify(optionName, optionValue);

	if (wantsRepaint)
		ret = menu_return::RETURN_REPAINT;

	return ret;
}

int CMenuOptionStringChooser::paint( bool selected )
{
	int height = getHeight();
	const char * l_optionName = g_Locale->getText(optionName);

	//paint item
	prepareItem(selected, height);

	//paint item icon
	paintItemButton(selected, height, NEUTRINO_ICON_BUTTON_OKAY);

	//paint text
	paintItemCaption(selected, height, l_optionName, optionValue);

	return y+height;
}


//-------------------------------------------------------------------------------------------------------------------------------
CMenuForwarder::CMenuForwarder(const neutrino_locale_t Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = Option;
	option_string = NULL;
	text=Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
}

CMenuForwarder::CMenuForwarder(const neutrino_locale_t Text, const bool Active, const std::string &Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = NULL;
	option_string = &Option;
	text=Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
}

CMenuForwarder::CMenuForwarder(const char * const Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = Option;
	option_string = NULL;
	text = NONEXISTANT_LOCALE;
	forwarder_text = Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
}

CMenuForwarder::CMenuForwarder(const char * const Text, const bool Active, const std::string &Option, CMenuTarget* Target, const char * const ActionKey, neutrino_msg_t DirectKey, const char * const IconName)
{
	option = NULL;
	option_string = &Option;
	text = NONEXISTANT_LOCALE;
	forwarder_text = Text;
	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
}

int CMenuForwarder::getHeight(void) const
{
	return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

// used gets set by the addItem() function. This is for set to paint Option string by just not calling the addItem() function.
// Without this, the changeNotifiers would become machine-dependent.
void CMenuForwarder::setOption(const char *Option)
{
	option = Option;

	if (used && x != -1)
		paint();
}

// used gets set by the addItem() function. This is for set to paint Text from locales by just not calling the addItem() function.
// Without this, the changeNotifiers would become machine-dependent.
void CMenuForwarder::setTextLocale(const neutrino_locale_t Text)
{
	text=Text;

	if (used && x != -1)
		paint();
}

// used gets set by the addItem() function. This is for set to paint non localized Text by just not calling the addItem() function.
// Without this, the changeNotifiers would become machine-dependent.
void CMenuForwarder::setText(const char * const Text)
{
	forwarder_text = Text;

	if (used && x != -1)
		paint();
}

int CMenuForwarder::exec(CMenuTarget* parent)
{
	if (jumpTarget)
		return jumpTarget->exec(parent, actionKey);
	return menu_return::RETURN_EXIT;
}

const char * CMenuForwarder::getOption(void)
{
	if (option)
		return option;
	if (option_string)
		return option_string->c_str();
	if (jumpTarget)
		return jumpTarget->getTargetValue();
	return NULL;
}

const char * CMenuForwarder::getName(void)
{
	if (text != NONEXISTANT_LOCALE)
		return g_Locale->getText(text);
	return forwarder_text.c_str();
}

int CMenuForwarder::paint(bool selected)
{
	int height = getHeight();
	const char * l_text = getName();
	const char * option_text = getOption();

	//paint item
	prepareItem(selected, height);

	//paint item icon
	paintItemButton(selected, height);

	//paint text
	paintItemCaption(selected, height, l_text, option_text);

	return y+ height;
}


//-------------------------------------------------------------------------------------------------------------------------------
CMenuSeparator::CMenuSeparator(const int Type, const neutrino_locale_t Text)
{
	directKey = CRCInput::RC_nokey;
	iconName = "";
	type     = Type;
	text     = Text;
}

int CMenuSeparator::getHeight(void) const
{
	if (separator_text.empty() && text == NONEXISTANT_LOCALE)
		return 10;
	return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
}

const char * CMenuSeparator::getString(void)
{
	if (!separator_text.empty())
		return separator_text.c_str();
	return g_Locale->getText(text);
}

void CMenuSeparator::setString(const std::string& s_text)
{
	separator_text = s_text;
}


int CMenuSeparator::paint(bool /*selected*/)
{
	int height = getHeight();
	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	
	if ((type & SUB_HEAD))
	{
		item_color   = COL_MENUHEAD;
		item_bgcolor = COL_MENUHEAD_PLUS_0;
	}
	else
	{
		item_color   = COL_MENUCONTENTINACTIVE;
		item_bgcolor = COL_MENUCONTENT_PLUS_0;
	}

	frameBuffer->paintBoxRel(x,y, dx, height, item_bgcolor);
	if ((type & LINE))
	{
		frameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1), COL_MENUCONTENT_PLUS_3);
		frameBuffer->paintHLineRel(x+10,dx-20,y+(height>>1)+1, COL_MENUCONTENT_PLUS_1);
	}
	if ((type & STRING))
	{
		const char * l_text = getString();
	
		if (text != NONEXISTANT_LOCALE || strlen(l_text) != 0)
		{
			int stringstartposX;
			
			int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true); // UTF-8

			/* if no alignment is specified, align centered */
			if (type & ALIGN_LEFT)
				stringstartposX = x + (!(type & SUB_HEAD) ? 20 : offx + 10);
			else if (type & ALIGN_RIGHT)
				stringstartposX = x + dx - stringwidth - 20;
			else /* ALIGN_CENTER */
				stringstartposX = x + (dx >> 1) - (stringwidth >> 1);

			if (type & LINE)
				frameBuffer->paintBoxRel(stringstartposX - 5, y, stringwidth + 10, height, item_bgcolor);

			g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y + height, dx - (stringstartposX - x) - 10, l_text, item_color, 0, true); // UTF-8

			if (type & SUB_HEAD)
				CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, l_text);
		}
	}

	return y+ height;
}

bool CPINProtection::check()
{
	char cPIN[4];
	neutrino_locale_t hint = NONEXISTANT_LOCALE;
	do
	{
		cPIN[0] = 0;
		CPINInput* PINInput = new CPINInput(LOCALE_PINPROTECTION_HEAD, cPIN, 4, hint);
		PINInput->exec( getParent(), "");
		delete PINInput;
		hint = LOCALE_PINPROTECTION_WRONGCODE;
	} while ((strncmp(cPIN,validPIN,4) != 0) && (cPIN[0] != 0));
	return ( strncmp(cPIN,validPIN,4) == 0);
}


bool CZapProtection::check()
{

	int res;
	char cPIN[5];
	neutrino_locale_t hint2 = NONEXISTANT_LOCALE;
	do
	{
		cPIN[0] = 0;

		CPLPINInput* PINInput = new CPLPINInput(LOCALE_PARENTALLOCK_HEAD, cPIN, 4, hint2, fsk);

		res = PINInput->exec(getParent(), "");
		delete PINInput;

		hint2 = LOCALE_PINPROTECTION_WRONGCODE;
	} while ( (strncmp(cPIN,validPIN,4) != 0) &&
		  (cPIN[0] != 0) &&
		  ( res == menu_return::RETURN_REPAINT ) &&
		  ( fsk >= g_settings.parentallock_lockage ) );
	return ( ( strncmp(cPIN,validPIN,4) == 0 ) ||
			 ( fsk < g_settings.parentallock_lockage ) );
}

int CLockedMenuForwarder::exec(CMenuTarget* parent)
{
	Parent = parent;

	if (Ask && !check())
	{
		Parent = NULL;
		return menu_return::RETURN_REPAINT;
	}

	Parent = NULL;
	return CMenuForwarder::exec(parent);
}


int CMenuSelectorTarget::exec(CMenuTarget* /*parent*/, const std::string & actionKey)
{
//	printf("CMenuSelector: %s\n", actionKey.c_str());
	if (!actionKey.empty())
		*m_select = atoi(actionKey.c_str());
	else
		*m_select = -1;
	return menu_return::RETURN_EXIT;
}
