/*
 * transpositiontable.cpp
 *
 *  Created on: Dec 9, 2009
 *      Author: gpiez
 */

#include <cstdlib>
#include "transpositiontable.h"
#include "constants.h"

namespace Zobrist {

KeyTable zobrist;

void initTables() {
	srand(1);
	for (int p = -nPieces; p <= (signed int)nPieces; p++)
		for (unsigned int i = 0; i < nSquares; ++i)
			if (p) {
				uint64_t r;
				do {
					r = (uint64_t) rand()
					^ (uint64_t) rand() << 8
					^ (uint64_t) rand() << 16
					^ (uint64_t) rand() << 24
					^ (uint64_t) rand() << 32
					^ (uint64_t) rand() << 40
					^ (uint64_t) rand() << 48
					^ (uint64_t) rand() << 56;
				} while (popcount(r) >= 29 && popcount(r) <= 36);
				zobrist[p+nPieces][i] = r;
			} else {
				zobrist[p+nPieces][i] = 0;				
			}
		}
}
