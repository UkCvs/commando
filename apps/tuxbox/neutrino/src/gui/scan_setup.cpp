/*
	$Id: scan_setup.cpp,v 1.23 2012/09/12 07:25:12 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	scan setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

	Copyright (C) 2013 Stefan Seyfried

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

#include "gui/scan_setup.h"

#include <global.h>
#include <neutrino.h>

#include "gui/scan.h"
#include "gui/motorcontrol.h"

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/hintbox.h>

#include <driver/screen_max.h>

#include <xmltree/xmlinterface.h>

#include <system/debug.h>

#define scanSettings (CNeutrinoApp::getInstance()->ScanSettings())
#define CABLES_LOCALE "/var/etc/cables_locale.xml"

CZapitClient::SatelliteList satList;

char zapit_lat[12];
char zapit_long[12];

CScanSetup::CScanSetup()
{
	width = w_max (500, 100);
	selected = -1;
	sat_list_size = 0;
	provider_list_size = 0;
}

CScanSetup::~CScanSetup()
{

}

int CScanSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init scan service\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	if(actionKey == "save_action") {
		scanSettings.gotoXXLatitude = strtod(zapit_lat, NULL);
		scanSettings.gotoXXLongitude = strtod(zapit_long, NULL);
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		g_Zapit->loadScanSetupSettings();
		return res;
	}
	else if (actionKey == "show_scanmodes")
	{
		res = showScanModeMenue();
		return res;
	}

	res = showScanService();

	return res;
}

#define SATSETUP_SCANTP_FEC_COUNT 5
const CMenuOptionChooser::keyval SATSETUP_SCANTP_FEC[SATSETUP_SCANTP_FEC_COUNT] =
{
	{ FEC_1_2, LOCALE_SCANTP_FEC_1_2 },
	{ FEC_2_3, LOCALE_SCANTP_FEC_2_3 },
	{ FEC_3_4, LOCALE_SCANTP_FEC_3_4 },
	{ FEC_5_6, LOCALE_SCANTP_FEC_5_6 },
	{ FEC_7_8, LOCALE_SCANTP_FEC_7_8 }
};

#define CABLESETUP_SCANTP_MOD_COUNT 7
const CMenuOptionChooser::keyval CABLESETUP_SCANTP_MOD[CABLESETUP_SCANTP_MOD_COUNT] =
{
	{ QPSK    , LOCALE_SCANTP_MOD_QPSK     },
	{ QAM_16  , LOCALE_SCANTP_MOD_QAM_16   },
	{ QAM_32  , LOCALE_SCANTP_MOD_QAM_32   },
	{ QAM_64  , LOCALE_SCANTP_MOD_QAM_64   },
	{ QAM_128 , LOCALE_SCANTP_MOD_QAM_128  },
	{ QAM_256 , LOCALE_SCANTP_MOD_QAM_256  }
#if HAVE_DVB_API_VERSION >= 3
	, { QAM_AUTO, LOCALE_SCANTP_MOD_QAM_AUTO }
#endif
};

#define SATSETUP_SCANTP_POL_COUNT 2
const CMenuOptionChooser::keyval SATSETUP_SCANTP_POL[SATSETUP_SCANTP_POL_COUNT] =
{
	{ 0, LOCALE_SCANTP_POL_H },
	{ 1, LOCALE_SCANTP_POL_V }
};

#define SATSETUP_DISEQC_OPTION_COUNT 7
const CMenuOptionChooser::keyval SATSETUP_DISEQC_OPTIONS[SATSETUP_DISEQC_OPTION_COUNT] =
{
	{ NO_DISEQC          , LOCALE_SATSETUP_NODISEQC    },
	{ MINI_DISEQC        , LOCALE_SATSETUP_MINIDISEQC  },
	{ DISEQC_1_0         , LOCALE_SATSETUP_DISEQC10    },
	{ DISEQC_1_1         , LOCALE_SATSETUP_DISEQC11    },
	{ DISEQC_1_2         , LOCALE_SATSETUP_DISEQC12    },
	{ DISEQC_UNICABLE    , LOCALE_SATSETUP_UNICABLE    },
	{ SMATV_REMOTE_TUNING, LOCALE_SATSETUP_SMATVREMOTE }

};

#define SCANTS_BOUQUET_OPTION_COUNT 5
const CMenuOptionChooser::keyval SCANTS_BOUQUET_OPTIONS[SCANTS_BOUQUET_OPTION_COUNT] =
{
	{ CZapitClient::BM_DELETEBOUQUETS        , LOCALE_SCANTS_BOUQUET_ERASE     },
	{ CZapitClient::BM_CREATEBOUQUETS        , LOCALE_SCANTS_BOUQUET_CREATE    },
	{ CZapitClient::BM_DONTTOUCHBOUQUETS     , LOCALE_SCANTS_BOUQUET_LEAVE     },
	{ CZapitClient::BM_UPDATEBOUQUETS        , LOCALE_SCANTS_BOUQUET_UPDATE    },
	{ CZapitClient::BM_CREATESATELLITEBOUQUET, LOCALE_SCANTS_BOUQUET_SATELLITE }
};

#define SCANTS_ZAPIT_SCANTYPE_COUNT 4
const CMenuOptionChooser::keyval SCANTS_ZAPIT_SCANTYPE[SCANTS_ZAPIT_SCANTYPE_COUNT] =
{
	{  CZapitClient::ST_TVRADIO	, LOCALE_ZAPIT_SCANTYPE_TVRADIO     },
	{  CZapitClient::ST_TV		, LOCALE_ZAPIT_SCANTYPE_TV    },
	{  CZapitClient::ST_RADIO	, LOCALE_ZAPIT_SCANTYPE_RADIO     },
	{  CZapitClient::ST_ALL		, LOCALE_ZAPIT_SCANTYPE_ALL }
};

#define SECTIONSD_SCAN_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval SECTIONSD_SCAN_OPTIONS[SECTIONSD_SCAN_OPTIONS_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF },
	{ 1, LOCALE_OPTIONS_ON  },
	{ 2, LOCALE_OPTIONS_ON_WITHOUT_MESSAGES  }
};

#define SCANTS_SCAN_OPTION_COUNT	3
const CMenuOptionChooser::keyval SCANTS_SCAN_OPTIONS[SCANTS_SCAN_OPTION_COUNT] =
{
	{ CScanTs::SCAN_COMPLETE,	LOCALE_SCANTP_SCAN_ALL_SATS },
	{ CScanTs::SCAN_ONE_TP,		LOCALE_SCANTP_SCAN_ONE_TP },
	{ CScanTs::SCAN_ONE_SAT,	LOCALE_SCANTP_SCAN_ONE_SAT }
};


#define SCANTS_CABLESCAN_OPTION_COUNT	2
const CMenuOptionChooser::keyval SCANTS_CABLESCAN_OPTIONS[SCANTS_CABLESCAN_OPTION_COUNT] =
{
	{ CScanTs::SCAN_COMPLETE,	LOCALE_SCANTP_SCAN_COMPLETE },
	{ CScanTs::SCAN_ONE_TP,		LOCALE_SCANTP_SCAN_ONE_TP }
};

#define OPTIONS_SOUTH0_NORTH1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_SOUTH0_NORTH1_OPTIONS[OPTIONS_SOUTH0_NORTH1_OPTION_COUNT] =
{
	{ 0, LOCALE_SATSETUP_SOUTH },
	{ 1, LOCALE_SATSETUP_NORTH }
};
#define OPTIONS_EAST0_WEST1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_EAST0_WEST1_OPTIONS[OPTIONS_EAST0_WEST1_OPTION_COUNT] =
{
	{ 0, LOCALE_SATSETUP_EAST },
	{ 1, LOCALE_SATSETUP_WEST }
};

#define SCANTP_RATE_OPTION_COUNT 3
const CMenuOptionChooser::keyval SCANTP_RATE_OPTIONS[SCANTP_RATE_OPTION_COUNT] =
{
	{ 6887,	LOCALE_SCANTP_RATE_6887 },
	{ 6952,	LOCALE_SCANTP_RATE_6952 },
	{ 6875,	LOCALE_SCANTP_RATE_6875 }
};

int CScanSetup::showScanService()
{
	dprintf(DEBUG_DEBUG, "init scansettings\n");
	initScanSettings();
	
	//menue init
	CMenuWidget* scansetup = new CMenuWidget(LOCALE_SERVICEMENU_HEAD, NEUTRINO_ICON_SETTINGS, width);
	scansetup->setPreselected(selected);

	//prepare scantype green
	CMenuOptionChooser* ojScantype = new CMenuOptionChooser(LOCALE_ZAPIT_SCANTYPE, (int *)&scanSettings.scanType, SCANTS_ZAPIT_SCANTYPE, SCANTS_ZAPIT_SCANTYPE_COUNT, true, NULL, CRCInput::RC_green);

	//prepare bouquet mode yellow
	CMenuOptionChooser* ojBouquets = new CMenuOptionChooser(LOCALE_SCANTS_BOUQUET, (int *)&scanSettings.bouquetMode, SCANTS_BOUQUET_OPTIONS, SCANTS_BOUQUET_OPTION_COUNT, true, NULL, CRCInput::RC_yellow);

	// intros
	scansetup->addIntroItems(LOCALE_SERVICEMENU_SCANTS);

	//save button red
	scansetup->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "save_action", CRCInput::RC_red));
	scansetup->addItem(GenericMenuSeparatorLine);

	//prepare sat-lnb-settings
	CMenuWidget* extSatSettings = NULL;
	CMenuWidget* extMotorSettings = NULL;
	CMenuWidget* extUnicableSettings = NULL;
	CStringInput* toff_lat = NULL;
	CStringInput* toff_long = NULL;
	CMotorControl* motorControl = NULL;
	CSatDiseqcNotifier* satDiseqcNotifier = NULL;
	CIntInput* uniqrg = NULL;

	//sat-lnb-settings
	if(g_info.delivery_system == DVB_S)
	{
		satList.clear();
 		g_Zapit->getScanSatelliteList(satList);

		//prepare diseqc
		CMenuOptionStringChooser* ojSat = new CMenuOptionStringChooser(LOCALE_SATSETUP_SATELLITE, (char*)&scanSettings.satNameNoDiseqc, ((scanSettings.diseqcMode == DISEQC_1_2) || (scanSettings.diseqcMode == NO_DISEQC)|| scanSettings.diseqcMode == DISEQC_UNICABLE), NULL, CRCInput::RC_nokey, "", true);

		bool sfound = false;
		for (CZapitClient::SatelliteList::iterator it = satList.begin(); it != satList.end(); ++it) {

			ojSat->addOption(it->satName);

			if (!sfound && strcmp(scanSettings.satNameNoDiseqc, it->satName) == 0)
				sfound = true;
			dprintf(DEBUG_DEBUG, "got scanprovider (sat): %s\n", it->satName);
		}

		if (!sfound && !satList.empty()) {
			CZapitClient::SatelliteList::iterator it = satList.begin();
			snprintf(scanSettings.satNameNoDiseqc, sizeof(scanSettings.satNameNoDiseqc), "%s", it->satName);
		}

		//prepare diseqc repeats
		CMenuOptionNumberChooser * ojDiseqcRepeats = new CMenuOptionNumberChooser(LOCALE_SATSETUP_DISEQCREPEAT, (int *)&scanSettings.diseqcRepeat, (scanSettings.diseqcMode != NO_DISEQC) && (scanSettings.diseqcMode != DISEQC_1_0) && scanSettings.diseqcMode != DISEQC_UNICABLE, 0, 2);

		//extended sat settings
		extSatSettings = new CMenuWidget(LOCALE_SATSETUP_EXTENDED, NEUTRINO_ICON_SETTINGS);

		//intros ext sat settings
		extSatSettings->addIntroItems();

		//prepare diseqc mode
		CMenuForwarder* ojExtSatSettings = new CMenuForwarder(LOCALE_SATSETUP_EXTENDED, (scanSettings.diseqcMode != NO_DISEQC && scanSettings.diseqcMode != DISEQC_UNICABLE), NULL, extSatSettings, NULL, CRCInput::RC_1);

		//make sat list
		for( uint i=0; i < sat_list_size; i++)
		{
			CMenuOptionNumberChooser * oj = new CMenuOptionNumberChooser(NONEXISTANT_LOCALE, scanSettings.diseqscOfSat(satList[i].satName), true, -1, sat_list_size - 1, 1, -1, LOCALE_OPTIONS_OFF, satList[i].satName);

			extSatSettings->addItem(oj);
		}

		//motor settings
		extMotorSettings = new CMenuWidget(LOCALE_SATSETUP_EXTENDED_MOTOR, NEUTRINO_ICON_SETTINGS);
		
		//intros motor settings
		extMotorSettings->addIntroItems();
		
		//save motorsettings red
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_SAVESETTINGSNOW, true, NULL, this, "save_action", CRCInput::RC_red));
		extMotorSettings->addItem(GenericMenuSeparatorLine);

		//motorspeed (how long to set wait timer for dish to travel to correct position) 
		extMotorSettings->addItem(new CMenuOptionNumberChooser(LOCALE_SATSETUP_MOTORSPEED, (int *)&scanSettings.motorRotationSpeed, true, 0, 64)) ;
		extMotorSettings->addItem(GenericMenuSeparatorLine);

		//gotoxx settings
		extMotorSettings->addItem(new CMenuOptionChooser(LOCALE_SATSETUP_USEGOTOXX,  (int *)&scanSettings.useGotoXX, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

		sprintf(zapit_lat, "%02.6f", scanSettings.gotoXXLatitude);
		sprintf(zapit_long, "%02.6f", scanSettings.gotoXXLongitude);

		extMotorSettings->addItem(new CMenuOptionChooser(LOCALE_SATSETUP_LADIR,  (int *)&scanSettings.gotoXXLaDirection, OPTIONS_SOUTH0_NORTH1_OPTIONS, OPTIONS_SOUTH0_NORTH1_OPTION_COUNT, true));
		toff_lat = new CStringInput(LOCALE_SATSETUP_LAT, (char *) zapit_lat, 10, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789.");
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_LAT, true, zapit_lat, toff_lat));

		extMotorSettings->addItem(new CMenuOptionChooser(LOCALE_SATSETUP_LODIR,  (int *)&scanSettings.gotoXXLoDirection, OPTIONS_EAST0_WEST1_OPTIONS, OPTIONS_EAST0_WEST1_OPTION_COUNT, true));
		toff_long = new CStringInput(LOCALE_SATSETUP_LONG, (char *) zapit_long, 10, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789.");
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_LONG, true, zapit_long, toff_long));

		extMotorSettings->addItem(GenericMenuSeparatorLine);
		
		//manual motor control
		motorControl = new CMotorControl();
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_MOTORCONTROL, true, NULL, motorControl, NULL, CRCInput::RC_green));
		extMotorSettings->addItem(GenericMenuSeparatorLine);

		//prepare motor control
		CMenuForwarder* ojExtMotorSettings = new CMenuForwarder(LOCALE_SATSETUP_EXTENDED_MOTOR, (scanSettings.diseqcMode == DISEQC_1_2), NULL, extMotorSettings, NULL, CRCInput::RC_2);

		//prepare/show sat list with options
		for( uint i=0; i < sat_list_size; i++)
		{
			CMenuOptionNumberChooser * oj = new CMenuOptionNumberChooser(NONEXISTANT_LOCALE, scanSettings.motorPosOfSat(satList[i].satName), true, 0, 64/*sat_list_size*/, 0, 0, LOCALE_OPTIONS_OFF, satList[i].satName);

			extMotorSettings->addItem(oj);
		}

		extUnicableSettings = new CMenuWidget(LOCALE_SATSETUP_UNICABLE_SETTINGS, NEUTRINO_ICON_SETTINGS);
		extUnicableSettings->addIntroItems();
		CMenuOptionNumberChooser *uniscr = new CMenuOptionNumberChooser(LOCALE_SATSETUP_UNICABLE_SCR, (int *)&scanSettings.uni_scr, true, 0, 7);
		uniqrg = new CIntInput(LOCALE_SATSETUP_UNICABLE_QRG, (int &)scanSettings.uni_qrg, 4);
		extUnicableSettings->addItem(uniscr);
		extUnicableSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_UNICABLE_QRG, true, uniqrg->getValue(), uniqrg));
		CMenuForwarder* ojExtUnicableSettings = new CMenuForwarder(LOCALE_SATSETUP_UNICABLE, (scanSettings.diseqcMode == DISEQC_UNICABLE), NULL, extUnicableSettings, NULL, CRCInput::RC_3);

		//prepare sat list with diseqc options
		satDiseqcNotifier = new CSatDiseqcNotifier(ojSat, ojExtSatSettings, ojExtMotorSettings, ojDiseqcRepeats, ojExtUnicableSettings);
		CMenuOptionChooser* ojDiseqc = new CMenuOptionChooser(LOCALE_SATSETUP_DISEQC, (int *)&scanSettings.diseqcMode, SATSETUP_DISEQC_OPTIONS, SATSETUP_DISEQC_OPTION_COUNT, true, satDiseqcNotifier);


		//show entries
		scansetup->addItem( ojScantype );
		scansetup->addItem( ojBouquets );
		scansetup->addItem(GenericMenuSeparatorLine);
		scansetup->addItem( ojDiseqc );
		scansetup->addItem( ojSat );
		scansetup->addItem( ojDiseqcRepeats );

		scansetup->addItem( ojExtSatSettings );
		scansetup->addItem( ojExtMotorSettings );
		scansetup->addItem( ojExtUnicableSettings );
	}
	else
	{//cable

		CZapitClient::SatelliteList providerList;
		g_Zapit->getScanSatelliteList(providerList);

		//cable provider selector
		CMenuOptionStringChooser * ojProv = new CMenuOptionStringChooser(LOCALE_CABLESETUP_PROVIDER, (char*)&scanSettings.satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true, true);

		bool sfound = false;
		for (CZapitClient::SatelliteList::iterator it = providerList.begin(); it != providerList.end(); ++it) {

			ojProv->addOption(it->satName);

			if (!sfound && strcmp(scanSettings.satNameNoDiseqc, it->satName) == 0)
				sfound = true;
			dprintf(DEBUG_DEBUG, "got scanprovider (cable): %s\n", it->satName);
		}

		if (!sfound && !providerList.empty()) {
			CZapitClient::SatelliteList::iterator it = providerList.begin();
			snprintf(scanSettings.satNameNoDiseqc, sizeof(scanSettings.satNameNoDiseqc), "%s", it->satName);
		}

		//show general entries
		scansetup->addItem( ojScantype );
		scansetup->addItem( ojBouquets );
		
		//show cable provider
		scansetup->addItem( ojProv );
	}

	//prepare scan mode (fast->on/off)
	CMenuOptionChooser* onoff_mode = ( new CMenuOptionChooser(LOCALE_SCANTP_SCANMODE, (int *)&scanSettings.scan_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	scansetup->addItem(GenericMenuSeparatorLine);

	if(scanSettings.TP_fec == 0) {
		scanSettings.TP_fec = 1;
	}

	//sub menue scanmode
	scan_mode_string = getScanModeString(scanSettings.TP_scan);
	CMenuForwarder* fw_scanmode = new CMenuForwarder(LOCALE_SERVICEMENU_SCANMODES, true, scan_mode_string, this, "show_scanmodes", (g_info.delivery_system == DVB_S) ? CRCInput::RC_3 : CRCInput::RC_1);
	scansetup->addItem(fw_scanmode); 

	//show scan mode (fast->on/off)
	//if(g_info.delivery_system != DVB_C)
		scansetup->addItem(onoff_mode);

	//prepare auto scan
	CMenuOptionChooser* onoffscanSectionsd = ( new CMenuOptionChooser(LOCALE_SECTIONSD_SCANMODE, (int *)&scanSettings.scanSectionsd, SECTIONSD_SCAN_OPTIONS, SECTIONSD_SCAN_OPTIONS_COUNT, true, this));

	//show auto scan
	scansetup->addItem(onoffscanSectionsd);
	scansetup->addItem(GenericMenuSeparatorLine);

	CScanTs* scanTs = new CScanTs();
	scansetup->addItem(new CMenuForwarder(LOCALE_SCANTS_STARTNOW, true, NULL, scanTs, NULL, CRCInput::RC_blue));

	int res = scansetup->exec(NULL, "");
	selected = scansetup->getSelected();
	delete scansetup;

	delete extSatSettings;
	delete extMotorSettings;
	delete extUnicableSettings;
	delete toff_lat;
	delete toff_long;
	delete motorControl;
	delete satDiseqcNotifier;
	delete scanTs;
	delete uniqrg;

	return res;
}

//sub menue scanmode
int CScanSetup::showScanModeMenue()
{
	CMenuWidget* scanmode = new CMenuWidget(LOCALE_SERVICEMENU_SCANTS, NEUTRINO_ICON_SETTINGS, width);

	//prepare input signal rate
	CStringInput rate(LOCALE_SCANTP_RATE, (char *) scanSettings.TP_rate, 8, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");

	//prepare input netid
	CStringInput* netid = new CStringInput(LOCALE_SCANTP_NETID, (char *) scanSettings.netid, 5, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");

	//prepare symrate select
	CMenuOptionChooser* symrate = new CMenuOptionChooser(LOCALE_SCANTP_RATE, (int *)&scanSettings.symrate, SCANTP_RATE_OPTIONS, SCANTP_RATE_OPTION_COUNT, scanSettings.TP_scan == 1);

	//prepare fec select
	CMenuOptionChooser* fec = new CMenuOptionChooser(LOCALE_SCANTP_FEC, (int *)&scanSettings.TP_fec, SATSETUP_SCANTP_FEC, SATSETUP_SCANTP_FEC_COUNT, scanSettings.TP_scan == 1);

	//prepare frequency input, polarisation
	CStringInput* freq;
	CMenuOptionChooser* pol_mod;

	if(g_info.delivery_system == DVB_S) // sat
	{
		freq = new CStringInput(LOCALE_SCANTP_FREQ, (char *) scanSettings.TP_freq, 8, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
		pol_mod = new CMenuOptionChooser(LOCALE_SCANTP_POL, (int *)&scanSettings.TP_pol, SATSETUP_SCANTP_POL, SATSETUP_SCANTP_POL_COUNT, scanSettings.TP_scan == 1);
	} 
	else // cable
	{
		freq = new CStringInput(LOCALE_SCANTP_FREQ, (char *) scanSettings.TP_freq, 6, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
		pol_mod = new CMenuOptionChooser(LOCALE_SCANTP_MOD, (int *)&scanSettings.TP_mod, CABLESETUP_SCANTP_MOD, CABLESETUP_SCANTP_MOD_COUNT, scanSettings.TP_scan == 1);
	}

	//prepare forwarders rate/freq/netid
	CMenuForwarder *Rate = new CMenuForwarder(LOCALE_SCANTP_RATE, scanSettings.TP_scan == 1, scanSettings.TP_rate, &rate);
	CMenuForwarder *Freq = new CMenuForwarder(LOCALE_SCANTP_FREQ, scanSettings.TP_scan == 1, scanSettings.TP_freq, freq);
	CMenuForwarder *Netid = new CMenuForwarder(LOCALE_SCANTP_NETID, scanSettings.TP_scan == 1, scanSettings.netid, netid);

	//cable-region-settings
	CMenuOptionStringChooser * CableRegion = NULL;
	if(g_info.delivery_system == DVB_C)
	{
		CableRegion = new CMenuOptionStringChooser(LOCALE_CABLEREGION_OPTION, (char*)&scanSettings.cable_region, scanSettings.TP_scan == 1, new CCableRegionNotifier(), CRCInput::RC_nokey, "", true, true);
		xmlDocPtr parser = parseXmlFile(CABLES_LOCALE);
		if (parser != NULL)
		{
			std::string name;
			bool name_found = false;
			xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
			while (search)
			{
				if (!strcmp(xmlGetName(search), "transponder"))
				{
					name = xmlGetAttribute(search, "name");
					dprintf(DEBUG_DEBUG, "[scan-setup] CABLE REGION: %s\n", name.c_str());

					if (!name_found && strcmp(scanSettings.cable_region, name.c_str()) == 0)
						name_found = true;

					CableRegion->addOption(name.c_str());
				}
				search = search->xmlNextNode;
			}
			if (!name_found)
				snprintf(scanSettings.cable_region, sizeof(scanSettings.cable_region), "%s", name.c_str());
		}
	}

	//sat-lnb-settings
	CMenuOptionStringChooser* TP_SatSelectMenu = NULL;
	if(g_info.delivery_system == DVB_S)
	{
		uint i;
		int satfound = -1;
		int firstentry = -1;

		TP_SatSelectMenu = new CMenuOptionStringChooser(LOCALE_SATSETUP_SATELLITE, scanSettings.TP_satname, ((scanSettings.diseqcMode != NO_DISEQC) && scanSettings.TP_scan), this);

		// add the sats which are configured (diseqc or motorpos) to the list of available sats */
		for (i = 0; i < sat_list_size; i++)
		{
			if ((((scanSettings.diseqcMode != DISEQC_1_2)) && (0 <= (*scanSettings.diseqscOfSat(satList[i].satName) ))) ||
			    (((scanSettings.diseqcMode == DISEQC_1_2)) && (0 <= (*scanSettings.motorPosOfSat(satList[i].satName)))))
			{
				if (firstentry == -1) firstentry = i;
				if (strcmp(scanSettings.TP_satname, satList[i].satName) == 0)
					satfound = i;
				TP_SatSelectMenu->addOption(satList[i].satName);
				dprintf(DEBUG_DEBUG, "satName = %s, diseqscOfSat(%d) = %d, motorPosOfSat(%d) = %d\n", satList[i].satName, i, *scanSettings.diseqscOfSat(satList[i].satName), i, *scanSettings.motorPosOfSat(satList[i].satName));
			}
		}
		// if scanSettings.TP_satname cannot be found in the list of available sats use 1st in list
		if ((satfound == -1) && (sat_list_size)) {
//			strcpy(scanSettings.TP_satname, satList[firstentry].satName);
			strcpy(scanSettings.TP_satname, scanSettings.satNameNoDiseqc);
		}
	}

	CTP_scanNotifier TP_scanNotifier(fec, pol_mod, symrate, Netid, Freq, Rate, TP_SatSelectMenu, CableRegion, scan_mode_string);
	CMenuOptionChooser* scan;
	if(g_info.delivery_system == DVB_S) {
		scan = new CMenuOptionChooser(LOCALE_SCANTP_SCAN, (int *)&scanSettings.TP_scan, SCANTS_SCAN_OPTIONS, SCANTS_SCAN_OPTION_COUNT, true/*(g_info.delivery_system == DVB_S)*/, &TP_scanNotifier);
	} else {
		scan = new CMenuOptionChooser(LOCALE_SCANTP_SCAN, (int *)&scanSettings.TP_scan, SCANTS_CABLESCAN_OPTIONS, SCANTS_CABLESCAN_OPTION_COUNT, true/*(g_info.delivery_system != DVB_S)*/, &TP_scanNotifier);
	}

	//intros scan mode
	scanmode->addIntroItems(LOCALE_SERVICEMENU_SCANMODES);
	scanmode->addItem(scan);
	if(g_info.delivery_system == DVB_S) {
		scanmode->addItem(TP_SatSelectMenu);
	}
	else
	{
		scanmode->addItem(GenericMenuSeparatorLine);
		scanmode->addItem(CableRegion);
		scanmode->addItem(Netid);
	}

	//show items for scanmode submenue
	scanmode->addItem(Freq);
	scanmode->addItem(pol_mod);
	if(g_info.delivery_system == DVB_C)
		scanmode->addItem(symrate);
	else
		scanmode->addItem(Rate);
	scanmode->addItem(fec);

	int res = scanmode->exec(NULL, "");
	delete scanmode;

	delete freq;

	return res;
}

void CScanSetup::initScanSettings()
{
	if(g_info.delivery_system == DVB_S) // sat
	{
		satList.clear();
		g_Zapit->getScanSatelliteList(satList);
	
		printf("[scan-setup] received %d sats\n", satList.size());
		t_satellite_position currentSatellitePosition = g_Zapit->getCurrentSatellitePosition();
	
		if (1/*scanSettings.diseqcMode == DISEQC_1_2*/)
		{
			for (uint i = 0; i < satList.size(); i++)
			{
				//printf("[neutrino] received %d: %s, %d\n", i, satList[i].satName, satList[i].satPosition);
				scanSettings.satPosition[i] = satList[i].satPosition;
				scanSettings.satMotorPos[i] = satList[i].motorPosition;
				strcpy(scanSettings.satName[i], satList[i].satName);
				//scanSettings.satDiseqc[i] = satList[i].satDiseqc;
				if (satList[i].satPosition == currentSatellitePosition) // make that sense?
					strcpy(scanSettings.satNameNoDiseqc, satList[i].satName);
			}
			for (uint i = satList.size(); i < MAX_SATELLITES; i++)
			{
				scanSettings.satName[i][0] = 0;
				scanSettings.satPosition[i] = 0;
				scanSettings.satDiseqc[i] = -1;
			}
		}
		
		sat_list_size = satList.size();
	}
	else // cable
	{
		CZapitClient::SatelliteList providerList;
		g_Zapit->getScanSatelliteList(providerList);

		printf("[scan-setup] received %d cable provider(s)\n", providerList.size());
		provider_list_size = providerList.size();	
	}
	
}

std::string CScanSetup::getScanModeString(const int& scan_type)
{
	int st = scan_type;
	if (g_info.delivery_system == DVB_S)
		return g_Locale->getText(SCANTS_SCAN_OPTIONS[st].value);
	else
		return g_Locale->getText(SCANTS_CABLESCAN_OPTIONS[st].value);
}

bool CCableRegionNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	bool found = false;
	int TP_mod, TP_fec, TP_symrate;
	std::string cable_region, netid, TP_freq;//, TP_rate;

	dprintf(DEBUG_DEBUG, "CCableRegionNotifier: %s\n", (char *) Data);

	xmlDocPtr parser = parseXmlFile(CABLES_LOCALE);
	if (parser != NULL) {
		xmlNodePtr locale = xmlDocGetRootElement(parser)->xmlChildrenNode;
		while (locale) {
			if (!strcmp(xmlGetName(locale), "transponder")) {
				cable_region = xmlGetAttribute(locale, "name");
				if(!strcmp(scanSettings.cable_region, cable_region.c_str())) {
					netid = xmlGetAttribute(locale, "netid");
					TP_freq = xmlGetAttribute(locale, "frequency");
					//TP_rate = xmlGetAttribute(locale, "symbol_rate");
					TP_symrate = xmlGetNumericAttribute(locale, "symbol_rate", 0);
					TP_mod = xmlGetNumericAttribute(locale, "modulation", 0);
					TP_fec = xmlGetNumericAttribute(locale, "fec_inner", 0);
					found = true;
					break;
				}
			}
			locale = locale->xmlNextNode;
		}
		xmlFreeDoc(parser);
	}

	if(found)
	{
		snprintf(scanSettings.netid, sizeof(scanSettings.netid), "%s", netid.c_str());
		snprintf(scanSettings.TP_freq, sizeof(scanSettings.TP_freq), "%s", TP_freq.c_str());
		//snprintf(scanSettings.TP_rate, sizeof(scanSettings.TP_rate), "%s", TP_rate.c_str());
		scanSettings.symrate = TP_symrate;
		scanSettings.TP_mod = TP_mod;
		scanSettings.TP_fec = TP_fec;

		dprintf(DEBUG_DEBUG, "Set Cable Region: %s -> %s,%s,%d,%d,%d\n",
			cable_region.c_str(),
			netid.c_str(),
			TP_freq.c_str(),
			//TP_rate.c_str(),
			TP_symrate,
			TP_mod,
			TP_fec);
	}
	return true;
}

bool CScanSetup::changeNotify(const neutrino_locale_t OptionName, void * Data)
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_SATSETUP_SATELLITE))
	{
		(CNeutrinoApp::getInstance()->ScanSettings()).TP_diseqc =
			 *((CNeutrinoApp::getInstance()->ScanSettings()).diseqscOfSat((char*)Data));
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_SECTIONSD_SCANMODE))
	{
		CNeutrinoApp::getInstance()->SendSectionsdConfig();
	}
	return false;
}

