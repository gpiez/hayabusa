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

#include "game.h"
#include "coloredboard.h"
#include "move.h"
#include "options.h"

#ifdef QT_GUI_LIB
#include "nodeitem.h"
#endif

// Abstract base class for functions called to be executed by a different thread
// To execute a function in a different thread, create a ***Job object with the
// function parameters given in the constructor. The constructor returns
// immediatly and the job() member is executed by the WorkThread run loop
// functions started this way can not have a return value, instead parameters
// by reference have to be used, with appropriate means of joining/synching
struct Job {
    virtual ~Job() {};            // the for calling destructor of QObject in signalJob
    virtual void job() = 0;
    virtual void stop() {};
    virtual unsigned getPly() {
        return 0; }; };

template<Colors C, typename A, typename B>
class SearchJob: public Job {
    Game& game;
    const ColoredBoard<C> b;
    bool doNull;
    unsigned reduction;
    unsigned int depth;
    const A& alpha;
    B& beta;
    ScoreMove<(Colors)-C,B>& retval;
    unsigned parent;
    const RepetitionKeys& rep;  //TODO really needed? This should be always thread-local keys
    NodeType nt;
#ifdef QT_GUI_LIB
    NodeItem* node;
    NodeItem* startnode;
#endif
public:
    SearchJob(Game& rb, const ColoredBoard<C>& b, bool, unsigned, unsigned int depth, const A& alpha, B& beta,
              ScoreMove<(Colors)-C,B>& retval, unsigned parent, const RepetitionKeys& rep, NodeType nt
#ifdef QT_GUI_LIB
              , NodeItem* node
#endif
             );
    void job();
};

template<Colors C, template <Colors> class T=Score >
class RootSearchJob: public Job {
    Game& game;
    const RepetitionKeys& rep;//TODO really needed? This should be always thread-local keys
    unsigned depth;

public:
    RootSearchJob(Game& rb, const RepetitionKeys& rep, unsigned depth);
    void job();
    void stop(); };

template<Colors C, typename T>
class PerftJob: public Job {
    Game& rb;
    T& n;
    const ColoredBoard<(Colors)-C>& b;
    Move m;
    unsigned int depth;
public:
    PerftJob(Game& rb, T& n, const ColoredBoard<(Colors)-C>& b, Move m, unsigned int depth): rb(rb), n(n), b(b), m(m), depth(depth) {
//         setNotReady(n);
    };
    void job() {
        rb.perft<C, trunk>(n, b, m, depth);
//         setReady(n);
    } };

template<Colors C>
class RootPerftJob: public Job {
    Game& game;
    unsigned int depth;

public:
    RootPerftJob(Game& rb, unsigned int depth): game(rb), depth(depth) {};
    void job(); };

template<Colors C>
class RootDivideJob: public Job {
    Game& game;
    unsigned int depth;

public:
    RootDivideJob(Game& rb, unsigned int depth): game(rb), depth(depth) {};
    void job(); };

#endif /* JOBS_H_ */
