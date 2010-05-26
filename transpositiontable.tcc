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
#ifndef TRANSPOSITIONTABLE_TCC_
#define TRANSPOSITIONTABLE_TCC_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "transpositiontable.h"
#include "rootboard.h"
#include "coloredboard.tcc"

#ifdef HAVE_HUGE_PAGES
extern "C" {
#include <hugetlbfs.h>
}
#endif

template<typename Entry, unsigned int assoc, typename Key>
Table<Entry, assoc, Key>::Table(uint64_t size) :
	table(NULL),
	size(0),
	usesHugePages(false)
{
	setSize(size);
}

template<typename Entry, unsigned int assoc, typename Key>
Table<Entry, assoc, Key>::~Table() {
	freeMemory();
}

template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::freeMemory() {
	if (table) {
#ifdef HAVE_HUGE_PAGES
		if (usesHugePages)
			free_huge_pages(table);
		else
#endif
		delete [] table;
		table = NULL;
	}
}

// this is only called from tree
template<typename Entry, unsigned int assoc, typename Key>
bool Table<Entry, assoc, Key>::retrieve(const SubTable* subTable, Key k, Entry &ret, bool &visited) const {
	visited = subTable->entries[0].visited;
	Key upperKey = k >> Entry::upperShift; //((Entry*) &k)->upperKey;
	for (unsigned int i = 0; i < assoc; ++i) {		//TODO compare all keys simultaniously suing sse
		if (subTable->entries[i].upperKey == upperKey) {		//only lock if a possible match is found
			// found same entry again, move repetition
			if (visited) {
				ret.zero();
				ret.loBound |= true;
				ret.hiBound |= true;
				ret.depth |= maxDepth-1;
			} else {
				ret = subTable->entries[i];
			}
			return true;
		}
	}
	return false;
}

template<typename Entry, unsigned int assoc, typename Key>
bool Table<Entry, assoc, Key>::retrieve(const SubTable* subTable, Key k, Entry& ret ) const {
	Key upperKey = k >> Entry::upperShift; //((Entry*) &k)->upperKey;
	for (unsigned int i = 0; i < assoc; ++i) {		//TODO compare all keys simultaniously suing sse
		if (subTable->entries[i].upperKey == upperKey) {		//only lock if a possible match is found
				//TODO rotate to first position
			ret = subTable->entries[i];
			return true;
		}
	}
	return false;
}

#pragma GCC diagnostic ignored "-Wtype-limits"
template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::store(SubTable* subTable, Entry entry) {
	stats.ttstore++;
	// look if the position is already stored. if it is, but the new depth
	// isn't sufficient, don't write anything.
	for (unsigned int i = 0; i < assoc; ++i) 		//TODO compare all keys simultaniously suing sse
		if (subTable->entries[i].upperKey == entry.upperKey) {
			if (entry.depth >= subTable->entries[i].depth) {
				stats.ttoverwrite++;
				stats.ttinsufficient--;
				subTable->entries[i] = entry;
			}
			stats.ttinsufficient++;
			return;
		}

	if (subTable->entries[assoc-1].data == 0)
		stats.ttuse++;
	unsigned int i;
	for (i = 0; i < assoc-1; ++i)				// TODO possibly checking only assoc/2 and a LRU in retrieve would be better
		if (entry.depth >= subTable->entries[i].depth)
			break;

	for (unsigned j = assoc-1; j>i; --j) {
		subTable->entries[j] = subTable->entries[j-1];
	}
	subTable->entries[i] = entry;
}

template<unsigned int assoc, typename Key>
void TranspositionTable<PawnEntry, assoc, Key>::store(Sub<PawnEntry, assoc>* subTable, PawnEntry entry) {
	stats.ttstore++;
	if (subTable->entries[assoc-1].upperKey == 0)
		stats.ttuse++;
	subTable->entries[assoc-1] = entry;
}

template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::setSize(size_t s)
{
	freeMemory();
	
	s = qMin(s, (size_t) sysconf(_SC_PAGESIZE) * (size_t) sysconf(_SC_PHYS_PAGES));

	s |= s >> 1;
	s |= s >> 2;
	s |= s >> 4;
	s |= s >> 8;
	s |= s >> 16;
	s |= s >> 32;
	s++;
	s >>= 1;
	size = s;
	
	while (!table) {
		nEntries = size/sizeof(SubTable);
#		ifdef HAVE_HUGE_PAGES
			table = (SubTable *) get_huge_pages(s, GHP_DEFAULT);
			usesHugePages = true;
			if (table) break;
			qWarning() << "Could not allocate" << size << "bytes in huge pages";
#       endif
		table = new SubTable[nEntries];
		usesHugePages = false;
		if (table) break;
		qWarning() << "Could not allocate" << size << "bytes";
		size >>= 1;
	}
	memset(table, 0, size);	// not strictly neccessary, but allocating pages
	mask = nEntries-1;
}

template<typename Entry, unsigned assoc, typename Key>
template<Colors C>
QString Table<Entry, assoc, Key>::bestLineNext(const ColoredBoard<(Colors)-C>& prev, Move m, const RootBoard& rb, QSet<Key>& visited) {
	QString line = m.string();
	const ColoredBoard<C> b(prev, m, rb);
	Key key = b.getZobrist();
	if (visited.contains(key)) return "";
	visited << key;
	SubTable* te = getSubTable(key);
	TTEntry subentry;

	Move ttMove = {{0}};
	if (retrieve(te, key, subentry) ) {
		ttMove.from = subentry.from;
		ttMove.to = subentry.to;
	}

	Move list[256];
	Move* const end = b.generateMoves(list);
	if ((uint32_t&)ttMove)
		for (Move *i=list; i<end; ++i) {
			if (i->from == ttMove.from && i->to == ttMove.to) {
				line += " " + bestLineNext<(Colors)-C>(b, *i, rb, visited);
				break;
			}
		}

	return line;
}

template<typename Entry, unsigned assoc, typename Key>
QString Table<Entry, assoc, Key>::bestLine(const RootBoard& b) {
	if (!(uint32_t&)b.bestMove) return QString();
	QSet<Key> visited;
	if (b.color == White) {
		return bestLineNext<Black>(b.currentBoard<White>(), b.bestMove, b, visited);
	} else {
		return bestLineNext<White>(b.currentBoard<Black>(), b.bestMove, b, visited);
	}

}

#endif
