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
#include <random>
#include "eval.h"
#include "boardbase.h"
#include "options.h"
#include "transpositiontable.tcc"
//#include <cstdio>

static const __v2di zero = {0};
static const __v2di mask01 = { 0x0101010101010101, 0x0202020202020202 };
static const __v2di mask23 = { 0x0404040404040404, 0x0808080808080808 };
static const __v2di mask45 = { 0x1010101010101010, 0x2020202020202020 };
static const __v2di mask67 = { 0x4040404040404040, 0x8080808080808080 };
static const int logEndgameTransitionSlope = 4; //16 pieces material between full endgame and full opening
static const int baseAttack = 512;
static const int maxAttack = 712;
static const int baseDefense = 256;
static const int maxDefense = 356;

int mobB1[64], mobB2[64], mobB3[64], mobN1[64], mobN2[64], mobN3[64], mobR1[64], mobR2[64], mobR3[64], mobQ1[64], mobQ2[64], mobQ3[64];
int attackTable[256], defenseTable[256];
int pawnFileOpen[4], pawnFileEnd[4];
int pawnRankOpen[6], pawnRankEnd[6];
int8_t wpawnOpen[64], wpawnEnd[64];
int8_t bpawnOpen[64], bpawnEnd[64];

#if !defined(__SSE_4_2__)
// popcount, which counts at most 15 ones, for counting pawns
static inline int popcount15( uint64_t x )
{
    x -=  x>>1 & 0x5555555555555555LL;
    x  = ( x>>2 & 0x3333333333333333LL ) + ( x & 0x3333333333333333LL );
    x *= 0x1111111111111111LL;
    return  x>>60;
}

static inline int popcount3( uint64_t x )
{
    x -=  x>>1 & 0x5555555555555555LL;
    x *= 0x5555555555555555LL;
    return  x>>62;
}

// special popcount, assuming each 2-bit block has max one bit set, for counting light/dark squares.
static inline int popcount2( uint64_t x )
{
    x -=  x>>1 & 0x5555555555555555LL;
    x  = (( x>>2 )+x) & 0x3333333333333333LL;
    x  = (( x>>4 )+x) & 0x0f0f0f0f0f0f0f0fLL;
    x *= 0x0101010101010101LL;
    return  x>>56;
}

#else
#define popcount15(x) __builtin_popcountll(x)
#define popcount3(x) __builtin_popcountll(x)
#define popcount2(x) __builtin_popcountll(x)
#endif

static inline int weightedPopcount(const uint64_t bb, const int8_t weights[64]) __attribute__((always_inline));
static inline int weightedPopcount(const uint64_t bb, const int8_t weights[64]) {

    __v2di x0 = _mm_set1_epi64x(bb);
    __v2di x3 = _mm_andnot_si128(x0, mask67);
    __v2di x2 = _mm_andnot_si128(x0, mask45);
    __v2di x1 = _mm_andnot_si128(x0, mask23);
    x0 = _mm_andnot_si128(x0, mask01);

    x3 = _mm_cmpeq_epi8(x3, zero);
    x2 = _mm_cmpeq_epi8(x2, zero);
    x1 = _mm_cmpeq_epi8(x1, zero);
    x0 = _mm_cmpeq_epi8(x0, zero);

    x3 &= ((const __v2di*)weights)[3];
    x2 &= ((const __v2di*)weights)[2];
    x1 &= ((const __v2di*)weights)[1];
    x0 &= ((const __v2di*)weights)[0];

    __v2di s = _mm_sad_epu8(x3+x2+x1+x0, zero);
    return _mm_extract_epi16(s,0) + _mm_extract_epi16(s,4);
}

Eval::Eval():
    pawnPasser{ 0, 50, 50, 60, 80, 115, 160, 0 }
{
    pt = new TranspositionTable<PawnEntry, 4, PawnKey>;
//    pt->setSize(0x100000000);
    pawn = 90;
    knight = 325;   
    bishop = 325;   
    rook = 500;
    queen = 975;    

    bishopPair = 50;
    bishopBlockPasser = 20;
    bishopAlone = -100;
    
    knightAlone = -125;
    knightBlockPasser = 30;

    rookTrapped = -50;
    rookOpen = 8;
    rookHalfOpen = 6;

    pawnBackward = -12;
    pawnBackwardOpen = -12;
    pawnIsolatedCenter = -16;// - Eval::pawnBackwardOpen;
    pawnIsolatedOpen = -8;// - Eval::pawnBackwardOpen;
    pawnIsolatedEdge = -8;// - Eval::pawnBackwardOpen;
    pawnEdge = -12;
    pawnCenter = 10;
    pawnDouble = -25;
    pawnShoulder = 4;
    pawnHole = -8;
    pawnConnPasser = 100;
    pawnUnstoppable = 100;

    attack1b1 = 4;        //direct attack of ring around king
    attack2b1 = 2;        //direct attack of 2nd ring around king
    attack1b2 = 2;        //indirect attack of ring around king
    attack2b2 = 1;        //indirect attack of 2nd ring around king
    attack1n1 = 4;
    attack2n1 = 2;
    attack1n2 = 2;
    attack2n2 = 1;
    attack1r1 = 6;
    attack2r1 = 2;
    attack1r2 = 3;
    attack2r2 = 1;
    attack1q1 = 8;
    attack2q1 = 4;
    attack1q2 = 4;
    attack2q2 = 2;
    attack1k1 = 3;        //king is not able to deliver mate
    attack2k1 = 1;
    attack1p1 = 4;
    attack2p1 = 1;

    endgameMaterial = 28;

    aspiration0 = 40;
    aspiration1 = 5;
    evalHardBudget = -20;

    sigmoid(mobN1, -25, 25, 0, 2.0);
    sigmoid(mobN2, -20, 20, 4, 5.0);
    sigmoid(mobN3, -5, 5, 0, 7.0);

    sigmoid(mobB1, -20, 20, 0, 2.5);
    sigmoid(mobB2, -25, 25, 4, 6.0);
    sigmoid(mobB3, -5, 5, 0, 2.0);
    
    sigmoid(mobR1, -15, 15, 0, 4);
    sigmoid(mobR2, -35, 35, 16, 8.0);
    sigmoid(mobR3, -3, 3, 0, 5.0);

    sigmoid(mobQ1, -10, 10, 0, 5);
    sigmoid(mobQ2, -20, 20, 10, 10);
    sigmoid(mobQ3, -3, 3, 0, 2.5);

    sigmoid(attackTable, baseAttack, maxAttack, 40, 30);
    sigmoid(defenseTable, maxDefense, baseDefense, 8, 4);

    sigmoid(pawnFileOpen, 0, 20, 2);
    sigmoid(pawnFileEnd, 30, 0, 1);
    sigmoid(pawnRankOpen, 0, 20, 6);
    sigmoid(pawnRankEnd, 0, 30, 6);

    for (unsigned x=0; x<4; ++x)
        for (unsigned y=1; y<7; ++y) {
            bpawnOpen[(7-x)*8 + 7-y] = bpawnOpen[    x*8 + 7-y] = wpawnOpen[(7-x)*8 + y] = wpawnOpen[    x*8 + y]
            = (pawn + pawnFileOpen[x])*(pawn + pawnRankOpen[y-1]) / pawn - pawn;
            bpawnEnd [(7-x)*8 + 7-y] = bpawnEnd [    x*8 + 7-y] = wpawnEnd [(7-x)*8 + y] = wpawnEnd [    x*8 + y]
            = (pawn + pawnFileEnd[x])*(pawn + pawnRankEnd[y-1]) / pawn - pawn;
        }
        
    initPS();
    initZobrist();
    initTables();

#ifndef NDEBUG
    printSigmoid(pawnFileOpen, "pfo");
    printSigmoid(pawnFileEnd, "pfe");
    for (unsigned y=0; y<8; ++y) {
        for (unsigned x=0; x<8; ++x) 
            std::cerr << std::setw(3) << (int)wpawnOpen[x*8+y];
        std::cerr << "       ";
        for (unsigned x=0; x<8; ++x)
            std::cerr << std::setw(3) << (int)bpawnOpen[x*8+y];
        std::cerr << std::endl;
    }
    for (unsigned y=0; y<8; ++y) {
        for (unsigned x=0; x<8; ++x)
            std::cerr << std::setw(3) << (int)wpawnEnd[x*8+y];
        std::cerr << "       ";
        for (unsigned x=0; x<8; ++x)
            std::cerr << std::setw(3) << (int)bpawnEnd[x*8+y];
        std::cerr << std::endl;
    }
#endif
}

