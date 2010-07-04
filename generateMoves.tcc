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
void ColoredBoard<C>::generateTargetMove(Move* &list, uint64_t tobit ) const {
#ifdef BITBOARD
    //special case where movin a pawn blocks a check
    //move is only legal if it is not pinned
    for (uint64_t p = getPieces<C,Pawn>() & shift<-C*8>(tobit) & pins[CI] & rank<7>(); p; p &= p-1) {
        *list++ = Move(bit(p), bit(p) + C*8, Queen, 0, true);
        *list++ = Move(bit(p), bit(p) + C*8, Knight, 0, true);
        *list++ = Move(bit(p), bit(p) + C*8, Rook, 0, true);
        *list++ = Move(bit(p), bit(p) + C*8, Bishop, 0, true);
    }

    for (uint64_t p = getPieces<C,Pawn>() & shift<-C*8>(tobit) & pins[CI] & ~rank<7>(); p; p &= p-1)
        *list++ = Move(bit(p), bit(p) + C*8, Pawn);

    for (uint64_t p = getPieces<C,Pawn>() & shift<-C*16>(tobit) & pins[CI] & rank<2>() & ~shift<-C*8>(occupied1); p; p &= p-1)
        *list++=  Move(bit(p), bit(p) + C*16, Pawn);
    
    for ( uint64_t p = getAttacks<C,Knight>() & tobit; p; p &= p-1 ) {
        unsigned to = bit(p);
        for ( uint64_t f = getPieces<C,Knight>() & pins[CI] & knightAttacks[to]; f; f &= f-1)
        	*list++ = Move(bit(f), to, Knight);
    }

    /* Check for attacks from sliding pieces. Bishop/Rook/Queen attacks are
     * stored in single[], with a mov template matching the attack in move[]
     */
    const MoveTemplate* psingle = single[CI];
    const __v2di zero = _mm_set1_epi64x(0);
    const __v2di d2 = _mm_set1_epi64x(tobit);
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
            	n.data = m.data + Move(0, bit(a), 0).data;
                *list++ = n;
            }
        }
    }
