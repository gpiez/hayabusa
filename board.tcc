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

template<Colors C>
Score<C> Board::rootSearch(unsigned int depth) const {

	const ColoredBoard<C>* const b = boards;
	
	Move firstMove[nMaxMoves];
	Move* lastMove = b->generateMoves(firstMove);
	Move bestMove;

	QDateTime start = QDateTime::currentDateTime();
	QDateTime softBudget = start.addMSecs( timeBudget/(2*movesToDo) );
	QDateTime hardBudget = start.addMSecs( 2*timeBudget / (movesToDo + 1) );

	for (unsigned int depth=2; depth<maxDepth; depth++) {
		QDateTime currentStart = QDateTime::currentDateTime();
		Score<C> alpha(-infinity);
		Score<C> beta(infinity);
		SearchFlag f;
		for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
			b->doMove(*currentMove);
			Score<C> currentScore = b->next()->search(eval, depth-1, beta, alpha, f);
			if (currentScore > alpha) {
				alpha = currentScore;
				bestMove = *currentMove;
			}
		}
		// assume geometrically increasing time per depth
		if (QDateTime::currentDateTime() > softBudget) break;
	}
}

