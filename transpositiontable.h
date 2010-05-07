/*
    hayabusa, chess engine
    Copyright (C) 2009-2010 Gunther Piez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef TRANSPOSITIONTABLE_H_
#define TRANSPOSITIONTABLE_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"
#include "move.h"
#include "ttentry.h"

template<Colors C> class ColoredBoard;
class RootBoard;

/* Global table with random 64 bit keys, for each piece of each color and for
 * each square.
 */
namespace Zobrist {
typedef Key KeyTable[nTotalPieces][nSquares];
extern KeyTable zobrist;  //12*64*8 = 6k
void initTables();
}

/* Hashtable for storing diagrams which are visited twice and for move ordering
 * The assoc parameter gives the number of different keys stored at the same
 * adress
 */
template<typename Entry, unsigned assoc>
class TranspositionTable {
	Entry* table;		// TODO remove indirection level
	uint64_t mask;
	QReadWriteLock lock;
	size_t size;
	size_t nEntries;
	bool usesHugePages;
//	TTEntry table;		// needs custom new operator

public:

	TranspositionTable();
	~TranspositionTable();

	void setSize(size_t s);
	Key nextKey(Key k, Move m);
	Entry* getEntry(Key k) const;
	bool retrieve(const Entry* subTable, Key k, Entry&);
	void store(Entry* subTable, Entry entry);
    void freeMemory();
	QString bestLine(const RootBoard& );
	template<Colors C> QString bestLineNext(const ColoredBoard<(Colors)-C>&, Move);
};


#endif /* TRANSPOSITIONTABLE_H_ */
