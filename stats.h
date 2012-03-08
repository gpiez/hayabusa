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
#ifndef STATS_H_
#define STATS_H_

#include "constants.h"
#include <cstdint>

// This is thread local defined in workthread.cpp, because thread local storage
// speeds up the program by a large factor.
union Stats {
    struct {
        uint64_t    node;
        uint64_t    internalNode;
        uint64_t    eval;
        uint64_t    ttuse;
        uint64_t    tthit;
        uint64_t    ttalpha;
        uint64_t    ttbeta;
        uint64_t    ttoverwrite;
        uint64_t    ttinsufficient;
        uint64_t    ttstore;
        uint64_t    ttmerge;
        uint64_t    leafcut;
        uint64_t    pthit;
        uint64_t    ptmiss;
        uint64_t    ptuse;
        uint64_t    ptcollision;
        uint64_t    jobs;
        uint64_t    taken;
        uint64_t    notTaken;
        uint64_t    cancelJob; };
    uint64_t data[]; };

extern __thread Stats stats;
#ifdef MYDEBUG
extern uint64_t mobStat[nColors][nPieces+1][2][nSquares];
#endif

#endif
