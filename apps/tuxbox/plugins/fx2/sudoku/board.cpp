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

#include "board.h"

#include <fstream>

#include "misc.h"

//#define TEST
#ifndef TEST
extern "C"
{
	#include <draw.h>
	#include <rcinput.h>
	#include <fx2math.h>
}
#endif

#define SCREEN_WIDTH										720
#define SCREEN_HEIGHT										576
#define SCREEN_BORDER										32

#define PROGRESSBAR_WIDTH								100
#define PROGRESSBAR_HEIGHT							24
#define PROGRESSBAR_LEFT								(SCREEN_WIDTH/2)-(PROGRESSBAR_WIDTH/2)
#define PROGRESSBAR_TOP									(SCREEN_HEIGHT/2)-(PROGRESSBAR_HEIGHT/2)

#define SCREEN_MINSIZE_PERC							30

#define BLOCK_SPACE											2
#define FIELD_BORDER										2

#define COLOR_BACKGROUND								BACKGROUND
#define COLOR_BORDER										STEELBLUE
#define COLOR_BORDER_HIGHLIGHTED				COLOR_BORDER
#define COLOR_TEXT											BLUE
#define COLOR_TEXT_FIXED								BLACKBLACK
#define COLOR_TEXT_MISTAKE							RED
#define COLOR_TEXTBG										0
#define COLOR_FIELD_HIGHLIGHTED					WHITE
#define COLOR_FIELDBG										GLASS
#define COLOR_MESSAGEBOX								RED
#define COLOR_MESSAGEBOXBG							BLACKBLACK

#define TEXT_HEIGHT											64
#define TEXT_HEIGHT_SMALL								32

#define CURSOR2COORDS(cursorx, cursory)	(cursorx / m_nBlockColumns), (cursory / m_nBlockRows), (cursorx % m_nFieldColumns), (cursory % m_nFieldRows)

#define MENU_CMD_EXIT										0
#define MENU_CMD_NEWGAME								1
#define MENU_CMD_LOADGAME								2
#define MENU_CMD_SAVEGAME								3
#define MENU_CMD_SHOW_MISTAKES					4
#define MENU_CMD_SHOW_SOLVABLE					5
#define MENU_CMD_BOARD_SIZE							6
#define MENU_CMD_SHOW_TV								7
#define MENU_CMD_ABOUT									8

#define MENU_CAPTION										"Sudoku"
#define MENU_TEXT_EXIT									"Exit"
#define	MENU_TEXT_NEWGAME								"Start new game (Level %d)"
#define	MENU_TEXT_LOADGAME							"Load board"
#define	MENU_TEXT_SAVEGAME							"Save board"
#define MENU_TEXT_SHOW_MISTAKES					"Show wrong fields on check: %s"
#define MENU_TEXT_SHOW_SOLVABLE					"Show if solvable on check: %s"
#define MENU_TEXT_BOARD_SIZE						"Change board size (%d%%)"
#define MENU_TEXT_SHOW_TV								"Show TV: %s"
#define MENU_TEXT_ABOUT									"About"

#define TEXT_ABOUT "Sudoku v1.2\nby M. Schlosser\nemail: tuxbox@software-schlosser.de\nIn memory of Klaus K."
#define TEXT_PLEASEWAIT									"please wait..."

#define TEXT_ON													"on"
#define TEXT_OFF												"off"

#define TEXT_RIGHT											"Right!"
#define TEXT_WRONG											"Wrong!"
#define TEXT_SOLVABLE										"Solvable"
#define TEXT_NOTSOLVABLE								"Not solvable!"

#define TEXTBOX_BORDER									128
#define TEXTBOX_X												300
#define TEXTBOX_Y												200
#define MSG_SLEEP												1000000

extern	int doexit;

extern	unsigned short	actcode;

// ----------------------------------------------------------------------------
// CBoard
// ----------------------------------------------------------------------------
CBoard::CBoard()
	: m_CursorX(0)
	, m_CursorY(0)
	, m_bShowMenu(false)
	, m_bShowMessageBox(false)
	, m_nLevel(LEVEL_DEFAULT)
	, m_bShowMistakes(false)
	, m_bShowSolvable(true)
	, m_nBoardSizePercent(100)
	, m_nShowTV(1)
	, m_nScreenLeft(SCREEN_BORDER)
	, m_nScreenTop(SCREEN_BORDER)
	, m_nScreenRight(SCREEN_WIDTH-SCREEN_BORDER)
	, m_nScreenBottom(SCREEN_HEIGHT-SCREEN_BORDER)
	, m_nProgressBarLeft(PROGRESSBAR_LEFT)
	, m_nProgressBarTop(PROGRESSBAR_TOP)
	, m_nProgress(0)
{
	InitColors();
	InitMenu();
}

