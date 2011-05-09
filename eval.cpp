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
static const int logEndgameTransitionSlope = 5;
static const int endgameTransitionSlope = 1<<logEndgameTransitionSlope; //16 pieces material between full endgame and full opening
static const int baseAttack = 1024;
static const int baseDefense = -256;

static const int ksv[nSquares] = {
     0,  2,  4,  6,  6,  4,  2,  0,
     2, 10, 12, 14, 14, 12, 10,  2,
     4, 12, 20, 22, 22, 20, 12,  4,
     6, 14, 22, 30, 30, 22, 14,  6,
     6, 14, 22, 30, 30, 22, 14,  6,
     4, 12, 20, 22, 22, 20, 12,  4,
     2, 10, 12, 14, 14, 12, 10,  2,
     0,  2,  4,  6,  6,  4,  2,  0
};
static const int ksvb[2][nSquares] = {
{
     0, 10, 20, 30, 30, 26, 22, 18,
    10, 21, 32, 43, 43, 36, 28, 22,
    20, 32, 44, 56, 56, 45, 36, 26,
    30, 43, 56, 69, 69, 56, 44, 30,
    30, 43, 56, 69, 69, 56, 43, 30,
    26, 36, 45, 56, 56, 44, 32, 20,
    22, 28, 36, 43, 43, 32, 21, 10,
    18, 22, 26, 30, 30, 20, 10,  0
},{
    18, 22, 26, 30, 30, 20, 10,  0,
    22, 28, 36, 43, 43, 32, 21, 10,
    26, 36, 45, 56, 56, 44, 32, 20,
    30, 43, 56, 69, 69, 56, 43, 30,
    30, 43, 56, 69, 69, 56, 44, 30,
    20, 32, 44, 56, 56, 45, 36, 26,
    10, 21, 32, 43, 43, 36, 28, 22,
     0, 10, 20, 30, 30, 26, 22, 18
}};

unsigned distance[nSquares][nSquares];

uint64_t mobStat[nColors][nPieces+1][2][nSquares];

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

