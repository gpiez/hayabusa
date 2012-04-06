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

#include "parameters.h"
#include "keyscore.h"
#include "compoundscore.h"
#include "move.h"

#define NEW_PAWN 1
static constexpr uint64_t darkSquares = 0xaa55aa55aa55aa55;

static constexpr int ipw = 1;
static constexpr int inw = 9*ipw;
static constexpr int ibw = 3*inw;
static constexpr int irw = 3*ibw;
static constexpr int iqw = 3*irw;		//243

static constexpr int iqb = 3*iqw;		//729
static constexpr int irb = 3*iqb;		//2187
static constexpr int ibb = 3*irb;		//6561
static constexpr int inb = 3*ibb;		//19683
static constexpr int ipb = 3*inb;

static constexpr int matIndex[nColors][nPieces+1] = {
    {	0, -irb,    -ibb,    -iqb,    -inb,    -ipb,     0 },
    {	0,  irb-irw, ibb-ibw, iqb-iqw, inb-inw, ipb-ipw, 0 } };

static constexpr int minIndex = 9*matIndex[0][Pawn]
                         + 3*matIndex[0][Knight]
                         + 3*matIndex[0][Queen]
                         + 3*matIndex[0][Bishop]
                         + 3*matIndex[0][Rook];

static constexpr int maxIndex = 9*matIndex[1][Pawn]
                         + 3*matIndex[1][Knight]
                         + 3*matIndex[1][Queen]
                         + 3*matIndex[1][Bishop]
                         + 3*matIndex[1][Rook];
class Parameters;
class PieceList;
class Board;
template<Colors> class ColoredBoard;
class PawnEntry;
template<typename, unsigned, typename> class TranspositionTable;

enum Endgames { Unspecified, KBPk, kpbK, KB_kb_, KPk, kpK };

class Eval {
public:
    struct Material {
        unsigned scaleIndex:6;  // 0..egTSlope index in table of scaling factors
        int bias:10;            // -511..511
        unsigned draw:1;
        unsigned drawish:1;     // the position is likely a draw
        unsigned won:1;         // the side with advantage has probably won
        unsigned reduce:1;      // don't search deep after this point
        unsigned doNull:1;      // null move allowed
        Endgames recognized:3;

    } __attribute__((packed));
private:
    KeyScore zobristPieceSquare[nTotalPieces][nSquares];

    TranspositionTable<PawnEntry, 4, PawnKey>* pt;

    PackedScore<> bishopOwnPawn[9];
    PackedScore<> bishopOppPawn[9];
    PackedScore<> bishopNotOwnPawn[9];
    PackedScore<> bishopNotOppPawn[9];
    PackedScore<> bishopBlockPasser[3];
    PackedScore<> bishopPin;
    PackedScore<> bishopTrapped;
    
    PackedScore<> knightBlockPasser[3];
    PackedScore<> knightPin;
    PackedScore<> knightTrapped;
    
    PackedScore<> rookTrapped;
    PackedScore<> rookOpen[9][8];
    PackedScore<> rookHalfOpen[9][8];
    PackedScore<> rookWeakPawn[5];
    PackedScore<> rookOwnPasser[5];
    PackedScore<> rookOppPasser[5];
    PackedScore<> rookSeventh[5];
    PackedScore<> rookPin;

    PackedScore<> pawnBackward2[8];
    PackedScore<> pawnBackwardOpen2[8];
    PackedScore<> pawnIsolatedCenter2[8];
    PackedScore<> pawnIsolatedEdge2[8];
    PackedScore<> pawnIsolatedOpen2[8];
    PackedScore<> pawnDouble2[8];
    PackedScore<> pawnShoulder2[8];
    PackedScore<> pawnConnPasser22[6];
    PackedScore<> pawnPasser22[6];
    PackedScore<> pawnCandidate[6];

    PackedScore<> queenPin;

    PackedScore<> bm[14];
    PackedScore<> nm[9];
    PackedScore<> rm[15];
    PackedScore<> qm[28];

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

    int oppKingOwnPawn[7][8];
    int ownKingOwnPawn[7][8];
    int oppKingOwnPasser[7][8];
    int ownKingOwnPasser[7][8];
    int kingShieldOpenFile;
    int kingShieldHalfOpenFile;

    int noEvalLimit;
public:    
    PackedScore<> scale[materialTotal+1];
private:
//    unsigned matFlag;
    static uint64_t ruleOfSquare[nColors][nSquares];
    static uint64_t keySquare[nColors][nSquares];

