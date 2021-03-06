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

unsigned Eval::distance[nSquares][nSquares];
uint64_t Eval::ruleOfSquare[nColors][nSquares];
uint64_t Eval::keySquare[nColors][nSquares];

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

#if defined(MYDEBUG)
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
#else
#define printBit(...)
#endif
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
CompoundScore Eval::pieces(const Board& b, const PawnEntry& p, uint64_t own, uint64_t opp, uint64_t ownpasser, uint64_t opppasser) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    CompoundScore value(0,0);

    if (uint64_t r = b.getPieces<C,Rook>()) {
		if  (   (  r & (file<'h'>()|file<'g'>()) & (rank<C,1>()|rank<C,2>())
				   && b.getPieces<C, Pawn>() & file<'h'>() & (rank<C,2>() | rank<C,3>())
				   &&  (   b.getPieces<C, King>() & file<'g'>() & rank<C,1>()
						   || (b.getPieces<C, King>() & file<'f'>() & rank<C,1>() && b.getPieces<C, Pawn>() & file<'g'>() & (rank<C,2>() | rank<C,3>()))))
				||  (  r & (file<'a'>()|file<'b'>()) & (rank<C,1>()|rank<C,2>())
					   && b.getPieces<C, Pawn>() & file<'a'>() & (rank<C,2>() | rank<C,3>())
					   &&  (   b.getPieces<C, King>() & file<'b'>() & rank<C,1>()
							   || (b.getPieces<C, King>() & file<'c'>() & rank<C,1>() && b.getPieces<C, Pawn>() & file<'b'>() & (rank<C,2>() | rank<C,3>())))))
			value = value + rookTrapped;

		uint64_t weak = p.weak[EI];
		weak += weak << 010;
		weak += weak << 020;
		weak += weak << 040;

		if ((b.getPieces<-C,Pawn>() | shift<-C*010>(b.getPieces<-C,King>())) & rank<C,7>())
		    value = value + rookSeventh[popcount(r & rank<C,7>())];
        print_debug(debugEval, "after rook7: %4d %4d\n", value.opening(), value.endgame());
		value = value + rookHalfOpen [popcount(own & ~opp)>>3] [popcount3((r & own & ~opp))];
        print_debug(debugEval, "after rookHO: %4d %4d\n", value.opening(), value.endgame());
		value = value + rookOpen     [popcount(own &  opp)>>3] [popcount3((r & own &  opp))];
        print_debug(debugEval, "after rookO: %4d %4d\n", value.opening(), value.endgame());
		value = value + rookWeakPawn[ popcount3((r & weak)) ];
        print_debug(debugEval, "after rookWP: %4d %4d\n", value.opening(), value.endgame());
		value = value + rookOwnPasser[ popcount3((r & ownpasser )) ];
        print_debug(debugEval, "after rookOWNP: %4d %4d\n", value.opening(), value.endgame());
		value = value + rookOppPasser[ popcount3((r & opppasser )) ];
		print_debug(debugEval, "after rookOPPP: %4d %4d\n", value.opening(), value.endgame());
    }

    value = value + knightBlockPasser [ popcount3((b.getPieces<C,Knight>() & shift<C* 8>(p.passers[EI]))) ];
    print_debug(debugEval, "after knight: %4d %4d\n", value.opening(), value.endgame());
    if unlikely((shift<-C*8>(b.getPieces<C,Knight>()) & b.getPieces<-C,Pawn>())) {
        if unlikely((b.getPieces<C,Knight>()
            & ((rank<C,7>() & file<'b'>()) | (rank<C,7>() & file<'a'>()))
            & shift<-1>(b.getPieces<-C,Pawn>())))
            value = value + knightTrapped;
        if unlikely((b.getPieces<C,Knight>()
            & ((rank<C,7>() & file<'g'>()) | (rank<C,7>() & file<'h'>()))
            & shift<1>(b.getPieces<-C,Pawn>())))
            value = value + knightTrapped;
    }
    if (uint64_t bishop = b.getPieces<C,Bishop>()) {
        value = value + bishopBlockPasser [ popcount3(bishop & shift<C* 8>(p.passers[EI])) ];

        uint64_t dark = bishop & darkSquares;
        uint64_t light = bishop & ~darkSquares;
        unsigned darkp = p.dark[CI];
        unsigned lightp = p.light[CI];
        value = value + bishopOwnPawn[ (dark ? darkp:0) + (light ? lightp:0) ];
        value = value + bishopNotOwnPawn[ (light ? darkp:0) + (dark ? lightp:0) ];

        unsigned ddef = p.darkDef[EI];
        unsigned ldef = p.lightDef[EI];

        value = value + bishopOppPawn[ (dark ? ddef:0) + (light ? ldef:0) ];
        value = value + bishopNotOppPawn[ (light ? ddef:0)+ (dark ? ldef:0) ];
        print_debug(debugEval, "after bishop: %4d %4d\n", value.opening(), value.endgame());

        if unlikely((shift<-C*8+1>(bishop) & b.getPieces<-C,Pawn>() & ((rank<C,6>() & file<'b'>()) | (rank<C,7>() & file<'c'>())))) {
            value = value + bishopTrapped;
        }
        if unlikely((shift<-C*8-1>(bishop) & b.getPieces<-C,Pawn>() & ((rank<C,6>() & file<'g'>()) | (rank<C,7>() & file<'f'>())))) {
            value = value + bishopTrapped;
        }
    }

    if ( b.getPieces<C,Bishop>() & b.pins[CI] )
        value = value + bishopPin;
    if ( b.getPieces<C,Knight>() & b.pins[CI] )
        value = value + knightPin;
    if ( b.getPieces<C,Rook>() & b.pins[CI] )
        value = value + rookPin;
    if ( b.getPieces<C,Queen>() & b.pins[CI] )
        value = value + queenPin;
    return value; }

