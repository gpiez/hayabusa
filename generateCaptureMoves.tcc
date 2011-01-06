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

static const int val[nPieces+1] = { 0, 5, 3, 9, 3, 1, 100 };

template<Colors C>
template<bool UPromo>
void ColoredBoard<C>::generateTargetCapture(Move* &good, Move* &bad, uint64_t d, unsigned cap) const {
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
    const __v2di zero = _mm_set1_epi64x(0);
    const __v2di d2 = _mm_set1_epi64x(d);

    for(const MoveTemplateB* bs = bsingle[CI];; bs++) {
        Move m = bs->move;
        if (!m.data) break;
        __v2di a13 = bs->d13;
#ifdef __SSE4_1__
        if (!_mm_testz_si128(d2, a13)) {
#else
        if (fold(d2 & a13)) {
#endif
            __v2di from2 = doublebits[m.from()];
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
                n.data = m.data + Move(0, bit(a), 0, cap).data;
                if ( cap==Pawn
                     &&  (
                           a & -a & getAttacks<-C,Pawn>()
/*                           || a & -a & getAttacks<-C,All>() &
                                         ~( getAttacks<C,Rook>()
                                          | getAttacks<C,Queen>()
                                          | getAttacks<C,King>()
                                          | getAttacks<C,Knight>()
                                          | getAttacks<C,Pawn>()
                                          )*/
                         )
                   ) *bad++ = n;
                else *--good = n;
            }
        }
    }

    for(const MoveTemplateR* rs = rsingle[CI];; rs++) {
        Move m = rs->move;
        if (!m.data) break;
        __v2di a02 = rs->d02;
#ifdef __SSE4_1__
        if (!_mm_testz_si128(d2, a02)) {
            __v2di from2 = doublebits[m.from()];
            __v2di pin02 = from2 & dpins[CI].d02;
            pin02 = _mm_cmpeq_epi64(pin02, zero);
#else
        if (fold(d2 & a02)) {
            __v2di from2 = doublebits[m.from()];
            __v2di pin02 = from2 & dpins[CI].d02;
            pin02 = _mm_cmpeq_epi32(pin02, zero);
            __v2di pin02s = _mm_shuffle_epi32(pin02, 0b10110001);
            pin02 = pin02 & pin02s;
#endif
            pin02 = ~pin02 & d2 & a02;
            uint64_t attPKB = getAttacks<-C,Pawn>() | getAttacks<-C,Knight>() | getAttacks<-C,Bishop>();
            for (uint64_t a=fold(pin02); a; a&=a-1) {
                Move n;
                n.data = m.data + Move(0, bit(a), 0, cap).data;
                if ( val[cap] < val[Rook]
                     &&  (
                           a & -a & attPKB
/*                           || a & -a & getAttacks<-C,All>() &
                                         ~( getAttacks<C,Bishop>()
                                          | getAttacks<C,Queen>()
                                          | getAttacks<C,King>()
                                          | getAttacks<C,Knight>()
                                          | getAttacks<C,Pawn>()
                                          )*/
                         )
                   ) *bad++ = n;
                else *--good = n;
            }
        }
    }

    for(const MoveTemplateQ* qs = qsingle[CI];; qs++) {
        Move m = qs->move;
        if (!m.data) break;
        __v2di a02 = qs->d02;
        __v2di a13 = qs->d13;
#ifdef __SSE4_1__
        if (!_mm_testz_si128(d2, (a02|a13))) {
#else
        if (fold(d2 & (a02|a13))) {
#endif
            __v2di from2 = doublebits[m.from()];
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
            uint64_t attPKBR = getAttacks<-C,Pawn>()
                                    | getAttacks<-C,Knight>()
                                    | getAttacks<-C,Bishop>()
                                    | getAttacks<-C,Rook>();
            uint64_t att2 = getAttacks<-C,All>() &
                                         ~( getAttacks<C,Rook>()
                                          | getAttacks<C,Bishop>()
                                          | getAttacks<C,King>()
                                          | getAttacks<C,Knight>()
                                          | getAttacks<C,Pawn>()
                                          );
            for (uint64_t a=fold((pin02|pin13) & d2); a; a&=a-1) {
                Move n;
                n.data = m.data + Move(0, bit(a), 0, cap).data;
                if ( val[cap] < val[Queen]
                     &&  (
                           a & -a & ( attPKBR | att2 )
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
            if (cap==Pawn && (p & -p & getAttacks<-C,Pawn>()
                // there are no x-ray defenses for capturing knights, so capturing
                // a defended pawn is very likely bad
                             || p & -p & getAttacks<-C,All>()& ~( getAttacks<C,Rook>()
                                                                | getAttacks<C,Bishop>()
                                                                | getAttacks<C,Queen>()
                                                                | getAttacks<C,Pawn>()
                                                                | getAttacks<C,King>()
                                                                )
                               )
               ) *bad++ = Move(bit(f), to, Knight, cap);
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
bool ColoredBoard<C>::generateSkewers( Move** const good ) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    if (!qsingle[EI][0].move.data) return false;
    const __v2di zero = _mm_set1_epi64x(0);
    uint64_t king = getPieces<-C,King>();
    unsigned k = bit(king);
    bool found = false;
    if (uint64_t forks = getAttacks<C,Rook>() & getAttacks<-C,Queen>()) {
        if (getAttacks<C,Rook>() & getPieces<-C,Queen>())
            forks = 0;
        else if(kingIncoming[EI].d[0] & getPieces<-C,Queen>()) {
            forks &= (((const uint64_t*)&mask02[k].x)[0]
                & ~occupied[CI]
                & ~(getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Knight>() | getAttacks<-C,Pawn>())
                & (getAttacks<C,Bishop>() | getAttacks<C,Queen>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>())
                );
        } else if (kingIncoming[EI].d[1] & getPieces<-C,Queen>()) {
            forks &= (((const uint64_t*)&mask02[k].x)[1]
                & ~occupied[CI]
                & ~(getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Knight>() | getAttacks<-C,Pawn>())
                & (getAttacks<C,Bishop>() | getAttacks<C,Queen>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>())
                );
        } else
            forks=0;
        if (forks) {
            for(const MoveTemplateR* rs = rsingle[CI]; rs->move.data; rs++) {
                __v2di a02 = rs->d02;
                __v2di from2 = doublebits[rs->move.from()];
                __v2di pin02 = from2 & dpins[CI].d02;
//                __v2di xray02 = a02 & datt[EI].d02;
    #ifdef __SSE4_1__
                pin02 = _mm_cmpeq_epi64(pin02, zero);
//                xray02 = _mm_cmpeq_epi64(xray02, zero);
    #else
                pin02 = _mm_cmpeq_epi32(pin02, zero);
                __v2di pin02s = _mm_shuffle_epi32(pin02, 0b10110001);
                pin02 = pin02 & pin02s;
//                xray02 = _mm_cmpeq_epi32(xray02, zero);
//                __v2di xray02s = _mm_shuffle_epi32(xray02, 0b10110001);
//                xray02 = xray02 & xray02s;
    #endif
                pin02 = ~pin02 & a02 /*& xray02*/;
                for (uint64_t a=fold(pin02) & forks; a; a&=a-1) {
                    Move n;
                    n.data = rs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                    if (good) {
                        *--*good = n;
                        found = true;
                    }
                    else return true;
                }
            }
        }
    }
    if (uint64_t skewers = getAttacks<C,Bishop>() & getAttacks<-C,Queen>()) {
        if (getAttacks<C,Bishop>() & getPieces<-C,Queen>())
            skewers = 0;
        else if (kingIncoming[EI].d[2] & getPieces<-C,Queen>()) {
            skewers &= (((const uint64_t*)&mask13x[k])[0]
                & ~occupied[CI]
                & ~(getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Knight>() | getAttacks<-C,Pawn>())
                & (getAttacks<C,Rook>() | getAttacks<C,Queen>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>())
                );
        } else if (kingIncoming[EI].d[3] & getPieces<-C,Queen>()) {
            skewers &= (((const uint64_t*)&mask13x[k])[1]
                & ~occupied[CI]
                & ~(getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Knight>() | getAttacks<-C,Pawn>())
                & (getAttacks<C,Rook>() | getAttacks<C,Queen>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>())
                );
        } else
            skewers = 0;
        if (skewers) {
            for(const MoveTemplateB* bs = bsingle[CI]; bs->move.data; bs++) {
                __v2di a13 = bs->d13;
                __v2di from2 = doublebits[bs->move.from()];
                __v2di pin13 = from2 & dpins[CI].d13;
//                __v2di xray13 = a13 & datt[EI].d13;     //TODO capture mate moves are wrongly recognized as moves on a x-ray protected square
    #ifdef __SSE4_1__
                pin13 = _mm_cmpeq_epi64(pin13, zero);
//                xray13 = _mm_cmpeq_epi64(xray13, zero);
    #else
                pin13 = _mm_cmpeq_epi32(pin13, zero);
                __v2di pin13s = _mm_shuffle_epi32(pin13, 0b10110001);
                pin13 = pin13 & pin13s;
//                xray13 = _mm_cmpeq_epi32(xray13, zero);
//                __v2di xray13s = _mm_shuffle_epi32(xray13, 0b10110001);
//                 xray13 = xray13 & xray13s;
    #endif
                pin13 = ~pin13 & a13 /*& xray13*/;
                for (uint64_t a=fold(pin13) & skewers; a; a&=a-1) {
                    Move n;
                    n.data = bs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                    if (good) {
                        *--*good = n;
                        found = true;
                    }
                    else return true;
                }
            }
        }
    }
    return found;
}

template<Colors C>
bool ColoredBoard<C>::generateForks( Move** const good ) const {
    return false;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
template<Colors C>
template<bool AbortOnFirst>
bool ColoredBoard<C>::generateMateMoves( Move** const good ) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    const __v2di zero = _mm_set1_epi64x(0);

    uint64_t king = getPieces<-C,King>();
    unsigned k = bit(king);
    uint64_t attNotEnemyKing = getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Knight>() | getAttacks<-C,Queen>() | getAttacks<-C,Pawn>();

    if (uint64_t checkingMoves = getAttacks<C,Rook>() & ~occupied[CI] & (kingIncoming[EI].d[0] | kingIncoming[EI].d[1])) {
        uint64_t attNotRook = getAttacks<C,Queen>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>();
        uint64_t rescape = getAttacks<-C,King>() & ~(occupied[EI] | attNotRook);
        rescape = ror(rescape, k-9);
        uint64_t rmate = 0;
        if ((rescape & 0x30003) == 0)
            rmate |= king << 1 & ~file<'a'>();
        if ((rescape & 0x00505) == 0)
            rmate |= king << 8;
        if ((rescape & 0x60006) == 0)
            rmate |= king >> 1 & ~file<'h'>();
        if ((rescape & 0x50500) == 0)
            rmate |= king >> 8;
        rmate &= checkingMoves & attNotRook & ~attNotEnemyKing;

        if ((rescape & 0x70007) == 0)
        if (uint64_t p = kingIncoming[EI].d[0] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d04 = kingIncoming[EI].d[0] & ( (getAttacks<-C,Rook>()   & (getAttacks<-C,Pawn>() | getAttacks<-C,Knight>() | getAttacks<-C,Queen>() | getAttacks<-C,Bishop>()))
                                                   | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Knight>() | getAttacks<-C,Queen>()))
                                                   | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Knight>()))
                                                   | (getAttacks<-C,Knight>() &  getAttacks<-C,Pawn>())
                                                   | (shift<-C*8>(getPieces<-C, Pawn>()) & getAttacks<-C,All>())
                                                   );
            uint64_t d0 = d04 >> k;
            d0   = d0<<1 | d0<<2;
            d0  |= d0<<2 | d0<<4;
            d0 <<= k;
            uint64_t d4 = d04 << (077-k);
            d4   = d4>>1 | d4>>2;
            d4  |= d4>>2 | d4>>4;
            d4 >>= 077-k;
            rmate |=  p & ~d0 & ~d4; //TODO check if capture mate moves should be generated here
        }
        if ((rescape & 0x50505) == 0)
        if (uint64_t p = kingIncoming[EI].d[1] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d26 = kingIncoming[EI].d[1] & ( (getAttacks<-C,Rook>()   & (getAttacks<-C,Pawn>() | getAttacks<-C,Knight>() | getAttacks<-C,Queen>() | getAttacks<-C,Bishop>()))
                                                   | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Knight>() | getAttacks<-C,Queen>()))
                                                   | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Knight>()))
                                                   | (getAttacks<-C,Knight>() &  getAttacks<-C,Pawn>())
                                                   );
            uint64_t d2 = d26 >> k;
            d2   = d2<<010 | d2<<020;
            d2  |= d2<<020 | d2<<040;
            d2 <<= k;
            uint64_t d6 = d26 << (077-k);
            d6   = d6>>010 | d6>>020;
            d6  |= d6>>020 | d6>>040;
            d6 >>= 077-k;
            rmate |= p & ~d2 & ~d6;
        }

        if (rmate) {
            for(const MoveTemplateR* rs = rsingle[CI]; rs->move.data; rs++) {
                __v2di a02 = rs->d02;
                __v2di from2 = doublebits[rs->move.from()];
                __v2di pin02 = from2 & dpins[CI].d02;
//                __v2di xray02 = a02 & datt[EI].d02;
    #ifdef __SSE4_1__
                pin02 = _mm_cmpeq_epi64(pin02, zero);
//                xray02 = _mm_cmpeq_epi64(xray02, zero);
    #else
                pin02 = _mm_cmpeq_epi32(pin02, zero);
                __v2di pin02s = _mm_shuffle_epi32(pin02, 0b10110001);
                pin02 = pin02 & pin02s;
//                xray02 = _mm_cmpeq_epi32(xray02, zero);
//                __v2di xray02s = _mm_shuffle_epi32(xray02, 0b10110001);
//                xray02 = xray02 & xray02s;
    #endif
                pin02 = ~pin02 & a02 /*& xray02*/;
                for (uint64_t a=fold(pin02) & rmate; a; a&=a-1) {
                    if (AbortOnFirst) return true;
                    Move n;
                    n.data = rs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                    *--*good = n;
                }
            }
        }
    }

    // promotion moves, always assuming queen promotions are counted as queen moves.
    // pawn promotions may only move if not pinned at all.
    uint64_t pMoves = getPieces<C,Pawn>() & pins[CI];
    pMoves  = ( (shift<C*8  >(pMoves)                & ~occupied[EI])
              | (shift<C*8+1>(pMoves) & ~file<'a'>() &  occupied[EI])
              | (shift<C*8-1>(pMoves) & ~file<'h'>() &  occupied[EI])
              )
            & rank<8>();
    if (uint64_t checkingMoves  = (getAttacks<C,Queen>() | pMoves)
                                & ~occupied[CI]
                                & (kingIncoming[EI].d[0] | kingIncoming[EI].d[1] | kingIncoming[EI].d[2] | kingIncoming[EI].d[3])) {
        uint64_t attNotQueen = getAttacks<C,Rook>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>();
        uint64_t qescape = getAttacks<-C,King>() & ~(occupied[EI] | attNotQueen);
        qescape = ror(qescape, k-9);
        uint64_t qmate = 0;
        if ((qescape & 0x10106) == 0)
            qmate |= king << 9 & ~file<'a'>();
        if ((qescape & 0x40403) == 0)
            qmate |= king << 7 & ~file<'h'>();
        if ((qescape & 0x30404) == 0)
            qmate |= king >> 9 & ~file<'h'>();
        if ((qescape & 0x60101) == 0)
            qmate |= king >> 7 & ~file<'a'>();
        if ((qescape & 0x10001) == 0)
            qmate |= king << 1 & ~file<'a'>();
        if ((qescape & 0x00005) == 0)
            qmate |= king << 8;
        if ((qescape & 0x40004) == 0)
            qmate |= king >> 1 & ~file<'h'>();
        if ((qescape & 0x50000) == 0)
            qmate |= king >> 8;
        qmate &= checkingMoves & attNotQueen & ~attNotEnemyKing;

        uint64_t qmate2 = 0;
        if ((qescape & 0x30003) == 0)
            qmate2 |= king << 2 & ~file<'a'>() & ~file<'b'>();
        if ((qescape & 0x00505) == 0)
            qmate2 |= king << 16;
        if ((qescape & 0x60006) == 0)
            qmate2 |= king >> 2 & ~file<'h'>() & ~file<'g'>();
        if ((qescape & 0x50500) == 0)
            qmate2 |= king >> 16;
        qmate2 &= checkingMoves & (kingIncoming[EI].d[0] | kingIncoming[EI].d[1]) & ~getAttacks<-C,All>();
        qmate |= qmate2;

        if ((qescape & 0x70007) == 0)
        if (uint64_t p=kingIncoming[EI].d[0] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d04 = kingIncoming[EI].d[0] & ( (getAttacks<-C,Rook>()   & (getAttacks<-C,Bishop>() | getAttacks<-C,Queen>() | getAttacks<-C,Knight>() | getAttacks<-C,Pawn>()))
                                                   | (getAttacks<-C,Bishop>() & (                          getAttacks<-C,Queen>() | getAttacks<-C,Knight>() | getAttacks<-C,Pawn>()))
                                                   | (getAttacks<-C,Queen>()  & (                                                   getAttacks<-C,Knight>() | getAttacks<-C,Pawn>()))
                                                   | (getAttacks<-C,Knight>() &                                                                               getAttacks<-C,Pawn>())
                                                   | (shift<-C*8>(getPieces<-C, Pawn>()) & getAttacks<-C,All>())
                                                   );
            uint64_t d0 = d04 >> k;
            d0   = d0<<1 | d0<<2;
            d0  |= d0<<2 | d0<<4;
            d0 <<= k;
            uint64_t d4 = d04 << (077-k);
            d4   = d4>>1 | d4>>2;
            d4  |= d4>>2 | d4>>4;
            d4 >>= 077-k;
            qmate |= p & ~d0 & ~d4;
        }
        if ((qescape & 0x50505) == 0)
        if (uint64_t p=kingIncoming[EI].d[1] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d26 = kingIncoming[EI].d[1]
                &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                    | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                    | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                    | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                    );
            uint64_t d2 = d26 >> k;
            d2   = d2<<010 | d2<<020;
            d2  |= d2<<020 | d2<<040;
            d2 <<= k;
            uint64_t d6 = d26 << (077-k);
            d6   = d6>>010 | d6>>020;
            d6  |= d6>>020 | d6>>040;
            d6 >>= 077-k;
            qmate |= p & ~d2 & ~d6;
        }

        if ((qescape & 0x30506) == 0)
        if (uint64_t p=kingIncoming[EI].d[2] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d15 = kingIncoming[EI].d[2]
                &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                    | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                    | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                    | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                    | (shift<-C*8>(getPieces<-C, Pawn>()) & getAttacks<-C,All>())
                    );
            uint64_t d1 = d15 >> k;
            d1   = d1<<011 | d1<<022;
            d1  |= d1<<022 | d1<<044;
            d1 <<= k;
            uint64_t d5 = d15 << (077-k);
            d5   = d5>>011 | d5>>022;
            d5  |= d5>>022 | d5>>044;
            d5 >>= 077-k;
            qmate |= p & ~d1 & ~d5;
        }

        if ((qescape & 0x60503) == 0)
        if (uint64_t p=kingIncoming[EI].d[3] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d37 = kingIncoming[EI].d[3]
                &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                    | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                    | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                    | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                    | (shift<-C*8>(getPieces<-C, Pawn>()) & getAttacks<-C,All>())
                    );
            uint64_t d3 = d37 >> k;
            d3   = d3<<7 | d3<<14;
            d3  |= d3<<14 | d3<<28;
            d3 <<= k;
            uint64_t d7 = d37 << (077-k);
            d7   = d7>>7 | d7>>14;
            d7  |= d7>>14 | d7>>28;
            d7 >>= 077-k;
            qmate |= p & ~d3 & ~d7;
        }

        if (qmate & getAttacks<C,Queen>()) {
            for(const MoveTemplateQ* qs = qsingle[CI]; qs->move.data; qs++) {
                __v2di a02 = qs->d02;
                __v2di a13 = qs->d13;
                __v2di from2 = doublebits[qs->move.from()];
                __v2di pin02 = from2 & dpins[CI].d02;
                __v2di pin13 = from2 & dpins[CI].d13;
//                __v2di xray02 = a02 & datt[EI].d02;
//                __v2di xray13 = a13 & datt[EI].d13;
    #ifdef __SSE4_1__
                pin02 = _mm_cmpeq_epi64(pin02, zero);
                pin13 = _mm_cmpeq_epi64(pin13, zero);
//                xray02 = _mm_cmpeq_epi64(xray02, zero);
//                xray13 = _mm_cmpeq_epi64(xray13, zero);
    #else
                pin02 = _mm_cmpeq_epi32(pin02, zero);
                pin13 = _mm_cmpeq_epi32(pin13, zero);
                __v2di pin02s = _mm_shuffle_epi32(pin02, 0b10110001);
                __v2di pin13s = _mm_shuffle_epi32(pin13, 0b10110001);
                pin02 = pin02 & pin02s;
                pin13 = pin13 & pin13s;
//                xray02 = _mm_cmpeq_epi32(xray02, zero);
//                xray13 = _mm_cmpeq_epi32(xray13, zero);
//                __v2di xray02s = _mm_shuffle_epi32(xray02, 0b10110001);
//                __v2di xray13s = _mm_shuffle_epi32(xray13, 0b10110001);
//                xray02 = xray02 & xray02s;
//                xray13 = xray13 & xray13s;
    #endif
                pin02 = ~pin02 & a02/* & xray02*/;
                pin13 = ~pin13 & a13/* & xray13*/;
                for (uint64_t a=fold(pin02|pin13) & qmate; a; a&=a-1) {
                    if (AbortOnFirst) return true;
                    Move n;
                    ASSERT((a ^ (a & (a-1))) == (a & -a));
                    n.data = qs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                    *--*good = n;
                }
            }
        }
        for (uint64_t p = qmate & pMoves; p; p &= p-1) {
            if (AbortOnFirst) return true;
            unsigned to = bit(p);
            if (p & -p & ~occupied1) {
                ASSERT((getPieces<C,Pawn>() & shift<-C*8>(p & -p) & pins[CI]));
                *--*good = Move(to-C*8, to, Queen, 0, true);
            } else {
                ASSERT(p & -p & occupied[EI]);
                if (getPieces<C,Pawn>() & shift<-C*8-1>(p & -p) & ~file<'h'>() & pins[CI]) {
                    *--*good = Move(to-C*8-1, to, Queen, getPieceFromBit(p & -p), true);
                }
                if (getPieces<C,Pawn>() & shift<-C*8+1>(p & -p) & ~file<'a'>() & pins[CI]) {
                    *--*good = Move(to-C*8+1, to, Queen, getPieceFromBit(p & -p), true);
                }
            }
        }
    }

    if (uint64_t checkingMoves = getAttacks<C,Bishop>() & ~occupied[CI] & (kingIncoming[EI].d[2] | kingIncoming[EI].d[3])) {
        uint64_t bescape0246 = getAttacks<-C,King>() & ~(occupied[EI] | getAttacks<C,All>());
        bescape0246 = ror(bescape0246, k-9);

        if ((bescape0246 & 0x20502) == 0) {
            uint64_t attNotBishop = getAttacks<C,Rook>() | getAttacks<C,Queen>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>();
            uint64_t bescape1357 = getAttacks<-C,King>() & ~(occupied[EI] | attNotBishop);
            bescape1357 = ror(bescape1357, k-9);
            uint64_t bmate = 0;
            if ((bescape1357 & 0x10004) == 0) {
                uint64_t b1 = kingIncoming[EI].d[2] & getAttacks<-C,King>();
                bmate = b1;
            }
            if ((bescape1357 & 0x40001) == 0) {
                uint64_t b1 = kingIncoming[EI].d[3] & getAttacks<-C,King>();
                bmate |= b1;
            }
            bmate &= checkingMoves & attNotBishop & ~attNotEnemyKing;

            if ((bescape1357 & 0x10004) == 0)
            if (uint64_t p=kingIncoming[EI].d[2] & checkingMoves & ~getAttacks<-C,All>()) {
                uint64_t d15 = kingIncoming[EI].d[2]
                    &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                        | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                        | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                        | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                        | (shift<-C*8>(getPieces<-C, Pawn>()) & getAttacks<-C,All>())
                        );
                uint64_t d1 = d15 >> k;
                d1   = d1<<011 | d1<<022;
                d1  |= d1<<022 | d1<<044;
                d1 <<= k;
                uint64_t d5 = d15 << (077-k);
                d5   = d5>>011 | d5>>022;
                d5  |= d5>>022 | d5>>044;
                d5 >>= 077-k;
                bmate |= p & ~d1 & ~d5;
            }

            if ((bescape1357 & 0x40001) == 0)
            if (uint64_t p=kingIncoming[EI].d[3] & checkingMoves & ~getAttacks<-C,All>()) {
                uint64_t d37 = kingIncoming[EI].d[3]
                    &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                        | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                        | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                        | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                        | (shift<-C*8>(getPieces<-C, Pawn>()) & getAttacks<-C,All>())
                        );
                uint64_t d3 = d37 >> k;
                d3   = d3<<7 | d3<<14;
                d3  |= d3<<14 | d3<<28;
                d3 <<= k;
                uint64_t d7 = d37 << (077-k);
                d7   = d7>>7 | d7>>14;
                d7  |= d7>>14 | d7>>28;
                d7 >>= 077-k;
                bmate |= p & ~d3 & ~d7;
            }

            if (bmate) {
                for(const MoveTemplateB* bs = bsingle[CI]; bs->move.data; bs++) {
                    __v2di a13 = bs->d13;
                    __v2di from2 = doublebits[bs->move.from()];
                    __v2di pin13 = from2 & dpins[CI].d13;
                    __v2di xray13 = a13 & datt[EI].d13;     //TODO capture mate moves are wrongly recognized as moves on a x-ray protected square
        #ifdef __SSE4_1__
                    pin13 = _mm_cmpeq_epi64(pin13, zero);
                    xray13 = _mm_cmpeq_epi64(xray13, zero);
        #else
                    pin13 = _mm_cmpeq_epi32(pin13, zero);
                    __v2di pin13s = _mm_shuffle_epi32(pin13, 0b10110001);
                    pin13 = pin13 & pin13s;
                    xray13 = _mm_cmpeq_epi32(xray13, zero);
                    __v2di xray13s = _mm_shuffle_epi32(xray13, 0b10110001);
                    xray13 = xray13 & xray13s;
        #endif
                    pin13 = ~pin13 & a13 & xray13;
                    for (uint64_t a=fold(pin13) & bmate; a; a&=a-1) {
                        if (AbortOnFirst) return true;
                        Move n;
                        n.data = bs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                        *--*good = n;
                    }
                }
            }
        }
    }

    if (uint64_t checkingMoves = getAttacks<C,Knight>() & ~occupied[CI] & knightAttacks[k] ) {
        uint64_t nblock0246 = occupied[EI] | getAttacks<C,Rook>() | getAttacks<C,Queen>() | getAttacks<C,Bishop>() |                          getAttacks<C,Pawn>() | getAttacks<C,King>();
        uint64_t nblock1357 = occupied[EI] | getAttacks<C,All>();
        uint64_t nescape0246 = getAttacks<-C,King>() & ~nblock0246;
        uint64_t nescape1357 = getAttacks<-C,King>() & ~nblock1357;
        nescape0246 = ror(nescape0246, k-9);
        nescape1357 = ror(nescape1357, k-9);
        if ((nescape0246 & 0x20502) == 0) {
            uint64_t nmate = 0;
            if ((nescape1357 & 0x50004) == 0) {
                nmate |= king << 6 & ~file<'h'>() & ~file<'g'>();
                nmate |= king >> 15 & ~file<'a'>();
            }
            if ((nescape1357 & 0x10005) == 0) {
                nmate |= king << 15 & ~file<'h'>();
                nmate |= king >> 6 & ~file<'a'>() & ~file<'b'>();
            }
            if ((nescape1357 & 0x50001) == 0) {
                nmate |= king >> 17 & ~file<'h'>();
                nmate |= king << 10 & ~file<'a'>() & ~file<'b'>();
            }
            if ((nescape1357 & 0x40005) == 0) {
                nmate |= king >> 10 & ~file<'h'>() & ~file<'g'>();
                nmate |= king << 17 & ~file<'a'>();
            }
            nmate &= checkingMoves & ~getAttacks<-C,All>() & ~occupied[CI];
            for (; nmate; nmate &= nmate-1 ) {
                unsigned to = bit(nmate);
                for ( uint64_t f = getPieces<C,Knight>() & pins[CI] & knightAttacks[to]; f; f &= f-1) {
                    if (AbortOnFirst) return true;
                    *--*good = Move(bit(f), to, Knight, getPieceFromBit(1ULL << to));
                }
            }
        }
    }
    if (AbortOnFirst) return false;
}
#pragma GCC diagnostic pop

#endif /* GENERATECAPTUREMOVES_TCC_ */
