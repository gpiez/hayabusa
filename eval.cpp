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
#include "eval.h"
#include "board.h"
#include "options.h"
#include "transpositiontable.tcc"
#include "parameters.h"
#include "bits.h"
#include "stats.h"

unsigned Eval::distance[nSquares][nSquares];
uint64_t Eval::ruleOfSquare[nColors][nSquares];
uint64_t Eval::keySquare[nColors][nSquares];

static const __v2di zero = {0 };
static constexpr uint64_t darkSquares = 0xaa55aa55aa55aa55;
static constexpr int ksv[nSquares] = {
    0,  2,  4,  6,  6,  4,  2,  0,
    2, 10, 12, 14, 14, 12, 10,  2,
    4, 12, 20, 22, 22, 20, 12,  4,
    6, 14, 22, 30, 30, 22, 14,  6,
    6, 14, 22, 30, 30, 22, 14,  6,
    4, 12, 20, 22, 22, 20, 12,  4,
    2, 10, 12, 14, 14, 12, 10,  2,
    0,  2,  4,  6,  6,  4,  2,  0 };
static constexpr int ksvb[2][nSquares] = {
    {
        0, 10, 20, 30, 30, 26, 22, 18,
        10, 21, 32, 43, 43, 36, 28, 22,
        20, 32, 44, 56, 56, 45, 36, 26,
        30, 43, 56, 69, 69, 56, 44, 30,
        30, 43, 56, 69, 69, 56, 43, 30,
        26, 36, 45, 56, 56, 44, 32, 20,
        22, 28, 36, 43, 43, 32, 21, 10,
        18, 22, 26, 30, 30, 20, 10,  0 },{
        18, 22, 26, 30, 30, 20, 10,  0,
        22, 28, 36, 43, 43, 32, 21, 10,
        26, 36, 45, 56, 56, 44, 32, 20,
        30, 43, 56, 69, 69, 56, 43, 30,
        30, 43, 56, 69, 69, 56, 44, 30,
        20, 32, 44, 56, 56, 45, 36, 26,
        10, 21, 32, 43, 43, 36, 28, 22,
        0, 10, 20, 30, 30, 26, 22, 18 } };

void collectArgs(std::vector<uint64_t>) {}

template<typename... Args>
void collectArgs(std::vector<uint64_t>& argvector, uint64_t first, Args... tail) {
    argvector.push_back(first);
    collectArgs(argvector, tail...); }

template<typename... Args>
void printBit(Args... argpack) {
    std::vector<uint64_t> argvector;
    collectArgs(argvector, argpack...);
    for (int y=8; y; ) {
        --y;
        for (size_t w=0; w<argvector.size(); ++w) {
            for (int x=0; x<8; ++x) {
                if ((argvector[w] >> (x+y*8)) & 1)
                    std::cout << "X";
                else
                    std::cout << "."; }
            std::cout << "  "; }
        std::cout << std::endl; }
    std::cout << std::endl; }

Eval::Eval(uint64_t pHashSize, const Parameters& p) {
    pt = new TranspositionTable<PawnEntry, 4, PawnKey>(pHashSize);
//     std::cerr << "Eval::Eval " << pHashSize << std::endl;
    init(p); }

Eval::~Eval() {
    delete pt; }

void Eval::init(const Parameters& p) {
    Init parameters(*this);
    parameters.setEvalParameters(p); }

