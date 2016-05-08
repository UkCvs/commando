/*
        $Id: personalize.cpp,v 1.48 2012/05/16 21:38:57 rhabarber1848 Exp $

        Customization Menu - Neutrino-GUI

        Copyright (C) 2007 Speed2206
        and some other guys
        
        Reworked by dbt (Thilo Graf)
        Copyright (C) 2010 dbt

        Comment:

        This is the customization menu, as originally showcased in
        Oxygen. It is a more advanced version of the 'user levels'
        patch currently available.
        
        The reworked version >1.24 works more dynamicly with input objects
        and their parameters and it's more code reduced. It's also independent
        from #ifdefs of items. 
        The personalize-object collects all incomming forwarder item objects.
        These will be handled here and will be shown after evaluation.


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
        
        
        Parameters:
	addItem(CMenuWidget *widget, CMenuItem *menuItem, const int *personalize_mode, const bool defaultselected, const int item_mode, const bool *item_active),
	addItem(const int& widget_id, CMenuItem *menuItem, const int *personalize_mode, const bool defaultselected, const int item_mode, const bool *item_active),
	
	CMenuWidget *widget 		= pointer to menue widget object, also to get with 'getWidget(const int& id)'
	const int& widget_id		= index of widget (overloaded), this index is definied in vector 'v_widget', to get with 'getWidgetId()' 
	CMenuItem *menuItem		= pointer to a menuitem object, can be forwarders, locked forwarders and separators...NO CHOOSERS!
	const int *personalize_mode	= optional, default NULL, pointer to a specified personalize setting look at: PERSONALIZE_MODE, this regulates the personalize mode
	const int item_mode		= optional, default PERSONALIZE_SHOW_AS_ITEM_OPTION, if you don't want to see this item in personalize menue, then set it to PERSONALIZE_SHOW_NO
	const bool *item_active		= optional, default NULL, pointer to a variable to control visibility of this item from outside
	
	Icon handling:
	If you define an icon in the item object, this will be shown in the personalized menu but not the personilazitions menue itself, otherwise a shortcut will be create
	
	Shortcuts (optional): default is set to '1':
	A default start-shortcut you can create with foo->setShortcut(), 
	this sets default value '1', e.g.foo->setShortcut(0) sets value '0'
	Only values 0-9 are possible, others will be ignored!
	
	Separators:
	Add separators with
	addSeparator(CMenuWidget &widget, const neutrino_locale_t locale_text, const bool item_mode)
	OR
	addSeparator(const int& widget_id, const neutrino_locale_t locale_text, const bool item_mode)
	
		Parameters:
		CMenuWidget &widget 			= rev to menue widget object
		const int& widget_id			= index of widget (overloaded), this index is definied in vector 'v_widget', to get with 'getWidgetId(widget_object)'
		
		const neutrino_locale_t locale_text	= optional, default NONEXISTANT_LOCALE, adds a line separator, is defined a locale then adds a text separator
		const bool item_mode			= optional, default true, if you don't want to see this sparator also in personalize menue, then set it to false, usefull for to much separtors ;-)
		
	Usage:
	It's possible to personalize only forwarders, locked forwarders and separators!

	Example:
	//we need an instance of CPersonalizeGUI()
	foo = CPersonalizeGui::getInstance();

	//do you need a start shortcut !=1 then set a start number for shortcuts with
	foo->setShortcut(0...9);

	//create a menue widget object, this will be automaticly shown as menu item in your peronalize menu
	CMenuWidget * mn =  new CMenuWidget(LOCALE_MAINMENU_HEAD, ICON    ,width);
	OR
	create a widget struct:
	const mn_widget_struct_t menu_widgets[count of available widgets] =
	{
		{LOCALE_1, 	NEUTRINO_ICON_1, 	width1},
		{LOCALE_2, 	NEUTRINO_ICON_2, 	width2},
		{LOCALE_3,	NEUTRINO_ICON_3, 	width3}, 
	};
	
	//add it to widget collection as single
	foo->addWidget(mn);
	OR as struct
	foo->addWidgets(widget_struct, count of available widgets);

	//create a forwarder object:
	CMenuItem *item = new CMenuForwarder(LOCALE_MAINMENU_TVMODE, true, NULL, this, "tv", CRCInput::RC_red);

	//now you can add this to personalization
	foo->addItem(&mn, tvswitch, &g_settings.personalize_tvmode);
	OR with widget id
	foo->addItem(0, tvswitch, &g_settings.personalize_tvmode);
	
	//if you want to add a non personalized separator use this function, you must use this anstead addItem(GenericMenuSeparatorLine)  
	foo->addSeparator(mn);
	OR with widget id
	foo->addSeparator(0);
	//otherwise you can add a separator at this kind:
	foo->addItem(&mn, GenericMenuSeparatorLine);
	OR with widget id
	foo->addItem(0, GenericMenuSeparatorLine);

	//finally add the menue items
	foo->addPersonalizedItems();
	//this member makes the same like mn->addItem(...) known from CMenuWidget()-class for all collected and evaluated objects
	
	//reset shortcuts:
	foo->setShortcut();
	
	Enums:
	PERSONALIZE_MODE: use as parameter 'personalize_mode'
		PERSONALIZE_MODE_NOTVISIBLE 	: not visible in your personalized menue
		PERSONALIZE_MODE_VISIBLE	: visible in your personalized menue
		PERSONALIZE_MODE_PIN		: visible in your personalized menue with PIN access
		
	PERSONALIZE_PROTECT_MODE: used also as parameter 'personalize_mode'
		PROTECT_MODE_NOT_PROTECTED	: visible in personalize settings menue with PIN setup, option 'no'
		PROTECT_MODE_PIN_PROTECTED	: visible in personalize settings menue with PIN setup, option 'yes'
		
	PERSONALIZE_ITEM_MODE: use as as parameter 'item_mode items in personalize settings menu 
		PERSONALIZE_SHOW_NO		: dont'show this item in personalize settings menu
		PERSONALIZE_SHOW_AS_ITEM_OPTION	: show as item with options 'visible, not visible or PIN'
		PERSONALIZE_SHOW_AS_ACCESS_OPTION: show as item with options 'PIN' with 'yes' or 'no'
		PERSONALIZE_SHOW_ONLY_IN_PERSONALIZE_MENU :usefull to hide separators in menu, but visible only in personalizing menu
	

*/

