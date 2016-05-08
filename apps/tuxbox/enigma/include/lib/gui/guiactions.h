#ifndef __src__lib__gui__guiactions_h__
#define __src__lib__gui__guiactions_h__

#include <lib/system/init.h>
#include <lib/base/i18n.h>
#include <lib/gui/actions.h>

struct cursorActions
{
	eActionMap map;
	eAction up, down, left, right, insertchar, deletechar, capslock, ok, cancel, help;
	cursorActions():
		map((char *)"cursor", (char *)"Cursor"),
		up(map, (char *)"up", 0, eAction::prioWidget),
		down(map, (char *)"down", 0, eAction::prioWidget),
		left(map, (char *)"left", 0, eAction::prioWidget),
		right(map, (char *)"right", 0, eAction::prioWidget),
		insertchar(map, (char *)"insertchar", 0, eAction::prioWidget),
		deletechar(map, (char *)"deletechar", 0, eAction::prioWidget),
		capslock(map, (char *)"capslock", 0, eAction::prioWidget),
		ok(map, (char *)"ok", 0, eAction::prioWidget),
		cancel(map, (char *)"cancel", 0, eAction::prioDialog),
		help(map, (char *)"help", 0, eAction::prioGlobal)
	{
	}
};

extern eAutoInitP0<cursorActions> i_cursorActions;

struct focusActions
{
	eActionMap map;
	eAction up, down, left, right;
	focusActions(): 
		map((char *)"focus", (char *)"Focus"),
		up(map, (char *)"up", 0, eAction::prioGlobal),
		down(map, (char *)"down", 0, eAction::prioGlobal),
		left(map, (char *)"left", 0, eAction::prioGlobal),
		right(map, (char *)"right", 0, eAction::prioGlobal)
	{
	}
};

extern eAutoInitP0<focusActions> i_focusActions;

struct listActions
{
	eActionMap map;
	eAction pageup, pagedown;
	listActions():
		map((char *)"list", (char *)"Listen"),
		pageup(map, (char *)"pageup", 0, eAction::prioWidget+1),
		pagedown(map, (char *)"pagedown", 0, eAction::prioWidget+1)
	{
	}
};

extern eAutoInitP0<listActions> i_listActions;

struct shortcutActions
{
	eActionMap map;
	eAction number0, number1, number2, number3, number4,
			number5, number6, number7, number8, number9, 
			red, green, yellow, blue, menu, escape;
	shortcutActions():
		map((char *)"shortcut", (char *)"Shortcuts"),
		number0(map, (char *)"0", 0, eAction::prioGlobal),
		number1(map, (char *)"1", 0, eAction::prioGlobal),
		number2(map, (char *)"2", 0, eAction::prioGlobal),
		number3(map, (char *)"3", 0, eAction::prioGlobal),
		number4(map, (char *)"4", 0, eAction::prioGlobal),
		number5(map, (char *)"5", 0, eAction::prioGlobal),
		number6(map, (char *)"6", 0, eAction::prioGlobal),
		number7(map, (char *)"7", 0, eAction::prioGlobal),
		number8(map, (char *)"8", 0, eAction::prioGlobal),
		number9(map, (char *)"9", 0, eAction::prioGlobal),
		red(map, (char *)"red", 0, eAction::prioGlobal),
		green(map, (char *)"green", 0, eAction::prioGlobal),
		yellow(map, (char *)"yellow", 0, eAction::prioGlobal),
		blue(map, (char *)"blue", 0, eAction::prioGlobal),
		menu(map, (char *)"menu", 0, eAction::prioGlobal),
		escape(map, (char *)"escape", 0, eAction::prioGlobal)
	{
	}
};

extern eAutoInitP0<shortcutActions> i_shortcutActions;


#endif
