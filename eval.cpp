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
#include <pch.h>

#include "eval.h"
#include "boardbase.h"
#include "transpositiontable.tcc"
//#include <cstdio>

__thread PawnEntry Eval::pawnEntry;

uint32_t Eval::borderTab4_0[nSquares];
uint32_t Eval::borderTab321[nSquares];
uint32_t Eval::borderTab567[nSquares];

__v16qi Eval::kMask0[nSquares];
__v16qi Eval::kMask1[nSquares];

int squareControl[nSquares];
//void sigmoid(Score* p, unsigned int n, double start, double end, double dcenter, double width) {
//    double t0 = -0.5*n-dcenter;
//    double t1 = 0.5*n-dcenter;
//    double l0 = 1/(1+exp(-t0/width));
//    double l1 = 1/(1+exp(-t1/width));
//
//    double r = (end - start)/(l1-l0);
//    double a = start - l0*r;
//    for (unsigned int i = 0; i < n; ++i) {
//        double t = i - 0.5*n - dcenter;
//        p[i] = lrint(a + r/(1.0 + exp(-t/width)));
//    }
//}

RawScore Eval::pawn, Eval::knight, Eval::bishop, Eval::rook, Eval::queen;
RawScore Eval::bishopPair;
RawScore Eval::knightAlone;
RawScore Eval::bishopAlone;
RawScore Eval::n_p_ram[9], Eval::n_p[17];
RawScore Eval::b_bad_own_p[9], Eval::b_p[17], Eval::b_p_ram[9], Eval::bpair_p[17];
RawScore Eval::r_p[17];
RawScore Eval::q_p[17];
RawScore Eval::pawnBackward, Eval::pawnBackwardOpen;
RawScore Eval::pawnPasser[8] = { 0, 50, 50, 60, 80, 110, 150, 0 };
RawScore Eval::pawnHalfIsolated;
RawScore Eval::pawnIsolated;

static const uint64_t mobBits[nDirs/2][64] = {{
    0x0, 0x2, 0x6, 0xe, 0x1e, 0x3e, 0x7e, 0xfe,
    0x8000000000000000, 0x8000000000000002, 0x8000000000000006, 0x800000000000000e, 0x800000000000001e, 0x800000000000003e, 0x800000000000007e, 0x0,
    0xc000000000000000, 0xc000000000000002, 0xc000000000000006, 0xc00000000000000e, 0xc00000000000001e, 0xc00000000000003e, 0x0, 0x0,
    0xe000000000000000, 0xe000000000000002, 0xe000000000000006, 0xe00000000000000e, 0xe00000000000001e, 0x0, 0x0, 0x0,
    0xf000000000000000, 0xf000000000000002, 0xf000000000000006, 0xf00000000000000e, 0x0, 0x0, 0x0, 0x0,
    0xf800000000000000, 0xf800000000000002, 0xf800000000000006, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xfc00000000000000, 0xfc00000000000002, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xfe00000000000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
},{
    0x0, 0x200, 0x40200, 0x8040200, 0x1008040200, 0x201008040200, 0x40201008040200, 0x8040201008040200,
    0x80000000000000, 0x80000000000200, 0x80000000040200, 0x80000008040200, 0x80001008040200, 0x80201008040200, 0xc0201008040200, 0x0,
    0x80400000000000, 0x80400000000200, 0x80400000040200, 0x80400008040200, 0x80401008040200, 0x80601008040200, 0x0, 0x0,
    0x80402000000000, 0x80402000000200, 0x80402000040200, 0x80402008040200, 0x80403008040200, 0x0, 0x0, 0x0,
    0x80402010000000, 0x80402010000200, 0x80402010040200, 0x80402018040200, 0x0, 0x0, 0x0, 0x0,
    0x80402010080000, 0x80402010080200, 0x804020100c0200, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x80402010080400, 0x80402010080600, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x80402010080402, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
},{
    0x0, 0x100, 0x10100, 0x1010100, 0x101010100, 0x10101010100, 0x1010101010100, 0x101010101010100,
    0x100000000000000, 0x100000000000100, 0x100000000010100, 0x100000001010100, 0x100000101010100, 0x100010101010100, 0x101010101010100, 0x0,
    0x101000000000000, 0x101000000000100, 0x101000000010100, 0x101000001010100, 0x101000101010100, 0x101010101010100, 0x0, 0x0,
    0x101010000000000, 0x101010000000100, 0x101010000010100, 0x101010001010100, 0x101010101010100, 0x0, 0x0, 0x0,
    0x101010100000000, 0x101010100000100, 0x101010100010100, 0x101010101010100, 0x0, 0x0, 0x0, 0x0,
    0x101010101000000, 0x101010101000100, 0x101010101010100, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x101010101010000, 0x101010101010100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x101010101010100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
},{
    0x0, 0x80, 0x4080, 0x204080, 0x10204080, 0x810204080, 0x40810204080, 0x2040810204080,
    0x200000000000000, 0x200000000000080, 0x200000000004080, 0x200000000204080, 0x200000010204080, 0x200000810204080, 0x200040810204080, 0x0,
    0x204000000000000, 0x204000000000080, 0x204000000004080, 0x204000000204080, 0x204000010204080, 0x204000810204080, 0x0, 0x0,
    0x204080000000000, 0x204080000000080, 0x204080000004080, 0x204080000204080, 0x204080010204080, 0x0, 0x0, 0x0,
    0x204081000000000, 0x204081000000080, 0x204081000004080, 0x204081000204080, 0x0, 0x0, 0x0, 0x0,
    0x204081020000000, 0x204081020000080, 0x204081020004080, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x204081020400000, 0x204081020400080, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x204081020408000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
}};

