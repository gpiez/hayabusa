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
#include <pch.h>

#include "boardbase.h"

#define use(x) out << #x << " = $" << hex << offsetof(BoardBase, x) << '\n'
int ld(unsigned int x) {
	return lrint(log(x)/log(2.0));
}

int main(int, char**) {
	QTextStream out(stdout);
	use(longAttack);
	use(shortAttack);
	use(attVec);
	use(attLen);
	use(pieces);
	use(pieceList);

	out << dec;
	out << "maskLen = " << ld (sizeof(masks[0])) << '\n';
	out << "maskDir = " << ld (sizeof(masks[0][0][0])) << '\n';
	out << "maskPos = " << ld (sizeof(masks[0][0])) << '\n';
	out << "maskSlice = " << ld (16) << '\n';

	out << "attDir = " << ld(sizeof(BoardBase::attVec[0])) << '\n';
	out << "attSlice = " << ld(16) << '\n';

	out << "shortColor = " << sizeof(shortAttacks[0][0][0]) << '\n';
	out << "shortPiece = " << ld(sizeof(shortAttacks[0])) << '\n';
	out << "shortPos = " << ld(sizeof(shortAttacks[0][0])) << '\n';

	//out << "pieceList = " << ld(sizeof(shortAttacks[0])) << '\n';
	return 0;
}
