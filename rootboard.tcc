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
#include "search.tcc"
#include "options.h"

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

    for (depth=2; depth<maxDepth; depth++) {
        QDateTime currentStart = QDateTime::currentDateTime();
        SharedScore<C> alpha; alpha.v = -infinity*C;
        SharedScore<(Colors)-C> beta; beta.v = infinity*C;    //both alpha and beta are lower limits, viewed from th color to move
//        SearchFlag f = null;
        QTextStream xout(stderr);
    
        for (Move* currentMove = firstMove; currentMove < lastMove; currentMove++) {
            //xout << depth << ":" << currentMove->string();
//            Search<2,2,1,2,8,2,2,1,2,8>::search<(Colors)-C, trunk>(tt, eval, b, *currentMove, depth-1, beta, alpha);
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
bool RootBoard::search(const T& prev, Move m, unsigned int depth, const A& alpha, B& beta) {
    stats.node++;
    KeyScore estimate ALIGN_XMM;
    uint64_t nextcep;
    estimate.vector = prev.estimatedEval(m, eval, nextcep);
    RawScore& eE = estimatedError[nPieces + prev.pieces[m.from]][m.to];
    if (P == leaf || P == vein) {                          
        ScoreBase<C> current(alpha);
        current.max(estimate.score-C*eE);
        if (current >= beta.v) {  // from the view of beta, current is a bad move,
            stats.leafcut++;    // so neither update beta nor return true
            return false;
        }
    }
    Key z;
    if (P != vein) {
        z = estimate.key + nextcep + (C+1);
        tt->prefetchSubTable(z);
    }
    const ColoredBoard<C> b(prev, m, estimate.vector, nextcep);
	if (WorkThread::isMain) *currentLine++ = m;

//    std::string moves;
//    for (Move* i = line; i<currentLine; i++)
//    	moves += i->string() + " ";
//    std::cout << std::setw(7) << alpha.v << std::setw(7) << beta.v << " " << moves << " ### " << stats.node;
//    usleep(100000);

    ASSERT(b.keyScore.score == estimate.score);
    ASSERT(P == vein || z == b.getZobrist());
    if (prev.CI == b.CI) {
        if (b.template attacks<(1-C)/2>(b.pieceList[(1+C)/2].getKing()) & attackMask) {
        	if (WorkThread::isMain) --currentLine;
//        	std::cout << " illegal" << std::endl;
        	return false;
        }
    } else {
        ASSERT((b.template attacks<(1-C)/2>(b.pieceList[(1+C)/2].getKing()) & attackMask) == 0);
    }
    TranspositionTable<TTEntry, transpositionTableAssoc, Key>::SubTable* st;
    TTEntry subentry;

    Move ttMove = {{0}};
    unsigned int ttDepth = 0;
    A current(alpha);    // current is always the maximum of (alpha, current), a out of thread increased alpha may increase current, but current has no influence on alpha.
    bool alreadyVisited;
    bool betaNode = false;
    if (P != vein) {
        st = tt->getSubTable(z);
        if (tt->retrieve(st, z, subentry, alreadyVisited)) {
            if ( P == trunk)
                tt->mark(st);
            stats.tthit++;
            ttDepth = subentry.depth;
            if (P == trunk && subentry.score == 0) ttDepth=0; // TODO workaround, fix this
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
                    if (P == trunk && !alreadyVisited)
                        tt->unmark(st);
                	if (WorkThread::isMain) --currentLine;
                	//std::cout << " beta" << std::endl;
                    return false;
                }
            } else {
                if (subentry.loBound && current < (RawScore)subentry.score)
                    betaNode = true;
            }
            ttMove.from = subentry.from;
            ttMove.to = subentry.to;
        }
    }

    Move list[256];
    Move* end;
    if ((P == leaf || P == vein) /*&& (b.template attacks<(C-Black)/2>(b.pieceList[(White-C)/2].getKing()) & attackMask) == 0*/) {
        stats.eval++;
        RawScore realScore = eval.eval(b);
        current.max(realScore);
        RawScore error = C*(estimate.score - realScore);
        if (WorkThread::isMain) {
            if (eE < error) eE = error;
            else eE--;
        }
        // if (P==leaf) std::cout << " leaf " << current.v << std::endl;
        // else std::cout << " vein " << current.v << std::endl;
        if (b.template attacks<(1+C)/2>(b.pieceList[(1-C)/2].getKing()) & attackMask) {
            end = b.generateMoves(list);
			if (end == list) {
				if (WorkThread::isMain) --currentLine;
	        	// std::cout << " mate" << std::endl;
				return beta.max(-infinity*C, m);
			}
            Move* j;
            for (Move* i = j = list; i<end; ++i)
                if (b.pieces[i->to])
                    *j++ = *i;
            end = j;
        } else
            end = b.generateCaptureMoves(list);
    } else {
        end = b.generateMoves(list);
        if (end == list) {
        	if (WorkThread::isMain) --currentLine;
        	return beta.max(
        	b.template attacks<(1+C)/2>(b.pieceList[(1-C)/2].getKing()) & attackMask ?
        		-infinity*C : 0, m);
        	// std::cout << " stale/mate" << std::endl;
        }
    }

//    // move best move from tt / history to front
    if (ttMove.data)
        for (Move* i = list; i<end; ++i)
            if ((ttMove.from == i->from) & (ttMove.to == i->to)) {
                ttMove = *i;
                *i = *list;
                *list = ttMove;
                break;
            }

    for (unsigned int d = ((ttDepth+1)&~1) + (~depth&1) + 1; d < depth; d+=2) {
        for (Move* i = list; i<end; ++i) {
            ASSERT(d>0);
            if (d == 1 ? search<(Colors)-C, leaf, B, A>(b, *i, 0, beta, current)
                : search<(Colors)-C, tree, B, A>(b, *i, d-1, beta, current)) {
                ASSERT(current.m.data == i->data);
                *i = *list;
                *list = current.m;
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
    for (Move* i = list; i<end && current < beta.v; ++i) {
        // Stay in leaf search if it already is or change to it if the next depth would be 0
        if (P == leaf || P == vein)
            search<(Colors)-C, vein>(b, *i, 0, (typename B::Base&)beta, current);
        else if (depth <= 1)
            search<(Colors)-C, leaf>(b, *i, 0, (typename B::Base&)beta, current);
        else { // possible null search in tree or trunk
            typename B::Base null;
            null.v = current.v + C;
            if (depth > 1 && current.v != -infinity*C && (i != list || !betaNode)) {
                if (depth > 4)
                    search<C, tree>(b, *i, depth-4, current, null);
                else
                    search<C, leaf>(b, *i, 0, current, null);
            }
            if (current < null.v) {
                if (P == tree || A::isNotShared || depth < Options::splitDepth) {
        /*                if (i != list)
                            search<(Colors)-C, tree>(b, *i, depth-1, null, current);
                        if (null.v != current.v)*/
                            search<(Colors)-C, tree>(b, *i, depth-1, (typename B::Base&)beta, current);
                // Multi threaded search: After the first move try to find a free thread, otherwise do a normal
                // search but stay in trunk. To avoid multithreading search at cut off nodes
                // where it would be useless.
                } else if (depth == Options::splitDepth){
                    if (i > list && WorkThread::canQueued()) {
                        WorkThread::queueJob(z, new SearchJob<(Colors)-C, typename B::Base, A>(*this, b, *i, depth-1, (typename B::Base&)beta, current));
                    } else {
                        search<(Colors)-C, P>(b, *i, depth-1, (typename B::Base&)beta, current);
                    }
                } else {
                    if (i > list && WorkThread::canQueued()) {
                        WorkThread::queueJob(z, new SearchJob<(Colors)-C, B, A>(*this, b, *i, depth-1, beta, current));
                    } else {
                        search<(Colors)-C, P>(b, *i, depth-1, beta, current);
                    }
                }
            }
        }
    }

    // if there are queued jobs started from _this_ node, do them first
    if (P == trunk) while(Job* job = WorkThread::getJob(z)) job->job();

    current.join();
    if (P != vein) {
        TTEntry stored;
        stored.zero();
        stored.depth |= depth;
        stored.upperKey |= z >> stored.upperShift;
        stored.score |= current.v;
        stored.loBound |= current > alpha.v;
        stored.hiBound |= current < beta.v;
        stored.from |= current.m.from;
        stored.to |= current.m.to;
        if (P == trunk && !alreadyVisited)
            tt->unmark(st);
        if (!alreadyVisited && current.v)    // don't overwerite current variant
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

    const ColoredBoard<C> b(prev, m, eval);

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
