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
#include <pch.h>

#include "boardbase.h"
/*
 * Short attack table for each piece, positioned at a square, attacking a white/black board.
 */
SAttack shortAttacks[nPieces+1][nSquares][nColors][nSquares] = {};
/*
 * Lookup table containing the length changes and the appropriate masks
 * for each left/right length combination (first dimension) on each square (2nd dimension)
 * for each possible left/right direction (3rd dimension). Only a small part of the
 * combinations are valid, so the memory and cache usage is far less (~214k) than the size of
 * the whole array (2M).
 *
 * allocated 8 times, for 8 possible lengths
 * usage for even dir on     pos    l1    l2    sum          (count is 16 for each posn,
 *                             0    0    1-7    7             there are only 4 distinct)
 *                             1    1    1-6    6
 *                             2    1-2    1-5 10
 *                             3   1-3 1-4 12 = 35 for 4 posn = 560 for 64 posn = 2240 for 4 dirs = 140k
 * usage for odd dirs on     pos l1    l2    count sum
 *                             0    0    1-7    2      14    (left corner)
 *                             1    0    1-6 4      24    (running
 *                             ...                      to right corner)
 *                                6    0   1   4     4         sum = 100 for 28 border posn
 *                             7    0    0    2      2
 *
 *                          9   1    1-6    2    12
 *                             10    1    1-5 4    20
 *                             11    1    1-4 4    16
 *                             12    1    1-3 4    12
 *                             13    1    1-2 4    8
 *                             14    1    1    2    2        sum = 70 for 20 2nd ring posn
 *
 *                          18    1-2    1-5    2    20
 *                             19    1-2    1-4 4    32
 *                             20    1-2    1-3 4    24
 *                             21    1-2    1-2 2    8        sum = 84 for 12
 *
 *                             27  1-3 1-4 2    24      sum = 42 for 4
 *                             28  1-3 1-3 2   18      total = 296 for 64 pos = 1184 for 4 dirs = 74k
 * used are 214k, allocated are 512 pages a 4k, total 2M.
 */
LenMask64 masks[nLengths][nSquares][nDirs/2];
__m128i broadcastTab[256];
/*
 * Table containing the direction from first index to 2nd index or 0xff
 */
uint8_t vec2dir[nSquares][nSquares];
uint8_t BoardBase::vec2pin[nSquares][nSquares];
bool BoardBase::attPinTable[0x100][0x100];
/*
 * lookup table from converting piece indices, which are signed 4-bit integers, to attack patterns
 * four tables, for white/black in straight/diagonal directions
 */
const __v16qi vecLookup[4] = {
    // 0        1        2        3        4        5        6        7       -8       -7       -6       -5       -4       -3       -2       -1
    { 0,  attackR,       0, attackQ,       0,       0, checkKR}, //horiz. white
    { 0,        0,       0,       0,       0,       0,       0,       0,       0,       0, checkKR,       0,       0, attackQ,       0, attackR}, //black
    { 0,        0, attackB, attackQ,       0,       0, checkKB}, //diag. white
    { 0,        0,       0,       0,       0,       0,       0,       0,       0,       0, checkKB,       0,       0, attackQ, attackB,       0}
};

const __v16qi PieceList::addShifts[16] = {
    {    0,    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14},    // pos[0] inserted
    {    0,    1,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14},
    {    0,    1,    2,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14},
    {    0,    1,    2,    3,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14},
    {    0,    1,    2,    3,    4,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14},    //4
    {    0,    1,    2,    3,    4,    5,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14},
    {    0,    1,    2,    3,    4,    5,    6,    6,    7,    8,    9,   10,   11,   12,   13,   14},
    {    0,    1,    2,    3,    4,    5,    6,    7,    7,    8,    9,   10,   11,   12,   13,   14},
    {    0,    1,    2,    3,    4,    5,    6,    7,    8,    8,    9,   10,   11,   12,   13,   14},    //8
    {    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    9,   10,   11,   12,   13,   14},
    {    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   10,   11,   12,   13,   14},
    {    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   11,   12,   13,   14},
    {    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   12,   13,   14},    //12
    {    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   13,   14},
    {    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   14},
    {    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15},
};

uint64_t PieceList::indexDiffs[nPieces+1] = {
    // k p n q b r 0 (index[])
    0,
    0x00000101010000,    // rook insert King, Pawn, Rook, Bishop, Queen, Knight
    0x00000101000000,    // b
    0x00000100000000,    // q
    0x00000000000000,    // n
    0x00000101010100,    // p
    0x00010101010100    // k
};

