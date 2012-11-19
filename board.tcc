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

#include "board.h"
#include "eval.h"
#include "bits.h"

#ifdef __SSE__
//bit reflected low and high nibbles
static const __v16qi swap16h = _mm_set_epi8( 0x0f, 0x07, 0x0b, 0x03, 0x0d, 0x05, 0x09, 0x01, 0x0e, 0x06, 0x0a, 0x02, 0x0c, 0x04, 0x08, 0x00 );
static const __v16qi swap16l = _mm_set_epi8( 0xf0, 0x70, 0xb0, 0x30, 0xd0, 0x50, 0x90, 0x10, 0xe0, 0x60, 0xa0, 0x20, 0xc0, 0x40, 0x80, 0x00 );

// mask lower nibble
static const __v2di nibmask= _mm_set1_epi8( 0xf );

//byte swap upper and lower 64 bit words
static const __v16qi swap16 = _mm_set_epi8( 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 );
#endif
inline __v2di Board::build13Attack(const unsigned sq) const {
#ifdef __SSSE3__

    __v2di maskedDirs = _mm_set1_epi64x(occupied1) & bits[sq].mask13;
    __v2di reverse = _mm_shuffle_epi8(maskedDirs, swap16);
    maskedDirs -= bits[sq].doublebits;
    reverse -= bits[sq].doublereverse;
    reverse = _mm_shuffle_epi8(reverse, swap16);
    maskedDirs ^= reverse;
    maskedDirs &= bits[sq].mask13;
    return maskedDirs;
#else
    uint64_t flood1 = 1ULL << sq;
    uint64_t flood3 = flood1, flood5 = flood1, flood7 = flood1;

    const uint64_t e01 = ~occupied1 & ~file<'a'>();
    const uint64_t e03 = ~occupied1 & ~file<'h'>();
    // uint64_t e05 = ~occupied1 & ~file<'h'>();
    // uint64_t e07 = ~occupied1 & ~file<'a'>();
    flood1 |= flood1 << 9 & e01;
    flood3 |= flood3 << 7 & e03;
    flood5 |= flood5 >> 9 & e03;
    flood7 |= flood7 >> 7 & e01;

    const uint64_t e11 = e01 & e01 << 9;
    const uint64_t e13 = e03 & e03 << 7;
    const uint64_t e15 = e03 & e03 >> 9;
    const uint64_t e17 = e01 & e01 >> 7;

    flood1 |= flood1 << 18 & e11;
    flood3 |= flood3 << 14 & e13;
    flood5 |= flood5 >> 18 & e15;
    flood7 |= flood7 >> 14 & e17;

    const uint64_t e21 = e11 & e11 << 18;
    const uint64_t e23 = e13 & e13 << 14;
    const uint64_t e25 = e15 & e15 >> 18;
    const uint64_t e27 = e17 & e17 >> 14;

    flood1 |= flood1 << 36 & e21;
    flood3 |= flood3 << 28 & e23;
    flood5 |= flood5 >> 36 & e25;
    flood7 |= flood7 >> 28 & e27;

#if defined(__SSE__)
    return _mm_set_epi64x( (flood7 >> 7 & ~file<'a'>()) | (flood3 << 7 & ~file<'h'>()),
                           (flood1 << 9 & ~file<'a'>()) | (flood5 >> 9 & ~file<'h'>()));
#else
    return (__v2di) { (flood1 << 9 & ~file<'a'>()) | (flood5 >> 9 & ~file<'h'>()),
    				  (flood7 >> 7 & ~file<'a'>()) | (flood3 << 7 & ~file<'h'>()) };
#endif
#endif
}
#if 0
inline uint64_t Board::build13Attack(uint64_t flood1) const {

    uint64_t flood3 = flood1, flood5 = flood1, flood7 = flood1;

    const uint64_t e01 = ~occupied1 & ~file<'a'>();
    const uint64_t e03 = ~occupied1 & ~file<'h'>();
    // uint64_t e05 = ~occupied1 & ~file<'h'>();
    // uint64_t e07 = ~occupied1 & ~file<'a'>();
    flood1 |= flood1 << 9 & e01;
    flood3 |= flood3 << 7 & e03;
    flood5 |= flood5 >> 9 & e03;
    flood7 |= flood7 >> 7 & e01;

    const uint64_t e11 = e01 & e01 << 9;
    const uint64_t e13 = e03 & e03 << 7;
    const uint64_t e15 = e03 & e03 >> 9;
    const uint64_t e17 = e01 & e01 >> 7;

    flood1 |= flood1 << 18 & e11;
    flood3 |= flood3 << 14 & e13;
    flood5 |= flood5 >> 18 & e15;
    flood7 |= flood7 >> 14 & e17;

    const uint64_t e21 = e11 & e11 << 18;
    const uint64_t e23 = e13 & e13 << 14;
    const uint64_t e25 = e15 & e15 >> 18;
    const uint64_t e27 = e17 & e17 >> 14;

    flood1 |= flood1 << 36 & e21;
    flood3 |= flood3 << 28 & e23;
    flood5 |= flood5 >> 36 & e25;
    flood7 |= flood7 >> 28 & e27;

    return ((flood1 << 9 | flood7 >> 7 ) & ~file<'a'>())
           | ((flood3 << 7 | flood5 >> 9 ) & ~file<'h'>());

    /*    uint64_t result;
        asm(" mov   %2, %%rsi               /n"
            " mov   %%rdi, %%rax ;flood1 /n "
            " mov   %%rdi, %%rbx ;flood3 /n "
            " mov   %%rdi, %%rcx ;flood5 /n "
            " mov   %%rdi, %%rdx ;flood7 /n "
            " not   %%rsi                   /n"
            " movabs %3, %%r8               /n"
            " movabs %4, %%r9               /n"
            " and   %%rsi, %%r8  ; e01      /n"
            " and   %%rsi, %%r9  ; e03      /n"
            " shl   9, %%rax                /n"
            " shl   7, %%rbx                /n"
            " shr   9, %%rcx                /n"
            " shr   7, %%rdx                /n"
            " and   %%r8, %%rax             /n"
            " and   %%r9, %%rbx             /n"
            " and   %%r9, %%rcx             /n"
            " and   %%r8, %%rdx             /n"
            " or    %%rdi, %%rax /n"
            " or    %%rdi, %%rbx /n"
            " or    %%rdi, %%rbx /n"
            " or    %%rdi, %%rbx /n"
            " mov   %%r8, %%r10 /n"
            " mov   %%r9, %%r11 /n"
            " mov   %%r9, %%r12 /n"
            " mov   %%r8, %%r13 /n"
            " shl   9, %%r10                /n"
            " shl   7, %%r11                /n"
            " shr   9, %%r12                /n"
            " shr   7, %%r13                /n"
            " and   %%r8, %%r10             /n"
            " and   %%r9, %%r11             /n"
            " and   %%r9, %%r12             /n"
            " and   %%r8, %%r13             /n"





            : "a" (result)
            : "D" (flood1), "m" (occupied1), "i" (~file<'a'>()), "i" (~file<'h'>())
            : "%rbx", "%rcx", "%rdx", "%rsi", "%r8", "%r9"
        );*/
}
#endif
#if 0
inline __v2di Board::build13Attack(__v2di flood1) const {

    __v2di flood3 = flood1, flood5 = flood1, flood7 = flood1;

    __v2di e01 = _mm_set1_epi64x(~occupied1 & ~file<'a'>());
    __v2di e03 = _mm_set1_epi64x(~occupied1 & ~file<'h'>());
    __v2di e05 = e03;
    __v2di e07 = e01;
    // uint64_t e05 = ~occupied1 & ~file<'h'>();
    // uint64_t e07 = ~occupied1 & ~file<'a'>();
    flood1 |= _mm_slli_epi64(flood1, 9) & e01;
    flood3 |= _mm_slli_epi64(flood3, 7) & e03;
    flood5 |= _mm_srli_epi64(flood5, 9) & e05;
    flood7 |= _mm_srli_epi64(flood7, 7) & e07;

    e01 &= _mm_slli_epi64(e01, 9);
    e03 &= _mm_slli_epi64(e03, 7);
    e05 &= _mm_srli_epi64(e05, 9);
    e07 &= _mm_srli_epi64(e07, 7);

    flood1 |= _mm_slli_epi64(flood1, 18) & e01;
    flood3 |= _mm_slli_epi64(flood3, 14) & e03;
    flood5 |= _mm_srli_epi64(flood5, 18) & e05;
    flood7 |= _mm_srli_epi64(flood7, 14) & e07;

    e01 &= _mm_slli_epi64(e01, 18);
    e03 &= _mm_slli_epi64(e03, 14);
    e05 &= _mm_srli_epi64(e05, 18);
    e07 &= _mm_srli_epi64(e07, 14);

    flood1 |= _mm_slli_epi64(flood1, 36) & e01;
    flood3 |= _mm_slli_epi64(flood3, 28) & e03;
    flood5 |= _mm_srli_epi64(flood5, 36) & e05;
    flood7 |= _mm_srli_epi64(flood7, 28) & e07;

    return ((_mm_slli_epi64(flood1, 9) | _mm_srli_epi64(flood7, 7) ) & _mm_set1_epi64x(~file<'a'>()))
           | ((_mm_slli_epi64(flood3, 7) | _mm_srli_epi64(flood5, 9) ) & _mm_set1_epi64x(~file<'h'>())); }
