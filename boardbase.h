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

#include "transpositiontable.h"
#include "score.h"
#include "eval.h"

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
        uint64_t enPassant;
    };
};

class RootBoard;

struct BoardBase {
    uint64_t occupied[nColors];
    uint64_t occupied1;
    uint64_t fill;
    uint64_t pins[nColors];                        // +16
    uint64_t attacks[nPieces+1][nColors];        // +32
    uint64_t pieces[nPieces+1][nColors];        // +144
    union {
        struct {
        __v2di d02;
        __v2di d13;
        };
        uint64_t d[4];
    } dpins[nColors],                            //+256
      datt[nColors],                             //+288 sum of all directed attacks, for each of the 4 main directions
      kingIncoming[nColors];                    //+320

    struct MoveTemplateB {
        Move move;
        __v2di d13;
    } bsingle[nColors][2+8+1];                    //704

    struct MoveTemplateR {
        Move move;
        __v2di d02;
    } rsingle[nColors][2+8+1];                   //704

    struct MoveTemplateQ {
        Move move;
        __v2di d02, d13;
    } qsingle[nColors][1+8+1];                   //960

    int material;

    struct XB {
        __v2di x;
        __v2di b;
    };
    static const XB mask02[nSquares];
    static const __v2di mask13x[nSquares]; // 1 KByte  antidiag : diagonal, excluding square
    static const __v2di doublebits[nSquares]; // 1 KByte    1<<sq  : 1<<sq
    static const __v2di doublereverse[nSquares]; // 1 KByte    1<<sq  : 1<<sq
    static const uint64_t knightAttacks[nSquares];
    static const uint64_t kAttacked1[nSquares];
    static const uint64_t kAttacked2[nSquares];
    static uint64_t kingAttacks[16][nSquares];
    static uint64_t epTab[nPieces+1][nSquares];

    template<int C>
    uint64_t getOcc() const {
        return occupied[C==Black];
    }
    template<int C, Pieces P>
    uint64_t getPieces() const {
        static_assert(P>0 && P<=King, "Wrong Piece");
        return pieces[P][C==Black];
    }
    template<int C, Pieces P>
    uint64_t& getPieces() {
        static_assert(P>0 && P<=King, "Wrong Piece");
        return pieces[P][C==Black];
    }
    template<int C>
    uint64_t getPieces(unsigned int p) const {
        ASSERT(p <= King);
        return pieces[p][C==Black];
    }
    template<int C>
    uint64_t& getPieces(unsigned int p) {
        ASSERT(p <= King);
        return pieces[p][C==Black];
    }
    template<int C, Pieces P>
    uint64_t getAttacks() const {
        static_assert(P>0 && P<=All, "Wrong Piece");
        return attacks[P-1][C==Black];
    }
    template<int C, Pieces P>
    uint64_t& getAttacks() {
        static_assert(P>0 && P<=All, "Wrong Piece");
        return attacks[P-1][C==Black];
    }
    template<Colors C, unsigned int dir>
    uint64_t getPins() const {
        enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
        return dpins[CI].d[((dir & 1) << 1) + ((dir & 2) >> 1)];
    }
    template<Colors C>
    bool inCheck() const {
        return getPieces<C,King>() & getAttacks<-C,All>();
    }
    template<Colors C> void setPiece(unsigned int piece, unsigned int pos, const Eval& e) {
        enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
        getPieces<C>(piece) |= 1ULL << pos;
        occupied[CI] |= 1ULL << pos;
        occupied1 |= 1ULL << pos;
        keyScore.vector += e.getKSVector(C*piece, pos);
        material += materialTab[piece];
    }

    inline __v2di build02Attack(const unsigned sq) const;
    inline __v2di build13Attack(const unsigned sq) const;
    inline __v2di build02Attack(const __v2di) const;
    inline __v2di build13Attack(const __v2di) const;
    inline uint64_t build02Attack(const uint64_t) const;
    inline uint64_t build13Attack(const uint64_t) const;
    inline uint64_t buildNAttack(uint64_t n) const;
    template<Colors C> inline void buildAttacks();
    template<Colors C> inline void buildPins();
    void buildAttacks();
    KeyScore keyScore;
    unsigned fiftyMoves;
    CastlingAndEP cep;
    static Castling castlingMask[nSquares];
    void init();
    void print() const;
    int getPiece(unsigned pos) const;
    inline unsigned getPieceFromBit(uint64_t bit) const {
        return
            (getPieces<White,King>() | getPieces<Black,King>()) & bit ? King:
            (getPieces<White,Pawn>() | getPieces<Black,Pawn>()) & bit ? Pawn:
            (getPieces<White,Knight>() | getPieces<Black,Knight>()) & bit ? Knight:
            (getPieces<White,Queen>() | getPieces<Black,Queen>()) & bit ? Queen:
            (getPieces<White,Bishop>() | getPieces<Black,Bishop>()) & bit ? Bishop:
            (getPieces<White,Rook>() | getPieces<Black,Rook>()) & bit ? Rook:
            NoPiece;
    }
    static void initTables();

} ALIGN_CACHE;                                    //sum:        3C0

#endif /* BOARDBASE_H_ */
