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

template<Colors C> struct ScoreBase
{
    RawScore v; //Absolute score. Less is good for black, more is good for white.

    enum { isNotShared = true };
    
    ScoreBase() {};
    explicit ScoreBase (const ScoreBase& a) { v = a.v; };
    
    bool operator >= (RawScore a) const {
        if ( C==White ) return v>=a;
        else            return v<=a;
    }
    bool operator > (RawScore a) const {
        if ( C==White ) return v>a;
        else            return v<a;
    }
    bool operator < (RawScore a) const {
        if ( C==White ) return v<a;
        else            return v>a;
    }
    void max(const RawScore b) {
        if (*this < b) 
            v = b;
    }
};

template<Colors C> struct Score: ScoreBase<C>
{
    using ScoreBase<C>::v;
    Move m;

    enum { isNotShared = true };

    typedef Score<C> Base;

    Score() {};
    explicit Score (const Score& a):
        ScoreBase<C>(a) {
        m.data = 0;
    };
    
    void join() const {};
    unsigned int isNotReady() const { return 0; }
    void setReady() {};
    void setNotReady() {};

    using ScoreBase<C>::max;
    
    bool max(const RawScore b, const Move n)         {
        if (*this < b) {
            v = b;
            m = n;
            return true;
        }
        return false;
    }
};

template<Colors C> struct SharedScore: public Score<C>
{
    using Score<C>::v;
    using Score<C>::m;
    typedef Score<C> Base;
    
    enum { isNotShared = false };
    
    volatile unsigned int notReady;
    std::recursive_mutex valueMutex;
    std::condition_variable readyCond;
    std::mutex readyMutex;
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

    void join();

    unsigned int isNotReady() const {
        return notReady;
    }

    bool max(const RawScore b)         {
        std::lock_guard<std::recursive_mutex> lock(valueMutex);
        if (*this < b) {
            v = b;
            return true;
        }
        return false;
    }
    bool max(const RawScore b, const Move n)         {
        std::lock_guard<std::recursive_mutex> lock(valueMutex);
        if (*this < b) {
            v = b;
            m = n;
            return true;
        }
        return false;
    }

    void setReady() {
        std::lock_guard<std::mutex> lock(readyMutex);
        --notReady;
//        ASSERT(notReady <= 6);
        readyCond.notify_one();
    }

    void setNotReady() {
        std::lock_guard<std::mutex> lock(readyMutex);
        ++notReady;
//        ASSERT(notReady <= 6);    //queued jobs + running
    }
};


#endif // SCORE_H