// ----------------------------------------------------------------------------
// ~CBoard
// ----------------------------------------------------------------------------
CBoard::~CBoard()
{
}

// ----------------------------------------------------------------------------
// InitColors
// ----------------------------------------------------------------------------
void CBoard::InitColors(void)
{
#ifndef TEST
	FBSetColor(YELLOW,		255,	255,	32);
	FBSetColor(GREEN,			32,		255,	32);
	FBSetColor(STEELBLUE,	32,		32,		192);
	FBSetColor(BLUE,			72,		64,		255);
	FBSetColor(GRAY,			128,	128,	144);
	FBSetColor(BLACKBLACK,0,		0,		0);
	FBSetColorEx(BACKGROUND,	128,	128,	160, m_nShowTV == 1 ? 255 : 0);
	FBSetColorEx(GLASS,	224,	192,	255, m_nShowTV == 1 ? 127 : 0);

	FBSetupColors();
#endif
}

// ----------------------------------------------------------------------------
// InitMenu
// ----------------------------------------------------------------------------
void CBoard::InitMenu(void)
{
	m_Menu.sCaption = MENU_CAPTION;

	MenuAddItem(&m_Menu, MENU_TEXT_EXIT);
	MenuAddItem(&m_Menu, MENU_TEXT_NEWGAME, m_nLevel);
	MenuAddItem(&m_Menu, MENU_TEXT_LOADGAME);
	MenuAddItem(&m_Menu, MENU_TEXT_SAVEGAME);
	MenuAddItem(&m_Menu, MENU_TEXT_SHOW_MISTAKES, m_bShowMistakes ? TEXT_ON : TEXT_OFF);
	MenuAddItem(&m_Menu, MENU_TEXT_SHOW_SOLVABLE, m_bShowSolvable ? TEXT_ON : TEXT_OFF);
	MenuAddItem(&m_Menu, MENU_TEXT_BOARD_SIZE, m_nBoardSizePercent);
	MenuAddItem(&m_Menu, MENU_TEXT_SHOW_TV, (m_nShowTV == 1) ? TEXT_ON : TEXT_OFF);
	MenuAddItem(&m_Menu, MENU_TEXT_ABOUT);
}

// ----------------------------------------------------------------------------
// UpdateMenu
// ----------------------------------------------------------------------------
void CBoard::UpdateMenu(void)
{
	MenuSetItem(&m_Menu, MENU_CMD_NEWGAME, MENU_TEXT_NEWGAME, m_nLevel);
	MenuSetItem(&m_Menu, MENU_CMD_SHOW_MISTAKES,			MENU_TEXT_SHOW_MISTAKES,			m_bShowMistakes ? TEXT_ON : TEXT_OFF);
	MenuSetItem(&m_Menu, MENU_CMD_SHOW_SOLVABLE,			MENU_TEXT_SHOW_SOLVABLE,			m_bShowSolvable ? TEXT_ON : TEXT_OFF);
	MenuSetItem(&m_Menu, MENU_CMD_BOARD_SIZE,					MENU_TEXT_BOARD_SIZE,					m_nBoardSizePercent);
	MenuSetItem(&m_Menu, MENU_CMD_SHOW_TV,						MENU_TEXT_SHOW_TV,						(m_nShowTV == 1) ? TEXT_ON : TEXT_OFF);
}

