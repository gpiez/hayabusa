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

#include "game.h"
#include "options.h"
#include "jobs.h"
#include "movelist.h"
#include "movelist.tcc"
#include "transpositiontable.tcc"
#include "history.tcc"
#include "genmates.tcc"

template<Colors C>
int Game::calcReduction(const ColoredBoard< C >& b, int movenr, Move m, int depth) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    depth -= eval.dMaxExt;
    if (Options::reduction && movenr > eval.dRed[depth]) {
        bool check = (!m.isSpecial() && (  (fold(b.bits[m.to()].doublebits & b.kingIncoming[EI].d02) && ((m.piece() == Rook) | ((m.piece()&7) == Queen)))
                                           || (fold(b.bits[m.to()].doublebits & b.kingIncoming[EI].d13) && ((m.piece() == Bishop) | ((m.piece()&7) == Queen)))
                                        )
                     )
                     || (Board::knightAttacks[m.to()] & b.template getPieces<-C,King>() && m.piece() == Knight)
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
        return red/* & ~1*/; }
    return 0; }

template<Colors C, Phase P, class A, class B>
int Game::search9(const bool doNull, const unsigned reduction, const ColoredBoard<C>& nextboard, const unsigned newDepth,
                  const A& alpha, const B& beta,
                  Extension, NodeType nextNT NODEDEF ) {
    ASSERT(P!=vein);
    ASSERT(P!=leaf);
    Score<C> value;
#ifdef QT_GUI_LIB    
    NodeItem* node = parent;
#endif
    if (doNull) {
        /*
         * Verify nullmove search. If it is <beta, dont't bother
         * with nullmove as the nullmove result will be even worse
         */
        const A alpha0(beta.v - C);
        value.v = search4<C, P>(nextboard, verifyReduction[newDepth], alpha0, beta, ExtNot, nextNT NODE);
        /*
         * The actual null move search. Search returns true if the
         * result in beta1 comes down to alpha0, in that case prune
         */
        if (value >= beta.v && !nextboard.template inCheck<C>()) {
            if (value >= infinity*C) return value.v;
            int temp = nextboard.fiftyMoves;
            nextboard.ply++;
            if (isMain) line[nextboard.ply].data = 0;            
            nextboard.fiftyMoves = 0;
            if (eval.flags & 1)
            nextboard.estScore = nextboard.psValue + nextboard.prevPositionalScore + *nextboard.diff;
            Score<C> nullvalue(search4<(Colors)-C, P>(nextboard.swapped(), nullReduction[newDepth], beta, alpha0, ExtNot, nextNT NODE ));
            if (nullvalue >= beta.v) return value.v;
            nextboard.fiftyMoves = temp;
            nextboard.ply--;
        } }
    
    if (reduction) {
        if (eval.flags & 1)
        nextboard.estScore = nextboard.psValue + nextboard.prevPositionalScore + *nextboard.diff; 
        const A alpha0(beta.v - C);
        value.v = search4<C, P>(nextboard, newDepth-reduction, alpha0, beta, ExtNot, nextNT NODE);
        if (value >= beta.v) {
            return value.v; } }

    if (eval.flags & 1)
    nextboard.estScore = nextboard.psValue + nextboard.prevPositionalScore + *nextboard.diff;
    value.v = search4<C, P>(nextboard, newDepth, alpha, beta, ExtNot, nextNT NODE);
#ifdef QT_GUI_LIB
    if (parent) {
        NodeItem::m.lock();
        parent->getParent()->moveToEnd(parent);
        NodeItem::m.unlock(); }
#endif
    return value.v; }
/*
 * Search with color C to move, executing a -C colored move m from a board prev.
 * beta is the return value, it is updated at the end
 * A, B can be a SharedScore (updated by other threads) or a Score (thread local)
 */
