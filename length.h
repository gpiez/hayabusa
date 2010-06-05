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
#ifndef LENGTH_H_
#define LENGTH_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"
/*
 * Structure which hold attacking ray length for the eight long
 * range directions. The target piece is always counted in.
 * Only three bits for right to have a continuous range from 0-63
 * if the structure is interpreted a a byte, five bits for left to make sure
 * the upper bits are always set to zero.
 */
union Length {
    struct {
    uint8_t    right:3;
    uint8_t    left:5;
    };
    uint8_t data;
    operator const uint8_t& () const {
        return data;
    }
};
static const unsigned int nLengths = 64;

/*
 * LongIndex holds indices for long range vectors.
 * The index is simply made from the number of the piece ranging from
 * -King...King truncated to 4 bits. The index is used
 * in vecLookup[] to get the LongAttack associated with
 * the piece.
 */
union LongIndex {
    struct {
    int8_t rIndex:4;
    int8_t lIndex:4;
    };
    uint8_t data;
    operator const uint8_t& () const {
        return data;
    }
};

/*
 *  Precalculated masks for LongIndex changes in Board::attVec and length
 *  changes in Board::attLen. Because in each element all 64 bytes are always
 *  accessed at once, they need to be cache line aligned.
 */
struct LenMask64 {
    Length    len[nSquares];
    LongIndex mask[nSquares];
} ALIGN_CACHE;

/*
 * Array dimensions are [Length][Direction][Position][Four Board slices]
 * of Lenmask(each containing 16 squares)
 * Because each direction contains left and right, only four directions are needed.
 */
extern "C" LenMask64 masks[nLengths][nSquares][nDirs/2];

#endif /* LENGTH_H_ */