void Eval::initTables() {
    for (int x0=0; x0<8; ++x0)
        for (int y0=0; y0<8; ++y0)
            for (int x1=0; x1<8; ++x1)
                for (int y1=0; y1<8; ++y1)
                    distance[x0+8*y0][x1+8*y1] = std::max(abs(x0-x1), abs(y0-y1));

    for (int x=0; x<8; ++x) {
        for (int y=1; y<7; ++y) {
            unsigned p=x+8*y;
            uint64_t w = 1ULL<<p;
            uint64_t b = w;
            for (int s = 0; s<7-y; ++s) {
                w |= (w & ~file<'a'>()) >> 1;
                w |= (w & ~file<'h'>()) << 1;
                w |= w << 010; }
            for (int s = 0; s<y; ++s) {
                b |= (b & ~file<'a'>()) >> 1;
                b |= (b & ~file<'h'>()) << 1;
                b |= b >> 010; }
            ruleOfSquare[0][p] = w;
            ruleOfSquare[1][p] = b;

            if (x==0) {
                keySquare[0][p] = (rank<White,8>() | rank<White,7>()) & file<'b'>();
                keySquare[1][p] = (rank<Black,8>() | rank<Black,7>()) & file<'b'>(); }
            else if (x==7) {
                keySquare[0][p] = (rank<White,8>() | rank<White,7>()) & file<'g'>();
                keySquare[1][p] = (rank<Black,8>() | rank<Black,7>()) & file<'g'>(); }
            else {
                w = 1ULL << (p+020);
                b = 1ULL << (p-020);
                if (y>=4) {
                    w |= 1ULL << (p+010);
                    b |= 1ULL << (p-010); }
                if (y==7) {
                    w |= 1ULL << p;
                    b |= 1ULL << p; }
                w |= (w & ~file<'a'>()) >> 1;
                w |= (w & ~file<'h'>()) << 1;
                b |= (b & ~file<'a'>()) >> 1;
                b |= (b & ~file<'h'>()) << 1;
                keySquare[0][p] = w;
                keySquare[1][p] = b; } } } }

template<Colors C>
CompoundScore Eval::pieces(const Board& b, const PawnEntry& p) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    CompoundScore value(0,0);
    if  (   (  b.getPieces<C, Rook>() & (file<'h'>()|file<'g'>()) & (rank<C,1>()|rank<C,2>())
               && b.getPieces<C, Pawn>() & file<'h'>() & (rank<C,2>() | rank<C,3>())
               &&  (   b.getPieces<C, King>() & file<'g'>() & rank<C,1>()
                       || (b.getPieces<C, King>() & file<'f'>() & rank<C,1>() && b.getPieces<C, Pawn>() & file<'g'>() & (rank<C,2>() | rank<C,3>()))))
            ||  (  b.getPieces<C, Rook>() & (file<'a'>()|file<'b'>()) & (rank<C,1>()|rank<C,2>())
                   && b.getPieces<C, Pawn>() & file<'a'>() & (rank<C,2>() | rank<C,3>())
                   &&  (   b.getPieces<C, King>() & file<'b'>() & rank<C,1>()
                           || (b.getPieces<C, King>() & file<'c'>() & rank<C,1>() && b.getPieces<C, Pawn>() & file<'b'>() & (rank<C,2>() | rank<C,3>())))))
        value = value + rookTrapped;

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

    uint64_t wpasser = p.passers[0];
    wpasser += wpasser >> 010;
    wpasser += wpasser >> 020;
    wpasser += wpasser >> 040;

    uint64_t bpasser = p.passers[1];
    bpasser += bpasser << 010;
    bpasser += bpasser << 020;
    bpasser += bpasser << 040;

    value = value + rookHalfOpen [popcount(own & ~opp)>>3] [popcount3((b.getPieces<C,Rook>() & own & ~opp))];
    value = value + rookOpen     [popcount(own &  opp)>>3] [popcount3((b.getPieces<C,Rook>() & own &  opp))];
    value = value + rookWeakPawn[ popcount3((b.getPieces<C,Rook>() & weak)) ];
    value = value + rookOwnPasser[ popcount3((b.getPieces<C,Rook>() & (C == White ? wpasser : bpasser) )) ];
    value = value + rookOppPasser[ popcount3((b.getPieces<C,Rook>() & (C == White ? bpasser : wpasser) )) ];

    value = value + knightBlockPasser [ popcount3((b.getPieces<C,Knight>() & shift<C* 8>(p.passers[EI]))) ];
    value = value + bishopBlockPasser [ popcount3((b.getPieces<C,Bishop>() & shift<C* 8>(p.passers[EI]))) ];

    int darkB = !!(b.getPieces<C,Bishop>() & darkSquares);
    int lightB = !!(b.getPieces<C,Bishop>() & ~darkSquares);
    value = value + bishopOwnPawn[ p.dark[CI] * darkB + p.light[CI] * lightB ];
    value = value + bishopOppPawn[ p.dark[EI] * darkB + p.light[EI] * lightB ];

    value = value + bishopNotOwnPawn[ p.dark[CI] * lightB + p.light[CI] * darkB ];
    value = value + bishopNotOppPawn[ p.dark[EI] * lightB + p.light[EI] * darkB ];
    return value; }

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
        return shieldMirrored[index]; }
