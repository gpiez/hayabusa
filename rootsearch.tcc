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

template<Colors C>
Move RootBoard::rootSearch(unsigned int endDepth) {
    start = system_clock::now();
    
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
    
    if (movestogo == 0)
        movestogo = 40;
    time -= operatorTime;
    time = std::max(minimumTime, time);
    
    system_clock::time_point softLimit = start + (time + inc*movestogo)/(2*movestogo);
    milliseconds hardBudget = std::min(time/2, (2*time + 2*inc*movestogo)/(movestogo+1));

    stopTimerMutex.lock();
    Thread* stopTimerThread = NULL;
    if (!infinite) stopTimerThread = new Thread(&RootBoard::stopTimer, this, hardBudget);
    
    infoTimerMutex.lock();
    Thread infoTimerThread(&RootBoard::infoTimer, this, milliseconds(1000));
    
    SharedScore<C> alpha0; alpha0.v = -infinity*C;
    nMoves = ml.count();
    for (depth=dMaxCapture+dMaxThreat+2; depth<endDepth+dMaxCapture+dMaxThreat && stats.node < maxSearchNodes && !stopSearch; depth++) {
        ml.begin();
        bestMove = *ml;
        system_clock::time_point now = system_clock::now();
        if (now >= lastStatus2) {
            std::stringstream g;
            lastStatus2 = now + milliseconds(1000);
            g << "info depth " << depth-(dMaxCapture+dMaxThreat) << " currmove " << (*ml).algebraic() << " currmovenumber 1";
            console->send(g.str());
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
                if (stopSearch) break;   //TODO check if trying other moves first is an better option
                beta2.v = alpha2.v;
                search<(Colors)-C, trunk>(NodePV, b, *ml, depth-1, beta2, alpha, ply+1, false NODE);
            } else if (alpha2 >= beta2.v) {
                // Fail-High at first root, research with high bound = beta and low bound = old high bound
                console->send(status(now, alpha2.v) + " lowerbound");
                if (stopSearch) break;   //TODO check if trying other moves first is an better option
                now = system_clock::now();
                if (!infinite && now > softLimit && alpha > alpha0.v + C*Eval::parameters.evalHardBudget) break;
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
        if (!infinite && now > softLimit && alpha > alpha0.v - C*Eval::parameters.evalHardBudget) break;
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
                if (!stopSearch && pruneNull && depth > 2*(nullReduction) + dMaxCapture + dMaxThreat) {
                    null.v = alpha.v + C;
                    search<(Colors)-C, trunk>(NodeFailHigh, b, *ml, depth-2*(nullReduction), null, nalpha, ply+1, false NODE);
                    pruneNull = alpha >= nalpha.v;
                }
            }
            if (!pruneNull && !stopSearch) {
                if (search<(Colors)-C, trunk>(NodeFailHigh, b, *ml, depth-1, beta2, alpha, ply+1, false NODE)) {
                    bestMove = *ml;
                    ml.currentToFront();
                    if (alpha >= beta2.v && !stopSearch) {
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
                if (now > softLimit && alpha > alpha0.v + C*Eval::parameters.evalHardBudget) break;
            }
        }
        now = system_clock::now();
        console->send(status(now, alpha.v) + " done");

        if (alpha >= infinity*C) break;
        if (alpha <= -infinity*C) break;
        alpha0.v = alpha.v;
    }
    
    infoTimerMutex.unlock();
    infoTimerThread.join();
    
    stopTimerMutex.unlock();
    if (stopTimerThread) stopTimerThread->join();
    delete stopTimerThread;
    stopSearch = false;
    console->send("bestmove "+bestMove.algebraic());
    return bestMove;
}
#endif