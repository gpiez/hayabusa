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
#ifndef COLOREDBOARD_TCC_
#define COLOREDBOARD_TCC_

#include "coloredboard.h"
#include "game.h"
#include "workthread.h"
#include "board.tcc"

/*
 * Construct a board with color C to move
 * with executing move m from a given position prev
 */
template<Colors C>
void ColoredBoard<C>::init(const ColoredBoard<(Colors)-C>& prev, Move m) {
    prev.copyPieces(*this);
    prev.doMove2(this, m);
    ply = prev.ply + 1;
    this->m = m;
//    keyScore.vector = est;
    buildAttacks();
}

/*
template<Colors C>
template<typename T>
ColoredBoard<C>::ColoredBoard(const T& prev, Move m, __v8hi est) {
    prev.copyPieces(*this);
    prev.doMove(this, m);
    ply = prev.ply + 1;
    this->m = m;
    keyScore.vector = est;
    buildAttacks();
}
*/

template<Colors C>
template<typename T>
ColoredBoard<C>::ColoredBoard(const T& prev, Move m, Game& game) {
    prev.copyPieces(*this);
    prev.doMove(this, m, game.eval);
    ply = prev.ply + 1;
    this->m = m;
#ifdef __SSE4_1__    
    ASSERT(!_mm_testz_si128(keyScore.vector, game.eval.estimate<C>(m, prev.keyScore)));
#endif
    if (isMain) {
        game.line[ply] = m;
        game.currentPly = ply; }
    buildAttacks(); 
    diff = & game.pe[6 - C*(m.piece() & 7)][m.from()][m.to()];
    psValue = game.eval.calc(*this, matIndex, CompoundScore(keyScore.vector));
    estScore = psValue + prev.positionalScore + *diff;
    prevPositionalScore = prev.positionalScore;
}

/*
 * Execute a move and put result in next
 */

template<Colors C>
void ColoredBoard<C>::doMove(Board* next, Move m) const {
    uint64_t from = 1ULL << m.from();
    uint64_t to = 1ULL << m.to();
    ASSERT(m.piece());

    next->cep.castling.data4 = cep.castling.data4 & castlingMask[m.from()].data4 & castlingMask[m.to()].data4;

    if (m.isSpecial()) {
        doSpecialMove(next, m, from, to); }
    else {
        next->fiftyMoves = (!m.capture() & (m.piece() != Pawn)) * (fiftyMoves+1); //(m.capture()) | (m.piece()==Pawn) ? 0:fiftyMoves+1;
        // Do only fill in the enpPassant field, if there is really a pawn which
        // can capture besides the target square. This is because enpassant goes
        // into the zobrist key.
        next->cep.enPassant = shift<C* 16>(getPieces<C,Pawn>() & from) & to & shift<C* 8>(getAttacks<-C,Pawn>());
        ASSERT(occupied[CI] & from);
        ASSERT(~occupied[CI] & to);
        ASSERT(from != to);
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] & ~to;
        next->getPieces<C>(m.piece()) += to - from;
        next->getPieces<-C>(m.capture()) -= to;
        next->matIndex = matIndex - ::matIndex[EI][m.capture()]; }
    next->occupied1 = next->occupied[CI] | next->occupied[EI]; }


template<Colors C>
void ColoredBoard<C>::doMove2(Board* next, Move m) const {
    uint64_t from = 1ULL << m.from();
    uint64_t to = 1ULL << m.to();
    ASSERT(m.piece());

    next->cep.castling.data4 = cep.castling.data4 & castlingMask[m.from()].data4 & castlingMask[m.to()].data4;

    if (m.isSpecial()) {
        doSpecialMove2(next, m, from, to); }
    else {
        next->fiftyMoves = (!m.capture() & (m.piece() != Pawn)) * (fiftyMoves+1); //(m.capture()) | (m.piece()==Pawn) ? 0:fiftyMoves+1;
        // Do only fill in the enpPassant field, if there is really a pawn which
        // can capture besides the target square. This is because enpassant goes
        // into the zobrist key.
        next->cep.enPassant = shift<C* 16>(getPieces<C,Pawn>() & from) & to & shift<C* 8>(getAttacks<-C,Pawn>());
        ASSERT(occupied[CI] & from);
        ASSERT(~occupied[CI] & to);
        ASSERT(from != to);
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] & ~to;
        next->getPieces<C>(m.piece()) += to - from;
        next->getPieces<-C>(m.capture()) -= to;
        next->matIndex = matIndex - ::matIndex[EI][m.capture()]; }
    next->occupied1 = next->occupied[CI] | next->occupied[EI]; }

