/*
	$Id: update.cpp,v 1.149 2012/09/23 08:18:03 rhabarber1848 Exp $

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/update.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>

#include <gui/color.h>
#include <gui/filebrowser.h>
#ifdef ENABLE_GUI_MOUNT
#include <system/fsmounter.h>
#endif
#include <gui/imageinfo.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>

#include <system/helper.h>
#include <system/flashtool.h>
#include <sectionsdclient/sectionsdclient.h>
#ifndef DISABLE_INTERNET_UPDATE
#include <system/httptool.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>

//#define TESTING

#define gTmpPath "/tmp/"
#define gUserAgent "neutrino/softupdater 1.2"

#define LIST_OF_UPDATES_LOCAL_FILENAME "cramfs.list"
#define UPDATE_LOCAL_FILENAME          "update.cramfs"
#define RELEASE_CYCLE                  "2.0"
#define FILEBROWSER_UPDATE_FILTER      "cramfs"
#define FILEBROWSER_UPDATE_FILTER_ALT  "squashfs"
//#define MTD_OF_WHOLE_IMAGE             4
#ifdef HAVE_DREAMBOX_HARDWARE
#define MTD_TEXT_OF_WHOLE_IMAGE		"DreamBOX cramfs+squashfs"
#else
#define MTD_TEXT_OF_WHOLE_IMAGE		"Flash without bootloader"
#define MTD_DEVICE_OF_UPDATE_PART      "/dev/mtd/2"
#endif


CFlashUpdate::CFlashUpdate()
	:CProgressWindow()
{
	setTitle(LOCALE_FLASHUPDATE_HEAD);
}



class CUpdateMenuTarget : public CMenuTarget
{
	int    myID;
	int *  myselectedID;

public:
	CUpdateMenuTarget(const int id, int * const selectedID)
		{
			myID = id;
			myselectedID = selectedID;
		}

	virtual int exec(CMenuTarget *, const std::string &)
		{
			*myselectedID = myID;
			return menu_return::RETURN_EXIT_ALL;
		}
};


#ifndef DISABLE_INTERNET_UPDATE
bool CFlashUpdate::selectHttpImage(void)
{
	CHTTPTool httpTool;
	std::string url;
	std::string md5;
	std::string name;
	std::string version;
	std::vector<std::string> updates_lists, urls, md5s, names, versions, descriptions;
	std::vector<CUpdateMenuTarget*> update_menu_targets;
	int selected = -1, listWidth = w_max (710, 50);

	// get default update url from .version
	CConfigFile config('\t');
	config.loadConfig("/.version");
	const std::string updateURL  = config.getString("update",  "");

	httpTool.setStatusViewer(this);
	showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_GETINFOFILE)); // UTF-8

	CMenuWidget SelectionWidget(LOCALE_SERVICEMENU_UPDATE, NEUTRINO_ICON_UPDATE, listWidth);
	SelectionWidget.addItem(new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_FLASHUPDATE_SELECTIMAGE));
	SelectionWidget.addItem(GenericMenuSeparator);
	SelectionWidget.addItem(GenericMenuBack);

	std::ifstream urlFile(g_settings.softupdate_url_file.c_str());

	unsigned int i = 0;
	bool update_prefix_tried = false;
	while (urlFile >> url)
	{
		// add update url from .version if exists, then seek back to start
		if (!updateURL.empty() && !update_prefix_tried)
		{
			url = updateURL + url;
			urlFile.seekg(0, std::ios::beg);
			update_prefix_tried = true;
		}

		std::string::size_type startpos, endpos;

		/* extract domain name */
		startpos = url.find("//");
		if (startpos == std::string::npos)
		{
			startpos = 0;
			endpos   = std::string::npos;
		}
		else
		{
			startpos += 2;
			endpos    = url.find('/', startpos);
		}
		updates_lists.push_back(url.substr(startpos, endpos - startpos));

		CMenuSeparator* sep = new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_FLASHUPDATE_SELECTIMAGE);
		sep->setString(updates_lists.back());
		SelectionWidget.addItem(sep);

		if (httpTool.downloadFile(url, gTmpPath LIST_OF_UPDATES_LOCAL_FILENAME, 20))
		{
			std::ifstream in(gTmpPath LIST_OF_UPDATES_LOCAL_FILENAME);
			while (in >> url >> md5 >> version >> std::ws)
			{
				urls.push_back(url);
				md5s.push_back(md5);
				versions.push_back(version);
				std::getline(in, name);
				names.push_back(name);

				CFlashVersionInfo versionInfo(versions[i]);

				std::string description = versionInfo.getType();
				description += ' ';
				description += versionInfo.getDate();
				description += ' ';
				description += versionInfo.getTime();

				descriptions.push_back(description); /* workaround since CMenuForwarder does not store the Option String itself */

				update_menu_targets.push_back(new CUpdateMenuTarget(i, &selected));
				CMenuForwarder* fw = new CMenuForwarder(names[i].c_str(), true, descriptions[i].c_str(), update_menu_targets.back());
				fw->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);
				SelectionWidget.addItem(fw);
				i++;
			}
		}
	}

	hide();

	if (urls.empty())
	{
		ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_FLASHUPDATE_GETINFOFILEERROR);
		return false;
	}

	SelectionWidget.exec(NULL, "");

	for (std::vector<CUpdateMenuTarget*>::iterator it = update_menu_targets.begin(); it != update_menu_targets.end(); ++it)
		delete *it;

	if (selected == -1)
		return false;

	filename = urls[selected];
	filemd5 = md5s[selected];
	newVersion = versions[selected];

	return true;
}