    // exists purely to hide access to the float (=slow) initialization parameters
    class Init {
        Eval& e;
        Parameters::Piece rook, bishop, queen, knight, pawn, king;

        PackedScore<float> bishopOwnPawn;
        PackedScore<float> bishopOppPawn;
        PackedScore<float> bishopNotOwnPawn;
        PackedScore<float> bishopNotOppPawn;
        PackedScore<float> bishopBlockPasser;
        PackedScore<> bishopPair;

        PackedScore<float> knightBlockPasser;

        PackedScore<float> rookOpen2;
        PackedScore<float> rookOpenSlope;
        PackedScore<float> rookTrapped;
        PackedScore<float> rookHalfOpen2;
        PackedScore<float> rookHalfOpenSlope;
        PackedScore<float> rookWeakPawn;
        PackedScore<float> rookOwnPasser;
        PackedScore<float> rookOppPasser;
        PackedScore<> rookSeventh;
        PackedScore<> rookSeventh2;
        
        float oppKingOwnPawn2;
        float ownKingOwnPawn2;
        float oppKingOwnPasser2;
        float ownKingOwnPasser2;
        float kingPawnRankFactor;
        float kingPawnRankSlope;
        float kingPawnDistSlope;

        PackedScore<float> pawnBackward;
        PackedScore<float> pawnBackwardOpen;
        PackedScore<float> pawnIsolatedCenter;
        PackedScore<float> pawnIsolatedEdge;
        PackedScore<float> pawnIsolatedOpen;
        PackedScore<float> pawnDouble;
        PackedScore<float> pawnShoulder;
        PackedScore<float> pawnPasser2, pawnPasser7, pawnPasserSlope;
        PackedScore<float> pawnConnPasser;
        PackedScore<float> pawnCandidate;

        int attackFirst;
        int attackSlope;
        int attackTotal;

        int endgameMaterial;
        int endgameTransitionSlope;

        int quant;
        
        PackedScore<> twoMinOneMaj;
        PackedScore<> oneMinThreePawns;
        PackedScore<> rookPerPawn;
        PackedScore<> bishopPerPawn;
        PackedScore<> knightPerPawn;
        PackedScore<> queenPerPawn;
        PackedScore<> pawnPerPawn;
        
        struct {
            int outer[3];
            int center[3];
            int inner[3];
            int base;
            float idelta;
            float odelta;
            float vdelta; } kingShield;
    public:
        Init(Eval& e);
        void setEvalParameters(const Parameters& p);
        void initPS();
        void initPS(Pieces pIndex, Parameters::Piece& piece);
        void initShield();
        template<typename T>
        void sigmoid(T& p, double start, double end, double dcenter = 0, double width = 1.5986, unsigned istart=0 );
        template<typename T>
        void sigmoid(T& p, PackedScore<float> start, PackedScore<float> end, PackedScore<float> dcenter, PackedScore<float> width, unsigned istart=0);
        void sigmoid(int n, int p[], double start, double end, double dcenter, double width = 1.5986);
        template<typename T>
        static void mulTab(T& p, PackedScore<float> step);
        void zobrist();
        void material();
        template<Colors C>
        void material(int r, int b, int q, int n, int p,
                      int er, int eb, int eq, int en, int ep, Material&); };