static const uint64_t knightBits[0x100] = {
    0x20400, 0x50800, 0xa1100, 0x142200, 0x284400, 0x508800, 0xa01000, 0x402000,
    0x2040004, 0x5080008, 0xa110011, 0x14220022, 0x28440044, 0x50880088, 0xa0100010, 0x40200020,
    0x204000402, 0x508000805, 0xa1100110a, 0x1422002214, 0x2844004428, 0x5088008850, 0xa0100010a0, 0x4020002040,
    0x20400040200, 0x50800080500, 0xa1100110a00, 0x142200221400, 0x284400442800, 0x508800885000, 0xa0100010a000, 0x402000204000,
    0x2040004020000, 0x5080008050000, 0xa1100110a0000, 0x14220022140000, 0x28440044280000, 0x50880088500000, 0xa0100010a00000, 0x40200020400000,
    0x204000402000000, 0x508000805000000, 0xa1100110a000000, 0x1422002214000000, 0x2844004428000000, 0x5088008850000000, 0xa0100010a0000000, 0x4020002040000000,
    0x400040200000000, 0x800080500000000, 0x1100110a00000000, 0x2200221400000000, 0x4400442800000000, 0x8800885000000000, 0x100010a000000000, 0x2000204000000000,
    0x4020000000000, 0x8050000000000, 0x110a0000000000, 0x22140000000000, 0x44280000000000, 0x88500000000000, 0x10a00000000000, 0x20400000000000
};

static const uint64_t pawnBits[nColors][0x100] = {{
    0x200, 0x500, 0xa00, 0x1400, 0x2800, 0x5000, 0xa000, 0x4000,
    0x20000, 0x50000, 0xa0000, 0x140000, 0x280000, 0x500000, 0xa00000, 0x400000,
    0x2000000, 0x5000000, 0xa000000, 0x14000000, 0x28000000, 0x50000000, 0xa0000000, 0x40000000,
    0x200000000, 0x500000000, 0xa00000000, 0x1400000000, 0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000,
    0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000, 0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000,
    0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000, 0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000,
    0x200000000000000, 0x500000000000000, 0xa00000000000000, 0x1400000000000000, 0x2800000000000000, 0x5000000000000000, 0xa000000000000000, 0x4000000000000000,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
},{
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x2, 0x5, 0xa, 0x14, 0x28, 0x50, 0xa0, 0x40,
    0x200, 0x500, 0xa00, 0x1400, 0x2800, 0x5000, 0xa000, 0x4000,
    0x20000, 0x50000, 0xa0000, 0x140000, 0x280000, 0x500000, 0xa00000, 0x400000,
    0x2000000, 0x5000000, 0xa000000, 0x14000000, 0x28000000, 0x50000000, 0xa0000000, 0x40000000,
    0x200000000, 0x500000000, 0xa00000000, 0x1400000000, 0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000,
    0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000, 0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000,
    0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000, 0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000
}};

