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
#ifndef JOBS_TCC_
#define JOBS_TCC_

#include "jobs.h"
template<Colors C, typename A, typename B, typename T>
SearchJob<C,A,B,T>::SearchJob(Game& rb, const T& b, bool doNull,
                              unsigned reduction, Move m, unsigned int depth, A& alpha, const B& beta, ScoreMove<C,A>& retval,
                              unsigned ply, unsigned parent, const RepetitionKeys& rep, NodeType nt
#ifdef QT_GUI_LIB
                              , NodeItem* node
#endif
                             ): game(rb), b(b), doNull(doNull), reduction(reduction), m(m), depth(depth),
    alpha(alpha), beta(beta), retval(retval), ply(ply), parent(parent), rep(rep), nt(nt)
#ifdef QT_GUI_LIB
    , node(node)
#endif
{
    alpha.setNotReady();
    retval.setNotReady();
#ifdef QT_GUI_LIB
    if (NodeItem::nNodes < MAX_NODES && node) {
        NodeData data = { };
        data.alpha = alpha.v;
        data.beta = beta.v;
        data.move = m;
        data.ply = ply;
        data.searchType = trunk;
        data.depth = depth;
        data.moveColor = b.CI == 0 ? White:Black;
        data.nodeColor = C;
        data.flags = 0;
        data.threadId = WorkThread::threadId;
        data.nodes = 0;
        data.nodeType = NodeStart;
        NodeItem::m.lock();
        startnode = new NodeItem(data, node);
        NodeItem::nNodes++;
        NodeItem::m.unlock(); }
    else
        startnode = nullptr;
#endif

};

template<Colors C, typename A, typename B, typename T>
void SearchJob<C,A,B,T>::job() {
#ifdef QT_GUI_LIB
    if (startnode) startnode->threadId = WorkThread::threadId;
#endif
    if (alpha < beta.v) {
        game.clone(b, rep, ply);
        const ColoredBoard<(Colors)-C> nextboard(b, m, game);
        int ret = game.search9<(Colors)-C,trunk>(doNull, reduction, nextboard, m, depth, beta, alpha, ply,
                  ExtNot, nt NODE 
                                                );
        alpha.max(ret);
        retval.max(ret, m); }
    else
        ++stats.cancelJob;
    alpha.setReady();
    retval.setReady(); }
#endif

template<Colors C>
void RootPerftJob<C>::job() {
    uint64_t n=game.rootPerft<C>(depth);
    std::ostringstream temp;
    temp << n;
    game.console->send(temp.str()); }

template<Colors C>
void RootDivideJob<C>::job() {
    uint64_t n=game.rootDivide<C>(depth);
    std::ostringstream temp;
    temp << n;
    game.console->send(temp.str()); }

template<Colors C>
RootSearchJob<C>::RootSearchJob(Game& rb, const RepetitionKeys& rep, unsigned depth): game(rb), rep(rep), depth(depth) {};

template<Colors C>
void RootSearchJob<C>::job() {
    game.clone<C>(game.currentBoard<C>(), rep, 0);
    game.rootSearch<C>(depth); }

template<Colors C>
void RootSearchJob<C>::stop() {
    game.stop(); }
