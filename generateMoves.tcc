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
#ifndef GENERATEMOVES_TCC_
#define GENERATEMOVES_TCC_

#include "generateCaptureMoves.tcc"
/*
 * Generate moves which place a piece at dst, used for check evasion generation
 */
template<Colors C>
void ColoredBoard<C>::generateTargetMove(Move* &bad, uint64_t tobit ) const {
    //special case where movin a pawn blocks a check
    //move is only legal if it is not pinned
    for (uint64_t p = getPieces<C,Pawn>() & shift<-C*8>(tobit) & pins[CI] & rank<7>(); p; p &= p-1) {
        *bad++ = Move(bit(p), bit(p) + C*8, Queen, 0, true);
        *bad++ = Move(bit(p), bit(p) + C*8, Knight, 0, true);
        *bad++ = Move(bit(p), bit(p) + C*8, Rook, 0, true);
        *bad++ = Move(bit(p), bit(p) + C*8, Bishop, 0, true);
    }

    for (uint64_t p = getPieces<C,Pawn>() & shift<-C*8>(tobit) & pins[CI] & ~rank<7>(); p; p &= p-1)
        *bad++ = Move(bit(p), bit(p) + C*8, Pawn);

    for (uint64_t p = getPieces<C,Pawn>() & shift<-C*16>(tobit) & pins[CI] & rank<2>() & ~shift<-C*8>(occupied1); p; p &= p-1)
        *bad++=  Move(bit(p), bit(p) + C*16, Pawn);

    for ( uint64_t p = getAttacks<C,Knight>() & tobit; p; p &= p-1 ) {
        unsigned to = bit(p);
        for ( uint64_t f = getPieces<C,Knight>() & pins[CI] & knightAttacks[to]; f; f &= f-1)
        	*bad++ = Move(bit(f), to, Knight);
    }

    /* Check for attacks from sliding pieces. Bishop/Rook/Queen attacks are
     * stored in single[], with a mov template matching the attack in move[]
     */
    const __v2di zero = _mm_set1_epi64x(0);
    const __v2di d2 = _mm_set1_epi64x(tobit);
    for(const MoveTemplateB* bs = bsingle[CI]; bs->move.data; bs++) {
		__v2di a13 = bs->d13;
#ifdef __SSE4_1__
		if (!_mm_testz_si128(d2, a13)) {
#else
        if (fold(d2 & a13)) {
#endif
			__v2di from2 = doublebits[bs->move.from()];
			__v2di pin13 = from2 & dpins[CI].d13;
#ifdef __SSE4_1__
			pin13 = _mm_cmpeq_epi64(pin13, zero);
#else
            pin13 = _mm_cmpeq_epi32(pin13, zero);
            __v2di pin13s = _mm_shuffle_epi32(pin13, 0b10110001);
            pin13 = pin13 & pin13s;
#endif
			pin13 = ~pin13 & d2 & a13;
			for (uint64_t a=fold(pin13); a; a&=a-1) {
				Move n;
            	n.data = bs->move.data + Move(0, bit(a), 0).data;
                *bad++ = n;
            }
        }
    }

    for(const MoveTemplateR* rs = rsingle[CI]; rs->move.data; rs++) {
        __v2di a02 = rs->d02;
#ifdef __SSE4_1__
        if (!_mm_testz_si128(d2, a02)) {
#else
        if (fold(d2 & a02)) {
#endif
            __v2di from2 = doublebits[rs->move.from()];
            __v2di pin02 = from2 & dpins[CI].d02;
#ifdef __SSE4_1__
            pin02 = _mm_cmpeq_epi64(pin02, zero);
#else
            pin02 = _mm_cmpeq_epi32(pin02, zero);
            __v2di pin02s = _mm_shuffle_epi32(pin02, 0b10110001);
            pin02 = pin02 & pin02s;
#endif
            pin02 = ~pin02 & d2 & a02;
            for (uint64_t a=fold(pin02); a; a&=a-1) {
                Move n;
                n.data = rs->move.data + Move(0, bit(a), 0).data;
                *bad++ = n;
            }
        }
    }

    for(const MoveTemplateQ* qs = qsingle[CI]; qs->move.data; qs++) {
        __v2di a02 = qs->d02;
        __v2di a13 = qs->d13;
#ifdef __SSE4_1__
        if (!_mm_testz_si128(d2, (a02|a13))) {
#else
        if (fold(d2 & (a02|a13))) {
#endif
            __v2di from2 = doublebits[qs->move.from()];
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
                n.data = qs->move.data + Move(0, bit(a), 0).data;
                *bad++ = n;
            }
        }
    }
}

template<Colors C>
void ColoredBoard<C>::generateCheckEvasions(Move* &good, Move* &bad) const {
    uint64_t king = getPieces<C,King>();
//    ASSERT(king & getAttacks<-C,All>());
    ASSERT(inCheck<C>());
    /*
     * Determine directions of checking, set bits in "check" accordingly
     * If attacked by a single piece, blocking moves or capturing the
     * attacking piece is allowed, otherwise only escape moves are considered
     * Escape directions are determined by precalculated kingAttacks.
     */
    __v2di king2 = _mm_set1_epi64x(king);
    __v2di zero = _mm_set1_epi64x(0);
    __v2di check02 = datt[EI].d02 & king2;
    __v2di check13 = datt[EI].d13 & king2;
#ifdef __SSE4_1__
    check02 = _mm_cmpeq_epi64(check02, zero);
    check13 = _mm_cmpeq_epi64(check13, zero);
#else
    check02 = _mm_cmpeq_epi32(check02, zero);
    check13 = _mm_cmpeq_epi32(check13, zero);
    check02 = check02 & _mm_shuffle_epi32(check02, 0b10110001);
    check13 = check13 & _mm_shuffle_epi32(check13, 0b10110001);
#endif
    check02 = ~check02 & _mm_set_epi64x(2,1);
    check13 = ~check13 & _mm_set_epi64x(8,4);

    unsigned check = fold(check02|check13)
                   + (king & getAttacks<-C, Knight>() ? 16:0)
                   + (king & getAttacks<-C, Pawn>() ? 32:0);
    ASSERT(check);
    unsigned int kingSq = bit(king);
    uint64_t pawn = 0;
    if (__builtin_parity(check)) {
        if (check == 16) {
            // attack by knight, try to capture it.
            if (uint64_t p = knightAttacks[kingSq] & getPieces<-C, Knight>())
                generateTargetCapture<true>(good, bad, p, Knight);
        } else if (check & 0xf) {
            // attack by a single sliding piece. determine position
            // and try to capture it. generate blocking moves
            uint64_t i02 = fold(kingIncoming[CI].d02) & getAttacks<C,All>();
            uint64_t i13 = fold(kingIncoming[CI].d13) & getAttacks<C,All>();
            if (uint64_t p = (i02|i13) & getPieces<-C,Queen>())
                generateTargetCapture<true>(good, bad, p, Queen);
            if (uint64_t p = i02 & getPieces<-C,Rook>())
                generateTargetCapture<true>(good, bad, p, Rook);
            if (uint64_t p = i13 & getPieces<-C,Bishop>())
                generateTargetCapture<true>(good, bad, p, Bishop);

            if (uint64_t p = datt[EI].d[bit(check)] & kingIncoming[CI].d[bit(check)] & ~occupied1)
                generateTargetMove(bad, p);
        } else {
            // No other pieces left, we are attacked by one pawn.
            ASSERT(check == 32);
            if (C == White)
                pawn = (king << 7 & ~file<'h'>()) + (king << 9 & ~file<'a'>());
            else
                pawn = (king >> 9 & ~file<'h'>()) + (king >> 7 & ~file<'a'>());
            pawn &= getPieces<-C, Pawn>();
            ASSERT(popcount(pawn)==1);
            generateTargetCapture<true>(good, bad, pawn, Pawn);
            if (pawn == cep.enPassant) {
                if (uint64_t p = getPieces<C,Pawn>() & ~file<'a'>() & cep.enPassant<<1 & getPins<C,2+C>())
                    *--good = Move(bit(p), bit(p) + C*8-1, Pawn, Pawn, true);

                if (uint64_t p = getPieces<C,Pawn>() & ~file<'h'>() & cep.enPassant>>1 & getPins<C,2-C>())
                    *--good = Move(bit(p), bit(p) + C*8+1, Pawn, Pawn, true);
            }

        }
    } else {
        // test, if capturing an adjacent checking piece with the king is possible
        // this is not covered by non capturing king moves below.
        if (uint64_t p = kingAttacks[0b1100][kingSq] & ~getAttacks<-C, All>() & getPieces<-C, Rook>())
            *--good = Move(kingSq, bit(p), King, Rook);
        else if (uint64_t p = kingAttacks[0b0011][kingSq] & ~getAttacks<-C, All>() & getPieces<-C, Bishop>())
            *--good = Move(kingSq, bit(p), King, Bishop);
        else if (uint64_t p = kingAttacks[0b0000][kingSq] & ~getAttacks<-C, All>() & getPieces<-C, Queen>())
            *--good = Move(kingSq, bit(p), King, Queen);
    }

    // non capturing check evasions
    ASSERT(popcount(check & 0xf) <= 2);
    for (uint64_t p = kingAttacks[check & 0xf][kingSq] & ~getAttacks<-C, All>() & ~occupied1; p; p &= p-1)
        *bad++ = Move(kingSq, bit(p), King);

    // capturing evasions of non checking adjacent pieces. checking pieces
    // have the direction disallowed by the check & 0xf mask, so they are
    // covered by generateTargetCapture in the single check sliding attack
    // or with the individual tests above
    for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                    & ~getAttacks<-C, All>()
                    & getPieces<-C,Rook>(); p; p &= p-1)
        *--good = Move(kingSq, bit(p), King, Rook);

    for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                    & ~getAttacks<-C, All>()
                    & getPieces<-C,Bishop>(); p; p &= p-1)
        *--good = Move(kingSq, bit(p), King, Bishop);

    for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                    & ~getAttacks<-C, All>()
                    & getPieces<-C,Knight>(); p; p &= p-1)
        *--good = Move(kingSq, bit(p), King, Knight);

    for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                    & ~getAttacks<-C, All>()
                    & getPieces<-C,Pawn>()
                    & ~pawn; p; p &= p-1)
        *--good = Move(kingSq, bit(p), King, Pawn);
    return;
}

