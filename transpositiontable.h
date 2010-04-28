/*
 * transpositiontable.h
 *
 *  Created on: Dec 9, 2009
 *      Author: gpiez
 */

#ifndef TRANSPOSITIONTABLE_H_
#define TRANSPOSITIONTABLE_H_

#include "constants.h"
#include "move.h"
#include "ttentry.h"

namespace Zobrist {
typedef Key KeyTable[nTotalPieces][nSquares];
extern KeyTable zobrist;  //12*64*8 = 6k
void initTables();
}

template<class Entry, unsigned assoc>
class TranspositionTable {
	Entry* table;		// TODO remove indirection level
	uint64_t mask;
	QReadWriteLock tt;
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
	const Entry* retrieve(const Entry* subTable, Key k);
	void store(Entry* subTable, Entry entry);
    void freeMemory();
};


#endif /* TRANSPOSITIONTABLE_H_ */
