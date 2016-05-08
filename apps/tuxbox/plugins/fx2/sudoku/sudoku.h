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

#ifndef _SUDOKU_H
#define _SUDOKU_H

#include <string.h>

#define GAMEFILE_VERSION								1

#define CHECK_OK												0
#define CHECK_INCOMPLETE								1
#define CHECK_ERRONEOUS									2

#define MAX_SOLVE_CALLS									10000

typedef struct
{
	char	cSymbol;
	char	cSolution;
	bool	bFixed;
} tSymbol;

class CSudoku
{
public:
	CSudoku();
	virtual ~CSudoku();

public:
	void								Init();
	void								Shutdown(bool bKeepSymbols = false);
	void								SetSymbols(const char *pszSymbols);
	tSymbol							*GetSymbol(int nBlockCol, int nBlockRow, int nFieldCol, int nFieldRow);
	tSymbol							*GetSymbol(int nIndex);
	bool								IsSymbolFixed(int nBlockCol, int nBlockRow, int nFieldCol, int nFieldRow);
	int									CheckBlock(int nBlockCol, int nBlockRow, char cSymbol);
	int									CheckCol(int nBlockRow, int nFieldRow, char cSymbol);
	int									CheckRow(int nBlockCol, int nFieldCol, char cSymbol);
	int									CheckBlockAll(int nBlockCol, int nBlockRow);
	int									CheckColAll(int nBlockRow, int nFieldRow);
	int									CheckRowAll(int nBlockCol, int nFieldCol);
	int									CheckBoard(void);

	int									GetSolutionsCount();
	int									Solve(bool bCountSolutions = false);
	void								Create(int nLevel);

	int									LoadGame(const char *pszFile);
	int									SaveGame(const char *pszFile);

private:
	int									Create();
	int									Solve(int nIndex, bool bCountSolutions = false);

protected:	// overridables
	virtual void				CallbackCreate(unsigned long nValue) {};

private:
	tSymbol							*m_Board;
	tSymbol							*m_TempBoard;
	char								*m_pszSymbols;

	int									m_nSolutionsCount;

public:	// board settings
	int									m_nBlockColumns;
	int									m_nBlockRows;
	int									m_nFieldColumns;
	int									m_nFieldRows;

private:	// pre calculated numbers (optimizations)
	size_t							m_nSymbolsCount;
	int									m_nFieldsCount;
	int									m_nBlockSize;
	int									m_nBlockRowsSize;

	int									**m_pnBlockLookup;
	int									*m_pnFieldRowLookup;

	int									*m_pnBlockColIndexLookup;
	int									*m_pnBlockRowIndexLookup;
	int									*m_pnFieldColIndexLookup;
	int									*m_pnFieldRowIndexLookup;

private:	// function call counters
	unsigned long				m_nCount1;
	unsigned long				m_nCount2;
};

#endif // _SUDOKU_H