static const uint64_t bits[0x100] = {
    0x000000000000001, 0x000000000000002, 0x000000000000004, 0x000000000000008, 0x0000000000000010, 0x0000000000000020, 0x0000000000000040, 0x0000000000000080,
    0x000000000000100, 0x000000000000200, 0x000000000000400, 0x000000000000800, 0x0000000000001000, 0x0000000000002000, 0x0000000000004000, 0x0000000000008000,
    0x000000000010000, 0x000000000020000, 0x000000000040000, 0x000000000080000, 0x0000000000100000, 0x0000000000200000, 0x0000000000400000, 0x0000000000800000,
    0x000000001000000, 0x000000002000000, 0x000000004000000, 0x000000008000000, 0x0000000010000000, 0x0000000020000000, 0x0000000040000000, 0x0000000080000000,
    0x000000100000000, 0x000000200000000, 0x000000400000000, 0x000000800000000, 0x0000001000000000, 0x0000002000000000, 0x0000004000000000, 0x0000008000000000,
    0x000010000000000, 0x000020000000000, 0x000040000000000, 0x000080000000000, 0x0000100000000000, 0x0000200000000000, 0x0000400000000000, 0x0000800000000000,
    0x001000000000000, 0x002000000000000, 0x004000000000000, 0x008000000000000, 0x0010000000000000, 0x0020000000000000, 0x0040000000000000, 0x0080000000000000,
    0x100000000000000, 0x200000000000000, 0x400000000000000, 0x800000000000000, 0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000
};

#if !defined(__SSE_4_2__)
// popcount, which counts at most 15 ones, for counting pawns
static inline uint64_t popcount15( uint64_t x )
{
    x -=  x>>1 & 0x5555555555555555LL;
    x  = ( x>>2 & 0x3333333333333333LL ) + ( x & 0x3333333333333333LL );
    x *= 0x1111111111111111LL;
    return  x>>60;
}

// special popcount, assuming each 2-bit block has max one bit set, for counting light/dark squares.
static inline uint64_t popcount2( uint64_t x )
{
    x -=  x>>1 & 0x5555555555555555LL;
    x  = (( x>>2 )+x) & 0x3333333333333333LL;
    x  = (( x>>4 )+x) & 0x0f0f0f0f0f0f0f0fLL;
    x *= 0x0101010101010101LL;
    return  x>>56;
}

#else
#define popcount(x) __builtin_popcountll(x)
#define popcount2(x) __builtin_popcountll(x)
#endif

Eval::Eval() {
    pt = new TranspositionTable<PawnEntry, 4, PawnKey>;
//    pt->setSize(0x100000000);
    pawn = 100;
    knight = 325;            // + 16 Pawns * 3 = 342 == 3 7/16s
    bishop = 325;
    rook = 500;
    queen = 925;

    bishopPair = 50;
    knightAlone = -125;
    bishopAlone = -100;

    sigmoid(n_p_ram, 0, 75, 5);             // bonus for each pawn ram, max is 75
    sigmoid(n_p, -25, 25, 12);

    sigmoid(b_bad_own_p, 0, -30, 6);
    sigmoid(b_p, 0, -30, 12);
    sigmoid(b_p_ram, 0, -30, 5);
    sigmoid(bpair_p, 15, -15, 12);

    sigmoid(r_p, 50, -50, 12);
    sigmoid(q_p, 50, -50, 12);

    initPS();
    initZobrist();
    initTables();
//    static RawScore bishopGoodD;
//    static RawScore bishopKnightD;
//    static RawScore bishopRookD;
#ifndef NDEBUG
    printSigmoid(n_p_ram, "n_p_ram");             // bonus for each pawn ram, max is 75
    printSigmoid(n_p, "n_p");

    printSigmoid(b_bad_own_p, "bad_own_p");
    printSigmoid(b_p, "b_p");
    printSigmoid(b_p_ram, "b_p_ram");
    printSigmoid(bpair_p, "bpair_p");

    printSigmoid(r_p, "r_p");
    printSigmoid(q_p, "r_p");

#endif
}