bool CFlashUpdate::getUpdateImage(const std::string & version)
{
	CHTTPTool httpTool;
	httpTool.setStatusViewer(this);
		
	showStatusMessageUTF(std::string(g_Locale->getText(LOCALE_FLASHUPDATE_GETUPDATEFILE)) + ' ' + version); // UTF-8

	printf("[update] get update (url): %s - %s\n", filename.c_str(), gTmpPath UPDATE_LOCAL_FILENAME);
	return httpTool.downloadFile(filename, gTmpPath UPDATE_LOCAL_FILENAME, 40 );
}
#endif

bool CFlashUpdate::checkVersion4Update()
{
	char msg[400];
	CFlashVersionInfo * versionInfo = NULL;
	neutrino_locale_t msg_body;

#ifndef DISABLE_INTERNET_UPDATE
#ifdef HAVE_DBOX_HARDWARE
	if(g_settings.softupdate_mode == UPDATEMODE_INTERNET)
	{
		if(!selectHttpImage())
			return false;

		showLocalStatus(100);
		showGlobalStatus(20);
		showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_VERSIONCHECK)); // UTF-8

		printf("[update] internet version: %s\n", newVersion.c_str());

		showLocalStatus(100);
		showGlobalStatus(20);
		hide();

		versionInfo = new CFlashVersionInfo(newVersion);

		msg_body = LOCALE_FLASHUPDATE_MSGBOX;
	}
	else