template<Colors C>
int Eval::evalShield2(uint64_t pawns, unsigned file) const {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };

    constexpr int bitrank2 = 28-20*C;
    constexpr int bitrank3 = 28-12*C;
    constexpr int bitrank4 = 28- 4*C;

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
    PawnKey k=b.keyScore.pawnKey();
    Sub<PawnEntry, 4>* st = pt->getSubTable(k);
    PawnEntry pawnEntry;
    if (pt->retrieve(st, b, pawnEntry))
        STATS(pthit);
    else {
        STATS(ptmiss);
        uint64_t wpawn = b.getPieces<White,Pawn>();
        uint64_t bpawn = b.getPieces<Black,Pawn>();

        CompoundScore score = pawnShoulder2[ popcount15(wpawn & wpawn<<1 & ~file<'a'>()) ];
        score = score - pawnShoulder2[ popcount15(bpawn & bpawn<<1 & ~file<'a'>()) ];
        print_debug(debugEval, "After shoulder    %3d %3d\n", score.opening(), score.endgame());

        // FEATURE the whole bit shift stuff can be done much fater using pshufb
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
        print_debug(debugEval, "After double     %3d %3d\n", score.opening(), score.endgame());

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
        print_debug(debugEval, "After bward       %3d %3d\n", score.opening(), score.endgame());

        // backward pawns on open files, reduce to (uint8_t) for rooks later
        uint64_t wBackOpen = wBackward & ~bFront & ~wBack;
        uint64_t bBackOpen = bBackward & ~wFront & ~bBack;
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
        score = score + pawnBackwardOpen2[popcount15(wBackOpen)];
        score = score - pawnBackwardOpen2[popcount15(bBackOpen)];
        print_debug(debugEval, "After bwardopen   %3d %3d\n", score.opening(), score.endgame());

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
            print_debug(debugEval, "After wpasser   %3d %3d\n", score.opening(), score.endgame());
            if (wPassedConnected & (1LL << pos)) score = score + pawnConnPasser22[y-1];

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

        print_debug(debugEval, "After wcandidates %3d %3d\n", score.opening(), score.endgame());

        pawnEntry.passers[1] = bPassed;
        for ( i=0; bPassed; bPassed &= bPassed-1, i++ ) {
            uint64_t pos = bit(bPassed);
            uint64_t y = pos  >> 3;
            ASSERT(y>0 && y<7);
            score = score - pawnPasser22[6-y];
            print_debug(debugEval, "After bpasser  %3d %3d\n", score.opening(), score.endgame());
            if (bPassedConnected & (1LL << pos)) score = score - pawnConnPasser22[6-y];

//             print_debug(debugEval, "pawn bpasser:   %d\n", pawnPasser22[6-y]);
        }
        for (; bCandidate; bCandidate &= bCandidate-1)
            score = score - pawnCandidate[6-(bit(bCandidate)>>3)];
        print_debug(debugEval, "After bcandidates %3d %3d\n", score.opening(), score.endgame());

        // rook pawns are always adjacent to a "open" file
        uint64_t wOpenFiles = ~(wFront | wpawn | wBack);
        uint64_t bOpenFiles = ~(bFront | bpawn | bBack);
        pawnEntry.openFiles[0] = (uint8_t)wOpenFiles;
        pawnEntry.openFiles[1] = (uint8_t)bOpenFiles;

        uint64_t wIsolani = wpawn & (wOpenFiles<<1 | 0x101010101010101LL) & (wOpenFiles>>1 | 0x8080808080808080LL);
        uint64_t bIsolani = bpawn & (bOpenFiles<<1 | 0x101010101010101LL) & (bOpenFiles>>1 | 0x8080808080808080LL);
        score = score + pawnIsolatedCenter2[ (popcount15(wIsolani & ~(file<'a'>()|file<'h'>()))) ];
        score = score - pawnIsolatedCenter2[ (popcount15(bIsolani & ~(file<'a'>()|file<'h'>()))) ];
        print_debug(debugEval, "After isocenter %3d %3d\n", score.opening(), score.endgame());
        score = score + pawnIsolatedEdge2[ (popcount15(wIsolani & (file<'a'>()|file<'h'>()))) ];
        score = score - pawnIsolatedEdge2[ (popcount15(bIsolani & (file<'a'>()|file<'h'>()))) ];
        print_debug(debugEval, "After isoedge %3d %3d\n", score.opening(), score.endgame());
        score = score + pawnIsolatedOpen2[ (popcount15(wIsolani & ~bFront)) ];
        score = score - pawnIsolatedOpen2[ (popcount15(bIsolani & ~wFront)) ];
        print_debug(debugEval, "After isoopen %3d %3d\n", score.opening(), score.endgame());
        pawnEntry.score = score.packed();

        uint64_t wDarkpawn = wpawn & darkSquares;
        uint64_t bDarkpawn = bpawn & darkSquares;
        pawnEntry.dark[0] = popcount(wDarkpawn);
        pawnEntry.dark[1] = popcount(bDarkpawn);
        pawnEntry.light[0] = popcount(wpawn - wDarkpawn);
        pawnEntry.light[1] = popcount(bpawn - bDarkpawn);

        uint64_t wDef = wpawn & b.getAttacks<White,Pawn>();
        uint64_t bDef = bpawn & b.getAttacks<Black,Pawn>();
        pawnEntry.darkDef[0] = popcount(wDef & darkSquares);
        pawnEntry.darkDef[1] = popcount(bDef & darkSquares);
        pawnEntry.lightDef[0] = popcount(wDef & ~darkSquares);
        pawnEntry.lightDef[1] = popcount(bDef & ~darkSquares);

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
#ifdef __SSE4_1__
        pawnEntry.pawns = b.get2Pieces<Pawn>();
#else
        pawnEntry.pawns[0] = b.getPieces<White,Pawn>();
        pawnEntry.pawns[1] = b.getPieces<Black,Pawn>();
#endif
        pt->store(st, pawnEntry); }
    return pawnEntry; }