template<Colors C, Phase P, class A, class B>
int Game::search4(const ColoredBoard<C>& nextboard, const unsigned depth,
                  const A& a, const B& b,
                  Extension extend, const NodeType nt NODEDEF ) {
    ASSERT(P!=vein);
    ASSERT(P!=leaf);
    ASSERT(!(depth <= eval.dMaxCapture));
#ifdef QT_GUI_LIB    
    NodeItem* node = parent;
#endif
    bool unused;
//     if (P == vein || depth <= eval.dMaxCapture)
//         return search3<C,vein>(prev,m,depth,a.unshared(),b.unshared(),ply,extend,unused,nt NODE);
    if (/*P == leaf ||*/ depth <= eval.dMaxExt)
        return search3<C,leaf>(nextboard,depth,a.unshared(),b.unshared(),extend,unused,nt NODE);
    if (P == tree || b.isNotShared || depth <= Options::splitDepth + eval.dMaxExt)
        return search3<C,tree>(nextboard,depth,a.unshared(),b,extend,unused,nt NODE);
    ASSERT(P==trunk);
    return search3<C,trunk>(nextboard,depth,a,b,extend,unused,nt NODE); }
/*
 * Search with color C to move, executing a -C colored move m from a board prev.
 * beta is the return value, it is updated at the end
 * A, B can be a SharedScore (updated by other threads) or a Score (thread local)
 */
template<Colors C, Phase P, class A, class B>
int Game::search3(const ColoredBoard<C>& b, unsigned depth,
                  const A& origAlpha, const B& origBeta,
                  Extension extend, bool& nextMaxDepth, const NodeType nt NODEDEF ) {
    WorkThread::stats.node++;
//     if (WorkThread::stats.node == 2327) asm("int3");
#ifdef MYDEBUG
    uint64_t __attribute__((unused)) localnode = WorkThread::stats.node;
#endif
    if unlikely( WorkThread::stats.node > maxSearchNodes ) {
        searchState = Stopping;
        return 0; }
    KeyScore estimate;
    estimate = b.keyScore;
#ifdef QT_GUI_LIB
    NodeItem* node = 0;
    if (NodeItem::nNodes < MAX_NODES && parent) {
        NodeItem::m.lock();
        NodeData data = { };
        data.alpha = origAlpha.v;
        data.beta = origBeta.v;
        data.move = b.m;
        data.ply = b.ply;
        data.key = estimate.key;
        data.searchType = P;
        data.depth = depth;
        data.moveColor = -C;
        data.nodeColor = C;
        data.flags = 0;
        data.threadId = WorkThread::getThisThreadId();
        data.nodes = 1;

        data.nodeType = NodeFull;
        node = new NodeItem(data, parent);
        NodeItem::nNodes++;
        NodeItem::m.unlock(); }
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
//    if (P != vein) tt->prefetchSubTable(estimate.key + C+1);

    if (P != vein && P != leaf && unlikely(searchState != Running)) return 0;
#ifdef QT_GUI_LIB
    if (node) node->gain = *b.diff.v;
    if (node) node->ps = b.psValue;
    if (node) node->real = 0;
#endif

    /*
     * Extensions in leaf search
     * ExtCheck:        You are checking, generate ALL moves, NO standpat.
     * ExtDual/Single:  You had only very few escapes. Additionally generate all checking and all threat moves
     * ExtPawnThreat:   You threat to promote a pawn, generate ALL moves, NO standpat. Next iteration generate pawn promos (leaf)
     * ExtMateThreat:   You threat to mate. Generate ALL moves, NO standpat. Next iteration generate all checking moves
     * ExtFork:         You are forking me. Generate ALL moves, NO standpat. Next iteration generates capures (leaf)
     */

    b.threatened = ExtNot;
    // This is needed here because we might be in vein with ExtTestMate
    bool inCheck = b.template inCheck<C>();
    if (P==vein) {
    	if (extend & ExtTestMate && inCheck) 
    		b.threatened = ExtCheck; }
    
    else if (P==leaf || depth <= eval.dMaxExt+3 ) { // max pruning depth
        uint64_t pMoves;
    	if (unlikely(inCheck) && likely(depth > eval.dMaxCheckExt)) 
            b.threatened = ExtCheck;
    	else if (likely(depth > eval.dMinForkExt) && unlikely(popcount(b.isPieceHanging(eval)) > 1))
        	b.threatened = ExtFork;
    	else if (likely(depth > eval.dMinPawnExt) 
    	    && unlikely((pMoves = b.template getPieces<-C,Pawn>() & b.template getPins<(Colors)-C>() & rank<C,2>()))
    	    && ((pMoves = ( (  shift<-C*8  >(pMoves)               & ~b.occupied1)
                           | (shift<-C*8+1>(pMoves) & ~file<'a'>() &  b.template getOcc<C>())
                           | (shift<-C*8-1>(pMoves) & ~file<'h'>() &  b.template getOcc<C>()))
                         & ~b.template getAttacks<C,All>())))
            b.threatened = ExtPawnThreat;
        else if (likely(depth > eval.dMinMateExt) && unlikely((b.swapped().template generateMateMoves<true, bool>()))) 
            b.threatened = ExtMateThreat;
//         if (eval.flags &1 && !b.threatened && depth > eval.dMinExtDisco)
//             if (((ColoredBoard<(Colors)-C>*)&b) -> template generateDiscoveredCheck<true, bool>())
//                 b.threatened = ExtDiscoveredCheck;
        else if (depth > eval.dMinForkExt && !b.isPieceHanging(eval) && unlikely(b.isForked()))
            b.threatened = ExtFork; }
    // Only needed for check extension later
    else if (unlikely(inCheck) && likely(depth > eval.dMaxCheckExt)) 
        b.threatened = ExtCheck; 


#ifdef QT_GUI_LIB
    if (node && b.threatened) node->flags |= b.threatened;
    if (node && extend) node->flags |= Extend;
#endif

    Key z;
    if (P!=vein) {
        z = b.getZobrist();
    }
    if ((P==vein || P==leaf) && !b.threatened) {
        Score<C> value(origAlpha);
        value.max(eval.quantize(b.estScore - C*eval.standardError));
        if (value >= origBeta.v && (value<limit<C>() || eval.allowLazyRoot)) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = value.v;
            if (node) node->nodeType = NodePrecut2;
#endif
            WorkThread::stats.leafcut++;
            return value.v; } }

    ASSERT( CompoundScore(b.keyScore.vector).opening() == CompoundScore(estimate.vector).opening());
    ASSERT( CompoundScore(b.keyScore.vector).endgame() == CompoundScore(estimate.vector).endgame());
    ASSERT(P == vein || z == b.getZobrist());
    ASSERT(!b.template inCheck<(Colors)-C>());
    TranspositionTable<TTEntry, transpositionTableAssoc, Key>::SubTable* st;
    TTEntry subentry;

    unsigned int ttDepth = 0;
