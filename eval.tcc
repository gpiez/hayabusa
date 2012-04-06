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

template<Colors C>
__v8hi Eval::estimate(const Move m, const KeyScore keyScore) const {
    enum { pov = C == White ? 0:070 };
    using namespace SquareIndex;
    ASSERT(m.piece());
    if (m.isSpecial()) {
        unsigned piece = m.piece() & 7;
        if (piece == King) {
            ASSERT(m.capture() == 0);
            __v8hi estKing = keyScore.vector
                             - getKSVector(C*King, m.from())
                             + getKSVector(C*King, m.to());
            if (m.to() == (pov^g1)) {
                return estKing - getKSVector(C*Rook, pov^h1)
                       + getKSVector(C*Rook, pov^f1); }
            else {
                return estKing - getKSVector(C*Rook, pov^a1)
                       + getKSVector(C*Rook, pov^d1); } }
        else if (piece == Pawn) {
            return keyScore.vector - getKSVector(C*Pawn, m.from())
                   + getKSVector(C*Pawn, m.to())
                   - getKSVector(-C*Pawn, m.to()-C*8);

        }
        else {
            return keyScore.vector - getKSVector(C*Pawn, m.from())
                   + getKSVector(C*piece, m.to())
                   - getKSVector(-C*m.capture(), m.to()); } }
    else {
        return keyScore.vector
               - getKSVector(C*m.piece(), m.from())
               + getKSVector(C*m.piece(), m.to())
               - getKSVector(-C*m.capture(), m.to()); } }

template<Colors C>
unsigned Eval::estimate(const Move m, unsigned matIndex) const {
    static constexpr unsigned CI = C == White ? 0:1;
    static constexpr unsigned EI = 1-CI;
    ASSERT(m.piece());
    if (m.isSpecial()) {
        unsigned piece = m.piece() & 7;
        if (piece == King) return matIndex;
        else if (piece == Pawn) 
            matIndex -= ::matIndex[EI][Pawn];
        else {
            matIndex -= ::matIndex[CI][Pawn];
            matIndex += ::matIndex[CI][piece];
            matIndex -= ::matIndex[EI][m.capture()];
        }
    } else {
        matIndex -= ::matIndex[EI][m.capture()];
    }
    return matIndex;
}

template<Colors C>
unsigned Eval::evalKBPk(const Board& b) const {
    if (((b.getPieces<C,Pawn>() & file<'a'>()) == b.getPieces<C,Pawn>() 
            && (b.getPieces<C,Bishop>() & (C==White ? darkSquares : ~darkSquares)))) {
        uint64_t front = b.getPieces<C,Pawn>();
        front |= front << 1;
        front |= shift<C* 010>(front);
        front |= shift<C* 020>(front);
        front |= shift<C* 040>(front);
        uint64_t back = b.getPieces<C,Pawn>();
        back |= back << 1;
        back  = shift<-C* 010>(back);
        back |= shift<-C* 020>(back);
        back |= shift<-C* 040>(back);
        if (b.getPieces<-C,King>() & front & ~back) return 3; }
    else if (((b.getPieces<C,Pawn>() & file<'h'>()) == b.getPieces<C,Pawn>()
              && (b.getPieces<C,Bishop>() & (C==White ? ~darkSquares : darkSquares)))) {
        uint64_t front = b.getPieces<C,Pawn>();
        front |= front >> 1;
        front |= shift<C* 010>(front);
        front |= shift<C* 020>(front);
        front |= shift<C* 040>(front);
        uint64_t back = b.getPieces<C,Pawn>();
        back |= back >> 1;
        back  = shift<-C* 010>(back);
        back |= shift<-C* 020>(back);
        back |= shift<-C* 040>(back);
        if (b.getPieces<-C,King>() & front & ~back) return 3;

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

template<Colors C>
unsigned Eval::recognizer(const ColoredBoard<C>& b, unsigned matreco) const {
    switch(matreco) {
    case KBPk:
        return evalKBPk<White>(b);
    case kpbK:
        return evalKBPk<Black>(b);
    case KB_kb_:
        return evalKB_kb_<Black>(b);
    case KPk:
        return evalKPk<White>(b, C);
    case kpK:
        return evalKPk<Black>(b, C);

    case Unspecified:
    default:
        return 0;
    }
    
}
template<Colors C> //FIXME reuse recognizer which was used in psScore
int Eval::operator() (const ColoredBoard<C>& b, int& wap, int& bap, int psValue, int& posScore ) const {
    int realScore;
    switch(material[b.matIndex].recognized) {
    case KBPk:
        realScore = psValue;// >> evalKBPk<White>(b);
        posScore = realScore - psValue;
        break;
    case kpbK:
        realScore = psValue;// >> evalKBPk<Black>(b);
        posScore = realScore - psValue;
        break;
    case KB_kb_:
        posScore = operator()(b, C, wap, bap);
        realScore = psValue + (posScore >> evalKB_kb_<Black>(b));
        posScore = realScore - psValue;
        break;
    case KPk:
        realScore = psValue;//<<2) >> evalKPk<White>(b, C);
        posScore = realScore - psValue;
        break;
    case kpK:
        realScore = psValue;//<<2) >> evalKPk<Black>(b, C);
        posScore = realScore - psValue;
        break;

    case Unspecified:
    default:
        posScore = operator()(b, C, wap, bap)
        + C*(tempo0 + (tempo64*popcount((b.template getAttacks<C,Rook>() | b.template getAttacks<C,Bishop>() | b.template getAttacks<C,Queen>() | b.template getAttacks<C,Knight>() | b.template getAttacks<C,King>()) & ~b.template getOcc<C>()) >> 6));
        realScore = psValue + posScore; 
    }
#ifdef MYDEBUG
    ColoredBoard<(Colors)-C> b2;
    for (int i=0; i<7; ++i) {
        b2.pieces[i][0] = __bswapq(b.pieces[i][1]);
        b2.pieces[i][1] = __bswapq(b.pieces[i][0]);
    }
    b2.occupied[0] = __bswapq(b.occupied[1]);
    b2.occupied[1] = __bswapq(b.occupied[0]);
    b2.occupied1 = __bswapq(b.occupied1);
    b2.keyScore.score.opening = -b.keyScore.score.opening;
    b2.keyScore.score.endgame = -b.keyScore.score.endgame;
    b2.keyScore.pawnKey = 0;
    b2.buildAttacks();
    int wap2 = wap;
    int bap2 = bap;
    int posScore2 = posScore;
//    int eval2 = -this->operator ()(b2, wap2, bap2, -psValue, posScore2);
//    ASSERT(quantize(eval2) == quantize(realScore));
#endif
    return quantize(realScore);
}

template<Colors C>
int Eval::calc(const ColoredBoard<C>& b, unsigned matIndex, CompoundScore score) const {
//    static unsigned ci = 0xdeadbeaf;
//    static CompoundScore cw;
//    static int cb;
//    static unsigned cd;
//    
//    if (matIndex == ci) 
//        return calcPS(cw, cb, cd, score);
    
    unsigned ci = matIndex;
    if unlikely(material[ci].draw) return 0;
    CompoundScore cw = scale[material[ci].scaleIndex];
    int cb = material[ci].bias;
    unsigned cd = material[ci].drawish + recognizer(b, material[ci].recognized) ;
    return calcPS(cw, cb, cd, score); }