template<Colors C>
void ColoredBoard<C>::doMove(Board* next, Move m, const Eval& eval) const {
    uint64_t from = 1ULL << m.from();
    uint64_t to = 1ULL << m.to();
    ASSERT(m.piece());

    next->cep.castling.data4 = cep.castling.data4 & castlingMask[m.from()].data4 & castlingMask[m.to()].data4;

    if (m.isSpecial()) {
        doSpecialMove(next, m, from, to, eval); }
    else {
        // standard move, e. p. is handled by captureOffset
        next->fiftyMoves = (!m.capture() & (m.piece() != Pawn)) * (fiftyMoves+1); //(m.capture()) | (m.piece()==Pawn) ? 0:fiftyMoves+1;
        // Do only fill in the enpPassant field, if there is really a pawn which
        // can capture besides the target square. This is because enpassant goes
        // into the zobrist key.
        next->cep.enPassant = shift<C* 16>(getPieces<C,Pawn>() & from) & to & shift<C* 8>(getAttacks<-C,Pawn>());
        ASSERT(occupied[CI] & from);
        ASSERT(~occupied[CI] & to);
        ASSERT(from != to);
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] & ~to;
        next->getPieces<C>(m.piece()) += to - from;
        next->getPieces<-C>(m.capture()) -= to;
        next->matIndex = matIndex - ::matIndex[EI][m.capture()];
        next->keyScore.vector = keyScore.vector - eval.keyScore(C*m.piece(), m.from()).vector
                                + eval.keyScore(C*m.piece(), m.to()).vector
                                - eval.keyScore(-C*m.capture(), m.to()).vector; }
    next->occupied1 = next->occupied[CI] | next->occupied[EI]; }

template<Colors C>
void ColoredBoard<C>::doSpecialMove(Board* next, Move m, uint64_t from, uint64_t to) const {
    using namespace SquareIndex;
    next->fiftyMoves = 0;
    next->cep.enPassant = 0;
    unsigned int piece = m.piece() & 7;
    if (piece == King) {
        ASSERT(m.capture() == 0);
        next->getPieces<C,King>() ^= from + to;
        next->occupied[EI] = occupied[EI];
        next->matIndex = matIndex;
        if (m.to() == (pov^g1)) {
            // short castling
            next->occupied[CI] = occupied[CI] ^ 0b1111ULL << m.from();
            next->getPieces<C,Rook>() ^= (from + to) << 1; }
        else {
            // long castling
            next->occupied[CI] = occupied[CI] ^ 0b11101ULL << (m.to() & 070);
            next->getPieces<C,Rook>() ^= (from >> 1) + (from >> 4); }
        ASSERT(popcount(next->getPieces<C,Rook>()) == popcount(getPieces<C,Rook>())); }
    else if (piece == Pawn) {
        // en passant
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] - shift<-C*8>(to);
        next->getPieces<C,Pawn>() += to - from;
        next->getPieces<-C,Pawn>() -= shift<-C*8>(to);
        next->matIndex = matIndex - ::matIndex[EI][Pawn]; }
    else {
        // promotion
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] & ~to;
        next->getPieces<C,Pawn>() -= from;
        next->getPieces<C>(piece) += to;
        next->getPieces<-C>(m.capture()) -= to;
        next->matIndex = matIndex - ::matIndex[EI][m.capture()] + ::matIndex[CI][piece]
                         - ::matIndex[CI][Pawn];

    } }

template<Colors C>
void ColoredBoard<C>::doSpecialMove2(Board* next, Move m, uint64_t from, uint64_t to) const {
    using namespace SquareIndex;
    next->fiftyMoves = 0;
    next->cep.enPassant = 0;
    unsigned int piece = m.piece() & 7;
    if (piece == King) {
        ASSERT(m.capture() == 0);
        next->getPieces<C,King>() ^= from + to;
        next->occupied[EI] = occupied[EI];
        if (m.to() == (pov^g1)) {
            // short castling
            next->occupied[CI] = occupied[CI] ^ 0b1111ULL << m.from();
            next->getPieces<C,Rook>() ^= (from + to) << 1; }
        else {
            // long castling
            next->occupied[CI] = occupied[CI] ^ 0b11101ULL << (m.to() & 070);
            next->getPieces<C,Rook>() ^= (from >> 1) + (from >> 4); }
        ASSERT(popcount(next->getPieces<C,Rook>()) == popcount(getPieces<C,Rook>())); }
    else if (piece == Pawn) {
        // en passant
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] - shift<-C*8>(to);
        next->getPieces<C,Pawn>() += to - from;
        next->getPieces<-C,Pawn>() -= shift<-C*8>(to); }
    else {
        // promotion
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] & ~to;
        next->getPieces<C,Pawn>() -= from;
        next->getPieces<C>(piece) += to;
        next->getPieces<-C>(m.capture()) -= to; } }