//    bool betaNode = false;
//     bool alphaNode = false;
    if (P == tree) {
        if (depth < eval.dMaxExtCheck && b.threatened & ExtCheck)
            depth++;
//         else if (depth < eval.dMaxExtPawn && b.threatened & ExtSingleReply)
//             depth++;
    }
    A alpha(origAlpha);
    B beta(origBeta);
    ScoreMove<C,A> current;
    current.v=(-infinity*C);
    current.m = {0,0,0 };
    int posScore = 0xdeadbeaf;
    if (P != vein) {
        if (findRepetition(b, z, b.ply) || b.fiftyMoves >= 100) {
#ifdef QT_GUI_LIB
            if (node) node->bestEval = 0;
            if (node) node->nodeType = NodeRepetition;
#endif
            return 0; }
        st = tt->getSubTable(z);
        if unlikely(tt->retrieve(st, z, subentry)) {
            WorkThread::stats.tthit++;
            ttDepth = subentry[::depth];
            posScore = tt2Score(subentry[::posScore]);
            Score<C> ttScore;
            ttScore.v = tt2Score(subentry[score]);
            if unlikely (ttDepth >= depth 
                || (ttScore>=infinity*C && subentry[loBound]) 
                || (ttScore<=-infinity*C && subentry[hiBound])) {
                if (P == leaf && depth <= eval.dMaxExt) {
                    nextMaxDepth = true; }
                    if unlikely(subentry[loBound] & subentry[hiBound]) {

//                        stats.ttexact++;
#ifdef QT_GUI_LIB
                        if (node) node->bestEval = ttScore.v;
                        if (node) node->nodeType = NodeTT;
#endif
                        return ttScore.v; } 
                    else if unlikely(subentry[loBound]) {
                        alpha.max(ttScore.v);
                        WorkThread::stats.ttalpha++;
                        if (alpha >= beta.v) {
#ifdef QT_GUI_LIB
                            if (node) node->bestEval = ttScore.v;
                            if (node) node->nodeType = NodeTT;
#endif
                            return ttScore.v; }
                        current.max(ttScore.v); }
                    else if (subentry[hiBound]) {
                        WorkThread::stats.ttbeta++;
                        beta.max(ttScore.v); 
                        if (alpha >= beta.v) {
#ifdef QT_GUI_LIB
                            if (node) node->bestEval = ttScore.v;
                            if (node) node->nodeType = NodeTT;
#endif
                            return ttScore.v; } } }

                current.m = Move(subentry[from], subentry[to], subentry[piece], subentry[capture], subentry[special]);
#ifdef MYDEBUG
                if (current.m.piece()) {
                    if (current.m.isSpecial() && (current.m.piece()&7) <= Pawn) {
                        ASSERT((b.template getPieces<C,Pawn>() & 1ULL<<current.m.from()));
                        if ((current.m.piece()&7) <= Knight) {
                            if (current.m.capture())
                                ASSERT(b.template getPieces<-C>(current.m.capture()) & 1ULL<<current.m.to());
                            else
                                ASSERT((b.occupied1 & 1ULL<<current.m.to()) == 0);
                        }
                    } else {
                        ASSERT(b.template getPieces<C>(current.m.piece() & 7) & 1ULL<<current.m.from());
                        if (current.m.capture())
                            ASSERT(b.template getPieces<-C>(current.m.capture()) & 1ULL<<current.m.to());
                        else
                            ASSERT((b.occupied1 & 1ULL<<current.m.to()) == 0);
                    }
                }
#endif                
            } }
    bool leafMaxDepth;
    if (P == leaf) leafMaxDepth=false;

    if (P==tree && Options::pruning && !b.threatened && !b.isPieceHanging(eval)) {
        if (depth == eval.dMaxExt + 1) { //TODO fall back to qsearch instead of hard prune
            Score<C> fScore;
            fScore.v = eval.quantize(b.estScore - C*eval.prune1 - !!b.m.capture()*C*eval.prune1c);
#ifdef QT_GUI_LIB
            if (node) node->bestEval = fScore.v;
            if (node) node->nodeType = NodeFutile1;
#endif

            if (fScore >= beta.v && (fScore<limit<C>() || eval.allowLazyRoot)) {
                return fScore.v; } }
        if (depth == eval.dMaxExt + 2) {
            Score<C> fScore;
            fScore.v = eval.quantize(b.estScore - C*eval.prune2 - !!b.m.capture()*C*eval.prune2c);
#ifdef QT_GUI_LIB
            if (node) node->bestEval = fScore.v;
            if (node) node->nodeType = NodeFutile2;
#endif
            if (fScore >= beta.v && (fScore<limit<C>() || eval.allowLazyRoot)) {
                return fScore.v; } }
        if (depth == eval.dMaxExt + 3) {
            Score< C> alpha3(beta.v + C*(eval.prune3-1));
            Score<(Colors)-C> beta3(beta.v + C*eval.prune3);
            if (Score<C>(b.estScore) >= beta3.v) {
                bool unused;
                Score<C> v(search3<C, leaf>(b, eval.dMaxExt, alpha3, beta3, ExtNot, unused, nt NODE));
                if (v >= beta3.v) {
                    WorkThread::stats.node--;
                    return v.v;
                }
            }
                 
        }
    }
    
    if (0 && P==tree && !(extend & ExtFutility)) {
        if (depth >= 3+eval.dMaxExt) {
            int limit = -C*1000/(depth-eval.dMaxExt);
            if (alpha > limit) {
                Score< C> alpha3(limit);
                Score<(Colors)-C> beta3(limit+C);
                bool unused;
                WorkThread::stats.node--;
                Score<C> v(search3<C, tree>(b, (depth-eval.dMaxExt+1)/2 + eval.dMaxExt, alpha3, beta3, ExtFutility, unused, nt NODE));
                if (v <= alpha3.v) {
                    WorkThread::stats.node--;
                    return v.v;
                }
            }    
        }
    }

    int wattack, battack;
    ASSERT(eval.calc(b, b.matIndex, CompoundScore(b.keyScore.score)) == b.psValue);
    Eval::Material mat=eval.material[b.matIndex];
    if (P != vein && P!=leaf) {
        if (mat.reduce) {
//                depth = eval.dMaxExt + (depth-eval.dMaxExt)/4;
        } }
    if (mat.draw) {
#ifdef QT_GUI_LIB
        if (node) node->bestEval = 0;
        if (node) node->nodeType = NodeEndgameEval;
#endif
        return 0; }

    if (P==vein) {
        ASSERT(!(b.threatened & ~ExtCheck)); };


    WorkThread::stats.eval++;
    int realScore;
    if (posScore == (signed)0xdeadbeaf)
        realScore = eval(b, wattack, battack, b.psValue, b.positionalScore);
    else {
        b.positionalScore = posScore;
        realScore = eval.quantize(b.psValue + b.positionalScore); }
    
