/*
  $Id: camdmenu.cpp, v2.3 2008/09/20 19:25:21 mohousch Exp $
*/

#include <global.h>
#include <neutrino.h>

#include <driver/screen_max.h>
#include "widget/hintbox.h"

#include "camdmenu.h"
#include "systeminfo.h"

CConfigFile ntp_config(',');
const std::string ntp_system_cmd_prefix = "/sbin/rdate -s ";
std::string ntpserver = ntp_config.getString("network_ntpserver", "time.mit.edu");
std::string ntp_system_cmd = ntp_system_cmd_prefix + ntpserver;

#define OPTIONS_OFF_ON_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF_ON_OPTIONS[OPTIONS_OFF_ON_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF  },
	{ 1, LOCALE_OPTIONS_ON }
};

CamdAuswahl::CamdAuswahl()
{
	frameBuffer = CFrameBuffer::getInstance();
	width = 600;
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;

	x = getScreenStartX (width);
	y = getScreenStartY (height);
}

int CamdAuswahl::exec(CMenuTarget* parent, const std::string & actionKey)
{
	int res = menu_return::RETURN_REPAINT;
	
	if(actionKey == "camdreset") 
	{
		this->CamdReset();
		return res;
	}
	
	if (parent)
	{
		parent->hide();
	}

	paint();

	Settings();
	
	return res;
}

void CamdAuswahl::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CamdAuswahl::paint()
{
	printf("[neutrino-rebuild] softcam emulators menu\n");
}

