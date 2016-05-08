/*
	$Id: network_setup.cpp,v 1.26 2012/09/12 07:25:12 rhabarber1848 Exp $

	network setup implementation - Neutrino-GUI

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "libnet.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>

#include "gui/network_setup.h"

#ifndef DISABLE_INTERNET_UPDATE
#ifndef HAVE_DREAMBOX_HARDWARE
#include "gui/proxyserver_setup.h"
#endif /*HAVE_DREAMBOX_HARDWARE*/
#endif /*DISABLE_INTERNET_UPDATE*/

#ifdef ENABLE_GUI_MOUNT
#include "gui/nfs.h"
#endif /*ENABLE_GUI_MOUNT*/

// TODO: k26 support for ENABLE_DRIVE_GUI, it's disabled with -enable-kernel26 yet
#if defined ENABLE_DRIVE_GUI && defined ENABLE_SAMBASERVER 
#include "gui/sambaserver_setup.h"
#endif /*ENABLE_DRIVE_GUI && defined ENABLE_SAMBASERVER*/

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/hintbox.h>

#include <driver/screen_max.h>

#include <system/debug.h>

extern int pinghost (const std::string &hostname, std::string *ip = NULL);

CNetworkSetup::CNetworkSetup()
{
	networkConfig = CNetworkConfig::getInstance();

	width = w_max (500, 100);
	selected = -1;

	network_automatic_start = old_network_automatic_start = networkConfig->automatic_start;
	network_dhcp            = old_network_dhcp            = networkConfig->inet_static ? NETWORK_DHCP_OFF : NETWORK_DHCP_ON;
	network_address         = old_network_address         = networkConfig->address;
	network_netmask         = old_network_netmask         = networkConfig->netmask;
	network_broadcast                                     = networkConfig->broadcast;
	network_nameserver      = old_network_nameserver      = networkConfig->nameserver;
	network_gateway         = old_network_gateway         = networkConfig->gateway;
}

CNetworkSetup::~CNetworkSetup()
{

}

int CNetworkSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}
	
	if(actionKey=="networkapply")
	{
		applyNetworkSettings();
		return res;
	}
	else if(actionKey=="networktest")
	{
		printf("[network setup] doing network test...\n");
		testNetworkSettings();
		return res;
	}
	else if(actionKey=="networkshow")
	{
		dprintf(DEBUG_INFO, "show current network settings...\n");
		showCurrentNetworkSettings();
		return res;
	}
	else if(actionKey=="networksave")
	{
		saveNetworkSettings();
		return res;
	}

	printf("[neutrino] init network setup...\n");
	res = showNetworkSetup();
	
	return res;
}

void CNetworkSetup::setBroadcast(void)
{
	in_addr_t na = inet_addr(network_address.c_str());
	in_addr_t nm = inet_addr(network_netmask.c_str());
	struct in_addr in;
	in.s_addr = na | ~nm;
	char tmp[40];
	network_broadcast = (inet_ntop(AF_INET, &in, tmp, sizeof(tmp))) ? std::string(tmp) : "0.0.0.0";
}

#define OPTIONS_NTPENABLE_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_NTPENABLE_OPTIONS[OPTIONS_NTPENABLE_OPTION_COUNT] =
{
	{ CNetworkSetup::NETWORK_NTP_OFF, LOCALE_OPTIONS_NTP_OFF },
	{ CNetworkSetup::NETWORK_NTP_ON, LOCALE_OPTIONS_NTP_ON }
};

