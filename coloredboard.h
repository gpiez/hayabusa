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

    ColoredBoard() = default;
#ifdef BITBOARD
    template<typename T> ColoredBoard(const T& prev, Move m, __v8hi est);
    inline __v8hi estimatedEval(const Move m, const Eval& rb) const;
    template<bool> void generateTargetCapture(Move* &list, Move* &bad, uint64_t to, unsigned int cap) const;
#else
    ColoredBoard(const ColoredBoard<(Colors)-C>& prev, Move m, const Eval&);
    ColoredBoard(const ColoredBoard<(Colors)-C>& prev, Move m, __v8hi est, uint64_t cep);
    ColoredBoard(const ColoredBoard<C>& prev, Move m, const Eval&);
    ColoredBoard(const ColoredBoard<C>& prev, Move m, __v8hi est, uint64_t cep);
    inline __v8hi estimatedEval(const Move m, const Eval& rb, uint64_t&) const;
    void generateTargetCapture(Move* &list, unsigned int to, unsigned int cap) const;
#endif    
//    ColoredBoard(const ColoredBoard<(Colors)-C>& prev, Move m) {
//        prev.doMove(this, m);
//        buildAttacks();
//    }

/*    operator const ColoredBoard<(Colors)-C>& () const {
        return *(ColoredBoard<(Colors)-C>*)(void*)this;
    }*/
    static void initTables();
    template<bool> Move* generateCaptureMoves(Move* list, Move* &bad) const;
    Move* generateMoves(Move* list) const;
    template<typename T>
    void doMove(T* next, Move m) const;
    void doMoveEst(ColoredBoard<(Colors)-C>* next, Move m, uint64_t cep) const;
    Key getZobrist() const;
#ifdef BITBOARD
#else    
    uint8_t getKing() const {
        return pieceList[CI].getKing();
    }
    uint8_t getOKing() const {
        return pieceList[EI].getKing();
    }
    Attack getAttacks(uint8_t pos) const {
        return attacks<CI>(pos);     
    }
    Attack getOAttacks(uint8_t pos) const {
        return attacks<EI>(pos);
    }
#endif
private:    
    void generateTargetMove(Move* &list, uint64_t tobit) const;
    uint64_t perft(unsigned int depth) const;
    void divide(unsigned int depth) const;

#ifndef BITBOARD    
    bool isPromoRank(uint8_t pos) const {
        return (C==White) ? (pos >= a7) : (pos <= h2);
    }

    template<uint8_t R>
    bool isRank(uint8_t pos) const {
        static_assert( R>=1 && R<=8, "Wrong Rank" );
        return (pos >= 28-C*36+C*R*8) & (pos < 36-C*36+C*R*8);
    }
#endif
    
    template<int R>
    uint64_t rank() const {
    	return ::rank<C,R>();
    }
#ifdef BITBOARD
#else    
    uint8_t detectPin( unsigned int pos) const;
    bool detectPin( unsigned int pos, unsigned int dir) const;
    void ray(Move* &list, uint8_t from, uint8_t dir) const;
    void ray_vectorized(Move* &list, uint8_t from, uint8_t dir) const;
    int8_t index( unsigned int dir, unsigned int pos) const {
        return dir<4 ? attVec[dir&3][pos].rIndex : attVec[dir&3][pos].lIndex;
    }

    uint8_t length( uint8_t dir, uint8_t pos) const {
        return dir<4 ? attLen[dir&3][pos].right : attLen[dir&3][pos].left;
    }
    bool isLongAttack( unsigned int dir, unsigned int pos) const {
        return (index(dir, pos) == -C*3) | (index(dir, pos) == -C*(int)((dir&1)+1));
    }
#endif

    bool isValid(uint8_t dir) const {
        return dir < 0x80;
    }
public:

    template<int piece> bool attackedBy(uint8_t pos);
};

#endif /* COLOREDBOARD_H_ */
