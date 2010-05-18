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
#ifndef PIECELIST_H_
#define PIECELIST_H_

#include <stdint.h>
#include <tmmintrin.h>

#include "constants.h"

/**
 * Contains a list of positions and counts of pieces for a color.
 * Because one can have at most 16 pieces, all pieces a stored in
 * adjacent uint8_t for fast vector manipulation. A list of indices
 * for pos[] is maintained, which contains the starting index for each
 * kind of piece.
 * The order of indices is:
 * King, Pawn, Rook, Bishop, Queen, Knight
 * to make sure the king is always at pos[0] and the pawns start at pos[1].
 * If a new piece is added, the entrys in pos[] right of the index of the new piece
 * are being shifted to the right and all index[] for the pieces right need to be
 * incremented. This is done with a single addition of a packed 8-byte.
 */
class PieceList {

	union {
		uint8_t pos[16];
		__m128i pos_v ALIGN_XMM;
	};
	union {
		uint8_t index[nPieces+1];
		uint64_t index_v;
	};
	union {
		uint8_t count[nPieces+1];
		uint64_t count_v;
	};

	/* Vector tables for updating all index[] at once */
	static uint64_t indexDiffs[nPieces+1];

	/* Vector tables for shifting elements right of pos[i] to the left or right using pshufb */
	static const __v16qi addShifts[16] ALIGN_XMM;

	// for masking out different kind of pieces from a consecutive block of eight positions
	static const uint64_t posMask[9];

public:
	// instead of a constructor, to keep it POD
	void init() {
		for (unsigned int i = 0; i < 16; ++i)
			pos[i] = 0;
		index_v = 0;
		count_v = 0;
	}

	uint8_t get(int piece, unsigned int i) const {
		ASSERT(piece >= Rook && piece <= King);
		return pos[index[piece] + i];
	}

	template<int piece>
	uint64_t getAll() const {
		static_assert(piece >= Rook && piece <= King);
		return *(uint64_t*)&pos[index[piece]] & posMask[count[piece]];
	}

	uint64_t getAll(int piece) const {
		ASSERT(piece >= Rook && piece <= King);
		return *(uint64_t*)&pos[index[piece]] & posMask[count[piece]];
	}

	uint8_t getKing() const {
		return pos[0];
	}

	uint8_t getPawn(unsigned int i) const {
		return pos[1 + i];
	}

	uint8_t operator [] (int piece) const {
		ASSERT(piece >= Rook && piece <= King);
		return count[piece];
	}

	uint64_t getCounts() const {
		return count_v;
	}

	template<int piece>	//TODO specialize for faster execution
	uint64_t bitBoard() const {
		uint64_t bb = 0;
		uint64_t piecePos = getAll<piece>();
		for (unsigned int i = count[piece]; i>0; --i, piecePos >>= 8)
			bb |= 1ULL << (uint8_t)piecePos;	//TODO check if (uint8_t) can be omitted
		return bb;
	}
	/*
	 * Move a piece. In the piece list this basically comes down to change
	 * an entry in pos[] with value 'to' to the value 'from'.
	 */
	void move( unsigned int from, unsigned int to) {
		ASSERT(from < nSquares);
		ASSERT(to < nSquares);
		__m128i mask = broadcastTab[from];
		mask = _mm_cmpeq_epi8(mask, pos_v);
		__m128i diff = broadcastTab[to ^ from];
		mask = _mm_and_si128(mask, diff);
		pos_v = _mm_xor_si128(pos_v, mask);
	}

	/*
	 * Add a piece. All elements right to the new piece have to
	 * be shifted to the right, all indices with higher order than piece
	 * have to be incremented.
	 */
	void add(int piece, unsigned int pos) {
		ASSERT(piece >= Rook && piece <= King);
		ASSERT(pos < nSquares);
		pos_v = _mm_shuffle_epi8(pos_v, (__m128i)addShifts[index[piece]]);
		this->pos[index[piece]] = pos;
		++count[piece];
		index_v += indexDiffs[piece];
	}

	/*
	 * Subtract a piece. The position has to be found in all pos[], from the resulting mask
	 * a shift vector table is constructed
	 * All elements right to the removed piece have to
	 * be shifted to the left, all indices with higher order than piece
	 * have to be decremented.
	 */
	void sub( int piece, unsigned int pos ) {
		ASSERT(piece >= Rook && piece <= King);
		ASSERT(pos < nSquares);
		__m128i mask = broadcastTab[pos];
		__m128i npos = pos_v;
		mask = _mm_cmpeq_epi8(mask, npos);
		mask |= _mm_slli_si128(mask, 1);
		mask |= _mm_slli_si128(mask, 2);
		mask |= _mm_slli_si128(mask, 4);
		mask |= _mm_slli_si128(mask, 8);
		mask = _mm_sub_epi8((__m128i)(__v16qi){ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 }, mask);
		pos_v = _mm_shuffle_epi8(npos, mask);
		--count[piece];							// TODO this dec needs 10-20 clocks, why
		index_v -= indexDiffs[piece];
	}
};

#endif /* PIECELIST_H_ */
