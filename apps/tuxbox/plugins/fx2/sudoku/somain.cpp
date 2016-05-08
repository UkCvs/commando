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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define TEST
#ifdef TEST
#include <iostream.h>
#include "sudoku.h"
void ShowSudoku(CSudoku &rSudoku, bool bFixed)
{
	int bc, br, fr, fc;
	tSymbol *pSymbol;
	for(br=0; br<rSudoku.m_nBlockRows; br++)
	{
		for(fr=0; fr<rSudoku.m_nFieldRows; fr++)
		{
			for(bc=0; bc<rSudoku.m_nBlockColumns; bc++)
			{
				for(fc=0; fc<rSudoku.m_nFieldColumns; fc++)
				{
					pSymbol = rSudoku.GetSymbol(bc, br, fc, fr);
					if(bFixed)
					{
						cout << pSymbol->cSolution;
					}
					else
					{
							if(pSymbol->cSymbol > 0)
								cout << pSymbol->cSymbol;
							else
								cout << " ";
					}
					cout << " ";
				}
				cout << " | ";
			}
			cout << endl;
		}
		cout << "------------------------" << endl;
	}
	cout << endl;
}

// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	srand(time(NULL));

	CSudoku Board;
//	Board.LoadSettings(SETTINGSFILE);
//	if(Board.LoadGame(GAMEFILE) <= 0)
//		Board.NewGame();
//	Board.InitBoard();

	Board.Create(5);

	ShowSudoku(Board, true);
	ShowSudoku(Board, false);

	return 0;
}
#else
extern "C"
{
	#include <draw.h>
	#include <fx2math.h>
	#include <rcinput.h>
	
	#include <pig.h>
	#include <plugin.h>
}

#include "board.h"
#include "misc.h"

extern unsigned short	actcode;
extern unsigned short realcode;
extern int doexit;
extern int debug;

// ----------------------------------------------------------------------------
// shutdown
// ----------------------------------------------------------------------------
void shutdown()
{
	Sleep(300000);	// 300ms pause
/*
	realcode = RC_0;
	while(realcode != 0xEE)
	{
		RcGetActCode();
	}
*/

	RcClose();
	FBClose();
}

// ----------------------------------------------------------------------------
// main_exec
// ----------------------------------------------------------------------------
int main_exec(int fdfb, int fdrc, int fdlcd, char *cfgfile)
{
//	srand(time(NULL));

	if(FBInitialize(720, 576, 8, fdfb) < 0)
		return -1;

	if(RcInitialize(fdrc) < 0)
		return -1;

	CBoard Board;
	Board.LoadSettings(SETTINGSFILE);
	if(Board.LoadGame(GAMEFILE) <= 0)
		Board.NewGame();
	Board.InitBoard();

	doexit = 0;
	while(!doexit)
	{
		actcode = 0xEE;
		RcGetActCode();

		Board.MoveCursor();

		while(realcode != 0xEE)
			RcGetActCode();
	}

	Board.SaveSettings(SETTINGSFILE);
	Board.SaveGame(GAMEFILE);

	shutdown();
	return 0;
}

extern "C"
{
	// ----------------------------------------------------------------------------
	// plugin_exec
	// ----------------------------------------------------------------------------
	int plugin_exec(PluginParam *par)
	{
		int fd_fb	= -1;
		int fd_rc	= -1;
		
		for( ; par; par=par->next)
		{
			if(strcmp(par->id, P_ID_FBUFFER) == 0)
				fd_fb				= _atoi(par->val);
			else if(strcmp(par->id, P_ID_RCINPUT) == 0)
				fd_rc				= _atoi(par->val);
			else if(strcmp(par->id, P_ID_NOPIG) == 0)
				fx2_use_pig	= !_atoi(par->val);
		}

		return main_exec(fd_fb, fd_rc, -1, NULL);
	}
}
#endif
