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
#ifndef ROOTSEARCH_TCC_
#define ROOTSEARCH_TCC_

#include "search.tcc"
#include "sortedmovelist.h"

template<Colors C>
Move RootBoard::rootSearch(unsigned int endDepth) {
    start = system_clock::now();
    
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
    SortedMoveList ml(b);

    milliseconds time( C==White ? wtime : btime );
    milliseconds inc( C==White ? winc : binc );
    
    if (movestogo == 0) movestogo = 20;
    
    if (time > operatorTime) time -= operatorTime;
    else time = milliseconds(0);
    
    time = std::max(minimumTime, time);
    
    system_clock::time_point softLimit = start + (time + inc*movestogo)/(2*movestogo);
    milliseconds hardBudget = std::min(time/2, (2*time + 2*inc*movestogo)/(movestogo+1));
    stopTimerMutex.lock();
    Thread* stopTimerThread = NULL;
    if (!infinite) stopTimerThread = new Thread(&RootBoard::stopTimer, this, hardBudget);
    
    infoTimerMutex.lock();
    Thread* infoTimerThread = NULL;
    if (!Options::quiet) infoTimerThread = new Thread(&RootBoard::infoTimer, this, milliseconds(1000));
    
    nMoves = ml.count();
    ml.begin();
    bestMove = *ml;
    if (nMoves == 1) stopSearch = true;
    #ifdef QT_GUI_LIB
        NodeData data;
        data.move.data = 0;
        data.ply = 1;
        NodeItem::m.lock();
        NodeItem* node=0;
        uint64_t startnode = NodeItem::nNodes;
        if (NodeItem::nNodes < MAX_NODES && NodeItem::nNodes >= MIN_NODES) {
            node = new NodeItem(data, statWidget->tree->root());
            NodeItem::nNodes++;
        }
        NodeItem::m.unlock();
    #endif

    SharedScore<C> alpha0; alpha0.v = -infinity*C;
    SharedScore<(Colors)-C> beta;  beta.v  =  infinity*C;    //both alpha and beta are lower limits, viewed from th color to move
    bool maxDepth = false;
    uint64_t subnode = stats.node;
    search<(Colors)-C, tree>(b, *ml, dMaxExt, beta, alpha0, ply+1, ExtNot, maxDepth NODE);
    ml.nodesCount(stats.node - subnode);
    for (++ml; ml.isValid() && !stopSearch; ++ml) {
        uint64_t subnode = stats.node;
        if (search<(Colors)-C, tree>(b, *ml, dMaxExt, beta, alpha0, ply+1, ExtNot, maxDepth NODE)) {
            bestMove = *ml;
            ml.nodesCount(stats.node - subnode);
            ml.currentToFront();            
        }
    }
#ifdef QT_GUI_LIB
    if (node) {
        node->nodes = stats.node - startnode;
    }
#endif

    unsigned bestMovesLimit = 1;
    for (depth=dMaxExt+2; depth<endDepth+dMaxExt && stats.node < maxSearchNodes && !stopSearch; depth++) {
        ml.begin();
        ml.sort(bestMovesLimit);
        bestMove = currentMove = *ml;
        currentMoveIndex = 1;
        std::stringstream g;
        g << "info depth " << depth-dMaxExt;
        console->send(g.str());
    #ifdef QT_GUI_LIB
        NodeData data;
        data.move.data = 0;
        data.ply = depth-dMaxExt;
        NodeItem::m.lock();
        NodeItem* node=0;
        uint64_t startnode = NodeItem::nNodes;
        if (NodeItem::nNodes < MAX_NODES && NodeItem::nNodes >= MIN_NODES) {
            node = new NodeItem(data, statWidget->tree->root());
            NodeItem::nNodes++;
        }
        NodeItem::m.unlock();
    #endif

        SharedScore<        C>  alpha; alpha.v = -infinity*C;
        SharedScore<(Colors)-C> beta;  beta.v  =  infinity*C;    //both alpha and beta are lower limits, viewed from th color to move
        SharedScore<         C> alpha2;
        SharedScore<(Colors)-C> beta2 ;
        uint64_t subnode = stats.node;
        system_clock::time_point now;
        if (alpha0.v != -infinity*C && abs(alpha0.v) < 1024) {
            alpha2.v = alpha0.v - eval.aspiration0*C;
            beta2.v  = alpha0.v + eval.aspiration1*C;
            if (!search<(Colors)-C, trunk>(b, *ml, depth-1, beta2, alpha2, ply+1, ExtNot, maxDepth NODE)) {
                // Fail-Low at first root, research with low bound = alpha and high bound = old low bound
                now = system_clock::now();
                console->send(status(now, alpha2.v) + " upperbound");
                if (stopSearch) {
                    alpha.v = alpha0.v;
                    break;   //TODO check if trying other moves first is an better option
                }
                beta2.v = alpha2.v;
                search<(Colors)-C, trunk>(b, *ml, depth-1, beta2, alpha, ply+1, ExtNot, maxDepth NODE);
            } else if (alpha2 >= beta2.v) {
                // Fail-High at first root, research with high bound = beta and low bound = old high bound
                now = system_clock::now();
                console->send(status(now, alpha2.v) + " lowerbound");
                if (stopSearch) {
                    alpha.v = alpha0.v;
                    break;   //TODO check if trying other moves first is an better option
                }
                alpha.v = alpha2.v;
                if (!infinite && now > softLimit && alpha > alpha0.v + C*eval.evalHardBudget) break;
                search<(Colors)-C, trunk>(b, *ml, depth-1, beta, alpha, ply+1, ExtNot, maxDepth NODE);
            } else {
                alpha.v = alpha2.v;
            }
        } else {
            search<(Colors)-C, trunk>(b, *ml, depth-1, beta, alpha, ply+1, ExtNot, maxDepth NODE);
        }
        ml.nodesCount(stats.node - subnode);
        
        if (stopSearch) {
            alpha.v = alpha0.v;
            break;   //TODO check if trying other moves first is an better option
        }
        
        now = system_clock::now();
        console->send(status(now, alpha.v));

        if (alpha >= infinity*C) break;
        if (!infinite && now > softLimit && alpha > alpha0.v + C*eval.evalHardBudget) break;
        if (abs(alpha.v) <= 1024)
            beta2.v = alpha.v + eval.aspiration1*C;
        else
            beta2.v = infinity*C;
        
        for (++ml; ml.isValid() && !stopSearch; ++ml) {
            currentMoveIndex++;
            currentMove = *ml;
            uint64_t subnode = stats.node;
            bool pruneNull = false;
            if (alpha.v != -infinity*C
                && depth > dMaxExt+2
                && ml.count() > maxMovesNull
                && !stopSearch)
            {
                SharedScore<(Colors)-C> null;
                SharedScore<C> nalpha(alpha);
                null.v = alpha.v + C;
                if (depth > nullReduction+1 + dMaxExt)
                    search<C, trunk>(b, *ml, depth-(nullReduction+1), nalpha, null, ply+2, ExtNot, maxDepth NODE);
                else
                    search<C, leaf>(b, *ml, depth-(nullReduction+1), nalpha, null, ply+2, ExtNot, maxDepth NODE);
                pruneNull = alpha >= null.v;
                if (!stopSearch && pruneNull && depth > 2*(nullReduction) + dMaxExt) {
                    null.v = alpha.v + C;
                    search<(Colors)-C, trunk>(b, *ml, depth-2*(nullReduction), null, nalpha, ply+1, ExtNot, maxDepth NODE);
                    pruneNull = alpha >= nalpha.v;
                }
            }
            if (!pruneNull && !stopSearch && search<(Colors)-C, trunk>(b, *ml, depth-1, beta2, alpha, ply+1, ExtNot, maxDepth NODE)) {
                if (ml.current >= bestMovesLimit) bestMovesLimit++;
                bestMove = *ml;
                if (alpha >= beta2.v && !stopSearch) {
                    now = system_clock::now();
                    console->send(status(now, alpha.v) + " lowerbound");
                    search<(Colors)-C, trunk>(b, bestMove, depth-1, beta, alpha, ply+1, ExtNot, maxDepth NODE);
                }
                ml.nodesCount(stats.node - subnode);
                ml.currentToFront();
                beta2.v = alpha.v + eval.aspiration1*C;
                now = system_clock::now();
                console->send(status(now, alpha.v));
            } else {
                ml.nodesCount(stats.node - subnode);
            }
            if (alpha >= infinity*C) break;
            if (!infinite) {
                system_clock::time_point now = system_clock::now();
                if (now > softLimit && alpha > alpha0.v + C*eval.evalHardBudget) stopSearch = true;
            }
        }
//         now = system_clock::now();
//         console->send(status(now, alpha.v) + " done");

#ifdef QT_GUI_LIB
    if (node) {
        node->nodes = stats.node - startnode;
    }
#endif
        if (alpha >= infinity*C) break;
        if (alpha <= -infinity*C) break;
        alpha0.v = alpha.v;
    }
    
    infoTimerMutex.unlock();
    if (infoTimerThread) infoTimerThread->join();
    delete infoTimerThread;
    
    stopTimerMutex.unlock();
    if (stopTimerThread) stopTimerThread->join();
    delete stopTimerThread;
    
    stopSearch = false;
    console->send("bestmove "+bestMove.algebraic());
    return bestMove;
}
#endif
