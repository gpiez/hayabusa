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

#include <unistd.h>

#include "rootboard.h"
#include "coloredboard.h"
#include "generateMoves.tcc"
#include "coloredboard.tcc"
#include "perft.h"
#include "result.h"
#include "workthread.h"
#include "jobs.h"
#include "score.tcc"
#include "options.h"
#include "testgame.h"

template<Colors C>
Move RootBoard::rootSearch() {
    WorkThread::isMain = true;
    const ColoredBoard<C>& b = currentBoard<C>();//color == White ? &boards[iMove].wb : &boards[iMove].bb;
    stats.node++;
    Move firstMove[nMaxMoves];
    Move* lastMove = b.generateMoves(firstMove);

    startTime = QDateTime::currentDateTime();
    QDateTime softBudget = startTime.addMSecs( timeBudget/(2*movesToDo) );
    QDateTime hardBudget = startTime.addMSecs( 2*timeBudget / (movesToDo + 1) );

    nMoves = lastMove - firstMove;
    for (depth=2; depth<maxDepth; depth++) {
        QDateTime currentStart = QDateTime::currentDateTime();
        SharedScore<C> alpha; alpha.v = -infinity*C;
        SharedScore<(Colors)-C> beta; beta.v = infinity*C;    //both alpha and beta are lower limits, viewed from th color to move
//        SearchFlag f = null;
        QTextStream xout(stderr);

        currentMoveIndex = 0;
        for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
        	currentMoveIndex++;
            //xout << depth << ":" << currentMove->string();
//            Search<2,2,1,2,8,2,2,1,2,8>::search<(Colors)-C, trunk>(tt, eval, b, *currentMove, depth-1, beta, alpha);
        	bool pruneNull = false;
        	if (alpha.v != -infinity*C && currentMove != firstMove) {
        		SharedScore<(Colors)-C> null;
        		SharedScore<C> nalpha(alpha);
        		null.v = alpha.v + C;
//                nalpha(alpha);
                if (depth > 4)
                    search<C, tree>(b, *currentMove, depth-4, nalpha, null);
                else
                    search<C, leaf>(b, *currentMove, 0, nalpha, null);
                pruneNull = nalpha >= null.v;
            }
        	if (!pruneNull)
            if (search<(Colors)-C, trunk>(b, *currentMove, depth-1, beta, alpha)) {
                bestMove = *currentMove;
                *currentMove = *firstMove;
                *firstMove = bestMove;
            }
            std::string newBestLine = tt->bestLine(*this);
            emit console->iterationDone(depth, stats.node, newBestLine, alpha.v);
            //if (QDateTime::currentDateTime() > hardBudget) break;
        }
        std::stringstream g;
        if (Options::humanreadable) g.imbue(std::locale("de_DE"));
        else                        g.imbue(std::locale("C"));
        g << "depth" << std::setw(3) << depth << " time" << std::showpoint << std::setw(13) << getTime() << " nodes"
          << std::setw(18) << getStats().node << " score " << alpha.v << " " << tt->bestLine(*this);
        emit console->send(g.str());
    }
    return bestMove;
}
/*
 * Search with color C to move, executing a -C colored move m from a board prev.
 * beta is the return value, it is updated at the end
 * A, B can be a SharedScore (updated by other threads) or a Score (thread local)
 */

