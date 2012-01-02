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
#include "history.tcc"
#include "genmates.tcc"

#ifdef QT_GUI_LIB
#define NODE ,node
#else
#define NODE
#endif

template<Colors C>
int RootBoard::calcReduction(const ColoredBoard< C >& b, int movenr, Move m, int depth) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    depth -= eval.dMaxExt;
    if (Options::reduction && movenr > eval.dRed[depth]) {
        bool check = (!m.isSpecial() && (  (fold(b.bits[m.to()].doublebits & b.kingIncoming[EI].d02) && ((m.piece() == Rook) | (m.piece() == Queen)))
                                        || (fold(b.bits[m.to()].doublebits & b.kingIncoming[EI].d13) && ((m.piece() == Bishop) | (m.piece() == Queen)))
                                        )
                     )
                     || (BoardBase::knightAttacks[m.to()] & b.template getPieces<-C,King>() && m.piece() == Knight)
                     || (1ULL << m.from()) & ~b.pins[EI];
/*        return 2;
        check = false;*/
        int red = bitr(2*movenr + depth)/2;
        red = 1;
        if (m.isSpecial() && (m.piece() == Rook || m.piece() == Knight || m.piece() == Bishop))
            red += 2;
        if (check && depth<eval.dRedCheck)
            red=0;
        if (m.capture() && depth<eval.dRedCapture)
            red=0;
        if (red<0) red=0;
        return red/* & ~1*/;
    }
    return 0;
}

template<Colors C, Phase P, class A, class B>
int RootBoard::search9(const bool doNull, const unsigned reduction, const ColoredBoard<(Colors)-C>& prev, const Move m, const unsigned newDepth,
                       const A& alpha, const B& beta,
                       const unsigned ply, Extension, NodeType nextNT
#ifdef QT_GUI_LIB
, NodeItem* node
#endif
) {
    ASSERT(P!=vein);
    ASSERT(P!=leaf);
    Score<C> value;
    if (doNull) {
        /*
         * Verify nullmove search. If it is <beta, dont't bother
         * with nullmove as the nullmove result will be even worse
         */
        const A alpha0(beta.v - C);
        value.v = search4<C, P>(prev, m, verifyReduction[newDepth], alpha0, beta, ply+1, ExtNot, nextNT NODE);
        /*
         * The actual null move search. Search returns true if the
         * result in beta1 comes down to alpha0, in that case prune
         */
        if (value >= beta.v) {
            if (value >= infinity*C) return value.v;
            Score<C> nullvalue(search4<(Colors)-C, P>(prev, m, nullReduction[newDepth], beta, alpha0, ply+2, ExtNot, nextNT NODE));
            if (nullvalue >= beta.v) {
                return value.v;
            }
        }
    }

    if (reduction) {
        const A alpha0(beta.v - C);
        value.v = search4<C, P>(prev, m, newDepth-reduction, alpha0, beta, ply+1, ExtNot, nextNT NODE);
        if (value >= beta.v) {
            return value.v;
        }
    }

    value.v = search4<C, P>(prev, m, newDepth, alpha, beta, ply+1, ExtNot, nextNT NODE);
#ifdef QT_GUI_LIB    
    if (node) {
        NodeItem::m.lock();
        node->getParent()->moveToEnd(node);
        NodeItem::m.unlock();
    }
#endif    
    return value.v;
}
/*
 * Search with color C to move, executing a -C colored move m from a board prev.
 * beta is the return value, it is updated at the end
 * A, B can be a SharedScore (updated by other threads) or a Score (thread local)
 */
