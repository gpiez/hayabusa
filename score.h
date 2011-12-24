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
#ifndef SCORE_H
#define SCORE_H

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"
#include "move.h"

typedef int16_t RawScore;
class BoardBase;

// struct PlainScore {
//     int         v;
// };
// 
// struct MoveScore {
//     int         v;
//     Move        m;
// };

template<Colors C> struct Score
{
    int v; //Absolute score. Less is good for black, more is good for white.

    enum { isNotShared = true };
    typedef Score<C> Base;

    Score() {};
    /* No implicit int conversion operator and constructor, they allow wrong comparisons */
    explicit Score (int a) __attribute__((__always_inline__));
    explicit Score (const Score& a) __attribute__((__always_inline__));
    bool operator >= (int a) const __attribute__((__always_inline__));
    bool operator <= (int a) const __attribute__((__always_inline__));
    bool operator > (int a) const __attribute__((__always_inline__));
    bool operator < (int a) const __attribute__((__always_inline__));
    bool max(const int b) __attribute__((__always_inline__));
    static std::string str(int v);

    Score<C>& unshared() __attribute__((__always_inline__));
    const Score<C>& unshared() const __attribute__((__always_inline__));
    unsigned int isNotReady() const __attribute__((__always_inline__));
    void join() const {};
    void setReady() {};
    void setNotReady() {};
};

template<Colors C> struct VolatileScore {
     typedef volatile Score<C> Type;
};

template<Colors C> struct SharedScore: public VolatileScore<C>::Type
{
    using Score<C>::v;

    typedef Score<C> Base;

    enum { isNotShared = false };

    unsigned int notReady;
    mutable int nChildren;
    const SharedScore<C>* parent;
    mutable SharedScore<C>* children[maxScoreChildren];
    RecursiveMutex valueMutex;
    Condition readyCond;
    Mutex readyMutex;
    mutable Mutex childrenMutex;
//    SharedScore<C>* depending;

public:
    SharedScore():
        notReady(0),
        nChildren(0),
        parent(nullptr),
        children({nullptr})
    {}
    ~SharedScore() {
        ASSERT(!notReady);
        ASSERT(!nChildren);
        if (parent) parent->deleteChild(this);
    }

    // construct a shared score depending on the parameter
    // if the parameter score gets a better value, a new
    // maximum is calculated for this score, and if it changes
    // for its depending scores too.
    explicit SharedScore(const SharedScore& a):
        Score<C>(a),
        notReady(0),
        nChildren(0),
        parent(&a),
        children({nullptr})

    {
        a.addChild(this);
    }

    explicit SharedScore(int a):
        Score<C>(a),
        notReady(0),
        nChildren(0),
        parent(nullptr),
        children({nullptr})
    {
    }

    const Score<C>& unshared() const {
        return *this;
    }
    
    Score<C>& unshared() {
        return *this;
    }

    void join();

    unsigned int isNotReady() const {
        return notReady;
    }

    bool max(const int b);
    void setReady();
    void setNotReady();
    void addChild(SharedScore<C>*) const;
    void deleteChild(SharedScore<C>*) const;
    void maximizeChildren(int) const;
};

/*
 * Score with bestMove, not used
 * only the partial specializations below are needed
 */
template<Colors C, typename T > struct ScoreMove;

template<Colors C> struct ScoreMove<C, Score<C> >: public Score<C> {
    using Score<C>::v;
    Move m;
    using Score<C>::max;
    bool max(const int b, Move bm);
};

template<Colors C> struct ScoreMove<C, SharedScore<C> >: public  SharedScore<C> {
    using SharedScore<C>::v;
    Move m;
    using SharedScore<C>::max;
    bool max(const int b, Move bm);
};

// template<Colors C> struct ScoreMove: public Score<C, MoveScore> {
//     using SharedScore<C, MoveScore>::v;
//     using SharedScore<C, MoveScore>::m;
//     bool max(const int b) = delete;
//     bool max(const int b, Move bm);
// };

#endif // SCORE_H