//TODO asess blocked pawns and their consequences on an attack
PawnEntry Eval::pawns(const Board& b) const {
    PawnKey k=b.keyScore.pawnKey;
    Sub<PawnEntry, 4>* st = pt->getSubTable(k);
    PawnEntry pawnEntry;
    if (k >> PawnEntry::upperShift && pt->retrieve(st, k, pawnEntry)) {
        stats.pthit++; }
    else {
        stats.ptmiss++;
        uint64_t wpawn = b.getPieces<White,Pawn>();
        uint64_t bpawn = b.getPieces<Black,Pawn>();

        CompoundScore score = pawnShoulder2[ popcount15(wpawn & wpawn<<1 & ~file<'a'>()) ];
        score = score - pawnShoulder2[ popcount15(bpawn & bpawn<<1 & ~file<'a'>()) ];

        uint64_t wFront = wpawn << 8 | wpawn << 16;
        wFront |= wFront << 16 | wFront << 32;
        uint64_t wBack = wpawn >> 8 | wpawn >> 16;
        wBack |= wBack >> 16 | wBack >> 32;

        uint64_t bBack = bpawn << 8 | bpawn << 16;
        bBack |= bBack << 16 | bBack << 32;
        uint64_t bFront = bpawn >> 8 | bpawn >> 16;
        bFront |= bFront >> 16 | bFront >> 32;

        score = score + pawnDouble2[popcount15(wpawn & wBack)];
        score = score - pawnDouble2[popcount15(bpawn & bBack)];

        // calculate squares which are or possibly are attacked by w and b pawns
        // take only the most advanced attacker and the most backward defender into account
        uint64_t wAttack = ((wFront & ~file<'a'>()) >> 1) | (wFront << 1 & ~file<'a'>());
        uint64_t bAttack = ((bFront & ~file<'a'>()) >> 1) | (bFront << 1 & ~file<'a'>());

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
        if(TRACE_DEBUG && Options::debug & debugEval) {
            printBit(wBackward);
            printBit(bBackward); }

        score = score + pawnBackward2[popcount15(wBackward)];
        score = score - pawnBackward2[popcount15(bBackward)];

        // backward pawns on open files, reduce to (uint8_t) for rooks later
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
        if(TRACE_DEBUG && Options::debug & debugEval) {
            printBit(wBack8);
            printBit(bBack8); }
        //TODO scale pbwo with the number of rooks + queens
        score = score + pawnBackwardOpen2[popcount15(wBackOpen)];
        score = score - pawnBackwardOpen2[popcount15(bBackOpen)];

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
        // true on either exactly 1 attack
        uint64_t wAttack1 = ((wFront & ~file<'a'>()) >> 1) ^ (wFront << 1 & ~file<'a'>());
        uint64_t bAttack1 = ((bFront & ~file<'a'>()) >> 1) ^ (bFront << 1 & ~file<'a'>());
        // nothing in front, but possibly attacked by =1 opponent pawn,
        // supported by at least one pawn
        uint64_t wCandidate1 = wpawn & ~bFront & bAttack1 & wAttack;
        uint64_t bCandidate1 = bpawn & ~wFront & wAttack1 & bAttack;
        // if directly attacked, we are attacked by ==1 pawn, we need support
        // by >=1 pawn then. Mask out attacked pawn with no support
        wCandidate1 &= ~(b.getAttacks<Black,Pawn>() & ~b.getAttacks<White,Pawn>());
        bCandidate1 &= ~(b.getAttacks<White,Pawn>() & ~b.getAttacks<Black,Pawn>());
        // nothing in front, but possibly attacked by >=1 opponent pawn and
        // not by ==1 pawn (=attack by 2), supported by >=1 pawn
        // and not ==1 pawn (=support by 2)
        uint64_t wCandidate2 = wpawn & ~bFront & bAttack & ~bAttack1 & wAttack & ~wAttack1;
        uint64_t bCandidate2 = bpawn & ~wFront & wAttack & ~wAttack1 & bAttack & ~bAttack1;
        // mask out attackeck pawns, which are not supported by at least 2 pawns
        // border masking is not needed, no border pawn can be Candidate2
        wCandidate2 &= ~(b.getAttacks<Black,Pawn>() & ~(wpawn>>9 & wpawn>>7));
        bCandidate2 &= ~(b.getAttacks<White,Pawn>() & ~(bpawn<<9 & bpawn<<7));
        uint64_t wCandidate = wCandidate1 | wCandidate2;
        uint64_t bCandidate = bCandidate1 | bCandidate2;

        unsigned i;
        pawnEntry.passers[0] = wPassed;
        for (i = 0; wPassed; wPassed &= wPassed - 1, i++) {
            uint64_t pos = bit(wPassed);
            uint64_t y = pos >> 3;
            ASSERT(y>0 && y<7);
            score = score + pawnPasser22[y-1];
            if (wPassedConnected & (1LL << pos)) score = score + pawnConnPasser22[y-1];

//             print_debug(debugEval, "pawn wpasser:   %d\n", pawnPasser22[y-1]);
        }
//		if (wCandidate) {
//			printBit(wpawn, bpawn, wCandidate);
//			std::cout << pawnCandidate[(bit(wCandidate)>>3)-1].opening <<
//					" / " << pawnCandidate[(bit(wCandidate)>>3)-1].endgame <<
//					std::endl << "-------------------------------------" <<
//					std::endl;
//		}
        for (; wCandidate; wCandidate &= wCandidate-1)
            score = score + pawnCandidate[(bit(wCandidate)>>3)-1];

        pawnEntry.passers[1] = bPassed;
        for ( i=0; bPassed; bPassed &= bPassed-1, i++ ) {
            uint64_t pos = bit(bPassed);
            uint64_t y = pos  >> 3;
            ASSERT(y>0 && y<7);
            score = score - pawnPasser22[6-y];
            if (bPassedConnected & (1LL << pos)) score = score - pawnConnPasser22[6-y];

//             print_debug(debugEval, "pawn bpasser:   %d\n", pawnPasser22[6-y]);
        }
        for (; bCandidate; bCandidate &= bCandidate-1)
            score = score - pawnCandidate[6-(bit(bCandidate)>>3)];

        // possible weak pawns are adjacent to open files or below pawns an a adjacent file on both sides
        // rook pawns are always adjacent to a "open" file
        uint64_t wOpenFiles = ~(wFront | wpawn | wBack);
        uint64_t bOpenFiles = ~(bFront | bpawn | bBack);
        pawnEntry.openFiles[0] = (uint8_t)wOpenFiles;
        pawnEntry.openFiles[1] = (uint8_t)bOpenFiles;

        uint64_t wIsolani = wpawn & (wOpenFiles<<1 | 0x101010101010101LL) & (wOpenFiles>>1 | 0x8080808080808080LL);
        uint64_t bIsolani = bpawn & (bOpenFiles<<1 | 0x101010101010101LL) & (bOpenFiles>>1 | 0x8080808080808080LL);
        score = score + pawnIsolatedCenter2[ (popcount15(wIsolani & ~(file<'a'>()|file<'h'>()))) ];
        score = score - pawnIsolatedCenter2[ (popcount15(bIsolani & ~(file<'a'>()|file<'h'>()))) ];
        score = score + pawnIsolatedEdge2[ (popcount15(wIsolani & (file<'a'>()|file<'h'>()))) ];
        score = score - pawnIsolatedEdge2[ (popcount15(bIsolani & (file<'a'>()|file<'h'>()))) ];
        score = score + pawnIsolatedOpen2[ (popcount15(wIsolani & ~bFront)) ];
        score = score - pawnIsolatedOpen2[ (popcount15(bIsolani & ~wFront)) ];
        pawnEntry.score = score.packed();

        uint64_t wDarkpawn = wpawn & darkSquares;
        uint64_t bDarkpawn = bpawn & darkSquares;
        pawnEntry.dark[0] = popcount(wDarkpawn);
        pawnEntry.dark[1] = popcount(bDarkpawn);
        pawnEntry.light[0] = popcount(wpawn - wDarkpawn);
        pawnEntry.light[1] = popcount(bpawn - bDarkpawn);

//         print_debug(debugEval, "wpawnDouble:    %3d\n", wdbl);
//         print_debug(debugEval, "bpawnDouble:    %3d\n", bdbl);
//         print_debug(debugEval, "wpawn backward: %3d\n", wpbw);
//         print_debug(debugEval, "bpawn backward: %3d\n", bpbw);
//         print_debug(debugEval, "wpawn bwo:      %3d\n", wpbwo);
//         print_debug(debugEval, "bpawn bwo:      %3d\n", bpbwo);
//         print_debug(debugEval, "wpawnShoulder:  %3d\n", wps);
//         print_debug(debugEval, "bpawnShoulder:  %3d\n", bps);
//         print_debug(debugEval, "wpawn ciso:     %3d\n", wpic);
//         print_debug(debugEval, "bpawn ciso:     %3d\n", bpic);
//         print_debug(debugEval, "wpawn eiso:     %3d\n", wpie);
//         print_debug(debugEval, "bpawn eiso:     %3d\n", bpie);
//         print_debug(debugEval, "wpawn oiso:     %3d\n", wpio);
//         print_debug(debugEval, "bpawn oiso:     %3d\n", bpio);

        for (unsigned i=0; i<nRows; ++i) {
            pawnEntry.shield2[0][i] = evalShield2<White>(wpawn, i);
//             if (pawnEntry.openFiles[0] & 1<<i)
//                 pawnEntry.shield2[0][i] += kingShield.halfOpenFile;

            pawnEntry.shield2[1][i] = evalShield2<Black>(bpawn, i);
//             if (pawnEntry.openFiles[1] & 1<<i)
//                 pawnEntry.shield2[1][i] += kingShield.halfOpenFile;

            if (pawnEntry.openFiles[0] & pawnEntry.openFiles[1] & 7<<i>>1) {
                pawnEntry.shield2[0][i] += kingShieldOpenFile;
                pawnEntry.shield2[1][i] += kingShieldOpenFile; } }
        pawnEntry.upperKey = k >> PawnEntry::upperShift;
        if (k >> PawnEntry::upperShift)
            pt->store(st, pawnEntry); }
    return pawnEntry; }

