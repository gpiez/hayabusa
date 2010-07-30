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
    uint16_t v[nTotalPieces+1][nSquares];
    int sum;
    static const __v16qi uinctab[16];
    static const __v16qi sinctab[16];
    
public:
    History();
    template<Colors C> void good(Move);
    template<Colors C> void bad(Move);
    template<Colors C> int get(Move);
    template<Colors C> void sort(Move* begin, unsigned n);
};

#endif /* HISTORY_H_ */