bool CSatDiseqcNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	if (*((int*) Data) == NO_DISEQC)
	{
		satMenu->setActive(true);
		extMenu->setActive(false);
		extMotorMenu->setActive(false);
		repeatMenu->setActive(false);
	}
	else
	if (*((int*) Data) == DISEQC_1_2)
	{
		satMenu->setActive(true);
		extMenu->setActive(true);
		extMotorMenu->setActive(true);
		repeatMenu->setActive(true);
	}
	else
	if (*((int*) Data) == DISEQC_UNICABLE)
	{
		satMenu->setActive(true);
		extMenu->setActive(false);
		extMotorMenu->setActive(false);
		repeatMenu->setActive(false);
		extUnicableMenu->setActive(true);
	}
	else
	{
		satMenu->setActive(false);
		extMenu->setActive(true);
		extMotorMenu->setActive(false);
		repeatMenu->setActive((*((int*) Data) != DISEQC_1_0));
	}
	return false;
}

CTP_scanNotifier::CTP_scanNotifier(CMenuOptionChooser* i1, CMenuOptionChooser* i2, CMenuOptionChooser* i3, CMenuForwarder* i4, CMenuForwarder* i5, CMenuForwarder* i6, CMenuOptionStringChooser* i7, CMenuOptionStringChooser* i8, std::string &s)
{
	toDisable1[0]=i1;
	toDisable1[1]=i2;
	toDisable1[2]=i3;
	toDisable2[0]=i4;
	toDisable2[1]=i5;
	toDisable2[2]=i6;
	toDisable3[0]=i7;
	toDisable3[1]=i8;
	scan_mode_string = &s;
}

bool CTP_scanNotifier::changeNotify(const neutrino_locale_t, void *Data)
{
	int tp_scan_mode = CNeutrinoApp::getInstance()->getScanSettings().TP_scan;
	bool set_true_false = tp_scan_mode;

	if ((*((int*) Data) == CScanTs::SCAN_COMPLETE) || (*((int*) Data) == CScanTs::SCAN_ONE_SAT))
		set_true_false = false;

	for (int i=0; i<3; i++)
	{
		if (toDisable1[i]) toDisable1[i]->setActive(set_true_false);
		if (toDisable2[i]) toDisable2[i]->setActive(set_true_false);
	}

	for (int i=0; i<2; i++)
	{
		if (toDisable3[i]) {
			if (*((int*) Data) == CScanTs::SCAN_COMPLETE)
				toDisable3[i]->setActive(false);
			else
				toDisable3[i]->setActive(true);
		}
	}

	if (g_info.delivery_system == DVB_S)
		*scan_mode_string = g_Locale->getText(SCANTS_SCAN_OPTIONS[tp_scan_mode].value);
	else
		*scan_mode_string = g_Locale->getText(SCANTS_CABLESCAN_OPTIONS[tp_scan_mode].value);

	return false;
}