#endif
#endif
	{
		CFileBrowser UpdatesBrowser;

		CFileFilter UpdatesFilter;
		UpdatesFilter.addFilter(FILEBROWSER_UPDATE_FILTER);
#ifdef HAVE_DBOX_HARDWARE
		/* try to make sure we only flash a similar (LZMA / no LZMA)
		   update image like the one that's already installed, to
		   avoid compatibility problems*/
		FILE *f;
		char s[128];

		f = fopen("/proc/mtd", "r");
		while (fgets(s, 128, f))
		{
			if (strstr(s, "\"root (squashfs)\"")) {
				UpdatesFilter.addFilter("squashfs_nolzma");
				break;
			} else if (strstr(s, "\"root (squashfs+lzma)\"")) {
				UpdatesFilter.addFilter("squashfs");
				break;
			}
		}
		fclose(f);
#else
		UpdatesFilter.addFilter(FILEBROWSER_UPDATE_FILTER_ALT);
#endif
		UpdatesBrowser.Filter = &UpdatesFilter;

		CFile * CFileSelected = NULL;

		UpdatesBrowser.ChangeDir(gTmpPath);
		for (CFileList::iterator file = UpdatesBrowser.filelist.begin(); file != UpdatesBrowser.filelist.end(); ++file)
		{
			if (!file->isDir())
			{
				if (CFileSelected == NULL)
					CFileSelected = &(*file);
				else
				{
					CFileSelected = NULL;
					break;
				}
			}
		}
		UpdatesBrowser.hide();

		if (CFileSelected == NULL)
		{
			if (!(UpdatesBrowser.exec(gTmpPath)))
				return false;

			CFileSelected = UpdatesBrowser.getSelectedFile();

			if (CFileSelected == NULL)
				return false;
		}

		filename = CFileSelected->Name;

		FILE* fd = fopen(filename.c_str(), "r");
		if(fd)
		{
			fclose(fd);
		}
		else
		{
			hide();
			printf("[update] flash-file not found: %s\n", filename.c_str());
			ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_FLASHUPDATE_CANTOPENFILE);
			return false;
		}
		hide();

		CFlashTool ft;
		versionInfo = new CFlashVersionInfo();
		if (!ft.GetVersionInfo(*versionInfo, filename))
		{
			delete versionInfo;
			ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_FLASHUPDATE_CANTOPENFILE);
			return false;			
		}

		msg_body = LOCALE_FLASHUPDATE_MSGBOX_MANUAL;
	}

	sprintf(msg, g_Locale->getText(msg_body), versionInfo->getDate(), versionInfo->getTime(), versionInfo->getReleaseCycle(), versionInfo->getType());

	if (strcmp(RELEASE_CYCLE, versionInfo->getReleaseCycle()))
	{
		delete versionInfo;
		ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_FLASHUPDATE_WRONGBASE);
		return false;
	}

	if ((strcmp("Release", versionInfo->getType()) != 0) &&
	    (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO,
				  LOCALE_FLASHUPDATE_EXPERIMENTALIMAGE,
				  CMessageBox::mbrYes,
				  CMessageBox::mbYes | CMessageBox::mbNo,
				  NEUTRINO_ICON_UPDATE) != CMessageBox::mbrYes))
	{
		delete versionInfo;
		return false;
	}

	delete versionInfo;

	return (ShowMsgUTF(LOCALE_MESSAGEBOX_INFO,
			   msg,
			   CMessageBox::mbrYes,
			   CMessageBox::mbYes | CMessageBox::mbNo,
			   NEUTRINO_ICON_UPDATE) == CMessageBox::mbrYes); // UTF-8
}

int CFlashUpdate::exec(CMenuTarget* parent, const std::string &)
{
	if(parent)
	{
		parent->hide();
	}
	paint();

	if(!checkVersion4Update())
	{
		hide();
		return menu_return::RETURN_REPAINT;
	}
	
#ifdef LCD_UPDATE
	CLCD::getInstance()->showProgressBar2(0,NULL,0, g_Locale->getText(LOCALE_FLASHUPDATE_GLOBALPROGRESS));
	CLCD::getInstance()->setMode(CLCD::MODE_PROGRESSBAR2);
#endif // LCD_UPDATE

	showGlobalStatus(19);
	paint();
	showGlobalStatus(20);

#ifndef DISABLE_INTERNET_UPDATE
	if(g_settings.softupdate_mode == UPDATEMODE_INTERNET)
	{
		if(!getUpdateImage(newVersion))
		{
			hide();
			ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_FLASHUPDATE_GETUPDATEFILEERROR);
			return menu_return::RETURN_REPAINT;
		}
		filename = std::string(gTmpPath UPDATE_LOCAL_FILENAME);
	}
#endif

	showGlobalStatus(40);

	CFlashTool ft;
	ft.setStatusViewer(this);

#ifndef DISABLE_INTERNET_UPDATE
#ifdef HAVE_DREAMBOX_HARDWARE
	// This check was previously used only on squashfs-images
	if(g_settings.softupdate_mode == UPDATEMODE_INTERNET)
	{
		showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_MD5CHECK)); // UTF-8

		if(!ft.MD5Check(filename, filemd5))
		{
			hide();
			ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_FLASHUPDATE_MD5SUMERROR);
			return menu_return::RETURN_REPAINT;
		}
	}