void Eval::initPS() {
    RawScore rook[nSquares] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        5, 10, 10, 10, 10, 10, 10,  5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        0,  0,  0,  5,  5,  0,  0,  0
    };
    RawScore bishop[nSquares] = {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    };
    RawScore queen[nSquares] = {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
        -5,  0,  5,  5,  5,  5,  0, -5,
        0,  0,  5,  5,  5,  5,  0, -5,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    };
    RawScore knight[nSquares] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };
    RawScore pawn[nSquares] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
        5,  5, 10, 25, 25, 10,  5,  5,
        0,  0,  0, 20, 20,  0,  0,  0,
        5, -5,-10,  0,  0,-10, -5,  5,
        5, 10, 10,-20,-20, 10, 10,  5,
        0,  0,  0,  0,  0,  0,  0,  0
    };
    RawScore king[nSquares] = {
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -10,-20,-20,-20,-20,-20,-20,-10,
        20, 20,  0,  0,  0,  0, 20, 20,
        20, 30, 10,  0,  0, 10, 30, 20
    };

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Pawn, sq) = Eval::pawn + pawn[sq ^ 0x38];
        getPS(-Pawn, sq)  = -Eval::pawn - pawn[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Rook, sq)  = Eval::rook + rook[sq ^ 0x38];
        getPS(-Rook, sq) = -Eval::rook - rook[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Bishop, sq) = Eval::bishop + bishop[sq ^ 0x38];
        getPS(-Bishop, sq) = -Eval::bishop - bishop[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Knight, sq) = Eval::knight + knight[sq ^ 0x38];
        getPS(-Knight, sq) = -Eval::knight - knight[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Queen, sq) = Eval::queen + queen[sq ^ 0x38];
        getPS(-Queen, sq) = -Eval::queen - queen[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( King, sq) = king[sq ^ 0x38];
        getPS(-King, sq) = - king[sq];
    }

}

void Eval::initZobrist() {
    srand(1);
    for (int p = -nPieces; p <= (signed int)nPieces; p++)
        for (unsigned int i = 0; i < nSquares; ++i) {
            uint64_t r;
            do {
                r = (uint64_t) rand()
                ^ (uint64_t) rand() << 8
                ^ (uint64_t) rand() << 16
                ^ (uint64_t) rand() << 24
                ^ (uint64_t) rand() << 32
                ^ (uint64_t) rand() << 40
                ^ (uint64_t) rand() << 48
                ^ (uint64_t) rand() << 56;
            } while (popcount(r) >= 29 && popcount(r) <= 36);
    
            if (p)
                zobristPieceSquare[p+nPieces][i].key = r;
            else
                zobristPieceSquare[p+nPieces][i].key = 0;
    
            if (abs(p) == Pawn)
                zobristPieceSquare[p+nPieces][i].pawnKey = r ^ r >> 32;
            else
                zobristPieceSquare[p+nPieces][i].pawnKey = 0;
        }
}

void Eval::initTables() {
    for (unsigned int x = 0; x<nFiles; ++x)
    for (unsigned int y = 0; y<nRows; ++y) {
        uint64_t oking = x+y*nFiles;
        uint64_t offset = (oking + 010) & 020;    // 070-077 -> 080, 060-067 -> 060
        borderTab4_0[oking] = (x==0) | (x==7) << 16;
        borderTab321[oking] = (x==0) | (x==7) << 16 | 0x10101*(y==7);
        borderTab567[oking] = (x==0) | (x==7) << 16 | 0x10101*(y==0);

        BoardBase b = {{{0}}};
        for (unsigned int dir = 0; dir<nDirs; ++dir) {
            b.pieces[oking+dirOffsets[dir]] = 0xff;
        }
        kMask0[oking] = (__v16qi&)b.pieces[offset];
        kMask1[oking] = (__v16qi&)b.pieces[offset-0x10];
    }
}

