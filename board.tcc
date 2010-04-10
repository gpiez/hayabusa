#include "pch.h"

#include "board.h"
#include "coloredboard.tcc"

//attacked by (opposite colored) piece.
// if color == White, attackedBy<-King> = b
template<int P>
bool Board::attackedBy(uint8_t pos) {
	if (color == White)
		return reinterpret_cast<ColoredBoard<White>*>(this)->attackedBy<P>(pos);
	else
		return reinterpret_cast<ColoredBoard<Black>*>(this)->attackedBy<P>(pos);

}
