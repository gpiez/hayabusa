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
#ifndef EVAL_H_
#define EVAL_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"
#include "score.h"
#include "ttentry.h"
#include "transpositiontable.h"
/*
 * A square attacked by a piece is higher evaluated if it inhibits moves from
 * enemy pieces to this square. For this to happen it must be either less
 * valuable than the inhibited piece or the SEE after placing the inhibited
 * piece on the square must be positive (this is both acuatally covered by the last
 * condition)
 * The inhibitor bonus depends on the number and kind of inhibited pieces, which is
 * a function of the attackTable and the distance to the enemy pieces
 *
 * v = (sum over pieces (v_inhPieces * n_inhPieces) + n_totalInhPieces^2 * v_totalInh) * squareValue
 * squareValue = attack_king(dist_king) + attack_piece(dist_piece) + attack_pawn(dist_pawn) + const
 */

struct CompoundScore {
    RawScore    opening;
    RawScore    endgame;

    CompoundScore operator + (const CompoundScore& x) const {
        CompoundScore temp;
        temp.opening = x.opening + opening;
        temp.endgame = x.endgame + endgame;
        return temp;
    }
    CompoundScore operator - () const {
        CompoundScore temp;
        temp.opening = -opening;
        temp.endgame = -endgame;
        return temp;
    }
    void operator /= (const int x) {
        opening /= x;
        endgame /= x;
    }
    int calc(unsigned /*material*/) const {
        return opening;
        //        return material >= endgameMaterial ? opening : endgame;
//        int compound = *(int*)this;
//        return material >= endgameMaterial ? (int16_t)compound : compound >> 16;
    }
    void operator = (int x) {
        opening = x;
        endgame = x;
    }
};

union KeyScore {
    struct {
        CompoundScore  score;
        PawnKey        pawnKey;
        Key            key;
    };
    __v8hi vector;
};

template<typename T>
void sigmoid(T& p, double start, double end, double dcenter = 0, double width = 1.5986 );

class PieceList;
class BoardBase;
class Eval {
    KeyScore zobristPieceSquare[nTotalPieces][nSquares];
    TranspositionTable<PawnEntry, 4, PawnKey>* pt;

    int pawn, knight, bishop, rook, queen;
    
    int bishopPair;
    int bishopBlockPasser;
    int bishopAlone;
    
    int knightAlone;
    int knightBlockPasser;

    int rookTrapped;
    int rookOpen;
    int rookHalfOpen;

    int pawnBackward;
    int pawnBackwardOpen;
    int pawnIsolatedCenter;
    int pawnIsolatedEdge;
    int pawnIsolatedOpen;
//    int pawnHalfIsolated;
    int pawnPasser[8];
    int pawnEdge;
    int pawnCenter;
    int pawnDouble;
    int pawnShoulder;
    int pawnHole;
    int pawnConnPasser;
    int pawnUnstoppable;
    
    int attack1b1;
    int attack2b1;
    int attack1b2;
    int attack2b2;
    int attack1n1;
    int attack2n1;
    int attack1n2;
    int attack2n2;
    int attack1r1;
    int attack2r1;
    int attack1r2;
    int attack2r2;
    int attack1q1;
    int attack2q1;
    int attack1q2;
    int attack2q2;
    int attack1p1;
    int attack2p1;
    int attack1k1;
    int attack2k1;
    
    int endgameMaterial;
    
    void initPS();
    void initZobrist();
    static void initTables();
    unsigned int attackHash( unsigned int  pos );
    int squareControl();
    template<Colors C> int mobility(const BoardBase&, unsigned& attackingPieces, unsigned& defendingPieces) const;
    template<Colors C> int attack(const BoardBase& b, const PawnEntry& p, unsigned attackingPieces, unsigned defendingPieces
) const;
    template<Colors C> int pieces(const BoardBase&, const PawnEntry&) const;
    PawnEntry pawns(const BoardBase&) const;
    template<Colors C> void mobilityRestrictions(const BoardBase &b, uint64_t (&restrictions)[nColors][nPieces+1]) const;
//    template<Colors C> int mobility(const BoardBase &b, unsigned& attackingPieces, unsigned& defendingPieces) const;

public:
    int aspiration0;
    int aspiration1;
    int evalHardBudget;
    
#ifdef MYDEBUG
    mutable uint64_t bmob1, bmob2, bmob3, bmobn;
    mutable uint64_t rmob1, rmob2, rmob3, rmobn;
    mutable uint64_t qmob1, qmob2, qmob3, qmobn;
#endif
    Eval();
    int eval(const BoardBase&) const;
    template<Colors C> Move evalMate(const BoardBase&) const;
    CompoundScore getPS(int8_t piece, uint8_t square) const {
        ASSERT(square < nSquares);
        ASSERT(piece >= (signed)-nPieces && piece <= (signed)nPieces);
        return zobristPieceSquare[piece+nPieces][square].score;
    }
    CompoundScore& getPS(int8_t piece, uint8_t square) {
        ASSERT(square < nSquares);
        ASSERT(piece >= (signed)-nPieces && piece <= (signed)nPieces);
        return zobristPieceSquare[piece+nPieces][square].score;
    }
    __v8hi getKSVector(int8_t piece, uint8_t square) const {
        static_assert(sizeof(KeyScore) == sizeof(__v8hi), "Structure size error");
        ASSERT(square < nSquares);
        ASSERT(piece >= (signed)-nPieces && piece <= (signed)nPieces);
        return zobristPieceSquare[piece+nPieces][square].vector;
    }
    PawnKey getPawnKey(int8_t piece, uint8_t square) const {
        ASSERT(square < nSquares);
        ASSERT(piece >= (signed)-nPieces && piece <= (signed)nPieces);
        return zobristPieceSquare[piece+nPieces][square].pawnKey;
    }
    template<Colors C>
    void draw(const BoardBase& b, int& upperbound, int& lowerbound) const;
    void ptClear();
};

template<typename T>
void sigmoid(T& p, double start, double end, double dcenter, double width) {
    static const size_t n = sizeof(T)/sizeof(p[0])-1;
    double t0 = -dcenter;
    double t1 = n-dcenter;
    double l0 = 1/(1+exp(-t0/width));
    double l1 = 1/(1+exp(-t1/width));

    double r = (end - start)/(l1-l0);
    double a = start - l0*r;
    for (unsigned int i = 0; i <= n; ++i) {
        double t = i - dcenter;
        p[i] = lrint(a + r/(1.0 + exp(-t/width)));
    }
}

template<typename T>
void printSigmoid(T& p, std::string str) {
    size_t n = sizeof(T)/sizeof(p[0])-1;
    std::cout << std::setw(16) << str;
    for (unsigned int i = 0; i <= n; ++i) {
        std::cout << std::setw(3) << p[i];
    }
    std::cout << std::endl;
}

#endif /* EVAL_H_ */
