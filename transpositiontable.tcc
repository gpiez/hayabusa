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
	Key upperKey = k >> Entry::upperShift;
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
			ret = subTable->entries[i];
			return true;
		}
	}
	return false;
}

template<unsigned int assoc, typename Key>
bool TranspositionTable<PawnEntry, assoc, Key>::retrieve(Sub<PawnEntry, assoc>* subTable, Key k, PawnEntry& ret ) {
	Key upperKey = k >> PawnEntry::upperShift; //((Entry*) &k)->upperKey;
	unsigned int i=0;
	PawnEntry first = subTable->entries[i];
	PawnEntry second;
	bool possibleUse = subTable->entries[assoc-1].upperKey == 0;
	do {		//TODO compare all keys simultaniously suing sse
		if (first.upperKey == upperKey) {
			subTable->entries[0] = first;
			ret = first;
			return true;
		}
		++i;
		if (i == assoc) break;
		second = subTable->entries[i];
		subTable->entries[i] = first;
		first = second;
	} while (true);

	stats.ptuse += possibleUse;
	return false;
}
//#pragma GCC diagnostic push
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
//#pragma GCC diagnostic pop

template<unsigned int assoc, typename Key>
void TranspositionTable<PawnEntry, assoc, Key>::store(Sub<PawnEntry, assoc>* subTable, PawnEntry entry) {
	subTable->entries[0] = entry;
}

template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::setSize(size_t s)
{
	freeMemory();

	s = qMin(s, (size_t) sysconf(_SC_PAGESIZE) * (size_t) sysconf(_SC_PHYS_PAGES));

	nEntries = s/sizeof(SubTable);

	nEntries |= nEntries >> 1;
	nEntries |= nEntries >> 2;
	nEntries |= nEntries >> 4;
	nEntries |= nEntries >> 8;
	nEntries |= nEntries >> 16;
	nEntries |= nEntries >> 32;
	nEntries++;
	nEntries >>= 1;
	size = nEntries*sizeof(SubTable);

	while (!table) {
#ifdef HAVE_HUGE_PAGES
			table = (SubTable *) get_huge_pages(s, GHP_DEFAULT);
			usesHugePages = true;
			if (table) break;
			std::cerr << "Could not allocate" << size << "bytes in huge pages" << std::endl;
#endif
		table = new SubTable[nEntries];
		usesHugePages = false;
		if (table) break;
		std::cerr << "Could not allocate" << size << "bytes" << std::endl;
		size >>= 1;
		nEntries >>= 1;
	}
	memset(table, 0, size);	// not strictly neccessary, but allocating pages
	mask = nEntries-1;
}

template<typename Entry, unsigned assoc, typename Key>
template<Colors C>
std::string Table<Entry, assoc, Key>::bestLineNext(const ColoredBoard<(Colors)-C>& prev, Move m, QSet<Key>& visited, const RootBoard &rb) {
	std::string line = m.string();
	__v8hi est = prev.estimatedEval(m, rb.eval);
	const ColoredBoard<C> b(prev, m, est);
	Key key = b.getZobrist();
	if (visited.contains(key)) return line;
	visited << key;
	SubTable* te = getSubTable(key);
	TTEntry subentry;

	Move ttMove(0,0,0);
	if (retrieve(te, key, subentry) ) {
		ttMove = Move(subentry.from, subentry.to, 0);
	}

	Move moveList[256];
	Move* good=moveList+192;
	Move* bad=good;
    if (b. template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.generateMoves(good);
        b.template generateCaptureMoves<true>(good, bad);
    }
	if (ttMove.data)
		for (Move *i=good; i<bad; ++i) {
#ifdef BITBOARD
			if (i->from() == ttMove.from() && i->to() == ttMove.to()) {
#else
            if (i->from == ttMove.from && i->to == ttMove.to) {
#endif
				line += " " + bestLineNext<(Colors)-C>(b, *i, visited, rb);
				break;
			}
		}

	return line;
}

template<typename Entry, unsigned assoc, typename Key>
std::string Table<Entry, assoc, Key>::bestLine(const RootBoard& b) {
	if (!b.bestMove.data) return "";
	QSet<Key> visited;
	if (b.color == White) {
		return bestLineNext<Black>(b.currentBoard<White>(), b.bestMove, visited, b);
	} else {
		return bestLineNext<White>(b.currentBoard<Black>(), b.bestMove, visited, b);
	}

}

namespace {
inline int tt2Score(int s) {
    if (s < 0x400 && s > -0x400)
        return s;
    if (s > 0) {
        if (s < 0x600)
            return s*2 - 0x400*1;
        if (s < 0x700)
            return s*4 - 0x600*3;
        return s*16 - 0x700*15;
    } else {
        if (s > -0x600)
            return s*2 + 0x400*1;
        if (s > -0x700)
            return s*4 + 0x600*3;
        return s*16 + 0x700*15;
    }
}

inline int score2tt(int s) {
    if (s < 0x400 && s > -0x400)
        return s;
    if (s > 0) {
        if (s<0x800)
            return (s-0x400)/2 + 0x400;
        if (s<0x1000)
            return (s-0x800)/4 + 0x600;
        return (s-0x1000)/16 + 0x700;
    } else {
        if (s>-0x800)
            return (s+0x400)/2 - 0x400;
        if (s>-0x1000)
            return (s+0x800)/4 - 0x600;
        return (s+0x1000)/16 - 0x700;
    }
}
}
#endif
