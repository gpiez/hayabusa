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

class History {
    int v[maxDepth][nTotalPieces+1][nSquares];
    int sum[maxDepth];
    static const __v16qi uinctab[16];
    static const __v16qi sinctab[16];
    
public:
    History();
    void init();
    template<Colors C> void good(Move, unsigned ply);
    template<Colors C> void bad(Move);
    template<Colors C> int get(Move, unsigned ply);
    template<Colors C> void sort(Move* begin, unsigned n, unsigned ply);
};

#endif /* HISTORY_H_ */
