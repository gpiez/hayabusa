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
#include "repetition.h"

template<Colors C>
inline bool RootBoard::find(const ColoredBoard<C>& b, Key k, unsigned ply) const {
    for (unsigned i = ply+fiftyMovesRoot; i+b.fiftyMoves >= ply+fiftyMovesRoot+4; i-=2) {
        ASSERT(i<=100);
        if (keys[i-4] == k) return true;
    }
    return false;
}
inline void RootBoard::store(Key k, unsigned ply) {
    keys[ply+fiftyMovesRoot] = k;
}
template<Colors C>
inline void RootBoard::clone(const ColoredBoard<C>& b, const RepetitionKeys& other, unsigned ply) const {
    for (int i = ply+fiftyMovesRoot; i+b.fiftyMoves >= ply+fiftyMovesRoot; --i)
        keys[i] = other[i];
}
