/*
    hayabusa, chess engine
    Copyright (C) 2009-2010 Gunther Piez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef MOVE_H_
#define MOVE_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include <stdint.h>

union Move {
	struct {
	uint8_t from;
	uint8_t to;
	int8_t capture;			// TODO currently only a bit would suffice, otherwise this should logically be uint, for use in set/clrPiece, which also accept a uint
	uint8_t special;
	};
	uint32_t data;
	QString string() const;
};

#endif /* MOVE_H_ */
