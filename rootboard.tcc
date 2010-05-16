/*
    hayabusa, chess engine
    Copyright (C) 2009-2010 Gunther Piez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
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

template<Colors C>
Move RootBoard::rootSearch() {
	const ColoredBoard<C>& b = currentBoard<C>();//color == White ? &boards[iMove].wb : &boards[iMove].bb;
	stats.node++;
	Move firstMove[nMaxMoves];
	Move* lastMove = b.generateMoves(firstMove);

	startTime = QDateTime::currentDateTime();
	QDateTime softBudget = startTime.addMSecs( timeBudget/(2*movesToDo) );
	QDateTime hardBudget = startTime.addMSecs( 2*timeBudget / (movesToDo + 1) );

	for (depth=2; depth<maxDepth; depth++) {
		QDateTime currentStart = QDateTime::currentDateTime();
		Score<C> alpha(-infinity);
		alpha.m = (Move) {0};
		Score<(Colors)-C> beta(-infinity);	//both alpha and beta are lower limits, viewed from th color to move
		beta.m = (Move) {0};
//		SearchFlag f = null;
		QTextStream xout(stderr);
		Score<C> bestScore = alpha;
		
		for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
			//xout << depth << ":" << currentMove->string();
			if (search<(Colors)-C, trunk>(b, *currentMove, depth-1, beta, bestScore)) {
				bestMove = *currentMove;
				*currentMove = *firstMove;
				*firstMove = bestMove;
			}
			QString newBestLine = tt->bestLine(*this);
			emit console->iterationDone(depth, stats.node, newBestLine, bestScore.get());
			//if (QDateTime::currentDateTime() > hardBudget) break;
		}
		emit console->send(QString("depth %1 time %2 nodes %3 score %4 pv %5")
		.arg(depth).arg(getTime()).arg(stats.node).arg(bestScore.get()).arg(tt->bestLine(*this)));
		// assume geometrically increasing time per depth
		//if (QDateTime::currentDateTime() > softBudget) break;
	}

	return bestMove;
}
/*
 * Search with color C to move, executing a -C colored move m from a board prev.
 * beta is the return value, it is updated at the end
 * A, B can be a SharedScore (updated by other threads) or a Score (thread local)
 */
