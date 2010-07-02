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
#include <smmintrin.h>

#ifndef BITBOARD

extern uint8_t vec2dir[nSquares][nSquares];
extern uint8_t vec2diff[nSquares*2];

/* Returns -1 if the piece is not pinned, else the direction (0..7) of the attacking piece.
 * A piece is pinned, if it is on a check ray of the king and attacked by a sliding piece
 * in the same direction
 */
template<Colors C>
uint8_t ColoredBoard<C>::detectPin( unsigned int pos ) const {
    /*
     * Pin by a rook/queen: (own & checkR) && (opp & attackMaskRQ), possible dirs 04 26
     * Pin by a bish/queen: (own & checkB) && (opp & attackMaskB) | attackMaskQ), possible dirs 15 37
     */
    // This condition isn't neccessary, but very likely to be true, so in the
    // end the branch version ist faster than the branchless version below alone.
//    if ( attPinTable[longAttack[CI][pos]][longAttack[EI][pos]] ) {
    if (pins[CI].pins & (1ULL << pos)) {
        __v2di zero = _mm_set1_epi64x(0);
        __v2di n02 = _mm_set_epi64x(0, -2);
        __v2di n13 = _mm_set_epi64x(-1, -3);
        __v2di dir02pin = _mm_cmpeq_epi64(pins[CI].dir02 & doublebits[pos], zero);
        __v2di dir13pin = _mm_cmpeq_epi64(pins[CI].dir13 & doublebits[pos], zero);
        dir02pin = _mm_mul_epi32(dir02pin, n02);
        dir13pin = _mm_mul_epi32(dir13pin, n13);
        dir02pin |= dir13pin;
        return _mm_cvtsi128_si64x( _mm_unpackhi_epi64(dir02pin, dir02pin))
               + _mm_cvtsi128_si64x(dir02pin);
    }

    if ( !!(longAttack[CI][pos] & (checkKB|checkKR)) & !!(longAttack[EI][pos]&attackMaskLong) ) {
        uint8_t king = pieceList[CI].getKing();
        uint8_t dir2 = vec2pin[pos][king];
        return diaPinTable[dir2][attVec[dir2][pos]];
    }
    return ~0;
}
#endif
/*
 * Check if to is attacked by a pawn. If so, iterate through both possible from positions.
 * If a opposite ppiece is there, a capturing move is found. Calculate possible pins/checks
 * and insert it in list, if it is legal.
 */