template<Colors C, Phase P, class A, class B, Colors PREVC>
int RootBoard::search4(const ColoredBoard<PREVC>& prev, const Move m, const unsigned depth,
                       const A& a, const B& b,
                       const unsigned ply, Extension extend, const NodeType nt
#ifdef QT_GUI_LIB
, NodeItem* node
#endif
) {
    ASSERT(P!=vein);
    ASSERT(P!=leaf);
    ASSERT(!(depth <= eval.dMaxCapture));
    bool unused;
//     if (P == vein || depth <= eval.dMaxCapture)
//         return search3<C,vein>(prev,m,depth,a.unshared(),b.unshared(),ply,extend,unused,nt NODE);
    if (/*P == leaf ||*/ depth <= eval.dMaxExt)
        return search3<C,leaf>(prev,m,depth,a.unshared(),b.unshared(),ply,extend,unused,nt NODE);
    if (P == tree || depth <= Options::splitDepth + eval.dMaxExt)
        return search3<C,tree>(prev,m,depth,a.unshared(),b,ply,extend,unused,nt NODE);
    ASSERT(P==trunk);
    return search3<C,trunk>(prev,m,depth,a,b,ply,extend,unused,nt NODE);
}
/*
 * Search with color C to move, executing a -C colored move m from a board prev.
 * beta is the return value, it is updated at the end
 * A, B can be a SharedScore (updated by other threads) or a Score (thread local)
 */
template<Colors C, Phase P, class A, class B, Colors PREVC>
int RootBoard::search3(const ColoredBoard<PREVC>& prev, const Move m, unsigned depth,
                       const A& origAlpha, const B& origBeta,
                       const unsigned ply, Extension extend, bool& nextMaxDepth, const NodeType nt
#ifdef QT_GUI_LIB
, NodeItem* parent
#endif
) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    stats.node++;
    if (stats.node > maxSearchNodes) {
        stopSearch = Stopping;
        return 0;
    }
    KeyScore estimate;
    if (P == leaf || P == vein)
        estimate.vector = eval.inline_estimate<PREVC>(m, prev.keyScore);
    else
        estimate.vector = eval.estimate<PREVC>(m, prev.keyScore);
#ifdef QT_GUI_LIB
    NodeItem* node = 0;
    if (NodeItem::nNodes < MAX_NODES && parent) {
        NodeItem::m.lock();
        NodeData data = {};
        data.alpha = origAlpha.v;
        data.beta = origBeta.v;
        data.move = m;
        data.ply = ply;
        data.key = estimate.key;
        data.searchType = P;
        data.depth = depth;
        data.moveColor = prev.CI == 0 ? White:Black;
        data.nodeColor = C;
        data.flags = 0;
        data.threadId = threadId;
        data.nodes = 1;
       
        data.nodeType = NodeFull;
        node = new NodeItem(data, parent);
        NodeItem::nNodes++;
        NodeItem::m.unlock();
    }
