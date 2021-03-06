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
#ifndef TRANSPOSITIONTABLE_TCC_
#define TRANSPOSITIONTABLE_TCC_

#include "transpositiontable.h"
#include "game.h"
#include "coloredboard.tcc"
#include "workthread.h"
#include <unistd.h>

template<typename Entry, unsigned int assoc, typename Key>
Table<Entry, assoc, Key>::Table(uint64_t size) :
    table(NULL),
    lru(1),
    usesHugePages(false) {
    setSize(size); }

template<typename Entry, unsigned int assoc, typename Key>
Table<Entry, assoc, Key>::~Table() {
    freeMemory(); }

template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::freeMemory() {
    if (table) {
        delete [] table;
        table = NULL; } }

// this is only called from tree
//template<typename Entry, unsigned int assoc, typename Key>
//bool Table<Entry, assoc, Key>::retrieve(const SubTable* subTable, Key k, Entry& ret, bool& visited) const {
//    visited = subTable->entries[0].visited;
//    Key upperKey = k >> Entry::upperShift;
//    for (unsigned int i = 0; i < assoc; ++i) {
//        if (subTable->entries[i].upperKey == upperKey) {        //only lock if a possible match is found
//            // found same entry again, move repetition
//            if (visited) {
//                ret.zero();
//                ret.loBound |= true;
//                ret.hiBound |= true;
//                ret.depth |= maxDepth-1; }
//            else {
//                ret = subTable->entries[i]; }
//            return true; } }
//    return false; }

template<typename Entry, unsigned int assoc, typename Key>
bool Table<Entry, assoc, Key>::retrieve(SubTable* subTable, Key k, Entry& ret ) {
    uint32_t upperKey = k >> 32;
    unsigned int i = 0;
    while (i < assoc-lru) {        //TODO compare all keys simultaniously suing sse
        if (subTable->entries[i].upperKey == upperKey
                && subTable->entries[i][key11] == (uint32_t)k >> (32-key11.size)) {        //only lock if a possible match is found
            ret = subTable->entries[i];
            return true; }
        ++i; }
    ASSERT(i==assoc-lru);
    while (i < assoc) {
        if (subTable->entries[i].upperKey == upperKey
                && subTable->entries[i][key11] == (uint32_t)k >> (32-key11.size)) {        //only lock if a possible match is found
            ret = subTable->entries[i];
            unsigned j=i;
            if (j>assoc-lru) {
                subTable->entries[j] = subTable->entries[j-1];
                while (--j>assoc-lru) subTable->entries[j] = subTable->entries[j-1];
                ASSERT(j==assoc-lru);
                subTable->entries[j] = ret;
            }
            return true; }
        ++i; }
    return false; }

#ifdef __SSE4_1__
inline bool compare128(__m128i a, __m128i b) {
    __m128i c = _mm_xor_si128(a, b);
    return _mm_testc_si128(zero, c);
}
#endif
template<unsigned int assoc, typename Key>
bool TranspositionTable<PawnEntry, assoc, Key>::retrieve(Sub<PawnEntry, assoc>* subTable, const Board& b, PawnEntry& ret ) {
    unsigned int i=0;
    PawnEntry first = subTable->entries[i];
    PawnEntry second;
    do {
#ifdef __SSE4_1__
#ifdef MYDEBUG
        ASSERT(compare128(first.pawns, b.get2Pieces<Pawn>()) == (((uint64_t*)&first.pawns)[0] == b.getPieces<White,Pawn>() && ((uint64_t*)&first.pawns)[1] == b.getPieces<Black,Pawn>()));
#endif
        if (compare128(first.pawns, b.get2Pieces<Pawn>()))
#else
        if (first.pawns[0] == b.getPieces<White,Pawn>() && first.pawns[1] == b.getPieces<Black,Pawn>())
#endif
        {
            subTable->entries[0] = first;
            ret = first;
            return true; }
        ++i;
        if (i == assoc) break;
        second = subTable->entries[i];
        subTable->entries[i] = first;
        first = second; }
    while (true);

    return false; }
