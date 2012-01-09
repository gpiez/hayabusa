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
#include "parameters.h"
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
#define NEW_PAWN 1
static const int logEndgameTransitionSlope = 5;
static const int endgameTransitionSlope = 1<<logEndgameTransitionSlope; //16 pieces material between full endgame and full opening

class Parameters;
class PieceList;
class BoardBase;

struct DeltaScore {
    typedef short vector_t __attribute__((vector_size(16)));
    vector_t data;
    DeltaScore() = default;
    DeltaScore(int opening, int endgame) {
        ASSERT(opening <= 0x7fff && opening >= -0x8000);
        ASSERT(endgame <= 0x7fff && endgame >= -0x8000);
        data = (vector_t){ (short)opening, (short)endgame };
    }
    DeltaScore(vector_t init): data(init) {}
    DeltaScore operator + (const DeltaScore& x) const {
        return data + x.data;
    }
    DeltaScore operator - (const DeltaScore& x) const {
        return data - x.data;
    }
    DeltaScore operator - () const {
        return DeltaScore(0,0) - *this;
    }
    int opening() {
        return _mm_extract_epi16(data, 0);
    }
    int endgame() {
        return _mm_extract_epi16(data, 1);
    }
};

struct BiasedScore {
    unsigned data;
    BiasedScore(int opening, int endgame) {
        ASSERT(opening <= 0x7fff && opening >= -0x8000);
        ASSERT(endgame <= 0x7fff && endgame >= -0x8000);
        data = 0x80008000 + (opening << 16 ) + (uint16_t)endgame;
    }
    int calc(int material, const Eval& eval) const;
    
};
struct CompoundScore {
    RawScore    opening;
    RawScore    endgame;

    CompoundScore operator + (const CompoundScore& x) const {
        CompoundScore temp;
        temp.opening = x.opening + opening;
        temp.endgame = x.endgame + endgame;
        return temp;
    }
    CompoundScore operator - (const CompoundScore& x) const {
        CompoundScore temp;
        temp.opening = x.opening - opening;
        temp.endgame = x.endgame - endgame;
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
    int calc(int material, const Eval& eval) const;
    void operator = (int x) {
        opening = x;
        endgame = x;
    }
};

union KeyScore {
    __v8hi vector;
    struct {
        CompoundScore  score;
        PawnKey        pawnKey;
        Key            key;
    };
};

template<typename T>
void sigmoid(T& p, double start, double end, double dcenter = 0, double width = 1.5986, unsigned istart=0 );
template<typename T>
void sigmoid(T& p, Parameters::Phase start, Parameters::Phase end, Parameters::Phase dcenter, Parameters::Phase width, unsigned istart=0);

void sigmoid(int n, int p[], double start, double end, double dcenter, double width = 1.5986);

class Eval {
    KeyScore zobristPieceSquare[nTotalPieces][nSquares];

    TranspositionTable<PawnEntry, 4, PawnKey>* pt;

    Parameters::Piece rook, bishop, queen, knight, pawn, king;
    int queenPair;
    
    int bishopOwnPawn;
    int bishopOppPawn;
    int bishopNotOwnPawn;
    int bishopNotOppPawn;
    
    int bishopPair;
    int bishopBlockPasser;

    int knightPair;
    int knightBlockPasser;
    
    int rookPair;
    
    int rookTrapped;
    int rookOpen;
    int rookHalfOpen;
    int rookWeakPawn;
    
    int pawnBackward;
    int pawnBackwardOpen;
    int pawnIsolatedCenter;
    int pawnIsolatedEdge;
    int pawnIsolatedOpen;
    int pawnDouble;
    int pawnShoulder;    
    int pawnConnPasser[6];
    int pawnPasser[6];
    float pawnPasser2, pawnPasser7, pawnPasserSlope;

    Parameters::Phase pawnBackwardC;
    Parameters::Phase pawnBackwardOpenC;
    Parameters::Phase pawnIsolatedCenterC;
    Parameters::Phase pawnIsolatedEdgeC;
    Parameters::Phase pawnIsolatedOpenC;
    Parameters::Phase pawnDoubleC;
    Parameters::Phase pawnShoulderC;
    Parameters::Phase pawnPasser2C, pawnPasser7C, pawnPasserSlopeC;
    Parameters::Phase pawnConnPasserVC;
    DeltaScore pawnBackward2[8];
    DeltaScore pawnBackwardOpen2[8];
    DeltaScore pawnIsolatedCenter2[8];
    DeltaScore pawnIsolatedEdge2[8];
    DeltaScore pawnIsolatedOpen2[8];
    DeltaScore pawnDouble2[8];
    DeltaScore pawnShoulder2[8];
    DeltaScore pawnConnPasser22[6];
    DeltaScore pawnPasser22[6];

    float oppKingOwnPawnV;  // only 1..7 used
    float ownKingOwnPawnV;
    float oppKingOwnPasserV;  // only 1..7 used
    float ownKingOwnPasserV;
    float pawnConnPasserV;

    int attackR[8];
    int attackB[8];
    int attackQ3[8];
    int attackN[8];
    int attackP2[8];
    int attackK2[8];

    int defenseR[8];
    int defenseB[8];
    int defenseQ[8];
    int defenseN[8];

    int bmo[14], bme[14];
    DeltaScore bm[14];
    int nmo[9], nme[9];
    DeltaScore nm[9];
    int rmo[15], rme[15];
    DeltaScore rm[15];
    int qmo[28], qme[28];
    DeltaScore qm[28];