int CNetworkSetup::showNetworkSetup()
{
	bool loop = true;
	int res = menu_return::RETURN_REPAINT;

	while (loop)
	{		
		//menue init
		CMenuWidget* networkSettings = new CMenuWidget(LOCALE_MAINSETTINGS_HEAD, NEUTRINO_ICON_SETTINGS, width);
		networkSettings->setPreselected(selected);

		//apply button
		CMenuForwarder *m0 = new CMenuForwarder(LOCALE_NETWORKMENU_SETUPNOW, true, NULL, this, "networkapply", CRCInput::RC_red);
	
		//prepare input entries
		CIPInput networkSettings_NetworkIP  (LOCALE_NETWORKMENU_IPADDRESS , network_address   , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, this);
		CIPInput networkSettings_NetMask    (LOCALE_NETWORKMENU_NETMASK   , network_netmask   , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, this);
		CIPInput networkSettings_Gateway    (LOCALE_NETWORKMENU_GATEWAY   , network_gateway   );
		CIPInput networkSettings_NameServer (LOCALE_NETWORKMENU_NAMESERVER, network_nameserver);

		//auto start
		CMenuOptionChooser* o1 = new CMenuOptionChooser(LOCALE_NETWORKMENU_SETUPONSTARTUP, &network_automatic_start, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

		//dhcp
		network_dhcp 	= networkConfig->inet_static ? NETWORK_DHCP_OFF : NETWORK_DHCP_ON;
	
		CMenuForwarder *m1 = new CMenuForwarder(LOCALE_NETWORKMENU_IPADDRESS , networkConfig->inet_static, network_address   , &networkSettings_NetworkIP );
		CMenuForwarder *m2 = new CMenuForwarder(LOCALE_NETWORKMENU_NETMASK   , networkConfig->inet_static, network_netmask   , &networkSettings_NetMask   );
		setBroadcast();
		CMenuForwarder *m3 = new CMenuForwarder(LOCALE_NETWORKMENU_BROADCAST , false                     , network_broadcast );
		CMenuForwarder *m4 = new CMenuForwarder(LOCALE_NETWORKMENU_GATEWAY   , networkConfig->inet_static, network_gateway   , &networkSettings_Gateway   );
		CMenuForwarder *m5 = new CMenuForwarder(LOCALE_NETWORKMENU_NAMESERVER, networkConfig->inet_static, network_nameserver, &networkSettings_NameServer);
		
		CDHCPNotifier dhcpNotifier(m1,m2,m4,m5);
		CMenuOptionChooser* o2 = new CMenuOptionChooser(LOCALE_NETWORKMENU_DHCP, &network_dhcp, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, &dhcpNotifier);
		
		//paint menu items
		//intros
		networkSettings->addIntroItems(LOCALE_MAINSETTINGS_NETWORK);
	
		networkSettings->addItem( m0 );
	
		networkSettings->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_TEST, true, NULL, this, "networktest", CRCInput::RC_green));
		networkSettings->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_SHOW, true, NULL, this, "networkshow", CRCInput::RC_help));
		networkSettings->addItem(GenericMenuSeparatorLine);
	
		networkSettings->addItem(o1);
		networkSettings->addItem(GenericMenuSeparatorLine);
		networkSettings->addItem(o2);
		networkSettings->addItem(GenericMenuSeparatorLine);
	
		networkSettings->addItem( m1);
		networkSettings->addItem( m2);
		networkSettings->addItem( m3);
	
		networkSettings->addItem(GenericMenuSeparatorLine);
		networkSettings->addItem( m4);
		networkSettings->addItem( m5);
		networkSettings->addItem(GenericMenuSeparatorLine);
		
		//ntp
		//prepare ntp input
		CStringInputSMS networkSettings_NtpServer(LOCALE_NETWORKMENU_NTPSERVER, &g_settings.network_ntpserver, 30, false, LOCALE_NETWORKMENU_NTPSERVER_HINT1, LOCALE_NETWORKMENU_NTPSERVER_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789-. ", this);
	
		CStringInput networkSettings_NtpRefresh(LOCALE_NETWORKMENU_NTPREFRESH, &g_settings.network_ntprefresh, 3, false, LOCALE_NETWORKMENU_NTPREFRESH_HINT1, LOCALE_NETWORKMENU_NTPREFRESH_HINT2 , "0123456789 ", this);

		CMenuWidget* ntp = new CMenuWidget(LOCALE_MAINSETTINGS_NETWORK, NEUTRINO_ICON_SETTINGS, width);
		networkSettings->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_NTPTITLE, true, NULL, ntp, NULL, CRCInput::RC_yellow));
		ntp->addIntroItems(LOCALE_NETWORKMENU_NTPTITLE);
		CMenuOptionChooser *ntp1 = new CMenuOptionChooser(LOCALE_NETWORKMENU_NTPENABLE, &g_settings.network_ntpenable, OPTIONS_NTPENABLE_OPTIONS, OPTIONS_NTPENABLE_OPTION_COUNT, true, this);
		CMenuForwarder *ntp2 = new CMenuForwarder(LOCALE_NETWORKMENU_NTPSERVER, true, g_settings.network_ntpserver, &networkSettings_NtpServer);
		CMenuForwarder *ntp3 = new CMenuForwarder(LOCALE_NETWORKMENU_NTPREFRESH, true, g_settings.network_ntprefresh, &networkSettings_NtpRefresh);
		
		ntp->addItem( ntp1);
		ntp->addItem( ntp2);
		ntp->addItem( ntp3);

	#ifdef ENABLE_GUI_MOUNT
		CMenuWidget* networkmounts = new CMenuWidget(LOCALE_MAINSETTINGS_NETWORK, NEUTRINO_ICON_SETTINGS, width);
		networkSettings->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_MOUNT, true, NULL, networkmounts, NULL, CRCInput::RC_blue));
		networkmounts->addIntroItems(LOCALE_NETWORKMENU_MOUNT);

		CNFSMountGui* nfsMountGui = new CNFSMountGui();
		networkmounts->addItem(new CMenuForwarder(LOCALE_NFS_MOUNT, true, NULL, nfsMountGui, NULL, CRCInput::RC_red));

		CNFSUmountGui* nfsUmountGui = new CNFSUmountGui();
		networkmounts->addItem(new CMenuForwarder(LOCALE_NFS_UMOUNT, true, NULL, nfsUmountGui, NULL, CRCInput::RC_green));
	#endif

	#ifndef DISABLE_INTERNET_UPDATE
	#ifndef HAVE_DREAMBOX_HARDWARE
		//proxyserver
		CProxySetup* proxySetup = new CProxySetup(LOCALE_MAINSETTINGS_NETWORK);
		networkSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYSERVER_SEP, true, NULL, proxySetup, NULL, CRCInput::RC_0));
 	#endif 
	#endif

	#if defined ENABLE_DRIVE_GUI && defined ENABLE_SAMBASERVER
		CSambaSetup* sambaSetup = new CSambaSetup(LOCALE_MAINSETTINGS_NETWORK);
		networkSettings->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_SAMBA, true, NULL, sambaSetup, NULL, CRCInput::RC_1));
	#endif

		res = networkSettings->exec(NULL, "");
		selected = networkSettings->getSelected();
		delete networkSettings;

		delete ntp;
	#ifdef ENABLE_GUI_MOUNT
		delete networkmounts;
		delete nfsMountGui;
		delete nfsUmountGui;
	#endif
	#ifndef DISABLE_INTERNET_UPDATE
	#ifndef HAVE_DREAMBOX_HARDWARE
		delete proxySetup;
 	#endif 
	#endif
	#if defined ENABLE_DRIVE_GUI && defined ENABLE_SAMBASERVER
		delete sambaSetup;
	#endif

		// Check for changes
 		loop = settingsChanged();
 	}

	return res;
}