//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::store(SubTable* subTable, Entry entry) {
    STATS(ttstore);
    // look if the position is already stored. if it is, but the new depth
    // isn't sufficient, don't write anything.
    for (unsigned int i = 0; i < assoc; ++i)         //TODO compare all keys simultaniously suing sse
        if (subTable->entries[i].upperKey == entry.upperKey) {
//            if (entry[score] > infinity && subTable->entries[i][score] > entry[score]) {
//                asm("int3\n");
//            }
            if (/*entry[loBound] + entry[hiBound] >= subTable->entries[i][loBound] + subTable->entries[i][hiBound]
                ||*/ entry[depth] >= subTable->entries[i][depth]) {
                STATS(ttoverwrite);
                STATS(ttinsufficient);
                if (entry[depth] == subTable->entries[i][depth]/* && subTable->entries[i].score == entry.score*/)
                    if (   (subTable->entries[i][loBound] && entry[hiBound])
                            || (subTable->entries[i][hiBound] && entry[loBound])) {
                        entry.set2(hiBound, true);
                        entry.set2(loBound, true);
                        STATS(ttmerge); }

                subTable->entries[i] = entry;
                /*                unsigned j;
                                for (j=i; j; ) {
                                    if (subTable->entries[--j].depth >= subTable->entries[i].depth) {
                                        ++j;
                                        break;
                                    }
                                }

                                if (j == i) {
                                    for (j=i; j+1<assoc; ) {
                                        if (subTable->entries[++j].depth <= subTable->entries[i].depth) {
                                            --j;
                                            break;
                                        }
                                    }
                                }

                                if (j != i) {
                                    subTable->entries[i] = subTable->entries[j];
                                    subTable->entries[j] = entry;
                                }*/
            }
            STATS(ttinsufficient);
            return; }

    if (subTable->entries[assoc-1].upperKey == 0 || subTable->entries[assoc-1][aged])
        STATS(ttuse);
    unsigned int i;
    for (i = 0; i < assoc-lru; ++i)
        if (subTable->entries[i][aged] || entry[depth] > subTable->entries[i][depth])
            break;
    /*
     * No depth entry with less depth, store in the first place of the LRU entries
     */
    unsigned j;
    for (j = assoc-1; j>i; --j) {
        subTable->entries[j] = subTable->entries[j-1]; }
    ASSERT(j==i);
    subTable->entries[j] = entry; }
//#pragma GCC diagnostic pop

template<unsigned int assoc, typename Key>
void TranspositionTable<PawnEntry, assoc, Key>::store(Sub<PawnEntry, assoc>* subTable, PawnEntry entry) {
    subTable->entries[0] = entry; }

template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::setSize(size_t s) {
    freeMemory();

#ifndef __WIN32__
    s = std::min(s, (size_t) sysconf(_SC_PAGESIZE) * (size_t) sysconf(_SC_PHYS_PAGES));
#endif

    nEntries = s/sizeof(SubTable);

    nEntries |= nEntries >> 1;
    nEntries |= nEntries >> 2;
    nEntries |= nEntries >> 4;
    nEntries |= nEntries >> 8;
    nEntries |= nEntries >> 16;
#ifdef __x86_64__
    nEntries |= nEntries >> 32;
#endif
    nEntries++;
    nEntries >>= 1;
    size = nEntries*sizeof(SubTable);

    while (!table) {
        table = new SubTable[nEntries];
        usesHugePages = false;
        if (table) break;
        std::cerr << "Could not allocate" << size << "bytes" << std::endl;
        size >>= 1;
        nEntries >>= 1; }
    clear();    // not strictly neccessary, but allocating pages
    mask = nEntries-1; }

template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::resetStats() {
    auto& threads = WorkThread::getThreads();
    for (auto th = threads.begin(); th !=threads.end(); ++th) {
#ifndef LEAN_AND_MEAN
        (*th)->getStats()->ttuse = 0;
        (*th)->getStats()->tthit = 0;
        (*th)->getStats()->ttalpha = 0;
        (*th)->getStats()->ttbeta = 0;
        (*th)->getStats()->ttoverwrite = 0;
        (*th)->getStats()->ttstore = 0;
#endif
    } }