#endif
inline __v2di Board::build02Attack(const unsigned sq) const {
#ifdef __SSSE3__
    __v2di maskedDirs = _mm_set1_epi64x(occupied1) & bits[sq].mask02;
    __v2di nibh = _mm_srli_epi16(maskedDirs, 4) & nibmask;
    __v2di nibl = maskedDirs & nibmask;
    __v2di reverseh = _mm_shuffle_epi8(swap16h, nibh);
    __v2di reversel = _mm_shuffle_epi8(swap16l, nibl);
    __v2di reverse = _mm_shuffle_epi8(reverseh | reversel, swap16);
    maskedDirs -= bits[sq].doublebits;
    reverse -= bits[sq].doublereverse2;
    nibh = _mm_srli_epi16(reverse, 4) & nibmask;
    nibl = reverse & nibmask;
    reverseh = _mm_shuffle_epi8(swap16h, nibh);
    reversel = _mm_shuffle_epi8(swap16l, nibl);
    reverse = _mm_shuffle_epi8(reverseh | reversel, swap16);
    maskedDirs ^= reverse;
    maskedDirs &= bits[sq].mask02;
    return maskedDirs;
#else
    uint64_t d0 = (occupied1 | 0x0101010101010101) & bits[sq].mask0x;
    uint64_t d2 = (occupied1 | 0xff) & bits[sq].mask2x;
    __v2di maskedDir02 = (__v2di) { d0, d2 };
    d0 <<= 63-sq;
    d2 <<= 63-sq;
    uint64_t lo = 3ULL << sq - __builtin_clzll(d0); // bitr is never < 56 here
    uint64_t hi = 3ULL << sq - __builtin_clzll(d2); // bitr is never < 56 here
//    uint64_t lo = 3ULL << sq - __builtin_clz(d0 >> 32); // bitr is never < 56 here
//    uint64_t hi = 3ULL << sq - __builtin_clz(d2 >> 32); // bitr is never < 56 here
//    uint64_t lo = rol(6, sq + bitr(d0)); // bitr is never < 56 here
//    uint64_t hi = rol(6, sq + bitr(d2));
    maskedDir02 ^= maskedDir02 - (__v2di) { lo, hi };
    maskedDir02 &= bits[sq].mask02;
    return maskedDir02;
#endif
}

