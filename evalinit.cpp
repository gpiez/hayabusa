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
#include "bits.h"
#include <bits/random.h>
#include <bits/random.tcc>
template<typename T>
void printSigmoid(T& p, std::string str, int offset=0) {
    size_t n;
    for (n = sizeof(T)/sizeof(p[0])-1; n > 0 && !p[n]; --n);
    std::cout << std::setw(5) << str;
    for (unsigned int i = 0; i <= n; ++i) {
        std::cout << std::setw(4) << std::right << p[i]-offset; }
    std::cout << std::endl; }

template<typename T>
void printSigmoid2(T& p, std::string str, int offset=0) {
    size_t n;
    for (n = sizeof(T)/sizeof(p[0])-1; n > 0 && (!p[n].opening | !p[n].endgame); --n);
    std::cout << std::setw(5) << str;
    for (unsigned int i = 0; i <= n; ++i) {
        std::stringstream s;
        s << p[i].opening-offset << "/" << p[i].endgame-offset;
        std::cout << std::setw(8) << s.str(); }
    std::cout << std::endl; }

Eval::Init::Init(Eval& e):
    e(e) {
    e.evalHardBudget = -20;

    if (Options::debug & DebugFlags::debugEval) {
//        printSigmoid(e.nAttackersTab, "nAttackersTab");
        printSigmoid(e.attackN, "attN");
        printSigmoid(e.attackTable2, "attTable2");
        printSigmoid2(e.pawnPasser22, "pass");
        printSigmoid2(e.pawnConnPasser22, "cpass"); }
#ifdef MYDEBUG
    	printSigmoid2(e.pawnCandidate, "Cand");
		for (unsigned rank=1; rank<=6; ++rank) {
			std::stringstream s;
			s << rank << ":";
			printSigmoid(e.oppKingOwnPasser[rank], "eKpass" + s.str());
			printSigmoid(e.ownKingOwnPasser[rank], "mKpass" + s.str());
			printSigmoid(e.oppKingOwnPawn[rank], "eKpawn" + s.str());
			printSigmoid(e.ownKingOwnPawn[rank], "mKpawn" + s.str()); }
#endif
}

template<typename T>
void Eval::Init::sigmoid(T& p, PackedScore<float> start, PackedScore<float> end, PackedScore<float> dcenter, PackedScore<float> width, unsigned istart) {
    const size_t n = sizeof(T)/sizeof(p[0]);
    int16_t o[n];
    int16_t e[n];
    sigmoid(o, start.opening, end.opening, dcenter.opening, width.opening, istart);
    sigmoid(e, start.endgame, end.endgame, dcenter.endgame, width.endgame, istart);
    for (unsigned int i = 0; i < n; ++i)
        p[i] = PackedScore<> { o[i], e[i] }; }

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
        p[i+istart] = a + r/(1.0 + exp(-t/width)); } }

template<typename T>
void Eval::Init::mulTab(T& p, PackedScore<float> step) {
    const size_t n = sizeof(T)/sizeof(p[0]);
    PackedScore<> s = {0,0};
    for (unsigned int i = 0; i < n; ++i) {
        p[i] = s;
        s = s + PackedScore<>{ step.opening, step.endgame }; } }

void Eval::Init::sigmoid(int n, int p[], double start, double end, double dcenter, double width) {
    double t0 = -dcenter;
    double t1 = n-dcenter;
    double l0 = 1/(1+exp(-t0/width));
    double l1 = 1/(1+exp(-t1/width));

    double r = (end - start)/(l1-l0);
    double a = start - l0*r;
    for (int i = 0; i <= n; ++i) {
        double t = i - dcenter;
        p[i] = lrint(a + r/(1.0 + exp(-t/width))); } }

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
        e.keyScore( pIndex, sq).opening(piece.value.opening + piece.hor.opening[xh] + piece.vert.opening[yo] + corner*piece.corner.opening);
        e.keyScore( pIndex, sq).endgame(piece.value.endgame + piece.hor.endgame[xh] + piece.vert.endgame[ye] + corner*piece.corner.endgame);
        if (pIndex == Pawn && y==6) {
            e.keyScore(pIndex, sq).score(e.keyScore(pIndex, sq).score() + e.pawnPasser22[y-1]);
        }
        e.keyScore(-pIndex, sq ^ 070).score(-e.keyScore( pIndex, sq).score());
        print_debug(debugEval, "%4d %4d  ", e.keyScore(pIndex, sq).opening(), e.keyScore(pIndex, sq).endgame());