Eval::Eval()
{
    pt = new TranspositionTable<PawnEntry, 4, PawnKey>;
//    pt->setSize(0x100000000);
    pawn = 80;
    knight = 300;   
    bishop = 300;   
    rook = 500;
    queen = 900;
    /*
     * These parameters decide the weight of the attack function
     * For very shallow ply searches the attack functions is actually
     * a disadvantage, relative ELO based on 4k games +/-9
     * ply/att  3       4       5       6       7
     * 150      2001    2022    1991    1980    1988
     * 200      1999    1978    2009    2020    2012
     */
    maxAttack = baseAttack + 250;
    maxDefense = baseDefense - 75;
    const int totalMaterial = 4*(materialTab[Rook]+materialTab[Bishop]+materialTab[Knight]) + 2*materialTab[Queen];
    ASSERT(totalMaterial == 56);
    endgameMaterial = 27;       // +/- 16 ->  16 (Q) ... 48 (QRRB or QRBBN)

    bishopPair = 50;
    bishopBlockPasser = 20;
    bishopAlone = -100;
    
    knightAlone = -125;
    knightBlockPasser = 30;

    rookTrapped = -50;
    rookOpen = 8;
    rookHalfOpen = 4;
    rookWeakPawn = 16;
    
    pawnBackward = -4;
    pawnBackwardOpen = -8;
    pawnIsolatedCenter = -4;
    pawnIsolatedEdge = -2;// - Eval::pawnBackwardOpen;
    pawnIsolatedOpen = -8;// - Eval::pawnBackwardOpen;
//     pawnEdge = -12;
//     pawnCenter = 10;
    pawnDouble = -20;
    pawnShoulder = 4;
//     pawnHole = -4;
    sigmoid( oppKingOwnPawn, -12,  12, 1, 10);  // only 1..7 used
    sigmoid( ownKingOwnPawn,   6,  -6, 1, 10);
    sigmoid( oppKingOwnPasser, -32,  32, 1, 10);  // only 1..7 used
    sigmoid( ownKingOwnPasser,  16, -16, 1, 10);
    sigmoid( pawnPasser, 25, 100, 6, 1.0 );
    sigmoid( pawnConnPasser, 0, 50, 6, 1.0 );
    pawnUnstoppable = 700;

//     attack1b1 = 4;        //direct attack of ring around king
//     attack2b1 = 2;        //direct attack of 2nd ring around king
//     attack1b2 = 2;        //indirect attack of ring around king
//     attack2b2 = 1;        //indirect attack of 2nd ring around king
//     attack1n1 = 4;
//     attack2n1 = 2;
//     attack1n2 = 2;
//     attack2n2 = 1;
//     attack1r1 = 6;
//     attack2r1 = 2;
//     attack1r2 = 3;
//     attack2r2 = 1;
//     attack1q1 = 8;
//     attack2q1 = 4;
//     attack1q2 = 4;
//     attack2q2 = 2;
//     attack1k1 = 3;        //king is not able to deliver mate
//     attack2k1 = 1;
//     attack1p1 = 4;
//     attack2p1 = 1;
    sigmoid(attackR1, 55, 80, 3.0, 3.0, 1); // max bits set = 21
    sigmoid(attackR2, 55, 80, 6.0, 6.0, 1); // max bits set = 21
    sigmoid(attackB1, 40, 60, 2.5, 2.5, 1); // max bits set = 21
    sigmoid(attackB2, 40, 60, 5.0, 5.0, 1); // max bits set = 21
    sigmoid(attackQ1, 100, 130, 3.5, 3.5, 1); // max bits set = 21
    sigmoid(attackQ2, 100, 130, 7.0, 7.0, 1); // max bits set = 21
    sigmoid(attackN1, 40, 60, 2.5, 2.5, 1); // max bits set = 21
    sigmoid(attackN2, 40, 60, 5.0, 5.0, 1); // max bits set = 21
    sigmoid(attackP, 40, 95, 2.5, 0.5, 1); // max bits set = 21
    sigmoid(attackK, 40, 50, 2.0, 1.5, 1); // max bits set = 21

    //attack Q                          = 230 = 14
    //attack Q + N                      = 330 = 20.5
    //attack Q + R + N = 200 + 80 + 110 = 475 = 30
    sigmoid(attackTable, baseAttack, maxAttack, 30, 7);

    aspiration0 = 40;
    aspiration1 = 5;
    evalHardBudget = -20;

    sigmoid(mobN1, -25, 25, 0, 2.0);
    sigmoid(mobN2, -25, 25, 0, 8.0);

    sigmoid(mobB1, -20, 20, 0, 3.25);
    sigmoid(mobB2, -20, 20, 0,13.0);
    
    sigmoid(mobR1, -15, 15, 0, 4.0);
    sigmoid(mobR2, -15, 15, 0,16.0);

    sigmoid(mobQ1, -10, 10, 0, 7.25);
    sigmoid(mobQ2, -10, 10, 0,29.0);

    sigmoid(defenseTable, baseDefense, maxDefense, 8, 4);

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
    printSigmoid(mobN1, "mobN1");
    printSigmoid(mobN2, "mobN2");
    printSigmoid(attackN1, "attN1");
    printSigmoid(attackN2, "attN2");
    printSigmoid(attackB1, "attB1");
    printSigmoid(attackB2, "attB2");
    printSigmoid(attackR1, "attR1");
    printSigmoid(attackR2, "attR2");
    printSigmoid(attackQ1, "attQ1");
    printSigmoid(attackQ2, "attQ2");
    printSigmoid(attackP, "attP");
    printSigmoid(attackK, "attK");
    printSigmoid(attackTable, "att", baseAttack);
    printSigmoid(pawnPasser, "pass");
    printSigmoid(pawnConnPasser, "cpass");
    printSigmoid(oppKingOwnPasser, "eKpas");
    printSigmoid(ownKingOwnPasser, "mKpas");
    printSigmoid(oppKingOwnPawn, "eKp");
    printSigmoid(ownKingOwnPawn, "mKp");
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
    for (int x0=0; x0<8; ++x0)
    for (int y0=0; y0<8; ++y0)
    for (int x1=0; x1<8; ++x1)
    for (int y1=0; y1<8; ++y1)
        distance[x0+8*y0][x1+8*y1] = std::max(abs(x0-x1), abs(y0-y1));

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
    int temp;
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
    own += own << 010;
    own += own << 020;
    own += own << 040;
    
    uint64_t opp = p.openFiles[EI];
    opp += opp << 010;
    opp += opp << 020;
    opp += opp << 040;

    uint64_t weak = p.weak[EI];
    weak += weak << 010;
    weak += weak << 020;
    weak += weak << 040;

    value += rookHalfOpen * popcount3(b.getPieces<C,Rook>() & own);
    value += rookOpen * popcount3(b.getPieces<C,Rook>() & own & opp);
    temp = rookWeakPawn * popcount3(b.getPieces<C,Rook>() & weak);
    print_debug(debugEval, "rookWeakP%d: %3d\n", CI, temp);
    value += temp;
    
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

        int wps = pawnShoulder * popcount15(wpawn & wpawn<<1 & ~file<'a'>());
        int bps = pawnShoulder * popcount15(bpawn & bpawn<<1 & ~file<'a'>());
        
/*        uint64_t wpawnHole = wpawn & wpawn<<2 & ~file<'a'>() & ~file<'b'>();
        uint64_t bpawnHole = bpawn & bpawn<<2 & ~file<'a'>() & ~file<'b'>();
        pawnEntry.score += pawnHole * popcount15(wpawnHole);
        pawnEntry.score -= pawnHole * popcount15(bpawnHole);
        print_debug(debugEval, "wpawnHole:     %3d\n", pawnHole * popcount15(wpawnHole));
        print_debug(debugEval, "bpawnHole:     %3d\n", pawnHole * popcount15(bpawnHole));*/
        
        uint64_t wFront = wpawn << 8 | wpawn << 16;
        wFront |= wFront << 16 | wFront << 32;
        uint64_t wBack = wpawn >> 8 | wpawn >> 16;
        wBack |= wBack >> 16 | wBack >> 32;

        uint64_t bBack = bpawn << 8 | bpawn << 16;
        bBack |= bBack << 16 | bBack << 32;
        uint64_t bFront = bpawn >> 8 | bpawn >> 16;
        bFront |= bFront >> 16 | bFront >> 32;

        int wdbl = pawnDouble*popcount15(wpawn & wBack);
        int bdbl = pawnDouble*popcount15(bpawn & bBack);
        // calculate squares which are or possibly are attacked by w and b pawns
        // take only the most advanced attacker and the most backward defender into account

        uint64_t wAttack = (wFront >> 1 & ~0x8080808080808080LL) | (wFront << 1 & ~0x101010101010101LL);
        uint64_t bAttack = (bFront >> 1 & ~0x8080808080808080LL) | (bFront << 1 & ~0x101010101010101LL);

        // backward pawns are pawns which may be attacked, if the advance,
        // but are not on a contested square (otherwise they would be defended)

        uint64_t wstop = b.getAttacks<Black,Pawn>() & ~wAttack;
        uint64_t bstop = b.getAttacks<White,Pawn>() & ~bAttack;
        wstop  = wstop>>010 | wstop>>020;
        bstop  = bstop<<010 | bstop<<020;
        wstop |= wstop>>040;
        bstop |= bstop<<040;
        uint64_t wBackward = wpawn & wstop;
        uint64_t bBackward = bpawn & bstop;
        if(TRACE_DEBUG && Options::debug & debugEval) { printBit(wBackward); printBit(bBackward); }
        
        int wpbw = pawnBackward * popcount15(wBackward);
        int bpbw = pawnBackward * popcount15(bBackward);
        
        uint64_t wBackOpen = wBackward & ~bFront;
        uint64_t bBackOpen = bBackward & ~wFront;
        uint64_t wBack8 = wBackOpen | wBackOpen>>010;
        uint64_t bBack8 = bBackOpen | bBackOpen>>010;
        wBack8 |= wBack8>>020;
        bBack8 |= bBack8>>020;
        wBack8 |= wBack8>>040;
        bBack8 |= bBack8>>040;
        pawnEntry.weak[0] = wBack8;
        pawnEntry.weak[1] = bBack8;
        if(TRACE_DEBUG && Options::debug & debugEval) { printBit(wBack8); printBit(bBack8); }
        //TODO scale pbwo with the number of rooks + queens
        int wpbwo = pawnBackwardOpen * popcount15(wBackOpen);
        int bpbwo = pawnBackwardOpen * popcount15(bBackOpen);

        // a white pawn is not passed, if he is below a opponent pawn or its attack squares
        // store positions of passers for later use in other eval functions

        uint64_t wNotPassedMask = bFront | bAttack;
        uint64_t bNotPassedMask = wFront | wAttack;
        uint64_t wPassed = wpawn & ~wNotPassedMask & ~wBack;
        uint64_t bPassed = bpawn & ~bNotPassedMask & ~bBack;
        uint64_t wPassedConnected  = wPassed & (wPassed>>1 | wPassed>>9 | wPassed<<7) & ~file<'h'>();
        wPassedConnected |= wPassed & (wPassed<<1 | wPassed<<9 | wPassed>>7) & ~file<'a'>();
        uint64_t bPassedConnected = bPassed & (bPassed>>1 | bPassed>>9 | bPassed<<7) & ~file<'h'>();
        bPassedConnected |= bPassed & (bPassed<<1 | bPassed<<9 | bPassed>>7) & ~file<'a'>();
        unsigned i;
        pawnEntry.passers[0] = wPassed;
        for (i = 0; wPassed; wPassed &= wPassed - 1, i++) {
            unsigned pos = bit(wPassed);
            unsigned y = pos >> 3;
            ASSERT(y>0 && y<7);
            pawnEntry.score += pawnPasser[y-1];
            if (wPassedConnected >> pos & 1) pawnEntry.score += pawnConnPasser[y-1];

            print_debug(debugEval, "pawn wpasser:   %d\n", pawnPasser[y-1]);
        }
        pawnEntry.passers[1] = bPassed;
        for ( i=0; bPassed; bPassed &= bPassed-1, i++ ) {
            unsigned pos = bit(bPassed);
            unsigned y = pos  >> 3;
            ASSERT(y>0 && y<7);
            pawnEntry.score -= pawnPasser[6-y];
            if (bPassedConnected >> pos & 1) pawnEntry.score += pawnConnPasser[6-y];
            print_debug(debugEval, "pawn bpasser:   %d\n", pawnPasser[6-y]);
        }

        // possible weak pawns are adjacent to open files or below pawns an a adjacent file on both sides
        // rook pawns are always adjacent to a "open" file
        uint64_t wOpenFiles = ~(wFront | wpawn | wBack);
        uint64_t bOpenFiles = ~(bFront | bpawn | bBack);
        pawnEntry.openFiles[0] = (uint8_t)wOpenFiles;
        pawnEntry.openFiles[1] = (uint8_t)bOpenFiles;
        
        uint64_t wIsolani = wpawn & (wOpenFiles<<1 | 0x101010101010101LL) & (wOpenFiles>>1 | 0x8080808080808080LL);
        uint64_t bIsolani = bpawn & (bOpenFiles<<1 | 0x101010101010101LL) & (bOpenFiles>>1 | 0x8080808080808080LL);
        int wpic = pawnIsolatedCenter * (popcount15(wIsolani & ~(file<'a'>()|file<'h'>())));
        int bpic = pawnIsolatedCenter * (popcount15(bIsolani & ~(file<'a'>()|file<'h'>())));
        int wpie = pawnIsolatedEdge * (popcount15(wIsolani & (file<'a'>()|file<'h'>())));
        int bpie = pawnIsolatedEdge * (popcount15(bIsolani & (file<'a'>()|file<'h'>())));
        int wpio = pawnIsolatedOpen * (popcount15(wIsolani & ~bFront));
        int bpio = pawnIsolatedOpen * (popcount15(bIsolani & ~wFront));
        
        pawnEntry.score += wdbl + wpbw + wpbwo + wps + wpic + wpio + wpie;
        pawnEntry.score -= bdbl + bpbw + bpbwo + bps + bpic + bpio + bpie;
        
        print_debug(debugEval, "wpawnDouble:    %3d\n", wdbl);
        print_debug(debugEval, "bpawnDouble:    %3d\n", bdbl);
        print_debug(debugEval, "wpawn backward: %3d\n", wpbw);
        print_debug(debugEval, "bpawn backward: %3d\n", bpbw);
        print_debug(debugEval, "wpawn bwo:      %3d\n", wpbwo);
        print_debug(debugEval, "bpawn bwo:      %3d\n", bpbwo);
        print_debug(debugEval, "wpawnShoulder:  %3d\n", wps);
        print_debug(debugEval, "bpawnShoulder:  %3d\n", bps);
        print_debug(debugEval, "wpawn ciso:     %3d\n", wpic);
        print_debug(debugEval, "bpawn ciso:     %3d\n", bpic);
        print_debug(debugEval, "wpawn eiso:     %3d\n", wpie);
        print_debug(debugEval, "bpawn eiso:     %3d\n", bpie);
        print_debug(debugEval, "wpawn oiso:     %3d\n", wpio);
        print_debug(debugEval, "bpawn oiso:     %3d\n", bpio);
        
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

template<Colors C, GamePhase P>
inline int Eval::mobility( const BoardBase &b, int& attackingPieces, int& defendingPieces) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    int score = 0;
    int king = bit(b.getPieces<-C,King>());
    const uint64_t oppking = b.kAttacked[king];
//     const uint64_t oppking2 = b.kAttacked2[king];
    const uint64_t ownking = b.getAttacks< C,King>() | shift<C* 8>(b.getAttacks< C,King>());
    const uint64_t noBlockedPawns = ~(b.getPieces<C,Pawn>() & shift<C*-8>(b.getPieces<-C,Pawn>()));      //only own pawns
//     const __v2di noBlockedPawns2 = _mm_set1_epi64x(noBlockedPawns);
//     const uint64_t empty = ~b.getOcc<C>();
//     const __v2di empty2 = _mm_set1_epi64x(empty);
    
//     __v2di w0 = zero, w1 = zero, w2 = zero, w3 = zero;
    
    const __v2di v2queen = _mm_set1_epi64x(b.getPieces<C,Queen>());

    uint64_t twoAttacks=( (b.getAttacks<C,Rook>()   &  b.getAttacks<C,Pawn>())
                        | (b.getAttacks<C,Bishop>() & (b.getAttacks<C,Pawn>() | b.getAttacks<C,Rook>()))
                        | (b.getAttacks<C,Queen>()  & (b.getAttacks<C,Pawn>() | b.getAttacks<C,Rook>() | b.getAttacks<C,Bishop>()))
                        | (b.getAttacks<C,Knight>() & (b.getAttacks<C,Pawn>() | b.getAttacks<C,Rook>() | b.getAttacks<C,Bishop>() | b.getAttacks<C,Queen>()))
                        | (b.getAttacks<C,King>()   & (b.getAttacks<C,Pawn>() | b.getAttacks<C,Rook>() | b.getAttacks<C,Bishop>() | b.getAttacks<C,Queen>() | b.getAttacks<C,King>()))
                        );

    uint64_t restrictions = ~b.getAttacks<-C,Pawn>() & ~(b.getAttacks<-C,All>() & ~twoAttacks);
//     __v2di restrictions2 = _mm_set1_epi64x(restrictions);
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
            uint64_t batt1 = fold(bs->d13);
            uint64_t bmob1 = batt1 & restrictions;
            // remove blocked pawns, this positions will not be reachable even in two moves
            uint64_t bmob2 = bmob1 & noBlockedPawns;
            // remove own pieces
            bmob1 &= ~b.getOcc<C>();
            // restriced mobility in two moves, including own defended pieces, excluding pieces defended in the 2nd move
            uint64_t batt2 = b.build13Attack(bmob1);
            bmob2 |= batt2 & restrictions & ~b.getOcc<C>();
//             batt2 |= batt1;
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
            uint64_t bxray = fold( ~ pcmpeqq(bs->d13 & v2queen, zero) & b.qsingle[CI][0].d13); //TODO optimize for queenless positions
            bmob1 |= bxray & ~b.getOcc<C>() & restrictions;
            bmob2 |= bxray & ~b.getOcc<C>() & restrictions;
            ASSERT((bmob2 & bmob1) == bmob1);
            if (P != Endgame) {
                batt1 |= bxray;
                batt2 |= bxray | batt1;
                ASSERT((batt2 & batt1) == batt1);
                attackingPieces += attackB1[popcount15(batt1 & oppking)];
                attackingPieces += attackB2[popcount15(batt2 & oppking)];
                if (batt1 & ownking) defendingPieces++;
                print_debug(debugMobility, "b attack  %d: d%2d:%3d, i%2d:%3d\n", CI, popcount15(batt1 & oppking), attackB1[popcount15(batt1 & oppking)], popcount15(batt2 & oppking), attackB2[popcount15(batt2 & oppking)]);
            }
            
            score += mobB1[popcount(bmob1)] + mobB2[popcount(bmob2)] /*+ mobB3[popcount(rmob3)]*/;
#ifdef MYDEBUG
            mobStat[CI][Bishop][0][popcount(bmob1)]++;
            mobStat[CI][Bishop][1][popcount(bmob2)]++;
#endif
            
            print_debug(debugMobility, "b mobility%d: d%2d:%3d, i%2d:%3d\n", CI, popcount(bmob1), mobB1[popcount(bmob1)], popcount(bmob2), mobB2[popcount(bmob2)]/*, popcount(rmob3)*/);
            if(TRACE_DEBUG && Options::debug & debugMobility) { printBit(bmob1); printBit(bmob2); /*printBit(rmob3);*/ }
        }
    }
    for (uint64_t p = b.getPieces<C, Knight>(); p; p &= p-1) {
        uint64_t sq = bit(p);
        uint64_t natt1 = b.knightAttacks[sq];
        uint64_t nmob1 = natt1 & restrictions;
        uint64_t nmob2 = nmob1 & noBlockedPawns;
        nmob1 &= ~b.getOcc<C>();
        uint64_t natt2 = b.buildNAttack(nmob1);
        nmob2 |= natt2 & restrictions & ~b.getOcc<C>();
        ASSERT((nmob2 & nmob1) == nmob1);
        if (P != Endgame) {
            natt2 |= natt1;
            ASSERT((natt2 & natt1) == natt1);
            attackingPieces += attackN1[popcount15(natt1 & oppking)];
            attackingPieces += attackN2[popcount15(natt2 & oppking)];
            if (b.knightAttacks[sq] & ownking) defendingPieces++;
            print_debug(debugMobility, "n attack  %d: d%3d:%2d, i%2d:%3d\n", CI, popcount15(natt1 & oppking), attackN1[popcount15(natt1 & oppking)], popcount15(natt2 & oppking), attackN2[popcount15(natt2 & oppking)]);
        }
//         if (rmob3 & oppking) attackingPieces++;
        score += mobN1[popcount(nmob1)] + mobN2[popcount(nmob2)] /*+ mobN3[popcount(rmob3)]*/;
#ifdef MYDEBUG        
        mobStat[CI][Knight][0][popcount(nmob1)]++;
        mobStat[CI][Knight][1][popcount(nmob2)]++;
#endif        

        print_debug(debugMobility, "n mobility%d: d%2d:%3d, i%2d:%3d\n", CI, popcount(nmob1), mobN1[popcount(nmob1)], popcount(nmob2), mobN2[popcount(nmob2)]/*, popcount(rmon3)*/);
        if(TRACE_DEBUG && Options::debug & debugMobility) { printBit(nmob1); printBit(nmob2); /*printBit(rmob3);*/ }
    }

    restrictions &= ~(b.getAttacks<-C,Bishop>() | b.getAttacks<-C,Knight>());
    __v2di allRookAttacks = zero;
    for (const BoardBase::MoveTemplateR* rs = b.rsingle[CI]; rs->move.data; ++rs) {
        // restriced mobility in one move, including own defended pieces
        allRookAttacks |= rs->d02;
        uint64_t ratt1 = fold(rs->d02);
        uint64_t rmob1 = ratt1 & restrictions;
        // remove blocked pawns, this positions will not be reachable even in two moves
        uint64_t rmob2 = rmob1 & noBlockedPawns;
        // remove own pieces
        rmob1 &= ~b.getOcc<C>();
        // restriced mobility in two moves, including own defended pieces, excluding pieces defended in the 2nd move
        uint64_t ratt2 = b.build02Attack(rmob1);
        rmob2 |= ratt2 & restrictions & ~b.getOcc<C>();
        
        __v2di connectedQR = ~pcmpeqq(rs->d02 & v2queen, zero) & b.qsingle[CI][0].d02;
        if (b.rsingle[CI][1].move.data)
            connectedQR |= ~ pcmpeqq(rs->d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero) & (b.rsingle[CI][0].d02 | b.rsingle[CI][1].d02);
        uint64_t rxray = fold(connectedQR); //TODO optimize for queenless positions
        rmob1 |= rxray & ~b.getOcc<C>() & restrictions;
        rmob2 |= rxray & ~b.getOcc<C>() & restrictions;
        ASSERT((rmob2 & rmob1) == rmob1);
        if (P != Endgame) {
            ratt1 |= rxray;
            ratt2 |= rxray | ratt1;
            ASSERT((ratt2 & ratt1) == ratt1);
            attackingPieces += attackR1[popcount15(ratt1 & oppking)];
            attackingPieces += attackR2[popcount15(ratt2 & oppking)];
            if (ratt1 & ownking) defendingPieces++;
            print_debug(debugMobility, "r attack  %d: d%3d:%2d, i%2d:%3d\n", CI, popcount15(ratt1 & oppking), attackR1[popcount15(ratt1 & oppking)], popcount15(ratt2 & oppking), attackR2[popcount15(ratt2 & oppking)]);
        }
        score += mobR1[popcount(rmob1)] + mobR2[popcount(rmob2)] /*+ mobR3[popcount(rmob3)]*/;
#ifdef MYDEBUG
        mobStat[CI][Rook][0][popcount(rmob1)]++;
        mobStat[CI][Rook][1][popcount(rmob2)]++;
#endif

        print_debug(debugMobility, "r mobility%d: d%2d:%3d, i%2d:%3d\n", CI, popcount(rmob1), mobR1[popcount(rmob1)], popcount(rmob2), mobR2[popcount(rmob2)]/*, popcourt(rmor3)*/);
        if(TRACE_DEBUG && Options::debug & debugMobility) { printBit(rmob1); printBit(rmob2); /*printBit(rmob3);*/ }
    }

    if (b.getPieces<C,Queen>()) {
        restrictions &= ~(b.getAttacks<-C,Rook>());
        uint64_t qatt1 = b.getAttacks<C,Queen>();
        uint64_t qmob1 = qatt1 & restrictions;
        uint64_t qmob2 = qmob1 & noBlockedPawns;
        qmob1 &= ~b.getOcc<C>();
        uint64_t qatt2 = b.build02Attack(qmob1) | b.build13Attack(qmob1);
        qmob2 |= qatt2 & restrictions & ~b.getOcc<C>();
        unsigned q=bit(b.getPieces<C,Queen>());
        __v2di connectedBR = ~ pcmpeqq(b.qsingle[CI][0].d13 & _mm_set1_epi64x(b.getPieces<C,Bishop>()), zero)
            & allBishopAttacks
            & b.mask13x[q];
        connectedBR |= ~ pcmpeqq(b.qsingle[CI][0].d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero)
            & allRookAttacks
            & b.mask02[q].x;
        uint64_t qxray = fold(connectedBR); //TODO optimize for queenless positions
        qmob1 |= qxray & ~b.getOcc<C>() & restrictions;
        qmob2 |= qxray & ~b.getOcc<C>() & restrictions;
        ASSERT((qmob2 & qmob1) == qmob1);
        if (P != Endgame) {
            qatt1 |= qxray;
            qatt2 |= qxray | qatt1;
            ASSERT((qatt2 & qatt1) == qatt1);
            attackingPieces += attackQ1[popcount15(qatt1 & oppking)];
            attackingPieces += attackQ2[popcount15(qatt2 & oppking)];
            if (qatt1 & ownking) defendingPieces++;
            print_debug(debugMobility, "q attack  %d: d%2d:%3d, i%2d:%3d\n", CI, popcount15(qatt1 & oppking), attackQ1[popcount15(qatt1 & oppking)], popcount15(qatt2 & oppking), attackQ2[popcount15(qatt2 & oppking)]);
        }
        score += mobQ1[popcount(qmob1)] + mobQ2[popcount(qmob2)] /*+ mobQ3[popcount(rmob3)]*/;
#ifdef MYDEBUG
        mobStat[CI][Queen][0][popcount(qmob1)]++;
        mobStat[CI][Queen][1][popcount(qmob2)]++;
#endif
        
        print_debug(debugMobility, "q mobility%d: d%2d:%3d, i%2d:%3d\n", CI, popcount(qmob1), mobQ1[popcount(qmob1)], popcount(qmob2), mobQ2[popcount(qmob2)]/*, popcourt(rmor3)*/);
        if(TRACE_DEBUG && Options::debug & debugMobility) { printBit(qmob1); printBit(qmob2); /*printBit(rmob3);*/ }
    }
        
    attackingPieces += attackP[popcount15(b.getAttacks<C,Pawn>() & oppking)];
    attackingPieces += attackK[popcount15(b.getAttacks<C,King>() & oppking)];
    
    return score;
}

