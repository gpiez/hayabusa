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
#include "result.h"
#include "workthread.h"
#include "jobs.h"
template<Colors> class PerftJob;

template<Colors C>
Score<C> RootBoard::search(const ColoredBoard<(Colors)-C>* const /*prev*/, const Move /*m*/, const unsigned int /*depth*/, Score<C> alpha, const Score<C> /*beta*/, const SearchFlag /*f*/) const {
//	ColoredBoard<C> b;
//	Move firstMove[nMaxMoves];
//	Move* lastMove = b.generateMoves(firstMove);

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

			if (depth > splitDepth && currentMove > firstMove  && fail-low node && isJobAvailable()//TODO ) {
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
Move RootBoard::rootSearch() {
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
		SearchFlag f = null;
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

	return (Move){0};
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
uint64_t RootBoard::rootPerft(unsigned int depth) {
	Move list[256];
	
	const ColoredBoard<C>* const b = currentBoard<C>();
	Move* end = b->generateMoves(list);

	if (depth == 1)
		return end-list;

	Result<uint64_t> n(0);
	for (Move* i = list; i<end; ++i) {
		n.setNotReady();
		perft<(Colors)-C>(&n, b, *i, depth-1);
	}
	return n.get();
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

extern uint64_t saved;

template<Colors C>
uint64_t RootBoard::perft(const ColoredBoard<(Colors)-C>* prev, const Move m, const unsigned int depth) const {
//	if (depth == 4) return Perft<C, 4>::perft(prev, m);
	const ColoredBoard<C> b(prev, m);
	Move list[256];

	Move* end = b.generateMoves(list);

	if (depth == 1) return end-list;

	uint64_t n2 = 0;
	bool n2invalid = true;
	Key z = b.getZobrist();
	PerftEntry* pe = pt->retrieve(z);
	if (pe && pe->depth == depth) {
		saved += pe->value;
		n2 = pe->value;
		n2invalid = false;
//		return pe->value;
	}

	uint64_t n=0;
	for (Move* i = list; i<end; ++i) {
		n += perft<(Colors)-C>(&b, *i, depth-1);
	}

	ASSERT(n2invalid || n == n2);
	PerftEntry temp;
	temp.zero();
	temp.depth |= depth;
	temp.upperKey |= z >> temp.upperShift;
	temp.value = n;
	pt->store(z, temp );
	return n;
}

template<Colors C>
void RootBoard::perft(Result<uint64_t>* result, const ColoredBoard<(Colors)-C>* prev, const Move m, const unsigned int depth)
{
	if (depth <= splitDepth) {
		uint64_t n =  perft<C>(prev, m, depth);
		result->update(n);
		result->setReady();
		return;
	}

	const ColoredBoard<C> b(prev, m);

	uint64_t n2 = 0;
	bool n2invalid = true;
	Key z = b.getZobrist();
	PerftEntry* pe = pt->retrieve(z);
	if (pe && pe->depth == depth) {
		saved += pe->value;
		n2 = pe->value;
		n2invalid = false;
//		result->update(pe->value);
//		result->setReady();
//		return;
	}
	
	Move list[256];
	Move* end = b.generateMoves(list);

	Result<uint64_t> n(0);
	for (Move* i = list; i<end; ++i) {
		if (WorkThread* th = findFreeThread()) {
			n.setNotReady();
			th->startJob(new PerftJob<(Colors)-C>(this, &n, &b, *i, depth-1));
		} else {
			n.setNotReady();
			perft<(Colors)-C>(&n, &b, *i, depth-1);			
		}

	}

	ASSERT(n2invalid || n.get() == n2);
	PerftEntry temp;
	temp.zero();
	temp.depth |= depth;
	temp.upperKey |= z >> temp.upperShift;
	temp.value = n.get();
	pt->store(z, temp );
	result->update(n);
	result->setReady();
}


#endif