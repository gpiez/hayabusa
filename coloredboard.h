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
#ifndef COLOREDBOARD_H_
#define COLOREDBOARD_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "boardbase.h"

class Eval;
class TestRootBoard;
/*
 * Board with <color> to move. Serves as a common template for all color-dependant functions.
 */
template<Colors C>
class ColoredBoard: public BoardBase {

    friend class TestRootBoard;
//    static uint8_t diaPinTable[nDirs][256];

public:
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    static const uint8_t pov = CI*56;    //for xoring square values to the other side

    ColoredBoard() = default;
    template<typename T> ColoredBoard(const T& prev, Move m, __v8hi est);
    inline __v8hi estimatedEval(const Move m, const Eval& rb) const;
    template<bool> void generateTargetCapture(Move* &list, Move* &bad, uint64_t to, unsigned int cap) const;
    static void initTables();
    template<bool> void generateCaptureMoves(Move* &list, Move* &bad) const;
    void generateCheckEvasions(Move* &list, Move* &bad) const;
    template<bool AbortOnFirst> bool generateMateMoves( Move** good = NULL) const;
    void generateMoves(Move* &good) const;
    template<typename T>
    void doMove(T* next, Move m) const;
    void doMoveEst(ColoredBoard<(Colors)-C>* next, Move m, uint64_t cep) const;
    Key getZobrist() const;
private:
    void generateTargetMove(Move* &good, uint64_t tobit) const;
    uint64_t perft(unsigned int depth) const;
    void divide(unsigned int depth) const;

    template<int R>
    uint64_t rank() const {
    	return ::rank<C,R>();
    }
};
#endif /* COLOREDBOARD_H_ */
