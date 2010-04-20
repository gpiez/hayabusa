/*
 * board.h
 *
 *  Created on: 23.09.2009
 *      Author: gpiez
 */

#ifndef BOARD_H_
#define BOARD_H_

#include "coloredboard.h"
/*
static inline unsigned int sq(const char name[2])
{
	return (name[0]-'a') + (name[1]-'1') * 8;
}
*/
#define MAX_GAME_LENGTH 4096

/// Contains all boards (moves) leading to the current position, from the beginning or a thread split point.
class Board {
	friend class TestBoard;
//	unsigned int iMove;
//	unsigned int threadID;
//	Colors color;
	
public:
	Board();

	template<int piece>	bool attackedBy(uint8_t pos);
	Move* generateCaptureMoves(Move* list) const;
	Move* generateMoves(Move* list) const;
};

#endif /* BOARD_H_ */

