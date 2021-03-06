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

#include "move.h"
#include "keyscore.h"

class Eval;
class Game;
enum Extension: unsigned;

struct Board {
    // Puts castling flags and e. p. square together in one struct, to make xoring
    // with the zobrist key faster. Strictly 7 bits are suffient, 3 for the e. p.
    // row and 4 for the bits, but this simplifies access.
    // FEATURE no longer relevant, this is split up in getZobrist() anyway
    // FEATURE use faster copy constructor for job()
    union Castling {
        struct {
            bool q,k; } color[nColors];
        uint32_t data4; };
    struct CastlingAndEP {
        Castling castling;
        uint64_t enPassant; };

    uint64_t occupied[nColors];                         // +  0
    uint64_t occupied1;                                 // + 16
    uint64_t fill;                                      // + 24
    uint64_t pins[nColors];                             // + 32
    uint64_t attacks[nPieces+1][nColors];               // + 48
    uint64_t pieces[nPieces+1][nColors];                // +160
    union {
        struct {
            __v2di d02;
            __v2di d13; };
        struct {
            uint64_t d0;
            uint64_t d2;
            uint64_t d1;
            uint64_t d3; };
        uint64_t d[4]; }

        dpins[nColors],                                 // +272
        datt[nColors],                                  // +304 sum of all directed attacks, for each of the 4 main directions
        kingIncoming[nColors];                          // +336
                                                        //  372 total size of core structure
    struct MoveTemplateB {
        Move move;
        __v2di d13; } bsingle[nColors][2+8+1];                   //704

    struct MoveTemplateR {
        Move move;
        __v2di d02; } rsingle[nColors][2+8+1];                  //704

    struct MoveTemplateQ {
        Move move;
        __v2di d02, d13; } qsingle[nColors][1+8+1];                  //960

    KeyScore keyScore;
    mutable CastlingAndEP cep;
    mutable unsigned fiftyMoves;
    unsigned matIndex;
    mutable int positionalScore;
    mutable int prevPositionalScore;

    mutable int16_t* diff;
    mutable int psValue;
    mutable int estScore;
    mutable unsigned ply;
    mutable Move m;
    mutable Extension threatened;

    struct Bits {
        __v2di mask02;
        __v2di mask13; // 1 KByte  antidiag : diagonal, excluding square
        __v2di doublebits; // 1 KByte    1<<sq  : 1<<sq
        __v2di doublereverse; // 1 KByte    1<<(sq^070)  : 1<<(sq^070)
        __v2di doublereverse2; // 1 KByte    1<<sq  : 1<<sq
        uint64_t mask0x;
        uint64_t mask2x;
//        uint64_t mask1x;
//        uint64_t mask3x;
        char fill[16]; };
    static const Bits bits[nSquares];
    static const uint64_t knightAttacks[nSquares+2];
    static const uint64_t pawnAttacks[nColors][nSquares+2];
    static const uint64_t kAttacked1[nSquares+2];
    static const uint64_t kAttacked2[nSquares+2];
    static const uint64_t kAttacked[nSquares+2];
    static uint64_t kingAttacks[16][nSquares];
    static Castling castlingMask[nSquares];

    template<int C>
    uint64_t getOcc() const {
        return occupied[C==Black]; }
    template<int C, Pieces P>
    uint64_t getPieces() const {
        static_assert(P>0 && P<=King, "Wrong Piece");
        return pieces[P][C==Black]; }
    template<int C, Pieces P>
    uint64_t& getPieces() {
        static_assert(P>0 && P<=King, "Wrong Piece");
        return pieces[P][C==Black]; }
    template<Pieces P>
    __v2di get2Pieces() const {
        static_assert(P>0 && P<=King, "Wrong Piece");
#ifdef __SSE__
        return _mm_load_si128((__m128i*)&pieces[P]);
#else
        return *(__v2di*) &pieces[P];
#endif
    }
    template<int C>
    uint64_t getPieces(unsigned int p) const {
        ASSERT(p <= King);
        return pieces[p][C==Black]; }
    template<int C>
    uint64_t& getPieces(unsigned int p) {
        ASSERT(p <= King);
        return pieces[p][C==Black]; }
    template<int C, Pieces P>
    uint64_t getAttacks() const {
        static_assert(P>0 && P<=All, "Wrong Piece");
        return attacks[P-1][C==Black]; }
    template<int C, Pieces P>
    uint64_t& getAttacks() {
        static_assert(P>0 && P<=All, "Wrong Piece");
        return attacks[P-1][C==Black]; }
    template<Colors C, unsigned int dir>
    uint64_t getPins() const {
        enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
        switch(dir) {
        case 0: return dpins[CI].d0;
        case 1: return dpins[CI].d1;
        case 2: return dpins[CI].d2;
        case 3: return dpins[CI].d3; }
//         return dpins[CI].d[((dir & 1) << 1) + ((dir & 2) >> 1)];
    }
    template<int C>
    uint64_t getPins() const {
        enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
        return pins[CI]; }
    template<Colors C>
    bool inCheck() const {
        return getPieces<C,King>() & getAttacks<-C,All>(); }
    template<Colors C> void setPiece(unsigned piece, unsigned pos, const Eval& e);

    inline __v2di build02Attack(const unsigned sq) const __attribute__((always_inline));
    inline __v2di build13Attack(const unsigned sq) const __attribute__((always_inline));
//    inline void build02Attack(const __v2di, __v2di& result0, __v2di& result1) const __attribute__((always_inline));
//    inline __v2di build13Attack(const __v2di) const;
//    uint64_t build02Attack(const uint64_t) const;
//    uint64_t build13Attack(const uint64_t) const;
    inline uint64_t buildNAttack(uint64_t n) const;
    template<Colors C> inline void buildAttacks() __attribute__((always_inline));
    template<Colors C> inline void buildPins() __attribute__((always_inline));
    void buildAttacks() __attribute__((noinline));
    void init();
    void print() const;
    unsigned getPieceKind(uint64_t bit) const;
    int getPiece(unsigned pos) const;
    static void initTables();
    void copyPieces(Board& next) const; } ALIGN_CACHE;                                   //sum:        3C0

#endif /* BOARDBASE_H_ */