template<Colors C>
int Eval::attack(const BoardBase& b, const PawnEntry& p, unsigned attackingPieces, unsigned defendingPieces) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    uint64_t king = b.getPieces<-C,King>();
//     unsigned kpos = bit(king);
    
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
            def -= 2;
        }
    } else if (side == QSide) {
        def = p.shield[EI].qside;
        if (b.cep.castling.color[EI].q) {
            if (b.occupied1 & rank<C,8>() & file<'b'>()) def--;
            if (b.occupied1 & rank<C,8>() & file<'c'>()) def--;
            if (b.occupied1 & rank<C,8>() & file<'d'>()) def--;
            def -= 2;
        }
    }

    attackingPieces >>= 4;
    def = std::max(def, 0);
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

template<Colors C>
int Eval::king(const BoardBase& b) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    unsigned kpos = bit(b.getPieces<C,King>());
    if (b.getPieces<-C,Bishop>() & !b.bsingle[EI][1].move.data) {
        unsigned pos = bit(b.getPieces<-C,Bishop>());
        unsigned corner = (pos ^ (pos >> 3)) & 1;
        return ksvb[corner][kpos];
    }
    return ksv[kpos];
}

template<Colors C>
int Eval::endgame(const BoardBase& b, const PawnEntry& pe, int sideToMove) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    
    int score = 0;
    unsigned eking = bit(b.getPieces<-C,King>());
    unsigned king = bit(b.getPieces<C,King>());
    // pawn moves, non capture, non promo
    print_debug(debugEval, "King dist pawn %d\n", CI);
    for (uint64_t p = b.getPieces<C,Pawn>() & ~pe.passers[CI]; p; p &= p-1) {
        unsigned pos = bit(p);
        score += oppKingOwnPawn[distance[eking][pos]];
        print_debug(debugEval, "(%3d:", oppKingOwnPawn[distance[eking][pos]]);
        score += ownKingOwnPawn[distance[king][pos]];
        print_debug(debugEval, "%3d)", ownKingOwnPawn[distance[king][pos]]);
    }
    print_debug(debugEval, "\nKing dist passer %d\n", CI);
    /*
     * calculate king distance to promotion square
     */
    for (uint64_t p = b.getPieces<C,Pawn>() & pe.passers[CI]; p; p &= p-1) {
        unsigned pos = bit(p);
/*        unsigned promo = pos & 7;
        unsigned promodist = pos >> 3;
        uint64_t pawnStops;
        if (C == White) {
            promo ^= 070;
            promodist ^= 7;
            pawnStops  = 1ULL << pos;
            pawnStops += pawnStops << 010;
            pawnStops += pawnStops << 020;
            pawnStops += pawnStops << 040;      // TODO use table
        } else {
            pawnStops  = 1ULL << pos;
            pawnStops += pawnStops >> 010;
            pawnStops += pawnStops >> 020;
            pawnStops += pawnStops >> 040;      // TODO use table
        }
        if (b.getAttacks<-C,All>() == b.getAttacks<-C,King>() && promodist + (C!=sideToMove) < distance[eking][promo])
            score += pawnUnstoppable;
        score += oppKingOwnPasser[distance[eking][promo]];
        print_debug(debugEval, "(%3d:", oppKingOwnPasser[distance[eking][promo]]);
        score += ownKingOwnPasser[distance[king][promo]];
        print_debug(debugEval, "%3d)", ownKingOwnPasser[distance[king][promo]]);*/
        score += oppKingOwnPasser[distance[eking][pos]];
        print_debug(debugEval, "(%3d:", oppKingOwnPasser[distance[eking][pos]]);
        score += ownKingOwnPasser[distance[king][pos]];
        print_debug(debugEval, "%3d)", ownKingOwnPasser[distance[king][pos]]);
    }
    print_debug(debugEval, "%d\n", 0);
    return score;
}

