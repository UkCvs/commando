/*
	$Id: software_update.cpp,v 1.12 2012/09/23 08:18:03 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	Software update implementation - Neutrino-GUI

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

#include <global.h>
#include <neutrino.h>

#include "software_update.h"

#include "gui/update.h"
#include "gui/imageinfo.h"

#include "gui/widget/stringinput.h"
#include <gui/widget/icons.h>

#include <driver/fontrenderer.h>
#include <driver/screen_max.h>

#include <system/debug.h>
#include <system/flashtool.h>

CSoftwareUpdate::CSoftwareUpdate()
{
	width = w_max (710, 100);
	selected = -1;
}

CSoftwareUpdate::~CSoftwareUpdate()
{

}

#define FLASHUPDATE_UPDATEMODE_OPTION_COUNT 2
const CMenuOptionChooser::keyval FLASHUPDATE_UPDATEMODE_OPTIONS[FLASHUPDATE_UPDATEMODE_OPTION_COUNT] =
{
	{ CFlashUpdate::UPDATEMODE_MANUAL  , LOCALE_FLASHUPDATE_UPDATEMODE_MANUAL   },
	{ CFlashUpdate::UPDATEMODE_INTERNET, LOCALE_FLASHUPDATE_UPDATEMODE_INTERNET }
};

int CSoftwareUpdate::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init software-update\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	if(actionKey=="experts") {  // expert functions
		res = showSoftwareUpdateExpert();
		return res;
	}

	//res = showSoftwareUpdate();
	res = showSoftwareUpdateExpert();
	return res;
}

int CSoftwareUpdate::showSoftwareUpdate()
/* shows the menue and options for software update */
{
	CMenuWidget* softUpdate = new CMenuWidget(LOCALE_SERVICEMENU_HEAD, NEUTRINO_ICON_UPDATE, width);
	softUpdate->setPreselected(selected);

	// intros
	softUpdate->addIntroItems(LOCALE_SERVICEMENU_UPDATE);

	// experts-functions 
	softUpdate->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_EXPERTFUNCTIONS, true, NULL, this, "experts", CRCInput::RC_red));

#ifndef DISABLE_INTERNET_UPDATE
#ifndef HAVE_DREAMBOX_HARDWARE
	softUpdate->addItem(GenericMenuSeparatorLine);
	CMenuOptionChooser *oj = new CMenuOptionChooser(LOCALE_FLASHUPDATE_UPDATEMODE, &g_settings.softupdate_mode, FLASHUPDATE_UPDATEMODE_OPTIONS, FLASHUPDATE_UPDATEMODE_OPTION_COUNT, true);
	softUpdate->addItem( oj );
#endif
#endif
	
 	/* show current version */
	showSoftwareUpdateImageinfo(softUpdate);

	// update check
	CFlashUpdate* flashUpdate = new CFlashUpdate();
	softUpdate->addItem(GenericMenuSeparatorLine);
	softUpdate->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CHECKUPDATE, true, NULL, flashUpdate, NULL, CRCInput::RC_green));

	int res = softUpdate->exec (NULL, "");
	selected = softUpdate->getSelected();
	delete softUpdate;

	delete flashUpdate;

	return res;
}

int CSoftwareUpdate::showSoftwareUpdateExpert()
/* shows experts-functions to read/write to the mtd */
{
	CFlashExpert* fe = new CFlashExpert();

	CMenuWidget* mtdexpert = new CMenuWidget(LOCALE_SERVICEMENU_UPDATE, NEUTRINO_ICON_UPDATE);
	mtdexpert->addIntroItems(LOCALE_FLASHUPDATE_EXPERTFUNCTIONS);
	
	mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_READFLASH, true, NULL, fe, "readflash", CRCInput::RC_red));
	mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_WRITEFLASH, true, NULL, fe, "writeflash", CRCInput::RC_green));
/*
	mtdexpert->addItem(GenericMenuSeparatorLine);

	mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_READFLASHMTD, true, NULL, fe, "readflashmtd", CRCInput::RC_yellow));
	mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_WRITEFLASHMTD, true, NULL, fe, "writeflashmtd", CRCInput::RC_blue));
*/
#ifndef DISABLE_INTERNET_UPDATE
	mtdexpert->addItem(GenericMenuSeparatorLine);
	CStringInputSMS softUpdate_url_file(LOCALE_FLASHUPDATE_URL_FILE, &g_settings.softupdate_url_file, 30, false, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789!""$%&/()=?-. ");
	mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_URL_FILE, true, g_settings.softupdate_url_file, &softUpdate_url_file));
#endif /*DISABLE_INTERNET_UPDATE*/

	/*show current version */
	showSoftwareUpdateImageinfo(mtdexpert);

	int res = mtdexpert->exec (NULL, "");
	delete mtdexpert;

	delete fe;

	return res;
}

void CSoftwareUpdate::showSoftwareUpdateImageinfo(CMenuWidget * entry)
/* shows entries with current installed version*/
{
	entry->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_FLASHUPDATE_CURRENTVERSION_SEP));

	/* get current version SBBBYYYYMMTTHHMM -- formatsting */
	CConfigFile versionFile('\t');

	const char * versionString = (versionFile.loadConfig("/.version")) ? (versionFile.getString( "version", "????????????????").c_str()) : "????????????????";

	dprintf(DEBUG_INFO, "current flash-version: %s\n", versionString);

	static CFlashVersionInfo versionInfo(versionString);
	static CImageInfo imageinfo;
	
	entry->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_IMAGENAME , false, imageinfo.getImageInfo(DISTRIBUTION)));
	entry->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CURRENTVERSIONDATE    , false, versionInfo.getDate()));
	entry->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CURRENTVERSIONTIME    , false, versionInfo.getTime()));
	entry->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CURRENTRELEASECYCLE   , false, versionInfo.getReleaseCycle()));
	/* versionInfo.getType() returns const char * which is never deallocated */
	entry->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CURRENTVERSIONSNAPSHOT, false, versionInfo.getType()));
}

