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

Board::Board()
	{}

// Move* Board::generateCaptureMoves(Move* list) const {
// 	if (color < 0)
// 		return reinterpret_cast<const ColoredBoard<Black>*>(boards + iMove)->generateCaptureMoves(list);
// 	else
// 		return reinterpret_cast<const ColoredBoard<White>*>(boards + iMove)->generateCaptureMoves(list);
// }
// 
// Move* Board::generateMoves(Move* list) const {
// 	if (color < 0)
// 		return reinterpret_cast<const ColoredBoard<Black>*>(boards + iMove)->generateMoves(list);
// 	else
// 		return reinterpret_cast<const ColoredBoard<White>*>(boards + iMove)->generateMoves(list);
// }
// 
// uint64_t Board::perft(unsigned int depth) {
// 	if (color < 0)
// 		return reinterpret_cast<ColoredBoard<Black>*>(boards + iMove)->perft(depth);
// 	else
// 		return reinterpret_cast<ColoredBoard<White>*>(boards + iMove)->perft(depth);
// }
// 
// void Board::divide(unsigned int depth) {
// 	if (color < 0)
// 		return reinterpret_cast<ColoredBoard<Black>*>(boards + iMove)->divide(depth);
// 	else
// 		return reinterpret_cast<ColoredBoard<White>*>(boards + iMove)->divide(depth);
// }
// 
// void Board::doMove(Move m) {
// 	if (color < 0)
// 		return reinterpret_cast<ColoredBoard<Black>*>(boards + iMove)->doMove(m);
// 	else
// 		return reinterpret_cast<ColoredBoard<White>*>(boards + iMove)->doMove(m);
// 	iMove++;
// }
// 