#include <global.h>
#include <neutrino.h>

#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>
#include <daemonc/remotecontrol.h>
#include <gui/widget/helpbox.h>
#include "widget/messagebox.h"
#include "widget/hintbox.h"
#include "widget/lcdcontroler.h"
#include "widget/keychooser.h"
#include "widget/stringinput.h"
#include "widget/stringinput_ext.h"
#include "color.h"
#include "personalize.h"
#include "user_menue_setup.h"

#define PERSONALIZE_STD_OPTION_COUNT 3
#define PERSONALIZE_EDP_OPTION_COUNT 3
#define PERSONALIZE_EOD_OPTION_COUNT 2
#define PERSONALIZE_YON_OPTION_COUNT 2

using namespace std;

const CMenuOptionChooser::keyval PERSONALIZE_STD_OPTIONS[PERSONALIZE_STD_OPTION_COUNT] =
{
	{ CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE, LOCALE_PERSONALIZE_NOTVISIBLE   },  // The option is NOT visible on the menu's
	{ CPersonalizeGui::PERSONALIZE_MODE_VISIBLE   , LOCALE_PERSONALIZE_VISIBLE      },  // The option is visible on the menu's
	{ CPersonalizeGui::PERSONALIZE_MODE_PIN       , LOCALE_PERSONALIZE_PIN          }   // PIN Protect the item on the menu
};

