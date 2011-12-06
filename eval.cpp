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
#include <cstdio>
#include "eval.h"
#include "boardbase.h"
#include "options.h"
#include "transpositiontable.tcc"
#include "parameters.h"
#include "eval.tcc"

unsigned Eval::distance[nSquares][nSquares];

static const __v2di zero = {0};
static const __v2di mask01 = { 0x0101010101010101, 0x0202020202020202 };
static const __v2di mask23 = { 0x0404040404040404, 0x0808080808080808 };
static const __v2di mask45 = { 0x1010101010101010, 0x2020202020202020 };
static const __v2di mask67 = { 0x4040404040404040, 0x8080808080808080 };
static const int baseAttack = 1024;
static const int baseDefense = -256;
static const uint64_t darkSquares = 0xaa55aa55aa55aa55;
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

#if 0
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
#endif
Eval::Eval(uint64_t pHashSize, const Parameters& p)
{
    pt = new TranspositionTable<PawnEntry, 4, PawnKey>(pHashSize);
//     std::cerr << "Eval::Eval " << pHashSize << std::endl;
    setParameters(p);
    init();
}

Eval::~Eval() {
    delete pt;
}

void Eval::init() {

    static_assert(56 == 4*(materialRook+materialBishop+materialKnight) + 2*materialQueen + 16*materialPawn, "Total material");

    for (unsigned i=0; i<sizeof(scale)/sizeof(*scale); ++i) {
        int openingScale = i - endgameMaterial + endgameTransitionSlope/2;
        openingScale = std::max(0, std::min(openingScale, endgameTransitionSlope));
        scale[i].endgame = endgameTransitionSlope-openingScale;
        scale[i].opening = openingScale;
    }

    evalHardBudget = -20;

    initZobrist();
    initShield();

    if (Options::debug & DebugFlags::debugEval) {
        printSigmoid(nAttackersTab, "nAttackersTab");
        printSigmoid(attackN, "attN");
        printSigmoid(attackTable2, "attTable2");
        printSigmoid(pawnPasser, "pass");
        printSigmoid(pawnConnPasser, "cpass");
        printSigmoid(oppKingOwnPasser, "eKpas");
        printSigmoid(ownKingOwnPasser, "mKpas");
        printSigmoid(oppKingOwnPawn, "eKp");
        printSigmoid(ownKingOwnPawn, "mKp");

        for (unsigned i=0; i<3; ++i) {
            std::cout << "kingShield" << i << ": " << std::setw(3) << kingShield.inner[i] << std::setw(3) << kingShield.center[i] << std::setw(3) << kingShield.outer[i] << std::endl;
        }
    }
}

void Eval::initPS(Pieces pIndex, Parameters::Piece& piece) {
    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        int xh = (sq & 7);
        int y = sq >> 3;
        double corner = 2-std::min(xh, std::min(y, std::min(7-xh, 7-y)));
        if (xh>3) xh ^= 7;
        int yo = y;
        if (y>piece.vcenter.opening)
            yo = 2*piece.vcenter.opening - y;
        yo = std::max(0, std::min(7, yo));
        int ye = y;
        if (y>piece.vcenter.endgame)
            ye = 2*piece.vcenter.endgame - y;
        ye = std::max(0, std::min(7, ye));
        getPS( pIndex, sq) = { (short) (piece.value.opening + piece.hor.opening[xh] + piece.vert.opening[yo] + corner*piece.corner.opening),
                               (short) (piece.value.endgame + piece.hor.endgame[xh] + piece.vert.endgame[ye] + corner*piece.corner.endgame) };
        getPS(-pIndex, sq ^ 070) = -getPS( pIndex, sq);
        print_debug(debugEval, "%4d %4d  ", getPS(pIndex, sq).opening, getPS(pIndex, sq).endgame);
        if ((sq & 7) == 7) print_debug(debugEval, "%c", '\n');
    }
    
}
void Eval::initPS() {

    for (unsigned int sq = 0; sq<nSquares; ++sq)
        getPS( 0, sq) = 0;

    initPS(Rook, rook);
    initPS(Bishop, bishop);
    initPS(Queen, queen);
    initPS(Knight, knight);
    initPS(Pawn, pawn);
    initPS(King, king);
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

    int collision=0;
    for (int p = -nPieces; p <= (signed int)nPieces; p++) 
    if (p)
    for (unsigned int s = 0; s < nSquares; ++s) 
    for (unsigned int d = 0; d < nSquares; ++d) 
    if (d!=s) {
        KeyScore z1, z2;
        z1.vector = zobristPieceSquare[p+nPieces][d].vector - zobristPieceSquare[p+nPieces][s].vector;
        z2.vector = zobristPieceSquare[p+nPieces][s].vector - zobristPieceSquare[p+nPieces][d].vector;
        for (int cp = -nPieces; cp <= (signed int)nPieces; cp++)
        if (cp)
        for (unsigned int c = 0; c < nSquares; ++c) 
        if (zobristPieceSquare[cp+nPieces][c].key == z1.key || zobristPieceSquare[p+nPieces][c].key == z2.key) 
            ++collision;
    }
    if (collision)
        std::cerr << collision << " Zobrist collisions" << std::endl;
}

void Eval::initTables() {
    for (int x0=0; x0<8; ++x0)
    for (int y0=0; y0<8; ++y0)
    for (int x1=0; x1<8; ++x1)
    for (int y1=0; y1<8; ++y1)
        distance[x0+8*y0][x1+8*y1] = std::max(abs(x0-x1), abs(y0-y1));
}

