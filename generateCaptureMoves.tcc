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

template<Colors C>
template<bool AbortOnFirst>
bool ColoredBoard<C>::generateMateMoves( Move** const good ) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    uint64_t king = getPieces<-C,King>();
    unsigned k = bit(king);

//    uint64_t o = getPieces<C,Rook>() | getPieces<C,Bishop>() | getPieces<C,Queen>() | getPieces<C,Knight>() | getPieces<C,Pawn>() | getPieces<C,King>();
	uint64_t qblock = getPieces<-C,Rook>() | getPieces<-C,Bishop>() | getPieces<-C,Knight>() | getPieces<-C,Pawn>() | getPieces<-C,Queen>()
					| getAttacks<C,Rook>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>();
	uint64_t qescape = getAttacks<-C,King>() & ~qblock;
	qescape = __rorq(qescape, k);
	uint64_t qmate = 0;
	if ((qescape & 0x8300000000000080) == 0)
		qmate |= king << 9 & ~file<'a'>();
	if ((qescape & 0x180000000000202) == 0)
		qmate |= king << 7 & ~file<'h'>();
	if ((qescape & 0x200000000000182) == 0)
		qmate |= king >> 9 & ~file<'h'>();
	if ((qescape & 0x8080000000000300) == 0)
		qmate |= king >> 7 & ~file<'a'>();
	if ((qescape & 0x80000000000080) == 0)
		qmate |= king << 1 & ~file<'a'>();
	if ((qescape & 0x280000000000000) == 0)
		qmate |= king << 8;
	if ((qescape & 0x200000000000200) == 0)
		qmate |= king >> 1 & ~file<'h'>();
	if ((qescape & 0x280) == 0)
		qmate |= king >> 8;
	qmate &= getAttacks<C,Queen>() & (getAttacks<C,Rook>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,King>() | getAttacks<C,Pawn>())
					& ~(getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Knight>() | getAttacks<-C,Queen>() | getAttacks<-C,Pawn>());

	uint64_t qmate2 = 0;
	if ((qescape & 0x180000000000180) == 0)
		qmate2 |= king << 2 & ~file<'a'>() & ~file<'b'>();
	if ((qescape & 0x8280000000000002) == 0)
		qmate2 |= king << 16;
	if ((qescape & 0x300000000000300) == 0)
		qmate2 |= king >> 2 & ~file<'h'>() & ~file<'g'>();
	if ((qescape & 0x8000000000000282) == 0)
		qmate2 |= king >> 16;
	qmate2 &= getAttacks<C,Queen>() & fold(kingIncoming[EI].d02) & ~getAttacks<-C,All>();
	qmate |= qmate2;

	uint64_t rblock = getPieces<-C,Rook>() | getPieces<-C,Queen>() | getPieces<-C,Bishop>() | getPieces<-C,Knight>() | getPieces<-C,Pawn>()
                            			   | getAttacks<C,Queen>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>();
	uint64_t rescape = getAttacks<-C,King>() & ~rblock;
	rescape = __rorq(rescape, k);
	uint64_t rmate = 0;
	if ((rescape & 0x180000000000180) == 0)
		rmate |= king << 1 & ~file<'a'>();
	if ((rescape & 0x8280000000000002) == 0)
		rmate |= king << 8;
	if ((rescape & 0x300000000000300) == 0)
		rmate |= king >> 1 & ~file<'h'>();
	if ((rescape & 0x8000000000000282) == 0)
		rmate |= king >> 8;
	rmate &= getAttacks<C,Rook>() & (getAttacks<C,Queen>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,King>() | getAttacks<C,Pawn>())
	                              & ~(getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Knight>() | getAttacks<-C,Queen>() | getAttacks<-C,Pawn>());

	uint64_t rmate2 = 0;
	if ((rescape & 0x380000000000380) == 0)
		rmate2 |= kingIncoming[EI].d[0] & ~getAttacks<-C,King>();
	if ((rescape & 0x8280000000000282) == 0)
		rmate2 |= kingIncoming[EI].d[2] & ~getAttacks<-C,King>();
	rmate2 &= getAttacks<C,Rook>() | getAttacks<C,Queen>();;
	rmate |= rmate2;

	uint64_t bblock = getPieces<-C,Rook>() | getPieces<-C,Queen>() | getPieces<-C,Bishop>() | getPieces<-C,Knight>() | getPieces<-C,Pawn>()
                    | getAttacks<C,Rook>() | getAttacks<C,Queen>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>();
	uint64_t bescape = getAttacks<-C,King>() & ~bblock;

	uint64_t bmate = 0;