int Eval::operator () (const BoardBase& b, int stm) const {
#if defined(MYDEBUG)
    int cmp = b.keyScore.score.calc(b.material);
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
    if (value != cmp) asm("int3");
#endif
    if (b.getPieces<White,Pawn>() + b.getPieces<Black,Pawn>()) {
        PawnEntry pe = pawns(b);

        int openingScale = b.material - endgameMaterial + endgameTransitionSlope/2;
//         openingScale = std::max(0, std::min(openingScale, endgameTransitionSlope));
        int m, a, e, pa;
/*        int wap, bap, wdp, bdp;
        wap = bap = wdp = bdp = 0;
        m = mobility<White, Opening>(b, wap, wdp) - mobility<Black, Opening>(b, bap, bdp);
        a = attack<White>(b, pe, wap, bdp) - attack<Black>(b, pe, bap, wdp);*/
        if (openingScale >= endgameTransitionSlope) {
            int wap, bap, wdp, bdp;
            wap = bap = wdp = bdp = 0;
            m = mobility<White, Opening>(b, wap, wdp) - mobility<Black, Opening>(b, bap, bdp);
            a = attack<White>(b, pe, wap, bdp) - attack<Black>(b, pe, bap, wdp);
            e = 0;
            pa = pe.score + pe.centerOpen;
        } else if (openingScale <= 0) {
            int wap, bap, wdp, bdp;
            m = mobility<White, Endgame>(b, wap, wdp) - mobility<Black, Endgame>(b, bap, bdp);
            a = 0;
            e = endgame<White>(b, pe, stm) - endgame<Black>(b, pe, stm);
            pa = pe.score + pe.centerEnd;
        } else {
            int wap, bap, wdp, bdp;
            wap = bap = wdp = bdp = 0;
            m = mobility<White, Opening>(b, wap, wdp) - mobility<Black, Opening>(b, bap, bdp);
            a = (openingScale*(attack<White>(b, pe, wap, bdp) - attack<Black>(b, pe, bap, wdp))) >> logEndgameTransitionSlope;
            e = ((endgameTransitionSlope-openingScale)*(endgame<White>(b, pe, stm) - endgame<Black>(b, pe, stm))) >> logEndgameTransitionSlope;
            pa = pe.score + ((openingScale*pe.centerOpen + (endgameTransitionSlope-openingScale)*pe.centerEnd) >> logEndgameTransitionSlope);
            
        }
        int pi = pieces<White>(b, pe) - pieces<Black>(b, pe);
//         int pa = pe.score + ((openingScale*pe.centerOpen + (endgameTransitionSlope-openingScale)*pe.centerEnd) >> logEndgameTransitionSlope);
        print_debug(debugEval, "endgame:        %d\n", e);
        print_debug(debugEval, "mobility:       %d\n", m);
        print_debug(debugEval, "pawns:          %d\n", pa);
        print_debug(debugEval, "attack:         %d\n", a);
        print_debug(debugEval, "pieces:         %d\n", pi);

        return e + m + pa + a + pi;
    } else {     // pawnless endgame
        int mat = b.keyScore.score.calc(b.material);
        int wap, bap, wdp, bdp; //FIXME not needed here
        int m = mobility<White, Endgame>(b, wap, wdp) - mobility<Black, Endgame>(b, bap, bdp);
        int p = (mat>0 ? 1:4)*king<White>(b) - (mat<0 ? 1:4)*king<Black>(b);
        return m + p;        
    }
}

void Eval::ptClear() {
    pt->clear();
}

