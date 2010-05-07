#ifndef TRANSPOSITIONTABLE_TCC_
#define TRANSPOSITIONTABLE_TCC_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "transpositiontable.h"
#include "rootboard.h"
#include "stats.h"
#include "coloredboard.tcc"

#ifdef HAVE_HUGE_PAGES
extern "C" {
#include <hugetlbfs.h>
}
#endif

template<typename Entry, unsigned int assoc>
TranspositionTable<Entry, assoc>::TranspositionTable() :
	table(NULL),
	size(0),
	usesHugePages(false)
{
	setSize(0x10000000);
}

template<typename Entry, unsigned int assoc>
TranspositionTable<Entry, assoc>::~TranspositionTable() {
	freeMemory();
}

template<typename Entry, unsigned int assoc>
void TranspositionTable<Entry, assoc>::freeMemory() {
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

template<typename Entry, unsigned int assoc>
Entry* TranspositionTable<Entry, assoc>::getEntry(Key k) const {
	return &table[k & mask];
}

template<typename Entry, unsigned int assoc>
bool TranspositionTable<Entry, assoc>::retrieve(const Entry* subTable, Key k, Entry& ret) {
	union EntryUnion {
		Entry temp;
		volatile __m128i xmm;
	} atomic_entry ALIGN_XMM;
	Key upperKey = k >> Entry::upperShift; //((Entry*) &k)->upperKey;
	for (unsigned int i = 0; i < assoc; ++i) {		//TODO compare all keys simultaniously suing sse
		atomic_entry.xmm = ((EntryUnion) { subTable[i] }).xmm;
		if (atomic_entry.temp.upperKey == upperKey) {		//only lock if a possible match is found
//			QReadLocker locker(&tt);
				//TODO rotate to first position
			ret = atomic_entry.temp;
			return true;
			
		}
	}
	return false;
}

template<typename Entry, unsigned int assoc>
void TranspositionTable<Entry, assoc>::store(Entry* subTable, Entry entry) {
	QWriteLocker l(&lock);

	// look if the position is already stored. if it is, but the new depth
	// isn't sufficient, don't write anything.
	for (unsigned int i = 0; i < assoc; ++i) 		//TODO compare all keys simultaniously suing sse
		if (subTable[i].upperKey == entry.upperKey) {
			if (entry.depth >= subTable[i].depth)
				subTable[i] = entry;
			return;
		}

	if (subTable[assoc-1].data == 0)
		stats.ttuse++;
	unsigned int i;
	for (i = 0; i < assoc-1; ++i)				// TODO possibly checking only assoc/2 and a LRU in retrieve would be better
		if (entry.depth > subTable[i].depth)
			break;

	for (unsigned j = assoc-1; j>i; --j) {
		subTable[j] = subTable[j-1];
	}
	subTable[i] = entry;
}

template<typename Entry, unsigned int assoc>
void TranspositionTable<Entry, assoc>::setSize(size_t s)
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
		nEntries = s/sizeof(Entry);
#		ifdef HAVE_HUGE_PAGES
			table = (Entry *) get_huge_pages(s, GHP_DEFAULT);
			usesHugePages = true;
			if (table) break;
			qWarning() << "Could not allocate" << size << "bytes in huge pages";
#       endif
		table = new Entry[nEntries];
		usesHugePages = false;
		if (table) break;
		qWarning() << "Could not allocate" << size << "bytes";
		s >>= 1;
	}
	//memset(table, 0, size);
	mask = nEntries-assoc;
}

template<typename Entry, unsigned assoc>
template<Colors C>
QString TranspositionTable<Entry, assoc>::bestLineNext(const ColoredBoard<(Colors)-C>& prev, Move m) {
	QString line = m.string();
	const ColoredBoard<C> b(prev, m);
	Key key = b.getZobrist();
	TTEntry* te = getEntry(key);
	TTEntry subentry;

	Move ttMove = {0};
	if (retrieve(te, key, subentry) ) {
		ttMove.from = subentry.from;
		ttMove.to = subentry.to;
	}

	Move list[256];
	Move* const end = b.generateMoves(list);
	if ((uint32_t&)ttMove)
		for (Move *i=list; i<end; ++i) {
			if (i->from == ttMove.from && i->to == ttMove.to) {
				line += " " + bestLineNext<(Colors)-C>(b, *i);
				break;
			}
		}

	return line;
}

template<typename Entry, unsigned assoc>
QString TranspositionTable<Entry, assoc>::bestLine(const RootBoard& b) {
	if (!(uint32_t&)b.bestMove) return QString();
	if (b.color == White) {
		return bestLineNext<Black>(b.currentBoard<White>(), b.bestMove);
	} else {
		return bestLineNext<White>(b.currentBoard<Black>(), b.bestMove);
	}

}

#endif
