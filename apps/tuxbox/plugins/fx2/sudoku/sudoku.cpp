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

#include <config.h>
#include "sudoku.h"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>

//#define _DEBUG_


#ifdef HAVE_DBOX_HARDWARE
	#include "misc.h"
#else
	int Rand()
	{
		return rand();
	}
#endif

//#define _TEST_
#ifdef _TEST_
	static char testsudoku[] =
	{
		'5', '3', 0, 0, '7', 0, 0, 0, 0,
		'6', 0, 0, '1', '9', '5', 0, 0, 0,
		0, '9', '8', 0, 0, 0, 0, '6', 0,
		'8', 0, 0, 0, '6', 0, 0, 0, '3',
		'4', 0, 0, '8', 0, '3', 0, 0, '1',
		'7', 0, 0, 0, '2', 0, 0, 0, '6',
		0, '6', 0, 0, 0, 0, '2', '8', 0,
		0, 0, 0, '4', '1', '9', 0, 0, '5',
		0, 0, 0, 0, '8', 0, 0, '7', '9'
	};
#endif

#define SYMBOLS													"123456789"

#define SAFEDELETEARRAY(ptr)	if(ptr != NULL) { delete [] ptr; ptr = NULL; }

#ifdef _DEBUG_
	#ifdef HAVE_DBOX_HARDWARE
		#include <iostream.h>
	#else
		#include <iostream>
		using namespace std;
	#endif

// ----------------------------------------------------------------------------
// _ShowSudoku
// ----------------------------------------------------------------------------
void _ShowSudoku(CSudoku &rSudoku, bool bFixed)
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
#endif

// ----------------------------------------------------------------------------
// CSudoku
// ----------------------------------------------------------------------------
CSudoku::CSudoku()
	: m_Board(NULL)
	, m_TempBoard(NULL)
	, m_pszSymbols(NULL)
	, m_nBlockColumns(3)
	, m_nBlockRows(3)
	, m_nFieldColumns(3)
	, m_nFieldRows(3)
	, m_pnBlockLookup(NULL)
	, m_pnFieldRowLookup(NULL)
	, m_pnBlockColIndexLookup(NULL)
	, m_pnBlockRowIndexLookup(NULL)
	, m_pnFieldColIndexLookup(NULL)
	, m_pnFieldRowIndexLookup(NULL)
	, m_nCount1(0)
	, m_nCount2(0)
{
	SetSymbols(SYMBOLS);
	Init();
}

// ----------------------------------------------------------------------------
// ~CSudoku
// ----------------------------------------------------------------------------
CSudoku::~CSudoku()
{
	Shutdown();
}

