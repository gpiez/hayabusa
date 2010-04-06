/*
 * transpositiontable.cpp
 *
 *  Created on: Dec 9, 2009
 *      Author: gpiez
 */

#include <cstdlib>
#ifdef HAVE_HUGE_PAGES
extern "C" {
#include <hugetlbfs.h>
}
#endif

#include "transpositiontable.h"
#include "constants.h"

KeyTable TranspositionTable::zobrist;

TranspositionTable::TranspositionTable() :
	size(0),
	usesHugePages(false),
	table(NULL)
{
	srand(1);
	for (unsigned p = -nPieces; p <= nPieces; p++)
		if (p) for (unsigned int i = 0; i < nSquares; ++i) {
			uint64_t r;
			do {
				r = (uint64_t) rand() ^ (uint64_t) rand() << 8 ^ (uint64_t) rand() << 16 ^ (uint64_t) rand() << 24 ^ (uint64_t) rand() << 32 ^ (uint64_t) rand() << 40 ^ (uint64_t) rand() << 48 ^ (uint64_t) rand() << 56;
			} while (popcount(r) >= 14 && popcount(r) <= 17);
			zobrist[p][i] = r;
		}

	setSize(0x200000);
}

TranspositionTable::~TranspositionTable()
{
}

void TranspositionTable::setSize(size_t s)
{
	if (table) {
#ifdef HAVE_HUGE_PAGES
		if (usesHugePages)
			free_huge_pages(table);
		else
#endif
		delete [] table;
		table = NULL;
	}

	s = qMin(s, (size_t) sysconf(_SC_PAGESIZE) * (size_t) sysconf(_SC_PHYS_PAGES));

	s |= s >> 1;
	s |= s >> 2;
	s |= s >> 4;
	s |= s >> 8;
	s |= s >> 16;
	s |= s >> 32;
	s++;
	s >>= 1;

	while (!table) {
		nEntries = s/sizeof(TTEntry);
#		ifdef HAVE_HUGE_PAGES
			table = (TTEntry *) get_huge_pages(s, GHP_DEFAULT);
			usesHugePages = true;
			if (table) break;
			qWarning() << "Could not allocate " << size << "bytes in huge pages";
#       endif
		table = new TTEntry[nEntries];
		usesHugePages = false;
		if (table) break;
		qWarning() << "Could not allocate " << size << "bytes";
		s >>= 1;
	}
	mask = (nEntries-assoc) * sizeof(TTEntry);

}

TTEntry* TranspositionTable::retrieve(Key k) const {
	TTEntry* subTable = &table[k & mask];
	Key upperKey = ((TTEntry*) &k)->upperKey;
	for (unsigned int i = 0; i < assoc; ++i)
		if (subTable[i].upperKey == upperKey)
			return &subTable[i];

	return 0;
}

//void TranspositionTable::store(Key k, TTEntry entry) {
//	TTEntry* subTable = &table[k & mask];
//	Key upperKey = ((TTEntry*) &k)->upperKey;
//	entry.upperKey |= upperKey;
//	unsigned int depth;
//	for (unsigned int i = 0; i < assoc; ++i)
//		if (subTable[i].data & TTEntry::mupperKey == upperKey)
//			if (subTable[i]->depth < entry->depth) {
//				// TODO replace
//			}
//
//
//}
