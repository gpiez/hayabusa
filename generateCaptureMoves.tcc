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
#ifndef GENERATECAPTUREMOVES_TCC_
#define GENERATECAPTUREMOVES_TCC_

#include "coloredboard.h"

template<Colors C>
template<bool UPromo>
void ColoredBoard<C>::generateTargetCapture(Move* &good, Move* &bad, uint64_t d, const unsigned int cap) const {
	static const int val[nPieces+1] = { 0, 5, 3, 9, 3, 1, 100 };
    /*
     * King captures something.
     */
    for ( uint64_t p = getAttacks<C,King>() & d & ~getAttacks<-C,All>(); p; p &= p-1 ) {
        unsigned from = bit(getPieces<C,King>());
        *--good = Move(from, bit(p), King, cap);
    }
    /* Check for attacks from sliding pieces. Bishop/Rook/Queen attacks are
     * stored in single[], with a mov template matching the attack in move[]
     */
    const MoveTemplate* psingle = single[CI];
    const __v2di zero = _mm_set1_epi64x(0);
    const __v2di d2 = _mm_set1_epi64x(d);
    for(;;) {
        Move m = psingle->move;
        if (!m.data) break;
        __v2di a02 = psingle->d02;
        __v2di a13 = psingle->d13;
        psingle++;
#ifdef __SSE4_1__
        if (!_mm_testz_si128(d2, (a02|a13))) {
#else
        if (fold(d2 & (a02|a13))) {
#endif
            __v2di from2 = _mm_set1_epi64x(1ULL<<m.from());
            __v2di pin02 = from2 & dpins[CI].d02;
            __v2di pin13 = from2 & dpins[CI].d13;
#ifdef __SSE4_1__            
            pin02 = _mm_cmpeq_epi64(pin02, zero);
            pin13 = _mm_cmpeq_epi64(pin13, zero);
#else            
            pin02 = _mm_cmpeq_epi32(pin02, zero);
            pin13 = _mm_cmpeq_epi32(pin13, zero);
            __v2di pin02s = _mm_shuffle_epi32(pin02, 0b10110001);
            __v2di pin13s = _mm_shuffle_epi32(pin13, 0b10110001);
            pin02 = pin02 & pin02s;
            pin13 = pin13 & pin13s;
#endif            
            pin02 = ~pin02 & d2 & a02;
            pin13 = ~pin13 & d2 & a13;
            for (uint64_t a=fold(pin02|pin13); a; a&=a-1) {
            	Move n;
            	n.data = m.data + Move(0, bit(a), 0, cap).data;
            	if ( val[cap] < val[m.piece()] &&
            		 a & -a & (                        getAttacks<-C,Pawn>() |
                			   ((m.piece() & 1)     ? (getAttacks<-C,Knight>()|getAttacks<-C,Bishop>()) : 0) |
                			   ((m.piece() == Queen)?  getAttacks<-C,Rook>() : 0)
                			  )
                   ) *bad++ = n;
            	else *--good = n;
            }
        }
    }
    // Knight captures something. Can't move at all if pinned.
    for ( uint64_t p = getAttacks<C,Knight>() & d; p; p &= p-1 ) {
        unsigned to = bit(p);
        for ( uint64_t f = getPieces<C,Knight>() & pins[CI] & knightAttacks[to]; f; f &= f-1)
            if (cap == Pawn && p & -p & getAttacks<-C,Pawn>())
            	*bad++ = Move(bit(f), to, Knight, cap);
            else *--good = Move(bit(f), to, Knight, cap);
    }
	// pawn capture to the absolute right, dir = 1 for white and 3 for black
	for(uint64_t p = getPieces<C,Pawn>() & ~file<'h'>() & shift<-C*8-1>(d) & getPins<C,2-C>();p ;p &= p-1) {
		unsigned from = bit(p);
		unsigned to = from + C*8+1;
		if (rank<7>() & (p&-p)) {
			if (UPromo) {
				*--good = Move(from, to, Knight, cap, true);
				*--good = Move(from, to, Rook, cap, true);
				*--good = Move(from, to, Bishop, cap, true);
			}
			*--good = Move(from, to, Queen, cap, true);
		} else
			*--good = Move(from, to, Pawn, cap);
	}
	// left
	for (uint64_t p = getPieces<C,Pawn>() & ~file<'a'>() & shift<-C*8+1>(d) & getPins<C,2+C>();p ;p &= p-1) {
		unsigned from = bit(p);
		unsigned to = from + C*8-1;
		if (rank<7>() & (p&-p)) {
			if (UPromo) {
				*--good = Move(from, to, Knight, cap, true);
				*--good = Move(from, to, Rook, cap, true);
				*--good = Move(from, to, Bishop, cap, true);
			}
			*--good = Move(from, to, Queen, cap, true);
		} else
			*--good = Move(from, to, Pawn, cap);
	}
}
/*
 * Generate all pawn to queen promotions and capture moves for each kind of piece.
 * A pinned piece may only move on the line between king and pinning piece.
 * For a pawn on the last rank this can only happen if the pawn captures the pinning piece.
 */
template<Colors C>
template<bool UPromo>
void ColoredBoard<C>::generateCaptureMoves( Move* &good, Move* &bad) const {
    /*
     * Generate non-capturing queen promotions. Capturing promotions are handled in
     * generateTargetCaptures().
     */
    if (uint64_t p = getPieces<-C,Pawn>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(good, bad, p, Pawn);

    if (uint64_t p = getPieces<-C,Knight>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(good, bad, p, Knight);

    if (uint64_t p = getPieces<-C,Bishop>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(good, bad, p, Bishop);

    if (uint64_t p = getPieces<-C,Rook>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(good, bad, p, Rook);

    if (uint64_t p = getPieces<-C,Queen>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(good, bad, p, Queen);

    uint8_t to, sq;
    for (uint64_t p = getPieces<C,Pawn>() & rank<7>() & shift<-C*8>(~occupied1) & pins[CI]; p; p &= p-1) {
        sq = bit(p);
        to = sq + C*dirOffsets[2];
        if (UPromo) {
            *--good = Move(sq, to, Knight, 0, true);
            *--good = Move(sq, to, Rook, 0, true);
            *--good = Move(sq, to, Bishop, 0, true);
        }
        *--good = Move(sq, to, Queen, 0, true);
    }
}
#endif /* GENERATECAPTUREMOVES_TCC_ */
