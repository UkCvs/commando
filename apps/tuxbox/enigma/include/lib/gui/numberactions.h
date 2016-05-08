#ifndef __CORE_GUI_NUMBERACTIONS__
#define __CORE_GUI_NUMBERACTIONS__

#include <lib/gui/actions.h>
#include <lib/base/i18n.h>
#include <lib/system/init.h>

struct numberActions
{
	eActionMap map;
	eAction key0, key1, key2, key3, key4, key5, key6, key7, key8, key9, keyExt1, keyExt2, keyBackspace;
	numberActions():
		map((char *)"numbers", (char *)"number actions"),
		key0(map, (char *)"0", 0, eAction::prioDialog),
		key1(map, (char *)"1", 0, eAction::prioDialog),
		key2(map, (char *)"2", 0, eAction::prioDialog),
		key3(map, (char *)"3", 0, eAction::prioDialog),
		key4(map, (char *)"4", 0, eAction::prioDialog),
		key5(map, (char *)"5", 0, eAction::prioDialog),
		key6(map, (char *)"6", 0, eAction::prioDialog),
		key7(map, (char *)"7", 0, eAction::prioDialog),
		key8(map, (char *)"8", 0, eAction::prioDialog),
		key9(map, (char *)"9", 0, eAction::prioDialog),
		keyExt1(map, (char *)"ext1", 0, eAction::prioDialog),
		keyExt2(map, (char *)"ext2", 0, eAction::prioDialog),
		keyBackspace(map, (char *)"backspace", 0, eAction::prioDialog)
	{
	}
};
extern eAutoInitP0<numberActions> i_numberActions;
#endif