template<Colors C, Phase P, typename A, typename B, typename T>
bool RootBoard::search(const T& prev, const Move m, const unsigned int depth, const A& alpha, B& beta) {
    stats.node++;
    KeyScore estimate;
    estimate.vector = prev.estimatedEval(m, eval);
    RawScore& eE = estimatedError[nPieces + C*(m.piece()&7)][m.to()];
    if (P == leaf || P == vein) {
        ScoreBase<C> current(alpha);
        current.max(estimate.score.calc(prev, eval)-C*eE);
        if (current >= beta.v) {  // from the view of beta, current is a bad move,
            stats.leafcut++;    // so neither update beta nor return true
            return false;
        }
    }
    Key z;
    const ColoredBoard<C> b(prev, m, estimate.vector);
    if (P != vein) {
        z = b.getZobrist();
    }
	if (WorkThread::isMain) *currentLine++ = m;

//    std::string moves;
//    for (Move* i = line; i<currentLine; i++)
//    	moves += i->string() + " ";
//    std::cout << std::setw(7) << alpha.v << std::setw(7) << beta.v << " " << moves << " ### " << stats.node;
//    usleep(100000);

    ASSERT(b.keyScore.score.calc(prev, eval) == estimate.score.calc(prev, eval));
    ASSERT(P == vein || z == b.getZobrist());
    if (prev.CI == b.CI) {
        if (b.template inCheck<(Colors)-C>()) {
        	if (WorkThread::isMain) --currentLine;
//        	std::cout << " illegal" << std::endl;
        	return false;
        }
    } else {
        ASSERT(!b.template inCheck<(Colors)-C>());
    }
    TranspositionTable<TTEntry, transpositionTableAssoc, Key>::SubTable* st;
    TTEntry subentry;

    Move ttMove(0,0,0);
    unsigned int ttDepth = 0;
    A current(alpha);    // current is always the maximum of (alpha, current), a out of thread increased alpha may increase current, but current has no influence on alpha.
//    if (abs(current.v) == 2) asm("int3");
    bool alreadyVisited;
    bool betaNode = false;
    bool alphaNode = false;
    if (P != vein) {
        st = tt->getSubTable(z);
        if (tt->retrieve(st, z, subentry, alreadyVisited)) {
            if (WorkThread::isMain && !alreadyVisited) tt->mark(st);
            stats.tthit++;
            ttDepth = subentry.depth;
//            if (P == trunk && subentry.score == 0) ttDepth=0; // TODO workaround, fix this
            if (ttDepth >= depth) {

                if (subentry.loBound) {
                    stats.ttalpha++;
                    current.max(subentry.score);
                }
                if (subentry.hiBound) {
                    stats.ttbeta++;
                    beta.max(subentry.score, m);
                }
                if (current >= beta.v) {
                    if (WorkThread::isMain && !alreadyVisited) tt->unmark(st);
                	if (WorkThread::isMain) --currentLine;
                	//std::cout << " beta" << std::endl;
                    return false;
                }
            } else {
                ScoreBase<C> ttScore;
                ttScore.v = subentry.score;
                if (subentry.loBound && ttScore >= beta.v)
                    betaNode = true;
                if (subentry.hiBound && ttScore < alpha.v)
                    alphaNode = true;
            }
            
            ttMove = Move(subentry.from, subentry.to, 0);
        }
    }

    Move list[256];
    Move* end;
    if (P == leaf || P == vein) {
        stats.eval++;
        int realScore = eval.eval(b);
        current.max(realScore);
//        if (abs(current.v) == 2) asm("int3");

        static const int minEstimatedError = 100;
        int error = C*(estimate.score.calc(b, eval) - realScore) + minEstimatedError;
        if (WorkThread::isMain) {
            if (eE < error) eE = error;
            else eE--;
        }
        if (current >= beta.v) {  // from the view of beta, current is a bad move,
            stats.leafcut++;    // so neither update beta nor return true
            if (P!=vein)
            	if (WorkThread::isMain && !alreadyVisited) tt->unmark(st);
        	if (WorkThread::isMain) --currentLine;
            return false;

        }
        // if (P==leaf) std::cout << " leaf " << current.v << std::endl;
        // else std::cout << " vein " << current.v << std::endl;
        if (b.template inCheck<C>()) {
            end = b.generateMoves(list);
			if (end == list) {
				if (WorkThread::isMain) --currentLine;
	        	// std::cout << " mate" << std::endl;
				return beta.max(-infinity*C, m);
			}
            Move* j;
            for (Move* i = j = list; i<end; ++i)
                if (i->capture())
                    *j++ = *i;
            end = j;
        } else {
            Move* badStart = list + 192;
            Move* bad = badStart;
            end = b.template generateCaptureMoves<false>(list, bad);
        	for (Move* i=badStart; i<bad; ++i)
        	  	*end++ = *i;
        }
    } else {
        end = b.generateMoves(list);
        if (end == list) {
        	if (WorkThread::isMain) --currentLine;
        	return beta.max(b.template inCheck<C>() ? -infinity*C:0, m);
        	// std::cout << " stale/mate" << std::endl;
        }
    }

//    TestGame::recordBoard(C, b);

/*
    for (Move* i = list; i<end; ++i) {
        if(currentLine->fromto() == i->fromto()) {
        	Move first = list[0];
        	list[0] = *i;
            for (Move* j = list+1; j<i; ++j) {
                Move second = *j;
                *j = first;
                first = second;
            }
            *i = first;
        }
    }
*/

    // move best move from tt / history to front
    if (ttMove.data && ttMove.fromto() != list[0].fromto()) {
    	Move first = list[0];
        for (Move* i = list+1; i<end; ++i) {
            Move second = *i;
            if (ttMove.fromto() == second.fromto()) {
                list[0] = second;
                break;
            }
            *i = first;
            first = second;
        }
    }

    if (0 && !alphaNode)
    for (unsigned int d = (depth+1)%2 + 1; d < depth; d+=2) {
    	if (d<=ttDepth) continue;
        for (Move* i = list; i<end; ++i) {
            ASSERT(d>0);
            if (d == 1 ? search<(Colors)-C, leaf, B, A>(b, *i, 0, beta, current)
                : search<(Colors)-C, tree, B, A>(b, *i, d-1, beta, current)) {
                ASSERT(current.m.data == i->data);
            	Move first = list[0];
            	list[0] = current.m;
                for (Move* j = list+1; j<i; ++j) {
                    Move second = *j;
                    *j = first;
                    first = second;
                }
                *i = first;
            }
            if (current >= beta.v) {
                if (d+2 >= depth)
                    betaNode = true;
                break;
            }
        }
        current.v = alpha.v;
    }

//  if (depth)  std::cout << " search " << depth << std::endl;
    Key zd;
    if (P == trunk)
        zd = (z &0xffffffffffff) + ((uint64_t)depth << 56);
    
    for (Move* i = list; i<end && current < beta.v; ++i) {
        // Stay in leaf search if it already is or change to it if the next depth would be 0
        if (P == leaf || P == vein)
            search<(Colors)-C, vein>(b, *i, 0, (typename B::Base&)beta, current);
        else {
//            if (WorkThread::doStop) return false;
            if (depth <= 1 /*|| (depth <= 2 && abs(b.keyScore.score) >= 400)*/)
                search<(Colors)-C, leaf>(b, *i, 0, (typename B::Base&)beta, current);
            else { // possible null search in tree or trunk
            	bool pruneNull = false;
            	if (depth > 1 && current.v != -infinity*C && (i != list || alphaNode)) {
            		typename B::Base null;
            		typename A::Base nalpha(current);
            		null.v = current.v + C;
//                nalpha(alpha);
                    if (depth > 4)
                        search<C, tree>(b, *i, depth-4, nalpha, null);
                    else
                        search<C, leaf>(b, *i, 0, nalpha, null);
                    pruneNull = nalpha >= null.v;
                }
            	if (!pruneNull) {
                    if (P == tree || A::isNotShared || depth < Options::splitDepth) {
            /*                if (i != list)
                                search<(Colors)-C, tree>(b, *i, depth-1, null, current);
                            if (null.v != current.v)*/
                                search<(Colors)-C, tree>(b, *i, depth-1, (typename B::Base&)beta, current);
                    // Multi threaded search: After the first move try to find a free thread, otherwise do a normal
                    // search but stay in trunk. To avoid multithreading search at cut off nodes
                    // where it would be useless.
                    } else if (depth == Options::splitDepth){
                        if (i > list && current.isNotReady() < 2*WorkThread::nThreads-1 && WorkThread::canQueued(zd)) {
                            if (current.isNotReady() >= 2*WorkThread::nThreads) {
                                std::cout << zd << std::endl;
                                WorkThread::printJobs();
                                ASSERT(0);
                            }
                            WorkThread::queueJob(zd, new SearchJob<(Colors)-C, typename B::Base, A>(*this, b, *i, depth-1, (typename B::Base&)beta, current));
                        } else {
                            search<(Colors)-C, P>(b, *i, depth-1, (typename B::Base&)beta, current);
                        }
                    } else {
                        if (i > list && current.isNotReady() < 2*WorkThread::nThreads-1 && WorkThread::canQueued(zd)) {
                            if (current.isNotReady() >= 2*WorkThread::nThreads) {
                                std::cout << zd << std::endl;
                                WorkThread::printJobs();
                                ASSERT(0);
                            }
                            WorkThread::queueJob(zd, new SearchJob<(Colors)-C, B, A>(*this, b, *i, depth-1, beta, current));
                        } else {
                            search<(Colors)-C, P>(b, *i, depth-1, beta, current);
                        }
                    }
                }
            }
        }
    }

    if (current >= beta.v)
    	*currentLine = current.m;
    // if there are queued jobs started from _this_ node, do them first
    if (P == trunk) {
        Job* job;
        while((job = WorkThread::getJob(zd))) job->job();
//        while(current.isNotReady() && (job = WorkThread::getChildJob(zd))) job->job();
    }

    current.join();
    if (P != vein) {
        TTEntry stored;
        stored.zero();
        stored.depth |= depth;
        stored.upperKey |= z >> stored.upperShift;
        stored.score |= current.v;
        stored.loBound |= current > alpha.v;
        stored.hiBound |= current < beta.v;
        stored.from |= current.m.from();
        stored.to |= current.m.to();
        if (!alreadyVisited && WorkThread::isMain) tt->unmark(st);
        if (!alreadyVisited)    // don't overwerite current variant
            tt->store(st, stored);
    }
//    std::cout << std::setw(7) << alpha.v << std::setw(7) << beta.v << " " << moves;
    bool newBM = beta.max(current.v, m);
//    std::cout << " return " << current.v << std::endl;
	if (WorkThread::isMain) --currentLine;
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

    uint64_t sum=0;
    for (Move* i = list; i<end; ++i) {
        Result<uint64_t> n(0);
        if (depth == 1) {
            n.update(1);
        } else {
            perft<(Colors)-C, trunk>(n, b, *i, depth-1);
        }
        std::cout << i->string() << " " << (uint64_t)n << std::endl;
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
    if (P == trunk && depth <= Options::splitDepth) {
        uint64_t n=0;
        perft<C, tree>(n, prev, m, depth);
        update(result, n);
        return;
    }

    __v8hi est = prev.estimatedEval(m, eval);
    const ColoredBoard<C> b(prev, m, est);

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