#endif
#endif

	struct stat buf;
	stat(filename.c_str(), &buf);
	int filesize = buf.st_size;

	// Is the file size that of a full image? Then flash as such.
	unsigned int mtd_of_whole_image = CMTDInfo::getInstance()->findMTDNumberFromDescription(MTD_TEXT_OF_WHOLE_IMAGE);
#ifdef HAVE_DBOX_HARDWARE
	unsigned int mtd_of_update_image = CMTDInfo::getInstance()->findMTDNumber(MTD_DEVICE_OF_UPDATE_PART);
#else
	unsigned int mtd_of_update_image = mtd_of_whole_image;
#endif
	if (mtd_of_whole_image == (unsigned int) -1 || mtd_of_update_image == (unsigned int) -1)
	{
		printf("Cannot determine partition numbers, aborting flashing\n");
		hide();
		ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, "Internal error");	// I don't care to localize...
		return menu_return::RETURN_REPAINT;
	}

	if (filesize == CMTDInfo::getInstance()->getMTDSize(mtd_of_whole_image))
	{
		ft.setMTDDevice(CMTDInfo::getInstance()->getMTDFileName(mtd_of_whole_image));
		printf("full image %d %d\n", filesize,mtd_of_whole_image);
#ifdef HAVE_DBOX_HARDWARE
	} else 
	// Is filesize <= root partition? Then flash as update.
	if (filesize <= CMTDInfo::getInstance()->getMTDSize(mtd_of_update_image)) {
	  	ft.setMTDDevice(MTD_DEVICE_OF_UPDATE_PART);
		printf("update image %d %d\n", filesize, mtd_of_update_image);
#endif
	} else {
	// Otherwise reject
		printf("NO update due to erroneous file size %d %d\n", filesize, CMTDInfo::getInstance()->getMTDSize(mtd_of_update_image));
		hide();
		ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_FLASHUPDATE_MD5SUMERROR);
		return menu_return::RETURN_REPAINT;
	}

	printf("[flashtool] stopping timerd\n");
	g_Timerd->shutdown();

	CSectionsdClient sd;
	bool sd_scan = sd.getIsScanningActive();
	// restart sectionsd, this frees up memory
	printf("[flashtool] restarting sectionsd to free memory\n");
	sd.Restart();

	CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
	sleep(2);
	showGlobalStatus(60);

#ifdef TESTING
	printf("+++++++++++++++++++ NOT flashing, just testing\n");
	hide();
	unlink(filename.c_str());
	sd.RegisterNeutrino();
	sd.setPauseScanning(!sd_scan);
	return menu_return::RETURN_REPAINT;
#endif

	//flash it...
	if(!ft.program(filename, 80, 100))
	{
		hide();
		ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, ft.getErrorMessage().c_str()); // UTF-8
		sd.RegisterNeutrino();
		sd.setPauseScanning(!sd_scan);
		return menu_return::RETURN_REPAINT;
	}

	//status anzeigen
	showGlobalStatus(100);
	showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_READY)); // UTF-8

	hide();

#ifdef ENABLE_GUI_MOUNT
	// Unmount all NFS & CIFS volumes
	nfs_mounted_once = false; /* needed by update.cpp to prevent removal of modules after flashing a new cramfs, since rmmod (busybox) might no longer be available */
	CFSMounter::umount();
#endif

	ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_FLASHUPDATE_FLASHREADYREBOOT);
	ft.reboot();
	sleep(20000);

	hide();
	return menu_return::RETURN_REPAINT;
}


//--------------------------------------------------------------------------------------------------------------


CFlashExpert::CFlashExpert()
	:CProgressWindow()
{
	selectedMTD = -1;
	width = w_max (500, 100);
}

