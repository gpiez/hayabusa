/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

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

#ifndef PERFT_H
#define PERFT_H

#ifndef PCH_H_
#include <pch.h>
#endif

#include "coloredboard.h"

template<Colors C, unsigned int Depth>
struct Perft
{
	static uint64_t perft(const ColoredBoard<(Colors)-C>* prev, const Move m) {
		const ColoredBoard<C> b(prev, m);
		Move list[256];

	//	prev->doMove(&b, m);

		Move* end = b.generateMoves(list);

		uint64_t n=0;
		for (Move* i = list; i<end; ++i) {
			n += Perft<(Colors)-C, Depth-1>::perft(&b, *i);
		}
		return n;
	}
	static uint64_t perft(const ColoredBoard<(Colors)-C>* prev, const Move m, unsigned int depth) {
		if (depth == Depth)
			return perft(prev, m);
		else
			return Perft<C, Depth-1>::perft(prev, m, depth);
	}
};

template<Colors C>
struct Perft<C,1>
{
	static uint64_t perft(const ColoredBoard<(Colors)-C>* prev, const Move m, unsigned int=0) {
		const ColoredBoard<C> b(prev, m);
		Move list[256];
		Move* end = b.generateMoves(list);
		return end-list;
	}
};

#endif // PERFT_H