#if 0
inline uint64_t Board::build02Attack(uint64_t flood0) const {

    uint64_t flood2 = flood0, flood4 = flood0, flood6 = flood0;

    uint64_t e00 =  ~occupied1 & ~file<'a'>();
    uint64_t e02 =  ~occupied1;
    uint64_t e04 =  ~occupied1 & ~file<'h'>();
    uint64_t e06 =  e02;

    flood0 |= flood0 << 1 & e00;
    flood2 |= flood2 << 8 & e02;
    flood4 |= flood4 >> 1 & e04;
    flood6 |= flood6 >> 8 & e06;

    e00 &= e00 << 1;
    e02 &= e02 << 8;
    e04 &= e04 >> 1;
    e06 &= e06 >> 8;

    flood0 |= flood0 <<  2 & e00;
    flood2 |= flood2 << 16 & e02;
    flood4 |= flood4 >>  2 & e04;
    flood6 |= flood6 >> 16 & e06;

    e00 &= e00 << 2;
    e02 &= e02 << 16;
    e04 &= e04 >> 2;
    e06 &= e06 >> 16;

    flood0 |= flood0 <<  4 & e00;
    flood2 |= flood2 << 32 & e02;
    flood4 |= flood4 >>  4 & e04;
    flood6 |= flood6 >> 32 & e06;

    return (flood0 << 1 & ~file<'a'>()) | (flood4 >> 1 & ~file<'h'>())
           | flood2 << 8 | flood6 >> 8; }