#else
    ASSERT(to < nSquares);
    Attack a = attacks<CI>( to );
    uint8_t cap = pieces[to];
    uint8_t dir;
    uint8_t from;
    uint8_t pin;
    unsigned int nAttacks;
    uint64_t sources;
    /*
     * If we are capturing, we need a special treatment if the captured piece is a pawn which
     * has made a doublestep, because we can escape from a check by capturing e.p.
     * Special treatment is also needed for pawn captures, they may be promotions
     * And for captured rooks, as this may disable castling for the opponent
     */
    if ( cap ) {
        if ( cep.enPassant == to )
            for ( unsigned int i=0; i<pieceList[CI][Pawn]; ++i ) {
                from = pieceList[CI].getPawn(i);
                if ( (from == to + 1) | (from == to - 1) )
                    /* The situation is now that a pawn is giving us check. This pawn has made a double
                     * step in the move before and can be captured en passant by one of our pawns.
                     * The move is only legal if our pawn is not pinned, since this pawn will move on
                     * a different vector than the pin in every possible case. Avoiding a check
                     * by moving a pawn in between with an e. p. capture is impossible
                     * leave capture field empty, otherwise we would capture twice */
                    if (!isValid(detectPin(from))) *list++ = (Move) {{ from, (uint8_t)(to+C*8), 0, EP }};
            }

        if ( a.s.PR ) {
            from = to-C*9;
            ASSERT(pieces[from] == C*Pawn);
            if (!isValid(detectPin(from))) {
                if (isPromoRank(from)) {
                    *list++ = (Move) {{ from, to, cap, promoteQ }};
                    *list++ = (Move) {{ from, to, cap, promoteN }};
                    *list++ = (Move) {{ from, to, cap, promoteR }};
                    *list++ = (Move) {{ from, to, cap, promoteB }};
                } else
                    *list++ = (Move) {{ from, to, cap, nothingSpecial }};
            }
        }

        if ( a.s.PL ) {
            from = to-C*7;
            ASSERT(pieces[from] == C*Pawn);
            if (!isValid(detectPin(from))) {
                if (isPromoRank(from)) {
                    *list++ = (Move) {{ from, to, cap, promoteQ }};
                    *list++ = (Move) {{ from, to, cap, promoteN }};
                    *list++ = (Move) {{ from, to, cap, promoteR }};
                    *list++ = (Move) {{ from, to, cap, promoteB }};
                } else
                    *list++ = (Move) {{ from, to, cap, nothingSpecial }};
            }
        }

    } else {
        //special case where movin a pawn blocks a check
        //move is only legal if it is not pinned
        from = to - C*8;
        if ( from < nSquares && pieces[from] == C*Pawn ) {
            if (!isValid(detectPin(from))) {
                if (isPromoRank(from)) {
                    *list++ = (Move) {{ from, to, 0, promoteQ }};
                    *list++ = (Move) {{ from, to, 0, promoteN }};
                    *list++ = (Move) {{ from, to, 0, promoteR }};
                    *list++ = (Move) {{ from, to, 0, promoteB }};
                } else
                    *list++ = (Move) {{ from, to, 0, 0 }};

            }
        } else {
            from = to - C*16;
            if ( isRank<2>(from) & (pieces[from] == C*Pawn) & (pieces[to-C*8] == 0) )
                if (!isValid(detectPin(from))) {
                    bool pawnadj = ((to & 7) != 7 && pieces[to+1] == -C*Pawn)
                                || ((to & 7) != 0 && pieces[to-1] == -C*Pawn);
                    *list++ = (Move) {{ from, to, 0, pawnadj ? enableEP:nothingSpecial }};
                }

        }
    }

    if (a.s.N) {
        nAttacks = a.s.N;
        sources = pieceList[CI].getAll<Knight>();
        unsigned int n = 0;
        while (true) {
            ASSERT(++n <= pieceList[CI][Knight]);
            from = sources;
            sources >>= 8;
            if (isKnightDistance(from, to)) {
                if (!isValid(detectPin(from))) *list++ = (Move) {{from, to, cap, nothingSpecial}};
                if (!--nAttacks) break;
            }
        }
    }

    if (a.l.B) {
        nAttacks = a.l.B;
        sources = pieceList[CI].getAll<Bishop>();
        unsigned int n = 0;
        while(true) {
            ASSERT(++n <= pieceList[CI][Bishop]);
            from = sources;
            sources >>= 8;
            dir = vec2dir[from][to];
            if (isValid(dir) & dir & 1 && length(dir^4, to)*dirOffsets[dir] + from == to) {
                pin = detectPin(from);
                if (!isValid(pin))    *list++ = (Move) {{from, to, cap, nothingSpecial}};
                if (!--nAttacks) break;
            }
        }
    }

    if (a.l.R) {
        nAttacks = a.l.R;
        sources = pieceList[CI].getAll<Rook>();
        unsigned int n = 0;
        while(true) {
            ASSERT(++n <= pieceList[CI][Rook]);
            from = sources;
            sources >>= 8;
            dir = vec2dir[from][to];
            if (~dir & 1 && length(dir^4, to)*dirOffsets[dir] + from == to) {
                pin = detectPin(from);
                if (!isValid(pin)) *list++ = (Move) {{from, to, cap, nothingSpecial}};
                if (!--nAttacks) break;
            }
        }
    }

    if (a.l.Q) {
        nAttacks = a.l.Q;
        sources = pieceList[CI].getAll<Queen>();
        unsigned int n = 0;
        while(true) {
            ASSERT(++n <= pieceList[CI][Queen]);
            from = sources;
            sources >>= 8;
            dir = vec2dir[from][to];
            if (isValid(dir) && from + length(dir^4, to)*dirOffsets[dir] == to) {
                pin = detectPin(from);
                if (!isValid(pin)) *list++ = (Move) {{from, to, cap, nothingSpecial}};
                if (!--nAttacks) break;
            }
        }
    }
#endif    
}