// ----------------------------------------------------------------------------
// InitBoard
// ----------------------------------------------------------------------------
void CBoard::InitBoard()
{
	m_CursorX = 0;
	m_CursorY = 0;

	// move cursor to first free field
	while(IsSymbolFixed(CURSOR2COORDS(m_CursorX, m_CursorY)))
	{
		if(++m_CursorX > (m_mColumnLength-1))
		{
			m_CursorY++;
			m_CursorX = 0;
		}
	}

	m_nProgress = 0;

	// some precalculations
	m_nBoardLeft			= m_nScreenLeft;
	m_nBoardTop				= m_nScreenTop;
	m_nBoardRight			= ((SCREEN_WIDTH		- SCREEN_BORDER) * m_nBoardSizePercent / 100);
	m_nBoardBottom		= ((SCREEN_HEIGHT		- SCREEN_BORDER) * m_nBoardSizePercent / 100);

	m_nBlockWidth			= ((m_nBoardRight		- m_nBoardLeft)	/ m_nBlockColumns);
	m_nBlockHeight		= ((m_nBoardBottom	- m_nBoardTop)	/ m_nBlockRows);
	m_nFieldWidth			= (m_nBlockWidth		/ m_nFieldColumns);
	m_nFieldHeight		=	(m_nBlockHeight		/ m_nFieldRows);

	// add board border
	int nBoardSpaceX	= (m_nBlockColumns-1)	* BLOCK_SPACE;
	int nBoardSpaceY	= (m_nBlockRows-1)		* BLOCK_SPACE;
	m_nBoardLeft			-= (nBoardSpaceX/2);
	m_nBoardTop				-= (nBoardSpaceY/2);
	m_nBoardRight			+= (nBoardSpaceX/2);
	m_nBoardBottom		+= (nBoardSpaceY/2);

	m_mColumnLength		= m_nBlockColumns		* m_nFieldColumns;
	m_mRowLength			= m_nBlockRows 			* m_nFieldRows;

#ifndef TEST
	// draw board
	DrawBoard(DRAWBOARD_SHOW_DEFAULT);
	#ifdef USEX
		FBFlushGrafic();
	#endif
#endif
}


// ----------------------------------------------------------------------------
// NewGame
// ----------------------------------------------------------------------------
void CBoard::NewGame()
{
#ifndef TEST
	Message(m_nProgressBarLeft, 192, 48, TEXT_PLEASEWAIT, COLOR_MESSAGEBOX, COLOR_MESSAGEBOXBG);
#endif
	Create(m_nLevel);
}

// ----------------------------------------------------------------------------
// LoadSettings
// ----------------------------------------------------------------------------
int CBoard::LoadSettings(const char *pszFile)
{
	std::ifstream F;
	F.open(pszFile, std::ios_base::in | std::ios_base::binary);
	if(!F.good())
		return -1;

	int nVersion;
	F.read((char *) &nVersion,								sizeof(nVersion));

	F.read((char *) &m_nScreenLeft, 					sizeof(m_nScreenLeft));
	F.read((char *) &m_nScreenTop, 						sizeof(m_nScreenTop));
	F.read((char *) &m_nScreenRight,					sizeof(m_nScreenRight));	// eigentlich ueberfluessig...
	F.read((char *) &m_nScreenBottom,					sizeof(m_nScreenBottom));	// eigentlich ueberfluessig...
	F.read((char *) &m_nBoardSizePercent,			sizeof(m_nBoardSizePercent));

	F.read((char *) &m_nLevel,								sizeof(m_nLevel));
	F.read((char *) &m_bShowMistakes,					sizeof(m_bShowMistakes));
	F.read((char *) &m_bShowSolvable,					sizeof(m_bShowSolvable));

	if(nVersion >= 2)
	{
		F.read((char *) &m_nShowTV,							sizeof(m_nShowTV));
	}

	F.close();
	return 1;
}

// ----------------------------------------------------------------------------
// SaveSettings
// ----------------------------------------------------------------------------
int CBoard::SaveSettings(const char *pszFile)
{
	std::ofstream F;
	F.open(pszFile, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!F.good())
		return -1;

	int nVersion = SETTINGSFILE_VERSION;
	F.write((const char *) &nVersion,		 			sizeof(nVersion));

	F.write((char *) &m_nScreenLeft, 					sizeof(m_nScreenLeft));
	F.write((char *) &m_nScreenTop, 					sizeof(m_nScreenTop));
	F.write((char *) &m_nScreenRight,					sizeof(m_nScreenRight));	// eigentlich ueberfluessig...
	F.write((char *) &m_nScreenBottom, 				sizeof(m_nScreenBottom));	// eigentlich ueberfluessig...
	F.write((char *) &m_nBoardSizePercent,		sizeof(m_nBoardSizePercent));

	F.write((const char *) &m_nLevel, 				sizeof(m_nLevel));
	F.write((const char *) &m_bShowMistakes,	sizeof(m_bShowMistakes));
	F.write((const char *) &m_bShowSolvable,	sizeof(m_bShowSolvable));

	if(nVersion >= 2)
	{
		F.write((char *) &m_nShowTV,						sizeof(m_nShowTV));
	}

	F.close();
	return 1;
}

