/*
 * history.tcc
 *
 *  Created on: Aug 20, 2010
 *      Author: gpiez
 */
#ifndef PCH_H_
#include <pch.h>
#endif

#include "history.h"

template<Colors C>
void History::good(Move m, unsigned ply) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    ASSERT((m.piece() & 7) <= 6);    
    ASSERT((m.piece() & 7));
    ASSERT((m.from() != m.to()));
    {
        uint8_t& h = v[ply+2][C*(m.piece() & 7) + nPieces][m.to()];
        int& m  = max[CI][ply+2];

        if (m != h) {

            if (++m < maxHistory) h = m;

            else {
                m = maxHistory/2;
                __v16qi mh2 = _mm_set1_epi8(maxHistory/2);
                for (unsigned p=1; p<=nPieces; ++p)
                    for (unsigned sq=0; sq<nSquares; sq += 16) {
                        __v16qi* v16 = (__v16qi*)&v[ply+2][C*p + nPieces][sq];
                        *v16 = _mm_subs_epu8(*v16, mh2);
                }
                h = maxHistory/2;
            }
        }
    }
    {
        uint8_t& h  = v2[ply+2][C*(m.piece() & 7) + nPieces][m.from()];
        int& m  = max2[CI][ply+2];
        /*
         * Only if the last move target changed, the history table will be
         * updated. As long as the current max stays below the allowed absolute
         * max, just update the entry for the move, otherwise subtract the half
         * of the allowed max from all entries, saturating to zero.
         */
        if (m != h) {

            if (++m < maxHistory) h = m;

            else {
                m = maxHistory/2;
                __v16qi mh2 = _mm_set1_epi8(maxHistory/2);
                for (unsigned p=1; p<=nPieces; ++p)
                    for (unsigned sq=0; sq<nSquares; sq += 16) {
                        __v16qi* v16 = (__v16qi*)&v2[ply+2][C*p + nPieces][sq];
                        *v16 = _mm_subs_epu8(*v16, mh2);
                    }
                h = maxHistory/2;
            }
        }
    }
}

template<Colors C>
int History::get(Move m, unsigned ply) {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    int value = v[ply+2][C*(m.piece() & 7) + nPieces][m.to()];
    if (!value) value = v2[ply+2][C*(m.piece() & 7) + nPieces][m.from()] - max2[CI][ply+2];

    if (!value) value = v[ply][C*(m.piece() & 7) + nPieces][m.to()] - max[CI][ply];

//    if (!value) value = (m.to() ^ (C == Black ? 070:0)) - 077;

    return std::max(value - max[CI][ply+2] + (signed)maxHistory - 1, 0);
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
    } count0, count1/*, count2, count3*/;

    __v16qi c0, c1/*, c2, c3*/;
    c0 = c1 = /*c2 = c3 = */_mm_set1_epi8(0);

    struct MoveScore {
        Move m;
        uint8_t score[4];
    };

    MoveScore mList0[maxMoves], mList1[maxMoves];

    unsigned i;
    for (i = 0; i<n; ++i) {
        mList0[i].m = list[i];
        int score = get<C>(list[i], ply);
        unsigned nibble0 = score & 0xf;
        unsigned nibble1 = score >> 4 & 0xf;
//         unsigned nibble2 = score >> 8 & 0xf;
//         unsigned nibble3 = score >> 12 & 0xf;
        mList0[i].score[0] = nibble0;
        mList0[i].score[1] = nibble1;
//         mList0[i].score[2] = nibble2;
//         mList0[i].score[3] = nibble3;
        c0 += uinctab[nibble0];
        c1 += uinctab[nibble1];
//         c2 += uinctab[nibble2];
//         c3 += sinctab[nibble3];
    }
    count0.whole = c0;
    count1.whole = c1;
//     count2.whole = c2;
//     count3.whole = c3;

    for (unsigned i=0; i<n; ++i)
       mList1[count0.parts[mList0[i].score[0]]++] = mList0[i];

    for (unsigned i=0; i<n; ++i)
       list[count1.parts[mList1[i].score[1]]++] = mList1[i].m;

/*    for (unsigned i=0; i<n; ++i)
       mList0[count1.parts[mList1[i].score[1]]++] = mList1[i];

    for (unsigned i=0; i<n; ++i)
       mList1[count2.parts[mList0[i].score[2]]++] = mList0[i];

    for (unsigned i=0; i<n; ++i)
       list[count3.parts[mList1[i].score[3]]++] = mList1[i].m;*/
}
