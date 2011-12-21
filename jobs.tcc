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
// template<Colors C, typename A, typename B, typename T>
// SearchJob<C,A,B,T>::SearchJob(RootBoard& rb, const T& b, Move m, unsigned int depth, const A& alpha, B& beta, B& retval, unsigned ply, unsigned parent, const RepetitionKeys& rep, NodeType nt
// #ifdef QT_GUI_LIB
//     , NodeItem* node
// #endif
//     ): rb(rb), b(b), m(m), depth(depth), alpha(alpha), beta(beta), retval(retval), ply(ply), parent(parent), rep(rep), nt(nt)
// #ifdef QT_GUI_LIB
//     , node(node)
// #endif
// {
//     beta.setNotReady();
//     retval.setNotReady();
// };
template<Colors C, typename A, typename B, typename T>
SearchJob<C,A,B,T>::SearchJob(RootBoard& rb, const T& b, bool doNull,
                              unsigned reduction, Move m, unsigned int depth, A& alpha, const B& beta, ScoreMove<C,A>& retval,
                              unsigned ply, unsigned parent, const RepetitionKeys& rep, NodeType nt
#ifdef QT_GUI_LIB
    , NodeItem* node
#endif
    ): rb(rb), b(b), doNull(doNull), reduction(reduction), m(m), depth(depth),
    alpha(alpha), beta(beta), retval(retval), ply(ply), parent(parent), rep(rep), nt(nt)
#ifdef QT_GUI_LIB
    , node(node)
#endif
{
//     std::stringstream ss;
//     ss << "Construct Job " << this << " with board " << &b << " from " << threadId << std::endl;
//     std::cerr << ss.str();
    alpha.setNotReady();
    retval.setNotReady();
};
template<Colors C, typename A, typename B, typename T>
void SearchJob<C,A,B,T>::job() {
    if (alpha < beta.v) {
        rb.clone(b, rep, ply);
    //     int ret = rb.search3<C,trunk>(b, m, depth, alpha, beta, ply, ExtNot, dummy, nt
    //     std::stringstream ss;
    //     usleep(100000);
    //     ss << "Execute Job " << this << " with board "  << &b << " from " << threadId << std::endl;
    //     std::cerr << ss.str();
        int ret = rb.search9<(Colors)-C,trunk>(doNull, reduction, b, m, depth, beta, alpha, ply,
                                      ExtNot, nt
    #ifdef QT_GUI_LIB
                           , node
    #endif
                           );
    //     ss.str("");
    //     ss << "Finished Job " << this << " with board "  << &b << " from " << threadId << std::endl;
    //     std::cerr << ss.str();

        alpha.max(ret);
        retval.max(ret, m);
    } else
        ++stats.cancelJob;
    alpha.setReady();
    retval.setReady();
}
#endif

template<Colors C>
RootSearchJob<C>::RootSearchJob(RootBoard& rb, const RepetitionKeys& rep, unsigned depth): rb(rb), rep(rep), depth(depth) {};

template<Colors C>
void RootSearchJob<C>::job() {
    threadId = 0;
    rb.clone<C>(rb.currentBoard<C>(), rep, 0);
    rb.rootSearch<C>(depth);
}

template<Colors C>
void RootSearchJob<C>::stop() {
    rb.stop();
}