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

#include <x86intrin.h>

//#define static_assert(x) char __y[(x) ? 1 : -1 ] __attribute__((unused));

#ifndef NDEBUG
#define ASSERT(x) do { if (!(x)) { \
				qDebug() << endl << "Assertion " << #x << " failed." << endl\
				<<  __FILE__ << __PRETTY_FUNCTION__ << __LINE__ << endl; \
				asm("int3"); \
} } while(0)
#else
#define ASSERT(x)
#endif

#define CACHE_LINE_SIZE 64
#define ALIGN_XMM __attribute__((aligned(16)))
#define ALIGN_CACHE __attribute__((aligned(CACHE_LINE_SIZE)))
#define ALIGN_PAGE __attribute__((aligned(4096)))

enum SearchFlag { null = 1 };

enum Pieces: unsigned int { Rook = 1, Bishop = 2, Queen = 3, Knight = 4, Pawn = 5, King = 6, All = 7 };

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

#ifndef BITBOARD
enum SpecialMoves: uint8_t {
	nothingSpecial=0,
	shortCastling, longCastling,
	promoteQ, promoteR, promoteB, promoteN,
	enableEP, EP
};
#endif

enum Phase { root, trunk, tree, mate, leaf, vein };

enum Sides { KSide, QSide, Middle };

static const int16_t maxHistory = 10000;
static const int nullReduction = 3;
static const int initialError = 100;
static const unsigned int nHashPassers = 2;
static const unsigned int nTTLocks = 1;
static const unsigned int maxMoves = 256; // maximum possible moves in a position
static const unsigned int goodMoves = 192; // maximum possible good moves in a position
static const unsigned int nMaxGameLength = 5000; // maximum possible moves in a position
static const int infinity = 10000;
static const unsigned int maxDepth = 64;
static const unsigned int transpositionTableAssoc = 2;
static const unsigned int pawnTableAssoc = 4;
static const unsigned int nPieces = 6;
static const unsigned int nColors = 2;
static const unsigned int nTotalPieces = nPieces*nColors + 1;	// -King,... -Pawn, 0, Pawn,... King

static const unsigned int nDirs = 8;	//long range directions
static const unsigned int nDirKinds = 2;//kinds of long range direction, horizontal/vertical vs diagonal
static const unsigned int nFiles = 8;
static const unsigned int nRows = 8;
static const unsigned int nSquares = nFiles*nRows;

#ifndef BITBOARD
/*
 * Number of bits reserved for the attackedBy counters.
 * The standard structure can handle:
 * Up to 3 bishop of the same color attacking the same square
 * Up to 3 queens attacking the same square
 * up to 3 rooks attacking the same square
 */
static const unsigned int nRBits = 2;
static const unsigned int nBBits = 2;
static const unsigned int nQBits = 2;
static const unsigned int nCheckKRBits = 1;	// horizontal king attacks from enemy pieces
static const unsigned int nCheckKBBits = 1;	// diagonal king attacks from enemy pieces

static const unsigned int nPRBits = 1;
static const unsigned int nPLBits = 1;
static const unsigned int nKBits = 1;
static const unsigned int nNBits = 3;
static const unsigned int nKNAttackBits = 1;	// king attacks from enemy knight
static const unsigned int nKPAttackBits = 1;	// king attacks from enemy pawn
#endif

/*
 * The eight possible directions of movement for a
 * king in a [8][8] array
 */
static const int dirOffsets[8] = { 1, 9, 8, 7, -1, -9, -8, -7 };
static const int xOffsets[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static const int yOffsets[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

#ifndef BITBOARD
static const int xShortOffsets[nPieces+1][nDirs] = {
	{},
	{},	//rook
	{},	//bishop
	{},	//queen
	{ 2, 1, -1, -2, -2, -1, 1, 2 },	//knight
	{ 1, -1 },						//pawn
	{ 1, 1, 0, -1, -1, -1, 0, 1 }	//king
};

static const int yShortOffsets[nColors][nPieces+1][nDirs] = {{
	{},
	{},	//rook
	{},	//bishop
	{},	//queen
	{ 1, 2, 2, 1, -1, -2, -2, -1 },	//knight
	{ 1, 1 },						//pawn white
	{ 0, 1, 1, 1, 0, -1, -1, -1 }	//king
},{
	{},
	{},	//rook
	{},	//bishop
	{},	//queen
	{ 1, 2, 2, 1, -1, -2, -2, -1 },	//knight
	{ -1, -1 },						//pawn black
	{ 0, 1, 1, 1, 0, -1, -1, -1 }	//king
}};

static const __v16qi zeroToFifteen = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
#endif

static inline uint64_t popcount(uint64_t x) {
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

static inline unsigned int bit(uint64_t x) {
    return __bsfq(x);
}

static inline unsigned int bit(unsigned int x) {
    return __bsfd(x);
}

static inline uint64_t fold(__v2di hilo) {
    return _mm_cvtsi128_si64(_mm_unpackhi_epi64(hilo, hilo))
        | _mm_cvtsi128_si64(hilo);
}

//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wall"
template<int N>
uint64_t shift(uint64_t b) {
    if (N > 0)
        return b << N;
    else
        return b >> -N;
}
//#pragma GCC diagnostic pop

template<Colors C,int R>
uint64_t rank() {
    static_assert( R>=1 && R<=8, "Wrong Rank" );
    return 0xffULL << (C == White ? R*8-8:64-8*R);
}

template<char F>
uint64_t file() {
    static_assert( F>='a' && F<='h', "Wrong File" );
    return 0x0101010101010101ULL << (F-'a');
}

//extern "C" __m128i broadcastTab[256] ALIGN_XMM;
#endif /* CONSTANTS_H_ */
