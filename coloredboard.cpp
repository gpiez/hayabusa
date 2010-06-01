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

#include "pch.h"
#include "coloredboard.h"

template<>
uint8_t ColoredBoard<White>::diaPinTable[nDirs][256];
template<>
uint8_t ColoredBoard<Black>::diaPinTable[nDirs][256];

template<Colors C>
void ColoredBoard<C>::initTables() {
    for (unsigned int dir = 0; dir<nDirs; dir++)
    for (int l = -King; l<=King; ++l)
    for (int r = -King; r<=King; ++r) {
        LongIndex i = { l, r };
        diaPinTable[dir][(uint8_t&)i] = ~0;
        if ( (dir&1) && l == King && (r == Bishop || r == Queen) )
            diaPinTable[dir][(uint8_t&)i] = dir;
        if ( (dir&1) && r == King && (l == Bishop || l == Queen) )
            diaPinTable[dir][(uint8_t&)i] = dir;
        if ( !(dir&1) && l == King && (r == Rook || r == Queen) )
            diaPinTable[dir][(uint8_t&)i] = dir;
        if ( !(dir&1) && r == King && (l == Rook || l == Queen) )
            diaPinTable[dir][(uint8_t&)i] = dir;
    }
}

