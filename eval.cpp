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

// uint32_t Eval::borderTab4_0[nSquares];
// uint32_t Eval::borderTab321[nSquares];
// uint32_t Eval::borderTab567[nSquares];
//
// __v16qi Eval::kMask0[nSquares];
// __v16qi Eval::kMask1[nSquares];

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

CompoundScore Eval::pawn, Eval::knight, Eval::bishop, Eval::rook, Eval::queen;
CompoundScore Eval::bishopPair;
CompoundScore Eval::knightAlone;
CompoundScore Eval::bishopAlone;
CompoundScore Eval::n_p_ram[9], Eval::n_p[17];
CompoundScore Eval::b_bad_own_p[9], Eval::b_p[17], Eval::b_p_ram[9], Eval::bpair_p[17];
CompoundScore Eval::r_p[17];
CompoundScore Eval::q_p[17];
int Eval::pawnBackward = -10;
int Eval::pawnBackwardOpen = -20;
int Eval::pawnPasser[8] = { 0, 50, 50, 60, 80, 115, 160, 0 };
int Eval::pawnHalfIsolated = -5;
int Eval::pawnIsolated = -30 - Eval::pawnBackwardOpen;
Parameters Eval::parameters;

#if !defined(__SSE_4_2__)
// popcount, which counts at most 15 ones, for counting pawns
static inline int popcount15( uint64_t x )
{
    x -=  x>>1 & 0x5555555555555555LL;
    x  = ( x>>2 & 0x3333333333333333LL ) + ( x & 0x3333333333333333LL );
    x *= 0x1111111111111111LL;
    return  x>>60;
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
#define popcount2(x) __builtin_popcountll(x)
#endif

/*int CompoundScore::calc(const BoardBase& b, const Eval& ) const
{
    return b.material >= endgameMaterial ? opening : endgame;
}
*/
Eval::Eval() {
    pt = new TranspositionTable<PawnEntry, 4, PawnKey>;
//    pt->setSize(0x100000000);
    pawn = 80;
    knight = 275;   // +25 psq = 300
    bishop = 284;   // +16 psq = 300
    rook = 430;     // +20 psq = 450
    queen = 900;    // +25 psq = 925

    bishopPair = 50;
    knightAlone = -125;
    bishopAlone = -100;

    parameters.aspiration0 = 40;
    parameters.aspiration1 = 5;
    parameters.evalHardBudget = 20;

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
    // highest rank (eight) comes first, for convenience.
    // while initializing it is flipped top-down.
    RawScore rook[2][nSquares] = {{
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
-90, -90, -95, -100, -100, -95, -90, -90,
-90, -90, -95, -100, -100, -95, -90, -90,
-90, -90, -95, -100, -100, -95, -90, -90,
-40, -40, -45, -50, -50, -45, -40, -40,
-10, -10, -15, -20, -20, -15, -10, -10,
 0,  0, -5, -10, -10, -5,  0,  0,
10, 10,  5,  0,  0,  5, 10, 10,
10, 10,  5,  0,  0,  5, 10, 10
},{
 0,  4,  8, 12, 12,  8,  4,  0,
 9, 13, 17, 21, 21, 17, 13,  9,
18, 22, 26, 30, 30, 26, 22, 18,
17, 21, 25, 29, 29, 25, 21, 17,
14, 18, 22, 26, 26, 22, 18, 14,
10, 14, 18, 22, 22, 18, 14, 10,
 5,  9, 13, 17, 17, 13,  9,  5,
 0,  4,  8, 12, 12,  8,  4,  0
}};

    for (unsigned int sq = 0; sq<nSquares; ++sq)
        getPS( 0, sq) = CompoundScore{ 0, 0 };

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Pawn, sq) = Eval::pawn + CompoundScore{ pawn[Opening][sq ^ 0x38], pawn[Endgame][sq ^ 0x38] };
        getPS(-Pawn, sq ^ 070) = -getPS( Pawn, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Rook, sq)  = Eval::rook + CompoundScore{ rook[Opening][sq ^ 0x38], rook[Endgame][sq ^ 0x38] };
        getPS(-Rook, sq ^ 070) = -getPS( Rook, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Bishop, sq) = Eval::bishop + CompoundScore{ bishop[Opening][sq ^ 0x38], bishop[Endgame][sq ^ 0x38] };
        getPS(-Bishop, sq ^ 070) = -getPS( Bishop, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Knight, sq) = Eval::knight + CompoundScore{ knight[Opening][sq ^ 0x38], knight[Endgame][sq ^ 0x38] };
        getPS(-Knight, sq ^ 070) = -getPS( Knight, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Queen, sq) = Eval::queen + CompoundScore{ queen[Opening][sq ^ 0x38], queen[Endgame][sq ^ 0x38] };
        getPS(-Queen, sq ^ 070) = -getPS( Queen, sq);
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( King, sq) = CompoundScore{ king[Opening][sq ^ 0x38], king[Endgame][sq ^ 0x38] };
        getPS(-King, sq ^ 070) = -getPS( King, sq);
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

int Eval::pieces(const BoardBase&) const {
//     if (pl.ge  unts() == 0x01000100000000)     //only one knight left
//         value += knightAlone;
//     if (pl.getCounts() == 0x01000000010000)     //only one bishop left
//         value += knightAlone;
//     if (pl[Bishop] > 1)
//         value += bishopPair;
//
//     value += b_p[pl[Pawn]];
    int value = 0;

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
int Eval::pawns(const BoardBase& b) const {
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

        // calculate squares which are or possibly are attacked by w and b pawns
        // take only the most advanced attacker and the most backward defender into account

        uint64_t wAttack = (wAbove >> 1 & ~0x8080808080808080LL) | (wAbove << 1 & ~0x101010101010101LL);
        uint64_t bAttack = (bBelow >> 1 & ~0x8080808080808080LL) | (bBelow << 1 & ~0x101010101010101LL);

        // backward pawns are pawns which may be attacked, if the advance,
        // but are not on a contested square (otherwise they would be defended)

        uint64_t wBackward = (wpawn<<8 & b.getAttacks<Black,Pawn>() & ~wAttack) >> 8;
        uint64_t bBackward = (bpawn>>8 & b.getAttacks<White,Pawn>() & ~bAttack) << 8;

        int pbw = pawnBackward * (popcount15(wBackward) - popcount15(bBackward));
        int pbwo = pawnBackwardOpen * (popcount15(wBackward & ~bAbove) - popcount15(bBackward & ~wBelow));
#ifdef MYDEBUG
        if (debug) {
            std::cout << "pawn backward:  " << pbw << std::endl;
            std::cout << "pawn bw open:   " << pbwo << std::endl;
        }
#endif
        pawnEntry.score += pbw + pbwo;
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
#ifdef MYDEBUG
            if (debug) {
                std::cout << "pawn wpasser:   " << pawnPasser[y] << std::endl;
            }
#endif
            pawnEntry.passers[0][i] = pos;
        }
        for ( i=0; bPassed && i<nHashPassers; bPassed &= bPassed-1, i++ ) {
            int pos = __builtin_ctzll(bPassed);
            int y = pos>>3;
            pawnEntry.score -= pawnPasser[7-y];
#ifdef MYDEBUG
            if (debug) {
                std::cout << "pawn wpasser:   " << pawnPasser[7-y] << std::endl;
            }
#endif
            pawnEntry.passers[1][i] = pos;
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
#ifdef MYDEBUG
        if (debug) {
            std::cout << "pawn hiso:      " << pawnHalfIsolated * (popcount15(wRightIsolani|wLeftIsolani) - popcount15(bRightIsolani|bLeftIsolani)) << std::endl;
            std::cout << "pawn iso:       " << pawnIsolated * (popcount15(wRightIsolani&wLeftIsolani) - popcount15(bRightIsolani&bLeftIsolani)) << std::endl;
        }
#endif

        pawnEntry.shield[0] = evalShield<White>(b);
        pawnEntry.shield[1] = evalShield<Black>(b);
        pawnEntry.upperKey = k >> PawnEntry::upperShift;
        if (k >> PawnEntry::upperShift)
            pt->store(st, pawnEntry);
    }
    return pawnEntry.score;
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

static const int8_t mobValues[nPieces+1][32] = {
    // unoccupied and not attacked by a less valuable piece
    // 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27
    {  0 },                                                                // nothing
    {  0,  3,  6,  9, 12, 14, 16, 18, 20, 22, 23, 24, 25, 25, 25 }, //rook
    {  0,  3,  5,  7,  9, 10, 11, 12, 13, 14, 15, 16, 16, 16 }, //bishop
    {  0,  3,  5,  7,  9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 },
    {  0,  6, 11, 15, 18, 20, 22, 24, 25 },
    {  0 },
    {  0 }
};

template<Colors C>
inline int Eval::mobility( const BoardBase &b, const uint64_t (&restrictions)[nColors][nPieces+1], unsigned& attackingPieces, unsigned& defendingPieces) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    int score = 0;
    uint64_t oppking = b.getAttacks<-C,King>();
    uint64_t ownking = b.getAttacks< C,King>();
//    std::cout << "k" << CI << std::hex << ownking << std::endl;
    for (const BoardBase::MoveTemplateB* bs = b.bsingle[CI]; bs->move.data; ++bs) {
        Move m = bs->move;
        __v2di a13 = bs->d13;
#ifdef USE_PINS_IN_MOBILITY
        __v2di from2 = _mm_set1_epi64x(1ULL<<m.from());
        __v2di pin13 = from2 & pins[CI].dir13;
#ifdef __SSE4_1__
        pin13 = _mm_cmpeq_epi64(pin13, zero);
#else
        pin13 = _mm_cmpeq_epi32(pin13, zero);
        __v2di pin13s = _mm_shuffle_epi32(pin13, 0b10110001);
        pin13 = pin13 & pin13s;
#endif
        pin13 = ~pin13 & a13;
        uint64_t mob = fold(pin13);
#else
        uint64_t mob = fold(a13);
#endif
        if (mob & oppking) attackingPieces++;
        if (mob & ownking) defendingPieces++;
        mob = popcount15(mob & ~restrictions[CI][Bishop]);
        ASSERT(mob <= 13);

        score += mobValues[Bishop][mob];
#ifdef MYDEBUG
        if (debug) {
            std::cout << "b mobility" << CI << "    :" << mob << std::endl;
            std::cout << "b mobility" << CI << "    :" << (int)mobValues[Bishop][mob] << std::endl;
        }
#endif
    }

    for (const BoardBase::MoveTemplateR* rs = b.rsingle[CI]; rs->move.data; ++rs) {
        Move m = rs->move;
        __v2di a02 = rs->d02;
#ifdef USE_PINS_IN_MOBILITY
        __v2di from2 = _mm_set1_epi64x(1ULL<<m.from());
        __v2di pin02 = from2 & pins[CI].dir02;
#ifdef __SSE4_1__
        pin02 = _mm_cmpeq_epi64(pin02, zero);
#else
        pin02 = _mm_cmpeq_epi32(pin02, zero);
        __v2di pin02s = _mm_shuffle_epi32(pin02, 0b10110001);
        pin02 = pin02 & pin02s;
#endif
        pin02 = ~pin02 & a02;
        uint64_t mob = fold(pin02);
#else
        uint64_t mob = fold(a02);
#endif
        if (mob & oppking) attackingPieces++;
        if (mob & ownking) defendingPieces++;
        mob = popcount15(mob & ~restrictions[CI][Rook]);
        ASSERT(mob <= 14);

        score += mobValues[Rook][mob];
#ifdef MYDEBUG
        if (debug) {
            std::cout << "r mobility" << CI << "    :" << mob << std::endl;
            std::cout << "r mobility" << CI << "    :" << (int)mobValues[Rook][mob] << std::endl;
        }
#endif
    }

    for (const BoardBase::MoveTemplateQ* qs = b.qsingle[CI]; qs->move.data; ++qs) {
        Move m = qs->move;
        __v2di a02 = qs->d02;
        __v2di a13 = qs->d13;
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
        if (mob & oppking) attackingPieces+=2;
        mob = popcount(mob & ~restrictions[CI][Queen]);
        ASSERT(mob <= 27 );

        score += mobValues[Queen][mob];
#ifdef MYDEBUG
        if (debug) {
            std::cout << "q mobility" << CI << "    :" << mob << std::endl;
            std::cout << "q mobility" << CI << "    :" << (int)mobValues[Queen][mob] << std::endl;
        }
#endif
    }

    for (uint64_t p = b.getPieces<C, Knight>(); p; p &= p-1) {
        uint64_t sq = bit(p);
        uint64_t mob = popcount15(b.knightAttacks[sq] & ~restrictions[CI][Knight]);
        ASSERT(mob <= 8);
        if (b.knightAttacks[sq] & oppking) attackingPieces++;
        if (b.knightAttacks[sq] & ownking) defendingPieces++;
        score += mobValues[Knight][mob];
#ifdef MYDEBUG
        if (debug) {
            std::cout << "n mobility" << CI << "    :" << mob << std::endl;
            std::cout << "n mobility" << CI << "    :" << (int)mobValues[Knight][mob] << std::endl;
        }
#endif
    }

    return score;
}

template<Colors C>
inline void Eval::mobilityRestrictions(const BoardBase &b, uint64_t (&restrictions)[nColors][nPieces+1]) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    uint64_t r = b.getOcc<C>();
    ASSERT(r == (b.getPieces<C,Rook>() + b.getPieces<C,Bishop>() +
                 b.getPieces<C,Queen>()+ b.getPieces<C,Knight>() +
                 b.getPieces<C,Pawn>() + b.getPieces<C,King>()));
    restrictions[CI][Knight] = restrictions[CI][Bishop] = r |= b.getAttacks<-C,Pawn>();
    restrictions[CI][Rook] = r |= b.getAttacks<-C,Knight>() | b.getAttacks<-C,Bishop>();
    restrictions[CI][Queen] = r |= b.getAttacks<-C,Rook>();
}

