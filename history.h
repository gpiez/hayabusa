/*
 * history.h
 *
 *  Created on: Jul 22, 2010
 *      Author: gpiez
 */

#ifndef HISTORY_H_
#define HISTORY_H_

#include "constants.h"
#include "move.h"
#include "score.h"

#define BESTKILLER 64

struct History {
    uint8_t v[nMaxGameLength+4][8][nSquares] ALIGN_XMM;
    int max[nMaxGameLength+4];
    unsigned size;
    static const __v16qi uinctab[16];
    static const __v16qi sinctab[16];

public:
//     History();
    void init();
    void init64(uint8_t v[nSquares]);
    void clear(unsigned start, unsigned end);
    void resize(unsigned ply);
    void good(Move, unsigned ply);
    int get(Move, unsigned ply);
    void sort(Move* begin, unsigned n, unsigned ply); };

#endif /* HISTORY_H_ */