const CMenuOptionChooser::keyval PERSONALIZE_EDP_OPTIONS[PERSONALIZE_EDP_OPTION_COUNT] =
{
	{ CPersonalizeGui::PERSONALIZE_MODE_DISABLED  , LOCALE_PERSONALIZE_DISABLED     },  // The menu is NOT enabled / accessible
	{ CPersonalizeGui::PERSONALIZE_MODE_ENABLED   , LOCALE_PERSONALIZE_ENABLED      },  // The menu is enabled / accessible
	{ CPersonalizeGui::PERSONALIZE_MODE_PIN       , LOCALE_PERSONALIZE_PIN          }   // The menu is enabled and protected with PIN
};

const CMenuOptionChooser::keyval PERSONALIZE_EOD_OPTIONS[PERSONALIZE_EOD_OPTION_COUNT] =
{
	{ CPersonalizeGui::PERSONALIZE_MODE_DISABLED  , LOCALE_PERSONALIZE_DISABLED     },  // The option is NOT enabled / accessible
	{ CPersonalizeGui::PERSONALIZE_MODE_ENABLED   , LOCALE_PERSONALIZE_ENABLED      }   // The option is enabled / accessible
};

const CMenuOptionChooser::keyval PERSONALIZE_YON_OPTIONS[PERSONALIZE_YON_OPTION_COUNT] =
{
	{ CPersonalizeGui::PROTECT_MODE_NOT_PROTECTED , LOCALE_PERSONALIZE_NOTPROTECTED },  // The menu/option is NOT protected
	{ CPersonalizeGui::PROTECT_MODE_PIN_PROTECTED , LOCALE_PERSONALIZE_PINPROTECT   }   // The menu/option is protected by a PIN
};



CPersonalizeGui::CPersonalizeGui()
{
	width 	= w_max (710, 100);
	widget_count = 0;
	selected = -1;
	shortcut = 1;
}

CPersonalizeGui* CPersonalizeGui::getInstance()
{
	static CPersonalizeGui* p = NULL;

	if(!p)
	{
		p = new CPersonalizeGui();
		printf("[neutrino] GUI-Personalize instance created...\n");
	}
	return p;
}

CPersonalizeGui::~CPersonalizeGui()
{

}

int CPersonalizeGui::exec(CMenuTarget* parent, const string & actionKey)
{
	int res = menu_return::RETURN_REPAINT;

	if (parent)
		parent->hide();

	for (int i = 0; i<(widget_count); i++)
	{
		ostringstream i_str;
		i_str << i;
		string s(i_str.str());
		string a_key = s;
		
		if(actionKey == a_key) 
		{                                     				// Personalize options menu
			res = ShowMenuOptions(i);
			return res;
		}
	}
		
	if (actionKey=="personalize_help") {                                    // Personalize help
		ShowHelpPersonalize();
		return res;
	}
	
	res = ShowPersonalizationMenu();                                        // Show main Personalization Menu
	SaveAndExit();
	return res;
}