//returns true, if any settings were changed
bool CNetworkSetup::settingsChanged()
{
	bool ret = false;

	if (networkConfig->modified_from_orig())
	{
		//open message box
		ret =  saveChangesDialog();
	}
	else
	{	
		ret = false;
	}

	return ret;
}

//prepares internal settings before commit
void CNetworkSetup::prepareSettings()
{
	networkConfig->automatic_start 	= network_automatic_start;
	networkConfig->inet_static 	= (network_dhcp ? false : true);
	networkConfig->address 		= network_address;
	networkConfig->netmask 		= network_netmask;
	networkConfig->broadcast 	= network_broadcast;
	networkConfig->gateway 		= network_gateway;
	networkConfig->nameserver 	= network_nameserver;
}

//check for ip-address, if dhcp disabled, returns false if no address definied and show message
bool CNetworkSetup::checkForIP()
{
	bool ret = true;

	if (!network_dhcp && network_address.empty()) //no ip definied
	{
		ShowLocalizedMessage(LOCALE_MAINSETTINGS_NETWORK,
		                     LOCALE_NETWORKMENU_ERROR_NO_ADDRESS,
		                     CMessageBox::mbrBack,
		                     CMessageBox::mbBack,
		                     NEUTRINO_ICON_ERROR,
		                     width);
		ret = false;
	}

	return ret;
}

const char * CNetworkSetup::mypinghost(std::string &host)
{
	int retvalue = pinghost(host);
	switch (retvalue)
	{
		case 1: return (g_Locale->getText(LOCALE_PING_OK));
		case 0: return (g_Locale->getText(LOCALE_PING_UNREACHABLE));
		case -1: return (g_Locale->getText(LOCALE_PING_PROTOCOL));
		case -2: return (g_Locale->getText(LOCALE_PING_SOCKET));
	}
	return "";
}

