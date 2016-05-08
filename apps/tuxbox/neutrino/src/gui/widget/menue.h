/*
	$Id: menue.h,v 1.102 2012/08/29 18:19:10 rhabarber1848 Exp $

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


#ifndef __MENU__
#define __MENU__

#include <driver/framebuffer.h>
#include <driver/rcinput.h>
#include <system/localize.h>
#include <gui/widget/icons.h>

#include <string>
#include <vector>

struct menu_return
{
	enum
		{
			RETURN_NONE	= 0,
			RETURN_REPAINT 	= 1,
			RETURN_EXIT 	= 2,
			RETURN_EXIT_ALL = 4
		};
};

class CChangeObserver
{
	public:
		virtual ~CChangeObserver(){}
		virtual bool changeNotify(const neutrino_locale_t /*OptionName*/, void * /*Data*/)
		{
			return false;
		}
};

class CMenuTarget
{
	public:

		CMenuTarget(){}
		virtual ~CMenuTarget(){}
		virtual void hide(){}
		virtual int exec(CMenuTarget* parent, const std::string & actionKey) = 0;
		virtual const char * getTargetValue() { return NULL; }
};


class CMenuItem
{
	private:
		std::string getIconName();

	protected:
		int x, y, dx, offx;
		bool used;
		unsigned char item_color;
		fb_pixel_t item_bgcolor;

		void initItemColors(const bool select_mode);

	public:
		bool           active;
		neutrino_msg_t directKey;
		std::string    iconName;
		std::string    selected_iconName;

		CMenuItem()
		{
			directKey = CRCInput::RC_nokey;
			iconName = "";
			used = false;
		}
		virtual ~CMenuItem(){}

		virtual void isUsed(void)
		{
			used = true;
		}

		virtual void init(const int X, const int Y, const int DX, const int OFFX);

		virtual int paint (bool selected = false) = 0;

		virtual int getHeight(void) const = 0;

		virtual bool isSelectable(void) const
		{
			return false;
		}

		virtual int exec(CMenuTarget* /*parent*/)
		{
			return 0;
		}
		virtual void setActive(const bool Active);

		virtual void paintItemButton(const bool select_mode, const int &item_height, const std::string& icon_Name = NEUTRINO_ICON_BUTTON_RIGHT);

		virtual void paintItemBackground (const bool select_mode, const int &item_height);

		virtual void prepareItem(const bool select_mode, const int &item_height);

		virtual void setItemButton(const std::string& icon_Name, const bool is_select_button = false);

		virtual void paintItemCaption(const bool select_mode, const int &item_height, const char * left_text = NULL, const char * right_text = NULL);
};

class CMenuSeparator : public CMenuItem
{
		int               type;

	protected:
		neutrino_locale_t text;
		std::string	  separator_text;

	public:
		enum
		{
			EMPTY =	0,
			LINE =	1,
			STRING =	2,
			ALIGN_CENTER = 4,
			ALIGN_LEFT =   8,
			ALIGN_RIGHT = 16,
			SUB_HEAD = 32
		};


		CMenuSeparator(const int Type = EMPTY, const neutrino_locale_t Text = NONEXISTANT_LOCALE);
		virtual ~CMenuSeparator(){}

		int paint(bool selected=false);
		int getHeight(void) const;

		virtual const char * getString(void);
		virtual void setString(const std::string& s_text);
};

class CMenuForwarder : public CMenuItem
{
	const char *        option;
	const std::string * option_string;
	CMenuTarget *       jumpTarget;
	std::string         actionKey;

 protected:
	neutrino_locale_t text;
	std::string forwarder_text;

	virtual const char * getOption(void);
	virtual const char * getName(void);
	
 public:

	CMenuForwarder(const neutrino_locale_t Text, const bool Active=true, const char * const Option=NULL, CMenuTarget* Target=NULL, const char * const ActionKey = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const char * const IconName = NULL);
	CMenuForwarder(const neutrino_locale_t Text, const bool Active, const std::string &Option, CMenuTarget* Target=NULL, const char * const ActionKey = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const char * const IconName = NULL);
	// Text must be UTF-8 encoded:
	CMenuForwarder(const char * const Text, const bool Active=true, const char * const Option=NULL, CMenuTarget* Target=NULL, const char * const ActionKey = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const char * const IconName = NULL);
	CMenuForwarder(const char * const Text, const bool Active, const std::string &Option, CMenuTarget* Target=NULL, const char * const ActionKey = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const char * const IconName = NULL);
	virtual ~CMenuForwarder(){}

	int paint(bool selected=false);
	int getHeight(void) const;
	void setOption(const char *Option);
	void setTextLocale(const neutrino_locale_t Text);
	neutrino_locale_t getTextLocale(){return text;};
	void setText(const char * const Text);
	CMenuTarget* getTarget(){return jumpTarget;};
	std::string getActionKey(){return actionKey;};
	int exec(CMenuTarget* parent);
	bool isSelectable(void) const
		{
			return active;
		}
};

class CAbstractMenuOptionChooser : public CMenuItem
{
 protected:
	neutrino_locale_t optionName;
	int *             optionValue;
	CChangeObserver * observ;

	bool isSelectable(void) const
		{
			return active;
		}
};

class CMenuOptionNumberChooser : public CAbstractMenuOptionChooser
{
	const char *       optionString;

	int                lower_bound;
	int                upper_bound;

	int                display_offset;

	int                localized_value;
	neutrino_locale_t  localized_value_name;

	bool               numeric_input;
	std::string        numberFormat;

 public:
	CMenuOptionNumberChooser(const neutrino_locale_t name, int * const OptionValue, const bool Active, const int min_value, const int max_value, const int print_offset = 0, const int special_value = 0, const neutrino_locale_t special_value_name = NONEXISTANT_LOCALE, const char * non_localized_name = NULL, CChangeObserver * const Observ = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const std::string & IconName = "", bool NumericInput = false);
	
	void setNumberFormat(const std::string &format);
	int paint(bool selected);
	int getHeight(void) const;

	int exec(CMenuTarget* parent);
};

class CMenuOptionChooser : public CAbstractMenuOptionChooser
{
 public:
	struct keyval
	{
		const int               key;
		const neutrino_locale_t value;
	};

 private:
	std::vector<const keyval *> options;
	unsigned                    number_of_options;
	std::string                 optionNameString;
	bool                        pulldown;
	bool                        optionsSort;

 public:
	CMenuOptionChooser(const neutrino_locale_t OptionName, int * const OptionValue, const struct keyval * const Options, const unsigned Number_Of_Options, const bool Active = false, CChangeObserver * const Observ = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const std::string & IconName= "", bool Pulldown = false, bool OptionsSort = false);
	CMenuOptionChooser(const char* OptionName, int * const OptionValue, const struct keyval * const Options, const unsigned Number_Of_Options, const bool Active = false, CChangeObserver * const Observ = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const std::string & IconName= "", bool Pulldown = false, bool OptionsSort = false);

	void setOptionValue(const int newvalue);
	int getOptionValue(void) const;

	int paint(bool selected);
	int getHeight(void) const;
	std::string getOptionName() {return optionNameString;};

	int exec(CMenuTarget* parent);
};

class CMenuOptionStringChooser : public CMenuItem
{
		neutrino_locale_t        optionName;
		char *                   optionValue;
		std::vector<std::string> options;
		CChangeObserver *        observ;
		bool                     pulldown;
		bool                     optionsSort;

	public:
		CMenuOptionStringChooser(const neutrino_locale_t OptionName, char* OptionValue, bool Active = false, CChangeObserver* Observ = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const std::string & IconName= "", bool Pulldown = false, bool OptionsSort = false);
		~CMenuOptionStringChooser();