void Eval::initPS() {
    // highest rank (eight) comes first, for convenience.
    // while initializing it is flipped top-down.
/*    RawScore rook[2][nSquares] = {{
 4,  4, 10, 12, 12, 10,  4,  4,
12, 12, 18, 20, 20, 18, 12, 12,
 5,  5, 11, 13, 13, 11,  5,  5,
 3,  3,  9, 11, 11,  9,  3,  3,
 2,  2,  8, 10, 10,  8,  2,  2,
 1,  1,  7,  9,  9,  7,  1,  1,
 0,  0,  6,  8,  8,  6,  0,  0,
 0,  0,  6,  8,  8,  6,  0,  0,
},{
 0, 10, 11, 12, 12, 11, 10,  0,
10, 20, 21, 22, 22, 21, 20, 10,
11, 21, 22, 23, 23, 22, 21, 11,
12, 22, 23, 24, 24, 23, 22, 12,
12, 22, 23, 24, 24, 23, 22, 12,
11, 21, 22, 23, 23, 22, 21, 11,
10, 20, 21, 22, 22, 21, 20, 10,
 0, 10, 11, 12, 12, 11, 10,  0
}};

    RawScore bishop[2][nSquares] = {{
 0,  4,  8,  7,  7,  8,  4,  0,
 4,  8, 12, 11, 11, 12,  8,  4,
 8, 12, 16, 15, 15, 16, 12,  8,
 8, 12, 16, 15, 15, 16, 12,  8,
 8, 12, 16, 15, 15, 16, 12,  8,
 8, 12, 16, 15, 15, 16, 12,  8,
 4,  8, 12, 11, 11, 12,  8,  4,
 0,  4,  8,  7,  7,  8,  4,  0
},{
 0,  3,  6,  9,  9,  6,  3,  0,
 3,  6,  9, 12, 12,  9,  6,  3,
 6,  9, 12, 15, 15, 12,  9,  6,
 9, 12, 15, 18, 18, 15, 12,  9,
 9, 12, 15, 18, 18, 15, 12,  9,
 6,  9, 12, 15, 15, 12,  9,  6,
 3,  6,  9, 12, 12,  9,  6,  3,
 0,  3,  6,  9,  9,  6,  3,  0
}};

    RawScore queen[2][nSquares] = {{
 0,  8, 10, 12, 12, 10,  8,  0,
10, 18, 20, 22, 22, 20, 18, 10,
12, 20, 22, 24, 24, 22, 20, 12,
13, 21, 23, 25, 25, 23, 21, 13,
12, 20, 22, 24, 24, 22, 20, 12,
10, 18, 20, 22, 22, 20, 18, 10,
 8, 16, 18, 20, 20, 18, 16,  8,
 0,  8, 10, 12, 12, 10,  8,  0
},{
 0,  6, 10, 12, 12, 10,  6,  0,
 6, 12, 16, 18, 18, 16, 12,  6,
10, 16, 20, 22, 22, 20, 16, 10,
12, 18, 22, 24, 24, 22, 18, 12,
12, 18, 22, 24, 24, 22, 18, 12,
10, 16, 20, 22, 22, 20, 16, 10,
 6, 12, 16, 18, 18, 16, 12,  6,
 0,  6, 10, 12, 12, 10,  6,  0
}};

    RawScore knight[2][nSquares] = {{
 0,  4,  6,  8,  8,  6,  4,  0,
 4, 10, 14, 17, 17, 14, 10,  4,
 8, 16, 22, 25, 25, 22, 16,  8,
 7, 14, 21, 23, 23, 21, 14,  7,
 6, 12, 18, 20, 20, 18, 12,  6,
 5, 10, 15, 15, 15, 15, 10,  5,
 3,  5, 10, 10, 10, 10,  5,  3,
 0,  2,  4,  6,  6,  4,  2,  0
},{
 0,  6,  8, 10, 10,  8,  6,  0,
 6, 12, 14, 16, 16, 14, 12,  6,
 8, 14, 16, 18, 18, 16, 14,  8,
10, 16, 18, 20, 20, 18, 16, 10,
10, 16, 18, 20, 20, 18, 16, 10,
 8, 14, 16, 18, 18, 16, 14,  8,
 6, 12, 14, 16, 16, 14, 12,  6,
 0,  6,  8, 10, 10,  8,  6,  0
}};

    RawScore pawn[2][nSquares] = {{
 0,  0,  0,  0,  0,  0,  0,  0,
10, 22, 26, 30, 30, 26, 22, 10,
 6, 18, 22, 26, 26, 22, 18,  6,
 3, 15, 19, 23, 23, 19, 15,  3,
 1, 13, 17, 21, 21, 17, 13,  1,
 0, 12, 16, 20, 20, 16, 12,  0,
 0, 12, 16, 20, 20, 16, 12,  0,
 0,  0,  0,  0,  0,  0,  0,  0
},{
 0,  0,  0,  0,  0,  0,  0,  0,
10, 26, 28, 30, 30, 28, 26, 10,
 6, 22, 24, 26, 26, 24, 22,  6,
 3, 19, 21, 23, 23, 21, 19,  3,
 1, 17, 19, 21, 21, 19, 17,  1,
 0, 16, 18, 20, 20, 18, 16,  0,
 0, 16, 18, 20, 20, 18, 16,  0,
 0,  0,  0,  0,  0,  0,  0,  0
}};

    RawScore king[2][nSquares] = {{
-90, -90, -95, -95, -95, -95, -90, -90,
-90, -90, -95, -95, -95, -95, -90, -90,
-90, -90, -95, -95, -95, -95, -90, -90,
-40, -40, -45, -50, -50, -45, -40, -40,
-40, -40, -40, -40, -40, -40, -40, -40, 
-40, -40, -40, -40, -40, -40, -40, -40, 
 10,  10,   5, -20, -20,   5,  10,  10,
 10,  10,   5,   0,   0,   5,  10,  10
},{
 0,  4,  8, 12, 12,  8,  4,  0,
 9, 13, 17, 21, 21, 17, 13,  9,
18, 22, 26, 30, 30, 26, 22, 18,
17, 21, 25, 29, 29, 25, 21, 17,
14, 18, 22, 26, 26, 22, 18, 14,
10, 14, 18, 22, 22, 18, 14, 10,
 5,  9, 13, 17, 17, 13,  9,  5,
 0,  4,  8, 12, 12,  8,  4,  0
}};*/

    for (unsigned int sq = 0; sq<nSquares; ++sq)
        getPS( 0, sq) = CompoundScore{ 0, 0 };

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Pawn, sq) = Eval::pawn;/* + CompoundScore{ pawn[Opening][sq ^ 0x38], pawn[Endgame][sq ^ 0x38] };*/
        getPS(-Pawn, sq ^ 070) = -getPS( Pawn, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Rook, sq)  = Eval::rook;/* + CompoundScore{ rook[Opening][sq ^ 0x38], rook[Endgame][sq ^ 0x38] };*/
        getPS(-Rook, sq ^ 070) = -getPS( Rook, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Bishop, sq) = Eval::bishop;/* + CompoundScore{ bishop[Opening][sq ^ 0x38], bishop[Endgame][sq ^ 0x38] };*/
        getPS(-Bishop, sq ^ 070) = -getPS( Bishop, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Knight, sq) = Eval::knight;/* + CompoundScore{ knight[Opening][sq ^ 0x38], knight[Endgame][sq ^ 0x38] };*/
        getPS(-Knight, sq ^ 070) = -getPS( Knight, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Queen, sq) = Eval::queen;/*+ CompoundScore{ queen[Opening][sq ^ 0x38], queen[Endgame][sq ^ 0x38] };*/
        getPS(-Queen, sq ^ 070) = -getPS( Queen, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( King, sq) = 0;/*CompoundScore{ king[Opening][sq ^ 0x38], king[Endgame][sq ^ 0x38] };*/
        getPS(-King, sq ^ 070) = -getPS( King, sq);
    }
}