//saves settings without apply, reboot is required 
void CNetworkSetup::saveNetworkSettings(bool show_message)
{
	printf("[network setup] saving current network settings...\n");

	prepareSettings();

  	networkConfig->commitConfig();

	if (show_message)
		ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO,
				  LOCALE_NETWORKMENU_SETUPSAVED,
				  width,
				  g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR],
				  NEUTRINO_ICON_INFO);
}

//saves settings and apply, reboot not required
void CNetworkSetup::applyNetworkSettings()
{
	printf("[network setup] apply network settings...\n");

	CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_NETWORKMENU_APPLY_SETTINGS)); // UTF-8
	hintBox->paint();

	prepareSettings();

	networkConfig->stopNetwork();
	networkConfig->commitConfig();
	networkConfig->startNetwork();

	hintBox->hide();
	delete hintBox;
}

//open a message dialog with three buttons,
//yes:		applies networksettings and exit network setup
//no:		saves networksettings and exit network setup
//back: 	exit message dialog, goes back to network setup 
int CNetworkSetup::saveChangesDialog()
{
	if (!checkForIP())
		return 1;	

	// Save the settings after changes, if user wants to!
	int result = ShowLocalizedMessage(LOCALE_MAINSETTINGS_NETWORK,
					  LOCALE_NETWORKMENU_APPLY_SETTINGS_NOW,
					  CMessageBox::mbrYes,
					  CMessageBox::mbYes | CMessageBox::mbNo | CMessageBox::mbBack,
					  NEUTRINO_ICON_QUESTION,
					  width);
	
	switch(result)
	{
		case CMessageBox::mbrYes:
			applyNetworkSettings();
			return 0;
			break;
	
		case CMessageBox::mbrNo:
			saveNetworkSettings(true);
			return 0;
			break;
	
		default:
			restoreNetworkSettings(true);
			return 1;
			break;
	}
}

//restores settings without any changes if user wants to
void CNetworkSetup::restoreNetworkSettings(bool show_message)
{
	int result = CMessageBox::mbrNo;

	if (show_message)
	{
		result = ShowLocalizedMessage(LOCALE_MAINSETTINGS_NETWORK,
					      LOCALE_NETWORKMENU_RESET_SETTINGS_NOW,
					      CMessageBox::mbrNo,
					      CMessageBox::mbYes | CMessageBox::mbNo,
					      NEUTRINO_ICON_QUESTION,
					      width);
	}

	if (result == CMessageBox::mbrYes)
	{
		network_automatic_start	= old_network_automatic_start;
		network_dhcp		= old_network_dhcp;
		network_address		= old_network_address;
		network_netmask		= old_network_netmask;
		setBroadcast();
		network_nameserver	= old_network_nameserver;
		network_gateway		= old_network_gateway;

		networkConfig->automatic_start 	= network_automatic_start;
		networkConfig->inet_static 	= (network_dhcp ? false : true);
		networkConfig->address 		= network_address;
		networkConfig->netmask 		= network_netmask;
		networkConfig->broadcast 	= network_broadcast;
		networkConfig->gateway 		= network_gateway;
		networkConfig->nameserver 	= network_nameserver;

		networkConfig->commitConfig();
	}

}

