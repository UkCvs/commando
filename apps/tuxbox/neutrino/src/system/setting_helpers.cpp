/*
	$Id: setting_helpers.cpp,v 1.204 2012/06/30 10:57:43 rhabarber1848 Exp $

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

#include <system/setting_helpers.h>
#include <system/configure_network.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "libnet.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <dirent.h>
#include <errno.h>

#include <libucodes.h>

#include <xmlinterface.h>
#include <config.h>

#include <global.h>
#include <neutrino.h>
#include <gui/widget/messagebox.h>

#define PROCDIR "/proc"

COnOffNotifier::COnOffNotifier(int OffValue)
{
	offValue = OffValue;
}

bool COnOffNotifier::changeNotify(const neutrino_locale_t, void *Data)
{
	bool active = (*(int*)(Data) != offValue);

	for (std::vector<CMenuItem*>::iterator it = toDisable.begin(); it != toDisable.end(); ++it)
		(*it)->setActive(active);

	return false;
}

void COnOffNotifier::addItem(CMenuItem* menuItem)
{
	toDisable.push_back(menuItem);
}

void COnOffNotifier::removeItems()
{
	toDisable.clear();
}

int CStreamFeaturesChangeExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	//printf("CStreamFeaturesChangeExec exec: %s\n", actionKey.c_str());
	int sel= atoi(actionKey.c_str());

	if(parent != NULL)
		parent->hide();
	// -- obsolete (rasc 2004-06-10)
	// if (sel==-1)
	// {
	// 	CStreamInfo StreamInfo;
	//	StreamInfo.exec(NULL, "");
	// } else
	if (sel>=0)
	{
		if (!g_PluginList->isUsingLcd(sel))
			CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		g_PluginList->startPlugin(sel);
	}

	return menu_return::RETURN_EXIT;
}

#ifdef HAVE_DBOX_HARDWARE
int CUCodeCheckExec::exec(CMenuTarget*, const std::string &)
{
	std::stringstream text;
	char res[60];

	switch (g_info.avia_chip)
	{
		case CControld::TUXBOX_AVIACHIP_AVIA500:
			text << g_Locale->getText(LOCALE_UCODECHECK_AVIA500) << ": ";
			checkFile(UCODEDIR "/avia500.ux", (char*) &res);
			text << res << "\n";
			break;
		case CControld::TUXBOX_AVIACHIP_AVIA600:
			text << g_Locale->getText(LOCALE_UCODECHECK_AVIA600) << ": ";
			checkFile(UCODEDIR "/avia600.ux", (char*) &res);
			text << res << "\n";
			break;
	}
	text << g_Locale->getText(LOCALE_UCODECHECK_UCODE) << ": ";
	checkFile(UCODEDIR "/ucode.bin", (char*) &res);
	if (strcmp("not found", res) == 0)
		text << "ucode_0014 (built-in)";
	else
		text << res;
	text << "\n" << g_Locale->getText(LOCALE_UCODECHECK_CAM_ALPHA) << ": ";
	checkFile(UCODEDIR "/cam-alpha.bin", (char*) &res);
	text << res;

	ShowMsgUTF(LOCALE_UCODECHECK_HEAD, text.str(), CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
	return menu_return::RETURN_NONE;
}
#endif

int CDVBInfoExec::exec(CMenuTarget*, const std::string &)
{
	std::stringstream text;

//	text<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)addr[i]<<':';
	text << g_Locale->getText(LOCALE_TIMERLIST_MODETV) << ": " << CNeutrinoApp::getInstance()->channelListTV->getSize() << "\n";
	text << g_Locale->getText(LOCALE_TIMERLIST_MODERADIO) << ": " << CNeutrinoApp::getInstance()->channelListRADIO->getSize() << "\n \n";
	text << g_Locale->getText(LOCALE_SERVICEMENU_CHAN_EPG_STAT_EPG_STAT) << ":\n" << g_Sectionsd->getStatusinformation() << "\n";

	ShowMsgUTF(LOCALE_SERVICEMENU_CHAN_EPG_STAT, text.str(), CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
	return menu_return::RETURN_NONE;
}

unsigned long long getcurrenttime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);
}

//helper: returns a selectable tab entry from file 
std::string getFileEntryString(const char* filename, const std::string& filter_entry, const int& column_num)
{
	std::string ret = "";
	char line[256];
	std::ifstream in (filename, std::ios::in);

	if (!in) 
	{
		std::cerr<<__FUNCTION__ <<": error while open "<<filename<<" "<< strerror(errno)<<std::endl;
		return ret;
	}

	while (in.getline (line, 256))
	{
		std::string tab_line = (std::string)line, str_res;
		std::string::size_type loc = tab_line.find( filter_entry, 0 );

		if ( loc != std::string::npos ) 
		{
			std::stringstream stream(tab_line);

			for(int i = 0; i <= 10; i++)
			{
				stream >> str_res;
				if (i==column_num) 
				{
					ret = str_res;
					in.close();
					return ret;
				}

			}
		}
	}
	in.close();
	return ret;
}

//helper, returns pid of process name as string, if no pid found returns an empty string
std::string getPidof(const std::string& process_name)
{
	std::string ret = "";
	std::string p_filter = process_name;
	std::string p_name;
	DIR *dir;
	struct dirent *entry;
	
	dir = opendir(PROCDIR);
	if (dir)
	{
		do
		{
			entry = readdir(dir);
			if (entry)
			{
				std::string dir_x = entry->d_name;

				char filename[255];
				sprintf(filename,"%s/%s/status", PROCDIR, dir_x.c_str());

				if(access(filename, R_OK) ==0)
				{
					p_name = getFileEntryString(filename, "Name:", 2);
					if (p_name == p_filter)
					{
						closedir(dir);
						return dir_x;
					}
				}
			}
		}
        	while (entry);
	}
	closedir(dir);

	return ret;
}

//returns interface
std::string getInterface()
{
	std::string ifname = "eth0";
	std::string our_ip, our_mask, our_broadcast;

	CNetworkConfig  *network = CNetworkConfig::getInstance();
	
	if (network->inet_static)
	{
		our_ip = network->address;
	}
	else 	//Note: netGetIP returns also mask and broadcast, but not needed here 
		netGetIP(ifname, our_ip, our_mask, our_broadcast);
	
	return our_ip + "/24";
}

bool CTZChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	bool found = false;
	std::string name, zone, tz;
	printf("CTZChangeNotifier::changeNotify: %s\n", (char *) Data);

	xmlDocPtr parser = parseXmlFile("/etc/timezone.xml");
	if (parser != NULL)
	{
		xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
		while (search)
		{
			if (!strcmp(xmlGetName(search), "zone"))
			{
				name = xmlGetAttribute(search, "name");
				if(!strcmp(g_settings.timezone, name.c_str()))
				{
					zone = xmlGetAttribute(search, "zone");
					tz = xmlGetAttribute(search, "tz");
					found = true;
					break;
				}
			}
			search = search->xmlNextNode;
		}
		xmlFreeDoc(parser);
	}

	if(found)
	{
		std::string cmd = "cp /share/zoneinfo/" + zone + " /var/etc/localtime";
		system(cmd.c_str());
		setenv("TZ", tz.c_str(), 1);
		cmd = "export TZ=" + tz;
		FILE *f = fopen("/var/etc/TZ","w");

		if (f != NULL)
		{
			fputs(cmd.c_str(),f);
			putc('\n',f);
			fclose(f);
		}
	}

	return true;
}

