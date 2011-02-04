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

    Current Node            Next Node
    check               ->  threatened mate/rep/material loss   -> no stand pat
    mate move           ->  threatened mate/repetition          -> no stand pat
    fork                ->  threatened material loss            -> no stand pat
    singular evasion    ->  threatening mate/rep                -> generate checks
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
#include "transpositiontable.tcc"
#include "history.tcc"
#include "repetition.tcc"
#include "nodeitem.h"
#include "eval.tcc"
#include "movelist.h"

#ifdef QT_GUI_LIB
#define NODE ,node
#else
#define NODE
#endif

template<Colors C>
Move RootBoard::rootSearch(unsigned int endDepth) {
    start = system_clock::now();
    lastStatus = start + milliseconds(1000);
    lastStatus2 = start;
#ifdef QT_GUI_LIB
    while (!statWidget->tree);
    statWidget->emptyTree();
#endif
#ifdef MYDEBUG        
    eval.bmob1 = eval.bmob2 = eval.bmob3 = eval.bmobn = 0;
#endif        
    isMain = true;
    const ColoredBoard<C>& b = currentBoard<C>();
    const unsigned ply=0;
    store(b.getZobrist(), ply);
    stats.node = 1;
    MoveList ml(b);

    milliseconds time( C==White ? wtime : btime );
    milliseconds inc( C==White ? winc : binc );
    system_clock::time_point softBudget;
    if (movestogo == 0)
        movestogo = 40;
    time -= operatorTime;
    time = std::max(minimumTime, time);
    
    softBudget = start + (time + inc*movestogo)/(2*movestogo);
    hardBudget = start + std::min(time/2, (2*time + 2*inc*movestogo)/(movestogo+1));

    SharedScore<C> alpha0; alpha0.v = -infinity*C;
    nMoves = ml.count();
    for (depth=dMaxCapture+dMaxThreat+2; depth<endDepth+dMaxCapture+dMaxThreat && stats.node < maxSearchNodes && !stopSearch; depth++) {
        ml.begin();
        bestMove = *ml;
        system_clock::time_point now = system_clock::now();
        if (now >= lastStatus2) {
            lastStatus2 = now + milliseconds(1000);
            console->send("info currmove " + bestMove.algebraic() + " currmovenumber 1");
        }
#ifdef QT_GUI_LIB
        NodeData data;
        data.move.data = 0;
        data.ply = depth;
        NodeItem* node=0;
        if (NodeItem::nNodes < MAX_NODES && NodeItem::nNodes >= MIN_NODES) {
            NodeItem::m.lock();
            node = new NodeItem(data, statWidget->tree->root());
            NodeItem::nNodes++;
            NodeItem::m.unlock();
        }
#endif

        currentMoveIndex = 1;
        SharedScore<        C>  alpha; alpha.v = -infinity*C;
        SharedScore<(Colors)-C> beta;  beta.v  =  infinity*C;    //both alpha and beta are lower limits, viewed from th color to move
        SharedScore<         C> alpha2;
        SharedScore<(Colors)-C> beta2 ;
        if (alpha0.v != -infinity*C) {
            alpha2.v = alpha0.v - Eval::parameters.aspiration0*C;
            beta2.v  = alpha0.v + Eval::parameters.aspiration1*C;
            if (!search<(Colors)-C, trunk>(NodePV, b, *ml, depth-1, beta2, alpha2, ply+1, false NODE)) {
                // Fail-Low at first root, research with low bound = alpha and high bound = old low bound
                now = system_clock::now();
                console->send(status(now, alpha2.v) + " upperbound");
                if (!infinite && now > hardBudget) break;   //TODO check if trying other moves first is an better option
                beta2.v = alpha2.v;
                search<(Colors)-C, trunk>(NodePV, b, *ml, depth-1, beta2, alpha, ply+1, false NODE);
            } else if (alpha2 >= beta2.v) {
                // Fail-High at first root, research with high bound = beta and low bound = old high bound
                now = system_clock::now();
                console->send(status(now, alpha2.v) + " lowerbound");
                if (!infinite) {
                    if (now > hardBudget) break;
                    if (now > softBudget && alpha > alpha0.v + C*Eval::parameters.evalHardBudget) break;
                }
                alpha.v = alpha2.v;
                search<(Colors)-C, trunk>(NodePV, b, *ml, depth-1, beta, alpha, ply+1, false NODE);
            } else {
                alpha.v = alpha2.v;
            }
        } else {
            search<(Colors)-C, trunk>(NodePV, b, *ml, depth-1, beta, alpha, ply+1, false NODE);
        }
        now = system_clock::now();
        console->send(status(now, alpha.v));

        if (alpha >= infinity*C) break;
        if (!infinite) {
            if (now > hardBudget) break;
            if (now > softBudget && alpha > alpha0.v - C*Eval::parameters.evalHardBudget) break;
        }
        beta2.v = alpha.v + Eval::parameters.aspiration1*C;

        for (++ml; ml.isValid() && !stopSearch; ++ml) {
            currentMoveIndex++;
            if (now > lastStatus2) {
                std::stringstream g;
                lastStatus2 = now + milliseconds(1000);
                g << "info currmove " << (*ml).algebraic() << " currmovenumber " << currentMoveIndex;
                console->send(g.str());
            }
            bool pruneNull = false;
            if (alpha.v != -infinity*C
                && depth > dMaxCapture+dMaxThreat+2
                && ml.count() > maxMovesNull)
            {
                SharedScore<(Colors)-C> null;
                SharedScore<C> nalpha(alpha);
                null.v = alpha.v + C;
                if (depth > nullReduction+1 + dMaxThreat + dMaxCapture)
                    search<C, trunk>(NodeFailHigh, b, *ml, depth-(nullReduction+1), nalpha, null, ply+2, false NODE);
                else
                    search<C, leaf>(NodeFailHigh, b, *ml, depth-(nullReduction+1), nalpha, null, ply+2, false NODE);
                pruneNull = alpha >= null.v;
                if (pruneNull && depth > 2*(nullReduction) + dMaxCapture + dMaxThreat) {
                    null.v = alpha.v + C;
                    search<(Colors)-C, trunk>(NodeFailHigh, b, *ml, depth-2*(nullReduction), null, nalpha, ply+1, false NODE);
                    pruneNull = alpha >= nalpha.v;
                }
            }
            if (!pruneNull) {
                if (search<(Colors)-C, trunk>(NodeFailHigh, b, *ml, depth-1, beta2, alpha, ply+1, false NODE)) {
                    bestMove = *ml;
                    ml.currentToFront();
                    if (alpha >= beta2.v) {
                        now = system_clock::now();
                        console->send(status(now, alpha.v) + " lowerbound");
                        search<(Colors)-C, trunk>(NodeFailHigh, b, bestMove, depth-1, beta, alpha, ply+1, false NODE);
                    }
                    beta2.v = alpha.v + Eval::parameters.aspiration1*C;
                    now = system_clock::now();
                    console->send(status(now, alpha.v));
                }
            }
            if (alpha >= infinity*C) break;
            if (!infinite) {
                system_clock::time_point now = system_clock::now();
                if (now > softBudget && alpha > alpha0.v + C*Eval::parameters.evalHardBudget) break;
            }
        }
        now = system_clock::now();
        console->send(status(now, alpha.v) + " done");

        if (alpha >= infinity*C) break;
        if (alpha <= -infinity*C) break;
        alpha0.v = alpha.v;
    }
    stopSearch = false;
    console->send("bestmove "+bestMove.algebraic());
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
bool RootBoard::search(const NodeType nodeType, const T& prev, const Move m, const unsigned depth, const A& alpha, B& beta, const unsigned ply, bool threatened
                                                                                                                                                #ifdef QT_GUI_LIB
                                                                                                                                                , NodeItem* parent
                                                                                                                                                #endif
                                                                                                                                                ) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    stats.node++;
    KeyScore estimate;
    estimate.vector = prev.estimatedEval(m, eval);
#ifdef QT_GUI_LIB
    uint64_t startnode = stats.node;
    NodeItem* node = 0;
    if (NodeItem::nNodes < MAX_NODES && NodeItem::nNodes >= MIN_NODES) {
        NodeItem::m.lock();
        NodeData data = {};
        data.alpha = alpha.v;
        data.beta = beta.v;
        data.move = m;
        data.ply = ply;
        data.key = estimate.key;
        data.searchType = P;
        data.depth = depth;
        data.moveColor = prev.CI == 0 ? White:Black;
        data.nodeColor = C;
        data.flags = 0;
        node = new NodeItem(data, parent);
        NodeItem::nNodes++;
        NodeItem::m.unlock();
    }
//     if (NodeItem::nNodes == 589577) asm("int3");
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

    if (P != vein && (stats.node & 0xff) == 0) {
        system_clock::time_point now = system_clock::now();
        if (!infinite && now > hardBudget) {
	    stopSearch = true;
        }
        if (now > lastStatus) {
            lastStatus = now + milliseconds(1000);
            std::stringstream g;
            uint64_t ntemp = getStats().node;
            g   << "info"
                << " nodes " << ntemp
                << " nps " << (1000*ntemp)/(duration_cast<milliseconds>(now-start).count()+1)
            ;
#ifdef MYDEBUG
            g << std::endl;
            g << "info string ";
            g << " bmob1 " << eval.bmob1 / (eval.bmobn+0.1);
            g << " bmob2 " << eval.bmob2 / (eval.bmobn+0.1);
            g << " bmob3 " << eval.bmob3 / (eval.bmobn+0.1);
#endif            
            console->send(g.str());
        }
    }
    if (stopSearch)
	return false;

    RawScore& eE = prev.CI == 0 ? estimatedError[nPieces + (m.piece()&7)][m.to()] : estimatedError[nPieces - (m.piece()&7)][m.to()];

    A current(alpha);    // current is always the maximum of (alpha, current), a out of thread increased alpha may increase current, but current has no influence on alpha.
    if (P==vein) {
        current.max(estimate.score.calc(prev.material)+lastPositionalEval-C*eE);
        if (current >= beta.v) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = beta.v;
            if (node) node->nodeType = NodePrecut1;
#endif
            stats.leafcut++;
            return false;
        }
    }

    const ColoredBoard<C> b(prev, m, estimate.vector);
    if (m.capture()) {
        int upperbound, lowerbound;
        eval.draw<C>(b, upperbound, lowerbound);
        current.max(lowerbound);
        beta.max(upperbound);
        if (current > beta.v)
            return false;
    }
    threatened |= b.template inCheck<C>();
    if (!threatened) {
        threatened = ((ColoredBoard<(Colors)-C>*)&b) -> template generateMateMoves<true>();
        if (!threatened) {
            threatened = ((ColoredBoard<(Colors)-C>*)&b) -> template generateSkewers(0);
            if (P==leaf && !threatened) {
                ScoreBase<C> current(alpha);
                current.max(estimate.score.calc(prev.material)+lastPositionalEval-C*eE);
                if (current >= beta.v) {
#ifdef QT_GUI_LIB
                    if (node) node->bestEval = beta.v;
                    if (node) node->nodeType = NodePrecut2;
#endif
                    stats.leafcut++;
                    return false;
                }
            }
        }
    }