//        print_debug(debugEval, "%4d %4d  ", e.getPS(-pIndex, sq ^ 070).opening, e.getPS(-pIndex, sq ^ 070).endgame);
        if ((sq & 7) == 7) print_debug(debugEval, "%c", '\n'); }

}

void Eval::Init::initPS() {

    for (unsigned int sq = 0; sq<nSquares; ++sq)
        e.keyScore( 0, sq) = KeyScore{{0}};

    initPS(Rook, rook);
    initPS(Bishop, bishop);
    initPS(Queen, queen);
    initPS(Knight, knight);
    initPS(Pawn, pawn);
    e.pawnPasser22[5] = PackedScore<>{0,0};
    initPS(King, king); }

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
        e.shieldMirrored[index] = score; }
#ifdef MYDEBUG
    for (unsigned i=0; i<3; ++i) {
        std::cout << "kingShield" << i << ": " << std::setw(3) << kingShield.inner[i] << std::setw(3) << kingShield.center[i] << std::setw(3) << kingShield.outer[i] << std::endl; }
#endif
}

void Eval::Init::zobrist() {
    std::mt19937 rng;

    rng.seed(1);
    for (int p = -nPieces; p <= (signed int)nPieces; p++)
        for (unsigned int i = 0; i < nSquares; ++i) {
            uint64_t r;
            do {
                r = rng() + ((uint64_t)rng() << 32); }
            while (popcount(r) >= 29 && popcount(r) <= 36);

            if (p)
                e.zobristPieceSquare[p+nPieces][i].key(r);
            else
                e.zobristPieceSquare[p+nPieces][i].key(0);

            if (abs(p) == Pawn)
                e.zobristPieceSquare[p+nPieces][i].pawnKey(r ^ r >> 32);
            else
                e.zobristPieceSquare[p+nPieces][i].pawnKey(0); }

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
                                    if (e.zobristPieceSquare[cp+nPieces][c].key() == z1.key() || e.zobristPieceSquare[p+nPieces][c].key() == z2.key())
                                        ++collision; }
    if (collision)
        std::cerr << collision << " Zobrist collisions" << std::endl;
#endif
}

