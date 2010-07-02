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
#ifndef BOARDBASE_H_
#define BOARDBASE_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include <emmintrin.h>

#include "attacks.h"
#include "length.h"
#include "piecelist.h"
#include "transpositiontable.h"
#include "score.h"
#include "eval.h"
/*
 * Global variables imported by setpiece.asm
 */
#ifndef BITBOARD
extern "C" {
    extern SAttack shortAttacks[nPieces+1][nSquares][nColors][nSquares] ALIGN_PAGE;
    extern const __v16qi vecLookup[4] ALIGN_CACHE;
    extern int squareControl[nSquares] ALIGN_PAGE;
}
#endif

union Castling {
    struct {
        bool q,k;
    } color[nColors];
    uint32_t data4;
};

// Puts castling flags and e. p. square together in one struct, to make xoring
// with the zobrist key faster. Strictly 7 bits are suffient, 3 for the e. p.
// row and 4 for the bits, but this simplfies access.
union CastlingAndEP {
    struct {
        Castling castling;
#ifdef BITBOARD
        uint64_t enPassant;
#else        
        int enPassant;
#endif        
    };
#ifndef BITBOARD    
    uint64_t data8;
#endif    
};

class RootBoard;

struct BoardBase {
#ifdef BITBOARD
    union {
        __v2di occupied2;
        uint64_t occupied1;
    };
    union {
        struct {
        __v2di dir02;   // 0, if piece at this position is pinned in dir 0/2
        __v2di dir13;   // 0, if piece at this position is pinned in dir 1/3
        uint64_t pins;
        };
        uint64_t dir[4];
    } pins[nColors];
    union {
    	struct {
        __v2di d02;
        __v2di d13;
    	};
    	uint64_t d[4];
    } datt[nColors],			// sum of all directed attacks, for each of the 4 main directions
      kingIncoming[nColors];
    uint64_t pieces[16];
    uint64_t attacks[16];
    __v2di single[nColors][32];
    Move moves[nColors][16];
    static __v2di mask02x[nSquares]; // 1 KByte  file : row, excluding square
    static __v2di dir02mask[nSquares]; // 1 KByte  file : row, excluding square
    static __v2di dir13mask[nSquares]; // 1 KByte  antidiag : diagonal, excluding square
    static __v2di doublebits[nSquares]; // 1 KByte    1<<sq  : 1<<sq
    static __v2di doublereverse[nSquares]; // 1 KByte    1<<sq  : 1<<sq
    static const __v16qi swap16; // needs to be initialized to swap the bytes in both quad-words
    static uint64_t knightAttacks[nSquares];
    static uint64_t kingAttacks[16][nSquares];
    static uint64_t epTab[nPieces+1][nSquares];
    template<int C, Pieces P>
    uint64_t getPieces() const {
        static_assert(P>0 && P<=All, "Wrong Piece");
        return pieces[nPieces+1 + C*P];
    }
    template<int C, Pieces P>
    uint64_t& getPieces() {
        static_assert(P>0 && P<=All, "Wrong Piece");
        return pieces[nPieces+1 + C*P];
    }
    template<int C>
    uint64_t& getPieces(unsigned int p) {
        ASSERT(p <= King && p>=0);
        return pieces[nPieces+1 + C*p];
    }
    template<int C, Pieces P>
    uint64_t getAttacks() const {
        static_assert(P>0 && P<=All, "Wrong Piece");
        return attacks[nPieces+1 + C*P];
    }
    template<int C, Pieces P>
    uint64_t& getAttacks() {
        static_assert(P>0 && P<=All, "Wrong Piece");
        return attacks[nPieces+1 + C*P];
    }
    template<Colors C, unsigned int dir>
    uint64_t getPins() const {
        enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
        return pins[CI].dir[((dir & 1) << 1) + ((dir & 2) >> 1)];
    }
    template<Colors C>
    bool inCheck() const {
        return getPieces<C,King>() & getAttacks<-C,All>();
    }
    template<Colors C> void setPiece(unsigned int piece, unsigned int pos) {
        getPieces<C>(piece) |= 1ULL << pos;
        occupied2 |= _mm_set1_epi64x(1ULL << pos);
    }