//This is the main personalization menu. From here we can go to the other sub-menu's and enable/disable
//the PIN code feature, as well as determine whether or not the EPG menu/Features menu is accessible.
int CPersonalizeGui::ShowPersonalizationMenu()
{
	CMenuWidget* pMenu = new CMenuWidget(LOCALE_PERSONALIZE_HEAD,NEUTRINO_ICON_PROTECTING, width);
	pMenu->setPreselected(selected);

	CPINChangeWidget pinChangeWidget(LOCALE_PERSONALIZE_PINCODE, g_settings.personalize_pincode, 4, LOCALE_PERSONALIZE_PINHINT);

	pMenu->addIntroItems(NONEXISTANT_LOCALE, LOCALE_PERSONALIZE_MENUCONFIGURATION);
	
	pMenu->addItem(new CMenuForwarder(LOCALE_PERSONALIZE_PINCODE, true, g_settings.personalize_pincode, &pinChangeWidget));
	pMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_PERSONALIZE_ACCESS));
	
	CMenuForwarder *p_mn[widget_count];
	string mn_name;
	
 	for (int i = 0; i<(widget_count); i++)
	{
		ostringstream i_str;
		i_str << i;
		string s(i_str.str());
		string action_key = s;
		mn_name = v_widget[i]->getName();
		p_mn[i] = new CMenuForwarder(mn_name.c_str(), true, NULL, this, action_key.c_str(), CRCInput::convertDigitToKey(i+1));
 		pMenu->addItem(p_mn[i]);
	}

	pMenu->addItem(GenericMenuSeparatorLine);
	pMenu->addItem(new CMenuOptionChooser(LOCALE_INFOVIEWER_STREAMINFO, (int *)&g_settings.personalize_bluebutton, PERSONALIZE_EOD_OPTIONS, PERSONALIZE_EOD_OPTION_COUNT, true));
	pMenu->addItem(new CMenuOptionChooser(LOCALE_INFOVIEWER_EVENTLIST, (int *)&g_settings.personalize_redbutton, PERSONALIZE_EOD_OPTIONS, PERSONALIZE_EOD_OPTION_COUNT, true));

	CUserMenuSetup* userMenuSetupRed = new CUserMenuSetup(LOCALE_USERMENU_BUTTON_RED, SNeutrinoSettings::BUTTON_RED);
	CUserMenuSetup* userMenuSetupGreen = new CUserMenuSetup(LOCALE_USERMENU_BUTTON_GREEN, SNeutrinoSettings::BUTTON_GREEN);
	CUserMenuSetup* userMenuSetupYellow = new CUserMenuSetup(LOCALE_USERMENU_BUTTON_YELLOW, SNeutrinoSettings::BUTTON_YELLOW);
	CUserMenuSetup* userMenuSetupBlue = new CUserMenuSetup(LOCALE_USERMENU_BUTTON_BLUE, SNeutrinoSettings::BUTTON_BLUE);

 	pMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_USERMENU_HEAD));
	pMenu->addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_RED, true, NULL, userMenuSetupRed, NULL, CRCInput::RC_red));
	pMenu->addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_GREEN, true, NULL, userMenuSetupGreen, NULL, CRCInput::RC_green));
	pMenu->addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_YELLOW, true, NULL, userMenuSetupYellow, NULL, CRCInput::RC_yellow));
	pMenu->addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_BLUE, true, NULL, userMenuSetupBlue, NULL, CRCInput::RC_blue));

	pMenu->addItem(GenericMenuSeparatorLine);
	pMenu->addItem(new CMenuForwarder(LOCALE_PERSONALIZE_HELP, true, NULL, this, "personalize_help", CRCInput::RC_help));

	int res = pMenu->exec(NULL, "");
	selected = pMenu->getSelected();
	delete pMenu;

	delete userMenuSetupRed;
	delete userMenuSetupGreen;
	delete userMenuSetupYellow;
	delete userMenuSetupBlue;

	return res;
}

//Here we give the user the option to enable, disable, or PIN protect items on the Main Menu.
//We also provide a means of PIN protecting the menu itself.
int CPersonalizeGui::ShowMenuOptions(const int& widget)
{
	string mn_name = v_widget[widget]->getName();
	printf("[neutrino-personalize] exec %s...\n", __FUNCTION__);

	CMenuWidget* pm = new CMenuWidget(LOCALE_PERSONALIZE_HEAD, NEUTRINO_ICON_PROTECTING, width);
	
	//subhead
	CMenuSeparator * pm_subhead = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING);
	string 	s_sh = g_Locale->getText(LOCALE_PERSONALIZE_ACCESS);
			s_sh += ": " + mn_name;
	pm_subhead->setString(s_sh);
	
	pm->addItem(pm_subhead);
	pm->addIntroItems();

	//add all needed items
	for (uint i = 0; i<v_item.size(); i++)
	{
		if (mn_name == v_item[i].widget->getName())
		{
			int show_mode = v_item[i].item_mode;
			
			if (show_mode != PERSONALIZE_SHOW_NO)
			{
				if (show_mode == PERSONALIZE_SHOW_AS_ITEM_OPTION) 
				{	
					if (v_item[i].personalize_mode != NULL) //option chooser
						pm->addItem(new CMenuOptionChooser(v_item[i].locale_name, v_item[i].personalize_mode, PERSONALIZE_STD_OPTIONS, PERSONALIZE_STD_OPTION_COUNT, v_item[i].menuItem->active));
					else 
						pm->addItem(v_item[i].menuItem); //separator
				}
				
				//pin protected items only
				if (show_mode == PERSONALIZE_SHOW_AS_ACCESS_OPTION)
				{
					string 	itm_name = g_Locale->getText(v_item[i].locale_name);
							itm_name += " ";
							itm_name += g_Locale->getText(LOCALE_PERSONALIZE_PINSTATUS);
							
					if (v_item[i].personalize_mode != NULL)
						pm->addItem(new CMenuOptionChooser(itm_name.c_str(), v_item[i].personalize_mode, PERSONALIZE_YON_OPTIONS, PERSONALIZE_YON_OPTION_COUNT, v_item[i].menuItem->active));
				}
				
				//only show in personalize menu, usefull to hide separators in menu, but visible only in personalizing menu
				if (show_mode == PERSONALIZE_SHOW_ONLY_IN_PERSONALIZE_MENU)
					pm->addItem(v_item[i].menuItem); 
			}	
		}
	}

	int res = pm->exec(NULL, "");
	delete pm;

	return res;
}

