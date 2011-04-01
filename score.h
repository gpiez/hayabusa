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

template<Colors C> struct ScoreBase
{
    int v; //Absolute score. Less is good for black, more is good for white.

    enum { isNotShared = true };

    ScoreBase() {};
    explicit ScoreBase (const ScoreBase& a) __attribute__((__always_inline__));
    bool operator >= (int a) const __attribute__((__always_inline__));
    bool operator <= (int a) const __attribute__((__always_inline__));
    bool operator > (int a) const __attribute__((__always_inline__));
    bool operator < (int a) const __attribute__((__always_inline__));
    void max(const int b) __attribute__((__always_inline__));
    
    static std::string str(int v);
};

template<Colors C> struct Score: ScoreBase<C>
{
    using ScoreBase<C>::v;
    Move m;

    enum { isNotShared = true };

    typedef Score<C> Base;

    Score() {};
    explicit Score (int a) __attribute__((__always_inline__));
    explicit Score (const Score& a) __attribute__((__always_inline__));
    Score<C>& unshared() __attribute__((__always_inline__));
    unsigned int isNotReady() const __attribute__((__always_inline__));
    bool max(const int b, const Move n) __attribute__((__always_inline__));
    
    using ScoreBase<C>::max;
    void join() const {};
    void setReady() {};
    void setNotReady() {};
};

template<Colors C> struct SharedScore: public Score<C>
{
    using Score<C>::v;
    using Score<C>::m;

    typedef Score<C> Base;

    enum { isNotShared = false };

    volatile unsigned int notReady;
    RecursiveMutex valueMutex;
    Condition readyCond;
    Mutex readyMutex;
//    SharedScore<C>* depending;

public:
    SharedScore():
        notReady(0) {}
    ~SharedScore() {
        ASSERT(!notReady);
    }

    // construct a shared score depending on the parameter
    // if the parameter score gets a better value, a new
    // maximum is calculated for this score, and if it changes
    // for its depending scores too.
    explicit SharedScore(const SharedScore& a):
        Score<C>(a),
        notReady(0)
//        depending(0)
    {
    }

    explicit SharedScore(int a):
        Score<C>(a),
        notReady(0)
//        depending(0)
    {
    }

    Score<C>& unshared() {
        return *(Score<C>*)this;
    }

    void join();

    unsigned int isNotReady() const {
        return notReady;
    }

    bool max(const int b)         {
        LockGuard<RecursiveMutex> lock(valueMutex);
        if (*this < b) {
            v = b;
            return true;
        }
        return false;
    }
    bool max(const int b, const Move n)         {
        LockGuard<RecursiveMutex> lock(valueMutex);
        if (*this < b) {
            v = b;
            m = n;
            return true;
        }
        return false;
    }

    void setReady() {
        {
            LockGuard<Mutex> lock(readyMutex);
            --notReady;
        }
//        ASSERT(notReady <= 6);
        readyCond.notify_one();
    }

    void setNotReady() {
        LockGuard<Mutex> lock(readyMutex);
        ++notReady;
//        ASSERT(notReady <= 6);    //queued jobs + running
    }
};

#endif // SCORE_H
