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

#include "board.h"
#include "bits.h"

class Game;
class TestRootBoard;

enum MoveType { AllMoves, NoUnderPromo, NoKingPawn, OnlyCaptures };
/*
 * Board with <color> to move. Serves as a common template for all color-dependant functions.
 */
template<Colors C>
class ColoredBoard: public Board {
    friend class TestRootBoard;
//    static uint8_t diaPinTable[nDirs][256];

public:
    using Board::init;
    static constexpr unsigned CI = C == White ? 0:1;
    static constexpr unsigned EI = C == White ? 1:0;
    static constexpr uint8_t pov = CI*56;    //for xoring square values to the other side

    ColoredBoard() = default;
    const ColoredBoard<(Colors)-C>& swapped() const {
        const union {
            const ColoredBoard<C>* const normal;
            const ColoredBoard<(Colors)-C>* const swapped; } swapper = { this };
        return *(swapper.swapped); }
    template<typename T>
    inline ColoredBoard(const T& prev, Move m, __v8hi est);
    void init(const ColoredBoard<(Colors)-C>& prev, Move m);
    template<typename T>
    ColoredBoard(const T& prev, Move m, Game&);
    void init(const ColoredBoard<(Colors)-C>& prev, Move m, Game&);
    static void initTables();
    template<MoveType>
    void generateCaptureMoves(Move*& list, Move*& bad) const __attribute__((noinline));
    template<MoveType=AllMoves>
    void generateCheckEvasions(Move*& list, Move*& bad) const __attribute__((noinline));
    template<bool AbortOnFirst, typename R>
    R generateMateMoves( Move** good = NULL, Move** bad = NULL) const __attribute__((noinline));
    template<bool AbortOnFirst, typename R>
    R generateDiscoveredCheck( Move** good = NULL) const __attribute__((noinline));
    bool generateSkewers( Move** good) const __attribute__((noinline));
    bool generateForks( Move** good) const __attribute__((noinline));
    void generateNonCap(Move*& good, Move*& bad) const __attribute__((noinline));
    void doMove(Board* next, Move m) const;
    void doMove2(Board* next, Move m) const;
    void doMove(Board* next, Move m, const Eval&) const;
    void doSpecialMove(Board* next, Move m, uint64_t from, uint64_t to) const;
    void doSpecialMove2(Board* next, Move m, uint64_t from, uint64_t to) const;
    void doSpecialMove(Board* next, Move m, uint64_t from, uint64_t to, const Eval&) const;

    bool isForked() const __attribute__((noinline));
    uint64_t isPieceHanging(const Eval& e) const;
    inline Key getZobrist() const;
private:
    void generateTargetMove(Move*& bad, uint64_t tobit) const;
    template<MoveType>
    void generateTargetCapture(Move*& list, Move*& bad, uint64_t to, unsigned cap) const;
    uint64_t perft(unsigned int depth) const;
    void divide(unsigned int depth) const;

    template<int R>
    uint64_t rank() const {
        return ::rank<C,R>(); }
    uint64_t generateRookMates( uint64_t checkingMoves, uint64_t blockedEscapes, uint64_t undefended, /*uint64_t king, */unsigned k) const;
    uint64_t generateKnightMates(uint64_t block, uint64_t king, unsigned k) const; };
#endif /* COLOREDBOARD_H_ */