template<Colors C, GamePhase P> // TODO use overload for P == Endgame, where attack and defend are not used
inline CompoundScore Eval::mobility( const Board& b, int& attackingPieces, int& defendingPieces) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    CompoundScore score(0,0);
    attackingPieces = 0;
    unsigned nAttackers = 0;
    if (P != Endgame) defendingPieces = 0;
    unsigned nDefenders = 0;
    unsigned nb = 0;
    unsigned nn = 0;
    unsigned nq = 0;
    unsigned nr = 0;

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
    for (const Board::MoveTemplateB* bs = b.bsingle[CI]; bs->move.data; ++bs) {
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

        nb = popcount(bmob1x);
        score = score + bm[nb]; }

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
        nn = popcount(nmob1x);
        score = score + nm[nn];

    }

    restrictions &= ~(b.getAttacks<-C,Bishop>() | b.getAttacks<-C,Knight>());
    __v2di allRookAttacks = zero;
    for (const Board::MoveTemplateR* rs = b.rsingle[CI]; rs->move.data; ++rs) {
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
        nr = popcount(rmob1x);
        score = score + rm[nr]; }

    if (b.getPieces<C,Queen>()) {
        restrictions &= ~(b.getAttacks<-C,Rook>());
        uint64_t qatt1 = b.getAttacks<C,Queen>();
        uint64_t qmob1 = qatt1 & restrictions;
//         uint64_t qmob2 = qmob1 & noBlockedPawns;
        qmob1 &= ~b.getOcc<C>();
        uint64_t qmob1x = qmob1;
//         uint64_t qatt2 = b.build02Attack(qmob1) | b.build13Attack(qmob1);
//         qmob2 |= qatt2 & restrictions & ~b.getOcc<C>();
//        unsigned q=bit(b.getPieces<C,Queen>());
        __v2di connectedBR = ~ pcmpeqq(b.qsingle[CI][0].d13 & _mm_set1_epi64x(b.getPieces<C,Bishop>()), zero)
                             & allBishopAttacks;
        connectedBR |= ~ pcmpeqq(b.qsingle[CI][0].d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero)
                       & allRookAttacks;
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
        // this may be the summed mobility of two queens or wrongly added rook mob
        nq = std::min(popcount(qmob1x), 27);
        score = score + qm[nq]; }

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
    return score; }

