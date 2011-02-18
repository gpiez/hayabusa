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

template <unsigned int pos, unsigned int width, typename T>
struct Bitfield {
    static const unsigned int tBits = CHAR_BIT * sizeof(T);
    static const unsigned int start = pos / tBits;
    static const unsigned int startpos = pos % tBits;
    static const unsigned int end = (pos+width-1)/ tBits;

    T data[end+1];
    operator T () const {
        static_assert(end == start, "Bitfield mismatch");
/*        if (width == 1)        // T is a bool type
            return data[start] & (T)1 << startpos;
        else*/ {
            T unmasked = data[start] >> startpos;
            if (width + startpos != tBits)
                unmasked &= ((T)1 << width) - 1;
            return unmasked;
        }
    }

//    void operator = (T value) {
//        data &= (1ULL<<width)-1 << pos;
//        data |= value << pos;
//    }

    void operator |= (T value) {
        static_assert(end == start, "Bitfield mismatch");
        data[start] |= value << startpos;
    }
    void reset() {
        static_assert(end == start, "Bitfield mismatch");
        data[start] &= ~ (((1<<width)-1) << startpos);
    }
};

/*
 * Transposition table entry. This assumes little endian order.
 * The static consts are the shift values for components.
 */
union TTEntry {
    Bitfield<0, 6, unsigned int> from;
    Bitfield<6, 6, unsigned int> to;
    Bitfield<12, 6, unsigned int> depth;
    Bitfield<18, 1, unsigned int> loBound;
    Bitfield<19, 1, unsigned int> hiBound;
    Bitfield<20, 12, int> score;
    Bitfield<32, 1, unsigned int> visited;
    Bitfield<33, 1, unsigned int> aged;
    Bitfield<34, 30, Key> upperKey;
    enum { upperShift = 34 };
    uint64_t data;

    void zero() { data = 0; };
};

struct PawnEntry {
    enum { upperShift = 16 };
    uint64_t passers[nColors];  // 16
    uint32_t openFiles[nColors];//  8
    struct Shield {
        uint8_t qside;
        uint8_t kside;
//        uint8_t weakLight;
//        uint8_t weakDark;
    } shield[nColors];          // 4
    uint16_t upperKey;          // 2
    int16_t score;              // 2
};

struct PerftEntry {
    union {
    Bitfield<0, 6, uint8_t> depth;
    Bitfield<6, 56, Key> upperKey;
    Bitfield<62, 1, uint64_t> loBound;
    Bitfield<63, 1, uint64_t> hiBound;
    uint64_t data;
    uint64_t aged; //dummy
    };
    enum { upperShift = 6 };
    uint64_t value;

    void zero() { data = 0; };
};

#endif /* TTENTRY_H_ */