RawScore Eval::pieces(const PieceList&, int ) const {
//     if (pl.getCounts() == 0x01000100000000)     //only one knight left
//         value += knightAlone;
//     if (pl.getCounts() == 0x01000000010000)     //only one bishop left
//         value += knightAlone;
//     if (pl[Bishop] > 1)
//         value += bishopPair;
//
//     value += b_p[pl[Pawn]];
    RawScore value = 0;

    return value;
}

template<Colors C>
PawnEntry::Shield evalShield(const BoardBase &b) {

    uint64_t p = b.getPieces<C,Pawn>();
    uint64_t kside = file<'f'>() | file<'g'>() | file<'h'>();
    uint64_t qside = file<'a'>() | file<'b'>() | file<'c'>();

    unsigned kshield = 4*popcount(kside & rank<C,2>() & p)
                     + 3*popcount(kside & rank<C,3>() & p)
    				 + 2*popcount(kside & rank<C,4>() & p);

    unsigned qshield = 4*popcount(qside & rank<C,2>() & p)
                     + 3*popcount(qside & rank<C,3>() & p)
    				 + 2*popcount(qside & rank<C,4>() & p);

    PawnEntry::Shield ret;
    ret.weakLight = 0;
    ret.weakDark = 0;
    ret.value = kshield + qshield;
    return ret;
}

RawScore Eval::pawns(const BoardBase& b) const {
    PawnKey k=b.keyScore.pawnKey;
    Sub<PawnEntry, 4>* st = pt->getSubTable(k);
    if (k >> PawnEntry::upperShift && pt->retrieve(st, k, pawnEntry)) {
        stats.pthit++;
    } else {
        stats.ptmiss++;
        pawnEntry.score = 0;
        uint64_t wpawn = b.getPieces<White,Pawn>();
        uint64_t bpawn = b.getPieces<Black,Pawn>();

        uint64_t wAbove = wpawn << 8 | wpawn << 16;
        wAbove |= wAbove << 16 | wAbove << 32;
        uint64_t wBelow = wpawn >> 8 | wpawn >> 16;
        wBelow |= wBelow >> 16 | wBelow >> 32;

        uint64_t bAbove = bpawn << 8 | bpawn << 16;
        bAbove |= bAbove << 16 | bAbove << 32;
        uint64_t bBelow = bpawn >> 8 | bpawn >> 16;
        bBelow |= bBelow >> 16 | bBelow >> 32;

        // squares above/below the most advanced pawns

        uint64_t wAboveA = wAbove & ~(wBelow | wpawn);
        uint64_t bBelowA = bBelow & ~(bAbove | bpawn);

        // calculate squares which are or possibly are attacked by w and b pawns
        // take only the most advanced attacker and the most backward defender into account

        uint64_t wAttack = (wAbove >> 1 & ~0x8080808080808080LL) | (wAbove << 1 & ~0x101010101010101LL);
        uint64_t bAttack = (bBelow >> 1 & ~0x8080808080808080LL) | (bBelow << 1 & ~0x101010101010101LL);
        uint64_t wAttackA = (wAboveA >> 1 & ~0x8080808080808080LL) | (wAboveA << 1 & ~0x101010101010101LL);
        uint64_t bAttackA = (bBelowA >> 1 & ~0x8080808080808080LL) | (bBelowA << 1 & ~0x101010101010101LL);
        uint64_t wContested = wAttack & bAttackA;
        uint64_t bContested = wAttackA & bAttack;

        // backward pawns are pawns which are may be attacked, if the advance,
        // but are not on a contested file (otherwise they would be defended)

        wContested |= wContested << 8 | wContested >> 8;
        wContested |= wContested << 24 | wContested >> 24;
        bContested |= bContested << 8 | bContested >> 8;
        bContested |= bContested << 24 | bContested >> 24;
        uint64_t wBackward = wpawn & bAttack & ~wContested;
        uint64_t bBackward = bpawn & wAttack & ~bContested;

        pawnEntry.score += pawnBackward * (popcount15(wBackward) - popcount15(bBackward));
        pawnEntry.score += pawnBackwardOpen * (popcount15(wBackward & ~bBelow) - popcount15(bBackward & ~wAbove));

        // a white pawn is not passed, if he is below a opponent pawn or its attack squares
        // store positions of passers for later use in other eval functions

        uint64_t wNotPassedMask = bBelow | bAttack;
        uint64_t bNotPassedMask = wAbove | wAttack;
        uint64_t wPassed = wpawn & ~wNotPassedMask;
        uint64_t bPassed = bpawn & ~bNotPassedMask;
        unsigned i;
        for (i = 0; wPassed && i < nHashPassers; wPassed &= wPassed - 1, i++) {
            int pos = __builtin_ctzll(wPassed);
            int y = pos >> 3;
            pawnEntry.score += pawnPasser[y];
            pawnEntry.passers[0][i] = pos + (y << 3);
        }
        for ( i=0; bPassed && i<nHashPassers; bPassed &= bPassed-1, i++ ) {
            int pos = __builtin_ctzll(bPassed);
            int y = pos>>3;
            pawnEntry.score -= pawnPasser[7-y];
            pawnEntry.passers[1][i] = pos + ( y<<3 );
        }

        // possible weak pawns are adjacent to open files or below pawns an a adjacent file on both sides
        // rook pawns are always adjacent to a "open" file
        uint64_t wOpenFiles = ~(wAbove | wpawn | wBelow);
        uint64_t bOpenFiles = ~(bAbove | bpawn | bBelow);
        pawnEntry.openFiles[0] = ~wOpenFiles;
        pawnEntry.openFiles[1] = ~bOpenFiles;
        uint64_t wRightIsolani = wpawn & (wOpenFiles<<1 | 0x101010101010101LL);
        uint64_t bRightIsolani = bpawn & (bOpenFiles<<1 | 0x101010101010101LL);
        uint64_t wLeftIsolani = wpawn & (wOpenFiles>>1 | 0x8080808080808080LL);
        uint64_t bLeftIsolani = bpawn & (bOpenFiles>>1 | 0x8080808080808080LL);
        pawnEntry.score += pawnHalfIsolated * (popcount15(wRightIsolani|wLeftIsolani) - popcount15(bRightIsolani|bLeftIsolani));
        pawnEntry.score += pawnIsolated * (popcount15(wRightIsolani&wLeftIsolani) - popcount15(bRightIsolani&bLeftIsolani));

        pawnEntry.lShield[0] = evalShield<White>(b);
        pawnEntry.rShield[0] = evalShield<White>(b);
        pawnEntry.lShield[1] = evalShield<Black>(b);
        pawnEntry.rShield[1] = evalShield<Black>(b);
        pawnEntry.upperKey = k >> PawnEntry::upperShift;
        pawnEntry.w = wpawn;
        pawnEntry.b = bpawn;
        if (k >> PawnEntry::upperShift)
            pt->store(st, pawnEntry);
    }
    return pawnEntry.score;
}