template<Colors C>
int Eval::attack2(const Board& b, const PawnEntry& p, int attackingPieces, int defendingPieces) const {
    using namespace SquareIndex;
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
        possibleShield = std::max(shieldH, possibleShield); }

    if (b.cep.castling.color[EI].q) {
        int shieldB = p.shield2[EI][b1] - 2*castlingTempo;
        int shieldA = p.shield2[EI][a1] - 3*castlingTempo;
        possibleShield = std::max(shieldB, possibleShield);
        possibleShield = std::max(shieldA, possibleShield); }

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
int Eval::kingSafety(const Board& b) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    unsigned kpos = bit(b.getPieces<C,King>());
    if (b.getPieces<-C,Bishop>() & !b.bsingle[EI][1].move.data) {
        unsigned pos = bit(b.getPieces<-C,Bishop>());
        unsigned corner = (pos ^ (pos >> 3)) & 1;
        return ksvb[corner][kpos]; }
    return ksv[kpos]; }

template<Colors C>
int Eval::endgame(const Board& b, const PawnEntry& pe, int /*sideToMove*/) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };

    int score = 0;
    unsigned eking = bit(b.getPieces<-C,King>());
    unsigned king = bit(b.getPieces<C,King>());
    // pawn moves, non capture, non promo
    print_debug(debugEval, "King dist pawn %d\n", CI);
    for (uint64_t p = b.getPieces<C,Pawn>() & ~pe.passers[CI]; p; p &= p-1) {
        uint64_t pos = bit(p);
        unsigned rank = (pos ^ (C==Black?070:0)) >> 3;
        ASSERT(rank <= 6);
        ASSERT(rank >= 1);
        score += oppKingOwnPawn[rank][distance[eking][pos]];
        print_debug(debugEval, "(%3d:", oppKingOwnPawn[rank][distance[eking][pos]]);
        score += ownKingOwnPawn[rank][distance[king][pos]];
        print_debug(debugEval, "%3d)", ownKingOwnPawn[rank][distance[king][pos]]); }
    print_debug(debugEval, "\nKing dist passer %d\n", CI);
    /*
     * calculate king distance to promotion square
     */
    for (uint64_t p = b.getPieces<C,Pawn>() & pe.passers[CI]; p; p &= p-1) {
        uint64_t pos = bit(p);
        unsigned rank = (pos ^ (C==Black?070:0)) >> 3;
        ASSERT(rank <= 6);
        ASSERT(rank >= 1);
        score += oppKingOwnPasser[rank][distance[eking][pos]];
        print_debug(debugEval, "(%3d:", oppKingOwnPasser[rank][distance[eking][pos]]);
        score += ownKingOwnPasser[rank][distance[king][pos]];
        print_debug(debugEval, "%3d)", ownKingOwnPasser[rank][distance[king][pos]]); }
    print_debug(debugEval, "%d\n", 0);
    return score; }

