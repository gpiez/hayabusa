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

/* Hashtable for storing diagrams which are visited twice and for move ordering
 * The assoc parameter gives the number of different keys stored at the same
 * adress
 */
template<typename Entry, unsigned assoc>
class TranspositionTable {
	struct SubTable {
		Entry entries[assoc];
	};
	SubTable* table;		// TODO remove indirection level
	uint64_t mask;
	QReadWriteLock lock[nTTLocks];
	size_t size;
	size_t nEntries;
	bool usesHugePages;
//	TTEntry table;		// needs custom new operator

public:

	TranspositionTable();
	~TranspositionTable();

	void setSize(size_t s);
	Key nextKey(Key k, Move m);
	SubTable* getSubTable( Key k, QReadWriteLock*& l) {
		l = lock + ((k>>4) & (nTTLocks-1));
		return &table[k & mask];
	}
	SubTable* getSubTable( Key k ) {
		return &table[k & mask];
	}

	bool retrieve(SubTable* subTable, Key k, Entry& ret, QReadWriteLock* );
	bool retrieve(const SubTable* subTable, Key k, Entry& ret, bool&) const ;
	void store(SubTable* subTable, Entry entry);
	void unmark(SubTable* subTable) {
		subTable->entries[0].visited.reset();
	}
	void mark(SubTable* subTable) {
		subTable->entries[0].visited |= true;
	}
	void freeMemory();
	QString bestLine(const RootBoard& );
	template<Colors C> QString bestLineNext(const ColoredBoard<(Colors)-C>&, Move, const RootBoard&, QSet<Key>&);
};


#endif /* TRANSPOSITIONTABLE_H_ */