    int oppKingOwnPawn[8];
    int ownKingOwnPawn[8];
    int oppKingOwnPasser[8];
    int ownKingOwnPasser[8];

    friend int CompoundScore::calc(int, const Eval&) const;
    int endgameMaterial;

    void initPS();
    void initPS(Pieces pIndex, Parameters::Piece& piece);
    void initZobrist();
    void initShield();
    template<GamePhase P>
    int mobilityDiff(const BoardBase& b, int& wap, int& bap, int& wdp, int& bdp) const __attribute__((noinline));
    template<Colors C, GamePhase P>
    int mobility(const BoardBase& b, int& attackingPieces, int& defendingPieces) const /*__attribute__((__always_inline__))*/;
    int attackDiff(const BoardBase& b, const PawnEntry& p, int wap, int bap, int wdp, int bdp) const __attribute__((noinline));
    template<Colors C> int attack2(const BoardBase& b, const PawnEntry& p, int attackingPieces, int defendingPieces) const __attribute__((__always_inline__));
    template<Colors C> int pieces(const BoardBase&, const PawnEntry&) const __attribute__((__always_inline__));
    PawnEntry pawns(const BoardBase&) const;
    template<Colors C> void mobilityRestrictions(const BoardBase &b, uint64_t (&restrictions)[nColors][nPieces+1]) const;
    template<Colors C> int kingSafety(const BoardBase& b) const;
    template<Colors C> int endgame(const BoardBase& b, const PawnEntry&, int sideToMoves) const;
    template<Colors C> int evalShield2(uint64_t pawns, unsigned file) const;
public:
    unsigned dMaxCapture;
    unsigned dMaxExt;
    unsigned dMinSingleExt;
    unsigned dMinDualExt;
    unsigned dMinForkExt;
    unsigned dMinPawnExt;
    unsigned dMinMateExt;
    unsigned dNullIncr;
    unsigned dVerifyIncr;
    unsigned dMinReduction;
    unsigned dMaxExtPawn;
    unsigned dMinExtDisco;
//     unsigned dMaxExtDisco;
    int dRedCapture;
    int dRedCheck;
    int standardError;
    float standardSigma;
    bool calcMeanError;

    int dRed[maxDepth+1];
    unsigned flags;
//     unsigned dMinSingleExt;
//     unsigned dMinMateExt;
    int aspirationLow;
    int aspirationHigh;
    int aspirationHigh2;
    int evalHardBudget;
    int prune1;
    int prune2;
    int prune1c;
    int prune2c;
    unsigned dMaxExtCheck;
    int tempo;
    int castlingTempo;
    struct {
        int outer[3];
        int center[3];
        int inner[3];
        int openFile;
        int halfOpenFile;
        int base;
        float idelta;
        float odelta;
        float vdelta;
    } kingShield;
    int pawnDefense;
    int pieceAttack;
    int pieceDefense;
    int attackTable2[1024];
    int attackTotal;
    int nAttackersTab[16];
    int attackFirst;
    int attackSlope;
    struct {
        int opening;
        int endgame;
    } scale[128];
    int shield[01000], shieldMirrored[01000];       //indexed by 9 bits in front of the king

    static unsigned distance[nSquares][nSquares];  //todo convert to 8 bit, lazy init
    
    Eval(uint64_t, const Parameters&);
    ~Eval();
    void init();
    static void initTables();
    int operator () (const BoardBase&, int sideToMove) const __attribute__((noinline));
    int operator () (const BoardBase&, int sideToMove, int&, int&) const __attribute__((noinline));
    template<Colors C> Move evalMate(const BoardBase&) const __attribute__((noinline));
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
    bool draw(const BoardBase& b, int& upperbound) const;
    void ptClear();
    void setParameters(const Parameters&);
    template<Colors C>
    __v8hi estimate(const Move m, const KeyScore keyScore) const;
    template<Colors C>
    __v8hi inline_estimate(const Move m, const KeyScore keyScore) const __attribute__((always_inline)) ;
    template<typename T>
    static void mulTab(T& p, Parameters::Phase step);

} ALIGN_XMM ;

template<typename T>
void sigmoid(T& p, double start, double end, double dcenter, double width, unsigned istart) {
    const size_t n = sizeof(T)/sizeof(p[0])-1-istart;
    dcenter -= istart;
    double t0 = -dcenter;
    double t1 = n-dcenter;
    double l0 = 1/(1+exp(-t0/width));
    double l1 = 1/(1+exp(-t1/width));

    double r = (end - start)/(l1-l0);
    double a = start - l0*r;
    for (unsigned int i = 0; i < istart; ++i)
        p[i] = 0;
    for (unsigned int i = 0; i <= n; ++i) {
        double t = i - dcenter;
        p[i+istart] = lrint(a + r/(1.0 + exp(-t/width)));
    }
}

template<typename T>
void Eval::mulTab(T& p, Parameters::Phase step) {
    const size_t n = sizeof(T)/sizeof(p[0]);
    p[0] = DeltaScore( 0, 0 );
    for (unsigned int i = 1; i < n; ++i) {
        p[i] = p[i-1] + DeltaScore( step.opening, step.endgame );
    }
}

template<typename T>
void printSigmoid(T& p, std::string str, int offset=0) {
    size_t n;
    for (n = sizeof(T)/sizeof(p[0])-1; n > 0 && !p[n]; --n);
    std::cout << std::setw(5) << str;
    for (unsigned int i = 0; i <= n; ++i) {
        std::cout << std::setw(4) << p[i]-offset;
    }
    std::cout << std::endl;
}

#endif /* EVAL_H_ */
