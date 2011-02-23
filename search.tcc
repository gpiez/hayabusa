/*
    Copyright (C) 2011  Gunther Piez

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
#ifndef SEARCH_TCC_
#define SEARCH_TCC_

#include "rootboard.h"
#include "options.h"
#include "transpositiontable.tcc"
#include "jobs.h"

#ifdef QT_GUI_LIB
#define NODE ,node
#else
#define NODE
#endif

//                 int reduction = 0;
//                 if (i>good) {
//                     if (Options::reduction
// //                        && depth > 3 + dMaxCapture + dMaxThreat
//                         && i > good // + 3
//     //                        && !i->isSpecial()
//                         && !threatened
//                         && ) {
//                         reduction = std::min(((depth-dMaxCapture-dMaxThreat)/2 + i-good + 2)/8*2, 4L);
//                         
// /*                        if (abs(realScore - alpha.v) > 500 && depth==4)
//                             reduction = 3;
//                         else if (abs(realScore - alpha.v) > 150 && depth==3)
//                             reduction = 2;*/
//                         }
//                     if (i->isSpecial() && (bool[nPieces+1]){false, true, true, false, true, false, false}[i->piece() & 7])
//                         reduction = 2+depth/2;
//                 }

template<Colors C>
int RootBoard::calcReduction(const ColoredBoard< C >& b, int movenr, Move m, int depth) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    depth -= dMaxThreat + dMaxCapture;
    if (Options::reduction && movenr >= 1 && depth > 2 ) {
        bool check = ((fold(b.doublebits[m.to()] & b.kingIncoming[EI].d02) && m.piece() & Rook)
                   || (fold(b.doublebits[m.to()] & b.kingIncoming[EI].d13) && m.piece() & Bishop)
                   || (BoardBase::knightAttacks[m.to()] & b.template getPieces<-C,King>() && m.piece() == Knight));
        int red = bitr(movenr + depth - 3)/2;
        if (check && depth<9)
            red=0;
        if (red >= depth-1)
            red = depth-2;
        if (red<0) red=0;
        return red;
    }
    return 0;
}

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
//    if(NodeItem::nNodes == 230012) asm("int3");
    }
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

    if (stopSearch) return false;

    RawScore& eE = prev.CI == 0 ? estimatedError[nPieces + (m.piece()&7)][m.to()] : estimatedError[nPieces - (m.piece()&7)][m.to()];
    // current is always the maximum of (alpha, current), a out of thread increased alpha may increase current, but current has no influence on alpha.
    // FIXME connect current with alpha, so that current is increased, if alpha inceases. Better update alpha explictly, requires no locking
    A current(alpha);

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
        if (current > beta.v) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = beta.v;
            if (node) node->nodeType = NodeEndgameEval;
#endif
            return false;
        }
    }
    
    Key z;
    if (P!=vein) {
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
            if (ttDepth >= depth || (ttScore>=infinity*C && subentry.loBound) || (ttScore<=-infinity*C && subentry.hiBound)) {

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
        int realScore = eval(b);
        if (P == tree && Options::pruning && !threatened) {
            if (depth == dMaxCapture + dMaxThreat + 1) {
                ScoreBase<C> fScore;
                fScore.v = realScore - C*150;
#ifdef QT_GUI_LIB
                if (node) node->bestEval = beta.v;
                if (node) node->nodeType = NodeFutile1;
#endif
                
                if (fScore >= beta.v) return false;
            }
            if (depth == dMaxCapture + dMaxThreat + 2) {
                ScoreBase<C> fScore;
                fScore.v = realScore - C*900;
#ifdef QT_GUI_LIB
                if (node) node->bestEval = beta.v;
                if (node) node->nodeType = NodeFutile2;
#endif
                if (fScore >= beta.v) return false;
            }
        }
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
/*            Move* j;
            for (Move* i = j = good; i<bad; ++i) {
                ASSERT(!"Never executed");
                if (i->capture())
                    *j++ = *i;
            }
            bad = j;*/
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
                    if (bad < endSort) endSort = bad;
//                    goto nosort;
                }
            }
        }
        if (endSort > afterGoodCap+1) {
            ASSERT(endSort <= bad);
            history.sort<C>(afterGoodCap, endSort-afterGoodCap, ply);
        }