template<GamePhase P>
CompoundScore Eval::mobilityDiff(const Board& b, int& wap, int& bap, int& wdp, int& bdp) const {
    return mobility<White, P>(b, wap, wdp) - mobility<Black, P>(b, bap, bdp); }

int Eval::attackDiff(const Board& b, const PawnEntry& pe, int wap, int bap, int wdp, int bdp) const {
    return attack2<White>(b, pe, wap, bdp) - attack2<Black>(b, pe, bap, wdp);
//         return attack<White>(b, pe, wap, bdp) - attack<Black>(b, pe, bap, wdp);
}

int Eval::operator () (const Board& b, Colors stm ) const {
    int wap, bap;
    return operator()(b, stm, wap, bap); }

int Eval::operator () (const Board& b, Colors stm, int& wap, int& bap ) const {
#if defined(MYDEBUG)
    int cmp = calc(b.matIndex, CompoundScore(b.keyScore.score));
    CompoundScore value( 0, 0 );
    for (int p=Rook; p<=King; ++p) {
        for (uint64_t x=b.getPieces<White>(p); x; x&=x-1) {
            uint64_t sq=bit(x);
            print_debug(debugEval, "materialw%d     %d\n", p, calc(b.matIndex, CompoundScore( getPS( p, sq))));
            value = value + getPS( p, sq); }
        for (uint64_t x=b.getPieces<Black>(p); x; x&=x-1) {
            uint64_t sq=bit(x);
            print_debug(debugEval, "materialb%d     %d\n", p, calc(b.matIndex, CompoundScore( getPS(-p, sq))));
            value = value + getPS(-p, sq); } }
    int v = calc(b.matIndex, value);
    if (v != cmp) asm("int3");
#endif
    if (b.getPieces<White,Pawn>() + b.getPieces<Black,Pawn>()) {
        PawnEntry pe = pawns(b);

        int wdp, bdp;
        CompoundScore mob = mobilityDiff<Opening>(b, wap, bap, wdp, bdp);
        CompoundScore attend ( material[b.matIndex].opening ? attackDiff(b, pe, wap, bap, wdp, bdp) : 0,
                               material[b.matIndex].opening != endgameTransitionSlope ? endgame<White>(b, pe, stm) - endgame<Black>(b, pe, stm) : 0);

        CompoundScore pawn( pe.score );
        CompoundScore piece = pieces<White>(b, pe) - pieces<Black>(b, pe);

//         print_debug(debugEval, "endgame:        %d\n", e);
//         print_debug(debugEval, "mobility:       %d\n", m);
//         print_debug(debugEval, "pawns:          %d\n", pa);
//         print_debug(debugEval, "attack:         %d\n", a);
//         print_debug(debugEval, "pieces:         %d\n", pi);

        CompoundScore score = mob + attend + pawn + piece;
        int o = material[b.matIndex].opening;
        int e = endgameTransitionSlope - material[b.matIndex].opening;
        int s = o*score.opening() + e*score.endgame();
        return s >> logEndgameTransitionSlope; }
    else {       // pawnless endgame
        int mat = b.keyScore.score.endgame;  //TODO use a simpler discriminator
        int wap, bap, wdp, bdp; //TODO not needed here
        int m = mobilityDiff<Endgame>(b, wap, bap, wdp, bdp).endgame();
        int p = (mat>0 ? 1:4)*kingSafety<White>(b) - (mat<0 ? 1:4)*kingSafety<Black>(b);
        return m + p; } }

