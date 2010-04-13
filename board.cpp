/*
 * board.cpp
 *
 *  Created on: 21.09.2009
 *      Author: gpiez
 */

#include <pch.h>

#include "board.h"
#include "board.tcc"
#include "generateCaptureMoves.tcc"
#include "generateMoves.tcc"

int squareControl[nSquares];

Board::Board() {
}

void Board::setup(QString fen) {
	boards[0].init();

	QString piecePlacement, activeColor, castling, enPassant;
	QTextStream str(&fen);
	str >> piecePlacement >> activeColor >> castling >> enPassant >> boards[0].fiftyMoves >> iMove;

	unsigned int p,x,y;
	for ( p=0, x=0, y=7; p<(unsigned int)piecePlacement.length(); p++, x++ ) {
		unsigned int square = x + 8*y;
		switch ( piecePlacement[p].toLatin1() ) {
		case '1'...'8':
			x += piecePlacement[p].toLatin1() - '1';
			break;
		case 'k':
			boards[0].setPiece( -King, square);
			break;
		case 'p':
			boards[0].setPiece( -Pawn, square);
			break;
		case 'b':
			boards[0].setPiece( -Bishop, square);
			break;
		case 'n':
			boards[0].setPiece( -Knight, square);
			break;
		case 'r':
			boards[0].setPiece( -Rook, square);
			break;
		case 'q':
			boards[0].setPiece( -Queen, square);
			break;
		case 'K':
			boards[0].setPiece( King, square);
			break;
		case 'P':
			boards[0].setPiece( Pawn, square);
			break;
		case 'B':
			boards[0].setPiece( Bishop, square);
			break;
		case 'N':
			boards[0].setPiece( Knight, square);
			break;
		case 'R':
			boards[0].setPiece( Rook, square);
			break;
		case 'Q':
			boards[0].setPiece( Queen, square);
			break;
		case '/':
			x = -1;
			y--;
			break;
		default:
			qWarning() << "Unknown char " << piecePlacement[p] << " in FEN import";
		}
	}

	switch ( activeColor[0].toLatin1() ) {
	case 'w':
	case 'W':
		color = White;
		break;
	case 'b':
	case 'B':
		color = Black;
	}

	for ( unsigned int p=0; p<(unsigned int)castling.length(); p++ )
		switch ( castling[p].toLatin1() ) {
		case 'Q':
			boards[0].castling[0].q = true;
			break;
		case 'K':
			boards[0].castling[0].k = true;
			break;
		case 'q':
			boards[0].castling[1].q = true;
			break;
		case 'k':
			boards[0].castling[1].k = true;
			break;
		}

	if ( enPassant.length() == 2 && enPassant[0] >= 'a' && enPassant[0] <= 'h' && ( enPassant[1] == '3' || enPassant[1] == '6' ) )
		boards[0].enPassant = 8 * ((enPassant[1].toLatin1() - '3' )/3 + 3) + enPassant[0].toLatin1() - 'a';
	else if ( enPassant != "-" )
		qWarning() << "error importing e. p. move " << enPassant;
}

Move* Board::generateCaptureMoves(Move* list) const {
	if (color < 0)
		return reinterpret_cast<const ColoredBoard<Black>*>(boards + iMove)->generateCaptureMoves(list);
	else
		return reinterpret_cast<const ColoredBoard<White>*>(boards + iMove)->generateCaptureMoves(list);
}

Move* Board::generateMoves(Move* list) const {
	if (color < 0)
		return reinterpret_cast<const ColoredBoard<Black>*>(boards + iMove)->generateMoves(list);
	else
		return reinterpret_cast<const ColoredBoard<White>*>(boards + iMove)->generateMoves(list);
}

uint64_t Board::perft(unsigned int depth) {
	if (color < 0)
		return reinterpret_cast<ColoredBoard<Black>*>(boards + iMove)->perft(depth);
	else
		return reinterpret_cast<ColoredBoard<White>*>(boards + iMove)->perft(depth);
}

// void Board::search(unsigned int depth) {
// 	if (color < 0)
// 		reinterpret_cast<ColoredBoard<Black>*>(boards + iMove)->search(depth);
// 	else
// 		reinterpret_cast<ColoredBoard<White>*>(boards + iMove)->search(depth);
// }

void Board::divide(unsigned int depth) {
	if (color < 0)
		return reinterpret_cast<ColoredBoard<Black>*>(boards + iMove)->divide(depth);
	else
		return reinterpret_cast<ColoredBoard<White>*>(boards + iMove)->divide(depth);
}

void Board::doMove(Move m) {
	if (color < 0)
		return reinterpret_cast<ColoredBoard<Black>*>(boards + iMove)->doMove(m);
	else
		return reinterpret_cast<ColoredBoard<White>*>(boards + iMove)->doMove(m);
	iMove++;
}