nosort:
#ifdef QT_GUI_LIB
//         if (node && threatening) node->flags |= Threatening;
#endif
        // move best move from tt / history to front
        if (P != vein && ttMove.data && good+1 < bad && ttMove.fromto() != good[0].fromto()) {
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
            typename A::Base preCurrent(current);
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
                int red = calcReduction(b, i-good, *i, d);
                if (d-red <= 1 + dMaxCapture + dMaxThreat ? search<(Colors)-C, leaf>(nextNodeType, b, *i, d -red - 1, beta.unshared(), preCurrent, ply+1, false NODE) :
    //                d == depth-4 ? search<(Colors)-C, tree, B, A>(nextNodeType, b, *i, d-1, B(C*infinity), current) :
                                   search<(Colors)-C, tree>(nextNodeType, b, *i, d-red-1, beta.unshared(), preCurrent, ply+1, false NODE)) {
                    ASSERT(preCurrent.m.data == i->data);
                    Move first = good[0];
                    good[0] = preCurrent.m;
                    for (Move* j = good+1; j<i; ++j) {
                        Move second = *j;
                        *j = first;
                        first = second;
                    }
                    *i = first;
                }
                if (preCurrent >= beta.v) {
                    if (d+2 >= depth) {
                        /*betaNode = true;*/
                    }
                    break;
                }
            }
            if (preCurrent >= infinity*C) break;
        }

        bool extSingle = false;
        if (bad > good) current.m = *good;
//         if (/*P == leaf &&*/ b.template inCheck<C>() && bad - good <= 2) {
//            nonMate+=2;
//             extSingle = true;
//         }
/*
 * The inner move loop
 */        
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
            // FIXME don't go immediatly to vein, allow one check or so in threat search, to get knight forks at c7
            // only go to vein, if two consecutive capture moves without check happen
            if ((P == leaf && i >= nonMate && !threatened) || P == vein || depth <= dMaxCapture + 1)
                search<(Colors)-C, vein>(nextNodeType, b, *i, 0, beta.unshared(), current.unshared(), ply+1, false NODE);
            else if (depth <= dMaxCapture + dMaxThreat + 1/*|| (depth <= 2 && abs(b.keyScore.score) >= 400)*/)
                search<(Colors)-C, leaf>(nextNodeType, b, *i, depth-1, beta.unshared(), current.unshared(), ply+1, i < nonMate NODE);
            else { // possible null search in tree or trunk
                int reduction = calcReduction(b, i-good, *i, depth);
                bool pruneNull = false;
                if (depth > 2 + dMaxCapture + dMaxThreat + reduction
                    && current.v != -infinity*C //FIXME compare to alpha0.v of rootsearch
                    && (i != good || alphaNode)
                    && bad-good > maxMovesNull)
                {
                    typename B::Base null;
                    null.v = current.v + C;
                    if (depth >= nullReduction + Options::splitDepth + dMaxCapture + dMaxThreat)
                        search<C, P>(nextNodeType, b, *i, depth-(nullReduction+1+reduction), current.unshared(), null, ply+2, i < nonMate NODE);
                    else if (depth > nullReduction+1+reduction + dMaxCapture + dMaxThreat)
                        search<C, tree>(nextNodeType, b, *i, depth-(nullReduction+1+reduction), current.unshared(), null, ply+2, i < nonMate NODE);
                    else
                        search<C, leaf>(nextNodeType, b, *i, depth-(nullReduction+1+reduction), current.unshared(), null, ply+2, i < nonMate NODE);
                    pruneNull = current >= null.v;
                    if (pruneNull) {
                        typename A::Base nalpha(current);
                        null.v = current.v + C;
                        if (depth > 2*nullReduction+reduction + dMaxCapture + dMaxThreat) {
                            search<(Colors)-C, tree>(nextNodeType, b, *i, depth-2*nullReduction-reduction, null, nalpha, ply+1, i < nonMate NODE);
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
//                              reduction += (bitr(depth) + bitr(i-good))/4;
                            typename A::Base nalpha(current);
                            if (depth <= dMaxCapture + dMaxThreat + 1 + reduction/*|| (depth <= 2 && abs(b.keyScore.score) >= 400)*/)
                                search<(Colors)-C, leaf>(nextNodeType, b, *i, depth-reduction-1, beta.unshared(), nalpha, ply+1, i < nonMate NODE);
                            else
                                search<(Colors)-C, tree>(nextNodeType, b, *i, depth-reduction-1, beta.unshared(), nalpha, ply+1, i < nonMate NODE);
                            research = current < nalpha.v;
                        }
                        if (research) {
                            search<(Colors)-C, tree>(nextNodeType, b, *i, depth-1, beta.unshared(), current, ply+1, i < nonMate NODE);
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

    if (P != vein && !stopSearch) {
        TTEntry stored;
        stored.zero();
        stored.depth |= depth;
        stored.upperKey |= z >> stored.upperShift;
        stored.score |= score2tt(current.v);
        stored.loBound |= current > alpha.v;
        if (current > alpha.v && current.m.capture() == 0 && current.m.data)
            history.good<C>(current.m, ply);
        stored.hiBound |= current < beta.v;
        stored.from |= current.m.from(); //TODO store a best move even if all moves are fail low
        stored.to |= current.m.to();
        tt->store(st, stored);
    }
#ifdef QT_GUI_LIB
    if (node) {
        node->bestEval = current.v;
        node->nodes = stats.node - startnode;
    }
#endif
    if (stopSearch)
        return false;
    else
        return beta.max(current.v, m);
}
#endif