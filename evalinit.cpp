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
#include "stringlist.h"
#include "options.h"

Eval::Init::Init(Eval& e):
    e(e) {}
    
template<typename T>
void Eval::Init::sigmoid(T& p, Parameters::Phase start, Parameters::Phase end, Parameters::Phase dcenter, Parameters::Phase width, unsigned istart) {
    const size_t n = sizeof(T)/sizeof(p[0]);
    int16_t o[n];
    int16_t e[n];
    sigmoid(o, start.opening, end.opening, dcenter.opening, width.opening, istart);
    sigmoid(e, start.endgame, end.endgame, dcenter.endgame, width.endgame, istart);
    for (unsigned int i = 0; i < n; ++i)
        p[i] = PackedScore{ o[i], e[i] };
}

template<typename T>
void Eval::Init::sigmoid(T& p, double start, double end, double dcenter, double width, unsigned istart) {
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
        p[i+istart] = a + r/(1.0 + exp(-t/width));
    }
}

template<typename T>
void Eval::Init::mulTab(T& p, Parameters::Phase step) {
    const size_t n = sizeof(T)/sizeof(p[0]);
    CompoundScore cs(0,0);
    for (unsigned int i = 0; i < n; ++i) {
        p[i] = cs.packed();
        cs = cs + CompoundScore( step.opening, step.endgame );
    }
}

void Eval::Init::sigmoid(int n, int p[], double start, double end, double dcenter, double width) {
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

template<typename T>
void printSigmoid(T& p, std::string str, int offset=0) {
    size_t n;
    for (n = sizeof(T)/sizeof(p[0])-1; n > 0 && !p[n]; --n);
    std::cout << std::setw(5) << str;
    for (unsigned int i = 0; i <= n; ++i) {
        std::cout << std::setw(4) << std::right << p[i]-offset;
    }
    std::cout << std::endl;
}

template<typename T>
void printSigmoid2(T& p, std::string str, int offset=0) {
    size_t n;
    for (n = sizeof(T)/sizeof(p[0])-1; n > 0 && (!p[n].opening | !p[n].endgame); --n);
    std::cout << std::setw(5) << str;
    for (unsigned int i = 0; i <= n; ++i) {
    	std::stringstream s;
    	s << p[i].opening-offset << "/" << p[i].endgame-offset; 
        std::cout << std::setw(8) << s.str(); 
    }
    std::cout << std::endl;
}

void Eval::Init::initPS(Pieces pIndex, Parameters::Piece& piece) {
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
        e.getPS( pIndex, sq) = { (short) (piece.value.opening + piece.hor.opening[xh] + piece.vert.opening[yo] + corner*piece.corner.opening),
                               (short) (piece.value.endgame + piece.hor.endgame[xh] + piece.vert.endgame[ye] + corner*piece.corner.endgame) };
        e.getPS(-pIndex, sq ^ 070) = (-CompoundScore(e.getPS( pIndex, sq))).packed();
        print_debug(debugEval, "%4d %4d  ", e.getPS(pIndex, sq).opening, e.getPS(pIndex, sq).endgame);
        if ((sq & 7) == 7) print_debug(debugEval, "%c", '\n');
    }

}

void Eval::Init::initPS() {

    for (unsigned int sq = 0; sq<nSquares; ++sq)
        e.getPS( 0, sq) = PackedScore{0,0};

    initPS(Rook, rook);
    initPS(Bishop, bishop);
    initPS(Queen, queen);
    initPS(Knight, knight);
    initPS(Pawn, pawn);
    initPS(King, king);
}

void Eval::Init::initShield() {
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
        e.shield[index] = score;

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
        e.shieldMirrored[index] = score;
    }
#ifdef MYDEBUG
    for (unsigned i=0; i<3; ++i) {
            std::cout << "kingShield" << i << ": " << std::setw(3) << kingShield.inner[i] << std::setw(3) << kingShield.center[i] << std::setw(3) << kingShield.outer[i] << std::endl;
    }
#endif
}

