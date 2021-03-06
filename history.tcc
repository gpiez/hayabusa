/*
 * history.tcc
 *
 *  Created on: Aug 20, 2010
 *      Author: gpiez
 */
#include "history.h"
#include "coloredboard.h"

void History::good(Move m, unsigned ply) {
    ASSERT((m.piece() & 7) <= 6);
    ASSERT((m.piece() & 7));
    ASSERT((m.from() != m.to()));
    {
    	resize(ply);
        uint8_t& h = v[ply][m.piece() & 7][m.to()];
        int& m  = max[ply];

        if (m != h) {
            if likely(++m < maxHistory) h = m;
            else {
                m = maxHistory/2;
#ifdef __SSE2__
                __v16qi mh2 = _mm_set1_epi8(maxHistory/2);
                for (unsigned p=1; p<=nPieces; ++p)
                    for (unsigned sq=0; sq<nSquares; sq += 16) {
                        __v16qi* v16 = (__v16qi*)&v[ply][p][sq];
                        *v16 = _mm_subs_epu8(*v16, mh2); }
                h = maxHistory/2;
#else
                for (unsigned p=1; p<=nPieces; ++p)
                    for (unsigned sq=0; sq<nSquares; sq++) {
                    	if (v[ply][p][sq] >= maxHistory/2)
                    		v[ply][p][sq] -= maxHistory/2;
                    	else
                    		v[ply][p][sq] = 0;
                    }
#endif
        } } } }

int History::get(Move m, unsigned ply) {
    ASSERT(ply < size);
    int value = v[ply][m.piece() & 7][m.to()];
    ASSERT(value >= 0);
    return value; }

void History::sort(Move* list, unsigned n, unsigned ply) {
    ASSERT(ply < size);
    /*
     * Four groups of 16 counters for the 4 fourbit digits, which are the
     * keys for sorting. Counter group 3 is for the most significant nibble,
     * which is signed, the other counters are unsigned
     */
    union {
        uint8_t parts[16];
        __v16qi whole; } count0, count1;

    __v16qi c0 = { 0 };
    __v16qi c1 = { 0 };

    struct MoveScore {
        Move m;
        uint8_t score[4]; };

    MoveScore mList0[maxMoves], mList1[maxMoves];

    unsigned i;
    for (i = 0; i<n; ++i) {
        mList0[i].m = list[i];
        int score = get(list[i], ply);
        unsigned nibble0 = score & 0xf;
        unsigned nibble1 = score >> 4 & 0xf;

        mList0[i].score[0] = nibble0;
        mList0[i].score[1] = nibble1;

        c0 += uinctab[nibble0];
        c1 += uinctab[nibble1];
    }
    count0.whole = c0;
    count1.whole = c1;

    for (unsigned i=0; i<n; ++i)
        mList1[count0.parts[mList0[i].score[0]]++] = mList0[i];

    for (unsigned i=0; i<n; ++i)
        list[count1.parts[mList1[i].score[1]]++] = mList1[i].m;

}