    __v2di build02Attack(const __v2di o, const unsigned sq) const;
    __v2di build13Attack(const __v2di o, const unsigned sq) const;
    template<Colors C> void buildAttacks();
    template<Colors C> void buildPins();
    void buildAttacks();
#else
    LongIndex    attVec[nDirs/2][nSquares];        //0x100
    Length        attLen[nDirs/2][nSquares];        //0x100
    LAttack        longAttack[nColors][nSquares];    //0x080
    SAttack        shortAttack[nColors][nSquares];    //0x080
    int8_t        pieces[nSquares];                //0x040
    PieceList     pieceList[nColors];                //0x040
    static union SevenMoves {
        struct {
            __m128i m0123;
            __m128i m456;
        };
        Move single[7];
    } moveOffsetTable[nSquares][4];
    static union FourMoves {
        __m128i m0123;
        Move single[4];
    } moveFromTable[nSquares];
    static uint8_t totalLen[64];
    static Length borderTable[0x100];
    static uint64_t knightDistanceTable[nSquares];
    static uint64_t kingDistanceTable[nSquares];
    static uint8_t vec2pin[nSquares][nSquares];
    static bool attPinTable[256][256];

    template<Colors C>
    bool inCheck() {
        enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
        return b.template attacks<EI>(b.pieceList[CI].getKing()) & attackMask;
    }
    static bool isKnightDistance(int from, int to) {
        return knightDistanceTable[from] >> to & 1;
    }
    static bool isKingDistance(int from, int to) {
        return kingDistanceTable[from] >> to & 1;
    }
    unsigned int getLen(unsigned int dir, unsigned int pos) {
        if (dir<4)
            return attLen[dir][pos].right;
        else
            return attLen[dir-4][pos].left;
    }


    // copy pieces and pieceList
    void copyPieces(BoardBase* next) const {
        __m128i xmm0 = _mm_load_si128(((__m128i *)pieces) + 0);
        __m128i xmm1 = _mm_load_si128(((__m128i *)pieces) + 1);
        __m128i xmm2 = _mm_load_si128(((__m128i *)pieces) + 2);
        __m128i xmm3 = _mm_load_si128(((__m128i *)pieces) + 3);
        _mm_store_si128(((__m128i *)next->pieces) + 0, xmm0);
        _mm_store_si128(((__m128i *)next->pieces) + 1, xmm1);
        _mm_store_si128(((__m128i *)next->pieces) + 2, xmm2);
        _mm_store_si128(((__m128i *)next->pieces) + 3, xmm3);
        xmm0 = _mm_load_si128(((__m128i *)pieceList) + 0);
        xmm1 = _mm_load_si128(((__m128i *)pieceList) + 1);
        xmm2 = _mm_load_si128(((__m128i *)pieceList) + 2);
        xmm3 = _mm_load_si128(((__m128i *)pieceList) + 3);
        _mm_store_si128(((__m128i *)next->pieceList) + 0, xmm0);
        _mm_store_si128(((__m128i *)next->pieceList) + 1, xmm1);
        _mm_store_si128(((__m128i *)next->pieceList) + 2, xmm2);
        _mm_store_si128(((__m128i *)next->pieceList) + 3, xmm3);
    }
    template<unsigned int ColorIndex>
    Attack attacks( unsigned int pos) const {
        return (Attack) {{
            longAttack[ColorIndex][pos],
            shortAttack[ColorIndex][pos]
        }};
    }
    template<Colors C> void setPiece(uint8_t piece, uint8_t pos, const Eval&);
    template<Colors C> void clrPiece(uint8_t piece, uint8_t pos, const Eval&);
    template<Colors C> void chgPiece(uint8_t oldpiece, uint8_t piece, uint8_t pos, const Eval&);
    template<Colors C> void copyBoardClrPiece(const BoardBase* prev, uint8_t piece, uint8_t pos, const Eval&);
    template<Colors C> void setPieceEst(uint8_t piece, uint8_t pos);
    template<Colors C> void clrPieceEst(uint8_t piece, uint8_t pos);
    template<Colors C> void chgPieceEst(uint8_t oldpiece, uint8_t piece, uint8_t pos);
    template<Colors C> void copyBoardClrPieceEst(const BoardBase* prev, uint8_t piece, uint8_t pos);
#endif
    KeyScore keyScore;
    unsigned int fiftyMoves;
    CastlingAndEP cep;
    static Castling castlingMask[nSquares];
    void init();
    void print();


    static void initTables();

} ALIGN_CACHE;                                    //sum:        3C0

#endif /* BOARDBASE_H_ */
