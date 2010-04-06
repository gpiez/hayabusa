/*
 * offsets.cpp
 *
 *  Created on: 30.10.2009
 *      Author: gpiez
 */

#include <pgn.h>

#include "boardbase.h"

#define use(x) out << #x << " = $" << hex << offsetof(BoardBase, x) << '\n'
int ld(unsigned int x) {
	return lrint(log(x)/log(2.0));
}

int main(int argc, char** argv) {
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