void Eval::Init::zobrist() {
    std::mt19937 rng;

    rng.seed(1);
    for (int p = -nPieces; p <= (signed int)nPieces; p++)
        for (unsigned int i = 0; i < nSquares; ++i) {
            uint64_t r;
            do {
                r = rng() + ((uint64_t)rng() << 32);
            } while (popcount(r) >= 29 && popcount(r) <= 36);

            if (p)
                e.zobristPieceSquare[p+nPieces][i].key = r;
            else
                e.zobristPieceSquare[p+nPieces][i].key = 0;

            if (abs(p) == Pawn)
                e.zobristPieceSquare[p+nPieces][i].pawnKey = r ^ r >> 32;
            else
                e.zobristPieceSquare[p+nPieces][i].pawnKey = 0;
        }

#ifdef MYDEBUG
    int collision=0;
    for (int p = -nPieces; p <= (signed int)nPieces; p++)
    if (p)
    for (unsigned int s = 0; s < nSquares; ++s)
    for (unsigned int d = 0; d < nSquares; ++d)
    if (d!=s) {
        KeyScore z1, z2;
        z1.vector = e.zobristPieceSquare[p+nPieces][d].vector - e.zobristPieceSquare[p+nPieces][s].vector;
        z2.vector = e.zobristPieceSquare[p+nPieces][s].vector - e.zobristPieceSquare[p+nPieces][d].vector;
        for (int cp = -nPieces; cp <= (signed int)nPieces; cp++)
        if (cp)
        for (unsigned int c = 0; c < nSquares; ++c)
        if (e.zobristPieceSquare[cp+nPieces][c].key == z1.key || e.zobristPieceSquare[p+nPieces][c].key == z2.key)
            ++collision;
    }
    if (collision)
        std::cerr << collision << " Zobrist collisions" << std::endl;
#endif
}