//shows a short help message
void CPersonalizeGui::ShowHelpPersonalize()
{
	Helpbox helpbox;
	
	for (int i = (int)LOCALE_PERSONALIZE_HELP_LINE1; i<= (int)LOCALE_PERSONALIZE_HELP_LINE8; i++)
		helpbox.addLine(g_Locale->getText((neutrino_locale_t)i));


	helpbox.show(LOCALE_PERSONALIZE_HELP);
}

void CPersonalizeGui::SaveAndExit()
{
	// Save the settings and left menu, if user wants to!
	if (haveChangedSettings())
	{
		if (ShowLocalizedMessage(LOCALE_PERSONALIZE_HEAD,
					 LOCALE_PERSONALIZE_APPLY_SETTINGS,
					 CMessageBox::mbrYes,
					 CMessageBox::mbYes | CMessageBox::mbNo,
					 NEUTRINO_ICON_PROTECTING) == CMessageBox::mbrYes)
		{
			CHintBox hintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MAINSETTINGS_SAVESETTINGSNOW_HINT)); // UTF-8
			hintBox.paint();
			// replace old settings with new settings
			for (uint i = 0; i < v_int_settings.size(); i++)
				v_int_settings[i].old_val = *v_int_settings[i].p_val;
			//CNeutrinoApp::getInstance()->saveSetup();

			addPersonalizedItems();
			hintBox.hide();
		}
		else
		{
			if (ShowLocalizedMessage(LOCALE_PERSONALIZE_HEAD,
						 LOCALE_MESSAGEBOX_DISCARD,
						 CMessageBox::mbrNo,
						 CMessageBox::mbYes | CMessageBox::mbNo,
						 NEUTRINO_ICON_PROTECTING) == CMessageBox::mbrYes)
				restoreSettings();
		}
	}
}



//adds a menu widget to v_widget and sets the count of available widgets 
void CPersonalizeGui::addWidget(CMenuWidget *widget)
{
	v_widget.push_back(widget);
	widget_count = v_widget.size();
}

//adds a group of menu widgets to v_widget and sets the count of available widgets
void CPersonalizeGui::addWidgets(const struct mn_widget_t * const widget, const int& widgetCount)
{
	for (int i = 0; i<(widgetCount); i++)
		addWidget(new CMenuWidget(widget[i].locale_text, widget[i].icon, widget[i].width));
}

//returns a menu widget from v_widget
CMenuWidget& CPersonalizeGui::getWidget(const int& id)
{
	return *v_widget[id];
}

//returns index of menu widget from 'v_widget'
int CPersonalizeGui::getWidgetId(CMenuWidget *widget)
{
	for (int i = 0; i<widget_count; i++)
		if (v_widget[i] == widget)
			return i;

	return -1;
}


//adds non personalized menu intro items objects with separator, back button and separator line to menu without personalizing parameters
void CPersonalizeGui::addIntroItems(const int& widget_id)
{
	addIntroItems(v_widget[widget_id]);
}

