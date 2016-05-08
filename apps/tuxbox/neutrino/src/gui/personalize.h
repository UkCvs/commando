/*
$Id: personalize.h,v 1.21 2012/05/05 20:09:25 dbt Exp $

Customization Menu - Neutrino-GUI

Copyright (C) 2007 Speed2206
and some other guys

Kommentar:

This is the customization menu, as originally showcased in
Oxygen. It is a more advanced version of the 'user levels'
patch currently available.


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
*/


#ifndef __personalize__
#define __personalize__

#include <gui/widget/menue.h>
#include <string>
#include <vector>
#include <configfile.h>
#include <system/lastchannel.h>
#include <system/setting_helpers.h>

typedef struct mn_widget_t
{
	const neutrino_locale_t locale_text;
	const std::string icon;
	const int width;
} mn_widget_struct_t;

typedef struct settings_int_t
{
	int old_val;
	int *p_val;
}settings_int_struct_t;

typedef struct menu_item_t
{
	CMenuWidget *widget;
	CMenuItem* menuItem;
	bool default_selected;
	neutrino_locale_t locale_name;
	int* personalize_mode;
	int item_mode;
	bool* item_active;
}menu_item_struct_t;

typedef struct personalize_settings_t
{
	const char* personalize_settings_name;
	int personalize_default_val;
} personalize_settings_struct_t;


class CPersonalizeGui : public CMenuTarget
{
	private:

		int width, selected, widget_count, shortcut;
				
		//stuff for settings handlers
		void	handleSetting(int *setting);
		void	restoreSettings();
		bool	haveChangedSettings();

		std::vector<settings_int_t> v_int_settings;
		

		std::vector<menu_item_t> v_item;
		
		std::vector<CMenuWidget *> v_widget;
		
		int 	ShowPersonalizationMenu();
		int 	ShowMenuOptions(const int& menu);
		void 	ShowHelpPersonalize();
		void 	SaveAndExit();
		
		neutrino_msg_t	getShortcut(const int & shortcut_num, neutrino_msg_t alternate_rc_key = CRCInput::RC_nokey);
		
	public:	
		enum PERSONALIZE_MODE
		{
			PERSONALIZE_MODE_NOTVISIBLE =  0,
			PERSONALIZE_MODE_VISIBLE  =  1,
			PERSONALIZE_MODE_PIN  = 2
		};

		enum PERSONALIZE_PROTECT_MODE
		{
			PROTECT_MODE_NOT_PROTECTED =  0,
			PROTECT_MODE_PIN_PROTECTED  =  1
		};

		enum PERSONALIZE_ACTIVE_MODE
		{
			PERSONALIZE_MODE_DISABLED =  0,
			PERSONALIZE_MODE_ENABLED  =  1
		};
		
		enum PERSONALIZE_ITEM_MODE
		{
			PERSONALIZE_SHOW_NO =  0,
			PERSONALIZE_SHOW_AS_ITEM_OPTION  =  1,
			PERSONALIZE_SHOW_AS_ACCESS_OPTION  =  2,
			PERSONALIZE_SHOW_ONLY_IN_PERSONALIZE_MENU  =  3 //usefull to hide separators in menu, but visible only in personalizing menu
		};
		
		CPersonalizeGui();
		~CPersonalizeGui();
				
		static CPersonalizeGui* getInstance();
		
		

		int 	exec(CMenuTarget* parent, const std::string & actionKey);
		
		CMenuWidget& getWidget(const int& id);
		
		void 	addWidget(CMenuWidget *widget);
		void 	addWidgets(const struct mn_widget_t * const widget, const int& widget_count);
		int 	getWidgetCount() {return widget_count;};
		int 	getWidgetId(CMenuWidget *widget);
		void	setShortcut(const int& short_cut = 1) {shortcut = short_cut;};
		void 	addItem(CMenuWidget *widget, CMenuItem *menu_Item, const int *personalize_mode = NULL, const bool defaultselected = false, const int& item_mode = PERSONALIZE_SHOW_AS_ITEM_OPTION, const bool *item_active = NULL);
		void 	addItem(const int& widget_id, CMenuItem *menu_Item, const int *personalize_mode = NULL, const bool defaultselected = false, const int& item_mode = PERSONALIZE_SHOW_AS_ITEM_OPTION, const bool *item_active = NULL);
		void	addIntroItems(CMenuWidget *widget);
		void	addIntroItems(const int& widget_id);
		void 	addSeparator(CMenuWidget &menu, const neutrino_locale_t locale_text = NONEXISTANT_LOCALE, const int& item_mode = PERSONALIZE_SHOW_AS_ITEM_OPTION);
		void 	addSeparator(const int& widget_id, const neutrino_locale_t locale_text = NONEXISTANT_LOCALE, const int& item_mode = PERSONALIZE_SHOW_AS_ITEM_OPTION);
		void 	addPersonalizedItems();
};
#endif
