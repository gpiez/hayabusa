/*
 * boardbase.h
 *
 *  Created on: 17.11.2009
 *      Author: gpiez
 */

#ifndef BOARDBASE_H_
#define BOARDBASE_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include <emmintrin.h>

#include "attacks.h"
#include "length.h"
#include "piecelist.h"
#include "transpositiontable.h"
#include "score.h"
/*
 * Global variables imported by setpiece.asm
 */
extern "C" {
	extern SAttack shortAttacks[nPieces+1][nSquares][nColors][nSquares] ALIGN_PAGE;
	extern const __v16qi vecLookup[4] ALIGN_CACHE;
	extern int squareControl[nSquares] ALIGN_PAGE;
}

struct BoardBase {
	LongIndex	attVec[nDirs/2][nSquares];		//0x100
	Length		attLen[nDirs/2][nSquares];		//0x100
	LAttack		longAttack[nColors][nSquares];	//0x080
	SAttack		shortAttack[nColors][nSquares];	//0x080
	int8_t		pieces[nSquares];				//0x040
	PieceList 	pieceList[nColors];				//0x040
	Key zobrist;
	unsigned int fiftyMoves;
	struct {
		bool q,k;
	} castling[nColors];
	RawScore pieceSquare;
	uint8_t enPassant;

	static void initTables();

	unsigned int getLen(unsigned int dir, unsigned int pos) {
		if (dir<4)
			return attLen[dir][pos].right;
		else
			return attLen[dir-4][pos].left;
	}

	void init();
	void print();

	template<Colors C> void setPiece(uint8_t piece, uint8_t pos);
	template<Colors C> void clrPiece(uint8_t piece, uint8_t pos);
	template<Colors C> void copyBoardClrPiece(const BoardBase* prev, uint8_t piece, uint8_t pos);

	void copyPieces(BoardBase* next) const {
		__m128i xmm0 = _mm_load_si128(((__m128i *)pieces) + 0);
		__m128i xmm1 = _mm_load_si128(((__m128i *)pieces) + 1);
		__m128i xmm2 = _mm_load_si128(((__m128i *)pieces) + 2);
		__m128i xmm3 = _mm_load_si128(((__m128i *)pieces) + 3);
		_mm_store_si128(((__m128i *)next->pieces) + 0, xmm0);
		_mm_store_si128(((__m128i *)next->pieces) + 1, xmm1);
		_mm_store_si128(((__m128i *)next->pieces) + 2, xmm2);
		_mm_store_si128(((__m128i *)next->pieces) + 3, xmm3);
		xmm0 = _mm_load_si128(((__m128i *)pieceList) + 0);
		xmm1 = _mm_load_si128(((__m128i *)pieceList) + 1);
		xmm2 = _mm_load_si128(((__m128i *)pieceList) + 2);
		xmm3 = _mm_load_si128(((__m128i *)pieceList) + 3);
		_mm_store_si128(((__m128i *)next->pieceList) + 0, xmm0);
		_mm_store_si128(((__m128i *)next->pieceList) + 1, xmm1);
		_mm_store_si128(((__m128i *)next->pieceList) + 2, xmm2);
		_mm_store_si128(((__m128i *)next->pieceList) + 3, xmm3);
	}
	template<unsigned int ColorIndex>
	Attack attacks( unsigned int pos) const {
		return (Attack) {
			longAttack[ColorIndex][pos],
			shortAttack[ColorIndex][pos]
		};
	}
} ALIGN_CACHE;									//sum:	    3C0

#endif /* BOARDBASE_H_ */
