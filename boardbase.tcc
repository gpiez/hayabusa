/*
 * boardbase.tcc
 *
 *  Created on: 12.04.2010
 *      Author: gpiez
 */
#ifndef BOARDBASE_TCC_
#define BOARDBASE_TCC_

#include <boardbase.h>

namespace as {
extern "C" {
	void setPiece(BoardBase*, int, int);
	void clrPiece(const BoardBase*, BoardBase*, int, int);
}
}

template<Colors C>
void BoardBase::setPiece(uint8_t piece, uint8_t pos) {
	ASSERT(piece <= King && piece > 0);
	ASSERT(pos < 64);
	ASSERT(pieces[pos] == 0);
	pieceList[C<0].add(piece, pos);
	zobrist ^= Zobrist::zobrist[C*piece + nPieces][pos];
	as::setPiece(this, C*piece, pos);
	for (unsigned int i = 0; i < 256; ++i) {
		ASSERT(64 > ((uint8_t*)attLen)[i]);
	}
	pieces[pos] = C*piece;
}

template<Colors C>
void BoardBase::copyBoardClrPiece(const BoardBase* prev, uint8_t piece, uint8_t pos) {
	ASSERT(piece <= King && piece > 0);
	ASSERT(pos < 64);
	ASSERT(pieces[pos] == C*piece);
	pieceList[C<0].sub(piece, pos); //TODO copy piecelist here, not in doMove()
	zobrist = prev->zobrist ^ Zobrist::zobrist[C*piece + nPieces][pos];
	as::clrPiece(prev, this, C*piece, pos);  //TODO use black and white specialized asm functions
	for (unsigned int i = 0; i < 256; ++i) {
		ASSERT(64 > ((uint8_t*)attLen)[i]);
	}
	pieces[pos] = 0;
}

template<Colors C>
void BoardBase::clrPiece(uint8_t piece, uint8_t pos) {
	copyBoardClrPiece<C>(this, piece, pos);	// FIXME
}

#endif