		void addOption(const char * value);
		void removeOptions(void);
		int paint(bool selected);
		int getHeight(void) const;
		bool isSelectable(void) const
		{
			return active;
		}

		int exec(CMenuTarget* parent);
};

class CMenuWidget : public CMenuTarget
{
	protected:
		neutrino_locale_t name;
		std::string		nameString;
		CFrameBuffer		*frameBuffer;
		std::vector<CMenuItem*>	items;
		std::vector<int> page_start;
		std::string			iconfile;

		int			width;
		int			sb_width;
		int			height;
		int			wanted_height;
		int			x;
		int			y;
		int			preselected;
		int			selected;
		int 			iconOffset;
		unsigned int         item_start_y;
		unsigned int         current_page;
		unsigned int         total_pages;

		virtual void paintItems();
		virtual bool updateSelection(int pos);

	public:
		CMenuWidget(const neutrino_locale_t Name, const std::string & Icon = "", const int mwidth = 400, const int mheight = 576);
		CMenuWidget(const char* Name, const std::string & Icon = "", const int mwidth = 400, const int mheight = 576);
		~CMenuWidget();

		enum
		{
			BTN_TYPE_BACK =		0,
			BTN_TYPE_CANCEL =	1,
			BTN_TYPE_NO =		-1
		};

		virtual void addItem(CMenuItem* menuItem, const bool defaultselected = false);
		virtual void addIntroItems(neutrino_locale_t subhead_text = NONEXISTANT_LOCALE, neutrino_locale_t section_text = NONEXISTANT_LOCALE, int buttontype = BTN_TYPE_BACK);
		bool hasItem();
		void resetWidget();
		void insertItem(const uint& item_id, CMenuItem* menuItem);
		void removeItem(const uint& item_id);
		int getItemId(CMenuItem* menuItem);
		int getItemsCount(){return items.size();};
		CMenuItem* getItem(const uint& item_id);

		virtual void paint();
		virtual void hide();
		virtual int exec(CMenuTarget* parent, const std::string & actionKey);
		virtual std::string getName();
		virtual void setPreselected(const int &Preselected){ preselected = Preselected; };
		virtual int getSelected(){ return selected; };
};

class CPINProtection
{
	protected:
		char* validPIN;
		bool check();
		virtual CMenuTarget* getParent() = 0;
	public:
		CPINProtection( char* validpin){ validPIN = validpin;};
		virtual ~CPINProtection(){}
};

class CZapProtection : public CPINProtection
{
	protected:
		virtual CMenuTarget* getParent() { return( NULL);};
	public:
		int	fsk;

		CZapProtection( char* validpin, int	FSK ) : CPINProtection(validpin){ fsk= FSK; };
		bool check();
};

class CLockedMenuForwarder : public CMenuForwarder, public CPINProtection
{
	CMenuTarget* Parent;
	bool Ask;

	protected:
		virtual CMenuTarget* getParent(){ return Parent;};
	public:
		CLockedMenuForwarder(const neutrino_locale_t Text, char* _validPIN, bool ask=true, const bool Active=true, char *Option=NULL,
		                     CMenuTarget* Target=NULL, const char * const ActionKey = NULL,
		                     neutrino_msg_t DirectKey = CRCInput::RC_nokey, const char * const IconName = NULL)

		                     : CMenuForwarder(Text, Active, Option, Target, ActionKey, DirectKey, IconName) ,
		                       CPINProtection(_validPIN){Ask = ask;};
				       
		virtual int exec(CMenuTarget* parent);
};


class CMenuSelectorTarget : public CMenuTarget
{
	public:
		CMenuSelectorTarget(int *select) {m_select = select;};
		int exec(CMenuTarget* parent, const std::string & actionKey);

	private:
		int *m_select;
};


extern CMenuSeparator * const GenericMenuSeparator;
extern CMenuSeparator * const GenericMenuSeparatorLine;
extern CMenuForwarder * const GenericMenuBack;
extern CMenuForwarder * const GenericMenuCancel;



#endif