int Eval::operator () (const Board& b, Colors stm, int& wap, int& bap, int psValue, int& posScore ) const {
    int realScore;
    switch(material[b.matIndex].recognized) {
    case KBPk:
        realScore = psValue >> evalKBPk<White>(b);
        posScore = realScore - psValue;
        return realScore;
    case kpbK:
        realScore = psValue >> evalKBPk<Black>(b);
        posScore = realScore - psValue;
        return realScore;
    case KB_kb_:
        realScore = psValue >> evalKB_kb_<Black>(b);
        posScore = realScore - psValue;
        return realScore;
    case KPk:
        realScore = (psValue<<2) >> evalKPk<White>(b, stm);
        posScore = realScore - psValue;
        return realScore;
    case kpK:
        realScore = (psValue<<2) >> evalKPk<Black>(b, stm);
        posScore = realScore - psValue;
        return realScore;

    case Unspecified:
    default:
        posScore = operator()(b, stm, wap, bap);
        return psValue + posScore; } }

void Eval::ptClear() {
    pt->clear(); }

int Eval::calc(unsigned matIndex, CompoundScore score) const {
    int o = material[matIndex].opening;
    int e = endgameTransitionSlope - material[matIndex].opening;
    int s = o*score.opening() + e*score.endgame();

    return ((s >> logEndgameTransitionSlope) + material[matIndex].bias) >> material[matIndex].drawish; }

