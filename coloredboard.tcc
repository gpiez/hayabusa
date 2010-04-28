/*
 * coloredboard.tcc
 *
 *  Created on: 17.11.2009
 *      Author: gpiez
 */

#ifndef COLOREDBOARD_TCC_
#define COLOREDBOARD_TCC_

#include <coloredboard.h>
#include <boardbase.tcc>

template<Colors C> ColoredBoard<C>::ColoredBoard(const ColoredBoard<(Colors)-C>* prev, Move m) {
	prev->doMove(this, m);
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

template<Colors C>
void ColoredBoard<C>::doMove(ColoredBoard<(Colors)-C>* next, Move m) const {
	copyPieces(next);
	uint8_t piece = C*pieces[m.from];
	next->copyBoardClrPiece<C>(this, piece, m.from);
	next->enPassant = 0;
	next->castling[0] = castling[0];
	next->castling[1] = castling[1];
	
	next->fiftyMoves = m.capture!=0 | piece==5 ? 0:fiftyMoves+1;
	ASSERT(C*m.capture <= King);
	
	if (m.special & disableOpponentLongCastling)
		next->castling[EI].q = 0;
	else if (m.special & disableOpponentShortCastling)
		next->castling[EI].k = 0;

	switch (m.special & 0xf) {
	case 0:
		break;
	case disableCastling:
		next->castling[CI].k = 0;
		next->castling[CI].q = 0;		//todo upper bits don't care, simplify access
		break;
	case disableShortCastling:
		next->castling[CI].k = 0;
		break;
	case disableLongCastling:
		next->castling[CI].q = 0;
		break;
	case shortCastling:
		next->castling[CI].k = 0;
		next->castling[CI].q = 0;		//todo upper bits don't care, simplify access
		next->clrPiece<C>(Rook, pov^h1);
		next->setPiece<C>(Rook, pov^f1);
		break;
	case longCastling:
		next->castling[CI].k = 0;
		next->castling[CI].q = 0;		//todo upper bits don't casr, simplify access
		next->clrPiece<C>(Rook, pov^a1);
		next->setPiece<C>(Rook, pov^d1);
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
		next->clrPiece<(Colors)-C>(Pawn, enPassant);
		break;
	}

	if (m.capture)
		next->clrPiece<(Colors)-C>(-C*m.capture, m.to);
	next->setPiece<C>(piece, m.to);
}

template<Colors C>
Key ColoredBoard<C>::getZobrist() const {
	return zobrist ^ castling[0].q ^ castling[1].q << 1 ^ castling[0].k << 2 ^ castling[1].k << 3 ^ enPassant << 4 ^ C;
}

#endif /* COLOREDBOARD_TCC_ */