#endif
/*
    prefetching is not very effective for core2, probably because each access
    is completely random and causes not only a cache miss, but additionally a
    tlb miss, which stalls the processor almost als long as the actual memory
    access would without. It gets slightly more useful, if 2M pages are used.
    Speed increase measured as 1-2% with 4k pages and ~5% witch 2M pages. 2M
    alone increase the speed by ~2%.
    For a core i7 the speed increase from prefetching is about 3%.
*/
    if (P != vein) tt->prefetchSubTable(estimate.key + C+1);

    if (P != vein && P != leaf) {
//         if (Options::cpuTime) {
//             boost::chrono::duration<boost::chrono::process_cpu_clock::times, boost::nano> t = boost::chrono::process_cpu_clock::now();
//             if (t.count().system+t.count().user > stopTime)
//                 stopSearch = Stopping;
//         }
        if (stopSearch != Running) return 0;
    }
    
    unsigned iPiece = prev.errorPieceIndex[ m.piece() & 7 ];
    PositionalError& diff = pe[iPiece][m.from()][m.to()];
    // current is always the maximum of (alpha, current), a out of thread increased alpha may increase current, but current has no influence on alpha.
    // TODO connect current with alpha, so that current is increased, if alpha inceases. Better update alpha explictly, requires no locking
    //     current.v = -infinity*C;
    RawScore estimatedScore = estimate.score.calc(prev.material, eval) + prev.positionalScore + diff.v - C*diff.error; //TODO move score() into ColorBoard ctor
    if (P==vein && !(extend & ~ExtFirstMove)) {
        Score<C> value(estimatedScore); //TODO get rid of this var
        if (value >= origBeta.v) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = value.v;
            if (node) node->nodeType = NodePrecut1;
#endif
            stats.leafcut++;
            return value.v;
        }
    }
    const ColoredBoard<C> b(prev, m, estimate.vector);
    int upperbound = C*infinity;
    if (m.capture()) {
        if (eval.draw<C>(b, upperbound) || origAlpha >= upperbound) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = upperbound;
            if (node) node->nodeType = NodeEndgameEval;
#endif
            return upperbound;
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

    Extension threatened = b.template inCheck<C>() && (extend & ExtTestMate || eval.flags & 2 || P!=vein) ? ExtCheck : ExtNot;  //This is needed here because we might be in vein with ExtTestMate
    if (P!=vein) {
        if (!threatened && depth > eval.dMinPawnExt) {
            uint64_t pMoves = b.template getPieces<-C,Pawn>() & b.template getPins<(Colors)-C>();
            pMoves = ( (  shift<-C*8  >(pMoves)               & ~b.occupied1)
                       | (shift<-C*8+1>(pMoves) & ~file<'a'>() &  b.template getOcc<C>())
                       | (shift<-C*8-1>(pMoves) & ~file<'h'>() &  b.template getOcc<C>()))
                     & rank<C,1>() & ~b.template getAttacks<C,All>();
            if (pMoves)
                threatened = ExtPawnThreat;
        }
        if (!threatened && depth > eval.dMinMateExt) {
            if (b.swapped().template generateMateMoves<true, bool>())
                threatened = ExtMateThreat;
        }
//         if (eval.flags &1 && !threatened && depth > eval.dMinExtDisco)
//             if (((ColoredBoard<(Colors)-C>*)&b) -> template generateDiscoveredCheck<true, bool>())
//                 threatened = ExtDiscoveredCheck;
            
        if (!threatened && depth > eval.dMinForkExt) { //FIXME this does not make sense if a piece of us is hanging, as this makes the threat irrelevant
            if (b.isForked())
                threatened = ExtFork;
        }

        if (0 && !threatened /*&& depth > eval.dMinHanging*/ && b.isPieceHanging() && eval.flags & 2) {
            threatened = ExtFork;
        }
    }

#ifdef QT_GUI_LIB
    if (node && threatened) node->flags |= Threatened;
    if (node && extend) node->flags |= Extend;
#endif

    if (P!=vein) {
        z = b.getZobrist();/* if (z==7326551192246482564) asm("int3");*/
        store(z, ply);          //TODO could be delayed?
    }

    if ((P==vein && !(threatened && eval.flags & 8)) || (P==leaf && !threatened)) {
        Score<C> value(origAlpha);
        value.max(estimatedScore);
        if (value >= origBeta.v) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = value.v;
            if (node) node->nodeType = NodePrecut2;
#endif
            stats.leafcut++;
            return value.v;
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

    ASSERT(b.keyScore.score.calc(prev.material, eval) == estimate.score.calc(prev.material, eval));
    ASSERT(P == vein || z == b.getZobrist());
    if ((int)prev.CI == (int)b.CI) {
        if (b.template inCheck<(Colors)-C>()) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = infinity*C;
            if (node) node->nodeType = NodeIllegal;
#endif
            return infinity*C;
        }
    } else {
        ASSERT(!b.template inCheck<(Colors)-C>());
    }
    TranspositionTable<TTEntry, transpositionTableAssoc, Key>::SubTable* st;
    TTEntry subentry;

    unsigned int ttDepth = 0;
//    bool betaNode = false;
//     bool alphaNode = false;
    if (P == tree) {
        if (depth < eval.dMaxExtCheck && threatened & ExtCheck)
            depth++;
//         else if (depth < eval.dMaxExtPawn && threatened & ExtSingleReply)
//             depth++;
    }
    A alpha(origAlpha);
    B beta(origBeta);
    beta.max(upperbound);           
    ScoreMove<C,A> current; current.v=(-infinity*C);
    current.m = {0,0,0};
    if (P != vein) {
        if (find(b, z, ply) || b.fiftyMoves >= 100) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = 0;
            if (node) node->nodeType = NodeRepetition;
#endif
            return 0;
        }
        st = tt->getSubTable(z);
        if (tt->retrieve(st, z, subentry)) {
            stats.tthit++;
            ttDepth = subentry.depth;
            Score<C> ttScore;
            if (P == leaf)
                ttScore.v = inline_tt2Score(subentry.score);
            else
                ttScore.v = tt2Score(subentry.score);
            if (ttDepth >= depth || (ttScore>=infinity*C && subentry.loBound) || (ttScore<=-infinity*C && subentry.hiBound)) {
                if (P == leaf && depth <= eval.dMaxExt) {
                    nextMaxDepth = true;
                }
                if (subentry.loBound) {
                    stats.ttalpha++;
                    alpha.max(ttScore.v);
                    current.max(ttScore.v);
                }
                if (subentry.hiBound) {
                    stats.ttbeta++;
                    beta.max(ttScore.v);
                }
                if (current >= beta.v) {
#ifdef QT_GUI_LIB
                    if (node) node->bestEval = ttScore.v;
                    if (node) node->nodeType = NodeTT;
#endif
//                     alpha.setReady();   //TODO avoid early construction fo alpha/beta
//                     beta.setReady();
                    return current.v;
                }
            }

            current.m = Move(subentry.from, subentry.to, 0);
        }
    }
    bool leafMaxDepth;
    if (P == leaf) leafMaxDepth=false;

    if (P==tree && Options::pruning && !threatened && nt != NodePV && !b.isPieceHanging()) {
        if (depth == eval.dMaxExt + 1) {
            Score<C> fScore;
            fScore.v = estimatedScore - C*eval.prune1 - !!m.capture()*C*eval.prune1c;
#ifdef QT_GUI_LIB
            if (node) node->bestEval = fScore.v;
            if (node) node->nodeType = NodeFutile1;
#endif

            if (fScore >= beta.v) {
                return fScore.v;
            }
        }
        if (depth == eval.dMaxExt + 2) {
            Score<C> fScore;
            fScore.v = estimatedScore - C*eval.prune2 - !!m.capture()*C*eval.prune2c;
#ifdef QT_GUI_LIB
            if (node) node->bestEval = fScore.v;
            if (node) node->nodeType = NodeFutile2;
#endif
            if (fScore >= beta.v) {
                return fScore.v;
            }
        }
/*            if (depth == dMaxCapture + dMaxThreat + 3) {
            Score<C> fScore;
            fScore.v = estimatedScore - C*1000;
#ifdef QT_GUI_LIB
            if (node) node->bestEval = fScore.v;
            if (node) node->nodeType = NodeFutile3;
#endif
            if (fScore >= beta.v) return false;
        }*/
    }

    RawScore realScore;
    if (0 && prev.isExact) {
        realScore = estimatedScore;
        b.positionalScore = prev.positionalScore + diff.v;
        b.isExact = false;
    } else {
        stats.eval++;
        int wattack, battack;
        b.positionalScore = eval(b, C, wattack, battack);
        b.isExact = true;
        realScore = b.keyScore.score.calc(prev.material, eval) + b.positionalScore;
        if (isMain & !m.isSpecial()) {
            int diff2 = b.positionalScore - prev.positionalScore;
            if (eval.calcMeanError) {
                diff.n++;
                diff.e  += diff2;
                diff.e2 += diff2*diff2;
                if (diff.n >= 256) {
                    diff.n *= 0.5;
                    diff.e *= 0.5;
                    diff.e2 *= 0.5;
                }
                float invn = 1.0/diff.n;
                float avg = diff.e*invn;
                if (diff.n > 1) {
                    float v = diff.e2*invn - avg*avg;
                    v = sqrtf(v);
                    diff.error = eval.standardSigma*v;
                } else
                    diff.error = eval.standardError*0.5;
                diff.v = avg;
            } else
                diff.v = (diff2 + diff.v)/2;
        }
    }
    if (P==vein) { ASSERT(!(threatened & ~ExtCheck)); };

    if ((P==vein && !(threatened && eval.flags & 8)) || (P==leaf && !threatened)) {
        alpha.max(realScore + C*eval.tempo);
        current.max(realScore + C*eval.tempo);
        if (current >= beta.v) {
            stats.leafcut++;
#ifdef QT_GUI_LIB
            if (node) node->nodeType = NodePrecut3;
#endif
//             current.m.data = 0;
            goto storeAndExit;
        }
/*            Move* j;
        for (Move* i = j = good; i<bad; ++i) {
            ASSERT(!"Never executed");
            if (i->capture())
                *j++ = *i;
        }
        bad = j;*/
    }

    if (P == vein && !depth) return current.v;
    ASSERT(depth);
    {
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
            ASSERT(!current.m.data);
            goto storeAndExit;
        }
        if (P==vein && threatened) {
            alpha.max(realScore);
            current.max(realScore);
            if (current >= beta.v) {
                stats.leafcut++;
#ifdef QT_GUI_LIB
                if (node) node->nodeType = NodePrecut3;
#endif
                ASSERT(current.m.piece() || !current.m.fromto());
                goto storeAndExit;
            }
        }
        captures = good;
        threats = good;
        if (P==vein) {
            Move* j;
            for (Move* i = j = good; i<bad; ++i)
                if (i->capture())
                    *j++ = *i;
            bad = j;
            if (bad == good) {
#ifdef QT_GUI_LIB
                if (node) node->nodeType = NodeStandpat;
                if (node) node->bestEval = current.v;
#endif
                goto storeAndExit;
            }
            goto nosort;
        }

    } else {    // not in check
        if ((P==vein  && !(threatened && eval.flags & 4)) || (P==leaf && !threatened)) {
//                 if ( P==leaf && extend & (ExtDualReply|ExtSingleReply)) b.template generateNonCap(good, bad); //FIXME should generate k-threat increasing moves only
            nonCaptures = good;
            if ( P==leaf ) b.generateSkewers(&good);
            if (P == leaf) b.template generateDiscoveredCheck<false, void>(&good);
            threats = good;
            b.template generateCaptureMoves<NoUnderPromo>(good, bad);
            captures = good;
            if ( P==leaf ) {
                Move checks[64];
                Move* pChecks = checks;
                b.template generateMateMoves<false, void>(&good, &pChecks);
                unsigned nChecks = pChecks-checks;
                memmove(threats+nChecks, threats, (bad-threats)*sizeof(Move));
                memcpy(threats, checks, nChecks*sizeof(Move));
                nonCaptures+=nChecks;
                bad+=nChecks;
                
                if (good < captures)
                    for (Move* removing = good; removing<captures; ++removing)
                        for (Move* removed = captures; removed<bad; ++removed)
                            if (removing->data == removed->data) {
                                memmove(removed, removed+1, sizeof(Move) * (bad-removed-1));
                                --bad;
                                break;
                            }
            }
            if (bad == good) {
#ifdef QT_GUI_LIB
                if (node) node->nodeType = NodeStandpat;
                if (node) node->bestEval = current.v;
#endif
                goto storeAndExit;
            }
            goto nosort;
        } else {
            b.template generateNonCap(good, bad);
            nonCaptures = good;
            b.template generateSkewers(&good);
            threats = good;
            b.template generateCaptureMoves<AllMoves>(good, bad);
            captures = good;
            if (bad == good) {
                ASSERT(!current.m.data);
#ifdef QT_GUI_LIB
                if (node) node->nodeType = NodeMate;
                if (node) node->bestEval = 0;
#endif
                current.v = 0;
                goto storeAndExit;
            }
            // FIXME not thread safe
/*                if ( P == leaf && alpha < 0 && ply>2 && b.fiftyMoves>2) {
                if (line[ply].from() == line[ply-2].to() && line[ply].to() == line[ply-2].from()) {
                    ASSERT(line[ply-1].capture() == 0);
                    *--good = Move(line[ply-1].to(), line[ply-1].from(), line[ply-1].piece());
                }
            }*/
            b.template generateMateMoves<false, void>(&good, &bad);
            if (good < captures) {
                for (Move* removing = good; removing<captures; ++removing) {
                    for (Move* removed = captures; ; ++removed) {
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
        history.sort<C>(nonCaptures, badCaptures-nonCaptures, ply + rootPly, pe, b.material, eval);
    }
    ASSERT(good<bad);

nosort:
#ifdef QT_GUI_LIB
//         if (node && threatening) node->flags |= Threatening;
#endif
    // move best move from tt / history to front
    if (P != vein && current.m.data && good+1 < bad && current.m.fromto() != good[0].fromto()) {
        for (Move* removed = good+1; removed<bad; ++removed) {
            if (current.m.fromto() == removed->fromto()) {
                current.m = *removed;
                memmove(good+1, good, sizeof(Move) * (removed-good));
                *good = current.m;
                break;
            }
        }
    }
    ASSERT(good<bad);
    if (!current.m.fromto()) current.m = *good;
//         ASSERT(current.m.piece() || !current.m.fromto());

    bool unused __attribute__((unused));
//         if (bad > good) current.m = *good;
    Extension leafExt = threatened;
    if (P != vein && b.template inCheck<C>()) {
        ASSERT ( depth >= eval.dMaxCapture );
        if (bad-good == 2) {
            if (depth > eval.dMinDualExt)
                leafExt = (Extension) (leafExt | ExtDualReply);
        } else if (bad-good == 1)
            if (depth > eval.dMinSingleExt)
                leafExt = (Extension) (leafExt | ExtSingleReply);
    }
    /*
     * The inner move loop
     */
    NodeType nextNT = (NodeType)-nt;
    for (Move* i = good; i<bad && current < beta.v; ++i) {
        if (!origBeta.isNotShared) beta.max(origBeta.v);
        if (!origAlpha.isNotShared) alpha.max(origAlpha.v);
//             if (P != vein && i == good+1) {
//                 if (current.v > 0)
//                     leafExt = (Extension) (leafExt & ~ExtDualReply);
//             }
        Score<C> value;
        unsigned newDepth = depth - 1;
        /*
         * Quiescense search, only material changing moves, other moves
         * aren't even generated
         */
        if (P == vein)  {
            ASSERT(newDepth < eval.dMaxCapture);
            value.v = search3<(Colors)-C, vein>(b, *i, newDepth, beta.unshared(), alpha.unshared(), ply+1, ExtNot, unused, nextNT NODE);
        /*
         * Transition from extended q search to pure q search for
         * for non-tactical moves
         */
        } else if (P == leaf && ((i>=captures && i<threats) || i>=nonCaptures)
                          && !threatened
                          && !(extend & (ExtDualReply | ExtSingleReply)) //TODO replace by a condition "not checking move"
                         )
        {
            if (!i->capture() && !i->isSpecial()) continue;
            value.v = search3<(Colors)-C, vein>(b, *i, std::min(newDepth, eval.dMaxCapture), beta.unshared(), alpha.unshared(), ply+1, ExtNot, unused, nextNT NODE);
        /*
         * Transition from extended q search to pure q search for tactical
         * moves, if the move is a possible mate, don't do a lazy eval in
         * the next iteration
         */
        } else if (P == leaf && newDepth <= eval.dMaxCapture) {
            ASSERT(newDepth == eval.dMaxCapture);
            leafExt = i<captures ? ExtTestMate : ExtNot;
            value.v = search3<(Colors)-C, vein>(b, *i, eval.dMaxCapture, beta.unshared(), alpha.unshared(), ply+1, leafExt, unused, nextNT NODE);
            leafMaxDepth = true;
        } else if ( P == leaf || newDepth <= eval.dMaxExt) {
            value.v = search3<(Colors)-C, leaf>(b, *i, newDepth, beta.unshared(), alpha.unshared(), ply+1, leafExt, leafMaxDepth, nextNT NODE);
        } else { // possible null search in tree or trunk
            ASSERT(P != leaf);
            bool doNull = newDepth >= eval.dMinReduction
                && current.v != -infinity*C //TODO compare to alpha0.v of rootsearch
                && b.material; /*&& eval.flags & 4*/
            bool ninf = current.v == -C*infinity;
            int reduction = !ninf && calcReduction(b, i-good, *i, depth); //FIXME compare to alpha0.v, reuse condition;
            if (P == trunk && depth > Options::splitDepth + eval.dMaxExt && (nt == NodeFailLow || i > good) && WorkThread::canQueued(WorkThread::threadId, current.isNotReady())) {
                WorkThread::queueJob(WorkThread::threadId,
                    new SearchJob<C, A, B, ColoredBoard<C> >(*this, b, doNull, reduction, *i, newDepth,
                                           alpha, beta, current, ply, WorkThread::threadId, keys, nextNT NODE));
                    if (stopSearch != Running) break;
                    continue;
            } else {
                value.v = search9<(Colors)-C, P>(doNull, reduction, b, *i, newDepth, beta, alpha, ply, ExtNot, nextNT NODE);
                WorkThread::reserve(bad-i-1);
            }
        }
        
        if (stopSearch != Running) break;
        nextNT = NodeFailHigh;
        alpha.max(value.v);
        current.max(value.v, *i);
    }  // for Move*

    // if there are queued jobs started from _this_ node, do them first
    // don't start threads from parent search but with same therad id here
    if (P == trunk) {
        Job* job;
        while((job = WorkThread::getJob(WorkThread::threadId, ply))) {
            job->job();
        }
//        while(current.isNotReady() && (job = WorkThread::getChildJob(zd))) job->job();
    }

    alpha.join();
    current.join();
    }
storeAndExit:
    if (P != vein) {
        TTEntry stored;
        stored.zero();
        if (P == leaf) {
            ASSERT(depth <= eval.dMaxExt);
            if (!leafMaxDepth) //FIXME dMinDual does not update hashMaxDepth correctly
                stored.depth |= eval.dMaxExt;
            else {
                stored.depth |= depth;
                nextMaxDepth = true;
            }
        } else
            stored.depth |= depth;
        

        stored.upperKey |= z >> stored.upperShift;
        if (P == leaf)
            stored.score |= inline_score2tt(current.v);
        else
            stored.score |= score2tt(current.v);
        
        stored.loBound |= current > origAlpha.v;
        if (current > origAlpha.v && current.m.capture() == 0 && current.m.piece()/* && !current.m.isSpecial()*/) {
            ASSERT(current.m.fromto());
            history.good<C>(current.m, ply + rootPly);
        }
        stored.hiBound |= (current < origBeta.v) & (stopSearch == Running);
        stored.from |= current.m.from();
        stored.to |= current.m.to();
        tt->store(st, stored);
    }
#ifdef QT_GUI_LIB
    if (node) {
        node->bestEval = current.v;
        for (int i=0; i<node->childCount(); ++i)
            node->nodes += node->child(i)->nodes;
    }
#endif
    return current.v;
}
#endif