#ifdef QT_GUI_LIB
    if (node && threatened) node->flags |= Threatened;
#endif

    Key z;
    if (P != vein) {
        z = b.getZobrist();
        store(z, ply);
    }

    if (isMain) {
        if ((int)prev.CI == (int)b.CI) {
            line[ply].data = 0;
            line[ply-1] = m;
        } else
            line[ply] = m;

        currentPly = ply;
    }

    ASSERT(b.keyScore.score.calc(prev.material) == estimate.score.calc(prev.material));
    ASSERT(P == vein || z == b.getZobrist());
    if ((int)prev.CI == (int)b.CI) {
        if (b.template inCheck<(Colors)-C>()) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = beta.v;
            if (node) node->nodeType = NodeIllegal;
#endif
            return false;
        }
    } else {
        ASSERT(!b.template inCheck<(Colors)-C>());
    }
    TranspositionTable<TTEntry, transpositionTableAssoc, Key>::SubTable* st;
    TTEntry subentry;

    Move ttMove(0,0,0);
    unsigned int ttDepth = 0;
//    bool betaNode = false;
    bool alphaNode = false;
    if (P != vein) {
        if (find(b, z, ply) || b.fiftyMoves >= 100) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = 0;
            if (node) node->nodeType = NodeRepetition;
#endif
            return beta.max(0, m);
        }
        st = tt->getSubTable(z);
        if (tt->retrieve(st, z, subentry)) {
            stats.tthit++;
            ttDepth = subentry.depth;
            ScoreBase<C> ttScore;
            ttScore.v = tt2Score(subentry.score);
            if (ttDepth >= depth) {

                if (subentry.loBound) {
                    stats.ttalpha++;
                    current.max(ttScore.v);
                }
                if (subentry.hiBound) {
                    stats.ttbeta++;
                    if (beta.max(ttScore.v, m)) {
#ifdef QT_GUI_LIB
                        if (node) node->bestEval = beta.v;
                        if (node) node->nodeType = NodeTT;
#endif
                        return true;
                    }
                }
                if (current >= beta.v) {
#ifdef QT_GUI_LIB
                    if (node) node->bestEval = beta.v;
                    if (node) node->nodeType = NodeTT;
#endif
                    return false;
                }
            } else {
                if (subentry.loBound && ttScore >= beta.v) {
                    /*betaNode = true*/
                }
                if (subentry.hiBound && ttScore < alpha.v)
                    alphaNode = true;
            }

            ttMove = Move(subentry.from, subentry.to, 0);
        }
    }

    do {
        Move moveList[256];
        Move* good = moveList+192;
        Move* endSort = good;
        Move* bad = good;
        Move* afterGoodCap;

        stats.eval++;
        int realScore = eval.eval(b);
        static const int minEstimatedError = 10;
        int error = C*(estimate.score.calc(b.material) + lastPositionalEval - realScore) + minEstimatedError;
        if (isMain) {
            if (eE < error) eE = error;
            else eE--;
        }
        lastPositionalEval = realScore - estimate.score.calc(b.material);

        if (P==vein || (P==leaf && !threatened)) {
            current.max(realScore);
            if (current >= beta.v) {
                stats.leafcut++;
#ifdef QT_GUI_LIB
                if (node) node->nodeType = NodePrecut3;
#endif
                break;
            }
            Move* j;
            for (Move* i = j = good; i<bad; ++i)
                if (i->capture())
                    *j++ = *i;
            bad = j;
        }

        Move *nonMate;
        if (b.template inCheck<C>()) {
            b.generateCheckEvasions(good, bad);
            if (bad == good) {
#ifdef QT_GUI_LIB
                if (node) node->nodeType = NodeMate;
                if (node) node->bestEval = -infinity*C;
#endif
                current.v = -infinity*C;
                break;
            }
            nonMate = good;
            afterGoodCap = good;
            if (P==vein) {
                Move* j;
                for (Move* i = j = good; i<bad; ++i)
                    if (i->capture())
                        *j++ = *i;
                bad = j;
                goto nosort;
            }

        } else {    // not in check
            if (P==vein || (P==leaf && !threatened)) {
                afterGoodCap = good;
                b.template generateCaptureMoves<false>(good, bad);
                nonMate = good;
                if ( P == leaf ) {
                    b.template generateSkewers(&good);
                    b.template generateMateMoves<false>(&good);
                    if (good < nonMate) {
//                        threatening = true;
                        for (Move* removing = good; removing<nonMate; ++removing) {
                            for (Move* removed = nonMate; removed<bad; ++removed) {
                                if (removing->data == removed->data) {
                                    memmove(removed, removed+1, sizeof(Move) * (bad-removed-1));
                                    --bad;
                                    break;
                                }
                            }
                        }
                    }
                }
                goto nosort;
            } else {
                b.template generateNonCap(good, bad);
                afterGoodCap = good;
                b.template generateCaptureMoves<true>(good, bad);
                if (bad == good) {
#ifdef QT_GUI_LIB
                    if (node) node->nodeType = NodeMate;
                    if (node) node->bestEval = 0;
#endif
                    current.v = 0;
                    break;
                }
                nonMate = good;
                // FIXME not thread safe
/*                if ( P == leaf && alpha < 0 && ply>2 && b.fiftyMoves>2) {
                    if (line[ply].from() == line[ply-2].to() && line[ply].to() == line[ply-2].from()) {
                        ASSERT(line[ply-1].capture() == 0);
                        *--good = Move(line[ply-1].to(), line[ply-1].from(), line[ply-1].piece());
                    }
                }*/
                b.template generateSkewers(&good);
                b.template generateMateMoves<false>(&good);
                if (good < nonMate) {
                    for (Move* removing = good; removing<nonMate; ++removing) {
                        for (Move* removed = nonMate; ; ++removed) {
                            ASSERT(removed<bad);
                            if (removing->data == removed->data) {
                                memmove(removed, removed+1, sizeof(Move) * (bad-removed-1));
                                --bad;
                                break;
                            }
                        }
                    }
//                    goto nosort;
                }
            }
        }
        if (endSort > afterGoodCap+1) {
            history.sort<C>(afterGoodCap, endSort-afterGoodCap, ply);
        }

nosort:
#ifdef QT_GUI_LIB
//         if (node && threatening) node->flags |= Threatening;
#endif
        // move best move from tt / history to front
        if (ttMove.data && ttMove.fromto() != good[0].fromto()) {
            Move first = good[0];
            for (Move* i = good+1; i<bad; ++i) {
                Move second = *i;
                *i = first;
                if (ttMove.fromto() == second.fromto()) {
                    good[0] = second;
                    break;
                }
                first = second;
            }
        }

        if (!alphaNode)
        for (unsigned int d = (depth+1)%2 + 1 + dMaxCapture + dMaxThreat; d < depth; d+=2) {
//      if (depth > 2 + dMaxCapture + dMaxThreat && depth > ttDepth+2) {
//          unsigned d = depth-2;
            if (d<=ttDepth) continue;
            for (Move* i = good; i<bad; ++i) {
                NodeType nextNodeType;
                if (nodeType == NodeFailHigh)
                    nextNodeType = NodeFailLow;
                else if (nodeType == NodeFailLow)
                    nextNodeType = NodeFailHigh;
                else if (i == good)
                    nextNodeType = NodePV;
                else
                    nextNodeType = NodeFailHigh;
//              if (!i->data) continue;
                ASSERT(d>0);
                if (d <= 1 + dMaxCapture + dMaxThreat ? search<(Colors)-C, leaf, B, A>(nextNodeType, b, *i, d - 1, beta, current, ply+1, false NODE) :
    //                d == depth-4 ? search<(Colors)-C, tree, B, A>(nextNodeType, b, *i, d-1, B(C*infinity), current) :
                                   search<(Colors)-C, tree, B, A>(nextNodeType, b, *i, d-1, beta, current, ply+1, false NODE)) {
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
                    if (d+2 >= depth) {
                        /*betaNode = true;*/
                    }
                    break;
                }
            }
            if (current >= infinity*C) break;
            current.v = alpha.v;
        }

        bool extSingle = false;
//         if (/*P == leaf &&*/ b.template inCheck<C>() && bad - good <= 2) {
//            nonMate+=2;
//             extSingle = true;
//         }
        for (Move* i = good; i<bad && current < beta.v; ++i) {
            if (extSingle) {
                if (alpha.v == current.v)
                    nonMate++;
                else
                    extSingle = false;
            }
            NodeType nextNodeType;
            if (nodeType == NodeFailHigh)
                nextNodeType = NodeFailLow;
            else if (nodeType == NodeFailLow)
                nextNodeType = NodeFailHigh;
            else if (i == good)
                nextNodeType = NodePV;
            else
                nextNodeType = NodeFailHigh;

            // Stay in leaf search if it already is or change to it if the next depth would be 0
            if ((P == leaf && i >= nonMate && !threatened) || P == vein || depth <= dMaxCapture + 1)
                search<(Colors)-C, vein>(nextNodeType, b, *i, 0, beta.unshared(), current, ply+1, false NODE);
            else if (depth <= dMaxCapture + dMaxThreat + 1/*|| (depth <= 2 && abs(b.keyScore.score) >= 400)*/)
                search<(Colors)-C, leaf>(nextNodeType, b, *i, depth-1, beta.unshared(), current, ply+1, i < nonMate NODE);
            else { // possible null search in tree or trunk
                int reduction = Options::reduction
                    && depth > 3 + dMaxCapture + dMaxThreat
                    && i > afterGoodCap + 3
//                        && !i->isSpecial()
                    && !threatened
                    && !((fold(b.doublebits[i->to()] & b.kingIncoming[EI].d02) && i->piece() & Rook)
                       || (fold(b.doublebits[i->to()] & b.kingIncoming[EI].d13) && i->piece() & Bishop)
                       || (BoardBase::knightAttacks[i->to()] & b.template getPieces<-C,King>() && i->piece() == Knight));
                if (i->isSpecial() && (bool[nPieces+1]){false, true, true, false, true, false, false}[i->piece() & 7]) {
                    reduction = 2+depth/2;
                }
                bool pruneNull = false;
                if (depth > 2 + dMaxCapture + dMaxThreat + reduction
                    && current.v != -infinity*C
                    && (i != good || alphaNode)
                    && bad-good > maxMovesNull)
                {
                    B null;
                    null.v = current.v + C;
                    if (depth >= nullReduction + Options::splitDepth + dMaxCapture + dMaxThreat)
                        search<C, P>(nextNodeType, b, *i, depth-(nullReduction+1+reduction), current, null, ply+2, i < nonMate NODE);
                    else if (depth > nullReduction+1+reduction + dMaxCapture + dMaxThreat)
                        search<C, tree>(nextNodeType, b, *i, depth-(nullReduction+1+reduction), current, null, ply+2, i < nonMate NODE);
                    else
                        search<C, leaf>(nextNodeType, b, *i, depth-(nullReduction+1+reduction), current, null, ply+2, i < nonMate NODE);
                    pruneNull = current >= null.v;
                    if (pruneNull) {
                        typename A::Base nalpha(current);
                        null.v = current.v + C;
                        if (depth > 2*(nullReduction+reduction) + dMaxCapture + dMaxThreat) {
                            search<(Colors)-C, tree>(nextNodeType, b, *i, depth-2*(nullReduction+reduction), null, nalpha, ply+1, i < nonMate NODE);
                            pruneNull = current >= nalpha.v;
                        }

//                            else
                            //search<(Colors)-C, leaf>(nextNodeType, b, *i, depth-2*nullReduction, beta, nalpha, ply+1, node);

                    }
                }
                if (!pruneNull) {
                    if (P == tree || alpha.isNotShared || depth < Options::splitDepth + dMaxCapture + dMaxThreat) {
                        bool research = true;
                        if (reduction) {
//                              reduction += (__bsrq(depth) + __bsrq(i-good))/4;
                            typename A::Base nalpha(current);
                            search<(Colors)-C, tree>(nextNodeType, b, *i, depth-reduction-1, beta, nalpha, ply+1, i < nonMate NODE);
                            research = current < nalpha.v;
                        }
                        if (research) {
                            search<(Colors)-C, tree>(nextNodeType, b, *i, depth-1, beta, current, ply+1, i < nonMate NODE);
                        }
                    // Multi threaded search: After the first move try to find a free thread, otherwise do a normal
                    // search but stay in trunk. To avoid multithreading search at cut off nodes
                    // where it would be useless.
                    } else {
                        if (depth == Options::splitDepth + dMaxCapture + dMaxThreat) {
                            if (i > good && WorkThread::canQueued(threadId, current.isNotReady())) {
                                WorkThread::queueJob(threadId, new SearchJob<(Colors)-C, typename B::Base, A, ColoredBoard<C> >(nextNodeType, *this, b, *i, depth-1, beta.unshared(), current, ply+1, threadId, keys NODE));
                            } else {
                                search<(Colors)-C, P>(nextNodeType, b, *i, depth-1, beta.unshared(), current, ply+1, i < nonMate NODE);
                            }
                        } else {
                            if (i > good && WorkThread::canQueued(threadId, current.isNotReady())) {
                                WorkThread::queueJob(threadId, new SearchJob<(Colors)-C,B,A, ColoredBoard<C> >(nextNodeType, *this, b, *i, depth-1, beta, current, ply+1, threadId, keys NODE));
                            } else {
                                search<(Colors)-C, P>(nextNodeType, b, *i, depth-1, beta, current, ply+1, i < nonMate NODE);
                            }
                        }
                        // alpha is shared here, it may have increased
                        if (alpha > current.v)
                            current.v = alpha.v;

                    }
                }
            }
        }

        // if there are queued jobs started from _this_ node, do them first
        if (P == trunk) {
            Job* job;
            while((job = WorkThread::getJob(threadId))) job->job();
    //        while(current.isNotReady() && (job = WorkThread::getChildJob(zd))) job->job();
        }

        current.join();

    } while (false);

    if (P != vein) {
        TTEntry stored;
        stored.zero();
        stored.depth |= depth;
        stored.upperKey |= z >> stored.upperShift;
        stored.score |= score2tt(current.v);
        stored.loBound |= current > alpha.v;
        if (current > alpha.v && current.m.capture() == 0 && current.m.data)
            history.good<C>(current.m, ply);
        stored.hiBound |= current < beta.v;
        stored.from |= current.m.from();
        stored.to |= current.m.to();
        tt->store(st, stored);
    }
#ifdef QT_GUI_LIB
    if (node) {
        node->bestEval = current.v;
        node->nodes = stats.node - startnode;
    }
#endif
    return beta.max(current.v, m);
}

template<Colors C>
uint64_t RootBoard::rootPerft(unsigned int depth) {
    Move moveList[256];

    const ColoredBoard<C>& b = currentBoard<C>();
    Move* good = moveList+192;
    Move* bad=good;
    if (b.template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.template generateCaptureMoves<true>(good, bad);
        b.generateNonCap(good, bad);
    }

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
    if (b.template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.template generateCaptureMoves<true>(good, bad);
        b.generateNonCap(good, bad);
    }

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
    if (b.template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.template generateCaptureMoves<true>(good, bad);
        b.generateNonCap(good, bad);
    }
    if (depth == 1) {
        update(result, bad-good);
        return;
    }

    ResultType n(0);
    for (Move* i = good; i<bad; ++i) {
        WorkThread* th;
        if (P == trunk && !!(th = WorkThread::findFree())) {
            th->queueJob(0U, new (PerftJob<(Colors)-C, ResultType>)(*this, n, b, *i, depth-1));
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