#endif
#if 0
inline void Board::build02Attack(__v2di flood0, __v2di& x02, __v2di& y02) const {

    __v2di flood2 = flood0, flood4 = flood0, flood6 = flood0;

    __v2di e00 =  _mm_set1_epi64x(~occupied1) & ~_mm_set1_epi64x(file<'a'>());
    __v2di e02 =  _mm_set1_epi64x(~occupied1);
    __v2di e04 =  _mm_set1_epi64x(~occupied1) & ~_mm_set1_epi64x(file<'h'>());
    __v2di e06 =  e02;

    flood0 |= _mm_slli_epi64(flood0, 1) & e00;
    flood2 |= _mm_slli_epi64(flood2, 8) & e02;
    flood4 |= _mm_srli_epi64(flood4, 1) & e04;
    flood6 |= _mm_srli_epi64(flood6, 8) & e06;

    e00 &= _mm_slli_epi64(e00, 1);
    e02 &= _mm_slli_epi64(e02, 8);
    e04 &= _mm_srli_epi64(e04, 1);
    e06 &= _mm_srli_epi64(e06, 8);

    flood0 |= _mm_slli_epi64(flood0,  2) & e00;
    flood2 |= _mm_slli_epi64(flood2, 16) & e02;
    flood4 |= _mm_srli_epi64(flood4,  2) & e04;
    flood6 |= _mm_srli_epi64(flood6, 16) & e06;

    e00 &= _mm_slli_epi64(e00, 2);
    e02 &= _mm_slli_epi64(e02, 16);
    e04 &= _mm_srli_epi64(e04, 2);
    e06 &= _mm_srli_epi64(e06, 16);

    flood0 |= _mm_slli_epi64(flood0,  4) & e00;
    flood2 |= _mm_slli_epi64(flood2, 32) & e02;
    flood4 |= _mm_srli_epi64(flood4,  4) & e04;
    flood6 |= _mm_srli_epi64(flood6, 32) & e06;

    __v2di res00 = (_mm_slli_epi64(flood0, 1) & ~_mm_set1_epi64x(file<'a'>()))
                   |  (_mm_srli_epi64(flood4, 1) & ~_mm_set1_epi64x(file<'h'>()));
    __v2di res22 = _mm_slli_epi64(flood2, 8) | _mm_srli_epi64(flood6, 8);
    x02 = _mm_unpacklo_epi64(res00, res22);
    y02 = _mm_unpackhi_epi64(res00, res22); }