void CamdAuswahl::Settings()
{
	//oscam
	int oscam = 0;

	FILE* fdoscam = fopen("/var/etc/.oscam", "r");
	if (fdoscam)
	{
		oscam=1;
		fclose(fdoscam);
	}

	//mgcamd
	int mgcamd = 0;

	FILE* fdmgcamd = fopen("/var/etc/.mgcamd", "r");
	if(fdmgcamd)
	{
		mgcamd=1;
		fclose(fdmgcamd);
	}

	//evocamd
	int evocamd = 0;

	FILE* fdevocamd = fopen("/var/etc/.evocamd", "r");
	if(fdevocamd)
	{
		evocamd=1;
		fclose(fdevocamd);
	}

	//newcamd
	int newcamd = 0;

	FILE* fdnewcamd = fopen("/var/etc/.newcamd", "r");
	if(fdnewcamd)
	{
		newcamd=1;
		fclose(fdnewcamd);
	}

	//CCcam
	int CCcam = 0;

	FILE* fdCCcam = fopen("/var/etc/.cccam", "r");
	if(fdCCcam)
	{
		CCcam=1;
		fclose(fdCCcam);
	}

	//dynamic nnumeric
	int shortcut = 1;

	//MENU AUFBAUEN
	CMenuWidget* scSettings = new CMenuWidget(LOCALE_MAINMENU_EXTRASMENU, "lock.raw");
	scSettings->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_CAMDMENU_CAMDMENU));
	scSettings->addItem(GenericMenuSeparator);
	scSettings->addItem(GenericMenuBack);
	scSettings->addItem(GenericMenuSeparatorLine);

	//cam reset
	scSettings->addItem(new CMenuForwarder(LOCALE_CAMDMENU_CAMDRESET, true, "", this, "camdreset", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));

	//cam info
 	scSettings->addItem(new CMenuForwarder(LOCALE_CAMDMENU_INFO, true, NULL, new CBESysInfoWidget(5), NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

	//CCcam name checks
	FILE* fdCCcam_ppc_installed = fopen("/var/bin/emu/CCcam.ppc", "r");
	if(fdCCcam_ppc_installed)
	{
		int res = rename("/var/bin/emu/CCcam.ppc","/var/bin/emu/CCcam");
		if ( res == 0 )
			puts("CCcam.ppc renamed to CCcam");
		else
			perror("Error renaming CCcam.ppc");

		fclose(fdCCcam_ppc_installed);
	}

	FILE* fdcccam_installed = fopen("/var/bin/emu/cccam", "r");
	if(fdcccam_installed)
	{
		int res = rename("/var/bin/emu/cccam","/var/bin/emu/CCcam");
		if ( res == 0 )
			puts("cccam renamed to CCcam");
		else
			perror("Error renaming cccam");

		fclose(fdcccam_installed);
	}

	//camd auswahl
	FILE* fdoscam_installed = fopen("/var/bin/emu/oscam", "r");
	FILE* fdmgcamd_installed = fopen("/var/bin/emu/mgcamd", "r");
	FILE* fdevocamd_installed = fopen("/var/bin/emu/evocamd", "r");
	FILE* fdnewcamd_installed = fopen("/var/bin/emu/newcamd", "r");
	FILE* fdCCcam_installed = fopen("/var/bin/emu/CCcam", "r");

	CHintBox * CamdDetectionBox = new CHintBox(LOCALE_CAMDMENU_CAMDMENU,  g_Locale->getText(LOCALE_CAMDMENU_CAMDDETECT_HINT));
	CamdDetectionBox->paint();

	//oscam
	if(fdoscam_installed)
	{
		COscamDestChangeNotifier	 *OscamDestinationChanger = new COscamDestChangeNotifier;
		scSettings->addItem(new CMenuOptionChooser(LOCALE_CAMDMENU_OSCAM, &oscam, OPTIONS_OFF_ON_OPTIONS, OPTIONS_OFF_ON_OPTION_COUNT, true, OscamDestinationChanger, CRCInput::convertDigitToKey(shortcut++)));
		fclose(fdoscam_installed);
	}
	//mgcamd
	if(fdmgcamd_installed)
	{
		system("strings /var/bin/emu/mgcamd|grep 'v[0-9]..[0-9]' >>/tmp/emu_versions.txt");
		CMgCamdDestChangeNotifier	*MgCamdDestinationChanger = new CMgCamdDestChangeNotifier;
		scSettings->addItem(new CMenuOptionChooser(LOCALE_CAMDMENU_MGCAMD, &mgcamd, OPTIONS_OFF_ON_OPTIONS, OPTIONS_OFF_ON_OPTION_COUNT, true, MgCamdDestinationChanger, CRCInput::convertDigitToKey(shortcut++)));
		fclose(fdmgcamd_installed);
	}
	//evocamd
	if(fdevocamd_installed)
	{
		CEvoCamdDestChangeNotifier	*EvoCamdDestinationChanger = new CEvoCamdDestChangeNotifier;
		scSettings->addItem(new CMenuOptionChooser(LOCALE_CAMDMENU_EVOCAMD, &evocamd, OPTIONS_OFF_ON_OPTIONS, OPTIONS_OFF_ON_OPTION_COUNT, true, EvoCamdDestinationChanger, CRCInput::convertDigitToKey(shortcut++)));
		fclose(fdevocamd_installed);
	}
	//newcamd
	if(fdnewcamd_installed)
	{
		CNewCamdDestChangeNotifier	*NewCamdDestinationChanger = new CNewCamdDestChangeNotifier;
		scSettings->addItem(new CMenuOptionChooser(LOCALE_CAMDMENU_NEWCAMD, &newcamd, OPTIONS_OFF_ON_OPTIONS, OPTIONS_OFF_ON_OPTION_COUNT, true, NewCamdDestinationChanger, CRCInput::convertDigitToKey(shortcut++)));
		fclose(fdnewcamd_installed);
	}
	//CCcam
	if(fdCCcam_installed)
	{
		system("strings /var/bin/emu/CCcam|grep 'CCcam [0-9].[0-9].[0-9]' >>/tmp/emu_versions.txt");
		CCCcamDestChangeNotifier	*CCcamDestinationChanger = new CCCcamDestChangeNotifier;
		scSettings->addItem(new CMenuOptionChooser(LOCALE_CAMDMENU_CCCAM, &CCcam, OPTIONS_OFF_ON_OPTIONS, OPTIONS_OFF_ON_OPTION_COUNT, true, CCcamDestinationChanger, CRCInput::convertDigitToKey(shortcut++)));
		fclose(fdCCcam_installed);
	}

	CamdDetectionBox->hide();
	delete CamdDetectionBox;

	//show the emu version manually in "/var/etc/emu_versions.txt settings
	char oscamversion[7] = "N/A";
	char mgcamdversion[7] = "N/A";
	char evocamdversion[7] = "N/A";
	char newcamdversion[7] = "N/A";
	char CCcamversion[7]= "N/A";

	FILE *fdoscam_detected = fopen("/var/bin/emu/oscam", "r");
	FILE *fdmgcamd_detected = fopen("/var/bin/emu/mgcamd", "r");
	FILE *fdevocamd_detected = fopen("/var/bin/emu/evocamd", "r");
	FILE *fdnewcamd_detected = fopen("/var/bin/emu/newcamd", "r");
	FILE *fdCCcam_detected = fopen("/var/bin/emu/CCcam", "r");

	//show emu version if one emu is present
	if((fdoscam_detected) || (fdmgcamd_detected) || (fdevocamd_detected) || (fdnewcamd_detected) || (fdCCcam_detected))
	{
		scSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_CAMDMENU_CAMDVERSION));
	}
	
	//get auto detected emu version
	FILE *tmp_emu_versions = fopen("/tmp/emu_versions.txt", "r");
	if (tmp_emu_versions) 
	{
		char buffer[120];
		while(fgets(buffer, 120, tmp_emu_versions)!=NULL)
		{
			sscanf(buffer, "v%7s", mgcamdversion);
			sscanf(buffer, "CCcam %7s", CCcamversion);
		}
		fclose(tmp_emu_versions);
	}

	//get manual emu version, overriding auto detected version
	FILE *emu_versions = fopen("/var/etc/emu_versions.txt", "r");
	if (emu_versions) 
	{
		char buffer[120];
		while(fgets(buffer, 120, emu_versions)!=NULL)
		{
			sscanf(buffer, "oscam=%7s", oscamversion);
			sscanf(buffer, "mgcamd=%7s", mgcamdversion);
			sscanf(buffer, "evocamd=%7s", evocamdversion);
			sscanf(buffer, "newcamd=%7s", newcamdversion);
			sscanf(buffer, "CCcam=%7s", CCcamversion);
		}
		fclose(emu_versions);
	}

	if (fdoscam_detected)
	{
		scSettings->addItem(new CMenuForwarder("OScam Version", false, oscamversion));
		fclose(fdoscam_detected);
	}

	if (fdmgcamd_detected)
	{
		scSettings->addItem(new CMenuForwarder("MgCamd Version", false, mgcamdversion));
		fclose(fdmgcamd_detected);
	}

	if (fdevocamd_detected)
	{
		scSettings->addItem(new CMenuForwarder("EvoCamd Version", false, evocamdversion));
		fclose(fdevocamd_detected);
	}

	if (fdnewcamd_detected)
	{
		scSettings->addItem(new CMenuForwarder("NewCamd Version", false, newcamdversion));
		fclose(fdnewcamd_detected);
	}

	if (fdCCcam_detected)
	{
		scSettings->addItem(new CMenuForwarder("CCcam Version", false, CCcamversion));
		fclose(fdCCcam_detected);
	}

	scSettings->exec (NULL, "");
	scSettings->hide ();
	delete scSettings;
}

