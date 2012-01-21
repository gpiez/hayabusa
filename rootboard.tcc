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

    Current Node            Next Node
    check               ->  threatened mate/rep/material loss   -> no stand pat
    mate move           ->  threatened mate/repetition          -> no stand pat
    fork                ->  threatened material loss            -> no stand pat
    singular evasion    ->  threatening mate/rep                -> generate checks
*/
#ifndef ROOTBOARD_TCC_
#define ROOTBOARD_TCC_

#ifndef PCH_H_
#include <pch.h>
#endif

#include <unistd.h>
#include "rootboard.h"
#include "coloredboard.h"
#include "generateMoves.tcc"
#include "coloredboard.tcc"
#include "perft.h"
#include "workthread.h"
#include "jobs.h"
#include "testgame.h"
#include "transpositiontable.tcc"
#include "movelist.h"
#include "options.h"
#include "repetition.tcc"
#include "eval.tcc"

template<Colors C>
uint64_t RootBoard::rootDivide(unsigned int depth) {
    Move moveList[256];

    const ColoredBoard<C>& b = currentBoard<C>();
    Move* good = moveList+192;
    Move* bad=good;
    if (b.template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.template generateCaptureMoves<AllMoves>(good, bad);
        b.generateNonCap(good, bad);
    }

    uint64_t sum=0;
    for (Move* i = good; i<bad; ++i) {
        uint64_t n(0);
        if (depth == 1) {
            n++;
        } else {
            perft<(Colors)-C, trunk>(n, b, *i, depth-1);
        }
        std::cout << i->string() << " " << (uint64_t)n << std::endl;
        sum += n;
    }
    return sum;
}

void update(uint64_t& r, uint64_t v);

template<Colors C, Phase P, typename ResultType> void RootBoard::perft(ResultType& result, const ColoredBoard<(Colors)-C>& prev, Move m, unsigned int depth) {
    if (P == trunk && depth <= Options::splitDepth) {
        uint64_t n=0;
        perft<C, tree>(n, prev, m, depth);
        result += n;
        return;
    }

    __v8hi est = eval.estimate<(Colors)-C>(m, prev.keyScore);
    const ColoredBoard<C> b(prev, m, est);

    Key z = b.getZobrist();
    TranspositionTable<PerftEntry, 1, Key>::SubTable* pe = pt->getSubTable(z);
    PerftEntry subentry;

    if (pt->retrieve(pe, z, subentry) && subentry.depth == depth) {
        result += subentry.value;
        return;
    }
    Move list[256];
    Move* good = list + 192;
    Move* bad = good;
    if (b.template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.template generateCaptureMoves<AllMoves>(good, bad);
        b.generateNonCap(good, bad);
    }
    if (depth == 1) {
        result += bad-good;
        return;
    }

    ResultType n(0);
    for (Move* i = good; i<bad; ++i) {
        perft<(Colors)-C, P>(n, b, *i, depth-1);
    }

    PerftEntry stored;
    stored.zero();
    stored.depth |= depth;
    stored.upperKey |= z >> stored.upperShift;
    stored.value = (uint64_t)n;
    pt->store(pe, stored);
    result += n;
}

template<Colors C>
uint64_t RootBoard::rootPerft(unsigned int depth) {
    Move moveList[256];

    const ColoredBoard<C>& b = currentBoard<C>();
    Move* good = moveList+192;
    Move* bad=good;
    if (b.template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.template generateCaptureMoves<AllMoves>(good, bad);
        b.generateNonCap(good, bad);
    }

    if (depth <= 1)
        return bad-good;

    uint64_t n(0);
    for (Move* i = good; i<bad; ++i) {
        perft<(Colors)-C, trunk>(n, b, *i, depth-1);
    }
    return n;
}

template<Colors C>
bool RootBoard::isDraw(const ColoredBoard<C>& b) const
{
    return find(b, b.keyScore.key, 0) || b.fiftyMoves >= 100;
}

#endif