#ifndef BITBOARD
template<Colors C>
void ColoredBoard<C>::ray(Move* &list, uint8_t from, uint8_t dir) const {
    ASSERT (dir < 4);
    Length len = attLen[dir][from];
//    len.right -= attVec[dir][from].lIndex != 0;    // contains next piece right direction dir, or 0 if border
    len.data -= borderTable[attVec[dir][from].data].data;
    __m128i moveFrom = moveFromTable[from].m0123;    // contains 4* { from, from, 0, 0 }
    __m128i move0123 = _mm_add_epi8(moveOffsetTable[len.right][dir].m0123, moveFrom);
    _mm_storeu_si128( (__m128i*)&list[0], move0123);
    uint8_t totalLen = len.left + len.right;
    if (totalLen > 4) {
        __m128i move456 = _mm_add_epi8(moveOffsetTable[len.right][dir].m456, moveFrom);
        _mm_storeu_si128( (__m128i*)&list[4], move456);
    }
    list += totalLen;
}
#endif

/*template<Colors C>
void ColoredBoard<C>::ray(Move* &list, uint8_t from, uint8_t dir) const {
    ray_nonvectorized(list, from, dir);
    ray_nonvectorized(list, from, dir+4);
}
*/

/*template<Colors C>
void ColoredBoard<C>::ray_vectorized(Move* &list, uint8_t from, uint8_t dir) const {
    unsigned int l = length(dir, from);
    if (!l) return;
    uint32_t to = from + dirOffsets[dir];
    uint32_t m = from + ( to << 8 );
    for (unsigned int i=l; i>1; --i) {
        list++->data = m;
        m += dirOffsets[dir] << 8;
    }
    list->data = m;
    list += !pieces[m >> 8];
}
*/
template<Colors C>
Move* ColoredBoard<C>::generateMoves(Move* list) const {
#ifdef BITBOARD
    uint64_t king = getPieces<C,King>();
    if (king & getAttacks<-C,All>()) {       
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
                    generateTargetCapture<true>(list, p, Knight);
            } else if (check & 0xf) {
            	// attack by a single sliding piece. determine position
            	// and try to capture it. generate blocking moves
            	uint64_t i02 = fold(kingIncoming[CI].d02) & getAttacks<C,All>();
            	uint64_t i13 = fold(kingIncoming[CI].d13) & getAttacks<C,All>();
                if (uint64_t p = (i02|i13) & getPieces<-C,Queen>())
                    generateTargetCapture<true>(list, p, Queen);
                if (uint64_t p = i02 & getPieces<-C,Rook>())
                    generateTargetCapture<true>(list, p, Rook);
                if (uint64_t p = i13 & getPieces<-C,Bishop>())
                    generateTargetCapture<true>(list, p, Bishop);

                if (uint64_t p = datt[EI].d[bit(check)] & kingIncoming[CI].d[bit(check)] & ~occupied1)
                    generateTargetMove(list, p);
            } else {
                // No other pieces left, we are attacked by one pawn.
            	ASSERT(check == 32);
                if (C == White)
                    pawn = (king << 7 & 0x7f7f7f7f7f7f7f7f) + (king << 9 & 0xfefefefefefefefe);
                else
                    pawn = (king >> 9 & 0x7f7f7f7f7f7f7f7f) + (king >> 7 & 0xfefefefefefefefe);
                pawn &= getPieces<-C, Pawn>();
                ASSERT(popcount(pawn)==1);
                generateTargetCapture<true>(list, pawn, Pawn);
                if (pawn == cep.enPassant) {
					if (uint64_t p = getPieces<C,Pawn>() & ~file<'a'>() & cep.enPassant<<1 & getPins<C,2+C>())
						*list++ = Move(bit(p), bit(p) + C*8-1, Pawn, Pawn, true);

					if (uint64_t p = getPieces<C,Pawn>() & ~file<'h'>() & cep.enPassant>>1 & getPins<C,2-C>())
						*list++ = Move(bit(p), bit(p) + C*8+1, Pawn, Pawn, true);
                }

            }
        } else {
			// test, if capturing an adjacent checking piece with the king is possible
			// this is not covered by non capturing king moves below.
			if (uint64_t p = kingAttacks[0b1100][kingSq] & ~getAttacks<-C, All>() & getPieces<-C, Rook>())
				*list++ = Move(kingSq, bit(p), King, Rook);
			else if (uint64_t p = kingAttacks[0b0011][kingSq] & ~getAttacks<-C, All>() & getPieces<-C, Bishop>())
				*list++ = Move(kingSq, bit(p), King, Bishop);
			else if (uint64_t p = kingAttacks[0b0000][kingSq] & ~getAttacks<-C, All>() & getPieces<-C, Queen>())
				*list++ = Move(kingSq, bit(p), King, Queen);
        }

        // non capturing check evasions
        for (uint64_t p = kingAttacks[check & 0xf][kingSq] & ~getAttacks<-C, All>() & ~occupied1; p; p &= p-1)
            *list++ = Move(kingSq, bit(p), King);

        // capturing evasions of non checking adjacent pieces. checking pieces
        // have the direction disallowed by the check & 0xf mask, so they are
        // covered by generateTargetCapture in the single check sliding attack
        // or with the individual tests above
        for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                        & ~getAttacks<-C, All>()
                        & getPieces<-C,Rook>(); p; p &= p-1)
            *list++ = Move(kingSq, bit(p), King, Rook);

        for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                        & ~getAttacks<-C, All>()
                        & getPieces<-C,Bishop>(); p; p &= p-1)
            *list++ = Move(kingSq, bit(p), King, Bishop);

        for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                        & ~getAttacks<-C, All>()
                        & getPieces<-C,Knight>(); p; p &= p-1)
            *list++ = Move(kingSq, bit(p), King, Knight);

        for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                        & ~getAttacks<-C, All>()
                        & getPieces<-C,Pawn>()
                        & ~pawn; p; p &= p-1)
            *list++ = Move(kingSq, bit(p), King, Pawn);

        return list;
    }

    list = generateCaptureMoves<true>( list );

    unsigned int from, to;
    for (uint64_t p = getPieces<C,Pawn>() & ~rank<7>() & shift<-C*8>(~occupied1) & getPins<C,2>(); p; p &= p-1) {
        from = bit(p);
        to = from + C*dirOffsets[2];
        *list++ = Move(from, to, Pawn);
    }
    for (uint64_t p = getPieces<C,Pawn>() & rank<2>() & shift<-C*8>(~occupied1) & shift<-C*16>(~occupied1) & getPins<C,2>(); p; p &= p-1) {
        from = bit(p);
        to = from + 2*C*dirOffsets[2];
        *list++ = Move (from, to, Pawn);
    }
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
    if (uint64_t p = getPieces<C,Pawn>() & ~file<'a'>() & cep.enPassant<<1 & getPins<C,2+C>())
    	if (((datt[EI].d[0]|kingIncoming[CI].d[0]) & (p|cep.enPassant)) != (p|cep.enPassant))
    		*list++ = Move(bit(p), bit(p) + C*8-1, Pawn, Pawn, true);

    if (uint64_t p = getPieces<C,Pawn>() & ~file<'h'>() & cep.enPassant>>1 & getPins<C,2-C>())
    	if (((datt[EI].d[0]|kingIncoming[CI].d[0]) & (p|cep.enPassant)) != (p|cep.enPassant))
    		*list++ = Move(bit(p), bit(p) + C*8+1, Pawn, Pawn, true);
    
    //Knight moves. If a knight is pinned, it can't move at all.
    for (uint64_t p = getPieces<C, Knight>() & pins[CI]; p; p &= p-1) {
        from = bit(p);
        for (uint64_t q = knightAttacks[from] & ~occupied1; q; q &= q-1)
            *list++ = Move(from, bit(q), Knight);
    }
    /*
     * Sliding pieces. If we detect a possible pinning, allow only the moves in
     * the directions of the pin. For masking the pinned directions use the
     * attack information stored in singel[] in the same order as they were
     * generated earlier in buildAttacks()
     */
    const MoveTemplate* psingle = single[CI];
    const __v2di zero = _mm_set1_epi64x(0);
    for(;;) {
        Move m = psingle->move;
		if (!m.data) break;
        __v2di a02 = psingle->d02;
		__v2di a13 = psingle->d13;
		psingle++;
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
		pin02 = ~pin02 & a02;
		pin13 = ~pin13 & a13;
		for (uint64_t a=fold(pin02|pin13) & ~occupied1; a; a&=a-1) {
			Move n;
			n.data = m.data + Move(0, bit(a), 0).data;
			*list++ = n;
        }
    }
    /* castling
     * test the castling flags,
     * if there is enough free space besides the king (ATTACKLEN)
     * if the king-squares are not under attack, a test if we are in check is
     * not neccessary, we won't ever get here because of check evasion
     */
    if ( cep.castling.color[CI].q
        && (occupied1 &             (C==White ? 0xe:0xe00000000000000)) == 0
        && (getAttacks<-C, All>() & (C==White ? 0xc:0xc00000000000000)) == 0 )
            *list++ = Move(pov^e1, pov^c1, King, 0, true);

    if ( cep.castling.color[CI].k
        && (occupied1 &             (C==White ? 0x60:0x6000000000000000)) == 0
        && (getAttacks<-C, All>() & (C==White ? 0x60:0x6000000000000000)) == 0 ) 
            *list++ = Move(pov^e1, pov^g1, King, 0, true);

    from = bit(getPieces<C, King>());
    for (uint64_t p = kingAttacks[0][from] & ~occupied1 & ~getAttacks<-C,All>(); p; p &= p-1)
        *list++ = Move(from, bit(p), King);
