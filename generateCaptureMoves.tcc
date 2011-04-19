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
    const __v2di zero = _mm_set1_epi64x(0);
    const __v2di d2 = _mm_set1_epi64x(d);
    /*
     * Check for attacks from sliding pieces. Bishop/Rook/Queen attacks are
     * stored in single[], with a mov template matching the attack in move[]
     * Because the probability of a capture is low, it is profitable to check
     * if the target squares are attackable. If it is, there mus be at least one
     * piece of that kind in the single array. Skip every entry with no matching
     * attack squares and additionally we can skip the check for a valid move
     * template at the first target found.
     */
    if (getAttacks<C,Bishop>() & d) {
        const MoveTemplateB* bs = bsingle[CI];
        __v2di a13 = bs->d13;
        while (_mm_testz_si128(d2, a13))
            a13 = (++bs)->d13;
        Move m = bs->move;
        ASSERT(m.data);
        do {
            __v2di from2 = doublebits[m.from()];
            __v2di pin13 = from2 & dpins[CI].d13;
            pin13 = pcmpeqq(pin13, zero);
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
            m = (++bs)->move;
            a13 = bs->d13;
        } while (m.data);
    }

    if (getAttacks<C,Rook>() & d) {
        const MoveTemplateR* rs = rsingle[CI];
        __v2di a02 = rs->d02;
        while (_mm_testz_si128(d2, a02))
            a02 = (++rs)->d02;
        Move m = rs->move;
        ASSERT (m.data);
        do {
            __v2di from2 = doublebits[m.from()];
            __v2di pin02 = from2 & dpins[CI].d02;
            pin02 = pcmpeqq(pin02, zero);
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
            m = (++rs)->move;
            a02 = rs->d02;
        } while (m.data);
    }

    if (getAttacks<C,Queen>() & d) {
        const MoveTemplateQ* qs = qsingle[CI];
        Move m = qs->move;
        ASSERT(m.data);
        do {
            __v2di a02 = qs->d02;
            __v2di a13 = qs->d13;
            __v2di from2 = doublebits[m.from()];
            __v2di pin02 = from2 & dpins[CI].d02;
            __v2di pin13 = from2 & dpins[CI].d13;
            pin02 = pcmpeqq(pin02, zero);
            pin13 = pcmpeqq(pin13, zero);
            pin02 = ~pin02 & a02;
            pin13 = ~pin13 & a13;
            for (uint64_t a=fold((pin02|pin13) & d2); a; a&=a-1) {
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
                Move n;
                n.data = m.data + Move(0, bit(a), 0, cap).data;
                if ( val[cap] < val[Queen]
                     &&  (
                           a & -a & ( attPKBR | att2 )
                         )
                   ) *bad++ = n;
                else *--good = n;
            }
            m = (++qs)->move;
        } while (m.data);
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
//         if (C==White && from >= 48 || C==Black && from <16) {
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
//         if (C==White && from >= 48 || C==Black && from <16) {
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
                pin02 = pcmpeqq(pin02, zero);
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
                pin13 = pcmpeqq(pin13, zero);
//                xray13 = _mm_cmpeq_epi64(xray13, zero);
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

#endif /* GENERATECAPTUREMOVES_TCC_ */
