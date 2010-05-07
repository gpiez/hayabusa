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