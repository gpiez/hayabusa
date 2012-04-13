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

#include "game.h"
#include "coloredboard.h"
#include "options.h"
#include "eval.tcc"
#include "generateMoves.tcc"
#include "coloredboard.tcc"
#include "transpositiontable.tcc"
/*
 * search for an position repetition
 * reverse search only the last b.fiftyMoves, skipping the first because it
 * can't be a repetition. fiftyMoves may be given in a fen position, but the
 * moves leading to the start position may not, so we need the second condition
 */
template<Colors C>
inline bool Game::findRepetition(const ColoredBoard<C>& b, Key k, unsigned ply) const {
    for (unsigned i = ply+rootPly; i+b.fiftyMoves >= ply+rootPly+4 && i>=4; i-=2) {
        ASSERT(i<=nMaxGameLength);
        if (keys[i-4] == k) return true; }
    return false; }
inline void Game::store(Key k, unsigned ply) {
    keys[ply+rootPly] = k; }
template<Colors C>
inline void Game::clone(const ColoredBoard<C>& b, const RepetitionKeys& other, unsigned ply) const {
    for (unsigned i = ply+rootPly; i+b.fiftyMoves >= ply+rootPly; --i) {
        keys[i] = other[i];
        if (!i) break; } }

template<Colors C>
uint64_t Game::rootDivide(unsigned int depth) {
    Move moveList[256];

    pt = new TranspositionTable<PerftEntry, 1, Key>(Options::hash);
    const ColoredBoard<C>& b = currentBoard<C>();
    Move* good = moveList+192;
    Move* bad=good;
    if (b.template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.template generateCaptureMoves<AllMoves>(good, bad);
        b.generateNonCap(good, bad); }

    uint64_t sum=0;
    for (Move* i = good; i<bad; ++i) {
        uint64_t n(0);
        if (depth == 1) {
            n++; }
        else {
            perft<(Colors)-C, trunk>(n, b, *i, depth-1); }
        std::cout << i->string() << " " << (uint64_t)n << std::endl;
        sum += n; }
    delete pt;
    return sum; }

void update(uint64_t& r, uint64_t v);

template<Colors C, Phase P, typename ResultType> void Game::perft(ResultType& result, const ColoredBoard<(Colors)-C>& prev, Move m, unsigned int depth) {
    if (P == trunk && depth <= Options::splitDepth) {
        uint64_t n=0;
        perft<C, tree>(n, prev, m, depth);
        result += n;
        return; }

    __v8hi est = eval.estimate<(Colors)-C>(m, prev.kms);
    const ColoredBoard<C> b(prev, m, est, eval);

#if 0    
    Key z = b.getZobrist();
    TranspositionTable<PerftEntry, 1, Key>::SubTable* pe = pt->getSubTable(z);
    PerftEntry subentry;

    if (pt->retrieve(pe, z, subentry) && subentry.depth == depth) {
        result += subentry.value;
        return; }
#endif
    Move list[256];
    Move* good = list + 192;
    Move* bad = good;
    if (b.template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.template generateCaptureMoves<AllMoves>(good, bad);
        b.generateNonCap(good, bad); }
    if (depth == 1) {
        result += bad-good;
        return; }

    ResultType n(0);
    for (Move* i = good; i<bad; ++i) {
        perft<(Colors)-C, P>(n, b, *i, depth-1); }

#if 0    
    PerftEntry stored;
//    stored.zero();
    stored.depth = depth;
    stored.upperKey = z >> stored.upperShift;
    stored.value = (uint64_t)n;
    pt->store(pe, stored);
#endif
    result += n; }

template<Colors C>
uint64_t Game::rootPerft(unsigned int depth) {
    Move moveList[256];

    pt = new TranspositionTable<PerftEntry, 1, Key>(Options::hash);
    const ColoredBoard<C>& b = currentBoard<C>();
    Move* good = moveList+192;
    Move* bad=good;
    if (b.template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.template generateCaptureMoves<AllMoves>(good, bad);
        b.generateNonCap(good, bad); }

    if (depth <= 1)
        return bad-good;

    uint64_t n(0);
    for (Move* i = good; i<bad; ++i) {
        perft<(Colors)-C, trunk>(n, b, *i, depth-1); }
    delete pt;
    return n; }

template<Colors C>
bool Game::isDraw(const ColoredBoard<C>& b) const {
    return findRepetition(b, b.kms.key(), 0) || b.fiftyMoves >= 100; }

#endif