template<Colors C, GamePhase P>
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

    unsigned king = bit(b.getPieces<-C,King>());
    const uint64_t oppking = b.kAttacked[king];// b.getAttacks<-C,King>();
    const uint64_t ownking = b.getAttacks< C,King>()/* | shift<C* 8>(b.getAttacks< C,King>())*/;

    uint64_t restrictions = ~b.getAttacks<-C,Pawn>() & ~(b.getAttacks<-C,All>());
    for (uint64_t p = b.getPieces<C, Knight>(); p; p &= p-1) {
        uint64_t sq = bit(p);
        uint64_t natt = b.knightAttacks[sq];
        uint64_t nmob =  natt & restrictions & ~b.getOcc<C>();
        if (P != Endgame) {
            uint64_t a = natt & oppking;
            uint64_t d = natt & ownking;
            attackingPieces += attackN[popcount15(a)];
            nAttackers += !!a;
            defendingPieces += defenseN[popcount15(d)];
            nDefenders += !!d;
            print_debug(debugEval, "nAtt index: %d, nAtt value: %d\n", popcount15(a), attackN[popcount15(a)]);
            print_debug(debugEval, "nDef index: %d, nDef value: %d\n", popcount15(d), defenseN[popcount15(d)]);
        }
        nn = popcount(nmob);
        score = score + nm[nn];
        print_debug(debugEval, "nMob %4d%4d\n", nm[nn].opening, nm[nn].endgame);

    }
    if (b.getPieces<C,Queen>()) {
        const __v2di v2queen = _mm_set1_epi64x(b.getPieces<C,Queen>());
		__v2di allBishopAttacks = zero;
		for (const Board::MoveTemplateB* bs = b.bsingle[CI]; bs->move.data; ++bs) {
			allBishopAttacks |= bs->d13;
			uint64_t batt1 = fold(bs->d13 | ~pcmpeqq(bs->d13 & v2queen, zero) & b.qsingle[CI][0].d13);
            /*
            * If the queen is on an attack line of the bishop, include the attack
            * line of the queen in the attack line of the bishop. This leads to
            * increased one move mobility and the bishop gets counted in an possible
            * king attack
            */
            // restriced mobility in one move
			uint64_t bmob1 = batt1 & restrictions & ~b.getOcc<C>();
			if (P != Endgame) {
				uint64_t a = batt1 & oppking;
				uint64_t d = batt1 & ownking;
				attackingPieces += attackB[popcount15(a)];
				nAttackers += !!a;
				defendingPieces += defenseB[popcount15(d)];
				nDefenders += !!d;
				print_debug(debugEval, "bAtt index: %d, bAtt value: %d\n", popcount15(a), attackB[popcount15(a)]);
				print_debug(debugEval, "bDef index: %d, bDef value: %d\n", popcount15(d), defenseB[popcount15(d)]);
			}

			nb = popcount(bmob1);
            print_debug(debugEval, "bMob %4d%4d\n", bm[nb].opening, bm[nb].endgame);
			score = score + bm[nb]; }

		restrictions &= ~(b.getAttacks<-C,Bishop>() | b.getAttacks<-C,Knight>());
		__v2di allRookAttacks = zero;
		for (const Board::MoveTemplateR* rs = b.rsingle[CI]; rs->move.data; ++rs) {
			// restriced mobility in one move, including own defended pieces
			allRookAttacks |= rs->d02;
            __v2di connectedQR = ~pcmpeqq(rs->d02 & v2queen, zero) & b.qsingle[CI][0].d02;
            if (b.rsingle[CI][1].move.data)
                connectedQR |= ~pcmpeqq(rs->d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero) & (b.rsingle[CI][0].d02 | b.rsingle[CI][1].d02);
            uint64_t ratt = fold(rs->d02 | connectedQR);
			uint64_t rmob1 = ratt & restrictions & ~b.getOcc<C>();;
			if (P != Endgame) {
				uint64_t a = ratt & oppking;
				uint64_t d = ratt & ownking;
				attackingPieces += attackR[popcount15(a)];
				nAttackers += !!a;
				defendingPieces += defenseR[popcount15(d)];
				nDefenders += !!d;
				print_debug(debugEval, "rAtt index: %d, rAtt value: %d\n", popcount15(a), attackR[popcount15(a)]);
				print_debug(debugEval, "rDef index: %d, rDef value: %d\n", popcount15(d), defenseR[popcount15(d)]);
			}
			nr = popcount(rmob1);
            print_debug(debugEval, "rMob %4d%4d\n", rm[nr].opening, rm[nr].endgame);
			score = score + rm[nr]; }

		restrictions &= ~b.getAttacks<-C,Rook>();
		__v2di connectedBR = ~pcmpeqq(b.qsingle[CI][0].d13 & _mm_set1_epi64x(b.getPieces<C,Bishop>()), zero)
							 & allBishopAttacks;
		connectedBR |= ~ pcmpeqq(b.qsingle[CI][0].d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero)
					   & allRookAttacks;
        uint64_t qatt1 = b.getAttacks<C,Queen>() | fold(connectedBR);
        uint64_t qmob1 = qatt1 & restrictions & ~b.getOcc<C>();
		if (P != Endgame) {
			uint64_t a = qatt1 & oppking;
			uint64_t d = qatt1 & ownking;
			attackingPieces += attackQ3[popcount15(a)];
			nAttackers += !!a;
			defendingPieces += defenseQ[popcount15(d)];
			nDefenders += !!d;
			print_debug(debugEval, "qAtt index: %d, qAtt value: %d\n", popcount15(a), attackQ3[popcount15(a)]);
			print_debug(debugEval, "qDef index: %d, qDef value: %d\n", popcount15(d), defenseQ[popcount15(d)]);
		}
		// this may be the summed mobility of two queens or wrongly added rook mob
		nq = std::min(popcount(qmob1), 27);
        print_debug(debugEval, "qMob %4d%4d\n", qm[nq].opening, qm[nq].endgame);

		score = score + qm[nq]; }
    else {
		for (const Board::MoveTemplateB* bs = b.bsingle[CI]; bs->move.data; ++bs) {
			uint64_t batt1 = fold(bs->d13);
			uint64_t bmob1 = batt1 & restrictions & ~b.getOcc<C>();
			if (P != Endgame) {
				uint64_t a = batt1 & oppking;
				uint64_t d = batt1 & ownking;
				attackingPieces += attackB[popcount15(a)];
				nAttackers += !!a;
				defendingPieces += defenseB[popcount15(d)];
				nDefenders += !!d;
				print_debug(debugEval, "bAtt index: %d, bAtt value: %d\n", popcount15(a), attackB[popcount15(a)]);
				print_debug(debugEval, "bDef index: %d, bDef value: %d\n", popcount15(d), defenseB[popcount15(d)]);
			}

			nb = popcount(bmob1);
			score = score + bm[nb]; }

		restrictions &= ~(b.getAttacks<-C,Bishop>() | b.getAttacks<-C,Knight>());
		for (const Board::MoveTemplateR* rs = b.rsingle[CI]; rs->move.data; ++rs) {
			__v2di vratt = rs->d02;
			if (b.rsingle[CI][1].move.data)
			const __v2di v2rook = { b.getPieces<C,Rook>(), b.getPieces<C,Rook>() };
			vratt |= ~pcmpeqq(rs->d02 & _mm_set1_epi64x(b.getPieces<C,Rook>()), zero) & (b.rsingle[CI][0].d02 | b.rsingle[CI][1].d02);
            uint64_t ratt1 = fold(vratt);
            uint64_t rmob1 = ratt1 & restrictions & ~b.getOcc<C>();
			if (P != Endgame) {
				uint64_t a = ratt1 & oppking;
				uint64_t d = ratt1 & ownking;
				attackingPieces += attackR[popcount15(a)];
				nAttackers += !!a;
				defendingPieces += defenseR[popcount15(d)];
				nDefenders += !!d;
				print_debug(debugEval, "rAtt index: %d, rAtt value: %d\n", popcount15(a), attackR[popcount15(a)]);
				print_debug(debugEval, "rDef index: %d, rDef value: %d\n", popcount15(d), defenseR[popcount15(d)]);
			}
			nr = popcount(rmob1);
			score = score + rm[nr]; } }

    uint64_t a = b.getAttacks<C,Pawn>() & oppking;
    attackingPieces += attackP2[popcount15(a)];
    nAttackers += !!a;
    print_debug(debugEval, "pAtt index: %d, pAtt value: %d\n", popcount15(a), attackP2[popcount15(a)]);
    a = b.getAttacks<C,King>() & oppking;
    attackingPieces += attackK2[popcount15(a)];
    nAttackers += !!a;
    print_debug(debugEval, "kAtt index: %d, kAtt value: %d\n", popcount15(a), attackK2[popcount15(a)]);

    attackingPieces = (attackingPieces * (0x100 - (0xB0>>nAttackers))) >> 8;