template<Colors C>
void ColoredBoard<C>::generateMoves(Move* &good) const {
    unsigned int from, to;
    from = bit(getPieces<C, King>());
    for (uint64_t p = kingAttacks[0][from] & ~occupied1 & ~getAttacks<-C,All>(); p; p &= p-1)
        *--good = Move(from, bit(p), King);

    // pawn moves, non capture, non promo
    for (uint64_t p = getPieces<C,Pawn>() & ~rank<7>() & shift<-C*8>(~occupied1) & getPins<C,2>(); p; p &= p-1) {
        from = bit(p);
        to = from + C*dirOffsets[2];
        *--good = Move(from, to, Pawn);
    }
    for (uint64_t p = getPieces<C,Pawn>() & rank<2>() & shift<-C*8>(~occupied1) & shift<-C*16>(~occupied1) & getPins<C,2>(); p; p &= p-1) {
        from = bit(p);
        to = from + 2*C*dirOffsets[2];
        *--good = Move (from, to, Pawn);
    }
    /*
     * Sliding pieces. If we detect a possible pinning, allow only the moves in
     * the directions of the pin. For masking the pinned directions use the
     * attack information stored in single[] in the same order as they were
     * generated earlier in buildAttacks()
     */
    const __v2di zero = _mm_set1_epi64x(0);
    for(const MoveTemplateB* bs = bsingle[CI]; bs->move.data; bs++) {
		__v2di a13 = bs->d13;
		__v2di from2 = doublebits[bs->move.from()];
		__v2di pin13 = from2 & dpins[CI].d13;
#ifdef __SSE4_1__
        pin13 = _mm_cmpeq_epi64(pin13, zero);
#else
        pin13 = _mm_cmpeq_epi32(pin13, zero);
        __v2di pin13s = _mm_shuffle_epi32(pin13, 0b10110001);
        pin13 = pin13 & pin13s;
#endif
		pin13 = ~pin13 & a13;
		for (uint64_t a=fold(pin13) & ~occupied1; a; ) {
			Move n;
            unsigned to = C==White ? bit(a):bitr(a);
			n.data = bs->move.data + Move(0, to, 0).data;
			*--good = n;
            a &= ~(1ULL << to);
        }
    }

    for(const MoveTemplateR* rs = rsingle[CI]; rs->move.data; rs++) {
        __v2di a02 = rs->d02;
        __v2di from2 = doublebits[rs->move.from()];
        __v2di pin02 = from2 & dpins[CI].d02;
#ifdef __SSE4_1__
        pin02 = _mm_cmpeq_epi64(pin02, zero);
#else
        pin02 = _mm_cmpeq_epi32(pin02, zero);
        __v2di pin02s = _mm_shuffle_epi32(pin02, 0b10110001);
        pin02 = pin02 & pin02s;
#endif
        pin02 = ~pin02 & a02;
        for (uint64_t a=fold(pin02) & ~occupied1; a; ) {
            Move n;
            unsigned to = C==White ? bit(a):bitr(a);
            n.data = rs->move.data + Move(0, to, 0).data;
            *--good = n;
            a &= ~(1ULL << to);
        }
    }

    //Knight moves. If a knight is pinned, it can't move at all.
    for (uint64_t p = getPieces<C, Knight>() & pins[CI]; p; ) {
        from = C==White ? bitr(p):bit(p);
        for (uint64_t q = knightAttacks[from] & ~occupied1; q; ) {
            unsigned to = C==White ? bit(q):bitr(q);
            *--good = Move(from, to, Knight);
            q &= ~(1ULL << to);
        }
        p &= ~(1ULL << from);
    }

    for(const MoveTemplateQ* qs = qsingle[CI]; qs->move.data; qs++) {
        __v2di a02 = qs->d02;
        __v2di a13 = qs->d13;
        __v2di from2 = doublebits[qs->move.from()];
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
        for (uint64_t a=fold(pin02|pin13) & ~occupied1; a; ) {
            Move n;
            unsigned to = C==White ? bit(a):bitr(a);
            n.data = qs->move.data + Move(0, to, 0).data;
            *--good = n;
            a &= ~(1ULL << to);
        }
    }
    /*
     * test the castling flags and if there is free room between k and r
     * additionally check for non threatened squares at the king
     */
    if ( cep.castling.color[CI].q
        && (occupied1 &             (C==White ? 0xe:0xe00000000000000)) == 0
        && (getAttacks<-C, All>() & (C==White ? 0xc:0xc00000000000000)) == 0 )
            *--good = Move(pov^e1, pov^c1, King, 0, true);

    if ( cep.castling.color[CI].k
        && (occupied1 &             (C==White ? 0x60:0x6000000000000000)) == 0
        && (getAttacks<-C, All>() & (C==White ? 0x60:0x6000000000000000)) == 0 )
            *--good = Move(pov^e1, pov^g1, King, 0, true);
    /*
     * En passant capture
     * Allow a pinned pawn to move, if it is moving in the direction of the pin. The captured pawn can never be pinned,
     * since it must have been moved before and so our king would have been in check while the enemy was on move.
     * Additionally check if we have reached the 5th rank, else we might capture our own pawn,
     * if we made two moves in a row and the last move was a double step of our own pawn
	 * Handle special case, where king, capturing pawn, captured pawn and horizontal
	 * attacking piece are on one line. Although neither pawn is pinned, the capture
	 * is illegal, since both pieces are removed and the king will be in check
     */
    if (uint64_t p = getPieces<C,Pawn>() & ~file<'a'>() & rank<5>() & cep.enPassant<<1 & getPins<C,2+C>())
    	if (((datt[EI].d[0]|kingIncoming[CI].d[0]) & (p|cep.enPassant)) != (p|cep.enPassant))
    		*--good = Move(bit(p), bit(p) + C*8-1, Pawn, Pawn, true);

    if (uint64_t p = getPieces<C,Pawn>() & ~file<'h'>() & rank<5>() & cep.enPassant>>1 & getPins<C,2-C>())
    	if (((datt[EI].d[0]|kingIncoming[CI].d[0]) & (p|cep.enPassant)) != (p|cep.enPassant))
    		*--good = Move(bit(p), bit(p) + C*8+1, Pawn, Pawn, true);
}

#endif /* GENERATEMOVES_TCC_ */