RawScore Eval::eval(const BoardBase& b) const {
#if defined(MYDEBUG)
    RawScore value = 0;
#ifdef BITBOARD
#else
    for (int pi = 0; pi <= 1; ++pi) {
        int C = 1-pi*2;        
        uint8_t king = b.pieceList[pi].getKing();
        value += getPS(C*King, king);
        for (uint8_t i = b.pieceList[pi][Pawn]; i > 0;) {
            --i;
            uint8_t pawn = b.pieceList[pi].getPawn(i);
            value += getPS(C*Pawn, pawn);
        }
        for (uint8_t i = b.pieceList[pi][Rook]; i > 0;) {
            --i;
            uint8_t rook = b.pieceList[pi].get(Rook, i);
            value += getPS(C*Rook, rook);
        }
        for (uint8_t i = b.pieceList[pi][Bishop]; i > 0;) {
            --i;
            uint8_t bishop = b.pieceList[pi].get(Bishop, i);
            value += getPS(C*Bishop, bishop);
        }
        for (uint8_t i = b.pieceList[pi][Knight]; i > 0;) {
            --i;
            uint8_t knight = b.pieceList[pi].get(Knight, i);
            value += getPS(C*Knight, knight);
        }
        for (uint8_t i = b.pieceList[pi][Queen]; i > 0;) {
            --i;
            uint8_t queen = b.pieceList[pi].get(Queen, i);
            value += getPS(C*Queen, queen);
        }
    }
    if (value != b.keyScore.score) asm("int3");
#endif    

#endif
    return b.keyScore.score + pawns(b) + mobility(b);
}

