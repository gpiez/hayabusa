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
#include "jobs.tcc"
#include "score.tcc"
#include "repetition.tcc"
#include "eval.tcc"
#include "history.tcc"
#include "genmates.tcc"

#ifdef QT_GUI_LIB
#define NODE ,node
#else
#define NODE
#endif

#define lazy1 1
#define lazy2 0

template<Colors C>
int RootBoard::calcReduction(const ColoredBoard< C >& b, int movenr, Move m, int depth) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    depth -= dMaxExt;
    if (Options::reduction && movenr >= 3 && depth > 5) {
        bool check = (!m.isSpecial() && (  (fold(b.doublebits[m.to()] & b.kingIncoming[EI].d02) && ((m.piece() == Rook) | (m.piece() == Queen)))
                                        || (fold(b.doublebits[m.to()] & b.kingIncoming[EI].d13) && ((m.piece() == Bishop) | (m.piece() == Queen)))
                                        )
                     )
                     || (BoardBase::knightAttacks[m.to()] & b.template getPieces<-C,King>() && m.piece() == Knight);
/*        return 2;
        check = false;*/
        int red = bitr(2*movenr + depth)/2;
        red = 1;
        if (m.isSpecial() && (m.piece() == Rook || m.piece() == Knight || m.piece() == Bishop))
            red += 2;
        if (check && depth<9)
            red=0;
        if (red<0) red=0;
        return red/* & ~1*/;
    }
    return 0;
}
/*
 * Search with color C to move, executing a -C colored move m from a board prev.
 * beta is the return value, it is updated at the end
 * A, B can be a SharedScore (updated by other threads) or a Score (thread local)
 */
template<Colors C, Phase P, typename A, typename B, typename T>
bool RootBoard::search(const T& prev, const Move m, const unsigned depth, const A& alpha, B& beta,
                       const unsigned ply, Extension extend, bool& nextMaxDepth, int& attack  //FIXME nextMaxDepth is only relevant for leaf search
#ifdef QT_GUI_LIB
, NodeItem* parent
#endif
) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
/*    stats.node++;*/
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
//         if (stats.node == 6437) asm("int3");
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

    //FIXME merge rooks/bishops/knights
    PositionalError& diff = prev.CI == 0 ? pe[nPieces + (m.piece()&7)][m.from()][m.to()] : pe[nPieces - (m.piece()&7)][m.from()][m.to()];
    // current is always the maximum of (alpha, current), a out of thread increased alpha may increase current, but current has no influence on alpha.
    // FIXME connect current with alpha, so that current is increased, if alpha inceases. Better update alpha explictly, requires no locking
    A current(alpha);
    RawScore estimatedScore = estimate.score.calc(prev.material) + prev.positionalScore + diff.v; //FIXME move score() into ColorBoard ctor
#ifdef CALCULATE_MEAN_POSITIONAL_ERROR
    if (diff.n > 0.0) {
        float invn = 1.0/diff.n;
        double v = diff.e2*invn - (diff.e*invn)*(diff.e*invn);
        v = sqrt(v);
        estimatedScore -= C*v;
    }
