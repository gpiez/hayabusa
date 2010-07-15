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
class BoardBase;

template<typename T>
void sigmoid(T& p, double start, double end, double dcenter = 0, double width = 1.5986 );

union CompoundScore {
    struct {
	RawScore    opening;
	RawScore    endgame;
    };
    int data;
    
    CompoundScore operator + (const CompoundScore& x) const {
        CompoundScore temp;
        temp.data = x.data + data;
        return temp;
    }
    CompoundScore operator - () const {
        CompoundScore temp;
        temp.opening = -opening;
        temp.endgame = -endgame;
        return temp;
    }
    int calc(const BoardBase&, const Eval&) const {
        return opening;
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

union Parameters {
    struct {
        struct {
            float center;
            float slopex;
            float curvx;
            float slopey;
            float curvy;
            float mobility;
            float mobilityCurv;
        } pp[nPieces];
        float rookOpen;
        float rookBehindPasser;
        float rookHalfOpen;
        float rookAttackP;
        float rookAttackK;
    };
    float data[];
};

class PieceList;
class BoardBase;
class Eval {
    KeyScore zobristPieceSquare[nTotalPieces][nSquares];
    
    static CompoundScore pawn, knight, bishop, rook, queen;
    static CompoundScore bishopPair;
    static CompoundScore knightAlone;
    static CompoundScore bishopAlone;
    static CompoundScore n_p_ram[9], n_p[17];
    static CompoundScore b_bad_own_p[9], b_p[17], b_p_ram[9], bpair_p[17];
    static CompoundScore r_p[17];
    static CompoundScore q_p[17];
    // needs to be the same kind of type as PawnEntry.score.
    static int pawnBackward;
    static int pawnBackwardOpen;
    static int pawnIsolated;
    static int pawnHalfIsolated;
    static int pawnPasser[8];
    
    TranspositionTable<PawnEntry, 4, PawnKey>* pt;
    
    RawScore controls[nSquares];
/*    static uint32_t borderTab4_0[nSquares];
    static uint32_t borderTab321[nSquares];
    static uint32_t borderTab567[nSquares];
    static __v16qi kMask0[nSquares];
    static __v16qi kMask1[nSquares];*/
    static __thread PawnEntry pawnEntry;
    void initPS();
    void initZobrist();
    static void initTables();
    unsigned int attackHash( unsigned int  pos );
    int squareControl();

    template<Colors C> int mobility(const BoardBase&) const;
    template<Colors C> int attack(const BoardBase&) const;
    int pieces(const BoardBase&) const;
    int pawns(const BoardBase&) const;
    template<Colors C> void mobilityRestrictions(const BoardBase &b, uint64_t (&restrictions)[nColors][nPieces+1]) const;
    template<Colors C> int mobility( const BoardBase &b, const uint64_t (&restrictions)[nColors][nPieces+1]) const;
public:
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
void printSigmoid(T& p, QString str) {
    size_t n = sizeof(T)/sizeof(p[0])-1;
    QTextStream xout(stderr);
    xout << qSetFieldWidth(16) << str;
    for (unsigned int i = 0; i <= n; ++i) {
        /*xout << qSetFieldWidth(4) << p[i]*/;
    }
    xout << endl;
}


#endif /* EVAL_H_ */