void Eval::initZobrist() {
    std::mt19937 rng;
    
    rng.seed(1);
    for (int p = -nPieces; p <= (signed int)nPieces; p++)
        for (unsigned int i = 0; i < nSquares; ++i) {
            uint64_t r;
            do {
                r = rng() + ((uint64_t)rng() << 32);                    
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
    /*    for (unsigned int x = 0; x<nFiles; ++x)
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
        }*/
}

template<Colors C>
int Eval::pieces(const BoardBase& b, const PawnEntry& p) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    int value = 0;
    if (popcount3(b.getPieces<C, Bishop>()) >= 2) {
        value += bishopPair;
    }
    if  (   (  b.getPieces<C, Rook>() & (file<'h'>()|file<'g'>()) & (rank<C,1>()|rank<C,2>()) 
            && b.getPieces<C, Pawn>() & file<'h'>() & (rank<C,2>() | rank<C,3>())
            &&  (   b.getPieces<C, King>() & file<'g'>() & rank<C,1>()
                || (b.getPieces<C, King>() & file<'f'>() & rank<C,1>() && b.getPieces<C, Pawn>() & file<'g'>() & (rank<C,2>() | rank<C,3>()))))
        ||  (  b.getPieces<C, Rook>() & (file<'a'>()|file<'b'>()) & (rank<C,1>()|rank<C,2>()) 
            && b.getPieces<C, Pawn>() & file<'a'>() & (rank<C,2>() | rank<C,3>())
            &&  (   b.getPieces<C, King>() & file<'b'>() & rank<C,1>()
                || (b.getPieces<C, King>() & file<'c'>() & rank<C,1>() && b.getPieces<C, Pawn>() & file<'b'>() & (rank<C,2>() | rank<C,3>())))))
            value += rookTrapped;
            
    uint64_t own = p.openFiles[CI];
    own += own << 8;
    own += own << 16;
    own += own << 32;
    
    uint64_t opp = p.openFiles[EI];
    opp += opp << 8;
    opp += opp << 16;
    opp += opp << 32;

    value += rookHalfOpen * popcount3(b.getPieces<C,Rook>() & own);
    value += rookOpen * popcount3(b.getPieces<C,Rook>() & own & opp);

    value += knightBlockPasser * popcount3(b.getPieces<C,Knight>() & shift<C*8>(p.passers[EI]));
    value += bishopBlockPasser * popcount3(b.getPieces<C,Bishop>() & shift<C*8>(p.passers[EI]));

    return value;
}

template<Colors C>
PawnEntry::Shield evalShield(const BoardBase &b) {

    uint64_t p = b.getPieces<C,Pawn>();
    uint64_t kside = file<'f'>() | file<'g'>() | file<'h'>();
    uint64_t qside = file<'a'>() | file<'b'>() | file<'c'>();

    unsigned kshield = 4*popcount(kside & rank<C,2>() & p)
                       + 2*popcount(kside & rank<C,3>() & p)
                       + 1*popcount(kside & rank<C,4>() & p)
                       + 2*!!(kside & file<'h'>())
                       + 2*!!(kside & file<'g'>());

    unsigned qshield = 4*popcount(qside & rank<C,2>() & p)
                       + 2*popcount(qside & rank<C,3>() & p)
                       + 1*popcount(qside & rank<C,4>() & p)
                       + 2*!!(qside & file<'a'>())
                       + 2*!!(qside & file<'b'>());

    PawnEntry::Shield ret;
//    ret.weakLight = 0;
//    ret.weakDark = 0;
    ret.qside = qshield;
    ret.kside = kshield;
/*#ifdef MYDEBUG
    if (debug) {
        std::cout << "kshield:        " << kshield << std::endl;
        std::cout << "qshield:        " << qshield << std::endl;
    }
#endif*/
    return ret;
}
//TODO asess blocked pawns and their consequences on an attack
PawnEntry Eval::pawns(const BoardBase& b) const {
    PawnKey k=b.keyScore.pawnKey;
    Sub<PawnEntry, 4>* st = pt->getSubTable(k);
    PawnEntry pawnEntry;
    if (k >> PawnEntry::upperShift && pt->retrieve(st, k, pawnEntry)) {
        stats.pthit++;
    } else {
        stats.ptmiss++;
        pawnEntry.score = 0;
        uint64_t wpawn = b.getPieces<White,Pawn>();
        uint64_t bpawn = b.getPieces<Black,Pawn>();

        int w0 = weightedPopcount(wpawn, wpawnOpen);
        int b0 = weightedPopcount(bpawn, bpawnOpen);
        pawnEntry.centerOpen = w0-b0;
        print_debug(debugEval, "pawnCenterOpen:%3d/%3d\n", w0, b0);
        
        w0 = weightedPopcount(wpawn, wpawnEnd);
        b0 = weightedPopcount(bpawn, bpawnEnd);
        pawnEntry.centerEnd = w0-b0;
        print_debug(debugEval, "pawnCenterEnd:%3d/%3d\n", w0, b0);

        uint64_t wpawnShoulder = wpawn & wpawn<<1 & ~file<'a'>();
        uint64_t bpawnShoulder = bpawn & bpawn<<1 & ~file<'a'>();
        pawnEntry.score += pawnShoulder * popcount15(wpawnShoulder);
        pawnEntry.score -= pawnShoulder * popcount15(bpawnShoulder);
        print_debug(debugEval, "wpawnShoulder: %3d\n", pawnShoulder * popcount15(wpawnShoulder));
        print_debug(debugEval, "bpawnShoulder: %3d\n", pawnShoulder * popcount15(bpawnShoulder));
        
        uint64_t wpawnHole = wpawn & wpawn<<2 & ~file<'a'>() & ~file<'b'>();
        uint64_t bpawnHole = bpawn & bpawn<<2 & ~file<'a'>() & ~file<'b'>();
        pawnEntry.score += pawnHole * popcount15(wpawnHole);
        pawnEntry.score -= pawnHole * popcount15(bpawnHole);
        print_debug(debugEval, "wpawnHole:     %3d\n", pawnHole * popcount15(wpawnHole));
        print_debug(debugEval, "bpawnHole:     %3d\n", pawnHole * popcount15(bpawnHole));
        
        uint64_t wFront = wpawn << 8 | wpawn << 16;
        wFront |= wFront << 16 | wFront << 32;
        uint64_t wBack = wpawn >> 8 | wpawn >> 16;
        wBack |= wBack >> 16 | wBack >> 32;

        uint64_t bBack = bpawn << 8 | bpawn << 16;
        bBack |= bBack << 16 | bBack << 32;
        uint64_t bFront = bpawn >> 8 | bpawn >> 16;
        bFront |= bFront >> 16 | bFront >> 32;

        pawnEntry.score += pawnDouble*popcount15(wpawn & wBack);
        pawnEntry.score -= pawnDouble*popcount15(bpawn & bBack);
        print_debug(debugEval, "wpawnDouble:   %3d\n", pawnDouble*popcount15(wpawn & wBack));
        print_debug(debugEval, "bpawnDouble:   %3d\n", pawnDouble*popcount15(bpawn & bBack));
        // calculate squares which are or possibly are attacked by w and b pawns
        // take only the most advanced attacker and the most backward defender into account

        uint64_t wAttack = (wFront >> 1 & ~0x8080808080808080LL) | (wFront << 1 & ~0x101010101010101LL);
        uint64_t bAttack = (bFront >> 1 & ~0x8080808080808080LL) | (bFront << 1 & ~0x101010101010101LL);

        // backward pawns are pawns which may be attacked, if the advance,
        // but are not on a contested square (otherwise they would be defended)

        uint64_t wBackward = (wpawn<<8 & b.getAttacks<Black,Pawn>() & ~wAttack) >> 8;
        uint64_t bBackward = (bpawn>>8 & b.getAttacks<White,Pawn>() & ~bAttack) << 8;

        int pbw = pawnBackward * (popcount15(wBackward) - popcount15(bBackward));
        int pbwo = pawnBackwardOpen * (popcount15(wBackward & ~bFront) - popcount15(bBackward & ~wFront));
        print_debug(debugEval, "pawn backward: %3d\n", pbw);
        print_debug(debugEval, "pawn bwo:      %3d\n", pbwo);
        pawnEntry.score += pbw + pbwo;
        // a white pawn is not passed, if he is below a opponent pawn or its attack squares
        // store positions of passers for later use in other eval functions

        uint64_t wNotPassedMask = bFront | bAttack;
        uint64_t bNotPassedMask = wFront | wAttack;
        uint64_t wPassed = wpawn & ~wNotPassedMask & ~wBack;
        uint64_t bPassed = bpawn & ~bNotPassedMask & ~bBack;
        unsigned i;
        pawnEntry.passers[0] = wPassed;
        for (i = 0; wPassed; wPassed &= wPassed - 1, i++) {
            int pos = __builtin_ctzll(wPassed);
            int y = pos >> 3;
            pawnEntry.score += pawnPasser[y];
            print_debug(debugEval, "pawn wpasser:   %d\n", pawnPasser[y]);
        }
        pawnEntry.passers[1] = bPassed;
        for ( i=0; bPassed; bPassed &= bPassed-1, i++ ) {
            int pos = __builtin_ctzll(bPassed);
            int y = pos>>3;
            pawnEntry.score -= pawnPasser[7-y];
            print_debug(debugEval, "pawn bpasser:   %d\n", pawnPasser[7-y]);
        }

        // possible weak pawns are adjacent to open files or below pawns an a adjacent file on both sides
        // rook pawns are always adjacent to a "open" file
        uint64_t wOpenFiles = ~(wFront | wpawn | wBack);
        uint64_t bOpenFiles = ~(bFront | bpawn | bBack);
        pawnEntry.openFiles[0] = wOpenFiles;
        pawnEntry.openFiles[1] = bOpenFiles;
        
        uint64_t wIsolani = wpawn & (wOpenFiles<<1 | 0x101010101010101LL) & (wOpenFiles>>1 | 0x8080808080808080LL);
        uint64_t bIsolani = bpawn & (bOpenFiles<<1 | 0x101010101010101LL) & (bOpenFiles>>1 | 0x8080808080808080LL);
        pawnEntry.score += pawnIsolatedCenter * (popcount15(wIsolani & ~(file<'a'>()|file<'h'>())));
        pawnEntry.score -= pawnIsolatedCenter * (popcount15(bIsolani & ~(file<'a'>()|file<'h'>())));
        print_debug(debugEval, "wpawn ciso:     %3d\n", pawnIsolatedCenter * (popcount15(wIsolani & ~(file<'a'>()|file<'h'>()))));
        print_debug(debugEval, "bpawn ciso:     %3d\n", pawnIsolatedCenter * (popcount15(bIsolani & ~(file<'a'>()|file<'h'>()))));
        pawnEntry.score += pawnIsolatedEdge * (popcount15(wIsolani & (file<'a'>()|file<'h'>())));
        pawnEntry.score -= pawnIsolatedEdge * (popcount15(bIsolani & (file<'a'>()|file<'h'>())));
        print_debug(debugEval, "wpawn eiso:     %3d\n", pawnIsolatedEdge * (popcount15(wIsolani & (file<'a'>()|file<'h'>()))));
        print_debug(debugEval, "bpawn eiso:     %3d\n", pawnIsolatedEdge * (popcount15(bIsolani & (file<'a'>()|file<'h'>()))));
        pawnEntry.score += pawnIsolatedOpen * (popcount15(wIsolani & ~bFront));
        pawnEntry.score -= pawnIsolatedOpen * (popcount15(bIsolani & ~wFront));
        print_debug(debugEval, "wpawn oiso:     %3d\n", pawnIsolatedOpen * (popcount15(wIsolani & ~bFront)));
        print_debug(debugEval, "bpawn oiso:     %3d\n", pawnIsolatedOpen * (popcount15(bIsolani & ~wFront)));
        
        pawnEntry.shield[0] = evalShield<White>(b);
        pawnEntry.shield[1] = evalShield<Black>(b);
        pawnEntry.upperKey = k >> PawnEntry::upperShift;
        if (k >> PawnEntry::upperShift)
            pt->store(st, pawnEntry);
    }
    return pawnEntry;
}

// double popcount of two quadwords
__v2di pop2count(__v2di x) {

    const __v16qi mask4 = {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf};
    const __v16qi count4 = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

    asm (
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

// double popcount of two quadwords
/*void pop4count(__v2di x01, __v2di x23, int& p0, int& p1, int& p2, int& p3) {

    const __v16qi mask4 = {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf};
    const __v16qi count4 = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

    __v8hi t01 = x01;
    __v8hi t23 = x23;
    t01 = _mm_srli_epi16(t01, 4);
    x01 = (__v16qi)x01 & mask4;
    t01 = (__v16qi)t01 & mask4;
    __v16qi c0 = count4;
    __v16qi c1 = count4;
    c0 = _mm_shuffle_epi8( x01, c0);
    c1 = _mm_shuffle_epi8( t01, c1);
    c1 += c0;
    c0 ^= c0;
    c0 = _mm_sad_epu8(c1, c0);*/
/*    asm(
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
    return x;*/
//}

int look2up(__v2di x, __v16qi tab) {
    __v8hi y = _mm_shuffle_epi8(tab, x);
    return _mm_extract_epi16(y, 0) + _mm_extract_epi16(y, 4);
}

void printBit(uint64_t bits) {
    for (int y=8; y; ) {
        --y;
        for (int x=0; x<8; ++x) {
            if ((bits >> (x+y*8)) & 1)
                std::cout << "X";
            else
                std::cout << ".";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

template<unsigned int I>
uint64_t extract(const __v2di &) {
    return 0;
    //return _mm_extract_epi64(v, I);
}

void printBit(__v2di bits) {
    for (int y=8; y; ) {
        --y;
        for (int x=0; x<8; ++x) {
            if ((extract<0>(bits) >> (x+y*8)) & 1)
                std::cout << "X";
            else
                std::cout << ".";
        }
        std::cout << " ";
        for (int x=0; x<8; ++x) {
            if ((extract<1>(bits) >> (x+y*8)) & 1)
                std::cout << "X";
            else
                std::cout << ".";
        }
        std::cout << std::endl;

    }
    std::cout << std::endl;
}

static const __v2di weightB1[4] = {
    { 0x0707070707070707,       //a-file
      0x0709090909090907 },     //b-file
    { 0x07090b0b0b0b0907,       //c-file
      0x07090b0d0d0b0907 },     //d-file
    { 0x07090b0d0d0b0907,       //e-file
      0x07090b0b0b0b0907 },     //f-file
    { 0x0709090909090907,       //g-file
      0x0707070707070707 }      //h-file
};

template<Colors C>
inline int Eval::mobility( const BoardBase &b, unsigned& attackingPieces, unsigned& defendingPieces) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    int score = 0;
    int king = bit(b.getPieces<-C,King>());
    const uint64_t oppking1 = b.kAttacked1[king];
    const uint64_t oppking2 = b.kAttacked2[king];
    const uint64_t ownking = b.getAttacks< C,King>() | shift<C* 8>(b.getAttacks< C,King>());
    const uint64_t noBlockedPawns = ~(b.getPieces<C,Pawn>() & shift<C*-8>(b.getPieces<-C,Pawn>()));
    const __v2di noBlockedPawns2 = _mm_set1_epi64x(noBlockedPawns);
    const uint64_t empty = ~b.getOcc<C>();
    const __v2di empty2 = _mm_set1_epi64x(empty);
    
    __v2di w0 = zero, w1 = zero, w2 = zero, w3 = zero;
    
    const __v2di v2queen = _mm_set1_epi64x(b.getPieces<C,Queen>());
    
    uint64_t restrictions = ~b.getAttacks<-C,Pawn>();
    __v2di restrictions2 = _mm_set1_epi64x(restrictions);
    __v2di allBishopAttacks = zero;
    //TODO generally optimize for at most two pieces of a kind. In positions with more the evaluation will most likely be way off anyway.
    if (b.bsingle[CI][0].move.data) {
#if 0        
        if (0 && b.bsingle[CI][1].move.data) {
            allBishopAttacks |= b.bsingle[CI][0].d13 | b.bsingle[CI][1].d13;
            __v2di rmob0 = _mm_set_epi64x( fold(b.bsingle[CI][0].d13), fold(b.bsingle[CI][1].d13) ) ;
            __v2di rmob1 = rmob0 &  restrictions2;
            // generator for two move squares, mask blocked pawns
            __v2di rmob2 = rmob1 & noBlockedPawns2;
            // remove own pieces
            rmob1 &= empty2;
            // restriced mobility in two moves, including own defended pieces, excluding pieces defended in the 2nd move        
            rmob2 |= b.build13Attack(rmob1) & restrictions2 & empty2;
            // generator for three move squares, mask blocked pawns
            __v2di rmob3 = rmob2 & noBlockedPawns2;
            // restriced mobility in three moves
            rmob3 |= b.build13Attack(rmob2) & restrictions2 & empty2;
            /*
            * If the queen is on an attack line of the bishop, include the attack
            * line of the queen in the attack line of the bishop. This leads to
            * increased one move mobility and the bishop gets counted in an possible
            * king attack
            */
            rmob1 |= _mm_set_epi64x( fold( ~pcmpeqq(b.bsingle[CI][0].d13 & v2queen, zero) & b.qsingle[CI][0].d13),
                                    fold( ~pcmpeqq(b.bsingle[CI][1].d13 & v2queen, zero) & b.qsingle[CI][0].d13) )
                    & empty2; 
            rmob2 &= ~rmob1;
            rmob3 &= ~(rmob1 | rmob2);

            __v2di oppking0 = _mm_set_epi64x(oppking, 0);
            __v2di ownking0 = _mm_set_epi64x(ownking, 0);
            if (!_mm_testz_si128(rmob1, oppking0)) attackingPieces+=4;
            if (!_mm_testz_si128(rmob0, ~rmob1 & oppking0)) attackingPieces+=2;
            if (!_mm_testz_si128(rmob2, oppking0)) attackingPieces+=2;
            if (!_mm_testz_si128(rmob3, oppking0)) attackingPieces++;
            if (!_mm_testz_si128(rmob0, ownking0)) defendingPieces++;
            
            __v2di oppking1 = _mm_set_epi64x(0, oppking);
            __v2di ownking1 = _mm_set_epi64x(0, ownking);
            if (!_mm_testz_si128(rmob1, oppking1)) attackingPieces+=4;
            if (!_mm_testz_si128(rmob0, ~rmob1 & oppking1)) attackingPieces+=2;
            if (!_mm_testz_si128(rmob2, oppking1)) attackingPieces+=2;
            if (!_mm_testz_si128(rmob3, oppking1)) attackingPieces++;
            if (!_mm_testz_si128(rmob0, ownking1)) defendingPieces++;
            
    /*        sumup(rmob1, weightB1, w0, w1, w2, w3);
            sumup(rmob2, weightB2, w0, w1, w2, w3);
            sumup(rmob3, weightB3, w0, w1, w2, w3);*/
            
            score += mobB1[popcount(_mm_extract_epi64(rmob1, 0))] + mobB2[popcount(_mm_extract_epi64(rmob2, 0))] + mobB3[popcount(_mm_extract_epi64(rmob3, 0))];
            score += mobB1[popcount(_mm_extract_epi64(rmob1, 1))] + mobB2[popcount(_mm_extract_epi64(rmob2, 1))] + mobB3[popcount(_mm_extract_epi64(rmob3, 1))];

            //print_debug(debugMobility, "b mobility%d: %d, %d, %d\n", CI, popcount(rmob1), popcount(rmob2), popcount(rmob3));
            if(TRACE_DEBUG && Options::debug & debugMobility) { printBit(rmob1); printBit(rmob2); printBit(rmob3); }
        } else {
#endif            
        for (const BoardBase::MoveTemplateB* bs = b.bsingle[CI]; bs->move.data; ++bs) {
            // restriced mobility in one move, including own defended pieces
            allBishopAttacks |= bs->d13;
            uint64_t rmob0 = fold(bs->d13);
            uint64_t rmob1 = rmob0 & restrictions;
            // generator for two move squares, mask blocked pawns
            uint64_t rmob2 = rmob1 & noBlockedPawns;
            // remove own pieces
            rmob1 &= ~b.getOcc<C>();
            // restriced mobility in two moves, including own defended pieces, excluding pieces defended in the 2nd move        
            rmob2 |= b.build13Attack(rmob1) & restrictions & ~b.getOcc<C>();
            // generator for three move squares, mask blocked pawns
//                 uint64_t rmob3 = rmob2 & noBlockedPawns;
            // restriced mobility in three moves
//                 rmob3 |= b.build13Attack(rmob2) & restrictions & ~b.getOcc<C>();
            /*
            * If the queen is on an attack line of the bishop, include the attack
            * line of the queen in the attack line of the bishop. This leads to
            * increased one move mobility and the bishop gets counted in an possible
            * king attack
            */
            rmob1 |= fold( ~ pcmpeqq(bs->d13 & v2queen, zero) & b.qsingle[CI][0].d13) & ~b.getOcc<C>(); //TODO optimize for queenless positions
            rmob2 &= ~rmob1;
//                 rmob3 &= ~(rmob1 | rmob2);

            attackingPieces += attack1b1 * popcount15(rmob0 & oppking1);
            attackingPieces += attack2b1 * popcount15(rmob0 & oppking2);
            attackingPieces += attack1b2 * popcount15(rmob2 & oppking1);
            attackingPieces += attack2b2 * popcount15(rmob2 & oppking2);
        print_debug(debugMobility, "b attack%d: %d, %d, %d, %d\n", CI, attack1b1 * popcount15(rmob0 & oppking1), attack2b1 * popcount15(rmob0 & oppking2), attack1b2 * popcount15(rmob2 & oppking1), attack2b2 * popcount15(rmob2 & oppking2));
//                 if (rmob3 & oppking) attackingPieces++;
            if (rmob0 & ownking) defendingPieces++;
            
    /*        sumup(rmob1, weightB1, w0, w1, w2, w3);
            sumup(rmob2, weightB2, w0, w1, w2, w3);
            sumup(rmob3, weightB3, w0, w1, w2, w3);*/
            
            score += mobB1[popcount(rmob1)] + mobB2[popcount(rmob2)] /*+ mobB3[popcount(rmob3)]*/;

            print_debug(debugMobility, "b mobility%d: %d, %d\n", CI, popcount(rmob1), popcount(rmob2)/*, popcount(rmob3)*/);
            if(TRACE_DEBUG && Options::debug & debugMobility) { printBit(rmob1); printBit(rmob2);/* printBit(rmob3);*/ }
        }
    }
    for (uint64_t p = b.getPieces<C, Knight>(); p; p &= p-1) {
        uint64_t sq = bit(p);
        uint64_t rmob1 = b.knightAttacks[sq] & restrictions;
        uint64_t rmob2 = rmob1 & noBlockedPawns;
        rmob1 &= ~b.getOcc<C>();
        rmob2 |= b.buildNAttack(rmob1) & restrictions & ~b.getOcc<C>();
//         uint64_t rmob3 = rmob2 & noBlockedPawns;
//         rmob3 |= b.buildNAttack(rmob2) & restrictions & ~b.getOcc<C>();
        rmob2 &= ~rmob1;
//         rmob3 &= ~(rmob1 | rmob2);
        
// #ifdef MYDEBUG        
//         bmob1 += popcount(rmob1);
//         bmob2 += popcount(rmob2);
//         bmob3 += popcount(rmob3);
//         bmobn ++;
// #endif  

        attackingPieces += attack1n1 * popcount15(rmob1 & oppking1);
        attackingPieces += attack2n1 * popcount15(rmob1 & oppking2);
        attackingPieces += attack1n2 * popcount15(rmob2 & oppking1);
        attackingPieces += attack2n2 * popcount15(rmob2 & oppking2);
        print_debug(debugMobility, "n attack%d: %d, %d, %d, %d\n", CI, attack1n1 * popcount15(rmob1 & oppking1), attack2n1 * popcount15(rmob1 & oppking2), attack1n2 * popcount15(rmob2 & oppking1), attack2n2 * popcount15(rmob2 & oppking2));
//         if (rmob3 & oppking) attackingPieces++;
        if (b.knightAttacks[sq] & ownking) defendingPieces++;
        score += mobN1[popcount(rmob1)] + mobN2[popcount(rmob2)] /*+ mobN3[popcount(rmob3)]*/;

        print_debug(debugMobility, "n mobility%d: %d, %d\n", CI, popcount(rmob1), popcount(rmob2)/*, popcount(rmob3)*/);
        if(TRACE_DEBUG && Options::debug & debugMobility) { printBit(rmob1); printBit(rmob2); /*printBit(rmob3);*/ }
    }

    restrictions &= ~(b.getAttacks<-C,Bishop>() | b.getAttacks<-C,Knight>());
    __v2di allRookAttacks = zero;
    for (const BoardBase::MoveTemplateR* rs = b.rsingle[CI]; rs->move.data; ++rs) {
        // restriced mobility in one move, including own defended pieces
        allRookAttacks |= rs->d02;
        uint64_t rmob0 = fold(rs->d02);
        uint64_t rmob1 = rmob0 & restrictions;
        // generator for two move squares, mask blocked pawns
        uint64_t rmob2 = rmob1 & noBlockedPawns;
        // remove own pieces
        rmob1 &= ~b.getOcc<C>();
        // restriced mobility in two moves, including own defended pieces, excluding pieces defended in the 2nd move        
        rmob2 |= b.build02Attack(rmob1) & restrictions & ~b.getOcc<C>();
        // generator for three move squares, mask blocked pawns
//         uint64_t rmob3 = rmob2 & noBlockedPawns;
        // restriced mobility in three moves
//         rmob3 |= b.build02Attack(rmob2) & restrictions & ~b.getOcc<C>();
        
        __v2di connectedQR = ~pcmpeqq(rs->d02 & v2queen, zero) & b.qsingle[CI][0].d02;
        if (b.rsingle[CI][1].move.data)
            connectedQR |= ~ pcmpeqq(rs->d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero) & (b.rsingle[CI][0].d02 | b.rsingle[CI][1].d02);
        rmob1 |= fold(connectedQR) & ~b.getOcc<C>() & restrictions;
        rmob2 &= ~rmob1;
//         rmob3 &= ~(rmob1 | rmob2);
        
        attackingPieces += attack1r1 * popcount15(rmob0 & oppking1);
        attackingPieces += attack2r1 * popcount15(rmob0 & oppking2);
        attackingPieces += attack1r2 * popcount15(rmob2 & oppking1);
        attackingPieces += attack2r2 * popcount15(rmob2 & oppking2);
        print_debug(debugMobility, "r attack%d: %d, %d, %d, %d\n", CI, attack1r1 * popcount15(rmob0 & oppking1), attack2r1 * popcount15(rmob0 & oppking2), attack1r2 * popcount15(rmob2 & oppking1), attack2r2 * popcount15(rmob2 & oppking2));
        if (rmob0 & ownking) defendingPieces++;
        score += mobR1[popcount(rmob1)] + mobR2[popcount(rmob2)] /*+ mobR3[popcount(rmob3)]*/;

        print_debug(debugMobility, "r mobility%d: %d, %d\n", CI, popcount(rmob1), popcount(rmob2)/*, popcount(rmob3)*/);
        if(TRACE_DEBUG && Options::debug & debugMobility) { printBit(rmob1); printBit(rmob2); /*printBit(rmob3);*/ }
    }

    if (b.getPieces<C,Queen>()) {
        restrictions &= ~(b.getAttacks<-C,Rook>());
        uint64_t rmob0 = b.getAttacks<C,Queen>();
        uint64_t rmob1 = rmob0 & restrictions;
        // generator for two move squares, mask blocked pawns
        uint64_t rmob2 = rmob1 & noBlockedPawns;
        // remove own pieces
        rmob1 &= ~b.getOcc<C>();
        // restriced mobility in two moves, including own defended pieces, excluding pieces defended in the 2nd move        
        rmob2 |= (b.build02Attack(rmob1) | b.build13Attack(rmob1)) & restrictions & ~b.getOcc<C>();
        // generator for three move squares, mask blocked pawns
//         uint64_t rmob3 = rmob2 & noBlockedPawns;
        // restriced mobility in three moves
//         rmob3 |= (b.build02Attack(rmob2) | b.build13Attack(rmob2)) & restrictions & ~b.getOcc<C>();
        
        unsigned q=bit(b.getPieces<C,Queen>());
        __v2di connectedBR = ~ pcmpeqq(b.qsingle[CI][0].d13 & _mm_set1_epi64x(b.getPieces<C,Bishop>()), zero) 
            & allBishopAttacks
            & b.mask13x[q];
        connectedBR |= ~ pcmpeqq(b.qsingle[CI][0].d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero) 
            & allRookAttacks 
            & b.mask02[q].x;
        
        rmob1 |= fold(connectedBR) & ~b.getOcc<C>() & restrictions;
        rmob2 &= ~rmob1;
//         rmob3 &= ~(rmob1 | rmob2);

        attackingPieces += attack1q1 * popcount15(rmob0 & oppking1);
        attackingPieces += attack2q1 * popcount15(rmob0 & oppking2);
        attackingPieces += attack1q2 * popcount15(rmob2 & oppking1);
        attackingPieces += attack2q2 * popcount15(rmob2 & oppking2);
        print_debug(debugMobility, "q attack%d: %d, %d, %d, %d\n", CI, attack1q1 * popcount15(rmob0 & oppking1), attack2q1 * popcount15(rmob0 & oppking2), attack1q2 * popcount15(rmob2 & oppking1), attack2q2 * popcount15(rmob2 & oppking2));
        score += mobQ1[popcount(rmob1)] + mobQ2[popcount(rmob2)] /*+ mobQ3[popcount(rmob3)]*/;
        
        print_debug(debugMobility, "q mobility%d: %d, %d\n", CI, popcount(rmob1), popcount(rmob2)/*, popcount(rmob3)*/);
        if(TRACE_DEBUG && Options::debug & debugMobility) { printBit(rmob1); printBit(rmob2); /*printBit(rmob3);*/ }
    }
    
    attackingPieces += attack1p1 * popcount15(b.getAttacks<C,Pawn>() & oppking1);
    attackingPieces += attack2p1 * popcount15(b.getAttacks<C,Pawn>() & oppking2);
    
    attackingPieces += attack1k1 * popcount15(b.getAttacks<C,King>() & oppking1);
    attackingPieces += attack2k1 * popcount15(b.getAttacks<C,King>() & oppking2);
    
    return score;
}

template<Colors C>
int Eval::attack(const BoardBase& b, const PawnEntry& p, unsigned attackingPieces, unsigned defendingPieces) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    uint64_t king = b.getPieces<-C,King>();
    unsigned kpos = bit(king);
    
    // If King has already moved, assess king safety by the pawn shield on the side he is standing
    // If on e/d file, treat him with no pawn shield at all for now.
    // If he can castle, assess king safety by the pawn shield on the side he can castle to or the
    // better pawn shield
    Sides side;
    if (king & (rank<C,8>() | rank<C,7>()) & (file<'a'>() | file<'b'>() | file<'c'>())) {
        side = QSide;
    } else if (king & (rank<C,8>() | rank<C,7>()) & (file<'f'>() | file<'g'>() | file<'h'>())) {
        side = KSide;
    } else if (b.cep.castling.color[EI].q && b.cep.castling.color[EI].k) {
        side = p.shield[EI].kside>=p.shield[EI].qside ? KSide:QSide;
    } else if (b.cep.castling.color[EI].q) {
        side = QSide;
    } else if (b.cep.castling.color[EI].k) {
        side = KSide;
    } else
        side = Middle;

    // give a slight malus to king safety if there a pieces in the way for castling
    // to encourage castling and to make a difference between potential king safety
    // and real safety after castling
    int def = 0;
    if (side == KSide) {
        def = p.shield[EI].kside;
        if (b.cep.castling.color[EI].k) {
            if (b.occupied1 & rank<C,8>() & file<'g'>()) def--;
            if (b.occupied1 & rank<C,8>() & file<'f'>()) def--;
        }
    } else if (side == QSide) {
        def = p.shield[EI].qside;
        if (b.cep.castling.color[EI].k) {
            if (b.occupied1 & rank<C,8>() & file<'b'>()) def--;
            if (b.occupied1 & rank<C,8>() & file<'c'>()) def--;
            if (b.occupied1 & rank<C,8>() & file<'d'>()) def--;
        }
    }

    ASSERT(attackingPieces < 256);
    int att = ( (attackTable[attackingPieces]      - baseAttack) * defenseTable[def + defendingPieces] / baseDefense
              + (defenseTable[def+defendingPieces] - baseDefense)* attackTable[attackingPieces]        / baseAttack);
    
    print_debug(debugEval, "pa defense%d: %3d\n", EI, def);
    print_debug(debugEval, "pi defense%d: %3d\n", EI, defendingPieces);
    print_debug(debugEval, "defenseval%d: %3d\n", EI, - (defenseTable[def+defendingPieces] - baseDefense)* attackTable[attackingPieces]        / baseAttack);
    print_debug(debugEval, "pi attack%d:  %3d\n", CI, attackingPieces);
    print_debug(debugEval, "attackval%d:  %3d\n", CI, (attackTable[attackingPieces]      - baseAttack) * defenseTable[def + defendingPieces] / baseDefense);
    print_debug(debugEval, "att*defval%d: %3d\n", CI, att);
    return att;
}

int Eval::operator () (const BoardBase& b) const {
    int e = b.keyScore.score.calc(b.material);
#if defined(MYDEBUG)
    int value = 0;
    for (int p=Rook; p<=King; ++p) {
        for (uint64_t x=b.getPieces<White>(p); x; x&=x-1) {
            unsigned sq=bit(x);
            print_debug(debugEval, "materialw%d     %d\n", p, getPS( p, sq).calc(b.material));                
            value += getPS( p, sq).calc(b.material);
        }
        for (uint64_t x=b.getPieces<Black>(p); x; x&=x-1) {
            unsigned sq=bit(x);
            print_debug(debugEval, "materialb%d     %d\n", p, getPS(-p, sq).calc(b.material));                
            value += getPS(-p, sq).calc(b.material);
        }
    }
    if (value != e) asm("int3");
#endif
    PawnEntry pe = pawns(b);
    unsigned wap = 0;
    unsigned bap = 0;
    unsigned wdp = 0;
    unsigned bdp = 0;
    
    int m = mobility<White>(b, wap, wdp) - mobility<Black>(b, bap, bdp);
    int openingScale = b.material - popcount(b.getPieces<White,Pawn>()+b.getPieces<Black,Pawn>()) - endgameMaterial + (1<<(logEndgameTransitionSlope-1));
    openingScale = std::max(0, std::min(openingScale, 1<<logEndgameTransitionSlope));
    int a = openingScale ? (openingScale*(attack<White>(b, pe, wap, bdp) - attack<Black>(b, pe, bap, wdp))) >> logEndgameTransitionSlope : 0;
    int pi = pieces<White>(b, pe) - pieces<Black>(b, pe);
    int pa = pe.score + (openingScale*pe.centerOpen + ((1<<logEndgameTransitionSlope)-openingScale)*pe.centerEnd) >> logEndgameTransitionSlope;
    print_debug(debugEval, "material:       %d\n", e);
    print_debug(debugEval, "mobility:       %d\n", m);
    print_debug(debugEval, "pawns:          %d\n", pa);
    print_debug(debugEval, "attack:         %d\n", a);
    print_debug(debugEval, "pieces:         %d\n", pi);
    
    return e + m + pa + a + pi;
}

void Eval::ptClear() {
    pt->clear();
}