#endif    
    if (P==vein && lazy1) {
        current.max(estimatedScore);
        if (current >= beta.v) {
            attack = 0;
#ifdef QT_GUI_LIB
            if (node) node->bestEval = beta.v;
            if (node) node->nodeType = NodePrecut1;
#endif
            stats.leafcut++;
            return false;
        }
    }
    const ColoredBoard<C> b(prev, m, estimate.vector);
    bool betaChange = false;
    if (m.capture()) {
        int upperbound, lowerbound;
        eval.draw<C>(b, upperbound, lowerbound);
        current.max(lowerbound);
        betaChange = beta.max(upperbound, m);
        if (current >= beta.v) {
            attack = 0;
#ifdef QT_GUI_LIB
            if (node) node->bestEval = beta.v;
            if (node) node->nodeType = NodeEndgameEval;
#endif
            return betaChange;
        }
    }

    Key z;
    /*
     * Extensions in leaf search
     * ExtCheck:        You are checking, generate ALL moves, NO standpat.
     * ExtDual/Single:  You had only very few escapes. Additionally generate all checking and all threat moves
     * ExtPawnThreat:   You threat to promote a pawn, generate ALL moves, NO standpat. Next iteration generate pawn promos (leaf)
     * ExtMateThreat:   You threat to mate. Generate ALL moves, NO standpat. Next iteration generate all checking moves
     * ExtFork:         You are forking me. Generate ALL moves, NO standpat. Next iteration generates capures (leaf)
     */

    Extension threatened = ExtNot;
    if (P!=vein) {
        if (b.template inCheck<C>())
            threatened = ExtCheck;
        else /*if(!(extend & ExtPawnThreat))*/ {
            uint64_t pMoves = b.template getPieces<-C,Pawn>() & b.pins[EI];
            pMoves = ( (  shift<-C*8  >(pMoves)               & ~b.occupied1)
                       | (shift<-C*8+1>(pMoves) & ~file<'a'>() &  b.occupied[CI])
                       | (shift<-C*8-1>(pMoves) & ~file<'h'>() &  b.occupied[CI]))
                     & rank<C,1>() & ~b.template getAttacks<C,All>();
            if (pMoves)
                threatened = ExtPawnThreat;
        }
        if (!threatened /*&& !(extend & ExtMateThreat)*/) {
            if (((ColoredBoard<(Colors)-C>*)&b) -> template generateMateMoves<true, bool>())
                threatened = ExtMateThreat;
        }
        if (!threatened && !(extend & ExtForkThreat)) { //FIXME this does not make sense if a piece of us is hanging, as this makes the threat irrelevant
            if (b.isForked())
                threatened = ExtFork;
        }
    }

#ifdef QT_GUI_LIB
    if (node && threatened) node->flags |= Threatened;
    if (node && extend) node->flags |= Extend;