const uint64_t PieceList::posMask[9] = {
    0xffffffffffffffff,
    0xffffffffffffff00,
    0xffffffffffff0000,
    0xffffffffff000000,
    0xffffffff00000000,
    0xffffff0000000000,
    0xffff000000000000,
    0xff00000000000000,
    0x0000000000000000
};

Castling BoardBase::castlingMask[nSquares];
uint64_t BoardBase::knightDistanceTable[nSquares] = {0};
uint64_t BoardBase::kingDistanceTable[nSquares] = {0};
BoardBase::FourMoves BoardBase::moveFromTable[nSquares];
BoardBase::SevenMoves BoardBase::moveOffsetTable[nSquares][4];
uint8_t BoardBase::totalLen[64];
Length BoardBase::borderTable[0x100];

/// Initialize the static tables used by BoardBase and the low level asm routines
void BoardBase::initTables()
{
    //  Fill table for calculating directions from vectors
    for (unsigned int y0 = 0; y0 < nRows; ++y0)
    for (unsigned int x0 = 0; x0 < nFiles; ++x0)
    for (unsigned int y1 = 0; y1 < nRows; ++y1)
    for (unsigned int x1 = 0; x1 < nFiles; ++x1) {
        uint8_t dir=0;
        int dx = x1 - x0;
        int dy = y1 - y0;
        if ((dx|dy) == 0) dir = ~0;
        else {
            double alpha = atan2(dy, dx) / (2.0*M_PI) * 8.0;
            if (alpha < -0.03125) alpha += 8.0;
            dir = rint(alpha);
            if (fabs(alpha-dir) > 0.03125)
                dir = ~0;
        }
        vec2dir[8*y0+x0][8*y1+x1] = dir;
        vec2pin[8*y0+x0][8*y1+x1] = dir & 3;    // 3 in case there is no valid dir.
        // this doesn't matter, because it is used for determining a possible pin
        // if there is none, the second pin lookup will return 0
    }

    // Fill table containing one byte broadcasted to all locations in the __v16qi vector
    for (unsigned int i = 0; i < 256; ++i)
        broadcastTab[i] = _mm_set1_epi8(i);

    for (unsigned int right = 0; right < 8; ++right)
    for (unsigned int left = 0; left < 8-right; ++left) {
        unsigned int lr = left*8+right;
        for (unsigned int dir = 0; dir < nDirs/2; ++dir)
        for (unsigned int y = 0; y < nRows; ++y)
        for (unsigned int x = 0; x < nFiles; ++x) {
            int xt = x + xOffsets[dir];
            int yt = y + yOffsets[dir];
            for (unsigned int len=1; xt >= 0 && xt <= 7 && yt >= 0 && yt <= 7 && len<=right; len++) {
                masks[lr][8 * y + x][dir].len[yt*8 + xt].left = left;
                masks[lr][8 * y + x][dir].mask[yt*8 + xt].rIndex = -1;
                xt += xOffsets[dir];
                yt += yOffsets[dir];
            }
            xt = x - xOffsets[dir];
            yt = y - yOffsets[dir];
            for (unsigned int len=1; xt >= 0 && xt <= 7 && yt >= 0 && yt <= 7 && len<=left; len++) {
                masks[lr][8 * y + x][dir].len[yt*8 + xt].right = right;
                masks[lr][8 * y + x][dir].mask[yt*8 + xt].lIndex = -1;
                xt -= xOffsets[dir];
                yt -= yOffsets[dir];
            }
        }
    }

    for (unsigned int c = 0; c < nColors; ++c)
    for (unsigned int p = 1; p <= nPieces; ++p)
    for (unsigned int y = 0; y < nRows; ++y)
    for (unsigned int x = 0; x < nFiles; ++x)
    for (unsigned int dir = 0; dir < nDirs; ++dir) {
        int xs = xShortOffsets[p][dir];
        int ys = yShortOffsets[c][p][dir];
        if (xs == 0 && ys == 0) break;
        int xt = x + xs;
        int yt = y + ys;

        if ( xt>=0 && xt<=7 && yt>=0 && yt<=7) {
            if ((xs>0) ^ c)
                shortAttacks[p][y*8+x][c][yt*8+xt] = (SAttack[nPieces+1]){ {}, {}, {}, {}, attackN_, attackPR_, attackK_ }[p];
            else
                shortAttacks[p][y*8+x][c][yt*8+xt] = (SAttack[nPieces+1]){ {}, {}, {}, {}, attackN_, attackPL_, attackK_ }[p];
        }
    }

    // if a piece is moved from or to a position in this table, the castling status
    // is disabled if the appropriate rook or king squares match.
    for (unsigned int sq=0; sq<nSquares; ++sq) {
        castlingMask[sq].data4 = ~0;
        if (sq == a1 || sq == e1)
            castlingMask[sq].color[0].q = 0;
        if (sq == h1 || sq == e1)
            castlingMask[sq].color[0].k = 0;
        if (sq == a8 || sq == e8)
            castlingMask[sq].color[1].q = 0;
        if (sq == h8 || sq == e8)
            castlingMask[sq].color[1].k = 0;
    }

    for (unsigned int a=0; a<nSquares; ++a)
    for (unsigned int b=0; b<nSquares; ++b) {
        if (abs( (a&7) - (b&7) ) * abs( (a&0x38) - (b&0x38) ) == 16)
            knightDistanceTable[a] |= 1ULL << b;

        if ((abs( (a&7) - (b&7) ) | (abs( (a & 0x38) - (b & 0x38) ) >> 3)) == 1)
            kingDistanceTable[a] |= 1ULL << b;
    }

    for (unsigned int a=0; a<0x100; ++a)
    for (unsigned int b=0; b<0x100; ++b) {
        attPinTable[a][b] = ( !!(a & checkKB) & !!(b & (attackMaskQ | attackMaskB)) ) |
                            ( !!(a & checkKR) & !!(b & (attackMaskQ | attackMaskR)) );
    }

    for (uint8_t from = 0; from<nSquares; ++from) {
        Move m = {{ from, from }};
        for (unsigned int i=0; i<7; i++) {
            moveFromTable[from].single[i] = m;
        }
    }
    
    LongIndex l;
    for (l.lIndex = -King; l.lIndex <= King; l.lIndex++)
    for (l.rIndex = -King; l.rIndex <= King; l.rIndex++) {
        Length len = {{ 0, 0 }};
        if (l.rIndex)
            len.left = 1;
        if (l.lIndex)
            len.right = 1;
        borderTable[l] = len;
    }

    for (int left = 0; left < 8; left++)
    for (int right = 0; right < 8; right++) {
        Length len = {{ right, left }};
        totalLen[len] = left + right;
    }
    for (int dir=0; dir<4; ++dir)
    for (int r = 0; r < 8; ++r) {
        for (int i = 0;  i < r; ++i)
        {
            Move m = {{ 0, (i+1)*dirOffsets[dir] }};
            moveOffsetTable[r][dir].single[i] = m;
        }
        for (int i = r;  i < 7; ++i)
        {
            Move m = {{ 0, (r-i-1)*dirOffsets[dir] }};
            moveOffsetTable[r][dir].single[i] = m;
        }
    }
}

