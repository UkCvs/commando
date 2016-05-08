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

#include "menu.h"

#include <cstdio>
#include <stdarg.h>

//#define TEST
#ifndef TEST
extern "C"
{
	#include <draw.h>
}
#else
	#define RED	0
#endif

#include "colors.h"

#define SCREEN_WIDTH										720
#define SCREEN_HEIGHT										576

#undef BLACK
#define BLACK														BLACKBLACK

#define	COLOR_TEXTBG										0

#define FORMAT_TMP_LEN									1024


// ----------------------------------------------------------------------------
// tMenuItem::tMenuItem()
// ----------------------------------------------------------------------------
tMenuItem::tMenuItem()
	: nLeft(-1)
	, nTop(-1)
	, nWidth(-1)
	, nHeight(-1)
{
}
/*
// ----------------------------------------------------------------------------
// tMenuItem::operator =
// ----------------------------------------------------------------------------
tMenuItem &tMenuItem::operator = (const tMenuItem &rMenuItem)
{
	sText = rMenuItem.sText;
	return *this;
}
*/
// ----------------------------------------------------------------------------
// tMenu
// ----------------------------------------------------------------------------
tMenu::tMenu()
	: nLeft(128)
	, nTop(64)
	, nWidth(SCREEN_WIDTH-(2*nLeft))
	, nColorMenuBorder(BLACK)
	, nColorMenuBackground(STEELBLUE)
	, nColorItemText(WHITE)
	, nColorItemTextHighlighted(YELLOW)
	, nColorItemBgHighlighted(BLUE)
	, nColorCaptionBorder(RED)
	, nColorCaptionBackground(BLACK)
	, nColorCaptionText(RED)
	, nCaptionHeight(48)
	, nItemHeight(34)
	, nItemsCount(0)
	, nSelectedItem(0)
{
}

// ----------------------------------------------------------------------------
// MenuAddItem
// ----------------------------------------------------------------------------
tMenuItemIterator MenuGetItem(tMenu *pMenu, int nIndex)
{
	if(nIndex < 0 || nIndex > (pMenu->nItemsCount-1))
		return pMenu->MenuItemsList.end();

	int i=0;
	tMenuItemIterator pMenuItem;
	for(pMenuItem = pMenu->MenuItemsList.begin(); pMenuItem != pMenu->MenuItemsList.end(); pMenuItem++)
	{
		if(i == nIndex)
			return pMenuItem;
		i++;
	}

	return pMenu->MenuItemsList.end();
}

// ----------------------------------------------------------------------------
// MenuAddItem
// ----------------------------------------------------------------------------
void MenuAddItem(tMenu *pMenu, const char *pszFormat, ...)
{
	tMenuItem Item;
//	Item.sText = pszText;
	va_list argList;
	va_start(argList, pszFormat);

		// TODO:
		char szString[FORMAT_TMP_LEN];
		vsprintf(szString, pszFormat, argList);
		Item.sText = szString;

	va_end(argList);

	pMenu->MenuItemsList.push_back(Item);
	pMenu->nItemsCount++;
}

// ----------------------------------------------------------------------------
// MenuSetItem
// ----------------------------------------------------------------------------
void MenuSetItem(tMenu *pMenu, int nIndex, const char *pszFormat, ...)
{
	tMenuItemIterator pMenuItem = MenuGetItem(pMenu, nIndex);
	va_list argList;
	va_start(argList, pszFormat);

		// TODO:
		char szString[FORMAT_TMP_LEN];
		vsprintf(szString, pszFormat, argList);
		pMenuItem->sText = szString;

	va_end(argList);

	if(pMenuItem->nLeft != -1 && pMenuItem->nTop != -1 && pMenuItem->nWidth != -1 && pMenuItem->nHeight != -1)
		MenuDrawItem(pMenu, pMenuItem, (nIndex == pMenu->nSelectedItem));
}

// ----------------------------------------------------------------------------
// MenuDrawItem
// ----------------------------------------------------------------------------
void MenuDrawItem(tMenu *pMenu, tMenuItemIterator pMenuItem, bool bHighlighted)
{
	FBFillRect(pMenuItem->nLeft, pMenuItem->nTop, pMenuItem->nWidth, pMenuItem->nHeight, bHighlighted ? pMenu->nColorItemBgHighlighted : pMenu->nColorMenuBackground);
	FBDrawString(pMenuItem->nLeft, pMenuItem->nTop, pMenuItem->nHeight, (char *) pMenuItem->sText.c_str(), bHighlighted ? pMenu->nColorItemTextHighlighted : pMenu->nColorItemText, COLOR_TEXTBG);
}

// ----------------------------------------------------------------------------
// MenuDrawItem
// ----------------------------------------------------------------------------
void MenuDrawItem(tMenu *pMenu, int nIndex)
{
	MenuDrawItem(pMenu, MenuGetItem(pMenu, nIndex), (nIndex == pMenu->nSelectedItem));
}

// ----------------------------------------------------------------------------
// MenuDraw
// ----------------------------------------------------------------------------
void MenuDraw(tMenu *pMenu)
{
#ifndef TEST
	int x		= pMenu->nLeft;
	int y		= pMenu->nTop;
	int w		= pMenu->nWidth;

	if(pMenu->sCaption.length() > 0)
	{
		if(pMenu->nCaptionHeight <= 0)
			pMenu->nCaptionHeight = 48;

		FBFillRect(x, y, w, pMenu->nCaptionHeight, pMenu->nColorCaptionBackground);
		FBDrawRect(x, y, w, pMenu->nCaptionHeight-1, pMenu->nColorCaptionBorder);

		FBDrawString(x+4, y+4, pMenu->nCaptionHeight-2, (char *) pMenu->sCaption.c_str(), pMenu->nColorCaptionText, COLOR_TEXTBG);

		y += pMenu->nCaptionHeight;
		FBFillRect(x, y, w, 2, BLACK);
		y += 2;
	}

	int i=0;
	tMenuItemIterator pMenuItem;
	for(pMenuItem = pMenu->MenuItemsList.begin(); pMenuItem != pMenu->MenuItemsList.end(); pMenuItem++)
	{
		if(pMenuItem->sText.length() > 0)
		{
			pMenuItem->nLeft		= x;
			pMenuItem->nTop			= y;
			pMenuItem->nWidth		= w;	
			pMenuItem->nHeight	= pMenu->nItemHeight;
			MenuDrawItem(pMenu, pMenuItem, (i == pMenu->nSelectedItem));

			y += pMenu->nItemHeight;
			FBFillRect(x, y, w, 2, BLACK);
			y += 2;
			i++;
		}
	}
#endif
}