//         attackingPieces = (nAttackersTab[nAttackers] * attackingPieces) >> 8;
//         if (P != Endgame) {
//             attackingPieces += attackP[popcount15(b.getAttacks<C,Pawn>() & oppking)];
//             attackingPieces += attackK[popcount15(b.getAttacks<C,King>() & oppking)];
//         }
    return score; }

/*
 * King attack and defense. Uses attackingPieces and defendingPieces from
 * mobility and a pawn shield before the king or the shields at the castling
 * positions. It uses a sigmoid centered around 0, so that for a strong defense
 * a slight attack or the other way around has a weaker effect, while for
 * average positions a change in attack/defense has a larger score difference
 * as result. The score returned may be negative, this can be interpreted as
 * a positive score for the opponents defense.
 */
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
    print_debug(debugEval, "attack%d: %6d/%6d\n", CI, attackingPieces*pieceAttack, maxAttack);
    print_debug(debugEval, "defense%d: %6d/%6d\n", EI, defendingPieces*pieceDefense + possibleShield*pawnDefense, maxDefense);

    int attack = (attackingPieces*pieceAttack + maxAttack) * (2*maxDefense - defendingPieces*pieceDefense - possibleShield*pawnDefense) - maxAttack*maxDefense;
    print_debug(debugEval, "raw attack%d: %6d\n", EI, attack);
    attack = attack / (128*(maxAttack+maxDefense)) + sizeof attackTable2/sizeof(int)/2;
    ASSERT(attack < sizeof attackTable2/sizeof(int));

    print_debug(debugEval, "attack index%d: %3d\n", CI, attack);
    print_debug(debugEval, "attack value%d: %3d\n", CI, attackTable2[attack]);

    return attackTable2[attack]; }

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
     * calculate king distance to passer
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
    int cmp;
    if (stm == White)
        cmp = calc<White>((const ColoredBoard<White>&)b, b.matIndex, b.keyScore.score());
    else
        cmp = calc<Black>((const ColoredBoard<Black>&)b, b.matIndex, b.keyScore.score());
    CompoundScore value( 0, 0 );
    for (int p=Rook; p<=King; ++p) {
        for (uint64_t x=b.getPieces<White>(p); x; x&=x-1) {
            unsigned sq=bit(x);
            print_debug(debugEval, "materialw%d %c%d    %4d%4d\n", p, (sq&7)+'a', sq/8+1, keyScore( p, sq).opening(), keyScore( p, sq).endgame());
            value = value + keyScore( p, sq).score(); }
        for (uint64_t x=b.getPieces<Black>(p); x; x&=x-1) {
        	unsigned sq=bit(x);
            print_debug(debugEval, "materialb%d %c%d    %4d%4d\n", p, (sq&7)+'a', sq/8+1, keyScore(-p, sq).opening(), keyScore(-p, sq).endgame());
            value = value + keyScore(-p, sq).score(); } }
    int v;
    if (stm == White)
        v = calc<White>((const ColoredBoard<White>&)b, b.matIndex, value);
    else
        v = calc<Black>((const ColoredBoard<Black>&)b, b.matIndex, value);
    ASSERT (v == cmp) ;
