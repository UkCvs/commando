/*
	Sudoku
	Copyright (C) 2006-2009 Martin Schlosser
	email: sudoku@software-schlosser.de
	www:   http://www.software-schlosser.de

	Sudoku is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Sudoku is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Sudoku in a file named COPYING.
	If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MENU_H
#define _MENU_H

#include <string>
#include <list>

typedef struct tMenuItem
{
	tMenuItem();

	std::string					sText;
	int									nLeft;
	int									nTop;
	int									nWidth;
	int									nHeight;

//	tMenuItem &operator = (const tMenuItem &rMenuItem);
};

typedef std::list<tMenuItem>						tMenuItemsList;
typedef std::list<tMenuItem>::iterator	tMenuItemIterator;

typedef struct tMenu
{
	tMenu();

	// menu position
	int									nLeft;
	int									nTop;
	int									nWidth;

	// menu colors
	int									nColorMenuBorder;
	int									nColorMenuBackground;
	int									nColorItemText;
	int									nColorItemTextHighlighted;
	int									nColorItemBgHighlighted;

	int									nColorCaptionBorder;
	int									nColorCaptionBackground;
	int									nColorCaptionText;

	// menu caption
	std::string					sCaption;
	int									nCaptionHeight;

	// menu items
	int									nItemHeight;
	int									nItemsCount;
	int									nSelectedItem;

	tMenuItemsList			MenuItemsList;
};

extern tMenuItemIterator MenuGetItem(tMenu *pMenu, int nIndex);
extern void	MenuAddItem(tMenu *pMenu, const char *pszFormat, ...);
extern void	MenuSetItem(tMenu *pMenu, int nIndex, const char *pszFormat, ...);
extern void	MenuDrawItem(tMenu *pMenu, tMenuItemIterator pMenuItem, bool bHighlighted);
extern void	MenuDrawItem(tMenu *pMenu, int nIndex);
extern void	MenuDraw(tMenu *pMenu);

#endif // _MENU_H