void Eval::initShield() {
    for (unsigned index=0; index<=0777; ++index) {
        int score = 0;
        if (index &    1) score += kingShield.outer[0];
        if (index &  010) score += kingShield.outer[1];
        if (index & 0100) score += kingShield.outer[2];
        if (index &    2) score += kingShield.center[0];
        if (index &  020) score += kingShield.center[1];
        if (index & 0200) score += kingShield.center[2];
        if (index &    4) score += kingShield.inner[0];
        if (index &  040) score += kingShield.inner[1];
        if (index & 0400) score += kingShield.inner[2];
        ASSERT(score >= 0);
        score = std::min(INT8_MAX, score);
        shield[index] = score;

        score = 0;
        if (index &    1) score += kingShield.inner[0];
        if (index &  010) score += kingShield.inner[1];
        if (index & 0100) score += kingShield.inner[2];
        if (index &    2) score += kingShield.center[0];
        if (index &  020) score += kingShield.center[1];
        if (index & 0200) score += kingShield.center[2];
        if (index &    4) score += kingShield.outer[0];
        if (index &  040) score += kingShield.outer[1];
        if (index & 0400) score += kingShield.outer[2];
        ASSERT(score >= 0);
        score = std::min(INT8_MAX, score);
        shieldMirrored[index] = score;
    }
}

template<Colors C>
int Eval::pieces(const BoardBase& b, const PawnEntry& p) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    int value = 0;
    int temp;
    if (popcount3((b.getPieces<C, Bishop>()) >= 2)) {
        value += bishopPair;
    }
    if (popcount3((b.getPieces<C, Knight>()) >= 2)) {
        value += knightPair;
    }
    if (popcount3((b.getPieces<C, Rook>()) >= 2)) {
        value += rookPair;
    }
    if (popcount3((b.getPieces<C, Queen>()) >= 2)) {
        value += queenPair;
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
            
    uint64_t own = p.openFiles[CI]; //TODO use lookuptable (2k) instead
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

    value += rookHalfOpen * popcount3((b.getPieces<C,Rook>() & own));
    value += rookOpen * popcount3((b.getPieces<C,Rook>() & own & opp));
    temp = rookWeakPawn * popcount3((b.getPieces<C,Rook>() & weak));
    print_debug(debugEval, "rookWeakP%d: %3d\n", CI, temp);
    value += temp;
    
    value += knightBlockPasser * popcount3((b.getPieces<C,Knight>() & shift<C*8>(p.passers[EI])));
    value += bishopBlockPasser * popcount3((b.getPieces<C,Bishop>() & shift<C*8>(p.passers[EI])));

    // TODO vectorize
    int darkB = !!(b.getPieces<C,Bishop>() & darkSquares);
    int lightB = !!(b.getPieces<C,Bishop>() & ~darkSquares);
    value += bishopOwnPawn * (p.dark[CI] * darkB + p.light[CI] * lightB);
    value += bishopOppPawn * (p.dark[EI] * darkB + p.light[EI] * lightB);

    value += bishopNotOwnPawn * (p.dark[CI] * lightB + p.light[CI] * darkB);
    value += bishopNotOppPawn * (p.dark[EI] * lightB + p.light[EI] * darkB);
    return value;
}

template<Colors C>
int Eval::evalShield2(uint64_t pawns, unsigned file) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };

    int bitrank2 = 28-20*C;
    int bitrank3 = 28-12*C;
    int bitrank4 = 28- 4*C;

    // Shift pawns in a position in which they will directly used as part of an
    // index in a lookup table, add pawns left to a2 and right to h2 pawn
    unsigned rank2 = pawns >> (bitrank2-1) |  0b1000000001;
    unsigned rank3 = pawns >> (bitrank3-4) & ~0b1000000001000;
    unsigned rank4 = pawns >> (bitrank4-7) & ~0b1000000001000000;
    
    unsigned index = (rank2>>file & 7) + (rank3>>file & 070) + (rank4>>file & 0700);

    if (file<4)
        return shield[index];
    else
        return shieldMirrored[index];
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

        int wps = pawnShoulder * popcount15(wpawn & wpawn<<1 & ~file<'a'>());
        int bps = pawnShoulder * popcount15(bpawn & bpawn<<1 & ~file<'a'>());
           
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

        uint64_t wDarkpawn = wpawn & darkSquares;
        uint64_t bDarkpawn = bpawn & darkSquares;
        pawnEntry.dark[0] = popcount(wDarkpawn);
        pawnEntry.dark[1] = popcount(bDarkpawn);
        pawnEntry.light[0] = popcount(wpawn - wDarkpawn);
        pawnEntry.light[1] = popcount(bpawn - bDarkpawn);
        
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
        
//         pawnEntry.shield[0] = evalShield<White>(b);
//         pawnEntry.shield[1] = evalShield<Black>(b);

        for (unsigned i=0; i<nRows; ++i) {
            pawnEntry.shield2[0][i] = evalShield2<White>(wpawn, i);
//             if (pawnEntry.openFiles[0] & 1<<i)
//                 pawnEntry.shield2[0][i] += kingShield.halfOpenFile;
            
            pawnEntry.shield2[1][i] = evalShield2<Black>(bpawn, i);
//             if (pawnEntry.openFiles[1] & 1<<i)
//                 pawnEntry.shield2[1][i] += kingShield.halfOpenFile;
            
            if (pawnEntry.openFiles[0] & pawnEntry.openFiles[1] & 7<<i>>1) {
                pawnEntry.shield2[0][i] += kingShield.openFile;
                pawnEntry.shield2[1][i] += kingShield.openFile;
            }
        }
        pawnEntry.upperKey = k >> PawnEntry::upperShift;
        if (k >> PawnEntry::upperShift)
            pt->store(st, pawnEntry);
    }
    return pawnEntry;
}

// static const __v2di weightB1[4] = {
//     { 0x0707070707070707,       //a-file
//       0x0709090909090907 },     //b-file
//     { 0x07090b0b0b0b0907,       //c-file
//       0x07090b0d0d0b0907 },     //d-file
//     { 0x07090b0d0d0b0907,       //e-file
//       0x07090b0b0b0b0907 },     //f-file
//     { 0x0709090909090907,       //g-file
//       0x0707070707070707 }      //h-file
// };
// 
template<Colors C, GamePhase P> // TODO use overload for P == Endgame, where attack and defend are not used
inline int Eval::mobility( const BoardBase &b, int& attackingPieces, int& defendingPieces) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    DeltaScore score(0,0);
    int score_endgame = 0;
    int score_opening = 0;
    attackingPieces = 0;
    unsigned nAttackers = 0;
    if (P != Endgame) defendingPieces = 0;
    unsigned nDefenders = 0;
    
