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
#ifndef GENMATES_TCC_
#define GENMATES_TCC_

#include "coloredboard.h"
#include "emmintrin.h"

static const __m128i zero = _mm_set1_epi64x(0);

template<Colors C>
uint64_t ColoredBoard<C>::generateRookMates( uint64_t checkingMoves, uint64_t blockedEscapes, uint64_t undefended, uint64_t king, unsigned k) const {
    uint64_t rescape = getAttacks<-C,King>() & ~(occupied[EI] | blockedEscapes);
    rescape = ror(rescape, k-9);
#if 0    
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
#else
    uint64_t rmate = checkingMoves & undefended & getAttacks<-C,King>();
#ifdef MYDEBUG
        mobStat[CI][Pawn][0][popcount(rmate)]++;
#endif
    if (rmate) {
        if (rescape & 0x30003)
            rmate &= ~(king<<1);
        if (rescape & 0x00505)
            rmate &= ~(king<<8);
        if (rescape & 0x60006)
            rmate &= ~(king>>1);
        if (rescape & 0x50500)
            rmate &= ~(king>>8);
    }
#endif

    if (!(rescape & 0x70007))
    if (uint64_t p = kingIncoming[EI].d0 & checkingMoves & ~getAttacks<-C,All>()) {
        uint64_t d04 = kingIncoming[EI].d0
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
    if (uint64_t p = kingIncoming[EI].d2 & checkingMoves & ~getAttacks<-C,All>()) {
        uint64_t d26 = kingIncoming[EI].d2
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
template<bool AbortOnFirst, typename R>
R ColoredBoard<C>::generateMateMoves( Move** const good, Move** const bad ) const {
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
    if (uint64_t checkingMoves = getAttacks<C,Rook>() & ~occupied[CI] & (kingIncoming[EI].d0 | kingIncoming[EI].d2)) {
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
            uint64_t checkR0Moves = r0attacks & ~occupied[CI] & (kingIncoming[EI].d0 | kingIncoming[EI].d2);
            uint64_t checkR1Moves = r1attacks & ~occupied[CI] & (kingIncoming[EI].d0 | kingIncoming[EI].d2);
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
                if (AbortOnFirst) return (R)true;
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
                                & (kingIncoming[EI].d0 | kingIncoming[EI].d1 | kingIncoming[EI].d2 | kingIncoming[EI].d3)) {
        uint64_t attNotQueen = getAttacks<C,Rook>() | getAttacks<C,Bishop>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>();
        uint64_t xprot=0;
        if (getAttacks<C,Queen>()) {
            uint64_t erook1 = getPieces<-C,Rook>() & (getPieces<-C,Rook>()-1);
            uint64_t ebishop1 = getPieces<-C,Bishop>() & (getPieces<-C,Bishop>()-1);
            uint64_t erook0 = getPieces<-C,Rook>() & ~erook1;
            uint64_t ebishop0 = getPieces<-C,Bishop>() & ~ebishop1;
            __v2di isQER1Connected = ~pcmpeqq(qsingle[CI][0].d02 & _mm_set1_epi64x(erook1), zero);
            __v2di isQEB1Connected = ~pcmpeqq(qsingle[CI][0].d13 & _mm_set1_epi64x(ebishop1), zero);
            __v2di isQER0Connected = ~pcmpeqq(qsingle[CI][0].d02 & _mm_set1_epi64x(erook0), zero);
            __v2di isQEB0Connected = ~pcmpeqq(qsingle[CI][0].d13 & _mm_set1_epi64x(ebishop0), zero);
            __v2di isQEQConnected02  = ~pcmpeqq(qsingle[CI][0].d02 & _mm_set1_epi64x(getPieces<-C,Queen>()), zero);
            __v2di isQEQConnected13  = ~pcmpeqq(qsingle[CI][0].d13 & _mm_set1_epi64x(getPieces<-C,Queen>()), zero);
            xprot = fold( (isQER1Connected & qsingle[CI][0].d02)
                                | (isQER0Connected & qsingle[CI][0].d02)
                                | (isQEQConnected02 & qsingle[CI][0].d02)
                                | (isQEB1Connected & qsingle[CI][0].d13)
                                | (isQEB0Connected & qsingle[CI][0].d13)
                                | (isQEQConnected13 & qsingle[CI][0].d13));
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
        uint64_t qmate = checkingMoves & attNotQueen & ~(attNotEnemyKing | xprot) & getAttacks<-C,King>();
#ifdef MYDEBUG
        mobStat[CI][King][0][popcount(qmate)]++;
#endif
        if (qmate) {
            // emptiness of near king squares in even directions
            // this is needed for the mate patterns, where the queen mates on an
            // adjacent diagonal square. if the rays to the other diagonal squares
            // are blocked, we need to test these squares too, otherwise
            // the queen will cover them

            uint64_t qsingle = 0;

            if (qmate & king<<9) {
                uint64_t k0 = ror((king << 1) & ~occupied1, k-9);
                uint64_t k2 = ror((king << 8) & ~occupied1, k-9);
                uint64_t qe1 = qescape & 0x10106 & ~(k2>>1) & ~(k0>>8);
                if (qe1)
                    qmate &= ~(king<<9);
                else {
                    uint64_t qse1 = qe1 & -qe1;
                    if (qse1 == qe1) qsingle |= qe1;
                }
            }

            if (qmate & king<<7) {
                uint64_t k2 = ror((king << 8) & ~occupied1, k-9);
                uint64_t k4 = ror((king >> 1) & ~occupied1, k-9);
                uint64_t qe3 = qescape & 0x40403 & ~(k2<<1) & ~(k4>>8);
                if (qe3)
                    qmate &= ~(king<<7);
                else {
                    uint64_t qse3 = qe3 & -qe3;
                    if (qse3 == qe3) qsingle |= qe3;
                }
            }

            if (qmate & king>>9) {
                uint64_t k4 = ror((king >> 1) & ~occupied1, k-9);
                uint64_t k6 = ror((king >> 8) & ~occupied1, k-9);
                uint64_t qe5 = qescape & 0x30404 & ~(k6<<1) & ~(k4<<8);
                if (qe5)
                    qmate &= ~(king>>9);
                else {
                    uint64_t qse5 = qe5 & -qe5;
                    if (qse5 == qe5) qsingle |= qe5;
                }
            }

            if (qmate & king>>7) {
                uint64_t k0 = ror((king << 1) & ~occupied1, k-9);
                uint64_t k6 = ror((king >> 8) & ~occupied1, k-9);
                uint64_t qe7 = qescape & 0x60101 & ~(k6>>1) & ~(k0<<8);
                if (qe7)
                    qmate &= ~(king>>7);
                else {
                    uint64_t qse7 = qe7 & -qe7;
                    if (qse7 == qe7) qsingle |= qe7;
                }
            }

            if (qmate & king<<1) {
                uint64_t qe0 = qescape & 0x10001;
                if (qe0)
                    qmate &= ~(king<<1);
                else {
                    uint64_t qse0 = qe0 & -qe0;
                    if (qse0 == qe0) qsingle |= qe0;
                }
            }

            if (qmate & king<<8) {
                uint64_t qe2 = qescape & 0x00005;
                if (qe2)
                    qmate &= ~(king<<8);
                else {
                    uint64_t qse2 = qe2 & -qe2;
                    if (qse2 == qe2) qsingle |= qe2;
                }
            }

            if (qmate & king>>1) {
                uint64_t qe4 = qescape & 0x40004;
                if (qe4)
                    qmate &= ~(king>>1);
                else {
                    uint64_t qse4 = qe4 & -qe4;
                    if (qse4 == qe4) qsingle |= qe4;
                }
            }

            if (qmate & king>>8) {
                uint64_t qe6 = qescape & 0x50000;
                if (qe6)
                    qmate &= ~(king>>8);
                else {
                    uint64_t qse6 = qe6 & -qe6;
                    if (qse6 == qe6) qsingle |= qe6;
                }
            }
        }

        uint64_t qmate2 = 0;
        if (!(qescape & 0x30003))
            qmate2 |= king << 2 & ~attNotEnemyKing<<1 & ~file<'a'>() & ~file<'b'>();
        if (!(qescape & 0x00505))
            qmate2 |= king << 16 & ~attNotEnemyKing<<8;
        if (!(qescape & 0x60006))
            qmate2 |= king >> 2 & ~attNotEnemyKing>>1 & ~file<'h'>() & ~file<'g'>();
        if (!(qescape & 0x50500))
            qmate2 |= king >> 16 & ~attNotEnemyKing>>8;
        qmate2 &= checkingMoves & (kingIncoming[EI].d0 | kingIncoming[EI].d2) & ~(getAttacks<-C,All>()|xprot);
        qmate |= qmate2;

        if (!(qescape & 0x70007))
        if (uint64_t p=kingIncoming[EI].d0 & checkingMoves & ~(getAttacks<-C,All>()|xprot)) {
            uint64_t d04 = kingIncoming[EI].d0
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
        if (uint64_t p=kingIncoming[EI].d2 & checkingMoves & ~(getAttacks<-C,All>()|xprot)) {
            uint64_t d26 = kingIncoming[EI].d2
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
        if (uint64_t p=kingIncoming[EI].d1 & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d15 = kingIncoming[EI].d1
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
        if (uint64_t p=kingIncoming[EI].d3 & checkingMoves & ~getAttacks<-C,All>()) {
            uint64_t d37 = kingIncoming[EI].d3
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
                    if (AbortOnFirst) return (R)true;
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
            if (AbortOnFirst) return (R)true;
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

    if (uint64_t checkingMoves = getAttacks<C,Bishop>() & ~occupied[CI] & (kingIncoming[EI].d1 | kingIncoming[EI].d3)) {
        uint64_t bescape0246 = getAttacks<-C,King>() & ~(occupied[EI] | getAttacks<C,All>());
        bescape0246 = ror(bescape0246, k-9);

        if (!(bescape0246 & 0x20502)) {
            uint64_t attNotBishop = getAttacks<C,Rook>() | getAttacks<C,Queen>() | getAttacks<C,Knight>() | getAttacks<C,Pawn>() | getAttacks<C,King>();
            uint64_t bescape1357 = getAttacks<-C,King>() & ~(occupied[EI] | attNotBishop);
            bescape1357 = ror(bescape1357, k-9);
            uint64_t bmate = checkingMoves & attNotBishop & ~attNotEnemyKing & getAttacks<-C,King>();
            if (bescape1357 & 0x10004) {
                bmate &= ~kingIncoming[EI].d1;
            }
            if (bescape1357 & 0x40001) {
                bmate &= ~kingIncoming[EI].d3;
            }
#ifdef MYDEBUG
            uint64_t bmate2 = 0;
            if (!(bescape1357 & 0x10004)) {
                uint64_t b1 = kingIncoming[EI].d1 & getAttacks<-C,King>();
                bmate2 = b1;
            }
            if (!(bescape1357 & 0x40001)) {
                uint64_t b1 = kingIncoming[EI].d3 & getAttacks<-C,King>();
                bmate2 |= b1;
            }
            bmate2 &= checkingMoves & attNotBishop & ~attNotEnemyKing;
            ASSERT(bmate == bmate2);
#endif
            if (!(bescape1357 & 0x10004))
            if (uint64_t p=kingIncoming[EI].d1 & checkingMoves & ~getAttacks<-C,All>()) {
                uint64_t d15 = kingIncoming[EI].d1
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
            if (uint64_t p=kingIncoming[EI].d3 & checkingMoves & ~getAttacks<-C,All>()) {
                uint64_t d37 = kingIncoming[EI].d3
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
                    if (AbortOnFirst) return (R)true;
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
                    if (AbortOnFirst) return (R)true;
                    *--*good = Move(bit(f), to, Knight, getPieceFromBit(1ULL << to));
                }
        }
    }

/*    if (uint64_t checkingMoves = shift<C*8>(getPieces<C,Pawn>()) & ~occupied1 & shift<-C*8>((king>>1 & ~file<'h'>()) | (king<<1 & ~file<'a'>()))) {

        }*/
    return (R)false;
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

#if 0
                    /*
                     * Bad pruning test
                     */
/*                    {
                        const B beta0(current.v - 149*C);
                        const A alpha0(current.v - 150*C);
                        unsigned newDepth = depth-4;
                        ASSERT(depth > 4);
                        value.v = search4<(Colors)-C, P>(b, *i, newDepth, beta0, alpha0, ply+1, ExtNot, hasMaxDepth, nattack NODE);
                        if (value <= alpha0.v) continue;
                    }*/
                }
                /*
                 * Reduced search
                 */
                if (Options::reduction
                    && i-good > 3
                    && depth > 3
                    && nattack <= oldattack
                    && (*i).capture() == 0
                    && current.v != -infinity*C) {

                    const B beta0(current.v + C);
                    unsigned newDepth = depth-(reduction+1);
                    ASSERT(depth > (unsigned)reduction+1);
                    value.v = search4<(Colors)-C, P>(b, *i, newDepth, beta0, current, ply+1, ExtNot, hasMaxDepth, nattack NODE);
                    if (value <= current.v) continue; //TODO second part could be earlier
                }
#endif

#endif