/*
 * keyscore.h
 *
 *  Created on: Jan 31, 2012
 *      Author: gpiez
 */

#ifndef KEYSCORE_H_
#define KEYSCORE_H_

#include "constants.h"
#include "packedscore.h"
#include "x86intrin.h"

union KeyScore {
    __v8hi vector;
    struct {
        PackedScore<>  score;
        PawnKey        pawnKey;
        Key            key; }; };

#endif /* KEYSCORE_H_ */