// ----------------------------------------------------------------------------
// Init
// ----------------------------------------------------------------------------
void CSudoku::Init()
{
	int nBlockIdx, nBlockRow, nBlockCol, nFieldIdx, nFieldRow, nFieldCol, nIndex;

	// some precalculations
	m_nBlockSize			= m_nFieldColumns	* m_nFieldRows;
	m_nFieldsCount		= m_nBlockSize		* m_nBlockColumns * m_nBlockRows;
	m_nBlockRowsSize	= m_nBlockSize		* m_nBlockColumns;

	Shutdown(true);

	m_pnBlockLookup					= new int*[m_nBlockRows];
	m_pnFieldRowLookup			= new int[m_nFieldRows];

	for(nBlockRow=0; nBlockRow<m_nBlockRows; nBlockRow++)
	{
		m_pnBlockLookup[nBlockRow] = new int[m_nBlockColumns];
		for(nBlockCol=0; nBlockCol<m_nBlockColumns; nBlockCol++)
		{
			m_pnBlockLookup[nBlockRow][nBlockCol] = (nBlockRow*m_nBlockRowsSize) + (nBlockCol*m_nBlockSize);
		}
	}

	for(nFieldRow=0; nFieldRow<m_nFieldRows; nFieldRow++)
		m_pnFieldRowLookup[nFieldRow] = nFieldRow * m_nFieldColumns;

	m_pnBlockColIndexLookup = new int[m_nFieldsCount];
	m_pnBlockRowIndexLookup = new int[m_nFieldsCount];
	m_pnFieldColIndexLookup = new int[m_nFieldsCount];
	m_pnFieldRowIndexLookup = new int[m_nFieldsCount];

	for(nIndex=0; nIndex<m_nFieldsCount; nIndex++)
	{
		nBlockIdx = nIndex / m_nBlockSize;
		nBlockRow = nBlockIdx % m_nBlockColumns;
		nBlockCol = nBlockIdx / 3;
		nFieldIdx = nIndex - (nBlockIdx * m_nBlockSize);
		nFieldRow = nFieldIdx % m_nFieldColumns;
		nFieldCol = nFieldIdx / m_nFieldColumns;

		m_pnBlockColIndexLookup[nIndex]	= nBlockCol;
		m_pnBlockRowIndexLookup[nIndex]	= nBlockRow;
		m_pnFieldColIndexLookup[nIndex]	= nFieldRow;
		m_pnFieldRowIndexLookup[nIndex]	= nFieldCol;
	}

	// board
	m_Board			= new tSymbol[m_nFieldsCount];
	m_TempBoard	= new tSymbol[m_nFieldsCount];

	m_nCount1		= 0;
	m_nCount2		= 0;
}

// ----------------------------------------------------------------------------
// Shutdown
// ----------------------------------------------------------------------------
void CSudoku::Shutdown(bool bKeepSymbols)
{
	if(m_pnBlockLookup != NULL)
	{
		int nBlockRow;
		for(nBlockRow=0; nBlockRow<m_nBlockRows; nBlockRow++)
		{
			if(m_pnBlockLookup[nBlockRow] != NULL)
			{
				delete [] m_pnBlockLookup[nBlockRow];
				m_pnBlockLookup[nBlockRow] = NULL;
			}
		}
		delete [] m_pnBlockLookup;
		m_pnBlockLookup = NULL;
	}
	SAFEDELETEARRAY(m_pnFieldRowLookup);

	SAFEDELETEARRAY(m_pnBlockColIndexLookup);
	SAFEDELETEARRAY(m_pnBlockRowIndexLookup);
	SAFEDELETEARRAY(m_pnFieldColIndexLookup);
	SAFEDELETEARRAY(m_pnFieldRowIndexLookup);

	if(!bKeepSymbols)
	{
		SAFEDELETEARRAY(m_pszSymbols);
	}

	SAFEDELETEARRAY(m_Board);
	SAFEDELETEARRAY(m_TempBoard);
}

// ----------------------------------------------------------------------------
// Shutdown
// ----------------------------------------------------------------------------
void CSudoku::SetSymbols(const char *pszSymbols)
{
	SAFEDELETEARRAY(m_pszSymbols);

	m_nSymbolsCount		= strlen(pszSymbols);
	m_pszSymbols			= new char[m_nSymbolsCount+1];
	strcpy(m_pszSymbols, pszSymbols);
}

// ----------------------------------------------------------------------------
// GetSymbol
// ----------------------------------------------------------------------------
tSymbol	*CSudoku::GetSymbol(int nBlockCol, int nBlockRow, int nFieldCol, int nFieldRow)
{
//	return &m_Board[(nBlockRow*m_nBlockRowsSize) + (nBlockCol*m_nBlockSize) + (nFieldRow*m_nFieldColumns) + nFieldCol];
	return &m_Board[m_pnBlockLookup[nBlockCol][nBlockRow] + m_pnFieldRowLookup[nFieldRow] + nFieldCol];
}

// ----------------------------------------------------------------------------
// GetSymbol
// ----------------------------------------------------------------------------
tSymbol	*CSudoku::GetSymbol(int nIndex)
{
	return &m_Board[nIndex];
}

