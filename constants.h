/*
 * constants.h
 *
 *  Created on: 09.11.2009
 *      Author: gpiez
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include <emmintrin.h>	//SSE2 for m128i
#include <stdint.h>

#define static_assert(x) char __y[(x) ? 1 : -1 ] __attribute__((unused));
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

enum Pieces { Rook = 1, Bishop = 2, Queen = 3, Knight = 4, Pawn = 5, King = 6 };

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

enum SpecialMoves {
	nothingSpecial=0,
	disableCastling, disableShortCastling, disableLongCastling,
	shortCastling, longCastling,
	promoteQ, promoteR, promoteB, promoteN,
	enableEP, EP,
	disableOpponentShortCastling = 16,
	disableOpponentLongCastling = 32
};

static const unsigned int nMaxMoves = 256; // maximum possible moves in a position
static const unsigned int nMaxGameLength = 5000; // maximum possible moves in a position
static const int infinity = 10000;
static const unsigned int maxDepth = 64;
static const unsigned int splitDepth = 3;
static const unsigned int nPieces = 6;
static const unsigned int nColors = 2;
static const unsigned int nTotalPieces = nPieces*nColors + 1;	// -King,... -Pawn, 0, Pawn,... King

static const unsigned int nDirs = 8;	//long range directions
static const unsigned int nDirKinds = 2;//kinds of long range direction, horizontal/vertical vs diagonal
static const unsigned int nFiles = 8;
static const unsigned int nRows = 8;
static const unsigned int nSquares = nFiles*nRows;

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

/*
 * The eight possible directions of movement for a
 * king in a [8][8] array
 */
static const int dirOffsets[8] = { 1, 9, 8, 7, -1, -9, -8, -7 };
static const int xOffsets[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static const int yOffsets[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

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

static inline uint64_t popcount(uint64_t x) {
    x -=  x>>1 & 0x5555555555555555;
    x  = ( x>>2 & 0x3333333333333333 ) + ( x & 0x3333333333333333 );
    x  = (( x>>4 )+x) & 0x0f0f0f0f0f0f0f0f;
    x *= 0x0101010101010101;
    return  x>>56;
}

extern "C" __m128i broadcastTab[256] ALIGN_XMM;
#endif /* CONSTANTS_H_ */