#endif
    
    if (P!=vein) {
        z = b.getZobrist();
        store(z, ply);          //TODO could be delayed?
    }

    if (P==vein || (P==leaf && !threatened)) {
        ScoreBase<C> current(alpha);
        current.max(estimatedScore);
        if (current >= beta.v) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = beta.v;
            if (node) node->nodeType = NodePrecut2;
#endif
            stats.leafcut++;
            attack = 0;
            return betaChange;
        }
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
            attack = 0;
            return betaChange;
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
    bool hasMaxDepth = false;
    if (P != vein) {
        if (find(b, z, ply) || b.fiftyMoves >= 100) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = 0;
            if (node) node->nodeType = NodeRepetition;
#endif
            attack = 0;
            return betaChange | beta.max(0, m);
        }
        st = tt->getSubTable(z);
        if (tt->retrieve(st, z, subentry)) {
            stats.tthit++;
            ttDepth = subentry.depth;
            ScoreBase<C> ttScore;
            ttScore.v = tt2Score(subentry.score);
            if (ttDepth >= depth || (ttScore>=infinity*C && subentry.loBound) || (ttScore<=-infinity*C && subentry.hiBound)) {
                if (ttDepth < dMaxExt) {
                    nextMaxDepth = true;
                    hasMaxDepth = true;
                }
                if (subentry.loBound) {
                    stats.ttalpha++;
                    current.max(ttScore.v);
                }
                if (subentry.hiBound) {
                    stats.ttbeta++;
                    betaChange |= beta.max(ttScore.v, m);
                }
                if (current >= beta.v) {
#ifdef QT_GUI_LIB
                    if (node) node->bestEval = ttScore.v;
                    if (node) node->nodeType = NodeTT;
#endif
                    attack = 0;  //TODO store attack in tt?
                    return betaChange;
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

    stats.node++;
    do {
        if (P==tree && Options::pruning && !threatened && !extend) {
            if (depth == dMaxExt + 1) {
                ScoreBase<C> fScore;
                fScore.v = estimatedScore - C*100;
#ifdef QT_GUI_LIB
                if (node) node->bestEval = fScore.v;
                if (node) node->nodeType = NodeFutile1;
#endif

                if (fScore >= beta.v) {
                    attack = 0;
                    return betaChange;
                }
            }
            if (depth == dMaxExt + 2) {
                ScoreBase<C> fScore;
                fScore.v = estimatedScore - C*1000;
#ifdef QT_GUI_LIB
                if (node) node->bestEval = fScore.v;
                if (node) node->nodeType = NodeFutile2;
#endif
                if (fScore >= beta.v) {
                    attack = 0;
                    return betaChange;
                }
            }
/*            if (depth == dMaxCapture + dMaxThreat + 3) {
                ScoreBase<C> fScore;
                fScore.v = estimatedScore - C*1000;
#ifdef QT_GUI_LIB
                if (node) node->bestEval = fScore.v;
                if (node) node->nodeType = NodeFutile3;
#endif
                if (fScore >= beta.v) return false;
            }*/
        }

        RawScore realScore;
        int oldattack;
        if (0 && prev.isExact) {
            realScore = estimatedScore;
            b.positionalScore = prev.positionalScore + diff.v;
            b.isExact = false;
        } else {
            stats.eval++;
            int wattack, battack;
            b.positionalScore = eval(b, C, wattack, battack);
            if (prev.CI==0) {
                attack = wattack;
            } else {
                attack = battack;
            }
            if (C==White) {
                oldattack = wattack;
            } else {
                oldattack = battack;
            }
            b.isExact = true;
            realScore = estimate.score.calc(prev.material) + b.positionalScore;
            if (isMain) {
                ScoreBase<C> diff2;
                diff2.v = b.positionalScore - prev.positionalScore;
#ifdef CALCULATE_MEAN_POSITIONAL_ERROR                
                diff.n++;
                double err = diff2.v - diff.v;
                diff.e  += err;
                diff.e2 += err*err;
                if (diff.n > 0)
                ASSERT( diff.e2/diff.n - diff.e*diff.e/(diff.n*diff.n) >= 0 );
#else                
                ASSERT(realScore == estimatedScore - diff.v + diff2.v);
#endif                
                diff.v = (diff2.v + diff.v)/2;
//                 if (diff2 < diff.v) diff.v = diff2.v;
//                 else diff.v += C;
            }
        }
        
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

        Move moveList[256];
        Move* good = moveList+192;
        Move* bad = good;
        Move* badCaptures = good;
        Move *captures, *threats, *nonCaptures;
        
        if (b.template inCheck<C>()) {
            nonCaptures = good;
            b.generateCheckEvasions(good, bad);
            if (bad == good) {
#ifdef QT_GUI_LIB
                if (node) node->nodeType = NodeMate;
                if (node) node->bestEval = -infinity*C;
#endif
                current.v = -infinity*C;
                break;
            }
            captures = good;
            threats = good;
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
                if ( P==leaf && extend & (ExtDualReply|ExtSingleReply)) b.template generateNonCap(good, bad); //FIXME should generate k-threat increasing moves only
                nonCaptures = good;
                if ( P==leaf ) b.generateSkewers(&good);
                threats = good;
                b.template generateCaptureMoves<NoUnderPromo>(good, bad);
                captures = good;
                if ( P==leaf ) {
                    b.template generateMateMoves<false, void>(&good, &bad);                    
                    if (good < captures)
                        for (Move* removing = good; removing<captures; ++removing)
                            for (Move* removed = captures; removed<bad; ++removed)
                                if (removing->data == removed->data) {
                                    memmove(removed, removed+1, sizeof(Move) * (bad-removed-1));
                                    --bad;
                                    break;
                                }
                }
                goto nosort;
            } else {
                b.template generateNonCap(good, bad);
                nonCaptures = good;
                b.template generateCaptureMoves<AllMoves>(good, bad);
                captures = good;
                if (bad == good) {
#ifdef QT_GUI_LIB
                    if (node) node->nodeType = NodeMate;
                    if (node) node->bestEval = 0;
#endif
                    current.v = 0;
                    break;
                }
                // FIXME not thread safe
/*                if ( P == leaf && alpha < 0 && ply>2 && b.fiftyMoves>2) {
                    if (line[ply].from() == line[ply-2].to() && line[ply].to() == line[ply-2].from()) {
                        ASSERT(line[ply-1].capture() == 0);
                        *--good = Move(line[ply-1].to(), line[ply-1].from(), line[ply-1].piece());
                    }
                }*/
                b.template generateSkewers(&good);
                threats = good;
                b.template generateMateMoves<false, void>(&good, &bad);
                if (good < threats) {
                    for (Move* removing = good; removing<threats; ++removing) {
                        for (Move* removed = threats; ; ++removed) {
                            ASSERT(removed<bad);
                            if (removing->data == removed->data) {
                                memmove(removed, removed+1, sizeof(Move) * (bad-removed-1));
                                --bad;
                                break;
                            }
                        }
                    }
                    if (bad < badCaptures) badCaptures = bad;
//                    goto nosort;
                }
            }
        }
        if (badCaptures > nonCaptures+1) {
            ASSERT(badCaptures <= bad);
            history.sort<C>(nonCaptures, badCaptures-nonCaptures, ply + rootPly);
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

        if (P != vein && P != leaf && !alphaNode)
        for (unsigned int d = (depth+1)%2 + 1 + dMaxExt; d < depth; d+=2) {
//      if (depth > 2 + dMaxCapture + dMaxThreat && depth > ttDepth+2) {
//          unsigned d = depth-2;
            if (d<=ttDepth) continue;
            Score<C> alpha0(current);
            Score<(Colors)-C> beta0(beta);
//             if (alpha0 > -0x400*C) alpha0.v -= 5*C;
//             if (beta0 > 0x400*C) beta0.v += 5*C;
            for (Move* i = good; i<bad; ++i) {
/*                NodeType nextNodeType;
                if (nodeType == NodeFailHigh)
                    nextNodeType = NodeFailLow;
                else if (nodeType == NodeFailLow)
                    nextNodeType = NodeFailHigh;
                else if (i == good)
                    nextNodeType = NodePV;
                else
                    nextNodeType = NodeFailHigh;*/
//              if (!i->data) continue;
                ASSERT(d>0);
                int red = calcReduction(b, i-good, *i, d);
                bool dummy __attribute__((unused));
                int dummy2 __attribute__((unused));
                if (d-red <= 1 + dMaxExt ? search<(Colors)-C, leaf>(b, *i, d -red - 1, beta0, alpha0, ply+1, ExtNot, dummy, dummy2 NODE) :
    //                d == depth-4 ? search<(Colors)-C, tree, B, A>(b, *i, d-1, B(C*infinity), current) :
                                   search<(Colors)-C, tree>(b, *i, d-red-1, beta0, alpha0, ply+1, ExtNot, dummy, dummy2 NODE)) {
                    ASSERT(preCurrent.m.data == i->data);
                    Move first = good[0];
                    good[0] = alpha0.m;
                    for (Move* j = good+1; j<i; ++j) {
                        Move second = *j;
                        *j = first;
                        first = second;
                    }
                    *i = first;
                }
                if (alpha0 >= beta0.v) {
                    if (d+2 >= depth) {
                        /*betaNode = true;*/
                    }
                    break;
                }
            }
            if (alpha0 >= infinity*C) break;
        }

        if (bad > good) current.m = *good;
        Extension leafExt = threatened;
        if (P != vein && b.template inCheck<C>()) {
            ASSERT ( depth > dMaxCapture );
            if (bad-good == 2) 
                leafExt = (Extension) (leafExt | ExtDualReply);
            else if (bad-good == 1)
                leafExt = (Extension) (leafExt | ExtSingleReply);
        }
/*
 * The inner move loop
 */
        for (Move* i = good; i<bad && current < beta.v; ++i) {
            int nattack;
            if ( ( P == leaf && ((i>=captures && i<threats) || i>=nonCaptures)
                             && !threatened
                             && !(extend & (ExtDualReply|ExtSingleReply))
                             /*&& ~ply & 1*/)
                || P == vein) {
                if (!i->capture() && !i->isSpecial()) continue;
                bool unused __attribute__((unused));
                search<(Colors)-C, vein>(b, *i, 0, beta.unshared(), current.unshared(), ply+1, ExtNot, unused, nattack NODE);
            } else if ( P == leaf  && (   depth <= dMaxCapture + 1
                                   || (depth <= dMinDualExt + 1 && extend & ExtDualReply))) {
                bool unused __attribute__((unused));
                search<(Colors)-C, vein>(b, *i, 0, beta.unshared(), current.unshared(), ply+1, ExtNot, unused, nattack NODE);
                hasMaxDepth = true;
            }
            else if ( P == leaf || depth <= dMaxExt + 1) {
                search<(Colors)-C, leaf>(b, *i, depth-1, beta.unshared(), current.unshared(), ply+1, leafExt, hasMaxDepth, nattack NODE);
            } else { // possible null search in tree or trunk
                int reduction = calcReduction(b, i-good, *i, depth);
                bool pruneNull = false;
                if (depth > 2 + dMaxExt
                    && current.v != -infinity*C //FIXME compare to alpha0.v of rootsearch
                    && (i != good || alphaNode)
                    && bad-good > maxMovesNull
                    && b.material) {
                    /*
                     * Verify nullmove search. If it is >alpha, dont't bother with nullmove
                     */
                    Score<C> alpha0(current);
                    Score<(Colors)-C> alpha1(current.v + C);
                    unsigned newDepth = depth-(nullReduction+2);
                    ASSERT(depth > nullReduction+2);
                    if (newDepth > dMaxExt)
                        pruneNull = !search<(Colors)-C, P>(b, *i, newDepth, alpha1, alpha0, ply+1, ExtNot, hasMaxDepth, nattack NODE);
                    else
                        pruneNull = !search<(Colors)-C, leaf>(b, *i, newDepth, alpha1, alpha0, ply+1, ExtNot, hasMaxDepth, nattack NODE);
                    ASSERT(pruneNull == (current >= alpha0.v));
                    /*
                     * The actual null move search. Search returns true if the
                     * result in alpha1 comes down to alpha0, in that case prune
                     */
                    if (pruneNull) {
                        newDepth = depth-(nullReduction+1);
                        ASSERT(depth > nullReduction+1);
                        ASSERT(current.v == alpha0.v);
                        if (newDepth > dMaxExt)
                            pruneNull = search<C, P>(b, *i, newDepth, alpha0, alpha1, ply+2, ExtNot, hasMaxDepth, nattack NODE);
                        else
                            pruneNull = search<C, leaf>(b, *i, newDepth, alpha0, alpha1, ply+2, leafExt, hasMaxDepth, nattack NODE);
                        ASSERT(pruneNull == (current >= alpha1.v));
                    } 
                }
                if (!pruneNull) {
                    if (P == tree || alpha.isNotShared || depth < Options::splitDepth + dMaxExt) {
                        bool research = true;
                        if (   (current.v != -infinity*C //FIXME compare to alpha0.v, reuse condition
                            && (i != good || alphaNode))
                            && reduction) {
                            Score<C> alpha0(current);
                            const Score<(Colors)-C> alpha1(current.v + C);
//                             const Score<(Colors)-C> alpha1(beta);
                            unsigned newDepth = depth-(reduction+1);
                            ASSERT(depth > (unsigned)reduction+1);
                            if (newDepth > dMaxExt)
                                research = search<(Colors)-C, P>(b, *i, newDepth, alpha1, alpha0, ply+1, ExtNot, hasMaxDepth, nattack NODE);
                            else
                                research = search<(Colors)-C, leaf>(b, *i, newDepth, alpha1, alpha0, ply+1, leafExt, hasMaxDepth, nattack NODE);
                            ASSERT(research == (current < alpha0.v));
                            ASSERT(research != (current >= alpha0.v));
                        }
                        if (research || (nattack > oldattack && nattack > 20*((signed)depth-(signed)dMaxExt) + 80)) {
                            ASSERT(research || reduction);
                            reduction = 0;
/*                                if (!research && nattack > 30*((signed)depth-(signed)dMaxExt) + 50)
                                reduction = -1;*/
                            unsigned newDepth = depth-reduction-1;
                            ASSERT(depth > reduction+1+dMaxExt);
                            const Score<(Colors)-C> alpha1(current.v + C);
                            if (i == good || alpha1.v == beta.v
                                || current.v <= -0x400 || current.v >= 0x400
                                || newDepth < 3+dMaxExt
                                || search<(Colors)-C, P>(b, *i, newDepth, alpha1, current, ply+1, ExtNot, hasMaxDepth, nattack NODE))
                                search<(Colors)-C, P>(b, *i, newDepth, beta, current, ply+1, ExtNot, hasMaxDepth, nattack NODE);
                        }
                        /*} else {
                            bool research = search<(Colors)-C, tree>(b, *i, newDepth, beta, current, ply+1, ExtNot, hasMaxDepth, nattack NODE);
                            if (0 && !research && reduction == 0 && nattack > 30*((signed)depth-(signed)dMaxExt) + 50) {
                                reduction = -1;
                                search<(Colors)-C, tree>(b, *i, depth-reduction-1, beta, current, ply+1, ExtNot, hasMaxDepth, nattack NODE);
                            }
                        }*/
                    // Multi threaded search: After the first move try to find a free thread, otherwise do a normal
                    // search but stay in trunk. To avoid multithreading search at cut off nodes
                    // where it would be useless.
                    } else {
                        if (depth == Options::splitDepth + dMaxExt) {
                            if (i > good && WorkThread::canQueued(threadId, current.isNotReady())) {
                                WorkThread::queueJob(threadId, new SearchJob<(Colors)-C, typename B::Base, A, ColoredBoard<C> >(*this, b, *i, depth-1, beta.unshared(), current, ply+1, threadId, keys NODE));
                            } else {
                                search<(Colors)-C, P>(b, *i, depth-1, beta.unshared(), current, ply+1, ExtNot, hasMaxDepth, nattack NODE);
                            }
                        } else {
                            if (i > good && WorkThread::canQueued(threadId, current.isNotReady())) {
                                WorkThread::queueJob(threadId, new SearchJob<(Colors)-C,B,A, ColoredBoard<C> >(*this, b, *i, depth-1, beta, current, ply+1, threadId, keys NODE));
                            } else {
                                search<(Colors)-C, P>(b, *i, depth-1, beta, current, ply+1, ExtNot, hasMaxDepth, nattack NODE);
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
        if (depth <= dMaxExt && !hasMaxDepth) //FIXME dMinDual does not update hashMaxDepth correctly
            stored.depth |= dMaxExt;
        else
            stored.depth |= depth;
        nextMaxDepth |= hasMaxDepth;
            
        stored.upperKey |= z >> stored.upperShift;
        stored.score |= score2tt(current.v);
        stored.loBound |= current > alpha.v;
        if (current > alpha.v && current.m.capture() == 0 && current.m.data)
            history.good<C>(current.m, ply + rootPly);
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
        return betaChange;
    else
        return betaChange | beta.max(current.v, m);
}
#endif