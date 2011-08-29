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
#ifndef PCH_H_
#include <pch.h>
#endif

#include "eval.h"
/*
 * called after a capture move of opponent
 * returns true if the score returned is an exact score
 */
template<Colors C>
bool Eval::draw(const BoardBase& b, int& upperbound) const {
    upperbound =  C*infinity;
    if (b.getPieces<White, Pawn>() | b.getPieces<Black, Pawn>()) return false;
#if 0    
    if (b.getPieces<White, Pawn>() | b.getPieces<Black, Pawn>()) {
        if (b.material) return;
        if (popcount(b.getPieces<White, Pawn>() | b.getPieces<Black, Pawn>()) != 1) return;
        uint64_t p = b.getPieces<White, Pawn>() | b.getPieces<Black, Pawn>();
        uint64_t win,lose;
        int value;
        if (b.getPieces<C, Pawn>()) {
            win = b.getPieces<C,King>();
            lose = b.getPieces<-C,King>();
            //int kwin = bit(win);
            int klose = bit(lose);
            int pwin = bit(p);
            if ((pwin & 070) > (klose & 070) + 010) {
                upperbound = value;
                return true;
            }
        }
    }
#endif
    switch (b.material) {
    case 0:
    case materialBishop:
        upperbound = 0;
        return true;
    case 2*materialBishop:
        if (b.getPieces<White,Bishop>() | b.getPieces<White,Knight>()
            && b.getPieces<Black,Bishop>() | b.getPieces<Black,Knight>()) {
            upperbound = 0;
            return true;
        }
        break;
    }
    uint64_t RQP = b.getPieces<C, Rook>() + b.getPieces<C, Queen>() + b.getPieces<C, Pawn>();
    if (!RQP) {
        if (popcount(b.getPieces<C, Bishop>() + b.getPieces<C, Knight>()) <= 1) //FIXME something wrong here, see tournament 37 game 154
            upperbound = 0;
    }
    return false;
}