template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::clear() {
    resetStats();
//     std::cerr << "clear " << size << std::endl;
    memset(table, 0, size); }

template<typename Entry, unsigned int assoc, typename Key>
void Table<Entry, assoc, Key>::agex() {
    resetStats();
//     std::cerr << "clear " << size << std::endl;
    for (size_t i=0; i<nEntries; ++i) {
        for (size_t j=0; j<assoc; ++j) {
            table[i].entries[j].set2(aged, 1); } } }

template<typename Entry, unsigned assoc, typename Key>
template<Colors C>
std::string Table<Entry, assoc, Key>::bestLineNext(const ColoredBoard<(Colors)-C>& prev, Move m, std::set<Key>& visited, const Game& rb) {
    std::string line = m.string();
    ColoredBoard<C> b;
    b.keyScore.vector = rb.eval.estimate<(Colors)-C>(m, prev.keyScore);
//    unsigned estmatIndex = rb.eval.estimate<(Colors)-C>(m, prev.matIndex);
    b.init(prev, m);
    Key key = b.getZobrist();
    if (visited.count(key)) return line;
    visited.insert(key);
    SubTable* te = getSubTable(key);
    TTEntry subentry;

    Move ttMove(0,0,0);
    if (retrieve(te, key, subentry) ) {
        ttMove = Move(subentry[from], subentry[to], 0); }

    Move moveList[256];
    Move* good=moveList+192;
    Move* bad=good;
    if (b. template inCheck<C>())
        b.generateCheckEvasions(good, bad);
    else {
        b.generateNonCap(good, bad);
        b.template generateCaptureMoves<AllMoves>(good, bad); }
    if (ttMove.data)
        for (Move* i=good; i<bad; ++i) {
            if (i->from() == ttMove.from() && i->to() == ttMove.to()) {
                line += " " + bestLineNext<(Colors)-C>(b, *i, visited, rb);
                break; } }

    return line; }

template<typename Entry, unsigned assoc, typename Key>
std::string Table<Entry, assoc, Key>::bestLine(const Game& b) {
    if (!b.bestMove.data) return "";
    std::set<Key> visited;
    if (b.color == White) {
        return bestLineNext<Black>(b.currentBoard<White>(), b.bestMove, visited, b); }
    else {
        return bestLineNext<White>(b.currentBoard<Black>(), b.bestMove, visited, b); }

}
/*
 * float12 format:
 *
 * SEE E/M MMMM MMMM
 *
 * S sign of mantissa
 * E exponent, 0/1      0.. 2,00  d=1/256
 *             2        2.. 4,00  d=1/128
 *             3        4.. 8,00  d=1/64
 *             4        8..16,00  d=1/32
 *             5       16..32,00  d=1/16
 *             6       32..64,00  d=1/8
 *             7       M1..M256   d=1
 */

inline int tt2Score(int s) {
    ASSERT(s < 0x800);
    ASSERT(s >= -0x800);

    if (s < 0x400 && s > -0x400)
        return s;
    if (s > 0) {
        if (s < 0x600)
            return s*2 - 0x400*2 + 0x400;
        if (s < 0x700)
            return s*4 - 0x600*4 + 0x800;
        return s*16 - 0x700*16 + 0x1000; }
    else {
        if (s > -0x600)
            return s*2 + 0x400*2 - 0x400;
        if (s > -0x700)
            return s*4 + 0x600*4 - 0x800;
        return s*16 + 0x700*16 - 0x1000; } }

inline int score2tt(int s) {
    ASSERT(s < 0x8000);
    ASSERT(s >= -0x8000);

    if (s < 0x400 && s > -0x400)
        return s;
    if (s > 0) {
        if (s<0x800)
            return (s-0x400)/2 + 0x400;
        if (s<0x1000)
            return (s-0x800)/4 + 0x600;
        return (s-0x1000)/16 + 0x700; }
    else {
        if (s>-0x800)
            return (s+0x400)/2 - 0x400;
        if (s>-0x1000)
            return (s+0x800)/4 - 0x600;
        return (s+0x1000)/16 - 0x700; } }
#endif