// ----------------------------------------------------------------------------
// IsSymbolFixed
// ----------------------------------------------------------------------------
bool CSudoku::IsSymbolFixed(int nBlockCol, int nBlockRow, int nFieldCol, int nFieldRow)
{
	return GetSymbol(nBlockCol, nBlockRow, nFieldCol, nFieldRow)->bFixed;
}

// ----------------------------------------------------------------------------
// CheckBlock
// ----------------------------------------------------------------------------
int CSudoku::CheckBlock(int nBlockCol, int nBlockRow, char cSymbol)
{
	int fr, fc;
	tSymbol *pSymbol;
	for(fr=0; fr<m_nFieldRows; fr++)
	{
		for(fc=0; fc<m_nFieldColumns; fc++)
		{
			pSymbol = GetSymbol(nBlockCol, nBlockRow, fc, fr);
			if(pSymbol->bFixed)
			{
				if(pSymbol->cSolution	== cSymbol)
					return 1;
			}
			else
			{
				if(pSymbol->cSymbol		== cSymbol)
					return 1;
			}
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------
// CheckCol
// ----------------------------------------------------------------------------
int CSudoku::CheckCol(int nBlockRow, int nFieldRow, char cSymbol)
{
	int nBlockCol, nFieldCol;
	tSymbol *pSymbol;
	for(nBlockCol=0; nBlockCol<m_nBlockColumns; nBlockCol++)
	{
		for(nFieldCol=0; nFieldCol<m_nFieldColumns; nFieldCol++)
		{
			pSymbol = GetSymbol(nBlockCol, nBlockRow, nFieldCol, nFieldRow);
			if(pSymbol->bFixed)
			{
				if(pSymbol->cSolution	== cSymbol)
					return 1;
			}
			else
			{
				if(pSymbol->cSymbol		== cSymbol)
					return 1;
			}
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------
// CheckRow
// ----------------------------------------------------------------------------
int CSudoku::CheckRow(int nBlockCol, int nFieldCol, char cSymbol)
{
	int nBlockRow, nFieldRow;
	tSymbol *pSymbol;
	for(nBlockRow=0; nBlockRow<m_nBlockRows; nBlockRow++)
	{
		for(nFieldRow=0; nFieldRow<m_nFieldRows; nFieldRow++)
		{
			pSymbol = GetSymbol(nBlockCol, nBlockRow, nFieldCol, nFieldRow);
			if(pSymbol->bFixed)
			{
				if(pSymbol->cSolution	== cSymbol)
					return 1;
			}
			else
			{
				if(pSymbol->cSymbol		== cSymbol)
					return 1;
			}
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------
// CheckBlockAll
// ----------------------------------------------------------------------------
int CSudoku::CheckBlockAll(int nBlockCol, int nBlockRow)
{
	int nFound;
	size_t nSymbol;
	for(nSymbol=0; nSymbol<m_nSymbolsCount; nSymbol++)
		if((nFound = CheckBlock(nBlockCol, nBlockRow, m_pszSymbols[nSymbol])) != 1)
			return nFound;
	return 1;
}

// ----------------------------------------------------------------------------
// CheckColAll
// ----------------------------------------------------------------------------
int CSudoku::CheckColAll(int nBlockRow, int nFieldRow)
{
	int nFound;
	size_t nSymbol;
	for(nSymbol=0; nSymbol<m_nSymbolsCount; nSymbol++)
		if((nFound = CheckCol(nBlockRow, nFieldRow, m_pszSymbols[nSymbol])) != 1)
			return nFound;
	return 1;
}

// ----------------------------------------------------------------------------
// CheckRowAll
// ----------------------------------------------------------------------------
int CSudoku::CheckRowAll(int nBlockCol, int nFieldCol)
{
	int nFound;
	size_t nSymbol;
	for(nSymbol=0; nSymbol<m_nSymbolsCount; nSymbol++)
		if((nFound = CheckRow(nBlockCol, nFieldCol, m_pszSymbols[nSymbol])) != 1)
			return nFound;
	return 1;
}

// ----------------------------------------------------------------------------
// CheckBoard
// ----------------------------------------------------------------------------
int CSudoku::CheckBoard(void)
{
	int nCheck = CHECK_OK;

	int i;
	for(i=0; i<m_nFieldsCount; i++)
	{
		if(GetSymbol(i)->cSymbol == 0)
		{
			nCheck |= CHECK_INCOMPLETE;
			break;
		}
	}

	for(i=0; i<m_nFieldsCount; i++)
	{
		if(GetSymbol(i)->cSymbol != 0 && GetSymbol(i)->cSymbol != GetSymbol(i)->cSolution)
		{
			nCheck |= CHECK_ERRONEOUS;
			break;
		}
	}

	return nCheck;
}

// ----------------------------------------------------------------------------
// Solve
// ----------------------------------------------------------------------------
int CSudoku::Solve(bool bCountSolutions)
{
	m_nSolutionsCount = 0;

	if(bCountSolutions)
	{
		memcpy(m_TempBoard, m_Board, m_nFieldsCount*sizeof(tSymbol));
	}

	int nRet = Solve(0, bCountSolutions);
#ifdef _DEBUG_
//cout << "moep..." << endl;
cout << m_nCount1 << ", " << m_nCount2 << endl;
#endif

	if(bCountSolutions)
	{
		memcpy(m_Board, m_TempBoard, m_nFieldsCount*sizeof(tSymbol));
	}

	if(bCountSolutions)
		return m_nSolutionsCount;
	else
		return nRet;
}

// ----------------------------------------------------------------------------
// GetSolutionsCount
// ----------------------------------------------------------------------------
int CSudoku::GetSolutionsCount()
{
	return m_nSolutionsCount;
}

// ----------------------------------------------------------------------------
// Solve
// ----------------------------------------------------------------------------
int CSudoku::Solve(int nIndex, bool bCountSolutions)
{
	if(nIndex >= m_nFieldsCount)
		return 1;
#ifdef _DEBUG_
//	cout << nIndex << ", " << cc++ << "              \r";
#endif

	if(bCountSolutions && m_nSolutionsCount > 1)
		return 0;

	m_nCount1++;
	m_nCount2++;
	if(m_nCount1 > MAX_SOLVE_CALLS)
		return 0;

	// array index to coords
//	int nBlockIdx, nBlockRow, nBlockCol, nFieldIdx, nFieldRow, nFieldCol;

	// find next unsolved field
	tSymbol *pSymbol;
	while(1)
	{
//		nBlockIdx = nIndex / m_nBlockSize;
//		nBlockRow = nBlockIdx % m_nBlockColumns;
//		nBlockCol = nBlockIdx / 3;
//		nFieldIdx = nIndex - (nBlockIdx * m_nBlockSize);
//		nFieldRow = nFieldIdx % m_nFieldColumns;
//		nFieldCol = nFieldIdx / m_nFieldColumns;

//		pSymbol = GetSymbol(nBlockCol, nBlockRow, nFieldCol, nFieldRow);
//		pSymbol = GetSymbol(m_pnBlockColIndexLookup[nIndex], m_pnBlockRowIndexLookup[nIndex], m_pnFieldColIndexLookup[nIndex], m_pnFieldRowIndexLookup[nIndex]);
		pSymbol = GetSymbol(nIndex);

		if(pSymbol->cSolution != 0)
		{
			if(++nIndex >= m_nFieldsCount)
			{
#ifdef _DEBUG_
				cout << "++nIndex >= m_nFieldsCount" << endl;
#endif
				return 1;
			}
		}
		else
		{
			break;
		}
	}

	int nSolutions = 0;
	// try each symbol
	size_t i;
	for(i=0; i<m_nSymbolsCount; i++)
	{
		// check if this symbol keeps the rules in that field
//		if(	CheckCol(nBlockRow, nFieldRow, m_pszSymbols[i])	== 0	&&
//				CheckRow(nBlockCol, nFieldCol, m_pszSymbols[i])	== 0	&&
//				CheckBlock(nBlockCol, nBlockRow, m_pszSymbols[i])	== 0)
		if(	CheckCol(m_pnBlockRowIndexLookup[nIndex], m_pnFieldRowIndexLookup[nIndex], m_pszSymbols[i])	== 0	&&
				CheckRow(m_pnBlockColIndexLookup[nIndex], m_pnFieldColIndexLookup[nIndex], m_pszSymbols[i])	== 0	&&
				CheckBlock(m_pnBlockColIndexLookup[nIndex], m_pnBlockRowIndexLookup[nIndex], m_pszSymbols[i])	== 0)
		{
			// put symbol in this field
			pSymbol->cSolution	= m_pszSymbols[i];
			pSymbol->bFixed			= true;

			// check if the sudoku can be solved (recursive)
			if(Solve(nIndex+1, bCountSolutions) == 1)
			{
				nSolutions++;
				if(!bCountSolutions)
					return 1;
			}

			if(bCountSolutions && (m_nSolutionsCount > 1 || nSolutions > 1))
				return 0;
			// didn't solve, so try the next symbol
		}
	}

	m_nSolutionsCount += nSolutions;
	// none of the symbols were possible in this field so reset this symbol...
	// ...and return to the calling function and try the next symbol there
	pSymbol->cSolution	= 0;
	pSymbol->bFixed			= false;
	return 0;
}

// ----------------------------------------------------------------------------
// Create
// ----------------------------------------------------------------------------
void CSudoku::Create(int nLevel)
{
#ifdef _TEST_
	tSymbol *pSymbol;
	int br, bc, fr, fc, i = 0;
	for(br=0; br<m_nBlockRows; br++)
		for(fr=0; fr<m_nFieldRows; fr++)
			for(bc=0; bc<m_nBlockColumns; bc++)
				for(fc=0; fc<m_nFieldColumns; fc++)
				{
					pSymbol = GetSymbol(bc, br, fc, fr);
					pSymbol->cSymbol = testsudoku[i];
					pSymbol->cSolution = testsudoku[i];
					pSymbol->bFixed = (testsudoku[i] != 0);
					i++;
				}
#else
	memset(m_Board, 0, m_nFieldsCount*sizeof(tSymbol));
	for(int i=0; i<m_nFieldsCount; i++)
		GetSymbol(i)->bFixed = true;

	m_nCount2 = 0;
	Create();
//////	// put in the last field since the create algo leaves it empty
	// solve the unsolved rest ;)
	Solve(false);

	for(int i=0; i<m_nFieldsCount; i++)
		GetSymbol(i)->bFixed = false;

	tSymbol *pSymbol;
	int br, bc, nRandFieldRow, nRandFieldCol;
	for(br=0; br<m_nBlockRows; br++)
	{
		for(bc=0; bc<m_nBlockColumns; bc++)
		{
//			int nFields = m_nBlockSize - (int) (((float)nLevel/2.0+0.5) + (int)((float)(Rand() % nLevel) + 0.5));
			int nFields = m_nBlockSize - nLevel + (Rand() % 2);
			while(nFields > 0)
			{
				do
				{
					nRandFieldCol = Rand() % m_nFieldColumns;
					nRandFieldRow = Rand() % m_nFieldRows;
					pSymbol = GetSymbol(bc, br, nRandFieldCol, nRandFieldRow);
				}
				while(pSymbol->cSymbol != 0);

				pSymbol->cSymbol	= pSymbol->cSolution;
//				pSymbol->bFixed		= false;
				pSymbol->bFixed		= true;
				nFields--;
			}
		}
	}
#endif
}

// ----------------------------------------------------------------------------
// Create
// ----------------------------------------------------------------------------
int CSudoku::Create()
{
	// find an empty field
	int nBlockRow, nBlockCol, nFieldRow, nFieldCol;
	tSymbol *pSymbol;
	do
	{
		nBlockRow = Rand() % m_nBlockRows;
		nBlockCol = Rand() % m_nBlockColumns;
		nFieldRow = Rand() % m_nFieldRows;
		nFieldCol = Rand() % m_nFieldColumns;

		pSymbol = GetSymbol(nBlockCol, nBlockRow, nFieldCol, nFieldRow);
	}
	while(pSymbol->cSolution != 0);

	// loop through symbols
	size_t i;
	for(i=0; i<m_nSymbolsCount; i++)
	{
		// check if the symbol keeps the rules in that field
		if(	CheckCol(nBlockRow, nFieldRow, m_pszSymbols[i])	== 0	&&
				CheckRow(nBlockCol, nFieldCol, m_pszSymbols[i])	== 0	&&
				CheckBlock(nBlockCol, nBlockRow, m_pszSymbols[i])	== 0)
		{
			// put symbol in this field
			pSymbol->cSolution = m_pszSymbols[i];

			// count possible solutions (recursive)
			int nSolutions = Solve(true);
#ifdef _DEBUG_
_ShowSudoku(*this, true);
cout << endl << nSolutions << endl << endl;
#endif
			m_nCount1 = 0;

			if(nSolutions > 1)
			{
				CallbackCreate(m_nCount2);
				return Create();
			}
			else if(nSolutions == 1)
				return 1;
		}
	}

	pSymbol->cSolution = 0;
	return 0;
}

// ----------------------------------------------------------------------------
// LoadGame
// ----------------------------------------------------------------------------
int CSudoku::LoadGame(const char *pszFile)
{
	std::ifstream F;
	F.open(pszFile, std::ios_base::in | std::ios_base::binary);
	if(!F.good())
		return -1;

	// delete board and symbols
	Shutdown();

	// read savegame file version
	int nVersion;
	F.read((char *) &nVersion, sizeof(nVersion));

	// read symbols
	F.read((char *) &m_nSymbolsCount, sizeof(m_nSymbolsCount));
	m_pszSymbols = new char[m_nSymbolsCount+1];
	memset(m_pszSymbols, '\0', m_nSymbolsCount+1);
	F.read(m_pszSymbols, m_nSymbolsCount);

	// read board
	F.read((char *) &m_nBlockColumns,	sizeof(m_nBlockColumns));
	F.read((char *) &m_nBlockRows, 		sizeof(m_nBlockRows));
	F.read((char *) &m_nFieldColumns,	sizeof(m_nFieldColumns));
	F.read((char *) &m_nFieldRows, 		sizeof(m_nFieldRows));

	Init();
	F.read((char *) m_Board, m_nFieldsCount*sizeof(tSymbol));

	F.close();
	return 1;
}

// ----------------------------------------------------------------------------
// SaveGame
// ----------------------------------------------------------------------------
int CSudoku::SaveGame(const char *pszFile)
{
	std::ofstream F;
	F.open(pszFile, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!F.good())
		return -1;

	// write savegame file version
	int nVersion = GAMEFILE_VERSION;
	F.write((const char *) &nVersion,					sizeof(nVersion));

	// write symbols
	F.write((const char *) &m_nSymbolsCount,	sizeof(m_nSymbolsCount));
	F.write(m_pszSymbols, m_nSymbolsCount);

	// write board
	F.write((const char *) &m_nBlockColumns,	sizeof(m_nBlockColumns));
	F.write((const char *) &m_nBlockRows,			sizeof(m_nBlockRows));
	F.write((const char *) &m_nFieldColumns,	sizeof(m_nFieldColumns));
	F.write((const char *) &m_nFieldRows, 		sizeof(m_nFieldRows));

	F.write((const char *) m_Board, m_nFieldsCount*sizeof(tSymbol));

	F.close();
	return 1;
}
