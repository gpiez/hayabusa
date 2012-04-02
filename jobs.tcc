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
#include "workthread.h"
template<Colors C, typename A, typename B>
SearchJob<C,A,B>::SearchJob(Game& rb, const ColoredBoard<C>& b, bool doNull,
                              unsigned reduction, unsigned int depth, const A& alpha, B& beta, ScoreMove<(Colors)-C,B>& retval,
                              unsigned parent, const RepetitionKeys& rep, NodeType nt
#ifdef QT_GUI_LIB
                              , NodeItem* node
#endif
                             ): game(rb), b(b), doNull(doNull), reduction(reduction), depth(depth),
    alpha(alpha), beta(beta), retval(retval), parent(parent), rep(rep), nt(nt)
#ifdef QT_GUI_LIB
    , node(node)
#endif
{
    beta.setNotReady();
    retval.setNotReady();
#ifdef QT_GUI_LIB
    if (NodeItem::nNodes < MAX_NODES && node) {
        NodeData data = { };
        data.alpha = alpha.v;
        data.beta = beta.v;
        data.move = b.m;
        data.ply = b.ply;
        data.searchType = trunk;
        data.depth = depth;
        data.moveColor = -C;
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

template<Colors C, typename A, typename B>
void SearchJob<C,A,B>::job() {
    WorkThread::Unlock l;
#ifdef QT_GUI_LIB
    if (startnode) startnode->threadId = WorkThread::threadId;
#endif
    if (alpha < beta.v) {
        game.clone(b, rep, b.ply);
        int ret = game.search9<C,trunk>(doNull, reduction, b, depth, alpha, beta, ExtNot, nt NODE);
        beta.max(ret);
        retval.max(ret, b.m); }
    else
        ++WorkThread::stats.cancelJob;
    beta.setReady();
    retval.setReady(); 
    
}

template<Colors C>
void RootPerftJob<C>::job() {
    WorkThread::Unlock l;
    uint64_t n=game.rootPerft<C>(depth);
    std::ostringstream temp;
    temp << n;
    game.console->send(temp.str()); }

template<Colors C>
void RootDivideJob<C>::job() {
    WorkThread::Unlock l;
    uint64_t n=game.rootDivide<C>(depth);
    std::ostringstream temp;
    temp << n;
    game.console->send(temp.str()); }

template<Colors C, template <Colors> class T >
RootSearchJob<C,T>::RootSearchJob(Game& rb, const RepetitionKeys& rep, unsigned depth): game(rb), rep(rep), depth(depth) {};

template<Colors C, template <Colors> class T >
void RootSearchJob<C, T>::job() {
    WorkThread::Unlock l;
    game.clone<C>(game.currentBoard<C>(), rep, 0);
    game.rootSearch<C, T>(depth); }

template<Colors C, template <Colors> class T >
void RootSearchJob<C, T>::stop() {
    WorkThread::Unlock l;
    game.stop(); }

#endif