//        Options::debug = 0;
#ifdef QT_GUI_LIB
    if (node) node->real = realScore;
    if (node) node->pos = b.positionalScore;
#endif
    if (isMain & !b.m.isSpecial()) {
        int diff2 = b.positionalScore - b.prevPositionalScore;
        *b.diff = (diff2 + *b.diff)/2; 
    }
    /*
     * If we are in one of the q searches and are not threatened, 
     * assume a stand pat score. Without this score we need to make sure to
     * generate *all* moves later in movelist.
     */
    if (P==vein && b.threatened) ASSERT(extend & ExtTestMate && inCheck);
    if ((P==vein || P==leaf) && !b.threatened) {
        alpha.max(realScore);
        current.max(realScore);
        if (current >= beta.v && !b.threatened) {
            WorkThread::stats.leafcut++;
#ifdef QT_GUI_LIB
            if (node) node->nodeType = NodePrecut3;
#endif
            goto storeAndExit; }
    }


    if (P == vein && !depth) return current.v;
    ASSERT(depth);
    {
    	MoveList<C,P> i(b, current.m);
    	if (i.isEmpty()) {
    	    if ((P==vein || P==leaf) && !b.threatened) {
                // we have no moves, but the list wasn't generated as complete list
                // assume a stand pat move score.
                if (current >= beta.v) {
                    WorkThread::stats.leafcut++;
                    ASSERT(!"should not happen");
    #ifdef QT_GUI_LIB
                    if (node) node->nodeType = NodeStandpat;
    #endif
                    ASSERT(current.m.piece() || !current.m.fromto()); }
                goto storeAndExit; }
    	    
            if (inCheck) {
                if (P==vein) ASSERT(b.threatened == ExtCheck);
                if (P==vein) ASSERT(extend & ExtTestMate);
#ifdef QT_GUI_LIB
                if (node) node->nodeType = NodeMate;
                if (node) node->bestEval = -infinity*C;
#endif
                current.v = -infinity*C; 
                ASSERT(!current.m.isValid()); 
                goto storeAndExit; }
            // stalemate
#ifdef QT_GUI_LIB
            if (node) node->nodeType = NodeMate;
            if (node) node->bestEval = 0;
#endif
            current.v = 0;
            ASSERT(!current.m.isValid()); 
            goto storeAndExit; }

        bool unused __attribute__((unused));
        Extension leafExt = b.threatened;
        if (P != vein && inCheck) {
            ASSERT ( depth >= eval.dMaxCapture );
            if (i.size() == 2) {
                if (depth > eval.dMinDualExt)
                    leafExt = (Extension) (leafExt | ExtDualReply); }
            else if (i.size() == 1) 
                if (depth > eval.dMinSingleExt)
                    leafExt = (Extension) (leafExt | ExtSingleReply); }
        /*
         * The inner move loop
         */
        if (P!=vein) store(z, b.ply);
        NodeType nextNT = (NodeType)-nt;
        unsigned newDepth = depth - 1;
        do {
            ASSERT(!i.isEmpty());
            if (!origBeta.isNotShared) beta.max(origBeta.v);
            if (!origAlpha.isNotShared) alpha.max(origAlpha.v);
//             if (P != vein && i == good+1) {
//                 if (current.v > 0)
//                     leafExt = (Extension) (leafExt & ~ExtDualReply);
//             }
            Score<C> value;
            KeyScore estimate;
            estimate.vector = eval.estimate<C>(*i, b.keyScore);
            unsigned estmatIndex = eval.estimate<C>(*i, b.matIndex);
            if (P==vein || (P==leaf && (i.isCapture() | i.isNonCapture())
                                    && !b.threatened
                                    && !(extend & (ExtDualReply | ExtSingleReply)))
                        || (P==leaf && newDepth <= eval.dMaxCapture && !i.isMate())) {
                int nextPSValue = eval.calc(b.swapped() , estmatIndex, CompoundScore(estimate.vector));
                int nextEstimatedScore = eval.quantize(nextPSValue + b.positionalScore + C*eval.standardError);                
                value.v = nextEstimatedScore;
                if (value <= alpha.v) {
                    WorkThread::stats.node++;
//                    if (WorkThread::stats.node ==  432) asm("int3");
                    WorkThread::stats.leafcut++;
                    current.max(value.v);
#ifdef QT_GUI_LIB
                    int16_t diff = pe[6 + C*(i->piece() & 7)][i->from()][i->to()];
                    if (NodeItem::nNodes < MAX_NODES && node) {
                        NodeItem::m.lock();
                        NodeData data = { };
                        data.alpha = beta.v;
                        data.beta = alpha.v;
                        data.move = *i;
                        data.ply = b.ply;
                        data.key = estimate.key;
                        data.searchType = P;
                        data.depth = depth;
                        data.moveColor = C;
                        data.nodeColor = -C;
                        data.flags = 0;
                        data.threadId = WorkThread::getThisThreadId();
                        data.nodes = 1;
                
                        data.gain = diff;
                        data.error = eval.standardError;
                        data.ps = nextPSValue;
                        data.real = 0;
                        data.bestEval = value.v;
                        data.nodeType = NodePrecut3;

                        new NodeItem(data, node);
                        NodeItem::nNodes++;
                        NodeItem::m.unlock(); }
#endif
                    continue; } 
//                if (P == leaf && (i.isCapture() | i.isNonCapture())
//                    && !b.threatened
//                    && !(extend & (ExtDualReply | ExtSingleReply)) 
//                    && !i->capture() && !i->isSpecial()) continue;
            }
            if (P != vein) tt->prefetchSubTable(estimate.key - C+1);
            const ColoredBoard<(Colors)-C> nextboard(b, *i, estimate.vector);
            nextboard.psValue = eval.calc(nextboard , nextboard.matIndex, CompoundScore(estimate.vector));
            nextboard.diff = & pe[6 + C*(i->piece() & 7)][i->from()][i->to()];
            nextboard.prevPositionalScore = b.positionalScore;
            nextboard.estScore = nextboard.psValue + b.positionalScore + *nextboard.diff;
            /*
             * Quiescense search, only material changing moves, other moves
             * aren't even generated
             */
            if (P == vein)  {
            	ASSERT(i->capture() || i->isSpecial() || extend & ExtTestMate);
                ASSERT(newDepth < eval.dMaxCapture);
                value.v = search3<(Colors)-C, vein>(nextboard, newDepth, beta.unshared(), alpha.unshared(), ExtNot, unused, nextNT NODE);
                /*
                 * Transition from extended q search to pure q search for
                 * for non-tactical moves
                 */
            }
            else if (P == leaf && (i.isCapture() | i.isNonCapture())
                     && !b.threatened
                     && !(extend & (ExtDualReply | ExtSingleReply)) //TODO replace by a condition "not checking move"
                    ) {
                if (!i->capture() && !i->isSpecial()) continue;
                value.v = search3<(Colors)-C, vein>(nextboard, eval.dMaxCapture, beta.unshared(), alpha.unshared(), ExtNot, unused, nextNT NODE);
                /*
                 * Transition from extended q search to pure q search for tactical
                 * moves, if the move is a possible mate, don't do a lazy eval in
                 * the next iteration
                 */
            }
            else if (P == leaf && newDepth <= eval.dMaxCapture) {
                ASSERT(newDepth == eval.dMaxCapture);
                leafExt = i.isMate() ? ExtTestMate : ExtNot;
                value.v = search3<(Colors)-C, vein>(nextboard, newDepth, beta.unshared(), alpha.unshared(), leafExt, unused, nextNT NODE);
                leafMaxDepth = true; }
            else if ( P == leaf || newDepth <= eval.dMaxExt) {
                value.v = search3<(Colors)-C, leaf>(nextboard, newDepth, beta.unshared(), alpha.unshared(), leafExt, leafMaxDepth, nextNT NODE); }
            else {   // possible null search in tree or trunk
                ASSERT(P != leaf);
                bool ninf = current.v == -C*infinity;
                bool doNull = newDepth >= eval.dMinReduction
                              && !ninf
                              && eval.material[nextboard.matIndex].doNull;
                int reduction = !ninf && calcReduction(b, i.index(), *i, depth);
                if (P == trunk && !alpha.isNotShared && depth > Options::splitDepth + eval.dMaxExt && (nt == NodeFailLow || i.index()) && WorkThread::canQueued(current.isNotReady())) {
                    WorkThread::queueJob(new SearchJob<(Colors)-C, B, A>(*this, nextboard, doNull, reduction, newDepth,
                                                 beta, alpha, current, WorkThread::getThisThreadId(), keys, nextNT NODE));
                    if (searchState != Running) break;
                    continue; }
                else {
                    value.v = search9<(Colors)-C, P>(doNull, reduction, nextboard, newDepth, beta, alpha, ExtNot, nextNT NODE);
                } }

            if (searchState != Running) break;
            nextNT = NodeFailHigh;
            alpha.max(value.v);
            current.max(value.v, *i); 
        } while(current < beta.v && ++i);

        // if there are queued jobs started from _this_ node, do them first
        // don't start threads from parent search but with same therad id here
        if (P == trunk) {
            WorkThread::executeOldJobs(b.ply);
//        while(current.isNotReady() && (job = WorkThread::getChildJob(zd))) job->job();
        }

        alpha.join();
        current.join(); }
