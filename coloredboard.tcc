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

/* Execute move m from position prev, both of the previous (opposite) color
 * to construct a board with C to move
 */
template<Colors C>
ColoredBoard<C>::ColoredBoard(const ColoredBoard<(Colors)-C>& prev, Move m, const RootBoard& rb) {
	prev.doMove(this, m, rb);
}

template<Colors C>
void ColoredBoard<C>::doMove(ColoredBoard<(Colors)-C>* next, Move m, const RootBoard& rb) const {
	copyPieces(next);
	uint8_t piece = C*pieces[m.from];
	next->copyBoardClrPiece<C>(this, piece, m.from, rb);
	next->enPassant = 0;
	next->castling.data = castling.data & castlingMask[m.from].data & castlingMask[m.to].data;
	
	next->fiftyMoves = (m.capture!=0) | (piece==5) ? 0:fiftyMoves+1;
	ASSERT(C*m.capture <= King);
	
	switch (m.special & 0xf) {
	case 0:
		break;
	case shortCastling:
		next->clrPiece<C>(Rook, pov^h1, rb);
		next->setPiece<C>(Rook, pov^f1, rb);
		break;
	case longCastling:
		next->clrPiece<C>(Rook, pov^a1, rb);
		next->setPiece<C>(Rook, pov^d1, rb);
		break;
	case promoteQ:
		piece = Queen;
		break;
	case promoteR:
		piece = Rook;
		break;
	case promoteB:
		piece = Bishop;
		break;
	case promoteN:
		piece = Knight;
		break;
	case enableEP:
		next->enPassant = m.to;
		break;
	case EP:
		next->clrPiece<(Colors)-C>(Pawn, enPassant, rb);
		break;
	}

	if (m.capture)
		next->clrPiece<(Colors)-C>(-C*m.capture, m.to, rb);
	next->setPiece<C>(piece, m.to, rb);
}

template<Colors C>
Key ColoredBoard<C>::getZobrist() const {
	return zobrist ^ castling.castling[0].q ^ castling.castling[1].q << 1 ^ castling.castling[0].k << 2 ^ castling.castling[1].k << 3 ^ enPassant << 4 ^ C;
}

//attacked by (opposite colored) piece.
// if color == White, attackedBy<-King> = b
template<Colors C>
template<int P>
bool ColoredBoard<C>::attackedBy(uint8_t pos) {
	switch(C*P) {
	case King: 	// if color == Black, attackedBy<-King> = w
		return shortAttack[0][pos] & attackMaskK;
		break;
	case Pawn:
		return shortAttack[0][pos] & attackMaskP;
		break;
	case Knight:
		return shortAttack[0][pos] & attackMaskN;
		break;
	case -King: 	// if color == Black, attackedBy<-King> = w
		return shortAttack[1][pos] & attackMaskK;
		break;
	case -Pawn:
		return shortAttack[1][pos] & attackMaskP;
		break;
	case -Knight:
		return shortAttack[1][pos] & attackMaskN;
		break;
	case Queen:
		return longAttack[0][pos] & attackMaskQ;
		break;
	case Bishop:
		return longAttack[0][pos] & attackMaskB;
		break;
	case Rook:
		return longAttack[0][pos] & attackMaskR;
		break;
	case -Queen:
		return longAttack[1][pos] & attackMaskQ;
		break;
	case -Bishop:
		return longAttack[1][pos] & attackMaskB;
		break;
	case -Rook:
		return longAttack[1][pos] & attackMaskR;
		break;
	}
}

#endif /* COLOREDBOARD_TCC_ */
