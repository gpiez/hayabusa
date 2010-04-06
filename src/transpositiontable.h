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

typedef Key KeyTable[nTotalPieces][nSquares];

class TranspositionTable {
	static const unsigned int assoc = 2;
	size_t size;
	bool usesHugePages;
	TTEntry* table;
	size_t nEntries;
	uint64_t mask;

public:
	static KeyTable zobrist;  //12*64*8 = 6k

	TranspositionTable();
	~TranspositionTable();

	void setSize(size_t s);
	Key nextKey(Key k, Move m);
	TTEntry* retrieve(Key) const;
	void store(Key k, TTEntry entry);
};

#endif /* TRANSPOSITIONTABLE_H_ */