// ----------------------------------------------------------------------------
// Coord2Screen
// ----------------------------------------------------------------------------
tPoint CBoard::Coord2Screen(int nBlockCol, int nBlockRow, int nFieldCol, int nFieldRow)
{
	tPoint p;
	p.x = m_nBoardLeft	+ (nBlockCol * m_nBlockWidth)		+ (nFieldCol * m_nFieldWidth)		+ (nBlockCol * BLOCK_SPACE);
	p.y = m_nBoardTop		+ (nBlockRow * m_nBlockHeight)	+ (nFieldRow * m_nFieldHeight)	+ (nBlockRow * BLOCK_SPACE);
	return p;
}

// ----------------------------------------------------------------------------
// DrawField
// ----------------------------------------------------------------------------
void CBoard::DrawField(int nBlockCol, int nBlockRow, int nFieldCol, int nFieldRow, int nBorderColor, int nFillColor, int nShow)
{
#ifndef TEST
	tPoint p = Coord2Screen(nBlockCol, nBlockRow, nFieldCol, nFieldRow);
	if(nFillColor > 0)
	{
		FBFillRect(p.x+FIELD_BORDER, p.y+FIELD_BORDER, m_nFieldWidth-FIELD_BORDER, m_nFieldHeight-FIELD_BORDER, nFillColor);
	}

	tSymbol *pSymbol = GetSymbol(nBlockCol, nBlockRow, nFieldCol, nFieldRow);
	char szSymbol[2] = { pSymbol->cSymbol, '\0' };
	int nColorText = pSymbol->bFixed ? COLOR_TEXT_FIXED : COLOR_TEXT;
	if((nShow & DRAWBOARD_SHOW_MISTAKES) && pSymbol->cSymbol != pSymbol->cSolution)
		nColorText = COLOR_TEXT_MISTAKE;
	FBDrawString(p.x+(m_nFieldWidth/4)+(FIELD_BORDER*2), p.y+FIELD_BORDER, m_nFieldHeight, szSymbol, nColorText, COLOR_TEXTBG);
#endif
}

// ----------------------------------------------------------------------------
// DrawBlock
// ----------------------------------------------------------------------------
void CBoard::DrawBlock(int nBlockCol, int nBlockRow, int nFillColor, int nShow)
{
	int r, c;
	for(r=0; r<m_nFieldRows; r++)
	{
		for(c=0; c<m_nFieldColumns; c++)
		{
			DrawField(nBlockCol, nBlockRow, c, r, COLOR_BORDER, COLOR_FIELDBG, nShow);
		}
	}
}


// ----------------------------------------------------------------------------
// DrawBackground
// ----------------------------------------------------------------------------
void CBoard::DrawBackground(void)
{
#ifndef TEST
	FBFillRect(m_nBoardLeft, m_nBoardTop, m_nBoardRight-m_nBoardLeft, m_nBoardBottom-m_nBoardTop, BLACKBLACK);
#endif
}

// ----------------------------------------------------------------------------
// DrawBoard
// ----------------------------------------------------------------------------
void CBoard::DrawBoard(int nShow)
{
#ifndef TEST
	FBFillRect(0, 0, 720, 576, BACKGROUND);
	DrawBackground();

	int br, bc;
	for(br=0; br<m_nBlockRows; br++)
	{
		for(bc=0; bc<m_nBlockColumns; bc++)
		{
			DrawBlock(bc, br, GRAY, nShow);
		}
	}

	DrawField(CURSOR2COORDS(m_CursorX, m_CursorY), COLOR_BORDER_HIGHLIGHTED, COLOR_FIELD_HIGHLIGHTED, DRAWBOARD_SHOW_DEFAULT);
//	FBDrawFx2Logo(FX2LOGO_X, FX2LOGO_Y);
#endif
}