template<Colors C, Phase P, typename A, typename B>
bool RootBoard::search(const ColoredBoard<(Colors)-C>& prev, Move m, unsigned int depth, const A& alpha, B& beta) {
	stats.node++;
	if (P == leaf) {
		A current(alpha);
		current.max(prev.estimatedEval(m, *this), (Move){0});
		if (current >= beta) {
			stats.leafcut++;
			return false;
		}
	}
	const ColoredBoard<C> b(prev, m, *this);
	ASSERT(b.keyScore.score == prev.estimatedEval(m, *this));
	
	Key z = b.getZobrist();
	QReadWriteLock* l;
	TTEntry* te;
	if (P != leaf) te = tt->getEntry(z, l);
	TTEntry subentry;

	Move ttMove = {0};
	unsigned int ttDepth = 0;
	A current(alpha);	// current is always the maximum of (alpha, current), a out of thread increased alpha may increase current, but current has no influence on alpha.
	current.m = (Move) {0};
	if (P != leaf && tt->retrieve(te, z, subentry, l) ) {
		stats.tthit++;
		ttDepth = subentry.depth;
		if (subentry.depth >= depth) {
			ASSERT( subentry.loBound || subentry.hiBound );
			if (subentry.loBound) {
				stats.ttalpha++;
				current.max(subentry.score, (Move){0});
			}
			if (subentry.hiBound) {
				stats.ttbeta++;
				beta.max(subentry.score, m);
			}
			if (current >= beta) {
				beta.setReady();
				return false;
			}
		}
		ttMove.from = subentry.from;
		ttMove.to = subentry.to;
	}

	Move list[256];
	Move* end;
	if (P == leaf) {
		stats.eval++;
		current.max(eval(b), (Move){0});
		end = b.generateCaptureMoves(list);
	} else
		end = b.generateMoves(list);

	// move best move from tt / history to front
	if ((uint32_t&)ttMove)
		for (Move* i = list; i<end; ++i) 
			if ((ttMove.from == i->from) & (ttMove.to == i->to)) {
				ttMove = *i;
				*i = *list;
				*list = ttMove;
				break;
			}

/*	QTextStream xout(stderr);
	QString indentation;
	for (unsigned int i = depth; i<10; i++)
		indentation += "  ";*/
//	xout << indentation << alpha.get() << "," << beta.get();
	for (unsigned int d = ttDepth+1; d < depth; ++d) {
		for (Move* i = list; i<end; ++i) {
			search<(Colors)-C, tree, B, A>(b, *i, d-1, beta, current);
			if ((uint32_t&)current.m == (uint32_t&)*i) {
				*i = *list;
				*list = current.m;
			}
			if (current >= beta)
				break;
		}
		current.v = alpha.v;
	}
	for (Move* i = list; i<end; ++i) {
		if (current >= beta)
			break;
		if (P == leaf || depth <= 1) {
			// The leaf search always starts with the piece square value and
			// will return a value worse
//			if (current < b.estimatedEval(m, *this))
				search<(Colors)-C, leaf, B, A>(b, *i, 0, beta, current);
			
		} else if (P == tree || (P == trunk && depth <= splitDepth)) {
			search<(Colors)-C, tree, B, A>(b, *i, depth-1, beta, current);
		} else {
			WorkThread* th;
			if (0 && (th = findFreeThread())) {
				setNotReady(current);
				th->startJob(new SearchJob<(Colors)-C, B, A>(*this, b, *i, depth-1, beta, current));
			} else {
				setNotReady(current);
				search<(Colors)-C, P>(b, *i, depth-1, beta, current);
			}
		}
	}

	if (P != leaf) {
		TTEntry stored;
		stored.zero();
		stored.depth |= depth;
		stored.upperKey |= z >> stored.upperShift;
		stored.score |= current.get();
		stored.loBound |= current > alpha;
		stored.hiBound |= current < beta;
		ASSERT( stored.loBound || stored.hiBound );
		stored.from |= current.m.from;
		stored.to |= current.m.to;
		tt->store(te, stored, l);
	}
	bool newBM = beta.max(current, m);
	beta.setReady();
	return newBM;
}

template<Colors C>
uint64_t RootBoard::rootPerft(unsigned int depth) {
	Move list[256];
	
	const ColoredBoard<C>& b = currentBoard<C>();
	Move* end = b.generateMoves(list);

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

	const ColoredBoard<C>& b = currentBoard<C>();
	Move* end = b.generateMoves(list);

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

template<Colors C, Phase P, typename ResultType> void RootBoard::perft(ResultType& result, const ColoredBoard<(Colors)-C>& prev, Move m, unsigned int depth) {
	if (P == trunk && depth <= splitDepth) {
		uint64_t n=0;
		perft<C, tree>(n, prev, m, depth);
		updateAndReady(result, n);
		return;
	}

	const ColoredBoard<C> b(prev, m, *this);

	Key z = b.getZobrist();
	QReadWriteLock* l;
	PerftEntry* pe = pt->getEntry(z, l);
	PerftEntry subentry;
	
	if (pt->retrieve(pe, z, subentry, l) && subentry.depth == depth) {
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
			th->startJob(new (PerftJob<(Colors)-C, ResultType>)(*this, n, b, *i, depth-1));
		} else {
			setNotReady(n);
			perft<(Colors)-C, P>(n, b, *i, depth-1);
		}

	}

	PerftEntry stored;
	stored.zero();
	stored.depth |= depth;
	stored.upperKey |= z >> stored.upperShift;
	stored.value = (uint64_t)n;
	pt->store(pe, stored, l);
	updateAndReady(result, n);
}

#endif