//     int king = bit(b.getPieces<-C,King>());
    const uint64_t oppking = /*b.kAttacked[king];*/ b.getAttacks<-C,King>();
    const uint64_t ownking = b.getAttacks< C,King>()/* | shift<C* 8>(b.getAttacks< C,King>())*/;
//     const uint64_t noBlockedPawns = ~(b.getPieces<C,Pawn>() & shift<C*-8>(b.getPieces<-C,Pawn>()));      //only own pawns
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
    for (const BoardBase::MoveTemplateB* bs = b.bsingle[CI]; bs->move.data; ++bs) {
        // restriced mobility in one move, including own defended pieces
        allBishopAttacks |= bs->d13;
        uint64_t batt1 = fold(bs->d13);
        uint64_t bmob1 = batt1 & restrictions;
        // remove blocked pawns, this positions will not be reachable even in two moves
//         uint64_t bmob2 = bmob1 & noBlockedPawns;
        // remove own pieces
        bmob1 &= ~b.getOcc<C>();
        uint64_t bmob1x = bmob1;
        // restriced mobility in two moves, including own defended pieces, excluding pieces defended in the 2nd move
//         uint64_t batt2 = b.build13Attack(bmob1);
//         bmob2 |= batt2 & restrictions & ~b.getOcc<C>();
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
//         bmob2 |= bxray & ~b.getOcc<C>() & restrictions;
//         ASSERT((bmob2 & bmob1) == bmob1);
        if (P != Endgame) {
            batt1 |= bxray;
//             batt2 |= bxray | batt1;
//             ASSERT((batt2 & batt1) == batt1);
            uint64_t a = batt1 & oppking;
            uint64_t d = batt1 & ownking;
            attackingPieces += attackB[popcount15(a)];
            nAttackers += !!a;
            defendingPieces += defenseB[popcount15(d)];
            nDefenders += !!d;
            print_debug(debugEval, "bAtt index: %d, bAtt value: %d\n", popcount15(a), attackB[popcount15(a)]);
            print_debug(debugEval, "bDef index: %d, bDef value: %d\n", popcount15(d), defenseB[popcount15(d)]);
//                 attackingPieces += attackB1[popcount15(batt1 & oppking)];
//                 attackingPieces += attackB2[popcount15(batt2 & oppking)];
//                 if (batt1 & ownking) defendingPieces++;
//             print_debug(debugMobility, "b attack  %d: d%2d:%3d, i%2d:%3d\n", CI, popcount15(batt1 & oppking), attackB1[popcount15(batt1 & oppking)], popcount15(batt2 & oppking), attackB2[popcount15(batt2 & oppking)]);
        }

        unsigned m = popcount(bmob1x);
        score_opening += bmo[m];
        score_endgame += bme[m];
        score = score + bm[m];
    }
    
    for (uint64_t p = b.getPieces<C, Knight>(); p; p &= p-1) {
        uint64_t sq = bit(p);
        uint64_t natt1 = b.knightAttacks[sq];
        uint64_t nmob1 = natt1 & restrictions;
//         uint64_t nmob2 = nmob1 & noBlockedPawns;
        nmob1 &= ~b.getOcc<C>();
        uint64_t nmob1x = nmob1;
//         uint64_t natt2 = b.buildNAttack(nmob1);
//         nmob2 |= natt2 & restrictions & ~b.getOcc<C>();
//         ASSERT((nmob2 & nmob1) == nmob1);
        if (P != Endgame) {
//             natt2 |= natt1;
//             ASSERT((natt2 & natt1) == natt1);
            uint64_t a = natt1 & oppking;
            uint64_t d = natt1 & ownking;
            attackingPieces += attackN[popcount15(a)];
            nAttackers += !!a;
            defendingPieces += defenseN[popcount15(d)];
            nDefenders += !!d;
            print_debug(debugEval, "nAtt index: %d, nAtt value: %d\n", popcount15(a), attackN[popcount15(a)]);
            print_debug(debugEval, "nDef index: %d, nDef value: %d\n", popcount15(d), defenseN[popcount15(d)]);
//                 attackingPieces += attackN1[popcount15(natt1 & oppking)];
//                 attackingPieces += attackN2[popcount15(natt2 & oppking)];
//                 if (b.knightAttacks[sq] & ownking) defendingPieces++;
//             print_debug(debugMobility, "n attack  %d: d%3d:%2d, i%2d:%3d\n", CI, popcount15(natt1 & oppking), attackN1[popcount15(natt1 & oppking)], popcount15(natt2 & oppking), attackN2[popcount15(natt2 & oppking)]);
        }
//         if (rmob3 & oppking) attackingPieces++;
        unsigned m = popcount(nmob1x);
        score_opening += nmo[m];
        score_endgame += nme[m];
        score = score + nm[m];

    }

    restrictions &= ~(b.getAttacks<-C,Bishop>() | b.getAttacks<-C,Knight>());
    __v2di allRookAttacks = zero;
    for (const BoardBase::MoveTemplateR* rs = b.rsingle[CI]; rs->move.data; ++rs) {
        // restriced mobility in one move, including own defended pieces
        allRookAttacks |= rs->d02;
        uint64_t ratt1 = fold(rs->d02);
        uint64_t rmob1 = ratt1 & restrictions;
        // remove blocked pawns, this positions will not be reachable even in two moves
//         uint64_t rmob2 = rmob1 & noBlockedPawns;
        // remove own pieces
        rmob1 &= ~b.getOcc<C>();
        uint64_t rmob1x = rmob1;
        // restriced mobility in two moves, including own defended pieces, excluding pieces defended in the 2nd move
//         uint64_t ratt2 = b.build02Attack(rmob1);
//         rmob2 |= ratt2 & restrictions & ~b.getOcc<C>();

        __v2di connectedQR = ~pcmpeqq(rs->d02 & v2queen, zero) & b.qsingle[CI][0].d02;
        if (b.rsingle[CI][1].move.data)
            connectedQR |= ~ pcmpeqq(rs->d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero) & (b.rsingle[CI][0].d02 | b.rsingle[CI][1].d02);
        uint64_t rxray = fold(connectedQR); //TODO optimize for queenless positions
        rmob1 |= rxray & ~b.getOcc<C>() & restrictions;
//         rmob2 |= rxray & ~b.getOcc<C>() & restrictions;
//         ASSERT((rmob2 & rmob1) == rmob1);
        if (P != Endgame) {
            ratt1 |= rxray;
//             ratt2 |= rxray | ratt1;
//             ASSERT((ratt2 & ratt1) == ratt1);
            uint64_t a = ratt1 & oppking;
            uint64_t d = ratt1 & ownking;
            attackingPieces += attackR[popcount15(a)];
            nAttackers += !!a;
            defendingPieces += defenseR[popcount15(d)];
            nDefenders += !!d;
            print_debug(debugEval, "rAtt index: %d, rAtt value: %d\n", popcount15(a), attackR[popcount15(a)]);
            print_debug(debugEval, "rDef index: %d, rDef value: %d\n", popcount15(d), defenseR[popcount15(d)]);
//                 attackingPieces += attackR1[popcount15(ratt1 & oppking)];
//                 attackingPieces += attackR2[popcount15(ratt2 & oppking)];
//                 if (ratt1 & ownking) defendingPieces++;
//             print_debug(debugMobility, "r attack  %d: d%3d:%2d, i%2d:%3d\n", CI, popcount15(ratt1 & oppking), attackR1[popcount15(ratt1 & oppking)], popcount15(ratt2 & oppking), attackR2[popcount15(ratt2 & oppking)]);
        }
        unsigned m = popcount(rmob1x);
        score_opening += rmo[m];
        score_endgame += rme[m];
        score = score + rm[m];
    }

    if (b.getPieces<C,Queen>()) {
        restrictions &= ~(b.getAttacks<-C,Rook>());
        uint64_t qatt1 = b.getAttacks<C,Queen>();
        uint64_t qmob1 = qatt1 & restrictions;
//         uint64_t qmob2 = qmob1 & noBlockedPawns;
        qmob1 &= ~b.getOcc<C>();
        uint64_t qmob1x = qmob1;
//         uint64_t qatt2 = b.build02Attack(qmob1) | b.build13Attack(qmob1);
//         qmob2 |= qatt2 & restrictions & ~b.getOcc<C>();
        unsigned q=bit(b.getPieces<C,Queen>());
        __v2di connectedBR = ~ pcmpeqq(b.qsingle[CI][0].d13 & _mm_set1_epi64x(b.getPieces<C,Bishop>()), zero)
            & allBishopAttacks
            & b.bits[q].mask13;
        connectedBR |= ~ pcmpeqq(b.qsingle[CI][0].d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero)
            & allRookAttacks
            & b.bits[q].mask02;
        uint64_t qxray = fold(connectedBR); //TODO optimize for queenless positions
        qmob1 |= qxray & ~b.getOcc<C>() & restrictions;
//         qmob2 |= qxray & ~b.getOcc<C>() & restrictions;
//         ASSERT((qmob2 & qmob1) == qmob1);
        if (P != Endgame) {
            qatt1 |= qxray;
//             qatt2 |= qxray | qatt1;
//             ASSERT((qatt2 & qatt1) == qatt1);
            uint64_t a = qatt1 & oppking;
            uint64_t d = qatt1 & ownking;
            attackingPieces += attackQ3[popcount15(a)];
            nAttackers += !!a;
            defendingPieces += defenseQ[popcount15(d)];
            nDefenders += !!d;
            print_debug(debugEval, "qAtt index: %d, qAtt value: %d\n", popcount15(a), attackQ3[popcount15(a)]);
            print_debug(debugEval, "qDef index: %d, qDef value: %d\n", popcount15(d), defenseQ[popcount15(d)]);
//                 attackingPieces += attackQ1[popcount15(qatt1 & oppking)];
//                 attackingPieces += attackQ2[popcount15(qatt2 & oppking)];
//                 if (qatt1 & ownking) defendingPieces++;
//             print_debug(debugMobility, "q attack  %d: d%2d:%3d, i%2d:%3d\n", CI, popcount15(qatt1 & oppking), attackQ1[popcount15(qatt1 & oppking)], popcount15(qatt2 & oppking), attackQ2[popcount15(qatt2 & oppking)]);
        }
        unsigned m = popcount(qmob1x);
        score_opening += qmo[m];
        score_endgame += qme[m];
        score = score + qm[m];
    }

    uint64_t a = b.getAttacks<C,Pawn>() & oppking;
    attackingPieces += attackP2[popcount15(a)];
    nAttackers += !!a;
    print_debug(debugEval, "pAtt index: %d, pAtt value: %d\n", popcount15(a), attackP2[popcount15(a)]);
    a = b.getAttacks<C,King>() & oppking;
    attackingPieces += attackK2[popcount15(a)];
    nAttackers += !!a;
    print_debug(debugEval, "kAtt index: %d, kAtt value: %d\n", popcount15(a), attackK2[popcount15(a)]);

    if (nAttackers>1)
        attackingPieces = (attackingPieces * (0x100 - (0x200>>nAttackers))) >> 8;
    else
        attackingPieces = 0;