template<Colors C>
void Eval::Init::material(int r, int b, int q, int n, int p,
                          int er, int eb, int eq, int en, int ep, Material& m) {
    // Opponent has naked king

    int mat = 5*r + 3*n + 10*q + 3*b + p;
    int emat = 5*er + 3*en + 10*eq + 3*eb + ep;
    if (mat<emat) return;

    int scalemat = (r+er)*materialTab[Rook]
                   + (b+eb)*materialTab[Bishop]
                   + (q+eq)*materialTab[Queen]
                   + (n+en)*materialTab[Knight]
                   + (p+ep)*materialTab[Pawn];

	int scaleIndex = scalemat - endgameMaterial + endgameTransitionSlope/2;
	scaleIndex = std::max(0, std::min(scaleIndex, endgameTransitionSlope));
    m.scale = e.scale[scaleIndex];

    if (b+n >= eb+en+2 && r+1 >= er) {
        int s = C*e.interpolate(m.scale, twoMinOneMaj);
        m.bias += s;
    }
    if (b+n >= eb+en+1 && p+3 >= ep) {
        int s = C*e.interpolate(m.scale, oneMinThreePawns);
        m.bias += s;
    }
    if (b >= 2) {
        int s = C*e.interpolate(m.scale, bishopPair);
        m.bias += s;
    }
    m.bias += e.interpolate(m.scale, rookPerPawn*r*(p+ep-8)/16*C);
    m.bias += e.interpolate(m.scale, bishopPerPawn*b*(p+ep-8)/16*C);
    m.bias += e.interpolate(m.scale, knightPerPawn*n*(p+ep-8)/16*C);
    m.bias += e.interpolate(m.scale, queenPerPawn*q*(p+ep-8)/16*C);
    m.bias += e.interpolate(m.scale, pawnPerPawn*p*(p-4)/16*C);

    if (b == 1 && eb == 1 && q+r+n+eq+er+en == 0)
        m.recognized = KB_kb_;

    if (q+r+b+n && eq+er+eb+en)
        m.doNull = true;

    // Reduce material advantage in likely drawn endgames to a value -pawn..pawn
    // If we have no winning chances at all (lone b or n), reduce to a negative
    // value, otherwise positive.
    if (!mat) {
        ASSERT(emat==0);
        m.draw = true;
        return; }

    // KPk
    if (mat==1 && emat==0) {
        m.recognized = C==White ? KPk : kpK; }
    // N* B*
    if (q+r+p == 0) {
        if (b + n == 1) {
            ASSERT(emat<=3);
            // N B
            if (emat==0) {
                m.draw = true;
                return; }
            // Np Bp, reduce to -¼ pawn
            if (emat==1) {
                m.scale = m.scale/2;
                m.bias = -112*C;
                return; }
            // Npp Bpp, reduce to -½ pawn
            if (emat==2) {
                m.scale = m.scale/2;
                m.bias = -75*C;
                return; }
            ASSERT(emat=3);
            // Nn Nb Bb Bn
            if (en+eb==1) {
                m.draw = true;
                return; }
            // Nppp Bppp, reduce to -¾ pawn
            ASSERT(ep==3);
            m.scale = m.scale/2;
            m.bias = -37*C;
            return; } }
    // NP* BP*

    if (q+r == 0 && p==1) {

        if (b + n == 1 && emat <= 3) {
            // KBPkb KNPkN
            if (emat==3 && en+eb==1) {
                m.scale = m.scale/16;
                return; } } }

    // B
    if (q+r+n == 0 && p && b==1 && emat<=1) {
        m.recognized = C==White ? KBPk : kpbK; }

    // KNNk KNNkp KNNk*
    if (q + r + b + p == 0) {
        if (n==2) {
            if (emat==0) {
                m.draw = true;
                return; }
            else if (emat==1) {
                m.scale = m.scale/2;
                m.bias = -200*C;
                return; }
            else if (emat==2) {
                m.scale = m.scale/2;
                m.bias = -150*C;
                return; }
            else if (emat==3 && eb+en==1) {
                m.draw = true;
                return; }
            else if (emat==4 && eb+en==1) {
                m.scale = m.scale/2;
                m.bias = -50*C;
                return; }
            m.scale = m.scale/2;
            return; } }

    // KQk KRk KBNk KBBk
    if (emat == 0) {
        if (q || r || b+n>=2 || b>=2 || p>=3) {
            m.won = true;
            return; } }

    // KRkb KRkn
    if (q+b+n+p==0 && r==1 && eq+er+ep==0 && eb+en==1) {
        m.bias = -50*C;
        m.scale = m.scale/2;
        return; }

    if (mat-emat >= e.noEvalLimit && mat>2*emat)
        m.reduce = true; }