// ----------------------------------------------------------------------------
// SetCurrentSymbol
// ----------------------------------------------------------------------------
void CBoard::SetCurrentSymbol(char cSymbol)
{
	tSymbol *pSymbol = GetSymbol(CURSOR2COORDS(m_CursorX, m_CursorY));
	if(!pSymbol->bFixed)
		pSymbol->cSymbol = cSymbol;
}
/*
#define MOVECURSOR(move)	{																																			\
		int nOldX = m_CursorX;																																			\
		int nOldY = m_CursorY;																																			\
		DrawField(CURSOR2COORDS(m_CursorX, m_CursorY), COLOR_BORDER, COLOR_FIELDBG, DRAWBOARD_SHOW_DEFAULT);	\
		do																																													\
		{																																														\
			move;																																											\
			if(m_CursorX < 0)	m_CursorX = (m_mColumnLength-1);																				\
			if(m_CursorY < 0)	m_CursorY = (m_mRowLength-1);																						\
			if(m_CursorX >= m_mColumnLength)	m_CursorX = 0;																					\
			if(m_CursorY >= m_mRowLength)			m_CursorY = 0;																					\
			if(m_CursorX == nOldX && m_CursorY == nOldY) break;																				\
		}																																														\
		while(IsSymbolFixed(CURSOR2COORDS(m_CursorX, m_CursorY)));																	\
		DrawField(CURSOR2COORDS(m_CursorX, m_CursorY), COLOR_BORDER_HIGHLIGHTED, COLOR_FIELD_HIGHLIGHTED, DRAWBOARD_SHOW_DEFAULT);	\
	}
*/
#define MOVECURSOR(move)	{															\
		DrawField(CURSOR2COORDS(m_CursorX, m_CursorY), COLOR_BORDER, COLOR_FIELDBG, DRAWBOARD_SHOW_DEFAULT);	\
		move;																								\
		if(m_CursorX < 0)	m_CursorX = (m_mColumnLength-1);	\
		if(m_CursorY < 0)	m_CursorY = (m_mRowLength-1);			\
		if(m_CursorX >= m_mColumnLength)	m_CursorX = 0;		\
		if(m_CursorY >= m_mRowLength)			m_CursorY = 0;		\
		DrawField(CURSOR2COORDS(m_CursorX, m_CursorY), COLOR_BORDER_HIGHLIGHTED, COLOR_FIELD_HIGHLIGHTED, DRAWBOARD_SHOW_DEFAULT);	\
	}

// ----------------------------------------------------------------------------
// MoveCursor
// ----------------------------------------------------------------------------
void CBoard::MoveCursor(void)
{
#ifndef TEST
	if(m_bShowMessageBox)
	switch(actcode)
	{
//		case RC_HOME:
//		case RC_OK:
		case 0xEE:
		break;
		default:
			m_bShowMessageBox = false;
			if(m_bShowMenu)
				MenuDraw(&m_Menu);
			else
				DrawBoard(DRAWBOARD_SHOW_DEFAULT);
		break;
	}
	else if(m_bShowMenu)
	switch(actcode)
	{
		case RC_DOWN:
		{
			int nLastSelection = m_Menu.nSelectedItem;
			if(++m_Menu.nSelectedItem >= m_Menu.nItemsCount)
				m_Menu.nSelectedItem = 0;
			MenuDrawItem(&m_Menu, nLastSelection);
			MenuDrawItem(&m_Menu, m_Menu.nSelectedItem);
		}			
		break;

		case RC_UP:
		{
			int nLastSelection = m_Menu.nSelectedItem;
			if(--m_Menu.nSelectedItem < 0)
				m_Menu.nSelectedItem = m_Menu.nItemsCount-1;
			MenuDrawItem(&m_Menu, nLastSelection);
			MenuDrawItem(&m_Menu, m_Menu.nSelectedItem);
		}			
		break;

		case RC_MINUS:
		case RC_LEFT:
			switch(m_Menu.nSelectedItem)
			{
				case MENU_CMD_NEWGAME:
					if(m_nLevel > LEVEL_MIN)	m_nLevel--;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_NEWGAME, m_nLevel);
				break;

				case MENU_CMD_SHOW_MISTAKES:
					m_bShowMistakes = false;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_SHOW_MISTAKES, m_bShowMistakes ? TEXT_ON : TEXT_OFF);
				break;

				case MENU_CMD_SHOW_SOLVABLE:
					m_bShowSolvable = false;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_SHOW_SOLVABLE, m_bShowSolvable ? TEXT_ON : TEXT_OFF);
				break;

				case MENU_CMD_BOARD_SIZE:
					if(m_nBoardSizePercent > SCREEN_MINSIZE_PERC)	m_nBoardSizePercent-=2;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_BOARD_SIZE, m_nBoardSizePercent);
				break;

				case MENU_CMD_SHOW_TV:
					m_nShowTV = 0;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_SHOW_TV, (m_nShowTV == 1) ? TEXT_ON : TEXT_OFF);
				break;
			}
		break;

		case RC_PLUS:
		case RC_RIGHT:
			switch(m_Menu.nSelectedItem)
			{
				case MENU_CMD_NEWGAME:
					if(m_nLevel < LEVEL_MAX)	m_nLevel++;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_NEWGAME, m_nLevel);
				break;

				case MENU_CMD_SHOW_MISTAKES:
					m_bShowMistakes = true;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_SHOW_MISTAKES, m_bShowMistakes ? TEXT_ON : TEXT_OFF);
				break;

				case MENU_CMD_SHOW_SOLVABLE:
					m_bShowSolvable = true;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_SHOW_SOLVABLE, m_bShowSolvable ? TEXT_ON : TEXT_OFF);
				break;

				case MENU_CMD_BOARD_SIZE:
					if(m_nBoardSizePercent < 100)	m_nBoardSizePercent+=2;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_BOARD_SIZE, m_nBoardSizePercent);
				break;

				case MENU_CMD_SHOW_TV:
					m_nShowTV = 1;
					MenuSetItem(&m_Menu, m_Menu.nSelectedItem, MENU_TEXT_SHOW_TV, (m_nShowTV == 1) ? TEXT_ON : TEXT_OFF);
				break;
			}
		break;

		case RC_OK:
			if(DoMenuCmd())
			{
				m_bShowMenu = false;
				InitColors();	// if m_nShowTV changed...
				DrawBoard(DRAWBOARD_SHOW_DEFAULT);
			}
		break;

