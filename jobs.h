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
#ifndef JOBS_H_
#define JOBS_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "rootboard.h"
#include "console.h"
#include "boardbase.h"
#include "workthread.h"
#include "nodeitem.h"

// Abstract base class for functions called to be executed by a different thread
// To execute a function in a different thread, create a ***Job object with the
// function parameters given in the constructor. The constructor returns
// immediatly and the job() member is executed by the WorkThread run loop
// functions started this way can not have a return value, instead parameters
// by reference have to be used, with appropriate means of joining/synching
struct Job {
    virtual ~Job() {};            // the for calling destructor of QObject in signalJob
    virtual void job() = 0;
};

template<Colors C, typename A, typename B, typename T>
class SearchJob: public Job {
    NodeType n;
    RootBoard& rb;
    const T& b;
    Move m;
    unsigned int depth;
    const A& alpha;
    B& beta;
    unsigned ply;
    unsigned parent;
    const RepetitionKeys& rep;
#ifdef QT_GUI_LIB
    NodeItem* node;
#endif
public:
    SearchJob(NodeType n, RootBoard& rb, const T& b, Move m, unsigned int depth, const A& alpha, B& beta, unsigned ply, unsigned parent, const RepetitionKeys& rep
#ifdef QT_GUI_LIB
        , NodeItem* node
#endif
        ):
        n(n), rb(rb), b(b), m(m), depth(depth), alpha(alpha), beta(beta), ply(ply), parent(parent), rep(rep)
#ifdef QT_GUI_LIB
    , node(node)
#endif
    {
        beta.setNotReady();
    };
    void job() {
        rb.clone(b, rep, ply);
        rb.search<C, trunk>(n, b, m, depth, alpha, beta, ply, false
#ifdef QT_GUI_LIB
                        , node
#endif
                        );
        beta.setReady();
    }
};

template<Colors C>
class RootSearchJob: public Job {
    RootBoard& rb;

public:
    RootSearchJob(RootBoard& rb): rb(rb) {};
    void job() {
        threadId = 0;
        /*Move m =*/ rb.rootSearch<C>();
    }
};

template<Colors C, typename T>
class PerftJob: public Job {
    RootBoard& rb;
    T& n;
    const ColoredBoard<(Colors)-C>& b;
    Move m;
    unsigned int depth;
public:
    PerftJob(RootBoard& rb, T& n, const ColoredBoard<(Colors)-C>& b, Move m, unsigned int depth): rb(rb), n(n), b(b), m(m), depth(depth) {
        setNotReady(n);
    };
    void job() {
        rb.perft<C, trunk>(n, b, m, depth);
        setReady(n);
    }
};

template<Colors C>
class RootPerftJob: public Job {
    RootBoard& rb;
    unsigned int depth;

public:
    RootPerftJob(RootBoard& rb, unsigned int depth): rb(rb), depth(depth) {};
    void job() {
        rb.pt = new TranspositionTable<PerftEntry, 1, Key>;
        uint64_t n=rb.rootPerft<C>(depth);
        std::ostringstream temp;
        temp << n;
        rb.console->send(temp.str());
        delete rb.pt;
    }
};

template<Colors C>
class RootDivideJob: public Job {
    RootBoard& rb;
    unsigned int depth;

public:
    RootDivideJob(RootBoard& rb, unsigned int depth): rb(rb), depth(depth) {};
    void job() {
        rb.pt = new TranspositionTable<PerftEntry, 1, Key>;
        uint64_t n=rb.rootDivide<C>(depth);
        std::ostringstream temp;
        temp << n;
        rb.console->send(temp.str());
        delete rb.pt;
    }
};

#endif /* JOBS_H_ */