template<Colors C>
inline int Eval::mobility(const BoardBase& b, unsigned& attackingPieces, unsigned& defendingPieces) const {
    uint64_t restrictions[nColors][nPieces+1];

    mobilityRestrictions<C>(b, restrictions);
    return mobility<C>(b, restrictions, attackingPieces, defendingPieces);
}

static const int kattPieces[] = { 0, 1, 2, 4, 7, 10, 10, 10, 10 };
static const int kdefPawn[] = { 20,20,20,19,18,17,16,15,14,13,12,11,10,10,10,10,10,10,10,10,10,10};
                            //   0           4           8          12 
static const int kdefPieces[] = { 30,28,24,20,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};

template<Colors C>
int Eval::attack(const BoardBase& b, unsigned attackingPieces, unsigned defendingPieces) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    uint64_t king = b.getPieces<-C,King>();
    Sides side;
    if (king & (rank<C,8>() | rank<C,7>()) & (file<'a'>() | file<'b'>() | file<'c'>())) {
        side = QSide;
    } else if (king & (rank<C,8>() | rank<C,7>()) & (file<'f'>() | file<'g'>() | file<'h'>())) {
        side = KSide;
    } else if (b.cep.castling.color[EI].q && b.cep.castling.color[EI].k) {
        side = pawnEntry.shield[EI].kside>=pawnEntry.shield[EI].qside ? KSide:QSide;
    } else if (b.cep.castling.color[EI].q) {
        side = QSide;
    } else if (b.cep.castling.color[EI].k) {
        side = KSide;
    } else
        side = Middle;

    int def = 0;
    if (side == KSide) def = pawnEntry.shield[EI].kside;
    else if (side == QSide) def = pawnEntry.shield[EI].qside;

    int att=kattPieces[attackingPieces] * (kdefPawn[def] + kdefPieces[defendingPieces])/4;
    