//         attackingPieces = (nAttackersTab[nAttackers] * attackingPieces) >> 8;
//         if (P != Endgame) {
//             attackingPieces += attackP[popcount15(b.getAttacks<C,Pawn>() & oppking)];
//             attackingPieces += attackK[popcount15(b.getAttacks<C,King>() & oppking)];
//         }
    union {
        struct {
            short opening;
            short endgame;
        };
        DeltaScore s;
    } converter;
    converter.s = score;

    int newscore = (scale[b.material].endgame*converter.endgame + scale[b.material].opening*converter.opening) >> logEndgameTransitionSlope;
#ifdef MYDEBUG
    int oldscore = (scale[b.material].endgame*score_endgame + scale[b.material].opening*score_opening) >> logEndgameTransitionSlope;
    ASSERT(newscore == oldscore);
#endif    
    return newscore;
}
#if 0
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
#endif
template<Colors C>
int Eval::attack2(const BoardBase& b, const PawnEntry& p, int attackingPieces, int defendingPieces) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    uint64_t king = b.getPieces<-C,King>();
    
    unsigned kfile = bit(king) & 7;
    int realShield = p.shield2[EI][kfile];
    int possibleShield = realShield;
    
    // If castling is still possible, assess king safety by the pawn shield on
    // the side he can castle to or the better pawn shield
    if (b.cep.castling.color[EI].k) {
        int shieldG = p.shield2[EI][g1] - castlingTempo;
        int shieldH = p.shield2[EI][h1] - 2*castlingTempo;
        possibleShield = std::max(shieldG, possibleShield);
        possibleShield = std::max(shieldH, possibleShield);
    }
       
    if (b.cep.castling.color[EI].q) {
        int shieldB = p.shield2[EI][b1] - 2*castlingTempo;
        int shieldA = p.shield2[EI][a1] - 3*castlingTempo;
        possibleShield = std::max(shieldB, possibleShield);
        possibleShield = std::max(shieldA, possibleShield);
    }

    print_debug(debugEval, "pawn shield%d: %3d\n", EI, possibleShield);
    print_debug(debugEval, "piece defense%d: %3d\n", EI, defendingPieces);
    print_debug(debugEval, "piece attack%d: %3d\n", CI, attackingPieces);

    unsigned attack = (-possibleShield*pawnDefense + attackingPieces*pieceAttack - defendingPieces*pieceDefense)/256 + sizeof attackTable2 / (2 * sizeof(int));
    ASSERT(attack < sizeof attackTable2/sizeof(int));

    print_debug(debugEval, "attack index%d: %3d\n", CI, attack);
    print_debug(debugEval, "attack value%d: %3d\n", CI, attackTable2[attack]);
    
    return attackTable2[attack];
    
}