template<Colors C>
unsigned Eval::evalKBPk(const Board& b) const {
    if (((b.getPieces<C,Pawn>() & file<'a'>())
            && (b.getPieces<C,Bishop>() & (C==White ? darkSquares : ~darkSquares)))) {
        uint64_t front = b.getPieces<C,Pawn>();
        front |= front << 1;
        front |= shift<C* 010>(front);
        front |= shift<C* 020>(front);
        front |= shift<C* 040>(front);
        if (b.getPieces<-C,King>() & front) return 3; }
    else if (((b.getPieces<C,Pawn>() & file<'h'>())
              && (b.getPieces<C,Bishop>() & (C==White ? ~darkSquares : darkSquares)))) {
        uint64_t front = b.getPieces<C,Pawn>();
        front |= front >> 1;
        front |= shift<C* 010>(front);
        front |= shift<C* 020>(front);
        front |= shift<C* 040>(front);
        if (b.getPieces<-C,King>() & front) return 3;

    }

    return 0; }
/*
 * Evaluate KPk. Returns a shift right value, which is applied to a pre-
 * shifted by 2 left value. A return value of 2 changes nothing.
 */
template<Colors C>
unsigned Eval::evalKPk(const Board& b, Colors stm) const {
    uint64_t bk = b.getPieces<-C,King>();
    uint64_t wp = b.getPieces<C,Pawn>();
    // Underpromotions may cause a position with no pawns mapped to this
    if (!wp) return 2;
    uint64_t wpPos = bit(wp);
    if (bk & ruleOfSquare[C==Black][wpPos]) {
        if (b.getPieces<C,King>() & keySquare[C==Black][wpPos] && stm==Black
                && !(wp & b.getAttacks<-C,King>()))
            return 0;
        if (b.getPieces<-C,King>() & keySquare[C==Black][wpPos] && stm==White)
            return 4; }
    else {
        if (stm==White)
            return 0; }
    return 2; }

template<Colors C>
unsigned Eval::evalKB_kb_(const Board& b) const {
    if (!(b.getPieces<C,Bishop>() & darkSquares) ^ !(b.getPieces<-C,Bishop>() & darkSquares)) {
        return 1; }
    return 0; }
