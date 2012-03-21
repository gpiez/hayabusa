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
#ifndef TTENTRY_H_
#define TTENTRY_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"
#include "packedscore.h"
#include <type_traits>
/*
 * Transposition table entry. This assumes little endian order.
 * The static consts are the shift values for components.
 */
#define AFTER(prev, newsize) { prev.pos + prev.size, newsize }
struct Bitfield {
    unsigned pos;
    unsigned size;
};
struct SignedBitfield {
    unsigned pos;
    unsigned size;
};
static constexpr Bitfield aged {  0,  1 };
static constexpr Bitfield hiBound AFTER(aged, 1);
static constexpr Bitfield loBound AFTER(hiBound, 1);
static constexpr Bitfield special AFTER(loBound, 1);
static constexpr Bitfield piece AFTER(special, 3);
static constexpr Bitfield capture AFTER(piece, 3);    
static constexpr Bitfield from AFTER(capture, 6);
static constexpr Bitfield to AFTER(from, 6);
static constexpr Bitfield depth AFTER(to, 7);
static constexpr SignedBitfield score AFTER(depth, 12);
static constexpr SignedBitfield posScore AFTER(score, 12);
struct TTEntry {
    
    constexpr int operator [] (const Bitfield bf) {
        return bitfield << (64-bf.pos-bf.size) >> (64-bf.size);
    }
    int operator [] (const SignedBitfield bf) {
        return (int64_t)bitfield << (64-bf.pos-bf.size) >> (64-bf.size);
    }
    void set(Bitfield bf, unsigned value) {
        ASSERT(value < 1ULL<<bf.size );
        ASSERT((*this)[bf] == 0);
        bitfield |= value << bf.pos;
    }
    void set2(Bitfield bf, unsigned value) {
        ASSERT(value < 1ULL<<bf.size );
        bitfield |= value << bf.pos;
    }
    void set(SignedBitfield bf, int value) {
        ASSERT(value < 1LL<<(bf.size-1) );
        ASSERT(value >= -1LL<<(bf.size-1) );
        ASSERT((*this)[bf] == 0);
        bitfield |= (uint64_t)value << (64-bf.size) >> (64-bf.pos-bf.size);
    }
    uint64_t    bitfield;
    uint32_t    upperKey;
};

struct PawnEntry {
#ifdef __SSE4_1__
    __v2di  pawns;
#else
    uint64_t pawns[nColors];
#endif    
    uint64_t passers[nColors];          // 16
    PackedScore<> score;                //  4
    uint8_t openFiles[nColors];         //  2
    uint8_t weak[nColors];              //  2
    uint8_t dark[nColors];              //  2
    uint8_t light[nColors];             //  2
    uint8_t darkDef[nColors];           //  2
    uint8_t lightDef[nColors];          //  2
    int8_t shield2[nColors][nRows];     // 16
    };

struct PerftEntry {
    union {
        unsigned depth:6;
        unsigned loBound:1;
        unsigned hiBound:1;
        unsigned upperKey;
        uint64_t data;
        uint64_t aged; //dummy
        uint64_t score; };
    enum { upperShift = 6 };
    uint64_t value;

    void zero() {
        data = 0; }; };

#endif /* TTENTRY_H_ */
