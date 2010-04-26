#ifndef TRANSPOSITIONTABLE_TCC_
#define TRANSPOSITIONTABLE_TCC_

#ifndef PCH_H_
#include <pch.h>
#endif

#include <transpositiontable.h>

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
	setSize(0x200000);
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
Entry* TranspositionTable<Entry, assoc>::retrieve(Key k) const {
	Entry* subTable = &table[k & mask];
	Key upperKey = ((Entry*) &k)->upperKey;
	for (unsigned int i = 0; i < assoc; ++i)
		if (subTable[i].upperKey == upperKey)
			return &subTable[i];

	return 0;
}

template<typename Entry, unsigned int assoc>
void TranspositionTable<Entry, assoc>::store(Key k, Entry entry) {
	Entry* subTable = &table[k & mask];
	Key upperKey = ((Entry*) &k)->upperKey;
	entry.upperKey |= upperKey;
	unsigned int depth;
	for (unsigned int i = 0; i < assoc; ++i)
		if (subTable[i].data & Entry::mupperKey == upperKey)
			if (subTable[i]->depth < entry->depth) {
				// TODO replace
			}
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

	while (!table) {
		nEntries = s/sizeof(Entry);
#		ifdef HAVE_HUGE_PAGES
			table = (Entry *) get_huge_pages(s, GHP_DEFAULT);
			usesHugePages = true;
			if (table) break;
			qWarning() << "Could not allocate " << size << "bytes in huge pages";
#       endif
		table = new Entry[nEntries];
		usesHugePages = false;
		if (table) break;
		qWarning() << "Could not allocate " << size << "bytes";
		s >>= 1;
	}
	mask = (nEntries-assoc) * sizeof(Entry);
}

#endif
