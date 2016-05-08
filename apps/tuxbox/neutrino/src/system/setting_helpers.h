/*
	$Id: setting_helpers.h,v 1.113 2012/06/18 16:53:35 rhabarber1848 Exp $

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

#ifndef __SETTING_HELPERS__
#define __SETTING_HELPERS__

#include <config.h>
#include <gui/widget/menue.h>

#include <string>
#include <vector>

unsigned long long getcurrenttime();
std::string getPidof(const std::string& process_name);
std::string getFileEntryString(const char* filename, const std::string& filter_entry, const int& column_num);
std::string getInterface();

class COnOffNotifier : public CChangeObserver
{
	private:
		int offValue;
		std::vector<CMenuItem*> toDisable;

	public:
		COnOffNotifier(int OffValue = 0);
		bool changeNotify(const neutrino_locale_t, void *Data);

		void addItem(CMenuItem* menuItem);
		void removeItems();
};

class CStreamFeaturesChangeExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CUCodeCheckExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CDVBInfoExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

#endif

