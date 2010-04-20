#include "pch.h"

#include "board.h"
#include "coloredboard.tcc"

//attacked by (opposite colored) piece.
// if color == White, attackedBy<-King> = b
// template<int P>
// bool Board::attackedBy(uint8_t pos) {
// 	if (color == White)
// 		return reinterpret_cast<ColoredBoard<White>*>(this)->attackedBy<P>(pos);
// 	else
// 		return reinterpret_cast<ColoredBoard<Black>*>(this)->attackedBy<P>(pos);
// 
// }
// 
//
// template<Colors C> Board::searchJob(const ColoredBoard<C>* board, unsigned int depth, Score<C> alpha, Score<C> beta, SearchFlag f) {
// 	Board* newBoard = findFreeBoard();
// 	copyBoard(board, newBoard);
// 	startThread(newBoard, newBoard->boards + (board-(ColoredBoard<C>*)boards), depth, alpha, beta, f);
// }
// 
// 