void CPersonalizeGui::addIntroItems(CMenuWidget *widget)
{
	addItem(widget, GenericMenuSeparator, 		NULL, false, PERSONALIZE_SHOW_NO);
	addItem(widget, GenericMenuBack, 		NULL, false, PERSONALIZE_SHOW_NO);
	addItem(widget, GenericMenuSeparatorLine, 	NULL, false, PERSONALIZE_SHOW_NO);
}


//overloaded version from 'addItem', first parameter is an id from widget collection 'v_widget'
void CPersonalizeGui::addItem(const int& widget_id, CMenuItem *menu_Item, const int *personalize_mode, const bool defaultselected, const int& item_mode, const bool *item_active)
{
	addItem(v_widget[widget_id], menu_Item, personalize_mode, defaultselected, item_mode, item_active);
}

//adds a personalized menu item object to menu with personalizing parameters
void CPersonalizeGui::addItem(CMenuWidget *widget, CMenuItem *menu_Item, const int *personalize_mode, const bool defaultselected, const int& item_mode, const bool *item_active)
{
 	CMenuForwarder *fw = static_cast <CMenuForwarder*> (menu_Item);
	
	menu_item_t item = {widget, menu_Item, defaultselected, fw->getTextLocale(), (int*)personalize_mode, item_mode, (bool*)item_active};

	if (item_mode == PERSONALIZE_SHOW_AS_ACCESS_OPTION)
	{
		v_item.push_back(item);
		handleSetting((int*)personalize_mode);
	}
	else if (personalize_mode != NULL)
	{
		v_item.push_back(item);
		if (item_mode != PERSONALIZE_SHOW_NO) //handle only relevant items
			handleSetting((int*)personalize_mode);
	}
	else if (personalize_mode == NULL)
		v_item.push_back(item);
}

//overloaded version from 'addSeparator', first parameter is an id from widget collection 'v_widget',
void CPersonalizeGui::addSeparator(const int& widget_id, const neutrino_locale_t locale_text, const int& item_mode)
{
	addSeparator(*v_widget[widget_id], locale_text, item_mode);
}

//adds a menu separator to menue, based upon GenericMenuSeparatorLine or CMenuSeparator objects with locale
//expands with parameter within you can show or hide this item in personalize options
void CPersonalizeGui::addSeparator(CMenuWidget &widget, const neutrino_locale_t locale_text, const int& item_mode)
{
	if (locale_text == NONEXISTANT_LOCALE)
	{
		menu_item_t to_add_sep = {&widget, GenericMenuSeparatorLine, false, locale_text, NULL, item_mode, NULL};
		v_item.push_back(to_add_sep);
	}
	else
	{
		menu_item_t to_add_sep = {&widget, new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, locale_text), false, locale_text, NULL, item_mode, NULL};
		v_item.push_back(to_add_sep);
	}
}