template<Colors C>
void ColoredBoard<C>::doSpecialMove(Board* next, Move m, uint64_t from, uint64_t to, const Eval& eval) const {
    using namespace SquareIndex;
    next->fiftyMoves = 0;
    next->cep.enPassant = 0;
    unsigned int piece = m.piece() & 7;
    if (piece == King) {
        ASSERT(m.capture() == 0);
        next->getPieces<C,King>() ^= from + to;
        next->occupied[EI] = occupied[EI];
        next->matIndex = matIndex;
        __v8hi estKing = keyScore.vector
                         - eval.keyScore(C*King, m.from()).vector
                         + eval.keyScore(C*King, m.to()).vector;
        if (m.to() == (pov^g1)) {
            // short castling
            next->occupied[CI] = occupied[CI] ^ 0b1111ULL << m.from();
            next->getPieces<C,Rook>() ^= (from + to) << 1;
            next->keyScore.vector = estKing - eval.keyScore(C*Rook, pov^h1).vector
                                    + eval.keyScore(C*Rook, pov^f1).vector; }
        else {
            // long castling
            next->occupied[CI] = occupied[CI] ^ 0b11101ULL << (m.to() & 070);
            next->getPieces<C,Rook>() ^= (from >> 1) + (from >> 4);
            next->keyScore.vector = estKing - eval.keyScore(C*Rook, pov^a1).vector
                                    + eval.keyScore(C*Rook, pov^d1).vector; }
        ASSERT(popcount(next->getPieces<C,Rook>()) == popcount(getPieces<C,Rook>())); }
    else if (piece == Pawn) {
        // en passant
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] - shift<-C*8>(to);
        next->getPieces<C,Pawn>() += to - from;
        next->getPieces<-C,Pawn>() -= shift<-C*8>(to);
        next->matIndex = matIndex - ::matIndex[EI][Pawn];
        next->keyScore.vector = keyScore.vector - eval.keyScore(C*Pawn, m.from()).vector
                                + eval.keyScore(C*Pawn, m.to()).vector
                                - eval.keyScore(-C*Pawn, m.to()-C*8).vector; }
    else {
        // promotion
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] & ~to;
        next->getPieces<C,Pawn>() -= from;
        next->getPieces<C>(piece) += to;
        next->getPieces<-C>(m.capture()) -= to;
        next->matIndex = matIndex - ::matIndex[EI][m.capture()] + ::matIndex[CI][piece]
                         - ::matIndex[CI][Pawn];
        next->keyScore.vector = keyScore.vector - eval.keyScore(C*Pawn, m.from()).vector
                                + eval.keyScore(C*piece, m.to()).vector
                                - eval.keyScore(-C*m.capture(), m.to()).vector; } }

template<Colors C>
Key ColoredBoard<C>::getZobrist() const {
    return keyScore.key() + cep.castling.data4 + cep.enPassant*0x123456789abcdef + (C+1); }

template<Colors C>
uint64_t ColoredBoard<C>::isPieceHanging(const Eval& ) const {
    uint64_t oppAtt = getAttacks<-C,Pawn>();
    uint64_t ownPieces = getOcc<C>() & ~getPieces<C,Pawn>();
#if 0    
    if (ownPieces & oppAtt) return true;
    oppAtt |= getAttacks<-C, Knight>() | getAttacks<-C, Bishop>();
    ownPieces &= ~(getPieces<C,Knight>() | getPieces<C,Bishop>());
    if (ownPieces & oppAtt) return true;
    oppAtt |= getAttacks<-C, Rook>();
    ownPieces &= ~getPieces<C,Rook>();
    if (ownPieces & oppAtt) return true;
#else
    uint64_t hanging = ownPieces & oppAtt;
    if (0) {
        uint64_t undefended = ownPieces & ~getAttacks<C,All>() & getAttacks<-C,All>();
    	hanging |= undefended;
    }
    oppAtt |= getAttacks<-C, Knight>() | getAttacks<-C, Bishop>();
    ownPieces &= ~(getPieces<C,Knight>() | getPieces<C,Bishop>());
    hanging |= ownPieces & oppAtt;
    oppAtt |= getAttacks<-C, Rook>();
    ownPieces &= ~getPieces<C,Rook>();
    hanging |= ownPieces & oppAtt;
    return hanging;
    
//    uint64_t twoAttacks=( (b.getAttacks<C,Rook>()   &  b.getAttacks<C,Pawn>())
//                          | (b.getAttacks<C,Bishop>() & (b.getAttacks<C,Pawn>() | b.getAttacks<C,Rook>()))
//                          | (b.getAttacks<C,Queen>()  & (b.getAttacks<C,Pawn>() | b.getAttacks<C,Rook>() | b.getAttacks<C,Bishop>()))
//                          | (b.getAttacks<C,Knight>() & (b.getAttacks<C,Pawn>() | b.getAttacks<C,Rook>() | b.getAttacks<C,Bishop>() | b.getAttacks<C,Queen>()))
//                          | (b.getAttacks<C,King>()   & (b.getAttacks<C,Pawn>() | b.getAttacks<C,Rook>() | b.getAttacks<C,Bishop>() | b.getAttacks<C,Queen>() | b.getAttacks<C,King>()))
//                        );

#endif

    return false; }
#endif /* COLOREDBOARD_TCC_ */