storeAndExit:
    if (P != vein) {
        TTEntry stored;
        stored.bitfield = 0;
        if (P == leaf) {
            ASSERT(depth <= eval.dMaxExt);
            if (!leafMaxDepth) //TODO dMinDual does not update hashMaxDepth correctly
                stored.set(::depth, eval.dMaxExt);
            else {
                stored.set(::depth, depth);
                nextMaxDepth = true; } }
        else
            stored.set(::depth, depth);


        stored.upperKey = z >> 32;
        stored.set(key11, (uint32_t)z >> (32-key11.size));
        stored.set(score, score2tt(current.v));

        stored.set(loBound, current > origAlpha.v);
        if (current > origAlpha.v && current.m.capture() == 0 && current.m.piece()/* && !current.m.isSpecial()*/) {
            ASSERT(current.m.fromto());
            history.good(current.m, b.ply + rootPly); }
        stored.set(hiBound, (current < origBeta.v) & (searchState == Running));
        stored.set(from, current.m.from());
        stored.set(to, current.m.to());
        stored.set(capture, current.m.capture()); 
        stored.set(piece, current.m.piece() & 7); 
        stored.set(special, current.m.isSpecial());
        stored.set(::posScore, score2tt(b.positionalScore));
        tt->store(st, stored); }
#ifdef QT_GUI_LIB
    if (node) {
        node->bestEval = current.v;
        for (int i=0; i<node->childCount(); ++i)
            node->nodes += node->child(i)->nodes; }
#endif
    return current.v; }
#endif
