/*
 * coloredboard.h
 *
 *  Created on: 17.11.2009
 *      Author: gpiez
 */

#ifndef COLOREDBOARD_H_
#define COLOREDBOARD_H_

#include "boardbase.h"

/*
 * Board with <color> to move. Serves as a common template for all color-dependant functions.
 */
template<Colors C>
class ColoredBoard: public BoardBase {

	friend class Board;
	template<Colors D> friend class ColoredBoard;
	friend class TestBoard;

	static const unsigned int CI = (White-C)/(White-Black);	// ColorIndex, 0 for White, 1 for Black
	static const unsigned int EI = (C-Black)/(White-Black);	// EnemyIndex, 1 for White, 0 for Black
	static const uint8_t pov = CI*56;	//for xoring square values to the other side
	static bool isKnightDistance(int from, int to) {
		return abs( (from&7) - (to&7) )	* abs( (from&0x38) - (to&0x38) ) == 16;
	}
	static bool isKingDistance(int8_t from, int8_t to) {
		return (abs( (from&7) - (to&7) ) | (abs( (from & 0x38) - (to & 0x38) ) >> 3)) == 1;
	}

public:

private:
	void doMove(Move m) const;
	Score search(Score alpha, Score beta, SearchFlag flags);
	uint8_t detectPin( unsigned int pos) const;
	void ray(Move* &list, uint8_t from, uint8_t dir) const;
	void rays(Move* &list, uint8_t from, uint8_t dir, uint8_t spec) const;
	void generateTargetMove(Move* &list, uint8_t to) const;
	void generateTargetCapture(Move* &list, uint8_t to, int8_t cap, Attack a, SpecialMoves spec) const;
	Move* generateCaptureMoves(Move* list) const;
	Move* generateMoves(Move* list) const;
	uint64_t perft(unsigned int depth) const;
	void divide(unsigned int depth) const;
	void search(unsigned int depth) const;
	void rootSearch(unsigned int depth) const;
	ColoredBoard<(Colors)-C>* next() const;

	bool isPromoRank(uint8_t pos) const {
		return (C==White) ? (pos >= a7) : (pos <= h2);
	}

	template<uint8_t R>
	bool isRank(uint8_t pos) const {
		static_assert( R>=1 && R<=8 );
		return pos >= 28-C*36+C*R*8 & pos < 36-C*36+C*R*8;
	}
	int8_t index( unsigned int dir, unsigned int pos) const {
		return dir<4 ? attVec[dir&3][pos].rIndex : attVec[dir&3][pos].lIndex;
	}

	uint8_t length( uint8_t dir, uint8_t pos) const {
		return dir<4 ? attLen[dir&3][pos].right : attLen[dir&3][pos].left;
	}

	bool isValid(uint8_t dir) const {
		return dir < 0x80;
	}

	bool isLongAttack( unsigned int dir, unsigned int pos) const {
		return index(dir, pos) == -C*3 | index(dir, pos) == -C*(int)((dir&1)+1);
	}

public:

	template<int piece>	bool attackedBy(uint8_t pos);
//	bool attackedBy(unsigned int pos);
};

#endif /* COLOREDBOARD_H_ */
