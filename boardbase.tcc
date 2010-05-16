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
#include "rootboard.h"

namespace as {
extern "C" {
	void setPieceW(BoardBase*, int, int);
	void setPieceB(BoardBase*, int, int);
	void clrPieceAndCopyW(const BoardBase*, BoardBase*, int, int);
	void clrPieceAndCopyB(const BoardBase*, BoardBase*, int, int);
	void clrPieceW(BoardBase*, BoardBase*, int, int);
	void clrPieceB(BoardBase*, BoardBase*, int, int);
	void chgPieceW(BoardBase*, int, int, int);
	void chgPieceB(BoardBase*, int, int, int);
}
}

template<Colors C>
void BoardBase::setPiece(uint8_t piece, uint8_t pos, const RootBoard& rb) {
	ASSERT(piece <= King && piece > 0);
	ASSERT(pos < 64);
	ASSERT(pieces[pos] == 0);
	pieceList[C<0].add(piece, pos);
	keyScore.vector += rb.getKSVector(C*piece, pos);
//	pieceSquare += rb.getPS(C*piece, pos);
	if (C==White)
		as::setPieceW(this, C*piece, pos);
	else
		as::setPieceB(this, C*piece, pos);
	for (unsigned int i = 0; i < 256; ++i) {
		ASSERT(64 > ((uint8_t*)attLen)[i]);
	}
	pieces[pos] = C*piece;
}

template<Colors C>
void BoardBase::copyBoardClrPiece(const BoardBase* prev, uint8_t piece, uint8_t pos, const RootBoard& rb) {
	ASSERT(piece <= King && piece > 0);
	ASSERT(pos < 64);
	ASSERT(pieces[pos] == C*piece);
	pieceList[C<0].sub(piece, pos); //TODO copy piecelist here, not in doMove()
	keyScore.vector = prev->keyScore.vector - rb.getKSVector(C*piece, pos);
//	pieceSquare = prev->pieceSquare - rb.getPS(C*piece, pos);
	if (C==White)
		as::clrPieceAndCopyW(prev, this, C*piece, pos);
	else
		as::clrPieceAndCopyB(prev, this, C*piece, pos);
	for (unsigned int i = 0; i < 256; ++i) {
		ASSERT(64 > ((uint8_t*)attLen)[i]);
	}
	pieces[pos] = 0;
}

template<Colors C>
void BoardBase::clrPiece(uint8_t piece, uint8_t pos, const RootBoard& rb) {
	ASSERT(piece <= King && piece > 0);
	ASSERT(pos < 64);
	ASSERT(pieces[pos] == C*piece);
	pieceList[C<0].sub(piece, pos);
	keyScore.vector -= rb.getKSVector(C*piece, pos);
//	pieceSquare -= rb.getPS(C*piece, pos);
	if (C==White)
		as::clrPieceW(this, this, C*piece, pos);
	else
		as::clrPieceB(this, this, C*piece, pos);
	for (unsigned int i = 0; i < 256; ++i) {
		ASSERT(64 > ((uint8_t*)attLen)[i]);
	}
	pieces[pos] = 0;
}

template<Colors C>
void BoardBase::chgPiece(uint8_t oldpiece, uint8_t piece, uint8_t pos, const RootBoard& rb) {
	ASSERT(piece <= King && piece > 0);
	ASSERT(oldpiece < King && oldpiece > 0);
	ASSERT(pos < 64);
	ASSERT(pieces[pos] == -C*oldpiece);
	pieceList[C>0].sub(oldpiece, pos);
	pieceList[C<0].add(piece, pos);
	keyScore.vector += rb.getKSVector(C*piece, pos) - rb.getKSVector(-C*oldpiece, pos);
//	pieceSquare += rb.getPS(C*piece, pos) - rb.getPS(-C*oldpiece, pos);	
	if (C==White) {
		as::chgPieceW(this, -C*oldpiece, C*piece, pos);
	} else {
		as::chgPieceB(this, -C*oldpiece, C*piece, pos);
	}
	for (unsigned int i = 0; i < 256; ++i) {
		ASSERT(64 > ((uint8_t*)attLen)[i]);
	}
	pieces[pos] = C*piece;
}

#endif