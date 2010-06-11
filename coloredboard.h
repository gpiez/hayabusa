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
public:    
    static const unsigned int CI = (White-C)/(White-Black);    // ColorIndex, 0 for White, 1 for Black
private:    
    static const unsigned int EI = (C-Black)/(White-Black);    // EnemyIndex, 1 for White, 0 for Black
    static const uint8_t pov = CI*56;    //for xoring square values to the other side
    static uint8_t diaPinTable[nDirs][256];
    
public:

    ColoredBoard() {};
    ColoredBoard(const ColoredBoard<(Colors)-C>& prev, Move m, const Eval&);
    ColoredBoard(const ColoredBoard<(Colors)-C>& prev, Move m, __v8hi est, uint64_t cep);
    ColoredBoard(const ColoredBoard<C>& prev, Move m, const Eval&);
    ColoredBoard(const ColoredBoard<C>& prev, Move m, __v8hi est, uint64_t cep);
/*    operator const ColoredBoard<(Colors)-C>& () const {
        return *(ColoredBoard<(Colors)-C>*)(void*)this;
    }*/
    static void initTables();
    Move* generateCaptureMoves(Move* list) const;
    Move* generateMoves(Move* list) const;
    void doMove(ColoredBoard<(Colors)-C>* next, Move m, const Eval&) const;
    void doMoveEst(ColoredBoard<(Colors)-C>* next, Move m, uint64_t cep) const;
    inline __v8hi estimatedEval(const Move m, const Eval& rb, uint64_t&) const;
    Key getZobrist() const;
    
private:
    
    uint8_t detectPin( unsigned int pos) const;
    void ray(Move* &list, uint8_t from, uint8_t dir) const;
    void ray_vectorized(Move* &list, uint8_t from, uint8_t dir) const;
    void generateTargetMove(Move* &list, uint8_t to) const;
    void generateTargetCapture(Move* &list, uint8_t to, int8_t cap, Attack a) const;
    uint64_t perft(unsigned int depth) const;
    void divide(unsigned int depth) const;

    bool isPromoRank(uint8_t pos) const {
        return (C==White) ? (pos >= a7) : (pos <= h2);
    }

    template<uint8_t R>
    bool isRank(uint8_t pos) const {
        static_assert( R>=1 && R<=8 );
        return (pos >= 28-C*36+C*R*8) & (pos < 36-C*36+C*R*8);
    }
    int8_t index( unsigned int dir, unsigned int pos) const {
        return dir<4 ? attVec[dir&3][pos].rIndex : attVec[dir&3][pos].lIndex;
    }

    uint8_t length( uint8_t dir, uint8_t pos) const {
        return dir<4 ? attLen[dir&3][pos].right : attLen[dir&3][pos].left;
    }

    bool isValid(uint8_t dir) const {
        return dir < 0x80;
    }

    bool isLongAttack( unsigned int dir, unsigned int pos) const {
        return (index(dir, pos) == -C*3) | (index(dir, pos) == -C*(int)((dir&1)+1));
    }

public:

    template<int piece>    bool attackedBy(uint8_t pos);
};

#endif /* COLOREDBOARD_H_ */
