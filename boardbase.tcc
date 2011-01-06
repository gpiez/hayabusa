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
#ifndef BOARDBASE_TCC_
#define BOARDBASE_TCC_

#include "boardbase.h"
#include "rootboard.h"

inline __v2di BoardBase::build13Attack(const unsigned sq) const {
    const __v16qi swap16 = { 7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8 };

    __v2di maskedDirs = _mm_set1_epi64x(occupied1) & mask13x[sq];
    __v2di reverse = _mm_shuffle_epi8(maskedDirs, swap16);
    maskedDirs -= doublebits[sq];
    reverse -= doublereverse[sq];
    reverse = _mm_shuffle_epi8(reverse, swap16);
    maskedDirs ^= reverse;
    maskedDirs &= mask13x[sq];
    return maskedDirs;
}

inline __v2di BoardBase::build02Attack(const unsigned sq) const {
    const __v2di b02 = _mm_set_epi64x(0xff, 0x0101010101010101); // border

    __v2di maskedDir02 = (_mm_set1_epi64x(occupied1)|b02) & mask02[sq].b;
    uint64_t d0 = _mm_cvtsi128_si64x(maskedDir02);
    uint64_t d2 = _mm_cvtsi128_si64x(_mm_unpackhi_epi64(maskedDir02,maskedDir02));
    d0 <<= 63-sq;
    d2 <<= 63-sq;
    uint64_t lo = rol(6, sq + __bsrq(d0));
    uint64_t hi = rol(6, sq + __bsrq(d2));
    maskedDir02 ^= maskedDir02 - _mm_set_epi64x(hi, lo);
    maskedDir02 &= mask02[sq].x;
    return maskedDir02;
//    return _mm_set1_epi64x(att0Tab[attLen0[sq]][sq], att2Tab[attLen2[sq]][sq]);
}

template<Colors C>
void BoardBase::buildAttacks() {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    uint64_t p, a;

    p = getPieces<C, Knight>();
    a = 0;
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        a |= knightAttacks[sq];
    }
    getAttacks<C, Knight>() = a;
    getAttacks<C, All>() = a;

    MoveTemplateB* bs = bsingle[CI];
    __v2di dir13 = _mm_set1_epi64x(0);
    p = getPieces<C, Bishop>();
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        __v2di a13 = build13Attack(sq);
        bs->move = Move(sq, 0, Bishop);
        bs->d13 = a13;
        bs++;
        dir13 |= a13;
    }
    bs->move = Move(0,0,0);
    a = fold(dir13);
    getAttacks<C, Bishop>() = a;
    getAttacks<C, All>() |= a;

    MoveTemplateR* rs = rsingle[CI];
    __v2di dir02 = _mm_set1_epi64x(0);
    p = getPieces<C, Rook>();
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        __v2di a02 = build02Attack(sq);
        rs->move = Move(sq, 0, Rook);
        rs->d02 = a02;
        rs++;
        dir02 |= a02;
    }
    rs->move = Move(0,0,0);
    a = fold(dir02);
    getAttacks<C, Rook>() = a;
    getAttacks<C, All>() |= a;

    MoveTemplateQ* qs = qsingle[CI];
    p = getPieces<C, Queen>();
    a = 0;
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        __v2di a13 = build13Attack(sq);
        __v2di a02 = build02Attack(sq);
        qs->move = Move(sq, 0, Queen);
        qs->d02 = a02;
        qs->d13 = a13;
        qs++;
        dir02 |= a02;
        dir13 |= a13;
        a |= fold(a13 | a02);
    }
    qs->move = Move(0,0,0);
    getAttacks<C, Queen>() = a;
    getAttacks<C, All>() |= a;
    datt[CI].d02 = dir02;
    datt[CI].d13 = dir13;

    p = getPieces<C, Pawn>();
    if (C == White)
        getAttacks<C, All>() |= getAttacks<C, Pawn>() = (p << 7 & 0x7f7f7f7f7f7f7f7f) | (p << 9 & 0xfefefefefefefefe);
    else
        getAttacks<C, All>() |= getAttacks<C, Pawn>() = (p >> 9 & 0x7f7f7f7f7f7f7f7f) | (p >> 7 & 0xfefefefefefefefe);

}

/*
    Generate king attacks and pins on own king. Assuming after executing a move
    at least a exact eval is needed, all the parts need for the eval are
    calculated here too.
*/
template<Colors C>
void BoardBase::buildPins() {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };

    uint64_t p = getPieces<C, King>();
    uint64_t sq = bit(p);
    getAttacks<C, All>() |= getAttacks<C, King>() = kingAttacks[0][sq];

    __v2di maskedDir13 = build13Attack(sq);
    __v2di maskedDir02 = build02Attack(sq);
    kingIncoming[CI].d13 = maskedDir13;
    kingIncoming[CI].d02 = maskedDir02;
    __v2di dir02 = ~(maskedDir02 & datt[EI].d02);
    __v2di dir13 = ~(maskedDir13 & datt[EI].d13);
    __v2di dir20 = _mm_shuffle_epi32(dir02, 0x4e);
    __v2di dir31 = _mm_shuffle_epi32(dir13, 0x4e);
    dpins[CI].d02 = dir20 & dir13 & dir31;
    dpins[CI].d13 = dir31 & dir02 & dir20;
    __v2di pins2 = dpins[CI].d02 & dpins[CI].d13;
    pins[CI] = _mm_cvtsi128_si64(_mm_unpackhi_epi64(pins2, pins2))
         & _mm_cvtsi128_si64(pins2);
}

#endif