bool CamdAuswahl::CamdReset()
{
	//oscam
	int oscam = 0;

	FILE* fdoscam = fopen("/var/etc/.oscam", "r");
	if(fdoscam)
	{
		oscam=1;
		fclose(fdoscam);
	}

	//mgcamd
	int mgcamd = 0;

	FILE* fdmgcamd = fopen("/var/etc/.mgcamd", "r");
	if(fdmgcamd)
	{
		mgcamd=1;
		fclose(fdmgcamd);
	}

	//evocamd
	int evocamd = 0;

	FILE* fdevocamd = fopen("/var/etc/.evocamd", "r");
	if(fdevocamd)
	{
		evocamd=1;
		fclose(fdevocamd);
	}

	//newcamd
	int newcamd = 0;

	FILE* fdnewcamd = fopen("/var/etc/.newcamd", "r");
	if(fdnewcamd)
	{
		newcamd=1;
		fclose(fdnewcamd);
	}

	//CCcam
	int CCcam = 0;

	FILE* fdCCcam = fopen("/var/etc/.cccam", "r");
	if(fdCCcam)
	{
		CCcam=1;
		fclose(fdCCcam);
	}

	CHintBox * CamdResetBox = new CHintBox(LOCALE_CAMDMENU_CAMDRESET, g_Locale->getText(LOCALE_CAMDMENU_CAMDRESET_HINT));
	CamdResetBox->paint();

	/* stop cams
	   ######### */

	//oscam
	if (oscam == 1)
	{
		system("touch /tmp/oscam.kill");
		system("killall -9 oscam");
		system("sleep 2");
	}

	//mgcamd
	if (mgcamd == 1)
	{
		system("kill $( cat /tmp/mgcamd.pid )");
		system("killall -9 epg-pause");
		system("killall -9 mgcamd");
		system("sleep 2");
	}

	//evocamd
	if (evocamd == 1)
	{
		system("kill $( pidof evocamd )");
		system("killall -9 evocamd");
		system("sleep 2");
	}

	//newcamd
	if (newcamd == 1)
	{
		system("kill $( cat /tmp/newcamd.pid )");
		system("killall -9 newcamd");
		system("sleep 2"); 
	}

	//CCcam
	if (CCcam == 1)
	{
		system("killall -9 CCcam");
		system("killall -9 epg-restart");
		system("sleep 2");
	}

	/* start cams
	   ########## */

	//oscam
	if (oscam == 1)
	{
		system(ntp_system_cmd.c_str());
		system("sleep 1");
		system("oscam &");
		system("sleep 6");
	}

	//mgcamd
	if (mgcamd == 1)
	{
		system("epg-pause");
		system("sleep 1");
		system("mgcamd");
		system("sleep 1");
	}

	//evocamd
	if (evocamd == 1)
	{
		system("evocamd");
		system("sleep 1");
	}

	//newcamd
	if (newcamd == 1)
	{
		system("newcamd");
		system("sleep 1"); 
	}

	//CCcam
	if (CCcam == 1)
	{
		system("epg-restart &");
		system("sleep 1");
		system("CCcam &");
		system("sleep 1");
	}

	//rezap and delete msgbox
	CChannelList  *channelList;
	channelList = CNeutrinoApp::getInstance()->channelList;
	CNeutrinoApp::getInstance()->channelList->ReZap();

	CamdResetBox->hide();
	delete CamdResetBox;
}

