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
#include "boardbase.tcc"

Castling BoardBase::castlingMask[nSquares];
__v2di BoardBase::mask02x[nSquares]; // 1 KByte  file : row, excluding square
__v2di BoardBase::dir02mask[nSquares]; // 1 KByte  file : row, including square
__v2di BoardBase::dir13mask[nSquares]; // 1 KByte  antidiag : diagonal, excluding square
__v2di BoardBase::doublebits[nSquares]; // 1 KByte    1<<sq  : 1<<sq
__v2di BoardBase::doublereverse[nSquares]; // 1 KByte    1<<sq  : 1<<sq
uint64_t BoardBase::knightAttacks[nSquares];
uint64_t BoardBase::kingAttacks[16][nSquares];
//Length masks[nLengths][nSquares][nDirs/2][nSquares];

void BoardBase::buildAttacks() {
    buildAttacks<White>();
    buildAttacks<Black>();
    buildPins<White>();
    buildPins<Black>();
}

void BoardBase::initTables() {
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

    for (int y = 0; y < (signed)nRows; ++y)
    for (int x = 0; x < (signed)nFiles; ++x) {
        uint64_t p=0;
        int dx=1, dy=0;
        for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
            p |= 1ULL << (x0+y0*nRows);
        dx=-1;
        for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
            p |= 1ULL << (x0+y0*nRows);
        uint64_t dir0x = p;
        if (x==0)
            p |= 1ULL << (x+y*nRows);
        uint64_t dir0 = p;

        p=0; dx=0; dy=1;
        for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
            p |= 1ULL << (x0+y0*nRows);
        dy=-1;
        for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
            p |= 1ULL << (x0+y0*nRows);
        uint64_t dir2x = p;
        if (y==0)
            p |= 1ULL << (x+y*nRows);
        uint64_t dir2 = p;

        p=0; dx=1; dy=1;
        for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
            p |= 1ULL << (x0+y0*nRows);
        dx=-1; dy=-1;
        for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
            p |= 1ULL << (x0+y0*nRows);
        uint64_t dir1 = p;

        p=0; dx=1; dy=-1;
        for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
            p |= 1ULL << (x0+y0*nRows);
        dx=-1; dy=1;
        for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
            p |= 1ULL << (x0+y0*nRows);
        uint64_t dir3 = p;

        mask02x[x+y*nRows] = _mm_set_epi64x(dir2x, dir0x);
        dir02mask[x+y*nRows] = _mm_set_epi64x(dir2, dir0);
        dir13mask[x+y*nRows] = _mm_set_epi64x(dir3, dir1);
        doublebits[x+y*nRows] = _mm_set1_epi64x(1ULL << (x+y*nRows));
        doublereverse[x+y*nRows] = _mm_set1_epi64x(1ULL << (x+(7-y)*nRows));

        p=0;
        for (dx=-2; dx<=2; dx++)
        for (dy=-2; dy<=2; dy++)
        if (abs(dx*dy) == 2) {
            int x0 = x+dx; int y0=y+dy;
            if ( x0>=0 && x0<=7 && y0>=0 && y0<=7 )
                p |= 1ULL << (x0+y0*nRows);
        }
        knightAttacks[x+y*nRows] = p;

        for (unsigned mask=0; mask<16; ++mask) {
            p=0;
            for (unsigned int dir=0; dir<8; ++dir)
            if (~mask & (1ULL<<(int[]){0,2,1,3}[(dir&3)])) {
                int x0 = x+xOffsets[dir]; int y0=y+yOffsets[dir];
                if ( x0>=0 && x0<=7 && y0>=0 && y0<=7 )
                    p |= 1ULL << (x0+y0*nRows);
            }
            kingAttacks[mask][x+y*nRows] = p;
        }
    }

    for (unsigned int right = 0; right < 8; ++right)
    for (unsigned int left = 0; left < 8-right; ++left) {
        unsigned int lr = left*8+right;
        for (unsigned int dir = 0; dir < nDirs/2; ++dir)
        for (unsigned int y = 0; y < nRows; ++y)
        for (unsigned int x = 0; x < nFiles; ++x) {
            int xt = x + xOffsets[dir];
            int yt = y + yOffsets[dir];
            for (unsigned int len=1; xt >= 0 && xt <= 7 && yt >= 0 && yt <= 7 && len<=right; len++) {
//                masks[lr][8 * y + x][dir][yt*8 + xt].left = left;
                xt += xOffsets[dir];
                yt += yOffsets[dir];
            }
            xt = x - xOffsets[dir];
            yt = y - yOffsets[dir];
            for (unsigned int len=1; xt >= 0 && xt <= 7 && yt >= 0 && yt <= 7 && len<=left; len++) {
//                masks[lr][8 * y + x][dir][yt*8 + xt].right = right;
                xt -= xOffsets[dir];
                yt -= yOffsets[dir];
            }
        }
    }
}

// Empty board and initialize length tables, instead of a constructor
// The default constructor should not be initialized, because the contents
// of the constructed object are generated anyway.
void BoardBase::init() {
    *this = (BoardBase){{{0}}};
    keyScore.pawnKey = 0x12345678;
/*
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
*/
}

void BoardBase::print() {
    static const QChar chessPieces[nTotalPieces] =
        { L'♚', L'♟', L'♞', L'♛', L'♝', L'♜', ' ', L'♖', L'♗', L'♕', L'♘', L'♙', L'♔' };
    QTextStream xout(stderr);
    xout.setCodec("UTF-8");
    xout << "--------------------------------" << endl;
    for (unsigned int y = 7; y < nRows; --y) {
        for (unsigned int x = 0; x < nFiles; ++x) {
            xout << "| " << chessPieces[6] << ' ';
        }
        xout << endl << "--------------------------------" << endl;
    }
}