void CFlashExpert::readmtd(int mtd)
{

	std::string filename = "/tmp/mtd" + to_string(mtd) + ".img"; // US-ASCII (subset of UTF-8 and ISO8859-1)
	if (mtd == -1)
	{
		filename = "/tmp/flashimage.img"; // US-ASCII (subset of UTF-8 and ISO8859-1)
		mtd = CMTDInfo::getInstance()->findMTDNumberFromDescription(MTD_TEXT_OF_WHOLE_IMAGE); //MTD_OF_WHOLE_IMAGE;
	}
	setTitle(LOCALE_FLASHUPDATE_TITLEREADFLASH);
	paint();
	showGlobalStatus(0);
	showStatusMessageUTF((std::string(g_Locale->getText(LOCALE_FLASHUPDATE_ACTIONREADFLASH)) + " (" + CMTDInfo::getInstance()->getMTDName(mtd) + ')')); // UTF-8
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice(CMTDInfo::getInstance()->getMTDFileName(mtd));
	if(!ft.readFromMTD(filename, 100))
	{
		showStatusMessageUTF(ft.getErrorMessage()); // UTF-8
		sleep(10);
	}
	else
	{
		showGlobalStatus(100);
		showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_READY)); // UTF-8
		char message[500];
		sprintf(message, g_Locale->getText(LOCALE_FLASHUPDATE_SAVESUCCESS), filename.c_str());
		sleep(1);
		hide();
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, message);
	}
}


void CFlashExpert::writemtd(const std::string & filename, int mtdNumber)
{
	
	char message[500];
	static CImageInfo imageinfo;
	const char* mtdtarget = CMTDInfo::getInstance()->getMTDName(mtdNumber).c_str();
	const char* imagefile = FILESYSTEM_ENCODING_TO_UTF8_STRING(filename).c_str();	

#ifdef HAVE_DBOX_HARDWARE
	if (mtdNumber >3) 
	{
		sprintf(message,
		g_Locale->getText(LOCALE_FLASHUPDATE_REALLYFLASHCHIPSET),
		imageinfo.getChipInfo().c_str(),
		imagefile, mtdtarget);
	}
	else
#endif
	{	
		sprintf(message,
		g_Locale->getText(LOCALE_FLASHUPDATE_REALLYFLASHMTD),
		imagefile, mtdtarget);
	}	
		
	if (ShowMsgUTF(LOCALE_MESSAGEBOX_INFO,
		       message,
		       CMessageBox::mbrNo,
		       CMessageBox::mbYes | CMessageBox::mbNo,
		       NEUTRINO_ICON_UPDATE) != CMessageBox::mbrYes) // UTF-8
		return;

#ifdef LCD_UPDATE
	CLCD::getInstance()->showProgressBar2(0,NULL,0, g_Locale->getText(LOCALE_FLASHUPDATE_GLOBALPROGRESS));
	CLCD::getInstance()->setMode(CLCD::MODE_PROGRESSBAR2);
#endif // LCD_UPDATE

	setTitle(LOCALE_FLASHUPDATE_TITLEWRITEFLASH);
	paint();
	showGlobalStatus(0);
	CFlashTool ft;
	ft.setStatusViewer( this );
	ft.setMTDDevice( CMTDInfo::getInstance()->getMTDFileName(mtdNumber) );

#ifdef ENABLE_RADIOTEXT
	/* stop Radiotext if in Radiomode */
	if (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio &&
	    g_settings.radiotext_enable && g_Radiotext != NULL)
	{
		delete g_Radiotext;
		g_Radiotext = NULL;
	}
#endif

	printf("[flashtool] stopping timerd\n");
	g_Timerd->shutdown();

	CSectionsdClient sd;
	bool sd_scan = sd.getIsScanningActive();
	// restart sectionsd, this frees up memory
	printf("[flashtool] restarting sectionsd to free memory\n");
	sd.Restart();

#ifdef TESTING
	printf("+++++++++++++++++++ NOT flashing, just testing\n");
	hide();
	sd.RegisterNeutrino();
	sd.setPauseScanning(!sd_scan);
	unlink(filename.c_str());
	return;
#endif
	if(!ft.program( "/tmp/" + filename, 50, 100))
	{
		showStatusMessageUTF(ft.getErrorMessage()); // UTF-8
		sd.RegisterNeutrino();
		sd.setPauseScanning(!sd_scan);
		sleep(10);
	}
	else
	{
		showGlobalStatus(100);
		showStatusMessageUTF(g_Locale->getText(LOCALE_FLASHUPDATE_READY)); // UTF-8
		sleep(1);
		hide();
		ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO,
				  LOCALE_FLASHUPDATE_FLASHREADYREBOOT,
				  450, -1, NULL);
		ft.reboot();
	}
}


