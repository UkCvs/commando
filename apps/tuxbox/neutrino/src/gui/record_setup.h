/*
	$Id: record_setup.h,v 1.8 2012/06/18 16:53:34 rhabarber1848 Exp $

	record setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/


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

#ifndef __record_setup__
#define __record_setup__

#include <gui/widget/menue.h>

#include <string>

class CRecordSetup : public CMenuTarget, CChangeObserver
{
	private:
		int width, selected;

		char record_safety_time_before[3];
		char record_safety_time_after[3];
		char zapto_safety_time_before[3];

		int recording_audio_pids_std;
		int recording_audio_pids_alt;
		int recording_audio_pids_ac3;

		int showRecordSetup();

	public:	
		CRecordSetup();
		~CRecordSetup();
		int exec(CMenuTarget* parent, const std::string & actionKey);
		bool changeNotify(const neutrino_locale_t OptionName, void *);
};

class CRecordingNotifier : public CChangeObserver
{
	private:
		CMenuItem* toDisable[9];
	public:
		CRecordingNotifier(CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*);
		bool changeNotify(const neutrino_locale_t OptionName, void*);
};

#endif
