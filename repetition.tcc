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
#include "rootboard.h"

/*
 * search for an position repetition
 * reverse search only the last b.fiftyMoves, skipping the first because it
 * can't be a repetition. fiftyMoves may be given in a fen position, but the
 * moves leading to the start position may not, so we need the second condition
 */
template<Colors C>
inline bool RootBoard::find(const ColoredBoard<C>& b, Key k, unsigned ply) const {
    for (unsigned i = ply+rootPly; i+b.fiftyMoves >= ply+rootPly+4 && i>=4; i-=2) {
        ASSERT(i<=nMaxGameLength);
        if (keys[i-4] == k) return true;
    }
    return false;
}
inline void RootBoard::store(Key k, unsigned ply) {
    keys[ply+rootPly] = k;
}
template<Colors C>
inline void RootBoard::clone(const ColoredBoard<C>& b, const RepetitionKeys& other, unsigned ply) const {
    for (unsigned i = ply+rootPly; i+b.fiftyMoves >= ply+rootPly; --i) {
        keys[i] = other[i];
        if (!i) break;
    }
}
