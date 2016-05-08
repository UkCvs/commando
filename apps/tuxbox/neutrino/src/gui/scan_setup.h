/*
	$Id: scan_setup.h,v 1.12 2012/07/22 06:24:46 rhabarber1848 Exp $

	Copyright (C) 2009 Thilo Graf (dbt)
	http://www.dbox2-tuning.de

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

*/

#ifndef __scan_setup__
#define __scan_setup__

#include <gui/widget/menue.h>

#include <zapit/settings.h>

#include <string>

class CScanSetup : public CMenuTarget, CChangeObserver
{
	private:
		int width, selected;

		uint sat_list_size;
		uint provider_list_size;

		std::string scan_mode_string;

		int showScanService();
		int showScanModeMenue();

	public:	
		CScanSetup();
		~CScanSetup();
		int exec(CMenuTarget* parent, const std::string & actionKey);
		bool changeNotify(const neutrino_locale_t OptionName, void * Data);

		void initScanSettings();
		std::string getScanModeString(const int& scan_type);
};

class CSatDiseqcNotifier : public CChangeObserver
{
	private:
		CMenuItem* satMenu;
		CMenuItem* extMenu;
		CMenuItem* extMotorMenu;
		CMenuItem* repeatMenu;
		CMenuItem* extUnicableMenu;
	protected:
		CSatDiseqcNotifier( ) : CChangeObserver(){};  // prevent calling constructor without data we need
	public:
		CSatDiseqcNotifier( CMenuItem* SatMenu, CMenuItem* ExtMenu, CMenuItem* ExtMotorMenu, CMenuItem* RepeatMenu, CMenuItem* ExtUnicableMenu) : CChangeObserver()
		{ satMenu = SatMenu; extMenu = ExtMenu; extMotorMenu = ExtMotorMenu; repeatMenu = RepeatMenu; extUnicableMenu = ExtUnicableMenu; };
		bool changeNotify(const neutrino_locale_t, void * Data);
};

class CTP_scanNotifier : public CChangeObserver
{
	private:
		CMenuOptionChooser* toDisable1[2];
		CMenuForwarder* toDisable2[2];
		CMenuOptionStringChooser* toDisable3[1];		
		std::string* scan_mode_string;
	public:
		CTP_scanNotifier(CMenuOptionChooser* i1, CMenuOptionChooser* i2, CMenuForwarder* i3, CMenuForwarder* i4, CMenuOptionStringChooser* i5, std::string &s);
		bool changeNotify(const neutrino_locale_t, void *);
};

#endif
