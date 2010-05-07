#ifndef STATS_H_
#define STATS_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include <stdint.h>

extern struct Stats {
	uint64_t	node;
	uint64_t	internalNode;
	uint64_t	eval;
	uint64_t	ttuse;
	uint64_t	ttfree;
	uint64_t	tthit;
} stats;

#endif