#ifdef BITBOARD
template<Colors C>
template<bool UPromo>
void ColoredBoard<C>::generateTargetCapture(Move* &list, uint64_t d, const unsigned int cap) const {

    unsigned int from;

	// pawn capture to the absolute right, dir = 1 for white and 3 for black
	for(uint64_t p = shift<C*8+1>(getPieces<C,Pawn>()) & ~file<'a'>() & d & getPins<C,2-C>();p ;p &= p-1) {
		unsigned to = bit(p);
		from = to -C*8 -1;
		if (rank<8>() & (p&-p)) {
			*list++ = Move(from, to, Queen, cap, true);
			if (UPromo) {
				*list++ = Move(from, to, Knight, cap, true);
				*list++ = Move(from, to, Rook, cap, true);
				*list++ = Move(from, to, Bishop, cap, true);
			}
		} else
			*list++ = Move(from, to, Pawn, cap);
	}
	// left
	for (uint64_t p = shift<C*8-1>(getPieces<C,Pawn>()) & ~file<'h'>() & d & getPins<C,2+C>();p ;p &= p-1) {
		unsigned to = bit(p);
		from = to -C*8 +1;
		if (rank<8>() & (p&-p)) {
			*list++ = Move(from, to, Queen, cap, true);
			if (UPromo) {
				*list++ = Move(from, to, Knight, cap, true);
				*list++ = Move(from, to, Rook, cap, true);
				*list++ = Move(from, to, Bishop, cap, true);
			}
		} else
			*list++ = Move(from, to, Pawn, cap);
	}
    /*
     * Knight captures something. Can't move at all if pinned.
     */
    for ( uint64_t p = getAttacks<C,Knight>() & d; p; p &= p-1 ) {
        unsigned to = bit(p);
        for ( uint64_t f = getPieces<C,Knight>() & pins[CI].pins & knightAttacks[to]; f; f &= f-1)
        	*list++ = Move(bit(f), to, Knight, cap);
    }
    /* Check for attacks from sliding pieces. Bishop/Rook/Queen attacks are
     * stored in single[], with a mov template matching the attack in move[]
     */
    const __v2di* psingle = single[CI];
    const Move* pmove = moves[CI];
    const __v2di zero = _mm_set1_epi64x(0);
    const __v2di d2 = _mm_set1_epi64x(d);
    for(;;) {
        Move m = *pmove++;
        __v2di a02 = *psingle++;
        __v2di a13 = *psingle++;
        if (!m.data) break;
        if (!_mm_testz_si128(d2, (a02|a13))) {
            __v2di from2 = _mm_set1_epi64x(1ULL<<m.from());
            __v2di pin02 = from2 & pins[CI].dir02;
            __v2di pin13 = from2 & pins[CI].dir13;
            pin02 = _mm_cmpeq_epi64(pin02, zero);
            pin13 = _mm_cmpeq_epi64(pin13, zero);
            pin02 = ~pin02 & d2 & a02;
            pin13 = ~pin13 & d2 & a13;
            for (uint64_t a=fold(pin02|pin13); a; a&=a-1) {
            	Move n;
            	n.data = m.data + Move(0, bit(a), 0, cap).data;
                *list++ = n;
            }
        }
    }
    /*
     * King captures something.
     */
    for ( uint64_t p = getAttacks<C,King>() & d & ~getAttacks<-C,All>(); p; p &= p-1 ) {
        from = bit(getPieces<C,King>());
        *list++ = Move(from, bit(p), King, cap);
    }
}
#else
template<Colors C>
void ColoredBoard<C>::generateTargetCapture(Move* &list, const uint8_t to, const int8_t cap, const Attack a) const {
    uint8_t from;
    uint8_t pin;
    uint8_t dir;
    unsigned int nAttacks;
    uint64_t sources;
    uint32_t m = ((uint32_t)to << 8) + ((uint32_t)(uint8_t)cap << 16);
    /*
     * Pawn captures to the right. Only queen promotions.
     */
    if ( a.s.PR ) {
        dir = 1;
        from = to - C*9;
        ASSERT(pieces[from] == C*Pawn);
        pin = detectPin(from);
        if ( !isValid(pin) | (pin==dir) )
            list++->data = m + from + (isPromoRank(from) ? promoteQ << 24 : nothingSpecial << 24) ;

    }
    /*
     * Pawn captures to the left.
     */
    if ( a.s.PL ) {
        dir = 3;
        from = to - C*7;
        ASSERT(pieces[from] == C*Pawn);
        pin = detectPin(from);
        if ( !isValid(pin) | (pin==dir) )
            list++->data = m + from + (isPromoRank(from) ? promoteQ << 24 : nothingSpecial << 24) ;

    }
    /*
     * Knight captures something. Can't move at all if captured.
     */
    if (a.s.N) {
        nAttacks = a.s.N;
        sources = pieceList[CI].getAll<Knight>();
        unsigned int n = 0;
        while (true) {
            ASSERT(++n <= pieceList[CI][Knight]);
            from = sources;
            if (isKnightDistance(from, to)) {
                if (!isValid(detectPin(from)))
                    list++->data = m + from;
                if (!--nAttacks) break;
            }
            sources >>= 8;
        }
    }
    /*
     * King captures something. If it could castle, it is on the starting position and
     * so it can't be capturing an enemy castling-rook.
     */
    if ( a.s.K )
        if (!(attacks<EI>(to) & attackMask)) {
            from = pieceList[CI].getKing();
            list++->data = m + from;
        }

    if (a.l.B) {
        nAttacks = a.l.B;
        sources = pieceList[CI].getAll<Bishop>();
        unsigned int n = 0;
        while (true) {
            ASSERT(++n <= pieceList[CI][Bishop]);
            from = sources;
            dir = vec2dir[from][to];
            if (isValid(dir) & dir & 1 && length(dir, from)*dirOffsets[dir] + from == to) {
                pin = detectPin(from);
                if (!isValid(pin) | (pin==(dir&3)))
                    list++->data = m + from;
                if (!--nAttacks) break;
            }
            sources >>= 8;
        }
    }
    /*
     * Rook captures something.
     */
    if (a.l.R) {
        nAttacks = a.l.R;
        sources = pieceList[CI].getAll<Rook>();
        unsigned int n = 0;
        while (true) {
            ASSERT(++n <= pieceList[CI][Rook]);
            from = sources;
            dir = vec2dir[from][to];
            if (~dir & 1 && length(dir, from)*dirOffsets[dir] + from == to) {
                pin = detectPin(from);
                if (!isValid(pin) | (pin==(dir&3)))
                    list++->data = m + from;
                if (!--nAttacks) break;
            }
            sources >>= 8;
        }
    }

    if (a.l.Q) {
        nAttacks = a.l.Q;
        sources = pieceList[CI].getAll<Queen>();
        unsigned int n = 0;
        while (true) {
            ASSERT(++n <= pieceList[CI][Queen]);
            from = sources;
            dir = vec2dir[from][to];
            if (isValid(dir) && from + length(dir, from)*dirOffsets[dir] == to) {
                pin = detectPin(from);
                if (!isValid(pin) | (pin==(dir&3)))
                    list++->data = m + from;
                if (!--nAttacks) break;
            }
            sources >>= 8;
        }
    }
}
#endif
/*
 * Generate all pawn to queen promotions and capture moves for each kind of piece.
 * A pinned piece may only move on the line between king and pinning piece.
 * For a pawn on the last rank this can only happen if the pawn captures the pinning piece.
 */
