#include <string>

#include <global.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/icons.h> 
#include <gui/widget/keychooser.h>
#include <gui/widget/menue.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h> 
#include <gui/widget/stringinput.h>
#include <string.h>

#include "audiopriority.h"

audioprioritySettingsMenu::audioprioritySettingsMenu()
{
}

void audioprioritySettingsMenu::hide()
{
}

int audioprioritySettingsMenu::exec(CMenuTarget* parent, const std::string & actionKey)
{
	if (parent)
	{
		parent->hide();
	}

	if (actionKey=="aps_help"){
		ShowHelpAPS();
		return menu_return::RETURN_REPAINT;
	}

	CMenuWidget* audioprioritySettings = new CMenuWidget(LOCALE_AUDIOPRIORITY_SETTINGS, "audio.raw");
	CStringInput* p_audio;
	audioprioritySettings->addItem(GenericMenuSeparator);
	audioprioritySettings->addItem(GenericMenuBack);
	audioprioritySettings->addItem(GenericMenuSeparatorLine);

	for(int i=0 ; i < AUDIO_PRIORITY_NR_OF_ENTRIES ; i++)
	{
		p_audio = new CStringInputSMS(LOCALE_AUDIOPRIORITY_PIDNAME, (char *) g_settings.audio_propids_name[i],3, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE,"abcdefghijklmnopqrstuvwxyz0123456789");
 		audioprioritySettings->addItem(new CMenuForwarder(LOCALE_AUDIOPRIORITY_PIDNAME, true, g_settings.audio_propids_name[i], p_audio, NULL, CRCInput::convertDigitToKey(i+1)));
	}

	audioprioritySettings->addItem(GenericMenuSeparator);
	audioprioritySettings->addItem(GenericMenuSeparatorLine);
	audioprioritySettings->addItem(new CMenuForwarder(LOCALE_AUDIOPRIORITY_HELP, true, NULL, this, "aps_help", CRCInput::RC_help, NEUTRINO_ICON_BUTTON_HELP));

	bool res= audioprioritySettings->exec (this, "");
	delete p_audio;
	delete audioprioritySettings;
	return res;
}

audioprioritySettingsMenu::~audioprioritySettingsMenu()
{
}

void audioprioritySettingsMenu::ShowHelpAPS()
{
	Helpbox helpbox;
	helpbox.addLine(g_Locale->getText(LOCALE_AUDIOPRIORITY_HELP1));
	helpbox.addLine(g_Locale->getText(LOCALE_PERSONALIZE_HELP_LINE4));
	helpbox.addLine(g_Locale->getText(LOCALE_AUDIOPRIORITY_HELP2));
	helpbox.addLine(g_Locale->getText(LOCALE_AUDIOPRIORITY_HELP3));
	helpbox.addLine(g_Locale->getText(LOCALE_AUDIOPRIORITY_HELP4));
	helpbox.addLine(g_Locale->getText(LOCALE_PERSONALIZE_HELP_LINE4));
	helpbox.addLine(g_Locale->getText(LOCALE_AUDIOPRIORITY_HELP5));
	helpbox.addLine(g_Locale->getText(LOCALE_AUDIOPRIORITY_HELP6));
	helpbox.addLine(g_Locale->getText(LOCALE_PERSONALIZE_HELP_LINE4));
	helpbox.addLine(g_Locale->getText(LOCALE_AUDIOPRIORITY_HELP7));
	hide();
	helpbox.show(LOCALE_AUDIOPRIORITY_HELP);
}

