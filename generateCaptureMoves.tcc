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
        }
    }

    for(const MoveTemplateR* rs = rsingle[CI];; rs++) {
        Move m = rs->move;
        if (!m.data) break;
        __v2di a02 = rs->d02;
#ifdef __SSE4_1__
        if (!_mm_testz_si128(d2, a02)) {
#else
        if (fold(d2 & a02)) {
#endif
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
            pin02 = pcmpeqq(pin02, zero);
            pin13 = pcmpeqq(pin13, zero);
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

template<Colors C>
uint64_t ColoredBoard<C>::generateRookMates( uint64_t checkingMoves, uint64_t blockedEscapes, uint64_t undefended, uint64_t king, unsigned k) const {
    uint64_t rescape = getAttacks<-C,King>() & ~(occupied[EI] | blockedEscapes);
    rescape = ror(rescape, k-9);
    uint64_t rmate = 0;
    if (!(rescape & 0x30003))
        rmate |= king << 1 & ~file<'a'>();
    if (!(rescape & 0x00505))
        rmate |= king << 8;
    if (!(rescape & 0x60006))
        rmate |= king >> 1 & ~file<'h'>();
    if (!(rescape & 0x50500))
        rmate |= king >> 8;
    rmate &= checkingMoves & undefended;

    if (!(rescape & 0x70007))
    if (uint64_t p = kingIncoming[EI].d[0] & checkingMoves & ~getAttacks<-C,All>()) {
        uint64_t d04 = kingIncoming[EI].d[0]
                &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                    | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                    | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                    | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                    | (getAttacks<-C,King>()   & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>() | getAttacks<-C,Knight>()))
                    | (shift<-C*8>(getPieces<-C, Pawn>()) & getAttacks<-C,All>()));
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
    if (!(rescape & 0x50505))
    if (uint64_t p = kingIncoming[EI].d[1] & checkingMoves & ~getAttacks<-C,All>()) {
        uint64_t d26 = kingIncoming[EI].d[1] 
                &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                    | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                    | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                    | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                    | (getAttacks<-C,King>()   & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>() | getAttacks<-C,Knight>()))
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
    return rmate;
}

#pragma GCC diagnostic ignored "-Wreturn-type"
template<Colors C>
template<bool AbortOnFirst>
bool ColoredBoard<C>::generateMateMoves( Move** const good, Move** const bad ) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    const __v2di zero = _mm_set1_epi64x(0);

    uint64_t king = getPieces<-C,King>();
    unsigned k = bit(king);
    uint64_t attNotEnemyKing = getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Knight>() | getAttacks<-C,Queen>() | getAttacks<-C,Pawn>();
    uint64_t mate;
    /*
     * Rook mates: Generate all possible checking moves by rooks. Determine
     * which squares are attacked by any piece but a rook, because a rook can
     * not defend itself when mating. If there are two rooks, the other rook
     * can assist in blocking escape squares. X-ray attacks may assist in
     * defending the attacking rook.
     */
    if (uint64_t checkingMoves = getAttacks<C,Rook>() & ~occupied[CI] & (kingIncoming[EI].d[0] | kingIncoming[EI].d[1])) {
        uint64_t attNotSelfRook;
        uint64_t attNotRook = getAttacks<C,Queen>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>();
        if (rsingle[CI][1].move.data) {
            __v2di connectedRR = rsingle[CI][0].d02|rsingle[CI][1].d02;
            __v2di areRooksConnected = ~pcmpeqq(connectedRR & _mm_set1_epi64x(getPieces<C,Rook>()), zero);
            uint64_t rxray = fold(areRooksConnected & connectedRR);
            if (getAttacks<C,Queen>()) {
                uint64_t rook1 = getPieces<C,Rook>() & (getPieces<C,Rook>()-1);
                uint64_t rook0 = getPieces<C,Rook>() & ~rook1;
                __v2di isRook1Connected = ~pcmpeqq(qsingle[CI][0].d02 & _mm_set1_epi64x(rook1), zero);
                __v2di isRook0Connected = ~pcmpeqq(qsingle[CI][0].d02 & _mm_set1_epi64x(rook0), zero);
                rxray |= fold(isRook1Connected & rsingle[CI][1].d02) | fold(isRook0Connected & rsingle[CI][0].d02);
            }
            uint64_t r0attacks = fold(rsingle[CI][0].d02); //TODO check if precalculating is cheaper, this is needed in mobility too
            uint64_t r1attacks = fold(rsingle[CI][1].d02); //TODO check if precalculating is cheaper, this is needed in mobility too
            uint64_t checkR0Moves = r0attacks & ~occupied[CI] & (kingIncoming[EI].d[0] | kingIncoming[EI].d[1]);
            uint64_t checkR1Moves = r1attacks & ~occupied[CI] & (kingIncoming[EI].d[0] | kingIncoming[EI].d[1]);
            if (checkR0Moves && checkR1Moves) {
                mate = generateRookMates(checkR0Moves, attNotRook | r1attacks | rxray, (attNotRook|rxray) & ~attNotEnemyKing, king, k);
                mate|= generateRookMates(checkR1Moves, attNotRook | r0attacks | rxray, (attNotRook|rxray) & ~attNotEnemyKing, king, k);
                goto haveMate;
            } else if (checkR0Moves) {
                attNotSelfRook = attNotRook | r1attacks;
                checkingMoves = checkR0Moves;
            } else if (checkR1Moves) {
                attNotSelfRook = attNotRook | r0attacks;
                checkingMoves = checkR1Moves;
            }
            attNotRook |= rxray;
            attNotSelfRook |= rxray;
        } else {
            if (getAttacks<C,Queen>()) {
                __v2di isRook0Connected = ~pcmpeqq(qsingle[CI][0].d02 & _mm_set1_epi64x(getPieces<C,Rook>()), zero);
                uint64_t rxray = fold(isRook0Connected & rsingle[CI][0].d02);
                attNotRook |= rxray;
            }
            attNotSelfRook = attNotRook;
        }
        
        mate = generateRookMates(checkingMoves, attNotSelfRook, attNotRook & ~attNotEnemyKing, king, k);
haveMate:
        for(const MoveTemplateR* rs = rsingle[CI]; rs->move.data; rs++) {
            __v2di a02 = rs->d02;
            __v2di from2 = doublebits[rs->move.from()];
            __v2di pin02 = from2 & dpins[CI].d02;
            pin02 = pcmpeqq(pin02, zero);
            pin02 = ~pin02 & a02 /*& xray02*/;
            for (uint64_t a=fold(pin02) & mate; a; a&=a-1) {
                if (AbortOnFirst) return true;
                Move n;
                n.data = rs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                *--*good = n;
            }
            if (!AbortOnFirst) for (uint64_t a = fold(pin02) & checkingMoves & ~mate & ~attNotEnemyKing
                                               & ( ~getAttacks<-C,King>() | (getAttacks<-C,King>() & attNotRook)); a; a&=a-1) {
                Move n;
                n.data = rs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                *(*bad)++ = n;
            }
        }
    }

    // promotion moves, always assuming queen promotions are counted as queen moves.
    // pawn promotions may only move if not pinned at all.
    uint64_t pMoves = getPieces<C,Pawn>() & pins[CI];
    pMoves  = ( (shift<C*8  >(pMoves)                & ~occupied1)
              | (shift<C*8+1>(pMoves) & ~file<'a'>() &  occupied[EI])
              | (shift<C*8-1>(pMoves) & ~file<'h'>() &  occupied[EI])
              )
            & rank<8>();
    if (uint64_t checkingMoves  = (getAttacks<C,Queen>() | pMoves)
                                & ~occupied[CI]
                                & (kingIncoming[EI].d[0] | kingIncoming[EI].d[1] | kingIncoming[EI].d[2] | kingIncoming[EI].d[3])) {
        uint64_t attNotQueen = getAttacks<C,Rook>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>();
        if (getAttacks<C,Queen>()) {
            uint64_t rook1 = getPieces<C,Rook>() & (getPieces<C,Rook>()-1);
            uint64_t bishop1 = getPieces<C,Bishop>() & (getPieces<C,Bishop>()-1);
            uint64_t rook0 = getPieces<C,Rook>() & ~rook1;
            uint64_t bishop0 = getPieces<C,Bishop>() & ~bishop1;
            __v2di isQR1Connected = ~pcmpeqq(qsingle[CI][0].d02 & _mm_set1_epi64x(rook1), zero);
            __v2di isQB1Connected = ~pcmpeqq(qsingle[CI][0].d13 & _mm_set1_epi64x(bishop1), zero);
            __v2di isQR0Connected = ~pcmpeqq(qsingle[CI][0].d02 & _mm_set1_epi64x(rook0), zero);
            __v2di isQB0Connected = ~pcmpeqq(qsingle[CI][0].d13 & _mm_set1_epi64x(bishop0), zero);
            uint64_t xray = fold( (isQR1Connected & qsingle[CI][0].d02)
                                | (isQR0Connected & qsingle[CI][0].d02)
                                | (isQB1Connected & qsingle[CI][0].d13)
                                | (isQB0Connected & qsingle[CI][0].d13));
            attNotQueen |= xray;
        }
        uint64_t qescape = getAttacks<-C,King>() & ~(occupied[EI] | attNotQueen);
        qescape = ror(qescape, k-9);
        uint64_t qmate = 0;
    
        // emptiness of near king squares in even directions
        // this is needed for the mate patterns, where the queen mates on an
        // adjacent diagonal square. if the rays to the other diagonal squares
        // are blocked, we need to test these squares too, otherwise
        // the queen will cover them
        uint64_t k0 = ror((king << 1) & ~occupied1 & ~file<'a'>(), k-9);
        uint64_t k2 = ror((king << 8) & ~occupied1, k-9);
        uint64_t k4 = ror((king >> 1) & ~occupied1 & ~file<'h'>(), k-9);
        uint64_t k6 = ror((king >> 8) & ~occupied1, k-9);
    
        if (!(qescape & 0x10106 & ~(k2>>1) & ~(k0>>8)))
            qmate |= king << 9 & ~file<'a'>();
        if (!(qescape & 0x40403 & ~(k2<<1) & ~(k4>>8)))
            qmate |= king << 7 & ~file<'h'>();
        if (!(qescape & 0x30404 & ~(k6<<1) & ~(k4<<8)))
            qmate |= king >> 9 & ~file<'h'>();
        if (!(qescape & 0x60101 & ~(k6>>1) & ~(k0<<8)))
            qmate |= king >> 7 & ~file<'a'>();
        if (!(qescape & 0x10001))
            qmate |= king << 1 & ~file<'a'>();
        if (!(qescape & 0x00005))
            qmate |= king << 8;
        if (!(qescape & 0x40004))
            qmate |= king >> 1 & ~file<'h'>();
        if (!(qescape & 0x50000))
            qmate |= king >> 8;
        qmate &= checkingMoves & attNotQueen & ~attNotEnemyKing;

        uint64_t qmate2 = 0;
        if (!(qescape & 0x30003))
            qmate2 |= king << 2 & ~attNotEnemyKing<<1 & ~file<'a'>() & ~file<'b'>();
        if (!(qescape & 0x00505))
            qmate2 |= king << 16 & ~attNotEnemyKing<<8;
        if (!(qescape & 0x60006))
            qmate2 |= king >> 2 & ~attNotEnemyKing>>1 & ~file<'h'>() & ~file<'g'>();
        if (!(qescape & 0x50500))
            qmate2 |= king >> 16 & ~attNotEnemyKing>>8;
        qmate2 &= checkingMoves & (kingIncoming[EI].d[0] | kingIncoming[EI].d[1]) & ~getAttacks<-C,All>();
        qmate |= qmate2;

        if (!(qescape & 0x70007))
        if (uint64_t p=kingIncoming[EI].d[0] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d04 = kingIncoming[EI].d[0] 
                &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                    | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                    | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                    | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                    | (getAttacks<-C,King>()   & attNotEnemyKing)
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
        if (!(qescape & 0x50505))
        if (uint64_t p=kingIncoming[EI].d[1] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d26 = kingIncoming[EI].d[1]
                &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                    | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                    | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                    | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                    | (getAttacks<-C,King>()   & attNotEnemyKing)
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

        if (!(qescape & 0x30506))
        if (uint64_t p=kingIncoming[EI].d[2] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d15 = kingIncoming[EI].d[2]
                &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                    | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                    | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                    | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                    | (getAttacks<-C,King>()   & attNotEnemyKing)
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

        if (!(qescape & 0x60503))
        if (uint64_t p=kingIncoming[EI].d[3] & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d37 = kingIncoming[EI].d[3]
                &   ( (getAttacks<-C,Rook>()   &  getAttacks<-C,Pawn>())
                    | (getAttacks<-C,Bishop>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>()))
                    | (getAttacks<-C,Queen>()  & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>()))
                    | (getAttacks<-C,Knight>() & (getAttacks<-C,Pawn>() | getAttacks<-C,Rook>() | getAttacks<-C,Bishop>() | getAttacks<-C,Queen>()))
                    | (getAttacks<-C,King>()   & attNotEnemyKing)
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

//         if (qmate & getAttacks<C,Queen>()) {
            for(const MoveTemplateQ* qs = qsingle[CI]; qs->move.data; qs++) {
                __v2di a02 = qs->d02;
                __v2di a13 = qs->d13;
                __v2di from2 = doublebits[qs->move.from()];
                __v2di pin02 = from2 & dpins[CI].d02;
                __v2di pin13 = from2 & dpins[CI].d13;
                pin02 = pcmpeqq(pin02, zero);
                pin13 = pcmpeqq(pin13, zero);
                pin02 = ~pin02 & a02;
                pin13 = ~pin13 & a13;
                for (uint64_t a=fold(pin02|pin13) & qmate; a; a&=a-1) {
                    if (AbortOnFirst) return true;
                    Move n;
                    ASSERT((a ^ (a & (a-1))) == (a & -a));
                    n.data = qs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                    *--*good = n;
                }
                if (!AbortOnFirst) for (uint64_t a = fold(pin02|pin13) & checkingMoves & ~qmate & ~attNotEnemyKing
                                                   & ( ~getAttacks<-C,King>() | (getAttacks<-C,King>() & attNotQueen)); a; a&=a-1) {
                    Move n;
                    ASSERT((a ^ (a & (a-1))) == (a & -a));
                    n.data = qs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                    *(*bad)++ = n;
                }
            }
//         }
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

        if (!(bescape0246 & 0x20502)) {
            uint64_t attNotBishop = getAttacks<C,Rook>() | getAttacks<C,Queen>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>();
            uint64_t bescape1357 = getAttacks<-C,King>() & ~(occupied[EI] | attNotBishop);
            bescape1357 = ror(bescape1357, k-9);
            uint64_t bmate = 0;
            if (!(bescape1357 & 0x10004)) {
                uint64_t b1 = kingIncoming[EI].d[2] & getAttacks<-C,King>();
                bmate = b1;
            }
            if (!(bescape1357 & 0x40001)) {
                uint64_t b1 = kingIncoming[EI].d[3] & getAttacks<-C,King>();
                bmate |= b1;
            }
            bmate &= checkingMoves & attNotBishop & ~attNotEnemyKing;

            if (!(bescape1357 & 0x10004))
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

            if (!(bescape1357 & 0x40001))
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

            for(const MoveTemplateB* bs = bsingle[CI]; bs->move.data; bs++) {
                __v2di a13 = bs->d13;
                __v2di from2 = doublebits[bs->move.from()];
                __v2di pin13 = from2 & dpins[CI].d13;
                __v2di xray13 = a13 & datt[EI].d13;     //TODO capture mate moves are wrongly recognized as moves on a x-ray protected square
                pin13 = pcmpeqq(pin13, zero);
                xray13 = pcmpeqq(xray13, zero);
                pin13 = ~pin13 & a13 & xray13;
                for (uint64_t a=fold(pin13) & bmate; a; a&=a-1) {
                    if (AbortOnFirst) return true;
                    Move n;
                    n.data = bs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                    *--*good = n;
                }
                if (!AbortOnFirst) for (uint64_t a = fold(pin13) & checkingMoves & ~bmate & ~attNotEnemyKing & 
                                                   ( ~getAttacks<-C,King>() | (getAttacks<-C,King>() & attNotBishop)); a; a&=a-1) {
                    Move n;
                    n.data = bs->move.data + Move(0, bit(a), 0, getPieceFromBit(a & -a)).data;
                    *(*bad)++ = n;
                }
                
            }
        }
    }

    if (uint64_t checkingMoves = getAttacks<C,Knight>() & ~occupied[CI] & knightAttacks[k] & ~getAttacks<-C,All>() ) {
        unsigned n0 = bit(getPieces<C,Knight>());
        unsigned n1 = bitr(getPieces<C,Knight>());
        uint64_t attNotKnight = getAttacks<C,Rook>() | getAttacks<C,Queen>() | getAttacks<C,Bishop>() |  getAttacks<C,Pawn>() | getAttacks<C,King>();
        if (n1 != n0) {
            uint64_t n0attacks = BoardBase::knightAttacks[n0]; 
            uint64_t n1attacks = BoardBase::knightAttacks[n1];
            uint64_t checkN0Moves = n0attacks & checkingMoves;
            uint64_t checkN1Moves = n1attacks & checkingMoves;
            if (checkN0Moves) mate = checkN0Moves & generateKnightMates(attNotKnight | n1attacks, king, k);
            else mate = 0;
            if (checkN1Moves) mate|= checkN1Moves & generateKnightMates(attNotKnight | n0attacks, king, k);
        } else
            mate = checkingMoves & generateKnightMates(attNotKnight, king, k);
          
            for (; mate; mate &= mate-1 ) {
                unsigned to = bit(mate);
                for ( uint64_t f = getPieces<C,Knight>() & pins[CI] & knightAttacks[to]; f; f &= f-1) {
                    if (AbortOnFirst) return true;
                    *--*good = Move(bit(f), to, Knight, getPieceFromBit(1ULL << to));
                }
        }
    }

    if (uint64_t checkingMoves = shift<C*8>(getPieces<C,Pawn>()) & ~occupied1 & shift<-C*8>((king>>1 & ~file<'h'>()) | (king<<1 & ~file<'a'>()))) {
            
        }
    if (AbortOnFirst) return false;
}
#pragma GCC diagnostic warning "-Wreturn-type"

template<Colors C>
uint64_t ColoredBoard<C>::generateKnightMates(uint64_t block, uint64_t king, unsigned k) const {
    uint64_t nblock0246 = occupied[EI] | block;
    uint64_t nblock1357 = occupied[EI] | getAttacks<C,All>();
    uint64_t nescape0246 = getAttacks<-C,King>() & ~nblock0246;
    uint64_t nescape1357 = getAttacks<-C,King>() & ~nblock1357;
    nescape0246 = ror(nescape0246, k-9);
    nescape1357 = ror(nescape1357, k-9);
    if (nescape0246 & 0x20502) return 0;
    uint64_t nmate = 0;
    if (!(nescape1357 & 0x50004)) {
        nmate |= king << 6 & ~file<'h'>() & ~file<'g'>();
        nmate |= king >> 15 & ~file<'a'>();
    }
    if (!(nescape1357 & 0x10005)) {
        nmate |= king << 15 & ~file<'h'>();
        nmate |= king >> 6 & ~file<'a'>() & ~file<'b'>();
    }
    if (!(nescape1357 & 0x50001)) {
        nmate |= king >> 17 & ~file<'h'>();
        nmate |= king << 10 & ~file<'a'>() & ~file<'b'>();
    }
    if (!(nescape1357 & 0x40005)) {
        nmate |= king >> 10 & ~file<'h'>() & ~file<'g'>();
        nmate |= king << 17 & ~file<'a'>();
    }
    return nmate;
}

#endif /* GENERATECAPTUREMOVES_TCC_ */
