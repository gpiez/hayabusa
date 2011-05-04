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

inline __v2di BoardBase::build13Attack(__v2di flood1) const {
    
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
         | ((_mm_slli_epi64(flood3, 7) | _mm_srli_epi64(flood5, 9) ) & _mm_set1_epi64x(~file<'h'>()));
}

inline __v2di BoardBase::build02Attack(const unsigned sq) const {
/*    const __v2di b02 = _mm_set_epi64x(0xff, 0x0101010101010101); // border
    __v2di _maskedDir02 = (_mm_set1_epi64x(occupied1)|b02) & mask02[sq].b;
    uint64_t d0 = _mm_cvtsi128_si64x(maskedDir02);
    uint64_t d2 = _mm_cvtsi128_si64x(_mm_unpackhi_epi64(maskedDir02,maskedDir02));*/
    uint64_t d0 = (occupied1 | 0x0101010101010101) & mask02[sq].d0;
    uint64_t d2 = (occupied1 | 0xff) & mask02[sq].d2;
    __v2di maskedDir02 = _mm_set_epi64x(d2, d0);
    d0 <<= ~sq; //63-sq;
    d2 <<= ~sq; //63-sq;
    uint64_t lo = rol(6, sq + bitr(d0));
    uint64_t hi = rol(6, sq + bitr(d2));
    maskedDir02 ^= maskedDir02 - _mm_set_epi64x(hi, lo);
    maskedDir02 &= mask02[sq].x;
    return maskedDir02;
//    return _mm_set1_epi64x(att0Tab[attLen0[sq]][sq], att2Tab[attLen2[sq]][sq]);
}

inline uint64_t BoardBase::build02Attack(uint64_t flood0) const {
    
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
        | flood2 << 8 | flood6 >> 8;
}

inline void BoardBase::build02Attack(__v2di flood0, __v2di& x02, __v2di& y02) const {

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
    y02 = _mm_unpackhi_epi64(res00, res22);
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
        if (0 & p) {
            unsigned sq2 = bit(p);
            __v2di a02, b02;
            build02Attack(_mm_set_epi64x(1ULL<<sq2, 1ULL<<sq), a02, b02);
            p &= p-1;
            rs[0].move = Move(sq, 0, Rook);
            rs[0].d02 = a02;
            rs[1].move = Move(sq2, 0, Rook);
            rs[1].d02 = b02;
            rs+=2;
            dir02 |= a02 | b02;
        } else {
            __v2di a02 = build02Attack(sq);
            rs->move = Move(sq, 0, Rook);
            rs->d02 = a02;
            rs++;
            dir02 |= a02;
        }
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

inline unsigned BoardBase::getPieceFromBit(uint64_t bit) const {
    ASSERT( !((getPieces<White,King>() | getPieces<Black,King>()) & bit) );
#ifdef __SSE4_1__
    __v2di piece = _mm_set_epi64x(bit, bit);
    __v2di zero = _mm_set_epi64x(0, 0);
    __v2di r = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Rook  >(), piece), zero) & _mm_set_epi64x(Rook, Rook);
    __v2di b = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Bishop>(), piece), zero) & _mm_set_epi64x(Bishop, Bishop);
    __v2di q = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Queen >(), piece), zero) & _mm_set_epi64x(Queen, Queen);
    __v2di n = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Knight>(), piece), zero) & _mm_set_epi64x(Knight, Knight);
    __v2di p = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Pawn  >(), piece), zero) & _mm_set_epi64x(Pawn, Pawn);
//        __v2di k = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<King  >(), piece), zero) & _mm_set_epi64x(King, King);
    return fold(r|b|q|n|p);
#else
    return
//            (getPieces<White,King>() | getPieces<Black,King>()) & bit ? King:
        (getPieces<White,Pawn>() | getPieces<Black,Pawn>()) & bit ? Pawn:
        (getPieces<White,Knight>() | getPieces<Black,Knight>()) & bit ? Knight:
        (getPieces<White,Queen>() | getPieces<Black,Queen>()) & bit ? Queen:
        (getPieces<White,Bishop>() | getPieces<Black,Bishop>()) & bit ? Bishop:
        (getPieces<White,Rook>() | getPieces<Black,Rook>()) & bit ? Rook:
        NoPiece;
#endif
}

#endif
