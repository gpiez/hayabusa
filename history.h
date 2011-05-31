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
#include "score.h"

#define BESTKILLER 32

struct PositionalError {
    RawScore v;
#ifdef CALCULATE_MEAN_POSITIONAL_ERROR
    float n;
    float e;
    float e2;
#endif
};

class History {
    uint8_t v[nMaxGameLength+4][16][nSquares] ALIGN_XMM;
#ifndef USE_DIFF_FOR_SORT    
    uint8_t v2[nMaxGameLength+4][16][nSquares] ALIGN_XMM;
#endif    
    int max[nColors][nMaxGameLength+4];
    int max2[nColors][nMaxGameLength+4];
    static const __v16qi uinctab[16];
    static const __v16qi sinctab[16];

public:
    History();
    void init();
    template<Colors C> void good(Move, unsigned ply);
    template<Colors C> int get(Move, unsigned ply);
    template<Colors C> void sort(Move* begin, unsigned n, unsigned ply
#ifdef USE_DIFF_FOR_SORT
            , const PositionalError (&pe)[nPieces*2+1][nSquares][nSquares]
#endif            
    );
};

#endif /* HISTORY_H_ */
