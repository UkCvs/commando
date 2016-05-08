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

#ifndef _BOARD_H
#define _BOARD_H

#ifndef _COLORS_H
	#include "colors.h"
#endif

#ifndef _SUDOKU_H
	#include "sudoku.h"
#endif

#ifndef _MENU_H
	#include "menu.h"
#endif

typedef struct
{
	int x;
	int y;
} tPoint;

#define DRAWBOARD_SHOW_DEFAULT					0
#define DRAWBOARD_SHOW_MISTAKES					1

#define LEVEL_MIN												1
#define LEVEL_MAX												7
#define LEVEL_DEFAULT										5

#define GAMEFILE												"/var/tuxbox/config/sudoku.sav"
#define SETTINGSFILE										"/var/tuxbox/config/sudoku.conf"
#define SETTINGSFILE_VERSION						2

class CBoard: public CSudoku
{
public:
	CBoard();
	virtual ~CBoard();

public:
	void								MoveCursor(void);
	void								InitBoard();
	void								NewGame();
	int									LoadSettings(const char *pszFile);
	int									SaveSettings(const char *pszFile);

private:
	void								InitColors(void);
	void								InitMenu(void);
	void								UpdateMenu(void);
	tPoint							Coord2Screen(int nBlockCol, int nBlockRow, int nFieldCol, int nFieldRow);
	void								DrawField(int nBlockCol, int nBlockRow, int nFieldCol, int nFieldRow, int nBorderColor, int nFillColor, int nShow);
	void								DrawBlock(int nBlockCol, int nBlockRow, int nFillColor, int nShow);
	void								DrawBackground(void);
	void								DrawBoard(int nShow);
	void								SetCurrentSymbol(char cSymbol);

	bool								DoMenuCmd(void);
	void								Message(int x, int y, int dy, const char *pszMessage, int nColorText, int nColorBg);
	void								MessageBox(int x, int y, int dy, const char *pszMessage, int nColorText, int nColorBg);

private:	// overrides
	void								CallbackCreate(unsigned long nValue);

private:
	int									m_CursorX;
	int									m_CursorY;

	bool								m_bShowMenu;
	tMenu								m_Menu;

	bool								m_bShowMessageBox;

	// settings
	int									m_nLevel;
	bool								m_bShowMistakes;
	bool								m_bShowSolvable;

	int									m_nBoardSizePercent;
	int									m_nShowTV;

private:	// pre calculated numbers (optimizations)
	int									m_nScreenLeft;
	int									m_nScreenTop;
	int									m_nScreenRight;
	int									m_nScreenBottom;

	int									m_nBoardLeft;
	int									m_nBoardTop;
	int									m_nBoardRight;
	int									m_nBoardBottom;

	int									m_nBlockWidth;
	int									m_nBlockHeight;
	int									m_nBlockSize;
	int									m_nFieldWidth;
	int									m_nFieldHeight;

	int									m_mColumnLength;
	int									m_mRowLength;

	int									m_nProgressBarLeft;
	int									m_nProgressBarTop;
	int									m_nProgress;
};

#endif	// _BOARD_H