bool COscamDestChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	//oscam
	int oscam = *(int *)Data;
	
	if (oscam == 1)
	{
		system("touch /var/etc/.oscam");
		system("chmod 755 /var/bin/emu/oscam");
		system(ntp_system_cmd.c_str());
		system("sleep 1");
		system("oscam &");
		system("sleep 1");
		CChannelList  *channelList;
		channelList = CNeutrinoApp::getInstance()->channelList;
		CNeutrinoApp::getInstance()->channelList->ReZap();
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_OSCAMSTART), 450, 2); // UTF-8("")
	}
	else
	{
		system("touch /tmp/oscam.kill");
		system("killall -9 oscam");
		system("rm /var/etc/.oscam");
		system("rm /tmp/ecm.info");
		system("rm /tmp/oscam.ver");
		system("rm /tmp/pid.info");
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_OSCAMSTOP), 450, 2); // UTF-8("")
	}
	return true;
}

bool CMgCamdDestChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	//MgCamd
	int mgcamd = *(int *)Data;
	
	if (mgcamd == 1)
	{
		system("touch /var/etc/.mgcamd");
		system("chmod 755 /var/bin/emu/mgcamd");
		system("epg-pause");
		system("sleep 1");
		system("mgcamd");
		system("sleep 1");
		CChannelList  *channelList;
		channelList = CNeutrinoApp::getInstance()->channelList;
		CNeutrinoApp::getInstance()->channelList->ReZap();
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_MGCAMDSTART), 450, 2); // UTF-8("")
	}
	else
	{
		system("kill $( cat /tmp/mgcamd.pid )");
		system("killall -9 mgcamd");
		system("killall -9 epg-pause");
		system("rm /var/etc/.mgcamd");
		system("rm /tmp/ecm.info");
		system("rm /tmp/mgshare.info");
		system("rm /tmp/mgstat.info");
		system("rm /tmp/pid.info");
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_MGCAMDSTOP), 450, 2); // UTF-8("")
	}
	return true;
}