void CNetworkSetup::testNetworkSettings()
{
	std::string ifname = "eth0";
	std::string our_ip, our_mask, our_broadcast, our_gateway, our_nameserver;

	std::string text, ethID, testsite, offset = "    ";

	//set default testdomain and wiki-IP
	std::string defaultsite = "www.google.de", wiki_IP = "91.224.67.93", wiki_URL = "wiki.tuxbox.org";
	
	//set physical adress
	std::string mac_addr = networkConfig->mac_addr;
	
	//get www-domain testsite from /.version 	
	CConfigFile config('\t');
	config.loadConfig("/.version");
	testsite = config.getString("homepage",defaultsite);	
	testsite.replace( 0, testsite.find("www",0), "" );
	
	//use default testdomain if testsite missing
	if (testsite.empty())
		testsite = defaultsite; 

	if (networkConfig->inet_static) {
		our_ip = networkConfig->address;
		our_mask = networkConfig->netmask;
		our_broadcast = networkConfig->broadcast;
		our_gateway = networkConfig->gateway;
		our_nameserver = networkConfig->nameserver;
	}
	else {
		netGetIP(ifname, our_ip, our_mask, our_broadcast);
		netGetDefaultRoute(our_gateway);
		netGetNameserver(our_nameserver);
	}
	
	printf("testNw IP: %s\n", our_ip.c_str());
	printf("testNw MAC-address: %s\n", mac_addr.c_str());
	printf("testNw Netmask: %s\n", our_mask.c_str());
	printf("testNw Broadcast: %s\n", our_broadcast.c_str());
	printf("testNw Gateway: %s\n", our_gateway.c_str());
	printf("testNw Nameserver: %s\n", our_nameserver.c_str());
	printf("testNw Testsite %s\n", testsite.c_str());

	if (our_ip.empty())
	{
		text = g_Locale->getText(LOCALE_NETWORKMENU_INACTIVE);
	}
	else
	{
		// Box
		text = "dbox (" + mac_addr + "):\n";
		text += offset + our_ip + ": " + mypinghost(our_ip) + "\n";
		// Gateway
		text += (std::string)g_Locale->getText(LOCALE_NETWORKMENU_GATEWAY) + " (Router):\n";
		text += offset + our_gateway + ": " + " " + mypinghost(our_gateway) + "\n";
		// Nameserver
		text += (std::string)g_Locale->getText(LOCALE_NETWORKMENU_NAMESERVER) + ":\n";
		text += offset + our_nameserver + ": " + " " + mypinghost(our_nameserver) + "\n";
		// Wiki
		text += wiki_URL + ":\n";
		text += offset + "via IP (" + wiki_IP + "): " + mypinghost(wiki_IP) + "\n";
		if (pinghost(our_nameserver) == 1)
		{
			text += offset + "via DNS: " + mypinghost(wiki_URL) + "\n";
			text += testsite + ":\n";
			text += offset + "via DNS: " + mypinghost(testsite) + "\n";
		}
	}

	ShowMsgUTF(LOCALE_NETWORKMENU_TEST, text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
}

void CNetworkSetup::showCurrentNetworkSettings()
{
	std::string ifname = "eth0";
	std::string ip, mask, broadcast, router, nameserver, text;
	netGetIP(ifname, ip, mask, broadcast);

	if (ip.empty()) {
		text = g_Locale->getText(LOCALE_NETWORKMENU_INACTIVE);
	}
	else {
		netGetNameserver(nameserver);
		netGetDefaultRoute(router);
		std::string dhcp = networkConfig->inet_static ? g_Locale->getText(LOCALE_OPTIONS_OFF) : g_Locale->getText(LOCALE_OPTIONS_ON);

		text = (std::string)g_Locale->getText(LOCALE_NETWORKMENU_DHCP) + ": " + dhcp + '\n'
				  + g_Locale->getText(LOCALE_NETWORKMENU_IPADDRESS ) + ": " + ip + '\n'
				  + g_Locale->getText(LOCALE_NETWORKMENU_NETMASK   ) + ": " + mask + '\n'
				  + g_Locale->getText(LOCALE_NETWORKMENU_BROADCAST ) + ": " + broadcast + '\n'
				  + g_Locale->getText(LOCALE_NETWORKMENU_NAMESERVER) + ": " + nameserver + '\n'
				  + g_Locale->getText(LOCALE_NETWORKMENU_GATEWAY   ) + ": " + router;
	}
	ShowMsgUTF(LOCALE_NETWORKMENU_SHOW, text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
}

bool CNetworkSetup::changeNotify(const neutrino_locale_t OptionName, void * /*Data*/)
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_NETWORKMENU_IPADDRESS) ||
	    ARE_LOCALES_EQUAL(OptionName, LOCALE_NETWORKMENU_NETMASK))
	{
		setBroadcast();
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_NETWORKMENU_NTPSERVER) ||
	         ARE_LOCALES_EQUAL(OptionName, LOCALE_NETWORKMENU_NTPREFRESH) ||
	         ARE_LOCALES_EQUAL(OptionName, LOCALE_NETWORKMENU_NTPENABLE))
	{
		CNeutrinoApp::getInstance()->SendSectionsdConfig();
	}
	return false;
}

CDHCPNotifier::CDHCPNotifier(CMenuForwarder* a1, CMenuForwarder* a2, CMenuForwarder* a3, CMenuForwarder* a4)
{
	toDisable[0] = a1;
	toDisable[1] = a2;
	toDisable[2] = a3;
	toDisable[3] = a4;
}

bool CDHCPNotifier::changeNotify(const neutrino_locale_t, void * data)
{
	CNetworkConfig::getInstance()->inet_static = ((*(int*)(data)) == CNetworkSetup::NETWORK_DHCP_OFF);
	for (int x = 0; x < 4; x++)
		toDisable[x]->setActive(CNetworkConfig::getInstance()->inet_static);	
	return false;
}