void Eval::Init::material() {

	/*
	 * Fill the interpolating table with 16 bit signed fixpoint integers.
	 * The constant 0x7fff comes from the left shift by 15 later in
	 * interpolate(). Although it would be possible to use the "exact" -0x8000
	 * and then negate the score afterwards, this produces actually worse
	 * results: Because negative -.5 fractions are rounded towards -infinity
	 * and positive .5 fractions are rounded to zero, negated Compoundscores
	 * do not always result in the exact value of the negated interpolated score
	 */
	for (int i=0; i<=endgameTransitionSlope; ++i) {
		int endgameScale = endgameTransitionSlope - i;
		e.scale[i].opening = (0x7fff*i )/endgameTransitionSlope;
		e.scale[i].endgame = (0x7fff*endgameScale )/endgameTransitionSlope;
#ifdef MYDEBUG
		PackedScore<> test;
		for (test.opening = -100; test.opening <= 100; test.opening += 1)
			for (test.endgame = -100; test.endgame <= 0; test.endgame+=1) {
				PackedScore<> neg;
				neg.endgame = -test.endgame;
				neg.opening = -test.opening;
				int s = e.interpolate(e.scale[i], test);
				int sneg = -e.interpolate(e.scale[i], neg);
				if ( s != sneg) {
					std::cout << "sold:" << s << " snew:" << sneg << " o " << test.opening << " e " << test.endgame << " scale " << i << std::endl;
					std::cout << this->e.scale[i].opening << "." << this->e.scale[i].endgame << ": "
						<< this->e.scale[i].opening*test.opening + this->e.scale[i].endgame*test.endgame << " = "
						<< (this->e.scale[i].opening*test.opening + this->e.scale[i].endgame*test.endgame + 0x4000)/32768.0 << std::endl;
				}
			}
#endif
	}

	static constexpr Material defaultMaterial = {
	    { 0x4000, 0x4000 },     //      PackedScore<> scale;
	    0,                      //      bias
	    0,                      //      draw
	    0,                      //      won
	    0,                      //      reduce
	    0,                      //      doNull
	    Unspecified };
	for (unsigned i=0; i<sizeof e.material / sizeof(Material); ++i)
	    e.material[i] = defaultMaterial;
	for (int wr=0; wr<=2; ++wr)
    for (int br=0; br<=2; ++br)
    for (int wn=0; wn<=2; ++wn)
    for (int bn=0; bn<=2; ++bn)
    for (int wq=0; wq<=2; ++wq)
    for (int bq=0; bq<=2; ++bq)
    for (int wb=0; wb<=2; ++wb)
    for (int bb=0; bb<=2; ++bb)
    for (int wp=0; wp<=8; ++wp)
    for (int bp=0; bp<=8; ++bp) {
        unsigned bi = bp*matIndex[0][Pawn]
                      + bn*matIndex[0][Knight]
                      + bq*matIndex[0][Queen]
                      + bb*matIndex[0][Bishop]
                      + br*matIndex[0][Rook]
                      + wp*matIndex[1][Pawn]
                      + wn*matIndex[1][Knight]
                      + wq*matIndex[1][Queen]
                      + wb*matIndex[1][Bishop]
                      + wr*matIndex[1][Rook]
                      - minIndex;
        unsigned wi = bp*matIndex[1][Pawn]
                      + bn*matIndex[1][Knight]
                      + bq*matIndex[1][Queen]
                      + bb*matIndex[1][Bishop]
                      + br*matIndex[1][Rook]
                      + wp*matIndex[0][Pawn]
                      + wn*matIndex[0][Knight]
                      + wq*matIndex[0][Queen]
                      + wb*matIndex[0][Bishop]
                      + wr*matIndex[0][Rook]
                      - minIndex;
        ASSERT(wi <= maxIndex-minIndex);
        ASSERT(bi <= maxIndex-minIndex);
        material<White>(wr, wb, wq, wn, wp, br, bb, bq, bn, bp, e.material[wi]);
        material<Black>(wr, wb, wq, wn, wp, br, bb, bq, bn, bp, e.material[bi]); } }
