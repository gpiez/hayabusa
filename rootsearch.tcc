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
    stopSearch = Running;
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
    if (nMoves == 1) stopSearch = Stopping;
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
    uint64_t subnode = stats.node;
    int dummy __attribute__((unused));
    alpha0.v = search4<(Colors)-C, tree, SharedScore<(Colors)-C>, SharedScore<C>, C>(b, *ml, dMaxExt, beta, alpha0, ply+1, ExtNot, dummy NODE);
    ml.nodesCount(stats.node - subnode);
    for (++ml; ml.isValid() && stopSearch == Running; ++ml) {
        uint64_t subnode = stats.node;
        Score<C> value;
        value.v = search4<(Colors)-C, tree>(b, *ml, dMaxExt, beta, alpha0, ply+1, ExtNot, dummy NODE);
        if (value > alpha0.v) {
            alpha0.v = value.v;
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
    for (depth=dMaxExt+2; depth<=endDepth && stats.node < maxSearchNodes && stopSearch == Running; depth++) {
        ml.begin();
        ml.sort(bestMovesLimit);
        bestMove = currentMove = *ml;
        currentMoveIndex = 1;
        if (!Options::humanreadable && !Options::quiet) {
            std::stringstream g;
            g << "info depth " << depth-dMaxExt;
            console->send(g.str());
        }
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
        SharedScore<(Colors)-C> beta2;
        Score<C>            value;
        uint64_t subnode = stats.node;
        system_clock::time_point now;
        if (alpha0.v != -infinity*C && abs(alpha0.v) < 1024) {
            SharedScore<         C> alpha2;
            alpha.v = alpha0.v;
            alpha2.v = alpha.v - eval.aspiration0*C;
            beta2.v  = alpha.v + eval.aspiration1*C;
            value.v = search4<(Colors)-C, trunk>(b, *ml, depth-1, beta2, alpha2, ply+1, ExtNot, dummy NODE);
            if (stopSearch) break;
            if (value <= alpha2.v) {
                // Fail-Low at first root, research with low bound = alpha and high bound = old low bound
                //TODO check if trying other moves first is an better option
                now = system_clock::now();
                console->send(status(now, value.v) + " upperbound");
                beta2.v = value.v;
                alpha.v = value.v;
                value.v = search4<(Colors)-C, trunk>(b, *ml, depth-1, beta2, SharedScore<C>(-infinity*C), ply+1, ExtNot, dummy NODE);
                if (stopSearch) break;
                alpha.v = value.v;
            } else if (value >= beta2.v) {
                // Fail-High at first root, research with high bound = beta and low bound = old high bound
                now = system_clock::now();
                console->send(status(now, value.v) + " lowerbound");
                alpha.v = value.v;
                if (!infinite && now > softLimit && alpha > alpha0.v + C*eval.evalHardBudget) break;
                value.v = search4<(Colors)-C, trunk>(b, *ml, depth-1, beta, alpha, ply+1, ExtNot, dummy NODE);
                if (stopSearch) break;
                alpha.v = value.v;
            } else {
                alpha.v = value.v;
            }
        } else {
            value.v = search4<(Colors)-C, trunk>(b, *ml, depth-1, beta, alpha, ply+1, ExtNot, dummy NODE);
            if (stopSearch) break;
            alpha.v = value.v;
        }
        ml.nodesCount(stats.node - subnode);
        
        now = system_clock::now();
        console->send(status(now, alpha.v));

        if (alpha >= infinity*C) break;
        if (!infinite && now > softLimit && alpha > alpha0.v + C*eval.evalHardBudget) break;
        if (abs(alpha.v) <= 1024)
            beta2.v = alpha.v + eval.aspiration1*C;
        else
            beta2.v = infinity*C;
        
        for (++ml; ml.isValid() && stopSearch == Running; ++ml) {
            currentMoveIndex++;
            currentMove = *ml;
            uint64_t subnode = stats.node;
            if (alpha.v != -infinity*C
                && depth > dMaxExt+2
                && ml.count() > maxMovesNull
                && b.material
                && stopSearch == Running)
            {
                const SharedScore<(Colors)-C> beta0(alpha.v + C);
                unsigned newDepth = depth-(nullReduction[depth]);
                value.v = search4<(Colors)-C, trunk>(b, *ml, newDepth, beta0, alpha, ply+1, ExtNot, dummy NODE);
                if (value <= -infinity*C) continue;

                if (value <= alpha.v && stopSearch == Running) {
                    newDepth = depth-(nullReduction[depth]+1);
                    Score<C> nullvalue(search4<C, trunk>(b, *ml, newDepth, alpha, beta0, ply+2, ExtNot, dummy NODE));
                    if (nullvalue <= alpha.v) continue;
                }

            }
            if (stopSearch == Running) {
                value.v = search4<(Colors)-C, trunk>(b, *ml, depth-1, beta2, alpha, ply+1, ExtNot, dummy NODE);
                if (stopSearch || value <= alpha.v) continue;
                alpha.v = value.v;
                if (ml.current >= bestMovesLimit) bestMovesLimit++;
                bestMove = *ml;
                if (alpha >= beta2.v && stopSearch == Running) {
                    now = system_clock::now();
                    console->send(status(now, alpha.v) + " lowerbound");
                    value.v = search4<(Colors)-C, trunk>(b, bestMove, depth-1, beta, alpha, ply+1, ExtNot, dummy NODE);
                    if (stopSearch) break;
                    alpha.v = value.v;
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
                if (now > softLimit && alpha > alpha0.v + C*eval.evalHardBudget) stopSearch = Stopping;
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
    
    console->send("bestmove "+bestMove.algebraic());
    UniqueLock<Mutex> l(stopSearchMutex);
    stopSearch = Stopped;
    stoppedCond.notify_one();
    return bestMove;
}
#endif