#endif
inline uint64_t Board::buildNAttack(uint64_t n) const {
    uint64_t n1 = (n<<1 & ~file<'a'>())
                  | (n>>1 & ~file<'h'>());
    uint64_t n2 = (n<<2 & ~(file<'a'>() | file<'b'>()))
                  | (n>>2 & ~(file<'g'>() | file<'h'>()));
    return n1<<16 | n1>>16 | n2<<8 | n2>>8; }

template<Colors C>
void Board::buildAttacks() {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    uint64_t p, a;

    p = getPieces<C, Knight>();
    a = 0;
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        a |= knightAttacks[sq]; }
    getAttacks<C, Knight>() = a;
    getAttacks<C, All>() = a;

    MoveTemplateB* bs = bsingle[CI];
#ifdef __SSE__
    __v2di dir13 = _mm_set1_epi64x(0);
#else
    __v2di dir13 = { 0, 0 };
#endif
    p = getPieces<C, Bishop>();
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        __v2di a13 = build13Attack(sq);
        bs->move = Move(sq, 0, Bishop);
        bs->d13 = a13;
        bs++;
        dir13 |= a13; }
    bs->move = Move(0,0,0);
    a = fold(dir13);
    getAttacks<C, Bishop>() = a;
    getAttacks<C, All>() |= a;

    MoveTemplateR* rs = rsingle[CI];
#ifdef __SSE__
    __v2di dir02 = _mm_set1_epi64x(0);
#else
    __v2di dir02 = { 0, 0 };
#endif
    p = getPieces<C, Rook>();
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        if (0 & p) {    //TODO why is the vector version slower?
#if 0
            uint64_t sq2 = bit(p);
            __v2di a02, b02;
            build02Attack(_mm_set_epi64x(1ULL<<sq2, 1ULL<<sq), a02, b02);
            p &= p-1;
            rs[0].move = Move(sq, 0, Rook);
            rs[0].d02 = a02;
            rs[1].move = Move(sq2, 0, Rook);
            rs[1].d02 = b02;
            rs+=2;
            dir02 |= a02 | b02;
#endif
        }
        else {
            __v2di a02 = build02Attack(sq);
            rs->move = Move(sq, 0, Rook);
            rs->d02 = a02;
            rs++;
            dir02 |= a02; } }
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
        a |= fold(a13 | a02); }
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
void Board::buildPins() {
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
#ifdef __SSE__
    __v2di dir20 = _mm_shuffle_epi32(dir02, 0x4e);
    __v2di dir31 = _mm_shuffle_epi32(dir13, 0x4e);
#else
    union {
    	__v2di v2;
    	uint64_t p2[2];
    } converter;
    converter.v2 = dir02;
    __v2di dir20 = (__v2di) { converter.p2[1], converter.p2[0] };
    converter.v2 = dir13;
    __v2di dir31 = (__v2di) { converter.p2[1], converter.p2[0] };
#endif
    dpins[CI].d02 = dir20 & dir13 & dir31;
    dpins[CI].d13 = dir31 & dir02 & dir20;
    __v2di pins2 = dpins[CI].d02 & dpins[CI].d13;
#ifdef __x86_64__
    pins[CI] = _mm_cvtsi128_si64(_mm_unpackhi_epi64(pins2, pins2))
               & _mm_cvtsi128_si64(pins2);
#else
#ifdef __SSE__
    __v2di folded = _mm_unpackhi_epi64(pins2, pins2) & pins2;
#else
    converter.v2 = pins2;
    __v2di folded = (__v2di) { converter.p2[0] & converter.p2[1], converter.p2[0] & converter.p2[1] };
#endif
    pins[CI] = *(uint64_t*) &folded;
#endif

}
template<Colors C> void Board::setPiece(unsigned piece, unsigned pos, const Eval& e) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    getPieces<C>(piece) |= 1ULL << pos;
    occupied[CI] |= 1ULL << pos;
    occupied1 |= 1ULL << pos;
    keyScore.vector += e.keyScore(C*piece, pos).vector;
    matIndex += ::matIndex[CI][piece]; }
#endif