//		case RC_HOME:
//		case RC_RED:
//		case RC_YELLOW:
//		case RC_GREEN:
		case RC_SETUP:
		case RC_BLUE:
			m_bShowMenu = false;
			DrawBoard(DRAWBOARD_SHOW_DEFAULT);
		break;
	}
	else
	switch(actcode)
	{
		case RC_RIGHT:
			MOVECURSOR(m_CursorX++);
		break;

		case RC_LEFT:
			MOVECURSOR(m_CursorX--);
		break;
	
		case RC_DOWN:
			MOVECURSOR(m_CursorY++);
		break;

		case RC_UP:
			MOVECURSOR(m_CursorY--);
		break;

//		case RC_1:
//		case RC_2:
//		case RC_3:
//		case RC_4:
//		case RC_5:
//		case RC_6:
//		case RC_7:
//		case RC_8:
//		case RC_9:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			SetCurrentSymbol('0' + actcode);
			DrawField(CURSOR2COORDS(m_CursorX, m_CursorY), COLOR_BORDER_HIGHLIGHTED, COLOR_FIELD_HIGHLIGHTED, DRAWBOARD_SHOW_DEFAULT);
		break;

//		case RC_0:
		case 0:
			SetCurrentSymbol(0);
			DrawField(CURSOR2COORDS(m_CursorX, m_CursorY), COLOR_BORDER_HIGHLIGHTED, COLOR_FIELD_HIGHLIGHTED, DRAWBOARD_SHOW_DEFAULT);
		break;

		case RC_GREEN:
		case RC_OK:
		{
			if(m_bShowMistakes)
				DrawBoard(DRAWBOARD_SHOW_MISTAKES);

			// check if sudoku is erroneous and/or incomplete
			int nCheck = CheckBoard();
			const char *pszMsg = NULL;
			if(nCheck & CHECK_ERRONEOUS)
			{
				if(nCheck & CHECK_INCOMPLETE)				// if the sudouku incomplete...
				{
					if(m_bShowSolvable)								// ...and if we want to know if the sudoku is solvable...
						pszMsg = TEXT_NOTSOLVABLE;			// ...show "not solvable"
				}
				else
				{
					pszMsg = TEXT_WRONG;							// ...show "wrong"
				}
			}
			else if(nCheck & CHECK_INCOMPLETE)
			{
				if(m_bShowSolvable)									// ...if we want to know if the sudoku is solvable...
					pszMsg = TEXT_SOLVABLE;						// ...show "solvable"
			}
			else
			{
				pszMsg = TEXT_RIGHT;
			}

			if(pszMsg != NULL)
			{
				MessageBox(TEXTBOX_X, TEXTBOX_Y, TEXT_HEIGHT, (char *) pszMsg, COLOR_MESSAGEBOX, COLOR_MESSAGEBOXBG);
			}
		}
		break;

//		case RC_RED:
//		case RC_YELLOW:
//		case RC_GREEN:
		case RC_SETUP:
		case RC_BLUE:
			m_bShowMenu = true;
			MenuDraw(&m_Menu);
			UpdateMenu();
		break;

		case RC_HOME:
			doexit = 1;
		break;
	}
