/*
 * history.cpp
 *
 *  Created on: Jul 22, 2010
 *      Author: gpiez
 */

#ifndef PCH_H_
#include <pch.h>
#endif

#include "history.h"

const __v16qi History::uinctab[16] = {
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } 
};

const __v16qi History::sinctab[16] = {
    { 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 }
};

History::History():
    sum(1)
{
    for (int p=-nPieces; p<nPieces; ++p) if (p) {
        for (unsigned sq=0; sq<nSquares; ++sq) {
            v[p + nPieces][sq] = 0;
        }
    }
}

template<Colors C>
void History::good(Move m) {
    sum++;
    if (++v[C*(m.piece() & 7) + nPieces][m.to()] > maxHistory) {
        sum >>= 1;
        for (int p=1; p<nPieces; ++p) 
            for (unsigned sq=0; sq<nSquares; ++sq) 
                v[C*p + nPieces][sq] >>= 1;
            
    }
}

template<Colors C>
void History::bad(Move m) {
    sum++;
    if (--v[C*m.piece() + nPieces][m.to()] < -maxHistory) {
        sum >>= 1;
        for (int p=1; p<nPieces; ++p) 
            for (unsigned sq=0; sq<nSquares; ++sq) 
                v[C*p + nPieces][sq] >>= 1;
            
    }
    
}

template<Colors C>
int History::get(Move m) {
    return v[C*(m.piece() & 7) + nPieces][m.to()];
}

template<Colors C>
void History::sort(Move* list, unsigned n) {
    /*
     * Four groups of 16 counters for the 4 fourbit digits, which are the
     * keys for sorting. Counter group 3 is for the most significant nibble,
     * which is signed, the other counters are unsigned
     */
    union {
        uint8_t parts[16];
        __v16qi whole;
    } count0, count1, count2, count3;
    count0.whole = _mm_set1_epi8(0);
    count1.whole = _mm_set1_epi8(0);
    count2.whole = _mm_set1_epi8(0);
    count3.whole = _mm_set1_epi8(0);

    struct MoveScore {
        Move m;
        uint8_t score[4];
    };
    
    MoveScore mList0[maxMoves], mList1[maxMoves];
    
    for (unsigned i = 0; i<n; ++i) {
        mList0[i].m = list[i];
        int score = get<C>(list[i]);
        unsigned nibble0 = score & 0xf;
        unsigned nibble1 = score >> 4 & 0xf;
        unsigned nibble2 = score >> 8 & 0xf;
        unsigned nibble3 = score >> 12 & 0xf;
        mList0[i].score[0] = nibble0;
        mList0[i].score[1] = nibble1;
        mList0[i].score[2] = nibble2;
        mList0[i].score[3] = nibble3;
        count0.whole += uinctab[nibble0];
        count1.whole += uinctab[nibble1];
        count2.whole += uinctab[nibble2];
        count3.whole += sinctab[nibble3];
    }
    /*
     * The fences are for avoiding wrong hardware prefetching.
     * If the counters are stored in different places or no fences
     * are used, the loops are executed much slower.
     */
//        _mm_lfence();
    for (unsigned i=0; i<n; ++i)
       mList1[count0.parts[mList0[i].score[0]]++] = mList0[i];

    for (unsigned i=0; i<n; ++i)
       mList0[count1.parts[mList1[i].score[1]]++] = mList1[i];

    for (unsigned i=0; i<n; ++i)
       mList1[count2.parts[mList0[i].score[2]]++] = mList0[i];

    for (unsigned i=0; i<n; ++i)
       list[count3.parts[mList1[i].score[3]]++] = mList1[i].m;
}
                       
                       
