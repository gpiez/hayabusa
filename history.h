/*
 * history.h
 *
 *  Created on: Jul 22, 2010
 *      Author: gpiez
 */

#ifndef HISTORY_H_
#define HISTORY_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"
#include "move.h"

class History {
    uint8_t v[nMaxGameLength+4][16][nSquares] ALIGN_XMM;
    uint8_t v2[nMaxGameLength+4][16][nSquares] ALIGN_XMM;
    int max[nColors][nMaxGameLength+4];
    int max2[nColors][nMaxGameLength+4];
    static const __v16qi uinctab[16];
    static const __v16qi sinctab[16];

public:
    History();
    void init();
    template<Colors C> void good(Move, unsigned ply);
    template<Colors C> int get(Move, unsigned ply);
    template<Colors C> void sort(Move* begin, unsigned n, unsigned ply);
};

#endif /* HISTORY_H_ */