#endif
}

// ----------------------------------------------------------------------------
// DoMenuCmd
// ----------------------------------------------------------------------------
bool CBoard::DoMenuCmd(void)
{
#ifndef TEST
	switch(m_Menu.nSelectedItem)
	{
		case MENU_CMD_EXIT:
		break;

		case MENU_CMD_NEWGAME:
			NewGame();
			InitBoard();
		break;

		case MENU_CMD_LOADGAME:
			LoadGame(GAMEFILE);
			InitBoard();
		break;

		case MENU_CMD_SAVEGAME:
			SaveGame(GAMEFILE);
		break;

		case MENU_CMD_BOARD_SIZE:
			InitBoard();
		break;

		case MENU_CMD_ABOUT:
			MessageBox(200, 200, 48, TEXT_ABOUT, COLOR_MESSAGEBOX, COLOR_MESSAGEBOXBG);
			return false;
		break;
	}
	return true;
#endif
}

// ----------------------------------------------------------------------------
// Message
// ----------------------------------------------------------------------------
void CBoard::Message(int x, int y, int dy, const char *pszMessage, int nColorText, int nColorBg)
{
#ifndef TEST
	std::string s(pszMessage);

	int nLineCount = 1;
	std::string::size_type nMaxStrLen = 0;
	std::string::size_type nStart = 0, nEnd;
	while((nEnd = s.find('\n', nStart)) != std::string::npos)
	{
		if(s.substr(nStart, nEnd-nStart).length() > nMaxStrLen)
			nMaxStrLen = s.substr(nStart, nEnd-nStart).length();
		nLineCount++;
		nStart = nEnd+1;
	}
	if(s.substr(nStart, s.length()-nStart).length() > nMaxStrLen)
		nMaxStrLen = s.substr(nStart, s.length()-nStart).length();

	if(nLineCount > 1)
	{
//		FBFillRect(x, y, nMaxStrLen*(dy/4), nLineCount*dy, nColorBg);
		FBFillRect(TEXTBOX_BORDER, y, SCREEN_WIDTH-(2*TEXTBOX_BORDER), nLineCount*dy, nColorBg);
	}

	nStart = 0;
	while((nEnd = s.find('\n', nStart)) != std::string::npos)
	{
		FBDrawString(x, y, dy, (char *) s.substr(nStart, nEnd-nStart).c_str(), nColorText, nColorBg);
		y += dy;
		nStart = nEnd+1;
	}
	FBDrawString(x, y, dy, (char *) s.substr(nStart, s.length()-nStart).c_str(), nColorText, nColorBg);
#endif
}

// ----------------------------------------------------------------------------
// MessageBox
// ----------------------------------------------------------------------------
void CBoard::MessageBox(int x, int y, int dy, const char *pszMessage, int nColorText, int nColorBg)
{
	m_bShowMessageBox = true;
	Message(x, y, dy, pszMessage, nColorText, nColorBg);
}

// ----------------------------------------------------------------------------
// CallbackCreate
// ----------------------------------------------------------------------------
void CBoard::CallbackCreate(unsigned long nValue)
{
	m_nProgress+=2;
	if(m_nProgress > PROGRESSBAR_WIDTH)
		m_nProgress = 0;
#ifndef TEST
	FBFillRect(m_nProgressBarLeft-1, m_nProgressBarTop-1, PROGRESSBAR_WIDTH+1, PROGRESSBAR_HEIGHT+1, BLACKBLACK);
	FBFillRect(m_nProgressBarLeft, m_nProgressBarTop, m_nProgress, PROGRESSBAR_HEIGHT, BLUE);
#else
//	cout << "Progress: " << m_nProgress << endl;	
#endif
}
