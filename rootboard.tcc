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
#include "score.tcc"

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
		SharedScore<C> alpha(-infinity);
		SharedScore<(Colors)-C> beta(-infinity);	//both alpha and beta are lower limits, viewed from th color to move
//		SearchFlag f = null;
		QTextStream xout(stderr);
		
		for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
			//xout << depth << ":" << currentMove->string();
			if (search<(Colors)-C, trunk>(b, *currentMove, depth-1, beta, alpha)) {
				bestMove = *currentMove;
				*currentMove = *firstMove;
				*firstMove = bestMove;
			}
			QString newBestLine = tt->bestLine(*this);
			emit console->iterationDone(depth, stats.node, newBestLine, alpha.v);
			//if (QDateTime::currentDateTime() > hardBudget) break;
		}
		emit console->send(QString("depth %1 time %2 nodes %3 score %4 pv %5")
		.arg(depth).arg(getTime()).arg(getStats().node).arg(alpha.v).arg(tt->bestLine(*this)));
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
	KeyScore estimate ALIGN_XMM;
	estimate.vector = prev.estimatedEval(m, *this);	//TODO in leaf phase use a estimate function which calculates only the score
	if (P == leaf || P == vein) {									//TODO in the very fist leaf iteration, use tt?
		A current(alpha);
		current.max(estimate.score-Score<C>(125));
		if (current >= beta) {
			stats.leafcut++;
			return false;
		}
	}
	if (P != vein)										//TODO use the precalculated key in the board constructor
		tt->prefetchSubTable(estimate.key + prev.cep.data8 + (C+1));
	const ColoredBoard<C> b(prev, m, *this);
	ASSERT(b.keyScore.score == estimate.score);
	
	Key z = b.getZobrist();
	TranspositionTable<TTEntry, transpositionTableAssoc, Key>::SubTable* st;
	TTEntry subentry;

	Move ttMove = {{0}};
	unsigned int ttDepth = 0;
	A current(alpha);	// current is always the maximum of (alpha, current), a out of thread increased alpha may increase current, but current has no influence on alpha.
	bool alreadyVisited;
	if (P == tree || P == trunk || P == leaf) {
		st = tt->getSubTable(z);
		if (tt->retrieve(st, z, subentry, alreadyVisited)) {
			if ( P == trunk)
				tt->mark(st);
			stats.tthit++;
			ttDepth = subentry.depth;
			if (P == trunk && subentry.score == 0) ttDepth=0;
			if (ttDepth >= depth) {

				if (subentry.loBound) {
					stats.ttalpha++;
					current.max(subentry.score);
				}
				if (subentry.hiBound) {
					stats.ttbeta++;
					beta.max(subentry.score, m);
				}
				if (current >= beta) {
					if (P == trunk && !alreadyVisited)
						tt->unmark(st);
					return false;
				}
			}
			ttMove.from = subentry.from;
			ttMove.to = subentry.to;
		}
	}

	Move list[256];
	Move* end;
	if ((P == leaf || P == vein) && (b.template attacks<(C-Black)/2>(b.pieceList[(White-C)/2].getKing()) & attackMask) == 0) {
		stats.eval++;
		current.max(eval(b));
		end = b.generateCaptureMoves(list);
	} else
		end = b.generateMoves(list);

	// move best move from tt / history to front
	if (ttMove.data)
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
	for (unsigned int d = ((ttDepth+2)&~1) + (depth&1); d < depth; d+=2) {
		for (Move* i = list; i<end; ++i) {
			search<(Colors)-C, tree, B, A>(b, *i, d-1, beta, current);
			if (current.m.data == i->data) {
				*i = *list;
				*list = current.m;
			}
			if (current >= beta)
				break;
		}
		current.v = alpha.v;
	}
	for (Move* i = list; i<end && current < beta; ++i) {
		// Stay in leaf search if it already is or change to it if the next depth would be 0
		if (P == leaf || P == vein)
			search<(Colors)-C, vein>(b, *i, 0, (typename B::Base&)beta, current);
		else if (depth <= 1) 
			search<(Colors)-C, leaf>(b, *i, 0, (typename B::Base&)beta, current);
		// Stay in tree search or change from trunk to it if the next depth would
		// be below the split depth
		else if (P == tree || (P == trunk && depth <= splitDepth)) {
			search<(Colors)-C, tree>(b, *i, depth-1, (typename B::Base&)beta, current);
		// Multi threaded search: After the first move try to find a free thread, otherwise do a normal
		// search but stay in trunk. To avoid multithreading search at cut off nodes
		// where it would be useless.
		} else {
			if (i > list && WorkThread::canQueued()) {
				WorkThread::queueJob(z, new SearchJob<(Colors)-C, B, A>(*this, b, *i, depth-1, beta, current));
			} else {
				search<(Colors)-C, P>(b, *i, depth-1, beta, current);
			}
		}
	}

	// if there are queued jobs started from _this_ node, do them first
	if (P == trunk) while(Job* job = WorkThread::getJob(z)) job->job();

	current.join();
	if (P == tree || P == trunk || P == leaf) {
		TTEntry stored;
		stored.zero();
		stored.depth |= depth;
		stored.upperKey |= z >> stored.upperShift;
		stored.score |= current.v;
		stored.loBound |= current > alpha;
		stored.hiBound |= current < beta;
		stored.from |= current.m.from;
		stored.to |= current.m.to;
		if (P == trunk && !alreadyVisited)
			tt->unmark(st);
		if (!alreadyVisited && current.v)	// don't overwerite current variant
			tt->store(st, stored);
	}
	bool newBM = beta.max(current, m);
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
			perft<(Colors)-C, trunk>(n, b, *i, depth-1);
		}
		xout << i->string() << " " << (uint64_t)n << endl;
		sum += n;
	}
	return sum;
}

void update(Result<uint64_t>& r, uint64_t v);
void update(uint64_t& r, uint64_t v);
void inline setReady(uint64_t ) {};
void inline setNotReady(uint64_t ) {};
void inline setReady(Result<uint64_t>& r) {
	r.setReady();
}

void inline setNotReady(Result<uint64_t>& r) {
	r.setNotReady();
}

template<Colors C, Phase P, typename ResultType> void RootBoard::perft(ResultType& result, const ColoredBoard<(Colors)-C>& prev, Move m, unsigned int depth) {
	if (P == trunk && depth <= splitDepth) {
		uint64_t n=0;
		perft<C, tree>(n, prev, m, depth);
		update(result, n);
		return;
	}

	const ColoredBoard<C> b(prev, m, *this);

	Key z = b.getZobrist();
	TranspositionTable<PerftEntry, 1, Key>::SubTable* pe = pt->getSubTable(z);
	PerftEntry subentry;

	if (pt->retrieve(pe, z, subentry) && subentry.depth == depth) {
		update(result, subentry.value);
		return;
	}
	Move list[256];
	Move* end = b.generateMoves(list);
	if (depth == 1) {
		update(result, end-list);
		return;
	}
	
	ResultType n(0);
	for (Move* i = list; i<end; ++i) {
		WorkThread* th;
		if (P == trunk && !!(th = WorkThread::findFree())) {
			th->startJob(new (PerftJob<(Colors)-C, ResultType>)(*this, n, b, *i, depth-1));
		} else {
			perft<(Colors)-C, P>(n, b, *i, depth-1);
		}

	}

	PerftEntry stored;
	stored.zero();
	stored.depth |= depth;
	stored.upperKey |= z >> stored.upperShift;
	stored.value = (uint64_t)n;
	pt->store(pe, stored);
	update(result, n);
}

#endif