#endif
    if (b.getPieces<White,Pawn>() + b.getPieces<Black,Pawn>()) {
        PawnEntry pe = pawns(b);

        int wdp, bdp;
        CompoundScore mob = mobilityDiff<Opening>(b, wap, bap, wdp, bdp);
        CompoundScore attend ( material[b.matIndex].scale.opening ? attackDiff(b, pe, wap, bap, wdp, bdp) : 0,
                               material[b.matIndex].scale.endgame ? endgame<White>(b, pe, stm) - endgame<Black>(b, pe, stm) : 0);

        CompoundScore pawn( pe.score );

#ifdef __SSE4_1__
        __v2di ofiles = _mm_loadu_si128((__m128i*)&pe.openFiles);
        ofiles = _mm_shuffle_epi8(ofiles, _mm_set_epi8(1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0));
        uint64_t wfile = _mm_extract_epi64(ofiles, 0);
        uint64_t bfile = _mm_extract_epi64(ofiles, 1);
#else
        uint64_t wfile = pe.openFiles[0]; //TODO use lookuptable (2k) instead
        wfile += wfile << 010;
        wfile += wfile << 020;
        wfile += wfile << 040;

        uint64_t bfile = pe.openFiles[1];
        bfile += bfile << 010;
        bfile += bfile << 020;
        bfile += bfile << 040;
#endif
        uint64_t wpasser = pe.passers[0];
        wpasser += wpasser >> 010;
        wpasser += wpasser >> 020;
        wpasser += wpasser >> 040;

        uint64_t bpasser = pe.passers[1];
        bpasser += bpasser << 010;
        bpasser += bpasser << 020;
        bpasser += bpasser << 040;

        CompoundScore piece = pieces<White>(b, pe, wfile, bfile, wpasser, bpasser) - pieces<Black>(b, pe, bfile, wfile, bpasser, wpasser);

//         print_debug(debugEval, "endgame:        %d\n", e);
//         print_debug(debugEval, "mobility:       %d\n", m);
//         print_debug(debugEval, "pawns:          %d\n", pa);
//         print_debug(debugEval, "attack:         %d\n", a);
//         print_debug(debugEval, "pieces:         %d\n", pi);

        CompoundScore score = mob + attend + pawn + piece;
        int s = interpolate(material[b.matIndex].scale, score);
        print_debug(debugEval, "piece:       %4d %4d\n", piece.opening(), piece.endgame());
        print_debug(debugEval, "attack/endg: %4d %4d\n", attend.opening(), attend.endgame());
        print_debug(debugEval, "mobility:    %4d %4d\n", mob.opening(), mob.endgame());
        print_debug(debugEval, "pawn:        %4d %4d\n", pawn.opening(), pawn.endgame());
        print_debug(debugEval, "mat.bias:    %4d\n", material[b.matIndex].bias);
        print_debug(debugEval, "mat.opening: %6.1f\n", material[b.matIndex].scale.opening/32767.0);
        print_debug(debugEval, "mat.endgame: %6.1f\n", material[b.matIndex].scale.endgame/32767.0);
        print_debug(debugEval, "posScore:    %4d\n", s);
#ifdef MYDEBUG
        print_debug(debugEval, "PSScore:     %4d\n", v);
#endif
        return s; }
    else {       // pawnless endgame
        int mat = b.keyScore.endgame();  //TODO use a simpler discriminator
        int wap, bap, wdp, bdp; //TODO not needed here
        int m = mobilityDiff<Endgame>(b, wap, bap, wdp, bdp).endgame();
        int p = (mat>0 ? 1:4)*kingSafety<White>(b) - (mat<0 ? 1:4)*kingSafety<Black>(b);
        return m + p; } }

void Eval::ptClear() {
    pt->clear(); }

/*
 * Interpolate between score.opening and score.endgame according to weights
 */
int Eval::interpolate(CompoundScore weights, CompoundScore score) const {
#ifdef __SSSE3__
	// The SSE version loses half a bit of precision, because is rounds first
	// and then sums up, where the normal code rounds last.
    __v8hi score16 = _mm_mulhrs_epi16(weights.data, score.data);
    int16_t s0 = _mm_extract_epi16( score16, 0 ) + _mm_extract_epi16( score16, 1 );
    return s0;
#else
	int o = weights.opening();
    int e = weights.endgame();
    int s = (o*score.opening() + e*score.endgame() + 0x4000) >> 15;
    return s;
#endif
	}

int Eval::calcPS(CompoundScore weights, int bias, unsigned drawish, CompoundScore score) const {
    return (interpolate(weights, score) + bias) >> drawish; }

int Eval::quantize(int value) const {
    if (value > 0)
        value += quantRound;
    if (value < 0)
        value += quantRoundNeg;
    value &= quantMask;
    return value;
}
