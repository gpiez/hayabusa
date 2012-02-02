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
#include "eval.h"

template<Colors C>
__v8hi Eval::estimate(const Move m, const KeyScore k) const {
    return inline_estimate<C>(m, k); }

template<Colors C>
__v8hi Eval::inline_estimate(const Move m, const KeyScore keyScore) const {
    enum { pov = C == White ? 0:070 };
    using namespace SquareIndex;
    ASSERT(m.piece());
    if (m.isSpecial()) {
        unsigned piece = m.piece() & 7;
        if (piece == King) {
            ASSERT(m.capture() == 0);
            __v8hi estKing = keyScore.vector
                             - getKSVector(C*King, m.from())
                             + getKSVector(C*King, m.to());
            if (m.to() == (pov^g1)) {
                return estKing - getKSVector(C*Rook, pov^h1)
                       + getKSVector(C*Rook, pov^f1); }
            else {
                return estKing - getKSVector(C*Rook, pov^a1)
                       + getKSVector(C*Rook, pov^d1); } }
        else if (piece == Pawn) {
            return keyScore.vector - getKSVector(C*Pawn, m.from())
                   + getKSVector(C*Pawn, m.to())
                   - getKSVector(-C*Pawn, m.to()-C*8);

        }
        else {
            return keyScore.vector - getKSVector(C*Pawn, m.from())
                   + getKSVector(C*piece, m.to())
                   - getKSVector(-C*m.capture(), m.to()); } }
    else {
        return keyScore.vector
               - getKSVector(C*m.piece(), m.from())
               + getKSVector(C*m.piece(), m.to())
               - getKSVector(-C*m.capture(), m.to()); } }
