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

inline uint64_t BoardBase::build13Attack(uint64_t flood1) const {
    
    uint64_t flood3 = flood1, flood5 = flood1, flood7 = flood1;
    
    const uint64_t e01 = ~occupied1 & ~file<'a'>();
    const uint64_t e03 = ~occupied1 & ~file<'h'>();
    // uint64_t e05 = ~occupied1 & ~file<'h'>();
    // uint64_t e07 = ~occupied1 & ~file<'a'>();
    const uint64_t e11 = e01 & e01 << 9;
    const uint64_t e13 = e03 & e03 << 7;
    const uint64_t e15 = e03 & e03 >> 9;
    const uint64_t e17 = e01 & e01 >> 7;
    
    const uint64_t e21 = e11 & e11 << 18;
    const uint64_t e23 = e13 & e13 << 14;
    const uint64_t e25 = e15 & e15 >> 18;
    const uint64_t e27 = e17 & e17 >> 14;
    
    flood1 |= flood1 << 9 & e01;
    flood3 |= flood3 << 7 & e03;
    flood5 |= flood5 >> 9 & e03;
    flood7 |= flood7 >> 7 & e01;
    
    flood1 |= flood1 << 18 & e11;
    flood3 |= flood3 << 14 & e13;
    flood5 |= flood5 >> 18 & e15;
    flood7 |= flood7 >> 14 & e17;
    
    flood1 |= flood1 << 36 & e21;
    flood3 |= flood3 << 28 & e23;
    flood5 |= flood5 >> 36 & e25;
    flood7 |= flood7 >> 28 & e27;
    
    return ((flood1 << 9 | flood7 >> 7 ) & ~file<'a'>())
         | ((flood3 << 7 | flood5 >> 9 ) & ~file<'h'>());
}

inline __v2di BoardBase::build13Attack(__v2di flood1) const {
    
    __v2di flood3 = flood1, flood5 = flood1, flood7 = flood1;
    
    const __v2di e01 = _mm_set1_epi64x(~occupied1 & ~file<'a'>());
    const __v2di e03 = _mm_set1_epi64x(~occupied1 & ~file<'h'>());
    // uint64_t e05 = ~occupied1 & ~file<'h'>();
    // uint64_t e07 = ~occupied1 & ~file<'a'>();
    const __v2di e11 = e01 & _mm_slli_epi64(e01, 9);
    const __v2di e13 = e03 & _mm_slli_epi64(e03, 7);
    const __v2di e15 = e03 & _mm_srli_epi64(e03, 9);
    const __v2di e17 = e01 & _mm_srli_epi64(e01, 7);
    
    const __v2di e21 = e11 & _mm_slli_epi64(e11, 18);
    const __v2di e23 = e13 & _mm_slli_epi64(e13, 14);
    const __v2di e25 = e15 & _mm_srli_epi64(e15, 18);
    const __v2di e27 = e17 & _mm_srli_epi64(e17, 14);
    
    flood1 |= _mm_slli_epi64(flood1, 9) & e01;
    flood3 |= _mm_slli_epi64(flood3, 7) & e03;
    flood5 |= _mm_srli_epi64(flood5, 9) & e03;
    flood7 |= _mm_srli_epi64(flood7, 7) & e01;
    
    flood1 |= _mm_slli_epi64(flood1, 18) & e11;
    flood3 |= _mm_slli_epi64(flood3, 14) & e13;
    flood5 |= _mm_srli_epi64(flood5, 18) & e15;
    flood7 |= _mm_srli_epi64(flood7, 14) & e17;
    
    flood1 |= _mm_slli_epi64(flood1, 36) & e21;
    flood3 |= _mm_slli_epi64(flood3, 28) & e23;
    flood5 |= _mm_srli_epi64(flood5, 36) & e25;
    flood7 |= _mm_srli_epi64(flood7, 28) & e27;
    
    return ((_mm_slli_epi64(flood1, 9) | _mm_srli_epi64(flood7, 7) ) & _mm_set1_epi64x(~file<'a'>()))
         | ((_mm_slli_epi64(flood3, 7) | _mm_srli_epi64(flood5, 9) ) & _mm_set1_epi64x(~file<'h'>()));
}

inline __v2di BoardBase::build02Attack(const unsigned sq) const {
    const __v2di b02 = _mm_set_epi64x(0xff, 0x0101010101010101); // border
    __v2di maskedDir02 = (_mm_set1_epi64x(occupied1)|b02) & mask02[sq].b;
    uint64_t d0 = _mm_cvtsi128_si64x(maskedDir02);
    uint64_t d2 = _mm_cvtsi128_si64x(_mm_unpackhi_epi64(maskedDir02,maskedDir02));
    d0 <<= 63-sq;
    d2 <<= 63-sq;
    uint64_t lo = rol(6, sq + bitr(d0));
    uint64_t hi = rol(6, sq + bitr(d2));
    maskedDir02 ^= maskedDir02 - _mm_set_epi64x(hi, lo);
    maskedDir02 &= mask02[sq].x;
    return maskedDir02;
//    return _mm_set1_epi64x(att0Tab[attLen0[sq]][sq], att2Tab[attLen2[sq]][sq]);
}

inline uint64_t BoardBase::build02Attack(uint64_t flood0) const {
    
    uint64_t flood2 = flood0, flood4 = flood0, flood6 = flood0;
    
    const uint64_t e00 =  ~occupied1 & ~file<'a'>();
    const uint64_t e02 =  ~occupied1;
    const uint64_t e04 =  ~occupied1 & ~file<'h'>();
    
    const uint64_t e10 = e00 & e00 << 1;
    const uint64_t e12 = e02 & e02 << 8;
    const uint64_t e14 = e04 & e04 >> 1;
    const uint64_t e16 = e02 & e02 >> 8;
    
    const uint64_t e20 = e10 & e10 << 2;
    const uint64_t e22 = e12 & e12 << 16;
    const uint64_t e24 = e14 & e14 >> 2;
    const uint64_t e26 = e16 & e16 >> 16;
    
    flood0 |= flood0 << 1 & e00;
    flood2 |= flood2 << 8 & e02;
    flood4 |= flood4 >> 1 & e04;
    flood6 |= flood6 >> 8 & e02;
    
    flood0 |= flood0 <<  2 & e10;
    flood2 |= flood2 << 16 & e12;
    flood4 |= flood4 >>  2 & e14;
    flood6 |= flood6 >> 16 & e16;
    
    flood0 |= flood0 <<  4 & e20;
    flood2 |= flood2 << 32 & e22;
    flood4 |= flood4 >>  4 & e24;
    flood6 |= flood6 >> 32 & e26;
    
    return (flood0 << 1 & ~file<'a'>()) | (flood4 >> 1 & ~file<'h'>())
        | flood2 << 8 | flood6 >> 8;
}

inline uint64_t BoardBase::buildNAttack(uint64_t n) const {
    uint64_t n1 = (n<<1 & ~file<'a'>())
                | (n>>1 & ~file<'h'>());
    uint64_t n2 = (n<<2 & ~(file<'a'>() | file<'b'>())) 
                | (n>>2 & ~(file<'g'>() | file<'h'>()));
    return n1<<16 | n1>>16 | n2<<8 | n2>>8;
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
        unsigned sq = bit(p);
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
        unsigned sq = bit(p);
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
        unsigned sq = bit(p);
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
    unsigned sq = bit(p);
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