int CFlashExpert::showMTDSelector(const std::string & actionkey)
{
	//mtd-selector erzeugen
	CMenuWidget* mtdselector = new CMenuWidget(LOCALE_FLASHUPDATE_EXPERTFUNCTIONS, NEUTRINO_ICON_UPDATE, width);
	mtdselector->addIntroItems(LOCALE_FLASHUPDATE_MTDSELECTOR, NONEXISTANT_LOCALE, CMenuWidget::BTN_TYPE_CANCEL);
	CMTDInfo* mtdInfo =CMTDInfo::getInstance();
	for (int i = 0; i < mtdInfo->getMTDCount(); i++)
	{
		char sActionKey[20];
		sprintf(sActionKey, "%s%d", actionkey.c_str(), i);
		mtdselector->addItem(new CMenuForwarder(mtdInfo->getMTDName(i).c_str(), true, NULL, this, sActionKey, CRCInput::convertDigitToKey(i+1)));
	}
	int res = mtdselector->exec(NULL,"");
	delete mtdselector;
	return res;
}

int CFlashExpert::showFileSelector(const std::string & actionkey)
{
	CMenuWidget* fileselector = new CMenuWidget(LOCALE_FLASHUPDATE_EXPERTFUNCTIONS, NEUTRINO_ICON_UPDATE, width);
	fileselector->addIntroItems(LOCALE_FLASHUPDATE_FILESELECTOR, NONEXISTANT_LOCALE, CMenuWidget::BTN_TYPE_CANCEL);
	struct dirent **namelist;
	int n = scandir("/tmp", &namelist, 0, alphasort);
	if (n < 0)
	{
		perror("no flashimages available");
		//should be available...
	}
	else
	{
		for(int count=0;count<n;count++)
		{
			std::string filen = namelist[count]->d_name;
			if((int(filen.find(".img")) != -1) 
			   || (int(filen.find(".squashfs")) != -1)
			   || (int(filen.find(".jffs2")) != -1)
			   || (int(filen.find(".flfs")) != -1)
			   )
			{
				CMenuForwarder* fw = new CMenuForwarder(filen.c_str(), true, NULL, this, (actionkey + filen).c_str());
				fw->setItemButton(NEUTRINO_ICON_BUTTON_OKAY, true);
				fileselector->addItem(fw);
#warning TODO: make sure filen is UTF-8 encoded
			}
			free(namelist[count]);
		}
		free(namelist);
	}
	int res = fileselector->exec(NULL,"");
	delete fileselector;
	return res;
}


int CFlashExpert::exec(CMenuTarget* parent, const std::string & actionKey)
{
	int res = menu_return::RETURN_REPAINT;

	if(parent)
	{
		parent->hide();
	}

	if(actionKey=="readflash")
	{
		readmtd(-1);
	}
	else if(actionKey=="writeflash")
	{
		selectedMTD = -1;
		res = showFileSelector("");
	}
	else if(actionKey=="readflashmtd")
	{
		res = showMTDSelector("readmtd");
	}
	else if(actionKey=="writeflashmtd")
	{
		res = showMTDSelector("writemtd");
	}
	else
	{
		int iReadmtd = -1;
		int iWritemtd = -1;
		sscanf(actionKey.c_str(), "readmtd%d", &iReadmtd);
		sscanf(actionKey.c_str(), "writemtd%d", &iWritemtd);
		if(iReadmtd!=-1)
		{
			readmtd(iReadmtd);
		}
		else if(iWritemtd!=-1)
		{
			printf("mtd-write\n\n");
			selectedMTD = iWritemtd;
			res = showFileSelector("");
		}
		else
		{
			if(selectedMTD==-1)
			{
				writemtd(actionKey, CMTDInfo::getInstance()->findMTDNumberFromDescription(MTD_TEXT_OF_WHOLE_IMAGE)/*MTD_OF_WHOLE_IMAGE*/);
			}
			else
			{
				writemtd(actionKey, selectedMTD);
				selectedMTD=-1;
			}
		}
		if (res != menu_return::RETURN_EXIT_ALL)
			res = menu_return::RETURN_EXIT;
	}

	hide();
	return res;
}
