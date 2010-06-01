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

template<typename Entry, unsigned int assoc>
struct Sub {
		Entry entries[assoc];
};

/* Hashtable for storing diagrams which are visited twice and for move ordering
 * The assoc parameter gives the number of different keys stored at the same
 * adress
 */
template<typename Entry, unsigned assoc, typename Key>
class Table {
	typedef Sub<Entry, assoc> SubTable;
	SubTable* table;		// TODO remove indirection level
	uint64_t mask;
	QReadWriteLock lock[nTTLocks];
	size_t size;
	size_t nEntries;
	bool usesHugePages;
//	TTEntry table;		// needs custom new operator

public:

	Table(uint64_t size = 0x1000000);
	~Table();

	void setSize(size_t s);
	Key nextKey(Key k, Move m);
	SubTable* getSubTable( Key k, QReadWriteLock*& l) {
		l = lock + ((k>>4) & (nTTLocks-1));
		return &table[k & mask];
	}
	SubTable* getSubTable( Key k ) {
		return &table[k & mask];
	}
	void prefetchSubTable( Key k ) {
		_mm_prefetch(&table[k & mask], _MM_HINT_T0);
	}

	bool retrieve(const SubTable* subTable, Key k, Entry& ret, bool&) const ;
	bool retrieve(const SubTable* subTable, Key k, Entry& ret) const ;
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

template<typename Entry, unsigned assoc, typename Key>
class TranspositionTable: public Table<Entry, assoc, Key> {};

template<unsigned assoc, typename Key>
class TranspositionTable<PawnEntry, assoc, Key>: public Table<PawnEntry, assoc, Key> {
public:
	void store(Sub<PawnEntry, assoc>* subTable, PawnEntry entry);
	bool retrieve(Sub<PawnEntry, assoc>* subTable, Key k, PawnEntry& entry);
};

#endif /* TRANSPOSITIONTABLE_H_ */