#if 1  //TODO knight mate is possible with one escape square
	uint64_t nblock = getPieces<-C,Rook>() | getPieces<-C,Queen>() | getPieces<-C,Bishop>() | getPieces<-C,Knight>() | getPieces<-C,Pawn>()
                    | getAttacks<C,Rook>() | getAttacks<C,Queen>() | getAttacks<C,Bishop>()                          | getAttacks<C,Pawn>();
	uint64_t nescape = getAttacks<-C,King>() & ~nblock;
	nescape = __rorq(nescape, k);
	if ((nescape & 0x8380000000000382) == 0) {
		for ( uint64_t p = knightAttacks[k] & ~occupied1 & getAttacks<C,Knight>() & ~getAttacks<-C,All>(); p; p &= p-1 ) {
	        unsigned to = bit(p);
	        for ( uint64_t f = getPieces<C,Knight>() & pins[CI] & knightAttacks[to]; f; f &= f-1) {
				if (AbortOnFirst) return true;
	        	*--*good = Move(bit(f), to, Knight);
	        }
	    }
	}
#endif
	const __v2di zero = _mm_set1_epi64x(0);
    const MoveTemplate* psingle = single[CI];
    if (bmate) {
		for(; psingle->move.piece() == Bishop; psingle++) {
			__v2di a02 = psingle->d02;
			__v2di a13 = psingle->d13;
			__v2di from2 = _mm_set1_epi64x(1ULL << psingle->move.from());
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
			pin02 = ~pin02 & a02;
			pin13 = ~pin13 & a13;
			for (uint64_t a=fold(pin02|pin13) & ~occupied1 & bmate; a; a&=a-1) {
				if (AbortOnFirst) return true;
				Move n;
				n.data = psingle->move.data + Move(0, bit(a), 0).data;
				*--*good = n;
			}
		}
    } else
		for(; psingle->move.piece() == Bishop; psingle++);

    if (rmate) {
    	for(; psingle->move.piece() == Rook; psingle++) {
			__v2di a02 = psingle->d02;
			__v2di a13 = psingle->d13;
			__v2di from2 = _mm_set1_epi64x(1ULL << psingle->move.from());
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
			pin02 = ~pin02 & a02;
			pin13 = ~pin13 & a13;
			for (uint64_t a=fold(pin02|pin13) & ~occupied1 & rmate; a; a&=a-1) {
				if (AbortOnFirst) return true;
				Move n;
				n.data = psingle->move.data + Move(0, bit(a), 0).data;
				*--*good = n;
			}
		}
    } else
    	for(; psingle->move.piece() == Rook; psingle++);

    if (qmate) {
		for(; psingle->move.piece() == Queen; psingle++) {
			__v2di a02 = psingle->d02;
			__v2di a13 = psingle->d13;
			__v2di from2 = _mm_set1_epi64x(1ULL << psingle->move.from());
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
			pin02 = ~pin02 & a02;
			pin13 = ~pin13 & a13;
			for (uint64_t a=fold(pin02|pin13) & ~occupied1 & qmate; a; a&=a-1) {
				if (AbortOnFirst) return true;
				Move n;
				n.data = psingle->move.data + Move(0, bit(a), 0).data;
				*--*good = n;
			}
		}
    }
    if (AbortOnFirst) return false;
}

#endif /* GENERATECAPTUREMOVES_TCC_ */