void Eval::Init::scale() {
    static_assert(56 == 4*(materialRook+materialBishop+materialKnight) + 2*materialQueen + 16*materialPawn, "Total material");

    for (unsigned i=0; i<sizeof(e.scale)/sizeof(*e.scale); ++i) {
        int openingScale = i - e.endgameMaterial + endgameTransitionSlope/2;
        openingScale = std::max(0, std::min(openingScale, endgameTransitionSlope));
        e.scale[i].endgame = endgameTransitionSlope-openingScale;
        e.scale[i].opening = openingScale;
    }

    e.evalHardBudget = -20;

    if (Options::debug & DebugFlags::debugEval) {
        printSigmoid(e.nAttackersTab, "nAttackersTab");
        printSigmoid(e.attackN, "attN");
        printSigmoid(e.attackTable2, "attTable2");
        printSigmoid2(e.pawnPasser22, "pass");
        printSigmoid2(e.pawnConnPasser22, "cpass");
    }
#ifdef MYDEBUG
    printSigmoid2(e.pawnCandidate, "Cand");
    for (unsigned rank=1; rank<=6; ++rank) {
    	std::stringstream s;
    	s << rank << ":";
		printSigmoid(e.oppKingOwnPasser[rank], "eKpass" + s.str());
		printSigmoid(e.ownKingOwnPasser[rank], "mKpass" + s.str());
		printSigmoid(e.oppKingOwnPawn[rank], "eKpawn" + s.str());
		printSigmoid(e.ownKingOwnPawn[rank], "mKpawn" + s.str());
    }
#endif    
}
#define SETPARM(x) x = p[ #x ].value; \
                   ASSERT( e.control.erase(toLower( #x )) );
#define SETPARM2(x) SETPARM(x.opening) \
                    SETPARM(x.endgame)
#define SETPARME(x) e.x = p[ #x ].value; \
                   ASSERT( e.control.erase(toLower( #x )) );
#define SETPARME2(x) SETPARME(x.opening) \
                     SETPARME(x.endgame)
void Eval::Init::setEvalParameters(const Parameters& p)
{
#ifdef MYDEBUG
    e.control = p.getIndex();
#endif    
    SETPARM2(pawn.value);
    SETPARM2(pawn.hor.value);
    SETPARM2(pawn.hor.inflection);
    SETPARM2(pawn.vert.value);
    SETPARM2(pawn.vert.inflection);
    SETPARM2(pawn.vcenter);
    SETPARM2(pawn.corner);
    sigmoid(pawn.hor.opening, -pawn.hor.value.opening, pawn.hor.value.opening, pawn.hor.inflection.opening );
    sigmoid(pawn.vcenter.opening, pawn.vert.opening, -pawn.vert.value.opening, pawn.vert.value.opening, pawn.vert.inflection.opening );
    sigmoid(pawn.hor.endgame, -pawn.hor.value.endgame, pawn.hor.value.endgame, pawn.hor.inflection.endgame );
    sigmoid(pawn.vcenter.endgame, pawn.vert.endgame, -pawn.vert.value.endgame, pawn.vert.value.endgame, pawn.vert.inflection.endgame );
    SETPARM2(pawnBackward);
    SETPARM2(pawnBackwardOpen);
    SETPARM2(pawnIsolatedCenter);
    SETPARM2(pawnIsolatedEdge);
    SETPARM2(pawnIsolatedOpen);
    SETPARM2(pawnDouble);
    SETPARM2(pawnShoulder);
    mulTab(e.pawnBackward2, pawnBackward);
    mulTab(e.pawnBackwardOpen2, pawnBackwardOpen);
    mulTab(e.pawnIsolatedCenter2, pawnIsolatedCenter);
    mulTab(e.pawnIsolatedEdge2, pawnIsolatedEdge);
    mulTab(e.pawnIsolatedOpen2, pawnIsolatedOpen);
    mulTab(e.pawnDouble2, pawnDouble);
    mulTab(e.pawnShoulder2, pawnShoulder);
    SETPARM2(pawnPasser2);
    SETPARM2(pawnPasser7);
    SETPARM2(pawnPasserSlope);
    sigmoid(e.pawnPasser22, pawnPasser2, pawnPasser7, Parameters::Phase{6,6}, pawnPasserSlope );
    SETPARM2(pawnConnPasser);
    sigmoid(e.pawnConnPasser22, Parameters::Phase{0,0}, pawnConnPasser, Parameters::Phase{6,6}, pawnPasserSlope );
    SETPARM2(pawnCandidate);
    sigmoid(e.pawnCandidate, Parameters::Phase{0,0}, pawnCandidate, Parameters::Phase{6,6}, pawnPasserSlope);
    SETPARME2(pawnPiece);
    SETPARM2(knight.value);
    SETPARM2(knight.hor.value);
    SETPARM2(knight.hor.inflection);
    SETPARM2(knight.vert.value);
    SETPARM2(knight.vert.inflection);
    SETPARM2(knight.vcenter);
    SETPARM2(knight.corner)
    sigmoid(knight.hor.opening, -knight.hor.value.opening, knight.hor.value.opening, knight.hor.inflection.opening );
    sigmoid(knight.vcenter.opening, knight.vert.opening, -knight.vert.value.opening, knight.vert.value.opening, knight.vert.inflection.opening );
    sigmoid(knight.hor.endgame, -knight.hor.value.endgame, knight.hor.value.endgame, knight.hor.inflection.endgame );
    sigmoid(knight.vcenter.endgame, knight.vert.endgame, -knight.vert.value.endgame, knight.vert.value.endgame, knight.vert.inflection.endgame );
    SETPARME2(knightPair);
    SETPARM2(knightBlockPasser);
    mulTab(e.knightBlockPasser, knightBlockPasser);
    SETPARM2(knight.mobility);
    SETPARM2(knight.mobslope);
    sigmoid(e.nm, Parameters::Phase{ -knight.mobility.opening,  -knight.mobility.endgame }, knight.mobility, Parameters::Phase{0,0}, knight.mobslope);

    SETPARM2(rook.value);
    SETPARM2(rook.hor.value);
    SETPARM2(rook.hor.inflection);
    SETPARM2(rook.vert.value);
    SETPARM2(rook.vert.inflection);
    SETPARM2(rook.vcenter);
    SETPARM2(rook.corner);
    sigmoid(rook.hor.opening, -rook.hor.value.opening, rook.hor.value.opening, rook.hor.inflection.opening );
    sigmoid(rook.vcenter.opening, rook.vert.opening, -rook.vert.value.opening, rook.vert.value.opening, rook.vert.inflection.opening );
    sigmoid(rook.hor.endgame, -rook.hor.value.endgame, rook.hor.value.endgame, rook.hor.inflection.endgame );
    sigmoid(rook.vcenter.endgame, rook.vert.endgame, -rook.vert.value.endgame, rook.vert.value.endgame, rook.vert.inflection.endgame );
    SETPARME2(rookTrapped);
    SETPARM2(rookOpen2);
    SETPARM2(rookOpenSlope);
    SETPARM2(rookHalfOpen2);
    SETPARM2(rookHalfOpenSlope);
    SETPARM2(rookWeakPawn);
    SETPARM2(rookOwnPasser);
    SETPARM2(rookOppPasser);
    for (int i=0; i<=8; ++i) {
        Parameters::Phase ro;
        ro.opening = std::max(rookOpenSlope.opening*(i-2)/7.0 + rookOpen2.opening, 0.0);
        ro.endgame = std::max(rookOpenSlope.endgame*(i-2)/7.0 + rookOpen2.endgame, 0.0);
        mulTab(e.rookOpen[i], ro);
        ro.opening = std::max(rookHalfOpenSlope.opening*(i-2)/7.0 + rookHalfOpen2.opening, 0.0);
        ro.endgame = std::max(rookHalfOpenSlope.endgame*(i-2)/7.0 + rookHalfOpen2.endgame, 0.0);
        mulTab(e.rookHalfOpen[i], ro);
    }
    mulTab(e.rookWeakPawn, rookWeakPawn);
    mulTab(e.rookOwnPasser, rookOwnPasser);
    mulTab(e.rookOppPasser, rookOppPasser);
    SETPARM2(rook.mobility);
    SETPARM2(rook.mobslope);
    sigmoid(e.rm, Parameters::Phase{ -rook.mobility.opening,  -rook.mobility.endgame }, rook.mobility, Parameters::Phase{0,0}, rook.mobslope);

    SETPARM2(bishop.value);
    SETPARM2(bishop.hor.value);
    SETPARM2(bishop.hor.inflection);
    SETPARM2(bishop.vert.value);
    SETPARM2(bishop.vert.inflection);
    SETPARM2(bishop.vcenter);
    SETPARM2(bishop.corner);
    sigmoid(bishop.hor.opening, -bishop.hor.value.opening, bishop.hor.value.opening, bishop.hor.inflection.opening );
    sigmoid(bishop.vcenter.opening, bishop.vert.opening, -bishop.vert.value.opening, bishop.vert.value.opening, bishop.vert.inflection.opening );
    sigmoid(bishop.hor.endgame, -bishop.hor.value.endgame, bishop.hor.value.endgame, bishop.hor.inflection.endgame );
    sigmoid(bishop.vcenter.endgame, bishop.vert.endgame, -bishop.vert.value.endgame, bishop.vert.value.endgame, bishop.vert.inflection.endgame );
    SETPARME2(bishopPair);
    SETPARM2(bishopBlockPasser);
    mulTab(e.bishopBlockPasser, bishopBlockPasser);
    SETPARM2(bishopOwnPawn);
    mulTab(e.bishopOwnPawn, bishopOwnPawn);
    SETPARM2(bishopOppPawn);
    mulTab(e.bishopOppPawn, bishopOppPawn);
    SETPARM2(bishopNotOwnPawn);
    mulTab(e.bishopNotOwnPawn, bishopNotOwnPawn);
    SETPARM2(bishopNotOppPawn);
    mulTab(e.bishopNotOppPawn, bishopNotOppPawn);
    SETPARM2(bishop.mobility);
    SETPARM2(bishop.mobslope);
    sigmoid(e.bm, Parameters::Phase{ -bishop.mobility.opening,  -bishop.mobility.endgame }, bishop.mobility, Parameters::Phase{0,0}, bishop.mobslope);

    SETPARM2(queen.value);
    SETPARM2(queen.hor.value);
    SETPARM2(queen.hor.inflection);
    SETPARM2(queen.vert.value);
    SETPARM2(queen.vert.inflection);
    SETPARM2(queen.vcenter);
    SETPARM2(queen.corner);
    sigmoid(queen.hor.opening, -queen.hor.value.opening, queen.hor.value.opening, queen.hor.inflection.opening );
    sigmoid(queen.vcenter.opening, queen.vert.opening, -queen.vert.value.opening, queen.vert.value.opening, queen.vert.inflection.opening );
    sigmoid(queen.hor.endgame, -queen.hor.value.endgame, queen.hor.value.endgame, queen.hor.inflection.endgame );
    sigmoid(queen.vcenter.endgame, queen.vert.endgame, -queen.vert.value.endgame, queen.vert.value.endgame, queen.vert.inflection.endgame );
    SETPARM2(queen.mobility);
    SETPARM2(queen.mobslope);
    sigmoid(e.qm, Parameters::Phase{ -queen.mobility.opening,  -queen.mobility.endgame }, queen.mobility, Parameters::Phase{0,0}, queen.mobslope);

    king.value.opening = 0;
    king.value.endgame = 0;
    SETPARM2(king.hor.value);
    SETPARM2(king.hor.inflection);
    SETPARM2(king.vert.value);
    SETPARM2(king.vert.inflection);
    SETPARM2(king.vcenter);
    SETPARM2(king.corner);
    sigmoid(king.hor.opening, -king.hor.value.opening, king.hor.value.opening, king.hor.inflection.opening );
    sigmoid(king.vcenter.opening, king.vert.opening, -king.vert.value.opening, king.vert.value.opening, king.vert.inflection.opening );
    sigmoid(king.hor.endgame, -king.hor.value.endgame, king.hor.value.endgame, king.hor.inflection.endgame );
    sigmoid(king.vcenter.endgame, king.vert.endgame, -king.vert.value.endgame, king.vert.value.endgame, king.vert.inflection.endgame );

    SETPARME(endgameMaterial);

    SETPARM(oppKingOwnPawn2);
    SETPARM(ownKingOwnPawn2);
    SETPARM(oppKingOwnPasser2);
    SETPARM(ownKingOwnPasser2);
    SETPARM(kingPawnRankFactor);
    SETPARM(kingPawnRankSlope);
    SETPARM(kingPawnDistSlope)
    float rankFactor[7];
    sigmoid(rankFactor, 1.0/sqrt(kingPawnRankFactor), sqrt(kingPawnRankFactor), 7, kingPawnRankSlope, 1);
    for (unsigned rank=1; rank<=6; ++rank) {
		sigmoid( e.oppKingOwnPawn[rank],   -oppKingOwnPawn2*rankFactor[rank],   oppKingOwnPawn2*rankFactor[rank], 1, kingPawnDistSlope);  // only 1..7 used
		sigmoid( e.ownKingOwnPawn[rank],    ownKingOwnPawn2*rankFactor[rank],  -ownKingOwnPawn2*rankFactor[rank], 1, kingPawnDistSlope);
		sigmoid( e.oppKingOwnPasser[rank], -oppKingOwnPasser2*rankFactor[rank], oppKingOwnPasser2*rankFactor[rank], 1, kingPawnDistSlope);  // only 1..7 used
		sigmoid( e.ownKingOwnPasser[rank],  ownKingOwnPasser2*rankFactor[rank],-ownKingOwnPasser2*rankFactor[rank], 1, kingPawnDistSlope);
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

    SETPARME(kingShieldOpenFile);
    SETPARME(kingShieldHalfOpenFile);

    SETPARME(pawnDefense);
    SETPARME(pieceDefense);
    SETPARME(pieceAttack);

    SETPARM(attackTotal);
    sigmoid(e.attackTable2, -attackTotal, attackTotal, sizeof attackTable2/(2*sizeof(int)), 128);

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

    SETPARME(castlingTempo);

    sigmoid(e.attackR, rook.attack, rook.attack*2, 0, 3.0, 1);
    sigmoid(e.attackB, bishop.attack, bishop.attack*2, 0, 3.0, 1);
    sigmoid(e.attackN, knight.attack, knight.attack*2, 0, 3.0, 1);
    sigmoid(e.attackQ3, queen.attack, queen.attack*2, 0, 3.0, 1);
    sigmoid(e.attackP2, pawn.attack, pawn.attack*2, 0, 3.0, 1);
    sigmoid(e.attackK2, king.attack, king.attack*2, 0, 3.0, 1);

    sigmoid(e.defenseR, rook.defense, rook.defense*2, 0, 3.0, 1);
    sigmoid(e.defenseB, bishop.defense, bishop.defense*2, 0, 3.0, 1);
    sigmoid(e.defenseN, knight.defense, knight.defense*2, 0, 3.0, 1);
    sigmoid(e.defenseQ, queen.defense, queen.defense*2, 0, 3.0, 1);

    SETPARM(attackFirst);
    SETPARM(attackSlope);
    sigmoid(e.nAttackersTab, attackFirst, 256, 0, attackSlope, 1);

    SETPARME(dMaxCapture);
    SETPARME(dMaxExt);
    e.dMaxExt += e.dMaxCapture;
    SETPARME(dMinDualExt);
    e.dMinDualExt = e.dMaxExt-e.dMinDualExt;
    SETPARME(dMinSingleExt);
    e.dMinSingleExt = e.dMaxExt-e.dMinSingleExt;
    SETPARME(flags);
    SETPARME(dMinForkExt);
    e.dMinForkExt = e.dMaxExt-e.dMinForkExt;
    SETPARME(dMinMateExt);
    e.dMinMateExt = e.dMaxExt-e.dMinMateExt;
    SETPARME(dMinPawnExt);
    e.dMinPawnExt = e.dMaxExt-e.dMinPawnExt;
    SETPARME(dMinReduction);
    e.dMinReduction += e.dMaxExt;
    SETPARME(dRedCapture);
    SETPARME(dRedCheck);

    SETPARME(dMaxExtCheck);
    e.dMaxExtCheck += e.dMaxExt;
    SETPARME(dMaxExtPawn);
    e.dMaxExtPawn += e.dMaxExt;
    SETPARME(dMinExtDisco);
    e.dMinExtDisco = e.dMaxExt - e.dMinExtDisco;
//     SETPARM(dMaxExtDisco);
//     dMaxExtDisco += dMaxExt;
    SETPARME(dNullIncr);
    SETPARME(dVerifyIncr);

    SETPARME(standardError);
    SETPARME(standardSigma);
    SETPARME(calcMeanError);
    SETPARME(prune1);
    SETPARME(prune2);
    SETPARME(prune1c);
    SETPARME(prune2c);

    SETPARME(dRed[0]);
    SETPARME(dRed[1]);
    SETPARME(dRed[2]);
    SETPARME(dRed[3]);
    SETPARME(dRed[4]);
    SETPARME(dRed[5]);
    SETPARME(dRed[6]);
    SETPARME(dRed[7]);

    SETPARME(aspirationLow);
    SETPARME(aspirationHigh);
    SETPARME(aspirationHigh2);
    SETPARME(tempo);
    for (unsigned i=8; i<=maxDepth; ++i) {
        e.dRed[i] = e.dRed[7];
    }
#ifdef MYDEBUG
    ASSERT(!e.control.size());
#endif
    initShield();
    zobrist();
    initPS();
    this->scale();
}
