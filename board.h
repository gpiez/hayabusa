/*
 * board.h
 *
 *  Created on: 23.09.2009
 *      Author: gpiez
 */

#ifndef BOARD_H_
#define BOARD_H_

#include "boardbase.h"

/*
static inline unsigned int sq(const char name[2])
{
	return (name[0]-'a') + (name[1]-'1') * 8;
}
*/
#define MAX_GAME_LENGTH 4096

class Board {
	friend class TestBoard;
	struct {
		ColoredBoard<White> w;
		ColoredBoard<Black> w;
	} boards[MAX_GAME_LENGTH];
	unsigned int iMove;
	unsigned int threadID;
	Colors color;

public:
	Board();

	void setup(QString fen = QString("rnbqkbnr/pppppppp/////PPPPPPPP/RNBQKBNR w KQkq - 0 0"));
	template<int piece>	bool attackedBy(uint8_t pos);
	Move* generateCaptureMoves(Move* list) const;
	Move* generateMoves(Move* list) const;
	uint64_t perft(unsigned int depth);
	void divide(unsigned int depth);
	void doMove(Move m);
	void rootSearch(unsigned int depth);
	void search(unsigned int depth);
};

enum SearchFlag { null = 1 };
typedef int16_t Score;

#endif /* BOARD_H_ */