template<Colors C>
int Eval::kingSafety(const BoardBase& b) const {
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
int Eval::endgame(const BoardBase& b, const PawnEntry& pe, int /*sideToMove*/) const {
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

template<GamePhase P>
int Eval::mobilityDiff(const BoardBase& b, int& wap, int& bap, int& wdp, int& bdp) const {
    return mobility<White, P>(b, wap, wdp) - mobility<Black, P>(b, bap, bdp);
}

int Eval::attackDiff(const BoardBase& b, const PawnEntry& pe, int wap, int bap, int wdp, int bdp) const {
    return attack2<White>(b, pe, wap, bdp) - attack2<Black>(b, pe, bap, wdp);     
//         return attack<White>(b, pe, wap, bdp) - attack<Black>(b, pe, bap, wdp);
}
            
int Eval::operator () (const BoardBase& b, int stm ) const {
    int wap, bap;
    return operator()(b, stm, wap, bap);
}

int Eval::operator () (const BoardBase& b, int stm, int& wap, int& bap ) const {
#if defined(MYDEBUG)
    int cmp = b.keyScore.score.calc(b.material, *this);
    CompoundScore value = { 0, 0 };
    for (int p=Rook; p<=King; ++p) {
        for (uint64_t x=b.getPieces<White>(p); x; x&=x-1) {
            unsigned sq=bit(x);
            print_debug(debugEval, "materialw%d     %d\n", p, getPS( p, sq).calc(b.material, *this));
            value = value + getPS( p, sq);
        }
        for (uint64_t x=b.getPieces<Black>(p); x; x&=x-1) {
            unsigned sq=bit(x);
            print_debug(debugEval, "materialb%d     %d\n", p, getPS(-p, sq).calc(b.material, *this));
            value = value + getPS(-p, sq);
        }
    }
    int v = value.calc(b.material, *this);
    if (v != cmp) asm("int3");
#endif
    if (b.getPieces<White,Pawn>() + b.getPieces<Black,Pawn>()) {
        PawnEntry pe = pawns(b);

        int m, a, e, pa;
        int wdp, bdp;
        m = mobilityDiff<Opening>(b, wap, bap, wdp, bdp);
        a = scale[b.material].opening ? scale[b.material].opening*attackDiff(b, pe, wap, bap, wdp, bdp) >> logEndgameTransitionSlope : 0;
        e = scale[b.material].endgame ? scale[b.material].endgame *(endgame<White>(b, pe, stm) - endgame<Black>(b, pe, stm)) >> logEndgameTransitionSlope : 0;
        pa = pe.score;
        int pi = pieces<White>(b, pe) - pieces<Black>(b, pe);
        print_debug(debugEval, "endgame:        %d\n", e);
        print_debug(debugEval, "mobility:       %d\n", m);
        print_debug(debugEval, "pawns:          %d\n", pa);
        print_debug(debugEval, "attack:         %d\n", a);
        print_debug(debugEval, "pieces:         %d\n", pi);

        return e + m + pa + a + pi;
    } else {     // pawnless endgame
        int mat = b.keyScore.score.calc(b.material, *this);
        int wap, bap, wdp, bdp; //FIXME not needed here
        int m = mobilityDiff<Endgame>(b, wap, bap, wdp, bdp);
        int p = (mat>0 ? 1:4)*kingSafety<White>(b) - (mat<0 ? 1:4)*kingSafety<Black>(b);
        return m + p;        
    }
}

void Eval::ptClear() {
    pt->clear();
}

#define SETPARM(x) x = p[ #x ].value;

void Eval::setParameters(const Parameters& p)
{
    SETPARM(pawn.hor.value.opening);
    SETPARM(pawn.hor.inflection.opening);
    SETPARM(pawn.vert.value.opening);
    SETPARM(pawn.vert.inflection.opening);
    sigmoid(pawn.hor.opening, -pawn.hor.value.opening, pawn.hor.value.opening, pawn.hor.inflection.opening );
    sigmoid(pawn.vert.opening, -pawn.vert.value.opening, pawn.vert.value.opening, pawn.vert.inflection.opening );
    SETPARM(pawn.hor.value.endgame);
    SETPARM(pawn.hor.inflection.endgame);
    SETPARM(pawn.vert.value.endgame);
    SETPARM(pawn.vert.inflection.endgame);
    sigmoid(pawn.hor.endgame, -pawn.hor.value.endgame, pawn.hor.value.endgame, pawn.hor.inflection.endgame );
    sigmoid(pawn.vert.endgame, -pawn.vert.value.endgame, pawn.vert.value.endgame, pawn.vert.inflection.endgame );
    SETPARM(pawn.vcenter.opening);
    SETPARM(pawn.vcenter.endgame);
    
    SETPARM(knight.hor.value.opening);
    SETPARM(knight.hor.inflection.opening);
    sigmoid(knight.hor.opening, -knight.hor.value.opening, knight.hor.value.opening, knight.hor.inflection.opening );
    SETPARM(knight.vert.value.opening);
    SETPARM(knight.vert.inflection.opening);
    SETPARM(knight.vcenter.opening);
    sigmoid(knight.vcenter.opening, knight.vert.opening, -knight.vert.value.opening, knight.vert.value.opening, knight.vert.inflection.opening );
    SETPARM(knight.hor.value.endgame);
    SETPARM(knight.hor.inflection.endgame);
    sigmoid(knight.hor.endgame, -knight.hor.value.endgame, knight.hor.value.endgame, knight.hor.inflection.endgame );
    SETPARM(knight.vert.value.endgame);
    SETPARM(knight.vert.inflection.endgame);
    SETPARM(knight.vcenter.endgame);
    sigmoid(knight.vcenter.endgame, knight.vert.endgame, -knight.vert.value.endgame, knight.vert.value.endgame, knight.vert.inflection.endgame );
    
    SETPARM(rookPair);
    SETPARM(rook.hor.value.opening);
    SETPARM(rook.hor.inflection.opening);
    sigmoid(rook.hor.opening, -rook.hor.value.opening, rook.hor.value.opening, rook.hor.inflection.opening );
    SETPARM(rook.vert.value.opening);
    SETPARM(rook.vert.inflection.opening);
    SETPARM(rook.vcenter.opening);
    sigmoid(rook.vcenter.opening, rook.vert.opening, -rook.vert.value.opening, rook.vert.value.opening, rook.vert.inflection.opening );
    SETPARM(rook.hor.value.endgame);
    SETPARM(rook.hor.inflection.endgame);
    sigmoid(rook.hor.endgame, -rook.hor.value.endgame, rook.hor.value.endgame, rook.hor.inflection.endgame );
    SETPARM(rook.vert.value.endgame);
    SETPARM(rook.vert.inflection.endgame);
    SETPARM(rook.vcenter.endgame);
    sigmoid(rook.vcenter.endgame, rook.vert.endgame, -rook.vert.value.endgame, rook.vert.value.endgame, rook.vert.inflection.endgame );

    SETPARM(bishop.hor.value.opening);
    SETPARM(bishop.hor.inflection.opening);
    sigmoid(bishop.hor.opening, -bishop.hor.value.opening, bishop.hor.value.opening, bishop.hor.inflection.opening );
    SETPARM(bishop.vert.value.opening);
    SETPARM(bishop.vert.inflection.opening);
    SETPARM(bishop.vcenter.opening);
    sigmoid(bishop.vcenter.opening, bishop.vert.opening, -bishop.vert.value.opening, bishop.vert.value.opening, bishop.vert.inflection.opening );
    SETPARM(bishop.hor.value.endgame);
    SETPARM(bishop.hor.inflection.endgame);
    sigmoid(bishop.hor.endgame, -bishop.hor.value.endgame, bishop.hor.value.endgame, bishop.hor.inflection.endgame );
    SETPARM(bishop.vert.value.endgame);
    SETPARM(bishop.vert.inflection.endgame);
    SETPARM(bishop.vcenter.endgame);
    sigmoid(bishop.vcenter.endgame, bishop.vert.endgame, -bishop.vert.value.endgame, bishop.vert.value.endgame, bishop.vert.inflection.endgame );

    SETPARM(queenPair);
    SETPARM(queen.hor.value.opening);
    SETPARM(queen.hor.inflection.opening);
    sigmoid(queen.hor.opening, -queen.hor.value.opening, queen.hor.value.opening, queen.hor.inflection.opening );
    SETPARM(queen.vert.value.opening);
    SETPARM(queen.vert.inflection.opening);
    SETPARM(queen.vcenter.opening);
    sigmoid(queen.vcenter.opening, queen.vert.opening, -queen.vert.value.opening, queen.vert.value.opening, queen.vert.inflection.opening );
    SETPARM(queen.hor.value.endgame);
    SETPARM(queen.hor.inflection.endgame);
    sigmoid(queen.hor.endgame, -queen.hor.value.endgame, queen.hor.value.endgame, queen.hor.inflection.endgame );
    SETPARM(queen.vert.value.endgame);
    SETPARM(queen.vert.inflection.endgame);
    SETPARM(queen.vcenter.endgame);
    sigmoid(queen.vcenter.endgame, queen.vert.endgame, -queen.vert.value.endgame, queen.vert.value.endgame, queen.vert.inflection.endgame );
    
    SETPARM(king.hor.value.opening);
    SETPARM(king.hor.inflection.opening);
    sigmoid(king.hor.opening, -king.hor.value.opening, king.hor.value.opening, king.hor.inflection.opening );
    SETPARM(king.vert.value.opening);
    SETPARM(king.vert.inflection.opening);
    SETPARM(king.vcenter.opening);
    sigmoid(king.vcenter.opening, king.vert.opening, -king.vert.value.opening, king.vert.value.opening, king.vert.inflection.opening );
    SETPARM(king.hor.value.endgame);
    SETPARM(king.hor.inflection.endgame);
    sigmoid(king.hor.endgame, -king.hor.value.endgame, king.hor.value.endgame, king.hor.inflection.endgame );
    SETPARM(king.vert.value.endgame);
    SETPARM(king.vert.inflection.endgame);
    SETPARM(king.vcenter.endgame);
    sigmoid(king.vcenter.endgame, king.vert.endgame, -king.vert.value.endgame, king.vert.value.endgame, king.vert.inflection.endgame );
    
    SETPARM(endgameMaterial);

    SETPARM(bishopPair);
    SETPARM(bishopBlockPasser);

    SETPARM(knightPair);
    SETPARM(knightBlockPasser);

    SETPARM(rookTrapped);
    SETPARM(rookOpen);
    SETPARM(rookHalfOpen);
    SETPARM(rookWeakPawn);

    SETPARM(pawnBackward);
    SETPARM(pawnBackwardOpen);
    SETPARM(pawnIsolatedCenter);
    SETPARM(pawnIsolatedEdge);
    SETPARM(pawnIsolatedOpen);
    SETPARM(pawnDouble);
    SETPARM(pawnShoulder);

    SETPARM(pawnPasser2);
    SETPARM(pawnPasser7);
    SETPARM(pawnPasserSlope);
    sigmoid( pawnPasser, pawnPasser2, pawnPasser7, 6, pawnPasserSlope );

    SETPARM(pawn.value.opening);
    SETPARM(knight.value.opening);
    SETPARM(rook.value.opening);
    SETPARM(queen.value.opening);
    SETPARM(bishop.value.opening);
    king.value.opening = 0;
    SETPARM(pawn.value.endgame);
    SETPARM(knight.value.endgame);
    SETPARM(rook.value.endgame);
    SETPARM(queen.value.endgame);
    SETPARM(bishop.value.endgame);
    king.value.endgame = 0;

    SETPARM(pawn.corner.opening);
    SETPARM(knight.corner.opening);
    SETPARM(rook.corner.opening);
    SETPARM(queen.corner.opening);
    SETPARM(bishop.corner.opening);
    SETPARM(king.corner.opening);
    SETPARM(pawn.corner.endgame);
    SETPARM(knight.corner.endgame);
    SETPARM(rook.corner.endgame);
    SETPARM(queen.corner.endgame);
    SETPARM(bishop.corner.endgame);
    SETPARM(king.corner.endgame);

    SETPARM(knight.mobility.opening);
    SETPARM(rook.mobility.opening);
    SETPARM(queen.mobility.opening);
    SETPARM(bishop.mobility.opening);
    SETPARM(knight.mobility.endgame);
    SETPARM(rook.mobility.endgame);
    SETPARM(queen.mobility.endgame);
    SETPARM(bishop.mobility.endgame);

    SETPARM(knight.mobslope.opening);
    SETPARM(bishop.mobslope.opening);
    SETPARM(rook.mobslope.opening);
    SETPARM(queen.mobslope.opening);
    SETPARM(knight.mobslope.endgame);
    SETPARM(bishop.mobslope.endgame);
    SETPARM(rook.mobslope.endgame);
    SETPARM(queen.mobslope.endgame);
    sigmoid(nmo, -knight.mobility.opening, knight.mobility.opening, 0, knight.mobslope.opening);
    sigmoid(bmo, -bishop.mobility.opening, bishop.mobility.opening, 0, bishop.mobslope.opening);
    sigmoid(rmo, -rook.mobility.opening, rook.mobility.opening, 0, rook.mobslope.opening);
    sigmoid(qmo, -queen.mobility.opening, queen.mobility.opening, 0, queen.mobslope.opening);
    sigmoid(nme, -knight.mobility.endgame, knight.mobility.endgame, 0, knight.mobslope.endgame);
    sigmoid(bme, -bishop.mobility.endgame, bishop.mobility.endgame, 0, bishop.mobslope.endgame);
    sigmoid(rme, -rook.mobility.endgame, rook.mobility.endgame, 0, rook.mobslope.endgame);
    sigmoid(qme, -queen.mobility.endgame, queen.mobility.endgame, 0, queen.mobslope.endgame);
    sigmoid(nm, Parameters::Piece::Phase{ -knight.mobility.opening,  -knight.mobility.endgame }, knight.mobility, Parameters::Piece::Phase{0,0}, knight.mobslope);
    sigmoid(bm, Parameters::Piece::Phase{ -bishop.mobility.opening,  -bishop.mobility.endgame }, bishop.mobility, Parameters::Piece::Phase{0,0}, bishop.mobslope);
    sigmoid(rm, Parameters::Piece::Phase{ -rook.mobility.opening,  -rook.mobility.endgame }, rook.mobility, Parameters::Piece::Phase{0,0}, rook.mobslope);
    sigmoid(qm, Parameters::Piece::Phase{ -queen.mobility.opening,  -queen.mobility.endgame }, queen.mobility, Parameters::Piece::Phase{0,0}, queen.mobslope);
 
    initPS();

    SETPARM(dMaxCapture);
    SETPARM(dMaxExt);
    dMaxExt += dMaxCapture;
    SETPARM(dMinDualExt);
    dMinDualExt = dMaxExt-dMinDualExt;
    SETPARM(dMinSingleExt);
    dMinSingleExt = dMaxExt-dMinSingleExt;
    SETPARM(flags);
    SETPARM(dMinForkExt);
    dMinForkExt = dMaxExt-dMinForkExt;
    SETPARM(dMinMateExt);
    dMinMateExt = dMaxExt-dMinMateExt;
    SETPARM(dMinPawnExt);
    dMinPawnExt = dMaxExt-dMinPawnExt;
    SETPARM(dNullIncr);
    SETPARM(dVerifyIncr);
    SETPARM(dMinReduction);
    dMinReduction += dMaxExt;
    SETPARM(dRedCapture);
    SETPARM(dRedCheck);

    SETPARM(dMaxExtCheck);
    dMaxExtCheck += dMaxExt;
    SETPARM(dMaxExtPawn);
    dMaxExtPawn += dMaxExt;
    SETPARM(dMinExtDisco);
    dMinExtDisco = dMaxExt - dMinExtDisco;
//     SETPARM(dMaxExtDisco);
//     dMaxExtDisco += dMaxExt;
    
    SETPARM(oppKingOwnPawnV);
    SETPARM(ownKingOwnPawnV);
    SETPARM(oppKingOwnPasserV);
    SETPARM(ownKingOwnPasserV);
    SETPARM(pawnConnPasserV);
    sigmoid( oppKingOwnPawn,   -oppKingOwnPawnV,   oppKingOwnPawnV, 1, 10);  // only 1..7 used
    sigmoid( ownKingOwnPawn,    ownKingOwnPawnV,  -ownKingOwnPawnV, 1, 10);
    sigmoid( oppKingOwnPasser, -oppKingOwnPasserV, oppKingOwnPasserV, 1, 10);  // only 1..7 used
    sigmoid( ownKingOwnPasser,  ownKingOwnPasserV,-ownKingOwnPasserV, 1, 10);
    sigmoid( pawnConnPasser, 0, pawnConnPasserV, 6, 1.0 );


    SETPARM(standardError);
    SETPARM(standardSigma);
    SETPARM(calcMeanError);
    SETPARM(prune1);
    SETPARM(prune2);
    SETPARM(prune1c);
    SETPARM(prune2c);

    SETPARM(dRed[0]);
    SETPARM(dRed[1]);
    SETPARM(dRed[2]);
    SETPARM(dRed[3]);
    SETPARM(dRed[4]);
    SETPARM(dRed[5]);
    SETPARM(dRed[6]);
    SETPARM(dRed[7]);

    SETPARM(bishopOwnPawn);
    SETPARM(bishopOppPawn);
    SETPARM(bishopNotOwnPawn);
    SETPARM(bishopNotOppPawn);

    SETPARM(aspirationLow);
    SETPARM(aspirationHigh);
    SETPARM(aspirationHigh2);
    SETPARM(tempo);
    for (unsigned i=8; i<=maxDepth; ++i) {
        dRed[i] = dRed[7];
    }

    SETPARM(kingShield.base);
    SETPARM(kingShield.vdelta);
    SETPARM(kingShield.idelta);
    SETPARM(kingShield.odelta);
    float scale = 9.0/((1.0+kingShield.odelta+kingShield.idelta) * (1.0+kingShield.vdelta+kingShield.vdelta*kingShield.vdelta));
    kingShield.center[0] = scale*kingShield.base;
    kingShield.outer[0] = kingShield.center[0]*kingShield.odelta;
    kingShield.inner[0] = kingShield.center[0]*kingShield.idelta;
    for (unsigned i=0; i<2; ++i) {
        kingShield.center[i+1] = kingShield.center[i] * kingShield.vdelta;
        kingShield.outer[i+1]  = kingShield.outer[i]  * kingShield.vdelta;
        kingShield.inner[i+1]  = kingShield.inner[i]  * kingShield.vdelta;
    }

    SETPARM(kingShield.openFile);
    SETPARM(kingShield.halfOpenFile);

    SETPARM(pawnDefense);
    SETPARM(pieceDefense);
    SETPARM(pieceAttack);

    SETPARM(attackTotal);
    sigmoid(attackTable2, -attackTotal, attackTotal, sizeof attackTable2/(2*sizeof(int)), 128);

    SETPARM(rook.attack);
    SETPARM(bishop.attack);
    SETPARM(knight.attack);
    SETPARM(queen.attack);
    SETPARM(pawn.attack);
    SETPARM(king.attack);
    SETPARM(rook.defense);
    SETPARM(bishop.defense);
    SETPARM(knight.defense);
    SETPARM(queen.defense);

    SETPARM(castlingTempo);

    sigmoid(attackR, rook.attack, rook.attack*2, 0, 3.0, 1);
    sigmoid(attackB, bishop.attack, bishop.attack*2, 0, 3.0, 1);
    sigmoid(attackN, knight.attack, knight.attack*2, 0, 3.0, 1);
    sigmoid(attackQ3, queen.attack, queen.attack*2, 0, 3.0, 1);
    sigmoid(attackP2, pawn.attack, pawn.attack*2, 0, 3.0, 1);
    sigmoid(attackK2, king.attack, king.attack*2, 0, 3.0, 1);

    sigmoid(defenseR, rook.defense, rook.defense*2, 0, 3.0, 1);
    sigmoid(defenseB, bishop.defense, bishop.defense*2, 0, 3.0, 1);
    sigmoid(defenseN, knight.defense, knight.defense*2, 0, 3.0, 1);
    sigmoid(defenseQ, queen.defense, queen.defense*2, 0, 3.0, 1);

    SETPARM(attackFirst);
    SETPARM(attackSlope);
    sigmoid(nAttackersTab, attackFirst, 256, 0, attackSlope, 1);
}

int CompoundScore::calc(int material, const Eval& eval) const
{
    return (eval.scale[material].opening*opening + eval.scale[material].endgame*endgame) >> logEndgameTransitionSlope;
}

void sigmoid(int n, int p[], double start, double end, double dcenter, double width) {
    double t0 = -dcenter;
    double t1 = n-dcenter;
    double l0 = 1/(1+exp(-t0/width));
    double l1 = 1/(1+exp(-t1/width));

    double r = (end - start)/(l1-l0);
    double a = start - l0*r;
    for (int i = 0; i <= n; ++i) {
        double t = i - dcenter;
        p[i] = lrint(a + r/(1.0 + exp(-t/width)));
    }
}
