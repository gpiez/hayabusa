#ifndef TRANSPOSITIONTABLE_TCC_
#define TRANSPOSITIONTABLE_TCC_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "transpositiontable.h"

#ifdef HAVE_HUGE_PAGES
extern "C" {
#include <hugetlbfs.h>
}
#endif

template<typename Entry, unsigned int assoc>
TranspositionTable<Entry, assoc>::TranspositionTable() :
	size(0),
	usesHugePages(false),
	table(NULL)
{
	setSize(0x20000000);
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
const Entry* TranspositionTable<Entry, assoc>::retrieve(const Entry* subTable, Key k) const {
	Key upperKey = k >> Entry::upperShift; //((Entry*) &k)->upperKey;
	for (unsigned int i = 0; i < assoc; ++i)		//TODO compare all keys simultaniously suing sse
		if (subTable[i].upperKey == upperKey) {
			//TODO rotate to first position
			return &subTable[i];
		}
	return 0;
}

template<typename Entry, unsigned int assoc>
void TranspositionTable<Entry, assoc>::store(Entry* subTable, Entry entry) const {
	subTable[assoc-1] = entry;
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
	memset(table, 0, size);
	mask = nEntries-assoc;
}

#endif
