/*
 * move.h
 *
 *  Created on: Dec 14, 2009
 *      Author: gpiez
 */

#ifndef MOVE_H_
#define MOVE_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include <stdint.h>

struct Move {
	uint8_t from;
	uint8_t to;
	int8_t capture;			// TODO currently only a bit would suffice, otherwise this should logically be uint, for use in set/clrPiece, which also accept a uint
	uint8_t special;
	QString string() const;
};

#endif /* MOVE_H_ */
