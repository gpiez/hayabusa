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
uint64_t  ColoredBoard<C>::perft(unsigned int depth) const {
	Move list[256];

	Move* end = generateMoves(list);

	if (depth == 1)
		return end-list;

	uint64_t n=0;
	for (Move* i = list; i<end; ++i) {
		doMove(*i);
		n += next()->perft(depth-1);
	}
	return n;
}

template<Colors C>
void ColoredBoard<C>::divide(unsigned int depth) const
{
	Move list[256];

	Move* end = generateMoves(list);

	uint64_t sum=0;
	QTextStream xout(stderr);
	for (Move* i = list; i < end; ++i) {
		doMove(*i);
		uint64_t n;
		if (depth == 1)
			n = 1;
		else
			n = next()->perft(depth - 1);
		xout << i->string() << " " << n << endl;
		sum += n;
	}
	xout << "Moves: " << end-list << endl << "Nodes: " << sum << endl;
}

template<Colors C>
ColoredBoard<(Colors)-C>* ColoredBoard<C>::next() const {
	// Don't use this+1 because of possible padding from alignment
	return (ColoredBoard<(Colors)-C>*) ((char*)this + sizeof(BoardBase));
}

template<Colors C>
void  ColoredBoard<C>::doMove(Move m) const {
	copyPieces(next());
	uint8_t piece = C*pieces[m.from];
	next()->copyBoardClrPiece<C>(this, piece, m.from);
	next()->enPassant = 0;
	next()->castling[0] = castling[0];
	next()->castling[1] = castling[1];

	next()->fiftyMoves = m.capture!=0 | piece==5 ? 0:fiftyMoves+1;
	ASSERT(C*m.capture <= King);
	
	if (m.special & disableOpponentLongCastling)
		next()->castling[EI].q = 0;
	else if (m.special & disableOpponentShortCastling)
		next()->castling[EI].k = 0;

	switch (m.special & 0xf) {
	case 0:
		break;
	case disableCastling:
		next()->castling[CI].k = 0;
		next()->castling[CI].q = 0;		//todo upper bits don't care, simplify access
		break;
	case disableShortCastling:
		next()->castling[CI].k = 0;
		break;
	case disableLongCastling:
		next()->castling[CI].q = 0;
		break;
	case shortCastling:
		next()->castling[CI].k = 0;
		next()->castling[CI].q = 0;		//todo upper bits don't care, simplify access
		next()->clrPiece<C>(Rook, pov^h1);
		next()->setPiece<C>(Rook, pov^f1);
		break;
	case longCastling:
		next()->castling[CI].k = 0;
		next()->castling[CI].q = 0;		//todo upper bits don't casr, simplify access
		next()->clrPiece<C>(Rook, pov^a1);
		next()->setPiece<C>(Rook, pov^d1);
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
		next()->enPassant = m.to;
		break;
	case EP:
		next()->clrPiece<(Colors)-C>(Pawn, enPassant);
		break;
	}

	if (m.capture)
		next()->clrPiece<(Colors)-C>(-C*m.capture, m.to);
	next()->setPiece<C>(piece, m.to);

}

template<Colors C>
Score<C>  ColoredBoard<C>::qsearch(const Eval& eval, Score<C> alpha, Score<C> beta) const {
	Move firstMove[nMaxMoves];
	Move* lastMove = generateCaptureMoves(firstMove);

	alpha = eval.eval(this);
	
	for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
			doMove(*currentMove);
			Score<C> current = next()->qsearch(eval, beta, alpha);
			if (current > alpha) {
				alpha = current;
			}

	}
	return alpha;
	
}

template<Colors C>
Score<C>  ColoredBoard<C>::search(const Eval& eval, unsigned int depth, Score<C> alpha, Score<C> beta, SearchFlag f) const {
	Move firstMove[nMaxMoves];
	Move* lastMove = generateMoves(firstMove);

	for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
			doMove(*currentMove);
			unsigned int newDepth = depth-1;
			Score<C> current;
			if (newDepth == 0)
				current = next()->search(eval, newDepth, beta, alpha, f);
			else
				current = next()->qsearch(eval, beta, alpha);
			if (current > alpha) {
				alpha = current;
			}

	}
	return alpha;
}

#endif /* COLOREDBOARD_TCC_ */
