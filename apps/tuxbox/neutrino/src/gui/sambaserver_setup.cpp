/*
	$Id: sambaserver_setup.cpp,v 1.18 2012/09/23 08:16:48 rhabarber1848 Exp $

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gui/sambaserver_setup.h"
#include "gui/drive_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/stringinput.h>
#include <gui/widget/dirchooser.h>

#include <driver/screen_max.h>
#include <system/debug.h>
#include <system/setting_helpers.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <errno.h>
#include <libnet.h>
#include <signal.h>
#include <sys/vfs.h> // statfs
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

using namespace std;

CSambaSetup::CSambaSetup(const neutrino_locale_t title, const char * const IconName)
{
	menue_title = title;
	menue_icon = IconName;

	width = w_max (550, 100);
	selected = -1;

	interface = getInterface();
}

CSambaSetup::~CSambaSetup()
{
}

int CSambaSetup::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init samba menu\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	res = Init();
	g_settings.smb_setup_samba_workgroup = upperString(g_settings.smb_setup_samba_workgroup);
	return res;
}

// init menue
int CSambaSetup::Init()
{
	int res = menu_return::RETURN_REPAINT;

	if (!haveSambaSupport())
		DisplayInfoMessage(g_Locale->getText(LOCALE_SAMBASERVER_SETUP_MSG_NOT_INSTALLED));
	else
		res = showSambaSetup();

	return res;
}


#define SMB_YES_NO_OPTION_COUNT 2
const CMenuOptionChooser::keyval SMB_YES_NO_OPTIONS[SMB_YES_NO_OPTION_COUNT] =
{
	{ CSambaSetup::OFF, LOCALE_SAMBASERVER_SETUP_SERVER_OFF  },
	{ CSambaSetup::ON, LOCALE_SAMBASERVER_SETUP_SERVER_ON }
};

/* shows entries for samba settings */
int CSambaSetup::showSambaSetup()
{
	//init
	CMenuWidget * sm = new CMenuWidget(menue_title, menue_icon, width);
	sm->setPreselected(selected);

	//start/stop sambaserver, set real server status
	if (getPidof(NMBD).empty() || getPidof(SMBD).empty())
	{
		g_settings.smb_setup_samba_on_off = OFF;
		remove(SAMBA_MARKER);
	}
	else
		g_settings.smb_setup_samba_on_off = ON;

	CSambaOnOffNotifier smb_notifier(SAMBA_MARKER);
	CMenuOptionChooser * sm_start = new CMenuOptionChooser(LOCALE_SAMBASERVER_SETUP_STAT, &g_settings.smb_setup_samba_on_off, SMB_YES_NO_OPTIONS, SMB_YES_NO_OPTION_COUNT, true, &smb_notifier, CRCInput::RC_standby);

	//workroup, domainname
	CStringInputSMS sm_input_domain(LOCALE_SAMBASERVER_SETUP_WORKGROUP, &g_settings.smb_setup_samba_workgroup, 15, false, LOCALE_SAMBASERVER_SETUP_WORKGROUP_HINT1, LOCALE_SAMBASERVER_SETUP_WORKGROUP_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789_ ");
	CMenuForwarder * sm_fw_domain = new CMenuForwarder(LOCALE_SAMBASERVER_SETUP_WORKGROUP, true, g_settings.smb_setup_samba_workgroup, &sm_input_domain, NULL, CRCInput::RC_green);

	//address,interface
	CMenuForwarder * sm_fw_interface = new CMenuForwarder(LOCALE_SAMBASERVER_SETUP_INTERFACE, false, interface);

	//add items
	sm->addIntroItems(menue_title != LOCALE_SAMBASERVER_SETUP ? LOCALE_SAMBASERVER_SETUP : NONEXISTANT_LOCALE);
	//-----------------------------------
	sm->addItem(sm_start);			//server stat
	sm->addItem(GenericMenuSeparatorLine);
	//-----------------------------------
	sm->addItem(sm_fw_domain);		//workgroup/domain input
	sm->addItem(sm_fw_interface);		//interface

	int res = sm->exec(NULL, "");
	selected = sm->getSelected();
	delete sm;

	return res;
}


//helper: uppercase string
string CSambaSetup::upperString(const string& to_upper_str)
{
	char s[23];
	sprintf(s, "%s", to_upper_str.c_str());
	unsigned n=0;

	while (*(s+n))
	{
		if (*(s+n)>96 && *(s+n)<123)
			*(s+n)-=32;
		n++;
	}

	return s;
}



//notfier for samba switching
#define SAMBA_COMMANDS_COUNT 2
typedef struct smb_cmd_t
{
	const string bin;
	const string option;
} smb_cmd_struct_t;

const smb_cmd_struct_t smb_cmd[SAMBA_COMMANDS_COUNT] =
{
	{NMBD, " -D"},
	{SMBD, " -D -a -s "},
};

//helper check if nmbd and smbd are available
bool CSambaSetup::haveSambaSupport()
{
	bool ret = true;
	string 	smbdir[] = {SMB_VAR_DIR, SMB_PRIVAT_VAR_DIR};

	int dirs = (sizeof(smbdir) / sizeof(smbdir[0]));

	//check and generate private dir, if not exists, usefull for updates from older smb versions without or other path of privat dir
	DIR   *dirCheck[dirs];

	for (int i = 0; i<dirs; i++)
	{
		dirCheck[i] = opendir(smbdir[i].c_str());

		if ( dirCheck[i] == NULL )
		{
			if(mkdir(smbdir[i].c_str(), 0777) !=0) 
			{	
				cerr << "[samba setup] "<<smbdir[i]<<" "<< strerror(errno) <<endl;
				ret = false;
			}
		}
		else 
			closedir( dirCheck[i] );
	}

	//check available samba binaries smbd, nmbd
	for (int i = 0; i<SAMBA_COMMANDS_COUNT; i++)
	{
		string smb_bin = "/bin/" + smb_cmd[i].bin;
		string smb_def_bin = "/var/bin/" + smb_cmd[i].bin;
		string smb_hdd_bin = "/hdd/bin/" + smb_cmd[i].bin;
		
		if(access(smb_bin.c_str(), X_OK) !=0 && access(smb_def_bin.c_str(), X_OK) !=0 && access(smb_hdd_bin.c_str(), X_OK) !=0) 
		{
			cerr << "[samba setup] "<<smb_cmd[i].bin<<" "<< strerror(errno) <<endl;
			return false;
		}
	}

	return ret;
}

//helper check if smb.conf is available
bool CSambaSetup::haveSambaConf()
{
	if((access( SMBCONF_VAR , R_OK) !=0) && (access( SMBCONF , W_OK) !=0))
			return false;

	return true;
}

// kills samba 
bool CSambaSetup::killSamba()
{
	bool ret = true;
	string pid;

	for (int i = 0; i<SAMBA_COMMANDS_COUNT; i++)
	{
		while (!(pid = getPidof(smb_cmd[i].bin)).empty())
		{
			stringstream Str;
			Str << pid;
			int i_pid;
			Str >> i_pid;
			
			if (!pid.empty())
			{	
				if (kill(i_pid, SIGKILL) !=0)
				{
					cerr << "[samba setup] "<<__FUNCTION__ <<":  error while terminating: "<<smb_cmd[i].bin <<" "<< strerror(errno)<<endl;
					ret = false;
				}
				else
					cout << "[samba setup] killed "<< smb_cmd[i].bin << " pid: "<<pid<<endl;
			}
		}
		/* $ echo -n "/tmp/smbd.pid"|wc -c
		 * 13
		 * 15 should be enough */
		char pid_file[15];
		sprintf( pid_file, "/tmp/%s.pid", smb_cmd[i].bin.c_str());
		if (ret)
			remove(pid_file);
		
	}

	if(ret)
		remove("/tmp/smbd-smb.conf.pid");

	return ret;
}

// starts samba 
bool CSambaSetup::startSamba()
{
	bool ret = true;	

	if (!CDriveSetup::getInstance()->mkSmbConf() || !haveSambaConf())
	{
		err_msg += CDriveSetup::getInstance()->getErrMsg();
		err_msg += g_Locale->getText(LOCALE_SAMBASERVER_SETUP_MSG_MISSING_SMBCONF);
		return false;
	}
	
	for (int i = 0; i<SAMBA_COMMANDS_COUNT; i++)
	{
		if (getPidof(smb_cmd[i].bin).empty())
		{
			string options = smb_cmd[i].option + " " +  (smb_cmd[i].bin == SMBD ? SMBCONF : "");
			string cmd = smb_cmd[i].bin + options;

			int result = CNeutrinoApp::getInstance()->execute_sys_command(cmd.c_str());

			if (result !=0)
			{
				cerr<<"[samba setup] "<<__FUNCTION__ <<": error while executing " <<smb_cmd[i].bin<<endl;
				if (result == 127)
					err_msg += smb_cmd[i].bin + " not found!\n";
				else if (result == 126)
					err_msg += smb_cmd[i].bin + " is not executable! Plaese check permissions!\n";
				else if (result == 139)
					err_msg += smb_cmd[i].bin + " Fatal error! Segfault\n" ;
				else
					err_msg += cmd + "\n" + strerror(result);

				killSamba();

				ret = false;

			}
			else
				cout << "[samba setup] started "<< smb_cmd[i].bin << " (PID: "<<getPidof(smb_cmd[i].bin)<< ") with options:" << options<<endl;
		}

	}

	return ret;
}

CSambaOnOffNotifier::CSambaOnOffNotifier( const char * file_to_modify)
{
	filename = file_to_modify;
}

bool CSambaOnOffNotifier::changeNotify(const neutrino_locale_t, void * data)
{
	CSambaSetup smb;
	CDriveSetup drivesetup;
	bool have_shares = drivesetup.haveMountedSmbShares();
	
	if ((*(int *)data) != 0)
	{
		if (have_shares)
		{
			if (!smb.startSamba())
			{
				DisplayErrorMessage(smb.getErrMsg().c_str());
			}
			else
				DisplayInfoMessage(g_Locale->getText(LOCALE_SAMBASERVER_SETUP_STAT_RUNNING));
		}
			
		//write markerfile
		FILE * fd = fopen(filename, "w");
		if (fd)
			fclose(fd);
	}
	else
	{
		if (smb.killSamba())
			DisplayInfoMessage(g_Locale->getText(LOCALE_SAMBASERVER_SETUP_STAT_STOPPED));

  		remove(filename); //remove markerfile
	}

	return false;
}

