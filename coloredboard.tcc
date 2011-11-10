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
#include "boardbase.tcc"
#include "rootboard.h"

template<Colors PREVC>
unsigned ColoredBoard<PREVC>::errorPieceIndex[7] =
  { 0, 6+1*PREVC, 6+1*PREVC, 6+3*PREVC, 6+1*PREVC, 6+5*PREVC, 6+6*PREVC };

/*
 * Construct a board with color C to move
 * with executing move m from a given position prev
 */
template<Colors C>
template<typename T>
ColoredBoard<C>::ColoredBoard(const T& prev, Move m, __v8hi est) {
    prev.copyPieces(*this);
    prev.doMove(this, m);
    if (prev.CI == (unsigned)CI)
        fiftyMoves = 0;
    keyScore.vector = est;
    buildAttacks();
}
/*
 * Execute a move and put result in next
 */
template<Colors C>
void ColoredBoard<C>::doMove(BoardBase* next, Move m) const {
    uint64_t from = 1ULL << m.from();
    uint64_t to = 1ULL << m.to();
    ASSERT(m.piece());

    next->cep.castling.data4 = cep.castling.data4 & castlingMask[m.from()].data4 & castlingMask[m.to()].data4;

    if (m.isSpecial()) {
        doSpecialMove(next, m, from, to);
    } else {
        // standard move, e. p. is handled by captureOffset
        next->fiftyMoves = (!m.capture() & (m.piece() != Pawn)) * (fiftyMoves+1); //(m.capture()) | (m.piece()==Pawn) ? 0:fiftyMoves+1;
        // Do only fill in the enpPassant field, if there is really a pawn which
        // can capture besides the target square. This is because enpassant goes
        // into the zobrist key.
        next->cep.enPassant = shift<C*16>(getPieces<C,Pawn>() & from) & to & shift<C*8>(getAttacks<-C,Pawn>());
	ASSERT(occupied[CI] & from);
	ASSERT(~occupied[CI] & to);
	ASSERT(from != to);
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] & ~to;
        next->getPieces<C>(m.piece()) += to - from;
        next->getPieces<-C>(m.capture()) -= to;
        next->material = material - materialTab[m.capture()];
    }
    next->occupied1 = next->occupied[CI] | next->occupied[EI];
}

template<Colors C>
void ColoredBoard<C>::doSpecialMove(BoardBase* next, Move m, uint64_t from, uint64_t to) const {
    next->fiftyMoves = 0;
    next->cep.enPassant = 0;
    unsigned int piece = m.piece() & 7;
    if (piece == King) {
        ASSERT(m.capture() == 0);
        next->getPieces<C,King>() ^= from + to;
        next->occupied[EI] = occupied[EI];
        next->material = material;
        if (m.to() == (pov^g1)) {
            // short castling
            next->occupied[CI] = occupied[CI] ^ 0b1111ULL << m.from();
            next->getPieces<C,Rook>() ^= (from + to) << 1;
        } else {
            // long castling
            next->occupied[CI] = occupied[CI] ^ 0b11101ULL << (m.to() & 070);
            next->getPieces<C,Rook>() ^= (from >> 1) + (from >> 4);
        }
        ASSERT(popcount(next->getPieces<C,Rook>()) == popcount(getPieces<C,Rook>()));
    } else if (piece == Pawn) {
        // en passant
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] - shift<-C*8>(to);
        next->getPieces<C,Pawn>() += to - from;
        next->getPieces<-C,Pawn>() -= shift<-C*8>(to);
        next->material = material - materialTab[Pawn];
    } else {
        // promotion
        next->occupied[CI] = occupied[CI] - from + to;
        next->occupied[EI] = occupied[EI] & ~to;
        next->getPieces<C,Pawn>() -= from;
        next->getPieces<C>(piece) += to;
        next->getPieces<-C>(m.capture()) -= to;
        next->material = material - materialTab[m.capture()] + materialTab[piece] - materialTab[Pawn];
    }
}

template<Colors C>
Key ColoredBoard<C>::getZobrist() const {
    return keyScore.key + cep.castling.data4 + cep.enPassant + (C+1);
}

template<Colors C>
int ColoredBoard<C>::isPieceHanging() const {
    uint64_t oppAtt = getAttacks<-C,Pawn>();
    uint64_t ownPieces = getOcc<C>() & ~getPieces<C,Pawn>();
    if (ownPieces & oppAtt) return true;
    oppAtt |= getAttacks<-C, Knight>() | getAttacks<-C, Bishop>();
    ownPieces &= ~(getPieces<C,Knight>() | getPieces<C,Bishop>());
    if (ownPieces & oppAtt) return true;
    oppAtt |= getAttacks<-C, Rook>();
    ownPieces &= ~getPieces<C,Rook>();
    if (ownPieces & oppAtt) return true;

    return false;
}
#endif /* COLOREDBOARD_TCC_ */