#else
    uint8_t king = pieceList[CI].getKing();
    Attack kingAttacks;
    kingAttacks.data = attacks<EI>(king) & attackMask;

    if (kingAttacks) {
        unsigned int attDir = ~0; //used as disallowed move directions in evasion
        unsigned int attDir2 = ~0;
        /*
         * Test if attacked by a single piece. The king can at most be attacked by two pieces, so
         * here are at least one and at most two bits set. The two pieces are not guaranteed to be
         * different, because a pawn may uncover a check and promote to the same piece. So a parity
         * check does not work in deciding if we have a double check.
         * If so, skip to the king moves, no other moves are legal
         * first check for a attack from a sliding piece. in this case all moves with destination
         * to a square between the king and the attacking piece or destination attacking piece are
         * allowed.
         */
        ASSERT(__builtin_popcount(kingAttacks) <= 2);
        if (__builtin_parity(kingAttacks) && !(kingAttacks.l & doubleAttack) ) {

            /* Test if attacked by a sliding piece. Find opposite direction of the attack.
             * Besides evading moves (which are genreated later), the only Legal moves are moves
             * blocking or capturing the piece */
            if (kingAttacks & attackMaskLong) {
                //TODO replace by a table lookup
                unsigned int len;
                for (unsigned int antiDir = 0;; ++antiDir) {
                    ASSERT( antiDir < 4);
                    if (isLongAttack(antiDir+4, king)) {
                        len = attLen[antiDir][king].right;
                        attDir = antiDir + 4;
                        break;
                    }

                    if (isLongAttack(antiDir, king)) {
                        len = attLen[antiDir][king].left;
                        attDir = antiDir;
                        break;
                    }
                }

                unsigned int dst = king;
                for (unsigned int i = len; i > 0; --i) {
                    dst -= dirOffsets[attDir];
                    generateTargetMove(list, dst);
                }

                // Test if attacked by a knight. Only moves capturing the knight are legal
            } else if (kingAttacks.s.N) {
                unsigned int i;
                for (i = 0; i < pieceList[EI][Knight]; ++i)
                    if (isKnightDistance(king, pieceList[EI].get(Knight, i))) break;

                ASSERT(isKnightDistance(king, pieceList[EI].get(Knight, i)));
                generateTargetMove(list, pieceList[EI].get(Knight, i));

                // No other pieces left, we are attacked by one pawn. only two possible destinations
            } else {
                ASSERT(kingAttacks.s.PR ^ kingAttacks.s.PL);
                if (kingAttacks.s.PR) {
                    unsigned int to = king + dirOffsets[1 + CI * 4];
                    ASSERT(pieces[to] == -C * Pawn);
                    generateTargetMove(list, to);
                } else {
                    unsigned int to = king + dirOffsets[3 + CI * 4];
                    ASSERT(pieces[to] == -C * Pawn);
                    generateTargetMove(list, to);
                }
            }
        } else {
            //double check. find if we are attacked by two sliding pieces, and caculate the direction of the second attack
            //king movement isn't allowd in neither of these directions
            for (unsigned int i = 0; i < 4; ++i) {
                if (isLongAttack(i, king)) {
                    attDir2 = attDir;
                    attDir = i;
                }
                if (isLongAttack(i+4, king)) {
                    attDir2 = attDir;
                    attDir = i + 4;
                }
            }
        }

        for (unsigned int i = 0; i < 8; ++i) {
            uint8_t to = king + dirOffsets[i];
            if (!isKingDistance(king, to)) continue;
            int8_t cap = pieces[to];
            if (i != attDir && i != attDir2 && C * cap <= 0 && !((longAttack[EI][to] & attackMaskLong) | (shortAttack[EI][to] & attackMaskShort)))
                *list++ = (Move) {{king, to, cap, nothingSpecial}};
        }
        return list;
    }

    list = generateCaptureMoves( list );

    uint8_t from;
    uint8_t to;
    uint8_t pin;
    int8_t cap;
    for (unsigned int i = 0; i < pieceList[CI][Pawn]; ++i) {
        from = pieceList[CI].getPawn(i);
        pin = detectPin(from);

        //generate underpromotions. queening is already done by movegenq.
        //if a promoting pawn is pinned it must not move - except if it is capturing in the opposite direction of the pin.
        if (isPromoRank(from)) {
            to = from + C * 9;
            if (((from&7) != 7*EI) & (!isValid(pin) | (pin == 1))) {
                cap = pieces[to];
                if (C*cap < 0) {
                    *list++ = (Move) {{from, to, cap, promoteN}};
                    *list++ = (Move) {{from, to, cap, promoteR}};
                    *list++ = (Move) {{from, to, cap, promoteB}};
                }
            }
            to = from + C * 7;
            if (((from&7) != 7*CI) & (!isValid(pin) | (pin == 3))) {
                cap = pieces[to];
                if (C*cap < 0) {
                    *list++ = (Move) {{from, to, cap, promoteN}};
                    *list++ = (Move) {{from, to, cap, promoteR}};
                    *list++ = (Move) {{from, to, cap, promoteB}};
                }
            }

            to = from + C * 8;
            cap = pieces[to];
            if (!cap & !isValid(pin)) {
                *list++ = (Move) {{from, to, 0, promoteN}};
                *list++ = (Move) {{from, to, 0, promoteR}};
                *list++ = (Move) {{from, to, 0, promoteB}};
            }
            continue; // skip normal and en passant moves;
        }
        //normal pawn moves.
        //if the pawn is pinned, it is allowed to move if the pin is from direction 2 or 6 (vertically)
        to = from + C * 8;
        if ((!pieces[to]) & (!isValid(pin) | (pin == 2))) {
            *list++ = (Move) {{ from, to }};
            to = from + C * 16;
            if (isRank<2> (from) && !pieces[to]) {
                bool pawnadj = (to != (h4^pov) && pieces[to+1] == -C*Pawn)
                             || (to != (a4^pov) && pieces[to-1] == -C*Pawn);
                *list++ = (Move) {{from, to, 0, pawnadj ? enableEP:nothingSpecial}};
                continue; // if a pawn has made a double step, skip the en passant check
            }
        }
/*
 * En passant capture
 * Allow a pinned pawn to move, if it is moving in the direction of the pin. The captured pawn can never be pinned,
 * since it must have been moved before and so our king would have been in check while the enemy was on move.
 * Additionally check if we have reached the 5th rank, else we might capture our own pawn,
 * if we made two moves in a row and the last move was a double step of our own pawn
 */
        if ( cep.enPassant && isRank<5>(from) && isRank<5>(cep.enPassant))  // last check is for null search, otherwise after h2-h4 a white pawn a5 may capture at h4 if a black pawn is present at g4. Happened.
        if (((( from+C == cep.enPassant ) && ( !isValid(pin) | (pin == 1)))
        || (( from-C == cep.enPassant ) && ( !isValid(pin) | (pin == 3))))) {
            /*
             * Handle special case, where king, capturing pawn, captured pawn and horizontal
             * attacking piece are on one line. Although neither pawn is pinned, the capture
             * is illegal, since both pieces are removed and the king will be in check
             */
            if (((attVec[0][from].lIndex != C*King) & (attVec[0][from].rIndex != C*King)
                || (attVec[0][cep.enPassant].lIndex != -C*Rook) & (attVec[0][cep.enPassant].rIndex != -C*Rook)
                    & (attVec[0][cep.enPassant].lIndex != -C*Queen) & (attVec[0][cep.enPassant].rIndex != -C*Queen))
            &&  ((attVec[0][cep.enPassant].lIndex != C*King) & (attVec[0][cep.enPassant].rIndex != C*King)
                || (attVec[0][from].lIndex != -C*Rook) & (attVec[0][from].rIndex != -C*Rook)
                    & (attVec[0][from].lIndex != -C*Queen) & (attVec[0][from].rIndex != -C*Queen))) {
                to = cep.enPassant + C*8;
                *list++ = (Move) {{ from, to, 0, EP}};    //leave capture field empty, otherwise we would capture twice
            }
        }
    }
    /*
     * Sliding pieces. If we detect a possible pinning, allow only the moves in
     * the directions of the pin. In this case the difference between the
     * direction
     */
    for (unsigned int i = 0; i<pieceList[CI][Queen]; ++i) {
        from = pieceList[CI].get(Queen, i);
        pin = detectPin(from);

        if (!isValid(pin)) {
            ray(list, from, 0);
            ray(list, from, 1);
            ray(list, from, 2);
            ray(list, from, 3);
        } else {
            ray(list, from, pin);
        }
    }

    for (unsigned int i = 0; i<pieceList[CI][Bishop]; ++i) {
        from = pieceList[CI].get(Bishop, i);
        pin = detectPin(from);

        if (!isValid(pin)) {
            ray(list, from, 1);
            ray(list, from, 3);
        } else if (pin & 1) {
            ray(list, from, pin);
        }
    }

    for (unsigned int i = 0; i<pieceList[CI][Rook]; ++i) {
        from = pieceList[CI].get(Rook, i);
        pin = detectPin(from);

        if (!isValid(pin)) {
            ray(list, from, 0);
            ray(list, from, 2);
        } else if (~pin & 1) {
            ray(list, from, pin);
        }
    }

    //Knight moves. If a knight is pinned, it can't move at all.
    for (unsigned int i = 0; i<pieceList[CI][Knight]; ++i) {
        from = pieceList[CI].get(Knight, i);
        pin = detectPin(from);
        if (!isValid(pin)) {    //TODO this are 2^8 = 256 possible move combinations, depending on the 8 pieces[to] and from. replace by a table?
            to = from + 10;        // get whole board in 4 xmms, mask according to from, pshufb
            if (to < 64 && isKnightDistance(from, to) & !pieces[to])
                *list++ = (Move) {{ from, to }};
            to = from + 17;
            if (to < 64 && isKnightDistance(from, to) & !pieces[to])
                *list++ = (Move) {{ from, to }};
            to = from + 15;
            if (to < 64 && isKnightDistance(from, to) & !pieces[to])
                *list++ = (Move) {{ from, to }};
            to = from + 6;
            if (to < 64 && isKnightDistance(from, to) & !pieces[to])
                *list++ = (Move) {{ from, to }};
            to = from - 10;
            if (to < 64 && isKnightDistance(from, to) & !pieces[to])
                *list++ = (Move) {{ from, to }};
            to = from - 17;
            if (to < 64 && isKnightDistance(from, to) & !pieces[to])
                *list++ = (Move) {{ from, to }};
            to = from - 15;
            if (to < 64 && isKnightDistance(from, to) & !pieces[to])
                *list++ = (Move) {{ from, to }};
            to = from - 6;
            if (to < 64 && isKnightDistance(from, to) & !pieces[to])
                *list++ = (Move) {{ from, to }};
        }
    }

    /* castling
     * test the castling flags,
     * if there is enough free space besides the king (ATTACKLEN)
     * if the king-squares are not under attack, a test if we are in check is
     * not neccessary, we won't ever get here because of check evasion
     */
    if ( cep.castling.color[CI].q && length(0, pov^a1) == 4
    && !((shortAttack[EI][pov^c1] & attackMaskShort) |
         (shortAttack[EI][pov^d1] & attackMaskShort) |
         (longAttack[EI][pov^c1] & attackMaskLong) |
         (longAttack[EI][pov^d1] & attackMaskLong)))
    {
        ASSERT(pieces[pov^a1] == C*Rook);
        ASSERT(pieces[pov^e1] == C*King);
        *list++ = (Move) {{ pov^e1, pov^c1, 0, longCastling }};
    }

    if ( cep.castling.color[CI].k && length(0, pov^e1) == 3
    && !((shortAttack[EI][pov^f1] & attackMaskShort) |
         (shortAttack[EI][pov^g1] & attackMaskShort) |
         (longAttack[EI][pov^f1] & attackMaskLong) |
         (longAttack[EI][pov^g1] & attackMaskLong)))
    {
        ASSERT(pieces[pov^h1] == C*Rook);
        ASSERT(pieces[pov^e1] == C*King);
        *list++ = (Move) {{ pov^e1, pov^g1, 0, shortCastling }};
    }

    from = pieceList[CI].getKing();

    to = from + dirOffsets[0];
    if (length(0, from) && !(pieces[to] | (shortAttack[EI][to] & attackMaskShort) | (longAttack[EI][to] & attackMaskLong)))
        *list++ = (Move) {{ from, to }};
    to = from + dirOffsets[1];
    if (length(1, from) && !(pieces[to] | (shortAttack[EI][to] & attackMaskShort) | (longAttack[EI][to] & attackMaskLong)))
        *list++ = (Move) {{ from, to }};
    to = from + dirOffsets[2];
    if (length(2, from) && !(pieces[to] | (shortAttack[EI][to] & attackMaskShort) | (longAttack[EI][to] & attackMaskLong)))
        *list++ = (Move) {{ from, to }};
    to = from + dirOffsets[3];
    if (length(3, from) && !(pieces[to] | (shortAttack[EI][to] & attackMaskShort) | (longAttack[EI][to] & attackMaskLong)))
        *list++ = (Move) {{ from, to, }};
    to = from + dirOffsets[4];
    if (length(4, from) && !(pieces[to] | (shortAttack[EI][to] & attackMaskShort) | (longAttack[EI][to] & attackMaskLong)))
        *list++ = (Move) {{ from, to, }};
    to = from + dirOffsets[5];
    if (length(5, from) && !(pieces[to] | (shortAttack[EI][to] & attackMaskShort) | (longAttack[EI][to] & attackMaskLong)))
        *list++ = (Move) {{ from, to, }};
    to = from + dirOffsets[6];
    if (length(6, from) && !(pieces[to] | (shortAttack[EI][to] & attackMaskShort) | (longAttack[EI][to] & attackMaskLong)))
        *list++ = (Move) {{ from, to, }};
    to = from + dirOffsets[7];
    if (length(7, from) && !(pieces[to] | (shortAttack[EI][to] & attackMaskShort) | (longAttack[EI][to] & attackMaskLong)))
        *list++ = (Move) {{ from, to, }};

#endif    
    return list;
}

#endif /* GENERATEMOVES_TCC_ */
