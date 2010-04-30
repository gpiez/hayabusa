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
template<Colors,typename> class PerftJob;

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
uint64_t RootBoard::rootPerft(unsigned int depth) {
	Move list[256];
	
	const ColoredBoard<C>* const b = currentBoard<C>();
	Move* end = b->generateMoves(list);

	if (depth <= 1)
		return end-list;

	Result<uint64_t> n(0);
	for (Move* i = list; i<end; ++i) {
		n.setNotReady();
		perft<(Colors)-C, trunk>(n, b, *i, depth-1);
	}
	return n;
}

template<Colors C>
uint64_t RootBoard::rootDivide(unsigned int depth) {
	Move list[256];

	const ColoredBoard<C>* const b = currentBoard<C>();
	Move* end = b->generateMoves(list);

	QTextStream xout(stderr);
	uint64_t sum=0;
	for (Move* i = list; i<end; ++i) {
		Result<uint64_t> n(0);
		if (depth == 1) {
			n.update(1);
		} else {
			n.setNotReady();
			perft<(Colors)-C, trunk>(n, b, *i, depth-1);
		}
		xout << i->string() << " " << (uint64_t)n << endl;
		sum += n;
	}
	return sum;
}

void updateAndReady(Result<uint64_t>& r, Result<uint64_t>& v);
void updateAndReady(Result<uint64_t>& r, uint64_t v);
void updateAndReady(uint64_t& r, uint64_t v);

template<typename ResultType> void setNotReady(ResultType&) {};

template<Colors C, Phase P, typename ResultType> void RootBoard::perft(ResultType& result, const ColoredBoard<(Colors)-C>* prev, Move m, unsigned int depth) {
	if (P == trunk && depth <= splitDepth) {
		uint64_t n=0;
		perft<C, tree>(n, prev, m, depth);
		updateAndReady(result, n);
		return;
	}

	const ColoredBoard<C> b(prev, m);

	Key z = b.getZobrist();
	PerftEntry* pe = pt->getEntry(z);
	PerftEntry subentry;
	
	if (pt->retrieve(pe, z, subentry) && subentry.depth == depth) {
		updateAndReady(result, subentry.value);
		return;
	}

	Move list[256];
	Move* end = b.generateMoves(list);
	if (depth == 1) {
		updateAndReady(result, end-list);
		return;
	}
	
	ResultType n(0);
	for (Move* i = list; i<end; ++i) {
		WorkThread* th;
		if (P == trunk && !!(th = findFreeThread())) {
			setNotReady(n);
			th->startJob(new PerftJob<(Colors)-C, ResultType>(this, n, &b, *i, depth-1));
		} else {
			setNotReady(n);
			perft<(Colors)-C, trunk>(n, &b, *i, depth-1);
		}

	}

	PerftEntry stored;
	stored.zero();
	stored.depth |= depth;
	stored.upperKey |= z >> stored.upperShift;
	stored.value = (uint64_t)n;
	pt->store(pe, stored);
	updateAndReady(result, n);
}


// template<Colors C>
// uint64_t RootBoard::perft(const ColoredBoard<(Colors)-C>* prev, const Move m, const unsigned int depth) const {
// 	const ColoredBoard<C> b(prev, m);
// 	Move list[256];
// 
// 	Move* end = b.generateMoves(list);
// 
// 	if (depth == 1) return end-list;
// 
// 	Key z = b.getZobrist();
// 	PerftEntry* pe = pt->getEntry(z);
// 	PerftEntry subentry;
// 	
// 	if (pt->retrieve(pe, z, subentry) && subentry.depth == depth) {
// 		return subentry.value;
// 	}
// 
// 	uint64_t n=0;
// 	for (Move* i = list; i<end; ++i) {
// 		n += perft<(Colors)-C>(&b, *i, depth-1);
// 	}
// 
// 	PerftEntry temp;
// 	temp.zero();
// 	temp.depth |= depth;
// 	temp.upperKey |= z >> temp.upperShift;
// 	temp.value = n;
// 	pt->store(pe, temp );
// 	return n;
// }
// 
// template<Colors C>
// void RootBoard::perft(Result<uint64_t>* result, const ColoredBoard<(Colors)-C>* prev, const Move m, const unsigned int depth)
// {
// 	if (depth <= splitDepth) {
// 		uint64_t n =  perft<C>(prev, m, depth);
// 		result->update(n);
// 		result->setReady();
// 		return;
// 	}
// 
// 	const ColoredBoard<C> b(prev, m);
// 
// 	Key z = b.getZobrist();
// 	PerftEntry* pe = pt->getEntry(z);
// 	PerftEntry subentry;
// 	if ( pt->retrieve(pe, z, subentry) && subentry.depth == depth) {
// 		result->update(subentry.value);
// 		result->setReady();
// 		return;
// 	}
// 	
// 	Move list[256];
// 	Move* end = b.generateMoves(list);
// 
// 	Result<uint64_t> n(0);
// 	for (Move* i = list; i<end; ++i) {
// 		if (WorkThread* th = findFreeThread()) {
// 			n.setNotReady();
// 			th->startJob(new PerftJob<(Colors)-C, Result<uint64_t> >(this, n, &b, *i, depth-1));
// 		} else {
// 			n.setNotReady();
// 			perft<(Colors)-C>(&n, &b, *i, depth-1);			
// 		}
// 
// 	}
// 
// 	PerftEntry temp;
// 	temp.zero();
// 	temp.depth |= depth;
// 	temp.upperKey |= z >> temp.upperShift;
// 	temp.value = (uint64_t)n;
// 	pt->store(pe, temp );
// 	result->update(n);
// 	result->setReady();
// }

#endif