    template<GamePhase P>
    CompoundScore mobilityDiff(const Board& b, int& wap, int& bap, int& wdp, int& bdp) const __attribute__((noinline));
    template<Colors C, GamePhase P>
    CompoundScore mobility(const Board& b, int& attackingPieces, int& defendingPieces) const /*__attribute__((__always_inline__))*/;
    int attackDiff(const Board& b, const PawnEntry& p, int wap, int bap, int wdp, int bdp) const __attribute__((noinline));
    template<Colors C> int attack2(const Board& b, const PawnEntry& p, int attackingPieces, int defendingPieces) const __attribute__((__always_inline__));
    template<Colors C> CompoundScore pieces(const Board& b, const PawnEntry& p, uint64_t, uint64_t, uint64_t, uint64_t) const __attribute__((__always_inline__));
    PawnEntry pawns(const Board&) const;
    template<Colors C> void mobilityRestrictions(const Board& b, uint64_t (&restrictions)[nColors][nPieces+1]) const;
    template<Colors C> int kingSafety(const Board& b) const;
    template<Colors C> int endgame(const Board& b, const PawnEntry&, int sideToMoves) const;
    template<Colors C> int evalShield2(uint64_t pawns, unsigned file) const;
    int operator () (const Board&, Colors sideToMove, int&, int&) const __attribute__((noinline));
    template<Colors C> unsigned evalKBPk(const Board& b) const;
    template<Colors C> unsigned evalKPk(const Board& b, Colors stm) const;
    template<Colors C> unsigned evalKB_kb_(const Board& b) const;
public:
    unsigned dMaxCapture;
    unsigned dMaxExt;
    unsigned dMaxCheckExt;
    unsigned dMinSingleExt;
    unsigned dMinDualExt;
    unsigned dMinForkExt;
    unsigned dMinPawnExt;
    unsigned dMinMateExt;
    unsigned dNullIncr;
    unsigned dVerifyIncr;
    unsigned dMinReduction;
//    unsigned dMaxExtPawn;
//    unsigned dMinExtDisco;
//     unsigned dMaxExtDisco;
    int dRedCapture;
    int dRedCheck;
    int standardError;
    float standardSigma;

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
    int prune3;
    int prune1c;
    int prune2c;
    int prune3c;
    unsigned dMaxExtCheck;
    int castlingTempo;
    int pawnDefense;
    int pieceAttack;
    int pieceDefense;
    int tempo0;
    int tempo64;
    int sortPrev;
    int sortNext;
    unsigned quantMask;
    int quantRound;
    int quantRoundNeg;
    
    int attackTable2[1024];
    int nAttackersTab[16];
    int allowLazyRoot;
    int shield[01000], shieldMirrored[01000];       //indexed by 9 bits in front of the king
    Material material[maxIndex-minIndex+1];

    static unsigned distance[nSquares][nSquares];  //todo convert to 8 bit, lazy init
#ifdef MYDEBUG
    std::map< std::string, unsigned > control;
#endif

    Eval(uint64_t, const Parameters&);
    ~Eval();
    void init(const Parameters& p);
    static void initTables();
    int operator () (const Board&, Colors sideToMove) const __attribute__((noinline));
    template<Colors C> int operator () (const ColoredBoard<C>&, int&, int&, int, int&) const __attribute__((noinline));
    template<Colors C> Move evalMate(const Board&) const __attribute__((noinline));
    PackedScore<> getPS(int8_t piece, uint8_t square) const {
        ASSERT(square < nSquares);
        ASSERT(piece >= (signed)-nPieces && piece <= (signed)nPieces);
        return zobristPieceSquare[piece+nPieces][square].score; }
    PackedScore<>& getPS(int8_t piece, uint8_t square) {
        ASSERT(square < nSquares);
        ASSERT(piece >= (signed)-nPieces && piece <= (signed)nPieces);
        return zobristPieceSquare[piece+nPieces][square].score; }
    __v8hi getKSVector(int8_t piece, uint8_t square) const {
        static_assert(sizeof(KeyScore) == sizeof(__v8hi), "Structure size error");
        ASSERT(square < nSquares);
        ASSERT(piece >= (signed)-nPieces && piece <= (signed)nPieces);
        return zobristPieceSquare[piece+nPieces][square].vector; }
    PawnKey getPawnKey(int8_t piece, uint8_t square) const {
        ASSERT(square < nSquares);
        ASSERT(piece >= (signed)-nPieces && piece <= (signed)nPieces);
        return zobristPieceSquare[piece+nPieces][square].pawnKey; }
    template<Colors C>
    bool draw(const Board& b, int& upperbound) const;
    void ptClear();
    void setParameters(const Parameters&);
    template<Colors C>
    __v8hi estimate(const Move m, const KeyScore keyScore) const;
    template<Colors C>
    unsigned estimate(const Move m, unsigned) const;
    template<Colors C>
    __v8hi inline_estimate(const Move m, const KeyScore keyScore) const __attribute__((always_inline)) ;
    template<Colors C>
    int calc(const ColoredBoard<C>&b, unsigned matIndex, CompoundScore score) const;
    int calcPS(CompoundScore weights, int bias, unsigned drawish, CompoundScore score) const;
	int interpolate(CompoundScore weights, CompoundScore score) const;
	int interpolate(unsigned iScale, CompoundScore score) const;
	template<Colors C>
	unsigned recognizer(const ColoredBoard<C>& b, unsigned matreco) const;
    int quantize(int) const; };

#endif /* EVAL_H_ */
