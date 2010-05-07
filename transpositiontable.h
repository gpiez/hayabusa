/*
 * transpositiontable.h
 *
 *  Created on: Dec 9, 2009
 *      Author: gpiez
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

namespace Zobrist {
typedef Key KeyTable[nTotalPieces][nSquares];
extern KeyTable zobrist;  //12*64*8 = 6k
void initTables();
}

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