// double popcount of two quadwords
__v2di pop2count(__v2di x) {
    
    static const __v16qi mask4 = {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf};
    static const __v16qi count4 = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

    asm(
        "movdqa        %0, %%xmm1    \n"

        "psrlw        $4, %%xmm1    \n"
        "pand        %1, %0    \n"    // xmm0 := lower nibbles
        "pand        %1, %%xmm1    \n"    // xmm1 := higher nibbles
        "movdqa        %2, %%xmm2    \n"
        "movdqa        %2, %%xmm3    \n"    // get popcount
        "pshufb        %0, %%xmm2    \n"    // for all nibbles
        "pshufb        %%xmm1, %%xmm3    \n"    // using PSHUFB

        "paddb        %%xmm2, %%xmm3    \n"    // popcount for all bytes
        "pxor        %0, %0    \n"
        "psadbw        %%xmm3, %0    \n"    // sum popcounts

        : "=x" (x)
        : "x" (mask4), "x" (count4), "0" (x)
        : "%xmm1", "%xmm2", "%xmm3"
    );
    return x;
}

RawScore look2up(__v2di x, __v16qi tab) {
    __v8hi y = _mm_shuffle_epi8(tab, x);
    return _mm_extract_epi16(y, 0) + _mm_extract_epi16(y, 4);
}

void printBit(uint64_t bits) {
    QTextStream cout(stdout);
    for (int y=8; y; ) {
        --y;
        for (int x=0; x<8; ++x) {
            if ((bits >> (x+y*8)) & 1)
                cout << "X";
            else
                cout << ".";
        }
        cout << endl;
    }
    cout << endl;
}

template<unsigned int I>
uint64_t extract(const __v2di &) {
    return 0;
    //return _mm_extract_epi64(v, I);
}

void printBit(__v2di bits) {
    QTextStream cout(stdout);
    for (int y=8; y; ) {
        --y;
        for (int x=0; x<8; ++x) {
            if ((extract<0>(bits) >> (x+y*8)) & 1)
                cout << "X";
            else
                cout << ".";
        }
        cout << " ";
        for (int x=0; x<8; ++x) {
            if ((extract<1>(bits) >> (x+y*8)) & 1)
                cout << "X";
            else
                cout << ".";
        }
        cout << endl;
    
    }
    cout << endl;
}

static const RawScore mobValues[nPieces+1][32] = {
	{0},	// nothing
	{ 0, 1, 2, 6, 10, 13, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 }, //rook
	{ 0, 1, 3, 6, 10, 13, 15, 17, 18, 19, 20, 20, 20, 20, 20, 20 }, //bishop
	{ 0, 0, 1, 1, 2, 3, 5, 7, 8, 8, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
	{ 0, 3, 7, 12, 17, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 },
	{0},
	{0}
};

template<Colors C>
RawScore Eval::mobilityValue( const BoardBase &b, const uint64_t (&restrictions)[nColors][nPieces+1]) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
	RawScore score = 0;
    const __v2di* psingle = b.single[CI];
    const Move* pmove = b.moves[CI];
    const __v2di zero = _mm_set1_epi64x(0);
    for(;;) {
        Move m = *pmove++;
		if (!m.data) break;
        __v2di a02 = *psingle++;
		__v2di a13 = *psingle++;
#ifdef USE_PINS_IN_MOBILITY
		__v2di from2 = _mm_set1_epi64x(1ULL<<m.from());
		__v2di pin02 = from2 & pins[CI].dir02;
		__v2di pin13 = from2 & pins[CI].dir13;
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
		uint64_t mob = fold(pin02|pin13);
#else
		uint64_t mob = fold(a02|a13);
#endif
		score += mobValues[m.piece()][popcount(mob & restrictions[CI][m.piece()])];
    }

    return score;
}

