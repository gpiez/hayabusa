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
	uint8_t capture;
	uint8_t special;
	QString string() const;
};

#endif /* MOVE_H_ */