//paint all available personalized menu items and separators to menu
//this replaces all collected actual and handled "widget->addItem()" tasks at once
void CPersonalizeGui::addPersonalizedItems()
{
	bool allow_sep = true;
	int old_w_id = 0;
	int widget_id = 0;
	int short_cut = shortcut;
	int old_p_mode = PERSONALIZE_MODE_NOTVISIBLE;

	for (int i = 0; i<(widget_count); i++)
		v_widget[i]->resetWidget();

 	for (uint i = 0; i < v_item.size(); i++)
	{
		int i_mode = v_item[i].item_mode;
			
		if (i_mode != PERSONALIZE_SHOW_ONLY_IN_PERSONALIZE_MENU) //skip if item only used in personalize settings
		{
			widget_id = getWidgetId(v_item[i].widget);
					
			if (old_w_id != widget_id)
			{
				//reset shortcut if widget has changed
				short_cut = shortcut; 
				
				//normalize previous widget: remove last item, if it is a separator line
				uint items_count = v_item[old_w_id].widget->getItemsCount(); 
				if (v_item[old_w_id].widget->getItem(items_count-1) == GenericMenuSeparatorLine)
					v_item[old_w_id].widget->removeItem(items_count-1);
				
				allow_sep = true;
			}
										
			if (v_item[i].personalize_mode != NULL) //handle personalized item and non separator
			{
				CMenuForwarder *fw = static_cast <CMenuForwarder*> (v_item[i].menuItem);
				
				bool use_pin 		= false;
				int p_mode 		= *v_item[i].personalize_mode;
				neutrino_msg_t d_key 	= fw->directKey;
				bool add_shortcut 	= false;

				//update visibility
				if (v_item[i].item_active != NULL)
					fw->active = *v_item[i].item_active;

				//get shortcut
				if (fw->active && (d_key == CRCInput::RC_nokey || CRCInput::isNumeric(d_key))) //if item is active and numeric or no key is assigned, allow to generate a shortcut
				{
					add_shortcut = true;
					d_key = getShortcut(short_cut);
				}					
				
				//set pin mode if required
				if (p_mode == PERSONALIZE_MODE_PIN || (p_mode == PROTECT_MODE_PIN_PROTECTED && i_mode == PERSONALIZE_SHOW_AS_ACCESS_OPTION))
					use_pin = true;
				
				//convert item to locked forwarder and use generated pin mode for usage as ask parameter 
				v_item[i].menuItem = new CLockedMenuForwarder(fw->getTextLocale(), g_settings.personalize_pincode, use_pin, fw->active, NULL, fw->getTarget(), fw->getActionKey().c_str(), d_key, fw->iconName.c_str());
				
				//add item if it's set to visible or pin protected and allow to add an forwarder as next
				if (v_item[i].menuItem->active && (p_mode != PERSONALIZE_MODE_NOTVISIBLE || i_mode == PERSONALIZE_SHOW_AS_ACCESS_OPTION))
				{
					//add item
					v_item[i].widget->addItem(v_item[i].menuItem, v_item[i].default_selected); //forwarders...
					allow_sep = true;
									
					//generate shortcut for next item
					if (add_shortcut)
						short_cut++;
				}
				else if (p_mode == PERSONALIZE_MODE_NOTVISIBLE)
				{
					//allow separator as next if personalize mode was changed
					if (p_mode != old_p_mode)
					{
						old_p_mode = p_mode;
						allow_sep = true;
					}
 				}
									
				delete fw;
			}
			else //handle and add separator as non personalized item and don't allow to add a separator as next but allow back button as next
			{						
				if (allow_sep || v_item[i].menuItem == GenericMenuBack)
				{
					v_item[i].widget->addItem(v_item[i].menuItem, v_item[i].default_selected); //separators
					allow_sep = v_item[i].menuItem == GenericMenuBack ? true : false;
				}
			}
		}
		old_w_id = widget_id;
	}
}


// returns RC_key depends of shortcut between key number 1 to 0, 10 returns 0, >10 returns no key
// parameter alternate_rc_key allows using an alternate key, default key is RC_nokey
neutrino_msg_t CPersonalizeGui::getShortcut(const int & shortcut_num, neutrino_msg_t alternate_rc_key)
{
	if (shortcut_num < 10) 
		return CRCInput::convertDigitToKey(shortcut_num);
	else if (shortcut_num == 10) 
		return CRCInput::RC_0;
	else	
		return alternate_rc_key;
}

//handle/collects old int settings
void  CPersonalizeGui::handleSetting(int *setting)
{	
	settings_int_t val	= {*setting, setting};
	v_int_settings.push_back(val);
}

//check for setup changes
bool  CPersonalizeGui::haveChangedSettings()
{
	//compare old settings with current settings
	for (uint i = 0; i < v_int_settings.size(); i++)
		if (v_int_settings[i].old_val != *v_int_settings[i].p_val)
			return true;
	
	return false;
}

//restore old settings
void CPersonalizeGui::restoreSettings()
{
	//restore settings with current settings
	for (uint i = 0; i < v_int_settings.size(); i++)
		*v_int_settings[i].p_val = v_int_settings[i].old_val;
}