#ifdef MYDEBUG
    if (debug) {
        std::cout << "defense" << CI << "       :" << def << std::endl;
        std::cout << "attack" << CI << "        :" << attackingPieces << std::endl;
        std::cout << "defpiece" << CI << "      :" << defendingPieces << std::endl;
        std::cout << "att value" << CI << "     :" << att << std::endl;
    }
#endif
    return att;
}

int Eval::eval(const BoardBase& b) const {
    int e = b.keyScore.score.calc(b.material);
#if defined(MYDEBUG)
    int value = 0;
    for (int p=Rook; p<=King; ++p) {
        for (uint64_t x=b.getPieces<White>(p); x; x&=x-1) {
            unsigned sq=bit(x);
            if (debug) std::cout << "materialw" << p << "     " << getPS( p, sq).calc(b.material) << std::endl;                
            value += getPS( p, sq).calc(b.material);
        }
        for (uint64_t x=b.getPieces<Black>(p); x; x&=x-1) {
            unsigned sq=bit(x);
            if (debug) std::cout << "materialb" << p << "     " << getPS(-p, sq).calc(b.material) << std::endl;                
            value += getPS(-p, sq).calc(b.material);
        }
    }
    if (value != e) asm("int3");
#endif
    int p = pawns(b);
    unsigned wap = 0;
    unsigned bap = 0;
    unsigned wdp = 0;
    unsigned bdp = 0;
    
    int m = mobility<White>(b, wap, wdp) - mobility<Black>(b, bap, bdp);
    int a;
    if (b.material >= endgameMaterial) {
        a = attack<White>(b, wap, bdp) - attack<Black>(b, bap, wdp);
    } else
        a = 0;
#ifdef MYDEBUG    
    if (debug) {
        std::cout << "material:       " << e << std::endl;
        std::cout << "mobility:       " << m << std::endl;
        std::cout << "pawns:          " << p << std::endl;
        std::cout << "attack:         " << a << std::endl;
    }
#endif
    
    return e + m + p + a;
}

void Eval::ptClear() {
    pt->clear();
}