bool CEvoCamdDestChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	//EvoCamd
	int evocamd = *(int *)Data;
	
	if (evocamd == 1)
	{
		system("touch /var/etc/.evocamd");
		system("chmod 755 /var/bin/emu/evocamd");
		system("evocamd");
		system("sleep 1");
		CChannelList  *channelList;
		channelList = CNeutrinoApp::getInstance()->channelList;
		CNeutrinoApp::getInstance()->channelList->ReZap();
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_EVOCAMDSTART), 450, 2); // UTF-8("")
	}
	else
	{
		system("kill $( pidof evocamd )");
		system("killall -9 evocamd");
		system("rm /var/etc/.evocamd");
		system("rm /tmp/ecm.info");
		system("rm /tmp/pid.info");
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_EVOCAMDSTOP), 450, 2); // UTF-8("")
	}
	return true;
}

bool CNewCamdDestChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	//NewCamd
	int newcamd = *(int *)Data;
	
	if (newcamd == 1)
	{
		system("touch /var/etc/.newcamd");
		system("chmod 755 /var/bin/emu/newcamd");
		system("newcamd");
		system("sleep 1");
		CChannelList  *channelList;
		channelList = CNeutrinoApp::getInstance()->channelList;
		CNeutrinoApp::getInstance()->channelList->ReZap();
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_NEWCAMDSTART), 450, 2); // UTF-8("")
	}
	else
	{
		system("kill $( cat /tmp/newcamd.pid )");
		system("killall -9 newcamd");
		system("rm /var/etc/.newcamd");
		system("rm /tmp/cainfo.txt");
		system("rm /tmp/pid.info");
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_NEWCAMDSTOP), 450, 2); // UTF-8("")
	}
	return true;
}

bool CCCcamDestChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	//CCcam
	int CCcam = *(int *)Data;
	
	if (CCcam == 1)
	{
		system("touch /var/etc/.cccam");
		system("epg-restart &");
		system("sleep 1");
		system("chmod 755 /var/bin/emu/CCcam");
		system("CCcam &");
		system("sleep 1");
		CChannelList  *channelList;
		channelList = CNeutrinoApp::getInstance()->channelList;
		CNeutrinoApp::getInstance()->channelList->ReZap();
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_CCCAMSTART), 450, 2); // UTF-8("")
	}
	else
	{
		system("killall -9 CCcam");
		system("killall -9 epg-restart");
		system("rm /var/etc/.cccam");
		system("rm /tmp/ecm.info");
		system("rm /tmp/ecm0.info");
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_CAMDMENU_CCCAMSTOP), 450, 2); // UTF-8("")
	}
	return true;
}

