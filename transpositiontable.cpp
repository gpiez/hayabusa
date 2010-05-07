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
#include <pch.h>

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

