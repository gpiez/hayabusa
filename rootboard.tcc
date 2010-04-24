#ifndef ROOTBOARD_TCC_
#define ROOTBOARD_TCC_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "rootboard.h"
#include "coloredboard.h"
#include "generateMoves.tcc"
#include "coloredboard.tcc"
#include "perft.h"

template<Colors C>
Score<C> RootBoard::search(const ColoredBoard<(Colors)-C>* const prev, const Move m, const unsigned int depth, Score<C> alpha, const Score<C> beta, const SearchFlag f) const {
	ColoredBoard<C> b;
	Move firstMove[nMaxMoves];
	Move* lastMove = b.generateMoves(firstMove);

/*	for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
			freeJobs.lock();
			if (freeJobs.isInList(this, board, *currentMove)) {
				while (!freeJobs.isfree(this, board, *currentMove) {
					currentMove++;
					if (currentMove >= lastMove) {
						freeJobs.unlock();
						break;
					}
				}
				freeJobs.use(this, board, *currentMove);
			}
			freeJobs.unlock();
			board->doMove(*currentMove);
			unsigned int newDepth = depth-1;
			Score<C> current;

			if (depth > splitDepth && currentMove > firstMove /* && fail-low node && isJobAvailable()//TODO ) {
				searchJob<(Colors)-C>(board->next(), newDepth, beta, alpha, f);
			} else {
				if (currentMove+1 < lastMove)
					freeJobs.add(this, board, currentMove[1]);
				current = search<(Colors)-C>(board->next(), newDepth, beta, alpha, f);
			}
			if (current > alpha) {
				alpha = current;
			}

	}*/
	return alpha;
}

template<Colors C>
Score<C> RootBoard::qsearch(const ColoredBoard<(Colors)-C>* const prev, const Move m, const WorkThread*, Score<C> alpha, const Score<C> beta) const {
	ColoredBoard<C> b;
	Move firstMove[nMaxMoves];
	Move* lastMove = b.generateCaptureMoves(firstMove);

/*	alpha = eval.eval(*this);

	for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
			doMove(*currentMove);
			Score<C> current = next()->qsearch(eval, beta, alpha);
			if (current > alpha) {
				alpha = current;
			}

	}*/
	return alpha;

}

template<Colors C>
Score<C> RootBoard::rootSearch() {
	const ColoredBoard<C>* const b = currentBoard<C>();//color == White ? &boards[iMove].wb : &boards[iMove].bb;

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
		QTextStream xout(stderr);
		for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
			xout << depth << ":" << currentMove->string() << endl;
			Score<C> currentScore = search<(Colors)-C>(b, *currentMove, depth-1, beta, alpha, f);
			if (currentScore > alpha) {
				alpha = currentScore;
				bestMove = *currentMove;
			}
			if (QDateTime::currentDateTime() > hardBudget) break;
		}
		// assume geometrically increasing time per depth
		if (QDateTime::currentDateTime() > softBudget) break;
	}

	return Score<C>(0);
}

template<Colors C>
uint64_t RootBoard::perft(const ColoredBoard<C>* b, unsigned int depth) const {
	Move list[256];

	Move* end = b->generateMoves(list);

	if (depth == 1)
		return end-list;

	uint64_t n=0;
	for (Move* i = list; i<end; ++i) {
		n += perft<(Colors)-C>(b, *i, depth-1);
	}
	return n;
}

template<Colors C>
void RootBoard::divide(const ColoredBoard<C>* b, unsigned int depth) const {
	Move list[256];

	Move* end = b->generateMoves(list);

	uint64_t sum=0;
	QTextStream xout(stderr);
	for (Move* i = list; i < end; ++i) {
		uint64_t n;
		if (depth == 1)
			n = 1;
		else
			n = perft<(Colors)-C>(b, *i, depth-1);
		xout << i->string() << " " << n << endl;
		sum += n;
	}
	xout << "Moves: " << end-list << endl << "Nodes: " << sum << endl;
}


template<Colors C>
uint64_t RootBoard::perft(const ColoredBoard<(Colors)-C>* prev, const Move m, const unsigned int depth) const {
//	if (depth == 4) return Perft<C, 4>::perft(prev, m);
	const ColoredBoard<C> b(prev, m);
	Move list[256];

	Move* end = b.generateMoves(list);

	if (depth == 1) return end-list;

	uint64_t n=0;
	for (Move* i = list; i<end; ++i) {
		n += perft<(Colors)-C>(&b, *i, depth-1);
	}
	return n;
}

#endif