// Empty board and initialize length tables, instead of a constructor
// The default constructor should not be initialized, because the contents
// of the constructed object are generated anyway.
void BoardBase::init() {
    *this = (BoardBase){{{{{0}}}}};

    for (unsigned int dir = 0; dir < nDirs; ++dir) {
        for (unsigned int y = 0; y < nRows; ++y) {
            for (unsigned int x = 0; x < nFiles; ++x) {
                unsigned int len = -1;
                int xt = x;
                int yt = y;
                do {
                    xt += xOffsets[dir];
                    yt += yOffsets[dir];
                    len++;
                } while (xt >= 0 && xt <= 7 && yt >= 0 && yt <= 7);
                if (dir < 4)
                    attLen[dir][y * 8 + x].right = len;
                else
                    attLen[dir-4][y * 8 + x].left = len;
            }
        }
    }
}

void BoardBase::print() {
    static const QChar chessPieces[nTotalPieces] =
        { L'♚', L'♟', L'♞', L'♛', L'♝', L'♜', ' ', L'♖', L'♗', L'♕', L'♘', L'♙', L'♔' };
    QTextStream xout(stderr);
    xout.setCodec("UTF-8");
    xout << "--------------------------------" << endl;
    for (unsigned int y = 7; y < nRows; --y) {
        for (unsigned int x = 0; x < nFiles; ++x) {
            xout << "| " << chessPieces[pieces[y*8 + x] + 6] << ' ';
        }
        xout << endl << "--------------------------------" << endl;
    }
}