template<Colors C>
template<bool UPromo>
Move* ColoredBoard<C>::generateCaptureMoves( Move* list) const {
#ifdef BITBOARD
    /*
     * Generate non-capturing queen promotions. Capturing promotions are handled in
     * generateTargetCaptures().
     */
    uint8_t to, sq;
    for (uint64_t p = getPieces<C,Pawn>() & rank<7>() & shift<-C*8>(~occupied1) & pins[CI].pins; p; p &= p-1) {
        sq = __builtin_ctzll(p);
        to = sq + C*dirOffsets[2];
        *list++ = Move(sq, to, Queen, 0, true);
        if (UPromo) {
            *list++ = Move(sq, to, Knight, 0, true);
            *list++ = Move(sq, to, Rook, 0, true);
            *list++ = Move(sq, to, Bishop, 0, true);
        }
    }

    if (uint64_t p = getPieces<-C,Queen>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(list, p, Queen);

    if (uint64_t p = getPieces<-C,Rook>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(list, p, Rook);

    if (uint64_t p = getPieces<-C,Bishop>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(list, p, Bishop);

    if (uint64_t p = getPieces<-C,Knight>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(list, p, Knight);

    if (uint64_t p = getPieces<-C,Pawn>() & getAttacks<C,All>())
        generateTargetCapture<UPromo>(list, p, Pawn);

#else
    uint8_t to;
    Attack a;
    /*
     * Generate non-capturing queen promotions. Capturing promotions are handled in
     * generateTargetCaptures().
     */
    for (unsigned int i = 0; i < pieceList[CI][Pawn]; ++i) {
        uint8_t pawn = pieceList[CI].getPawn(i);            //TODO load all 8 pieces at once and shift instead?
        if (isPromoRank(pawn)) {
            uint8_t pin=detectPin(pawn);

            to = pawn + C*dirOffsets[2];
            if ( !pieces[to] & !isValid(pin))
                *list++ = (Move) { {
                    pawn, to, 0, promoteQ
                }
            };
        }
    }
    /*
     * A queen can be captured.
     */
    for (unsigned int i = 0; i < pieceList[EI][Queen]; ++i) {
        to = pieceList[EI].get(Queen, i);
        a = attacks<CI>(to);
        if (a) generateTargetCapture(list, to, -C*Queen, a);
    }
    /*
     * A rook can be captured
     */
    for (unsigned int i = 0; i < pieceList[EI][Rook]; ++i) {
        to = pieceList[EI].get(Rook, i);
        a = attacks<CI>(to);
        if (a) generateTargetCapture(list, to, -C*Rook, a);
    }

    for (unsigned int i = 0; i < pieceList[EI][Bishop]; ++i) {
        to = pieceList[EI].get(Bishop, i);
        a = attacks<CI>(to);
        if (a) generateTargetCapture(list, to, -C*Bishop, a);
    }

    for (unsigned int i = 0; i < pieceList[EI][Knight]; ++i) {
        to = pieceList[EI].get(Knight, i);
        a = attacks<CI>(to);
        if (a) generateTargetCapture(list, to, -C*Knight, a);
    }

    for (unsigned int i = 0; i < pieceList[EI][Pawn]; ++i) {
        to = pieceList[EI].get(Pawn, i);
        a = attacks<CI>(to);
        if (a) generateTargetCapture(list, to, -C*Pawn, a);
    }
#endif
    return list;
}
#endif /* GENERATECAPTUREMOVES_TCC_ */
