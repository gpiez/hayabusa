/*
 * bits.h
 *
 *  Created on: Feb 1, 2012
 *      Author: gpiez
 */

#ifndef BITS_H_
#define BITS_H_

#include <cstdint>

namespace {
/*
 * All popcount functions return int, because they are usually used in summing
 * up a score or multiplication with possibly negative values.
 */
inline int __attribute__((__always_inline__)) popcount(uint64_t x) {
#if defined(__SSE4_2__) && defined(__x86_64__)
    return __popcntq(x);
#elif defined(__arm__)
    uint32_t x0 = x;
    uint32_t x1 = x >> 32;

    x0 -=   x0>>1        & 0x55555555;
    x0  = ( x0>>2        & 0x33333333 )
        + ( x0           & 0x33333333 );
    x0  = (( x0>>4 )+x0) & 0x0f0f0f0f;
//    x0 *=                  0x01010101;

    x1 -=   x1>>1        & 0x55555555;
    x1  = ( x1>>2        & 0x33333333 )
    	+ ( x1           & 0x33333333 );
    x1  = (( x1>>4 )+x1) & 0x0f0f0f0f;
    x1 += x0;
    x1 *=                  0x01010101;
    return  x1 >> 24;
#else
    x -=  x>>1 & 0x5555555555555555;
    x  = ( x>>2 & 0x3333333333333333 ) + ( x & 0x3333333333333333 );
    x  = (( x>>4 )+x) & 0x0f0f0f0f0f0f0f0f;
    x *= 0x0101010101010101;
    return  x>>56;
#endif
}

// popcount, which counts at most 15 ones, for counting pawns
inline int popcount15( uint64_t x ) {
#if defined(__SSE4_2__) && defined(__x86_64__)
    return __popcntq(x);
//#elif defined(__arm__)
//    return __builtin_popcountll(x);
#else
    x -=  x>>1 & 0x5555555555555555LL;
    x  = ( x>>2 & 0x3333333333333333LL ) + ( x & 0x3333333333333333LL );
    x *= 0x1111111111111111LL;
    return  x>>60;
#endif
}

// popcount, which counts at most 15 ones, for counting pieces
// this always returns values < 4, so for non relevant cases like 4 minor pieces
// of the same kind this returns a wrong value.
inline int popcount3( uint64_t x ) {
#if defined(__SSE4_2__) && defined(__x86_64__)
    return __popcntq(x);
//#elif defined(__arm__)
//    return __builtin_popcountll(x);
#else
    x -=  x>>1 & 0x5555555555555555LL;
    x *= 0x5555555555555555LL;
    return  x>>62;
#endif
}


inline __v2di __attribute__((__always_inline__)) pcmpeqq(__v2di a, __v2di b) {
#ifdef __SSE4_1__
    return _mm_cmpeq_epi64(a, b);
#elif defined(__SSE2__)
    /*
     * emulate a pcmpeqq instruction by comparing 32 bit wise, then swapping the
     * 32 bit halfes of each 64 bit operand. Only if both 32 bit halfes happen to
     * compare equal, the and operation will set all bits in one 64 bit half.
     */
    a = _mm_cmpeq_epi32(a, b);
    b = _mm_shuffle_epi32(a, 0b10110001);
    return a & b;
#else
    union {
    	__v2di in;
    	uint64_t out[2];
    } aa, bb;
    aa.in = a;
    bb.in = b;

    return _mm_set_epi64x(-(int64_t)(aa.out[1]==bb.out[1]),-(int64_t)(aa.out[0]==bb.out[0]));
#endif
}

inline uint64_t  __attribute__((__always_inline__)) ror(uint64_t x, unsigned k) {
#if defined(__x86_64__)
    return __rorq(x, k);
#else
    ASSERT(k<0x40);
    return x >> k | x << (64-k);
#endif
}

inline uint64_t __attribute__((__always_inline__)) rol(uint64_t x, unsigned k) {
#if defined(__x86_64__)
    return __rolq(x, k);
#else
    ASSERT(k<0x40);
    return x << k | x >> (64-k);
#endif
}

inline uint64_t __attribute__((__always_inline__)) bit(uint64_t x) {
#if defined(__x86_64__)
    uint64_t result;
    asm(" bsfq %1, %0 \n"
        : "=r"(result)
        : "r"(x)
       );
    return result;
#else
    return __builtin_ctzll(x);
#endif
//     return __bsfq(x);        //this causes an additional 32 to 64 bit zero extension being emitted
}

inline uint64_t __attribute__((__always_inline__)) bitr(uint64_t x) {
#if defined(__x86_64__)
    uint64_t result;
    asm(" bsrq %1, %0 \n"
        : "=r"(result)
        : "r"(x)
       );
    return result;
//     return __bsrq(x);
#else
    return 63-__builtin_clzll(x);
#endif
}

inline uint64_t __attribute__((__always_inline__)) fold(__v2di hilo) {
#ifdef __SSE__
#ifdef __x86_64__
    return _mm_cvtsi128_si64(_mm_unpackhi_epi64(hilo, hilo))
           | _mm_cvtsi128_si64(hilo);
#else
    __v2di folded = _mm_unpackhi_epi64(hilo, hilo) | hilo;
    return *(uint64_t*) &folded;
#endif
#else
    union {
    	__v2di v;
    	uint64_t p[2];
    } converter;
    converter.v = hilo;
    return converter.p[0] | converter.p[1];
#endif
}
}

template<int N> inline uint64_t __attribute__((__always_inline__)) shift(uint64_t b) {
    if (N > 0)
        return b << (N & 0x3f);
    else
        return b >> (-N & 0x3f); }

template<Colors C,int R> inline uint64_t __attribute__((__always_inline__)) rank() {
    static_assert( R>=1 && R<=8, "Wrong Rank" );
    return 0xffULL << (C == White ? R*8-8:64-8*R); }

template<char F> inline uint64_t __attribute__((__always_inline__)) file() {
    static_assert( F>='a' && F<='h', "Wrong File" );
    return 0x0101010101010101ULL << (F-'a'); }

#endif /* BITS_H_ */
