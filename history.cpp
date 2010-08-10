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
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 }
};

const __v16qi History::sinctab[16] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0 }
};

History::History()
{
    init();
}

void History::init() {
    for (unsigned d=0; d<maxDepth; ++d) {
        sum[d] = 1;
        for (unsigned p=0; p<2*nPieces+1; ++p) if (p) 
                for (unsigned sq=0; sq<nSquares; ++sq) 
                    v[d][p][sq] = 0;
    }
}

template<Colors C>
void History::good(Move m, unsigned ply) {
/*
    int& i = hashIndex(m);
    if (i == 0) return;
    memmove(k, k+1, i);
    i = 0;
*/    
    uint16_t& h = v[ply][C*(m.piece() & 7) + nPieces][m.to()];
    
    if (h == sum[ply]) return;
    
    if (maxHistory > ++sum[ply]) {
        h = sum[ply];
        return;
    }
    
    h = maxHistory/2;
    sum[ply] = maxHistory/2;
    for (unsigned p=1; p<nPieces; ++p) 
        for (unsigned sq=0; sq<nSquares; ++sq) 
            v[ply][C*p + nPieces][sq] = std::max(v[ply][C*p + nPieces][sq] - maxHistory/2, 0);
}

template<Colors C>
int History::get(Move m, unsigned ply) {
    int value = v[ply][C*(m.piece() & 7) + nPieces][m.to()];

    if (!value && ply>2) {
        value = v[ply-2][C*(m.piece() & 7) + nPieces][m.to()] - sum[ply-2];
    }

    if (!value && ply<maxDepth-2) {
        value = v[ply+2][C*(m.piece() & 7) + nPieces][m.to()] - sum[ply+2];
    }

/*
    if (!value && ply>4) {
        value = v[ply-4][C*(m.piece() & 7) + nPieces][m.to()] - sum[ply-4];
    }

    if (!value && ply<maxDepth-4) {
        value = v[ply+4][C*(m.piece() & 7) + nPieces][m.to()] - sum[ply+4];
    }
*/

    return value;
}

template<Colors C>
void History::sort(Move* list, unsigned n, unsigned ply) {
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
        int score = get<C>(list[i], ply);
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
                       
                       