template<Colors C>
void Eval::mobilityRestrictions(const BoardBase &b, uint64_t (&restrictions)[nColors][nPieces+1]) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
	uint64_t r = b.getPieces<C,Rook>() + b.getPieces<C,Bishop>() +
				 b.getPieces<C,Queen>()+ b.getPieces<C,Knight>() +
				 b.getPieces<C,Pawn>() + b.getPieces<C,King>();
	restrictions[CI][Knight] = restrictions[CI][Bishop] = r |= b.getAttacks<-C,Pawn>();
	restrictions[CI][Rook] = r |= b.getAttacks<-C,Knight>() | b.getAttacks<-C,Bishop>();
	restrictions[CI][Queen] = r |= b.getAttacks<-C,Rook>();
}

RawScore Eval::mobility(const BoardBase& b) const {
	uint64_t restrictions[nColors][nPieces+1];

	mobilityRestrictions<White>(b, restrictions);
	mobilityRestrictions<Black>(b, restrictions);

    return mobilityValue<White>(b, restrictions) - mobilityValue<Black>(b, restrictions);
}

// bit 0 = blocked for enemy king
// bit 1 = defended by B and other
// bit 2 = attacked by R and other
// bit 3 = attacked by Q and other
// 
template<Colors C>
void Eval::EvalMate(const ColoredBoard<C>& b) const {
#ifdef BITBOARD
#else    
    uint8_t oking = b.getOKing();
    uint64_t offset = (oking + 010) & 020;    // 070-077 -> 080, 060-067 -> 060
    const __v16qi zero = _mm_set1_epi8(0);
    const __v16qi low4bits = _mm_set1_epi8(0xf);
    const __v16qi mateBits = _mm_set_epi8(0,0,0,2,0,4,6,6,0,8,10,10,12,12,14,14);
    __v16qi m0 = kMask0[oking];
    __v16qi m1 = kMask1[oking];
    __v16qi p0 = (_mm_load_si128(&b.pieces[offset]) & m0) < zero;
    __v16qi p1 = (_mm_load_si128(&b.pieces[offset-0x10]) & m1) < zero;
    __v16qi sa0 = _mm_load_si128(&b.shortAttack[b.CI][offset]) & m0;
    __v16qi sa1 = _mm_load_si128(&b.shortAttack[b.CI][offset-0x10]) & m1;
    sa0 |= _mm_srli_epi16(sa0, nNBits);
    sa1 |= _mm_srli_epi16(sa1, nNBits);
    __v16qi la0 = _mm_load_si128(&b.longAttack[b.CI][offset]) & m0;
    __v16qi la1 = _mm_load_si128(&b.longAttack[b.CI][offset-0x10]) & m1;
//B bit 2->2, R bit 0->1, Q bit 4->3
    la0 |= (la0 + la0) | (__v16qi)_mm_srli_epi16(la0, nRBits + nBBits - 3) | sa0;
    la1 |= (la1 + la1) | (__v16qi)_mm_srli_epi16(la1, nRBits + nBBits - 3) | sa1;
    la0 &= low4bits;
    la1 &= low4bits;
    la0 = _mm_shuffle_epi8(mateBits, la0);
    la1 = _mm_shuffle_epi8(mateBits, la1);
    la0 = _mm_slli_epi16(la0, 4);
    la1 = _mm_slli_epi16(la1, 4);
    __v16qi ed0 = zero == ((_mm_load_si128(&b.shortAttack[b.EI][offset]) | _mm_load_si128(&b.longAttack[b.EI][offset])) & m0);
    __v16qi ed1 = zero == ((_mm_load_si128(&b.shortAttack[b.EI][offset-0x10]) | _mm_load_si128(&b.longAttack[b.EI][offset-0x10])) & m1);
#endif
    
/*    p4_0 |= (reinterpret_cast<uint32_t&>(b.longAttack[(1-C)/2][oking-1]) & (attackMaskLong | attackMaskLong << 8 | attackMaskLong << 16));
    p4_0 |= borderTab4_0[oking];
    uint32_t p321 = reinterpret_cast<uint32_t&>(b.pieces[oking+7]) | borderTab321[oking];
    uint32_t p567 = reinterpret_cast<uint32_t&>(b.pieces[oking-9]) | borderTab567[oking];
    uint64_t king8 = (p321 & 0xffffff) + ((p4_0 & 0xffL) << 24) + ((p4_0 & 0xff0000L) << 32) + ((p567 & 0xffffffL) << 40);*/
}
