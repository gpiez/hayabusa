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
#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#ifndef PCH_H_
#include <pch.h>
#endif

//#define static_assert(x) char __y[(x) ? 1 : -1 ] __attribute__((unused));

#ifndef NDEBUG
#define ASSERT(x) do { if (!(x)) { \
                std::cerr << std::endl << "Assertion " << #x << " failed." << std::endl\
                <<  __FILE__ << __PRETTY_FUNCTION__ << __LINE__ << std::endl; \
                asm("int3"); \
} } while(0)
#define TRACE_DEBUG 1
#else
#define ASSERT(x)
#define TRACE_DEBUG 0
#endif

#define print_debug(mask, fmt, ...) \
            do { if(TRACE_DEBUG) if (mask & Options::debug) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#undef foreach

#define CACHE_LINE_SIZE 64
#define ALIGN_XMM __attribute__((aligned(16)))
#define ALIGN_CACHE __attribute__((aligned(CACHE_LINE_SIZE)))
#define ALIGN_PAGE __attribute__((aligned(4096)))

#define STR(x) #x

enum GamePhase { Opening, Endgame };

enum Pieces { NoPiece = 0, Rook = 1, Bishop = 2, Queen = 3, Knight = 4, Pawn = 5, King = 6, All = 7 };

enum Colors { Black = -1, White = 1 };

enum Square {
    a1 = 0, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};

enum Phase { root, trunk, tree, mate, leaf, vein };

enum Sides { KSide, QSide, Middle };

enum NodeType { NodeFailLow, NodeFailHigh, NodePV, NodeFull, NodeTT, NodePrecut1, NodePrecut2, NodePrecut3, NodeNull, NodeFutile1, NodeFutile2, NodeFutile3, NodeMate, NodePresearch, NodeIllegal, NodeRepetition, NodeEndgameEval };

enum NodeFlag { Threatened = 1, Extend = 2 };

typedef uint64_t Key;
typedef uint32_t PawnKey;

static const int hashDefaultSize = 16ULL; //Mibibytes
// tcp server incurs some lag
static const std::chrono::milliseconds operatorTime = std::chrono::milliseconds(200);
static const std::chrono::milliseconds minimumTime = std::chrono::milliseconds(100);
static const unsigned int maxMovesNull = 8;     // So a lone king with 8 possible moves will always inhibit null moves
static const int maxRows = 1000;
static const unsigned int maxThreadId = 255;
static const int maxHistory = 256;
static const int nullReduction = 3;
static const unsigned int nTTLocks = 1;
static const unsigned int maxMoves = 256; // maximum possible moves in a position
static const unsigned int goodMoves = 192; // maximum possible good moves in a position
static const unsigned int nMaxGameLength = 5000; // maximum possible moves in a position
static const int infinity = 8176;
static const unsigned int maxDepth = 64;
static const unsigned int transpositionTableAssoc = 2;
static const unsigned int pawnTableAssoc = 4;
static const unsigned int nPieces = 6;
static const unsigned int nColors = 2;
static const unsigned int nTotalPieces = nPieces*nColors + 1;   // -King,... -Pawn, 0, Pawn,... King

static const unsigned int nDirs = 8;    //long range directions
static const unsigned int nDirKinds = 2;//kinds of long range direction, horizontal/vertical vs diagonal
static const unsigned int nFiles = 8;
static const unsigned int nRows = 8;
static const unsigned int nSquares = nFiles*nRows;
/*
 * The eight possible directions of movement for a
 * king in a [8][8] array
 */
static const int dirOffsets[8] = { 1, 9, 8, 7, -1, -9, -8, -7 };
static const int xOffsets[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static const int yOffsets[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static const int materialTab[nPieces+1] = { 0, 5, 3, 9, 3, 0, 0 };
static inline int popcount(uint64_t x) {
#ifdef __SSE4_2__
    return _popcnt64(x);
#else
    x -=  x>>1 & 0x5555555555555555;
    x  = ( x>>2 & 0x3333333333333333 ) + ( x & 0x3333333333333333 );
    x  = (( x>>4 )+x) & 0x0f0f0f0f0f0f0f0f;
    x *= 0x0101010101010101;
    return  x>>56;
#endif
}

static __v2di inline pcmpeqq(__v2di a, __v2di b) __attribute__((__always_inline__));
static __v2di inline pcmpeqq(__v2di a, __v2di b) {
#ifdef __SSE4_1__
    return _mm_cmpeq_epi64(a, b);
#else
/*
 * emulate a pcmpeqq instruction by comparing 32 bit wise, then swapping the
 * 32 bit halfes of each 64 bit operand. Only if both 32 bit halfes happen to
 * compare equal, the and operation will set all bits in one 64 bit half.
 */
    a = _mm_cmpeq_epi32(a, b);
    b = _mm_shuffle_epi32(a, 0b10110001);
    return a & b;
#endif
}

static inline uint64_t ror(uint64_t x, unsigned k) __attribute__((__always_inline__));
static inline uint64_t ror(uint64_t x, unsigned k) {
    return __rorq(x, k);
}

static inline uint64_t rol(uint64_t x, unsigned k) __attribute__((__always_inline__));
static inline uint64_t rol(uint64_t x, unsigned k) {
    return __rolq(x, k);
}

static inline unsigned int bit(uint64_t x) __attribute__((__always_inline__));
static inline unsigned int bit(uint64_t x) {
    return __bsfq(x);
}

static inline unsigned int bitr(uint64_t x) __attribute__((__always_inline__));
static inline unsigned int bitr(uint64_t x) {
#ifdef DRAGONEGG
    uint8_t result;
    asm(" bsrq %1, %0 \n"
	: "=r"(result)
	: "r"(x)
    );
#else
    return __bsrq(x);
#endif    
}

static inline uint64_t btr(uint64_t x, uint64_t b) __attribute__((__always_inline__));
static inline uint64_t btr(uint64_t x, uint64_t b) {
    asm(" btrq %1, %0 \n"
	: "=r"(x)
	: "r"(b), "0"(x) 
    );
    return x;
}

static inline unsigned int bit(unsigned int x) __attribute__((__always_inline__));
static inline unsigned int bit(unsigned int x) {
    return __bsfd(x);
}

static inline uint64_t fold(__v2di hilo) __attribute__((__always_inline__));
static inline uint64_t fold(__v2di hilo) {
    return _mm_cvtsi128_si64(_mm_unpackhi_epi64(hilo, hilo))
        | _mm_cvtsi128_si64(hilo);
}

template<int N> uint64_t shift(uint64_t b) __attribute__((__always_inline__));
template<int N> uint64_t shift(uint64_t b) {
    if (N > 0)
        return b << (N & 0x3f);
    else
        return b >> (-N & 0x3f);
}

template<Colors C,int R> uint64_t rank() __attribute__((__always_inline__));
template<Colors C,int R> uint64_t rank() {
    static_assert( R>=1 && R<=8, "Wrong Rank" );
    return 0xffULL << (C == White ? R*8-8:64-8*R);
}

template<char F> uint64_t file() __attribute__((__always_inline__));
template<char F> uint64_t file() {
    static_assert( F>='a' && F<='h', "Wrong File" );
    return 0x0101010101010101ULL << (F-'a');
}

//extern "C" __m128i broadcastTab[256] ALIGN_XMM;
#endif /* CONSTANTS_H_ */
