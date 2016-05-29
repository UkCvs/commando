
#ifndef __audiopriority_settings_menu__
#define __audiopriority_settings_menu__

#include <gui/widget/menue.h>

#include <string>

class audioprioritySettingsMenu : public CMenuTarget
{
	public:
		audioprioritySettingsMenu();
		~audioprioritySettingsMenu();
		int exec(CMenuTarget* parent, const std::string & actionKey);
		std::string menue_icon;
		void hide();
		void 	ShowHelpAPS();
};


#endif