#define SETPARM(x) x = p[ #x ].value; \
                   ASSERT( e.control.erase(toLower( #x )) );
#define SETPARM2(x) SETPARM(x.opening) \
                    SETPARM(x.endgame)
#define SETPARME(x) e.x = p[ #x ].value; \
                   ASSERT( e.control.erase(toLower( #x )) );
#define SETPARME2(x) SETPARME(x.opening) \
                     SETPARME(x.endgame)
void Eval::Init::setEvalParameters(const Parameters& p) {
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
    pawnIsolatedEdge.opening = pawnIsolatedEdge.opening + pawnIsolatedCenter.opening;
    pawnIsolatedEdge.endgame = pawnIsolatedEdge.endgame + pawnIsolatedCenter.endgame;
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
    sigmoid(e.pawnPasser22, pawnPasser2, pawnPasser7, PackedScore<float> {6,6 }, pawnPasserSlope );
    SETPARM2(pawnConnPasser);
    sigmoid(e.pawnConnPasser22, PackedScore<float> {0,0 }, pawnConnPasser, PackedScore<float> {6,6 }, pawnPasserSlope );
    SETPARM2(pawnCandidate);
    sigmoid(e.pawnCandidate, PackedScore<float> {0,0 }, pawnCandidate, PackedScore<float> {6,6 }, pawnPasserSlope);
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
    SETPARM2(knightBlockPasser);
    mulTab(e.knightBlockPasser, knightBlockPasser);
    SETPARM2(knight.mobility);
    SETPARM2(knight.mobslope);
    sigmoid(e.nm, PackedScore<float> { -knight.mobility.opening,  -knight.mobility.endgame }, knight.mobility, PackedScore<float> {0,0 }, knight.mobslope);

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
        PackedScore<float> ro;
        ro.opening = std::max(rookOpenSlope.opening*(i-2)/7.0 + rookOpen2.opening, 0.0);
        ro.endgame = std::max(rookOpenSlope.endgame*(i-2)/7.0 + rookOpen2.endgame, 0.0);
        mulTab(e.rookOpen[i], ro);
        ro.opening = std::max(rookHalfOpenSlope.opening*(i-2)/7.0 + rookHalfOpen2.opening, 0.0);
        ro.endgame = std::max(rookHalfOpenSlope.endgame*(i-2)/7.0 + rookHalfOpen2.endgame, 0.0);
        mulTab(e.rookHalfOpen[i], ro); }
    mulTab(e.rookWeakPawn, rookWeakPawn);
    mulTab(e.rookOwnPasser, rookOwnPasser);
    mulTab(e.rookOppPasser, rookOppPasser);
    SETPARM2(rook.mobility);
    SETPARM2(rook.mobslope);
    sigmoid(e.rm, PackedScore<float> { -rook.mobility.opening,  -rook.mobility.endgame }, rook.mobility, PackedScore<float> {0,0 }, rook.mobslope);

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
    SETPARM2(bishopPair);
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
    sigmoid(e.bm, PackedScore<float> { -bishop.mobility.opening,  -bishop.mobility.endgame }, bishop.mobility, PackedScore<float> {0,0 }, bishop.mobslope);

    queen.value.opening = 1000.0/480.0*rook.value.opening;
    queen.value.endgame = 890.0/480.0*rook.value.endgame;
//    SETPARM2(queen.value);
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
    sigmoid(e.qm, PackedScore<float> { -queen.mobility.opening,  -queen.mobility.endgame }, queen.mobility, PackedScore<float> {0,0 }, queen.mobslope);

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

    SETPARM(endgameMaterial);
    SETPARM(endgameTransitionSlope);

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
        sigmoid( e.ownKingOwnPasser[rank],  ownKingOwnPasser2*rankFactor[rank],-ownKingOwnPasser2*rankFactor[rank], 1, kingPawnDistSlope); }

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
        kingShield.inner[i+1]  = kingShield.inner[i]  * kingShield.vdelta; }

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

    e.maxDefense = (rook.defense + knight.defense + bishop.defense + queen.defense)*e.pieceDefense
        + kingShield.center[0]*3 * e.pawnDefense;
    e.maxAttack = (2*rook.attack + 2*knight.attack + 2*bishop.attack + queen.attack + pawn.attack)*e.pieceAttack;
//    SETPARM(attackFirst);
//    SETPARM(attackSlope);
//    sigmoid(e.nAttackersTab, attackFirst, 256, 0, attackSlope, 1);

    SETPARME(dMaxCapture);
    SETPARME(dMaxCheckExt);
    SETPARME(dMinDualExt);
    SETPARME(dMinSingleExt);
    SETPARME(dMinForkExt);
    SETPARME(dMinMateExt);
    SETPARME(dMinPawnExt);
    SETPARME(dMinReduction);
    e.dMaxExt = std::max(std::max(std::max(std::max(std::max(e.dMaxCheckExt,e.dMinDualExt),e.dMinSingleExt),e.dMinForkExt),e.dMinMateExt),e.dMinPawnExt);
    e.dMaxExt += e.dMaxCapture;
    e.dMinDualExt = e.dMaxExt-e.dMinDualExt;
    e.dMinSingleExt = e.dMaxExt-e.dMinSingleExt;
    e.dMinForkExt = e.dMaxExt-e.dMinForkExt;
    e.dMinMateExt = e.dMaxExt-e.dMinMateExt;
    e.dMinPawnExt = e.dMaxExt-e.dMinPawnExt;
    e.dMaxCheckExt = e.dMaxExt - e.dMaxCheckExt;
    e.dMinReduction += e.dMaxExt;
    SETPARME(dRedCapture);
    SETPARME(dRedCheck);
    SETPARME(flags);

    SETPARME(dMaxExtCheck);
    e.dMaxExtCheck += e.dMaxExt;
//    SETPARME(dMaxExtPawn);
//    e.dMaxExtPawn += e.dMaxExt;
//    SETPARME(dMinExtDisco);
//    e.dMinExtDisco = e.dMaxExt - e.dMinExtDisco;
//     SETPARM(dMaxExtDisco);
//     dMaxExtDisco += dMaxExt;
    SETPARME(dNullIncr);
    SETPARME(dVerifyIncr);

    SETPARME(standardError);
    SETPARME(standardSigma);
    SETPARME(prune1);
    SETPARME(prune2);
    SETPARME(prune3);
    SETPARME(prune1c);
    SETPARME(prune2c);
    SETPARME(prune3c);

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
    SETPARME(tempo0);
    SETPARME(tempo64);
    SETPARME(noEvalLimit);
    SETPARM2(rookSeventh);
    SETPARM2(rookSeventh2);
    e.rookSeventh[0] = PackedScore<>{0,0};
    for (int i=1; i<5; ++i) {
        e.rookSeventh[i].opening = e.rookSeventh[i-1].opening + rookSeventh.opening;
        e.rookSeventh[i].endgame = e.rookSeventh[i-1].endgame + rookSeventh.endgame;
        if (i>1) {
            e.rookSeventh[i].opening = e.rookSeventh[i].opening + rookSeventh2.opening;
            e.rookSeventh[i].endgame = e.rookSeventh[i].endgame + rookSeventh2.endgame;
        }
    }

    //    SETPARME(matFlag);
    for (unsigned i=8; i<=maxDepth; ++i) {
        e.dRed[i] = e.dRed[7]; }
    SETPARME(allowLazyRoot);
    SETPARME(sortPrev);
    SETPARME(sortNext);
    SETPARM(quant);
    e.quantMask = (-1)<<quant;
    e.quantRound = (1<<quant)>>1;
    e.quantRoundNeg = ((1 + (1<<quant))>>1 ) - 1;
    SETPARM2(twoMinOneMaj);
    twoMinOneMaj.endgame = twoMinOneMaj.opening;
    SETPARM2(oneMinThreePawns);
    oneMinThreePawns.endgame = oneMinThreePawns.opening;
    SETPARM2(rookPerPawn);
    rookPerPawn.endgame = rookPerPawn.opening;
    SETPARM2(bishopPerPawn);
    SETPARM2(queenPerPawn);
    SETPARM2(knightPerPawn);
//    knightPerPawn.endgame = knightPerPawn.opening;
    SETPARM2(pawnPerPawn);
    SETPARME2(bishopPin);
    e.bishopPin.endgame = e.bishopPin.opening;
    SETPARME2(knightPin);
    e.knightPin.endgame = e.knightPin.opening;
    SETPARME2(rookPin);
    e.rookPin.endgame = e.rookPin.opening;
    SETPARME2(queenPin);
    e.queenPin.endgame = e.queenPin.opening;
    SETPARME2(bishopTrapped);
    e.bishopTrapped.endgame = e.bishopTrapped.opening;
    SETPARME2(knightTrapped);
    e.knightTrapped.endgame = e.knightTrapped.opening;

#ifdef MYDEBUG
    ASSERT(!e.control.size());
#endif
    initShield();
    zobrist();
    initPS();
    material(); }
