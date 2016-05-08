#include <enigma.h>
#include <setup_osd_extra.h>
#include <lib/dvb/decoder.h>
#include <lib/gui/emessage.h>
#include <lib/system/info.h>

eOSDExpertSetup::eOSDExpertSetup()
	:eSetupWindow(_("OSD Settings"), 10, 400)
{
	init_eOSDExpertSetup();
}

void eOSDExpertSetup::init_eOSDExpertSetup()
{
	cmove(ePoint(170, 115));

	int showosd=1;
	if ( eConfig::getInstance()->getKey("/ezap/osd/showOSDOnSwitchService", showosd) )
		eConfig::getInstance()->setKey("/ezap/osd/showOSDOnSwitchService", showosd);

	list.setFlags(list.getFlags()|eListBoxBase::flagNoPageMovement);

	timeout_infobar = new eListBoxEntryMulti(&list, _("infobar timeout (left, right)"));
	timeout_infobar->add((eString)"  " + eString().sprintf(_("Infobar timeout %d sec"), 2) + (eString)" >", 2);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 3) + (eString)" >", 3);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 4) + (eString)" >", 4);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 5) + (eString)" >", 5);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 6) + (eString)" >", 6);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 7) + (eString)" >", 7);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 8) + (eString)" >", 8);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 9) + (eString)" >", 9);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 10) + (eString)" >", 10);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 11) + (eString)" >", 11);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 12) + (eString)"  ", 12);
	int timeoutInfobar = 6;
	eConfig::getInstance()->getKey("/enigma/timeoutInfobar", timeoutInfobar);
	timeout_infobar->setCurrent(timeoutInfobar);
	CONNECT( list.selchanged, eOSDExpertSetup::selInfobarChanged );

	timeout_volumebar = new eListBoxEntryMulti(&list, _("volumebar timeout (left, right)"));
	timeout_volumebar->add((eString)"  " + eString().sprintf(_("volumebar timeout %d.%d sec"), 0, 5) + (eString)" >", 500);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 1, 0) + (eString)" >", 1000);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 1, 5) + (eString)" >", 1500);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 2, 0) + (eString)" >", 2000);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 2, 5) + (eString)" >", 2500);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 3, 0) + (eString)" >", 3000);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 3, 5) + (eString)" >", 3500);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 4, 0) + (eString)" >", 4000);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 4, 5) + (eString)" >", 4500);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 5, 0) + (eString)" >", 5000);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 5, 5) + (eString)" >", 5500);
	timeout_volumebar->add((eString)"< " + eString().sprintf(_("volumebar timeout %d.%d sec"), 6, 0) + (eString)"  ", 6000);
	int timeoutVolumebar = 2000;
	eConfig::getInstance()->getKey("/enigma/timeoutVolumebar", timeoutVolumebar);
	timeout_volumebar->setCurrent(timeoutVolumebar);
	CONNECT( list.selchanged, eOSDExpertSetup::selVolumebarChanged );

	timeout_keypressed = new eListBoxEntryMulti(&list, _("channel numbers timeout (left, right)"));
	timeout_keypressed->add((eString)"  " + eString().sprintf(_("keypressed timeout %d.%d sec"), 0, 5) + (eString)" >", 500);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 1, 0) + (eString)" >", 1000);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 1, 5) + (eString)" >", 1500);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 2, 0) + (eString)" >", 2000);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 2, 5) + (eString)" >", 2500);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 3, 0) + (eString)" >", 3000);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 3, 5) + (eString)" >", 3500);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 4, 0) + (eString)" >", 4000);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 4, 5) + (eString)" >", 4500);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 5, 0) + (eString)" >", 5000);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 5, 5) + (eString)" >", 5500);
	timeout_keypressed->add((eString)"< " + eString().sprintf(_("keypressed timeout %d.%d sec"), 6, 0) + (eString)"  ", 6000);
	int timeoutKeypressed = 2000;
	eConfig::getInstance()->getKey("/enigma/channelKeypressedInitDelay", timeoutKeypressed);
	timeout_keypressed->setCurrent(timeoutKeypressed);
	CONNECT( list.selchanged, eOSDExpertSetup::selChannelKeypressedInitDelayChanged );

	new eListBoxEntryCheck(&list, _("show infobar on service switch"), "/ezap/osd/showOSDOnSwitchService", _("show infobar when switching to another service"));
	CONNECT((new eListBoxEntryCheck(&list,_("Serviceselector help buttons"),"/ezap/serviceselector/showButtons",_("show colored help buttons in service selector")))->selected, eOSDExpertSetup::colorbuttonsChanged );
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite)
		new eListBoxEntryCheck(&list, _("Show Sat position"), "/extras/showSatPos", _("show sat position in the infobar"));
	new eListBoxEntryCheck(&list, _("Skip confirmations"), "/elitedvb/extra/profimode", _("enable/disable confirmations"));
	new eListBoxEntryCheck(&list, _("Hide error windows"), "/elitedvb/extra/hideerror", _("don't show zap error messages"));
	new eListBoxEntryCheck(&list, _("Auto show Infobar"), "/ezap/osd/showOSDOnEITUpdate", _("always show infobar when new event info is avail"));
	new eListBoxEntryCheck(&list, _("Show remaining Time"), "/ezap/osd/showCurrentRemaining", _("show event remaining time in the infobar"));
	new eListBoxEntryCheck(&list, _("Hide shortcut icons"), "/ezap/osd/hideshortcuts", _("hide shortcut icons in menus"));
}

void eOSDExpertSetup::selInfobarChanged(eListBoxEntryMenu* e)
{
	if ( e == (eListBoxEntryMenu*)timeout_infobar )
		eConfig::getInstance()->setKey("/enigma/timeoutInfobar", (int)e->getKey());
}

void eOSDExpertSetup::selVolumebarChanged(eListBoxEntryMenu* e)
{
	if ( e == (eListBoxEntryMenu*)timeout_volumebar )
		eConfig::getInstance()->setKey("/enigma/timeoutVolumebar", (int)e->getKey());
}

void eOSDExpertSetup::selChannelKeypressedInitDelayChanged(eListBoxEntryMenu* e)
{
	if ( e == (eListBoxEntryMenu*)timeout_keypressed )
		eConfig::getInstance()->setKey("/enigma/channelKeypressedInitDelay", (int)e->getKey());
}

void eOSDExpertSetup::colorbuttonsChanged(bool b)
{
	eServiceSelector *sel = eZap::getInstance()->getServiceSelector();
	sel->setStyle( sel->getStyle(), true );
}
