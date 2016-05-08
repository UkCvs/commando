/*
	$Id: sambaserver_setup.h,v 1.8 2012/09/23 08:16:48 rhabarber1848 Exp $

	sambaserver setup menue - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2010 T. Graf 'dbt'
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

#ifndef __sambaserver_setup__
#define __sambaserver_setup__

#include <configfile.h>

#include <gui/widget/menue.h>
#include <gui/widget/icons.h>

#include <string>

#define NMBD		"nmbd"
#define SMBD		"smbd"
#define SAMBA_MARKER	VAR_ETC_DIR "/.sambaserver"
#define SMB_DIR		ETC_DIR	"/samba"
#define SMB_VAR_DIR	VAR_ETC_DIR "/samba"
#define SMB_PRIVAT_DIR		SMB_DIR	"/private"
#define SMB_PRIVAT_VAR_DIR	SMB_VAR_DIR "/private"


class CSambaSetup : public CMenuTarget
{
	private:
		int width, selected;

		neutrino_locale_t menue_title;
		std::string menue_icon;
		std::string interface;
		std::string err_msg;

		//helper
		std::string upperString(const std::string& to_upper_str);

		int showSambaSetup();
		int Init();

	public:	
		CSambaSetup(const neutrino_locale_t title = LOCALE_SAMBASERVER_SETUP, const char * const IconName = NEUTRINO_ICON_SETTINGS);
		~CSambaSetup();

		enum SMB_ON_OFF_NUM	
		{
			OFF,
			ON,
		};

		int exec(CMenuTarget* parent, const std::string & actionKey);

		bool haveSambaSupport();
		bool haveSambaConf();
		bool startSamba();
		bool killSamba();

		std::string getErrMsg() {return err_msg;};
};

class CSambaOnOffNotifier : public CChangeObserver
{
	private:
		const char * filename;
		std::string err_msg;

	public:
		inline CSambaOnOffNotifier(const char * file_to_modify);
		bool changeNotify(const neutrino_locale_t, void * data);
};

#endif
