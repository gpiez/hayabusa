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

//#define EVOLUTION
//#define EXTRA_STATS
#include <cstdint>
#if defined(__x86_64__)
#include <x86intrin.h>
#else
#if defined(__arm__)
#define LEAN_AND_MEAN
/* SSE2 */
typedef double __v2df __attribute__ ((__vector_size__ (16)));
typedef long long __v2di __attribute__ ((__vector_size__ (16)));
typedef int __v4si __attribute__ ((__vector_size__ (16)));
typedef short __v8hi __attribute__ ((__vector_size__ (16)));
typedef char __v16qi __attribute__ ((__vector_size__ (16)));

/* The Intel API is flexible enough that we must allow aliasing with other
   vector types, and their scalar components.  */
typedef long long __m128i __attribute__ ((__vector_size__ (16), __may_alias__));
typedef double __m128d __attribute__ ((__vector_size__ (16), __may_alias__));
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_set_epi64x (long long __q1, long long __q0)
{
  return __extension__ (__m128i)(__v2di){ __q0, __q1 };
}
extern __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_set1_epi64x (long long __A)
{
  return _mm_set_epi64x (__A, __A);
}
//#include <arm_neon.h>
#endif
#endif

#ifndef NDEBUG
#define MYDEBUG
#include <iostream>
#if defined(__x86_64__) || defined(__i386__)
#define ASSERT(x) do { if (!(x)) { \
                std::cerr << std::endl << "Assertion " << #x << " failed." << std::endl\
                <<  __FILE__ << __PRETTY_FUNCTION__ << __LINE__ << std::endl; \
                asm("int3"); \
} } while(0)
#else
#include <assert.h>
#define ASSERT(x) assert(x)
#endif

#define TRACE_DEBUG 1
#else
#define ASSERT(x)
#define TRACE_DEBUG 0
#endif

#define print_debug(mask, fmt, ...) \
            do { if(TRACE_DEBUG) if (mask & Options::debug) std::fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#undef foreach

#define CACHE_LINE_SIZE 64
#define ALIGN_XMM __attribute__((aligned(16)))
#define ALIGN_CACHE __attribute__((aligned(CACHE_LINE_SIZE)))
#define ALIGN_PAGE __attribute__((aligned(4096)))

#define STR(x) #x

#ifndef override
#define override
#endif

#if __SIZEOF_LONG__ == 8
#define likely(x) (__builtin_expect((x), 1))
#define unlikely(x) (__builtin_expect((x), 0))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

enum GamePhase { Opening, Endgame };

enum Pieces { NoPiece = 0, Rook = 1, Bishop = 2, Queen = 3, Knight = 4, Pawn = 5, King = 6, All = 7 };

enum Colors { Black = -1, White = 1 };

namespace SquareIndex {
enum {
    a1 = 0, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8 }; };

enum Phase { root, trunk, tree, mate, leaf, vein };

enum Sides { KSide, QSide, Middle };

enum NodeType { NodeFailLow=-1, NodePV=0, NodeFailHigh=1, NodeFull, NodeTT,
                NodePrecut1, NodePrecut2, NodePrecut3, NodeNull, NodeFutile1, NodeFutile2,
                NodeFutile3, NodeMate, NodePresearch, NodeIllegal, NodeRepetition,
                NodeEndgameEval, NodeStandpat, NodeStart };

enum NodeFlag { Threatened = 1, Extend = 2 };

typedef uint64_t Key;
typedef uint32_t PawnKey;

static constexpr int hashDefaultSize = 16ULL; //Mibibytes
static constexpr int maxRows = 1000;
static constexpr unsigned int maxThreadId = 255;
static constexpr int maxHistory = 256;
static constexpr unsigned int nTTLocks = 1;
static constexpr unsigned int maxMoves = 256; // maximum possible moves in a position
static constexpr unsigned int goodMoves = 192; // maximum possible good moves in a position
static constexpr unsigned int nMaxGameLength = 1000; // maximum moves in a game
static constexpr int mateScore = 0x1ff0; // highest value representable in the score field of a ttentry
static constexpr int mateStep = 0x10;
static constexpr int infinity = mateScore - 0xff*mateStep;
static constexpr unsigned int maxDepth = 63;
static constexpr unsigned int transpositionTableAssoc = 4;
static constexpr unsigned int pawnTableAssoc = 4;
static constexpr unsigned int nPieces = 6;
static constexpr unsigned int nColors = 2;
static constexpr unsigned int nTotalPieces = nPieces*nColors + 1;   // -King,... -Pawn, 0, Pawn,... King

static constexpr unsigned int nDirs = 8;    //long range directions
static constexpr unsigned int nDirKinds = 2;//kinds of long range direction, horizontal/vertical vs diagonal
static constexpr unsigned int nFiles = 8;
static constexpr unsigned int nRows = 8;
static constexpr unsigned int nSquares = nFiles*nRows;
/*
 * The eight possible directions of movement for a
 * king in a [8][8] array
 */
static constexpr int dirOffsets[8] = { 1, 9, 8, 7, -1, -9, -8, -7 };
static constexpr int xOffsets[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static constexpr int yOffsets[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

// For calculating opening...endgame scaling
static constexpr int materialTab[nPieces+1] = { 0, 5, 2, 10, 2, 0, 0 };
static constexpr int materialTotal = 4*(materialTab[Rook]+materialTab[Bishop]+materialTab[Knight]) + 4*materialTab[Queen] + 16*materialTab[Pawn];

static constexpr int maxScoreChildren = 16;

static const __m128i zero = {0};

#endif /* CONSTANTS_H_ */
