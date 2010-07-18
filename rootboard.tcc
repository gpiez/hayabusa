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
#include "nodemodel.h"

template<int dir> uint64_t qMate() {
	return (1ULL << (64+dirOffsets[(dir+3)%8])%64)
		 + (1ULL << (64+dirOffsets[(dir+5)%8])%64)
		 + (1ULL << (64+dirOffsets[(dir+2)%8])%64)
		 + (1ULL << (64+dirOffsets[(dir+6)%8])%64)
		 + (1ULL << (64+dirOffsets[(dir+1)%8])%64)
		 + (1ULL << (64+dirOffsets[(dir+7)%8])%64)
		 ;
}

static const unsigned dMaxCapture = 12;
static const unsigned dMaxThreat = 8;

template<Colors C>
Move RootBoard::rootSearch() {
/*
	std::cout << std::hex << qMate<0>() << std::endl;
	std::cout << std::hex << qMate<2>() << std::endl;
	std::cout << std::hex << qMate<4>() << std::endl;
	std::cout << std::hex << qMate<6>() << std::endl;
*/
#ifdef QT_GUI_LIB
	connect(this,SIGNAL(createModel()), statWidget, SLOT(createModel()));
//	statWidget->createModel();
	emit(createModel());
//	sleep(1);
	while (!statWidget->tree);
#endif
    WorkThread::isMain = true;
    const ColoredBoard<C>& b = currentBoard<C>();//color == White ? &boards[iMove].wb : &boards[iMove].bb;
    store(b.getZobrist());
    stats.node++;
    Move moveList[maxMoves];
    Move* good = moveList+goodMoves;
    Move* bad=good;
    b.generateMoves(good, bad);

    startTime = QDateTime::currentDateTime();
    QDateTime softBudget = startTime.addMSecs( timeBudget/(2*movesToDo) );
    QDateTime hardBudget = startTime.addMSecs( 2*timeBudget / (movesToDo + 1) );

    nMoves = bad-good;
    for (depth=dMaxCapture+dMaxThreat+2; depth<maxDepth; depth++) {
#ifdef QT_GUI_LIB
		NodeData data;
		data.move.data = 0;
		data.ply = depth;
		NodeItem* node = new NodeItem(data, statWidget->tree->root());
#endif
        QDateTime currentStart = QDateTime::currentDateTime();
        SharedScore<C> alpha; alpha.v = -infinity*C;
        SharedScore<(Colors)-C> beta; beta.v = infinity*C;    //both alpha and beta are lower limits, viewed from th color to move
        QTextStream xout(stderr);

        currentMoveIndex = 0;
        for (Move* currentMove = good; currentMove < bad; currentMove++) {
        	currentMoveIndex++;
        	bool pruneNull = false;
        	if (alpha.v != -infinity*C && currentMove != good && depth > dMaxCapture+dMaxThreat+2) {
        		SharedScore<(Colors)-C> null;
        		SharedScore<C> nalpha(alpha);
        		null.v = alpha.v + C;
//                nalpha(alpha);
                if (depth > nullReduction+1 + dMaxThreat + dMaxCapture)
                    search<C, tree>(b, *currentMove, depth-(nullReduction+1), nalpha, null, node);
                else
                    search<C, leaf>(b, *currentMove, depth-(nullReduction+1), nalpha, null, node);
                pruneNull = nalpha >= null.v;
            }
        	if (!pruneNull)
            if (search<(Colors)-C, trunk>(b, *currentMove, depth-1, beta, alpha, node)) {
                bestMove = *currentMove;
                *currentMove = *good;
                *good = bestMove;
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
 *
 * v < alpha reduction ~ P(v < alpha)
 *
 */
/*
 * Search with color C to move, executing a -C colored move m from a board prev.
 * beta is the return value, it is updated at the end
 * A, B can be a SharedScore (updated by other threads) or a Score (thread local)
 */
template<Colors C, Phase P, typename A, typename B, typename T>
bool RootBoard::search(const T& prev, const Move m, const unsigned int depth, const A& alpha, B& beta, NodeItem* parent) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    stats.node++;
    KeyScore estimate;
    estimate.vector = prev.estimatedEval(m, eval);
#ifdef QT_GUI_LIB
	NodeData data = {};
	data.alpha = alpha.v;
	data.beta = beta.v;
	data.move = m;
	data.ply = ply+1;
	data.key = estimate.key;
	data.searchType = P;
	NodeItem* node = stats.node < MAX_NODES && stats.node > MIN_NODES ? new NodeItem(data, parent) : 0;
//	if (node && node->id == 67636) asm("int3");
#endif
/*
    prefetching is not very effective for core2, probably because each access
    is completely random and causes not only a cache miss, but additionally a
    tlb miss, which stalls the processor almost als long as the actual memory
    access would without. It gets slightly more useful, if 2M pages are used.
    Speed increase measured as 1-2% with 4k pages and ~5% witch 2M pages. 2M
    alone increase the speed by ~2%.
*/
    if (P != vein) tt->prefetchSubTable(estimate.key + C+1);

    RawScore& eE = estimatedError[nPieces + C*(m.piece()&7)][m.to()];

    bool threatened = false;
	Move* dummy = NULL;

    if (P==leaf) {
    	threatened = ((fold(prev.doublebits[m.to()] & prev.kingIncoming[CI].d02) && m.piece() & Rook)
    				 || (fold(prev.doublebits[m.to()] & prev.kingIncoming[CI].d13) && m.piece() & Bishop)
    				 || (BoardBase::knightAttacks[m.to()] & prev.template getPieces<-C,King>() && m.piece() == Knight));
    	if (!threatened) {
    		threatened = prev.template generateMateMoves<true>(dummy); //TODO accept only mate threatening moves
    	}
    }

    if (P==vein || (P==leaf && !threatened)) {
		ScoreBase<C> current(alpha);
		current.max(estimate.score.calc(prev, eval)+lastPositionalEval-C*eE);
		if (current >= beta.v) {
			if (node) node->bestEval = beta.v;
			if (node) node->nodeType = NodePrecut;
			stats.leafcut++;    // so neither update beta nor return true
			return false;
		}
    }

    const ColoredBoard<C> b(prev, m, estimate.vector);
	bool threatening = b.template generateMateMoves<true>(dummy);
    ply++;
    ASSERT(ply >= b.fiftyMoves);
    Key z;
    if (P != vein) {
    	z = b.getZobrist();
    	store(z);
    }
	if (WorkThread::isMain) *currentLine++ = m;

    ASSERT(b.keyScore.score.calc(prev, eval) == estimate.score.calc(prev, eval));
    ASSERT(P == vein || z == b.getZobrist());
    if ((int)prev.CI == (int)b.CI) {
        if (b.template inCheck<(Colors)-C>()) {
        	if (WorkThread::isMain) --currentLine;
        	--ply;
			if (node) node->bestEval = beta.v;
        	if (node) node->nodeType = NodeIllegal;
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
    bool betaNode = false;
    bool alphaNode = false;
    if (P != vein) {
    	if (find(b, z)) {
        	if (WorkThread::isMain) --currentLine;
    		--ply;
			if (node) node->bestEval = 0;
    		if (node) node->nodeType = NodeRepetition;
    		return beta.max(0, m);
    	}
        st = tt->getSubTable(z);
        if (tt->retrieve(st, z, subentry)) {
            stats.tthit++;
            ttDepth = subentry.depth;
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
                	if (WorkThread::isMain) --currentLine;
                	--ply;
        			if (node) node->bestEval = beta.v;
                	if (node) node->nodeType = NodeTT;
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

    do {
		Move moveList[256];
		Move* good = moveList+192;
		Move* bad = good;
		if (P==vein || (P==leaf && !threatened)) {
			stats.eval++;
			int realScore = eval.eval(b);
			current.max(realScore);

			static const int minEstimatedError = 10;
			int error = C*(estimate.score.calc(b, eval) + lastPositionalEval - realScore) + minEstimatedError;
			avgE[nPieces + C*(m.piece()&7)][m.to()] += error;
			avgE2[nPieces + C*(m.piece()&7)][m.to()] += error*error;
			avgN[nPieces + C*(m.piece()&7)][m.to()]++;

			if (WorkThread::isMain) {
				if (eE < error) eE = error;
				else eE--;
			}
			lastPositionalEval = realScore - estimate.score.calc(b, eval);
			if (b.template inCheck<C>()) {
				b.generateMoves(good, bad);
				if (bad == good) {
					if (node) node->nodeType = NodeMate;
					if (node) node->bestEval = b.template inCheck<C>() ? -infinity*C:0;
					current.v = b.template inCheck<C>() ? -infinity*C:0;
					break;
				}
				Move* j;
				for (Move* i = j = good; i<bad; ++i)
					if (i->capture())
						*j++ = *i;
				bad = j;
			} else {
				if (current >= beta.v) {  // from the view of beta, current is a bad move,
					stats.leafcut++;    // so neither update beta nor return true
					if (node) node->nodeType = NodePrecut;
					break;

				}
				b.template generateCaptureMoves<false>(good, bad);
				if ( P == leaf ) b.template generateMateMoves<false>(good);
			}
		} else {
			lastPositionalEval = eval.eval(b) - estimate.score.calc(b, eval);
			b.generateMoves(good, bad);
			if (bad == good) {
				if (node) node->nodeType = NodeMate;
				if (node) node->bestEval = b.template inCheck<C>() ? -infinity*C:0;
				current.v = b.template inCheck<C>() ? -infinity*C:0;
				break;
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
		if (ttMove.data && ttMove.fromto() != good[0].fromto()) {
			Move first = good[0];
			for (Move* i = good+1; i<bad; ++i) {
				Move second = *i;
				if (ttMove.fromto() == second.fromto()) {
					good[0] = second;
					break;
				}
				*i = first;
				first = second;
			}
		}

	//    if (!alphaNode)
		for (unsigned int d = (depth+1)%2 + 1 + dMaxCapture + dMaxThreat; d < depth; d+=2) {
			if (d<=ttDepth) continue;
			for (Move* i = good; i<bad; ++i) {
				ASSERT(d>0);
				if (d <= 1 + dMaxCapture + dMaxThreat ? search<(Colors)-C, leaf, B, A>(b, *i, d - 1, beta, current, node) :
	//                d == depth-4 ? search<(Colors)-C, tree, B, A>(b, *i, d-1, B(C*infinity), current) :
								   search<(Colors)-C, tree, B, A>(b, *i, d-1, beta, current, node)) {
					ASSERT(current.m.data == i->data);
					Move first = good[0];
					good[0] = current.m;
					for (Move* j = good+1; j<i; ++j) {
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

		for (Move* i = good; i<bad && current < beta.v; ++i) {
			// Stay in leaf search if it already is or change to it if the next depth would be 0
			if ((P == leaf && !threatening && !threatened) || P == vein || depth <= dMaxCapture + 1)
				search<(Colors)-C, vein>(b, *i, 0, beta.unshared(), current, node);
			else {
	//            if (WorkThread::doStop) return false;
				if (depth <= dMaxCapture + dMaxThreat + 1/*|| (depth <= 2 && abs(b.keyScore.score) >= 400)*/) {
	/*
					if (mm.data)
						search<(Colors)-C, tree>(b, *i, 1, beta.unshared(), current);
					else
	*/
						search<(Colors)-C, leaf>(b, *i, depth-1, beta.unshared(), current, node);
				}
				else { // possible null search in tree or trunk
					bool pruneNull = false;
					if (depth > 2 + dMaxCapture + dMaxThreat && current.v != -infinity*C && (i != good || alphaNode)) {
						typename B::Base null;
						typename A::Base nalpha(current);
						null.v = current.v + C;
	//                nalpha(alpha);
						if (depth > nullReduction+1 + dMaxCapture + dMaxThreat)
							search<C, tree>(b, *i, depth-(nullReduction+1), nalpha, null, node);
						else
							search<C, leaf>(b, *i, depth-(nullReduction+1), nalpha, null, node);
						pruneNull = nalpha >= null.v;
					}
					if (!pruneNull) {
						if (P == tree || A::isNotShared || depth < Options::splitDepth + dMaxCapture + dMaxThreat) {
	/*
							bool research = true;
							if (i > good) {
								typename A::Base nalpha(current);
								research = search<(Colors)-C, tree>(b, *i, depth-2, beta, nalpha);
							}
							if (research) {
	*/
								search<(Colors)-C, tree>(b, *i, depth-1, beta, current, node);
	//                        }
						// Multi threaded search: After the first move try to find a free thread, otherwise do a normal
						// search but stay in trunk. To avoid multithreading search at cut off nodes
						// where it would be useless.
						} else if (depth == Options::splitDepth + dMaxCapture + dMaxThreat) {
							if (i > good && WorkThread::canQueued(z, current.isNotReady())) {
								WorkThread::queueJob(z, new SearchJob<(Colors)-C, typename B::Base, A>(*this, b, *i, depth-1, beta.unshared(), current, node));
							} else {
								search<(Colors)-C, P>(b, *i, depth-1, beta.unshared(), current, node);
							}
						} else {
							if (i > good && WorkThread::canQueued(z, current.isNotReady())) {
								WorkThread::queueJob(z, new SearchJob<(Colors)-C,B,A>(*this, b, *i, depth-1, beta, current, node));
							} else {
								search<(Colors)-C, P>(b, *i, depth-1, beta, current, node);
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
			while((job = WorkThread::getJob(z))) job->job();
	//        while(current.isNotReady() && (job = WorkThread::getChildJob(zd))) job->job();
		}

		current.join();

    } while (false);

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
        tt->store(st, stored);
    }
	if (WorkThread::isMain) --currentLine;
	--ply;
	if (node) node->bestEval = current.v;
    return beta.max(current.v, m);
}

template<Colors C>
uint64_t RootBoard::rootPerft(unsigned int depth) {
    Move moveList[256];
    
    const ColoredBoard<C>& b = currentBoard<C>();
    Move* good = moveList+192;
    Move* bad=good;
    b.generateMoves(good, bad);

    if (depth <= 1)
        return bad-good;

    Result<uint64_t> n(0);
    for (Move* i = good; i<bad; ++i) {
        perft<(Colors)-C, trunk>(n, b, *i, depth-1);
    }
    return n;
}

template<Colors C>
uint64_t RootBoard::rootDivide(unsigned int depth) {
    Move moveList[256];

    const ColoredBoard<C>& b = currentBoard<C>();
    Move* good = moveList+192;
    Move* bad=good;
    b.generateMoves(good, bad);

    uint64_t sum=0;
    for (Move* i = good; i<bad; ++i) {
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
    Move* good = list + 192;
    Move* bad = good;
    b.generateMoves(good, bad);
    if (depth == 1) {
        update(result, bad-good);
        return;
    }
    
    ResultType n(0);
    for (Move* i = good; i<bad; ++i) {
        WorkThread* th;
        if (P == trunk && !!(th = WorkThread::findFree())) {
            th->queueJob(0, new (PerftJob<(Colors)-C, ResultType>)(*this, n, b, *i, depth-1));
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
