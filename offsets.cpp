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

int main(int, char** argv) {
	QTextStream out(stdout);
	//out << "pieceList = " << ld(sizeof(shortAttacks[0])) << '\n';

	if (argv[1] && QString("knightbits") == argv[1]) {
		for (int y=0; y<8; ++y)for (int x=0; x<8; ++x)
			 {
				uint64_t bits=0;
				for (int u=-2; u<=2; ++u)
					for (int v=-2; v<=2; ++v)
						if (abs(u*v)==2)
							if (x+u >= 0 && x+u<8 && y+v>=0 && y+v<8)
								bits |= 1ULL << (x+u+(y+v)*8);
				out << showbase << hex << bits << ",";
				if (x == 7)
					out << endl;
				else
					out << " ";
			}
	} else if (argv[1] && QString("pawnbits") == argv[1]) {
		for (int v=1; v>=-1; v-=2) {
			out << "{";
			for (int y=0; y<8; ++y)for (int x=0; x<8; ++x)
				 {
					uint64_t bits=0;
					for (int u=-1; u<=1; u+=2)
						if (x+u >= 0 && x+u<8 && y+v>=0 && y+v<8)
							bits |= 1ULL << (x+u+(y+v)*8);
					out << showbase << hex << bits << ",";
					if (x == 7)
						out << endl;
					else
						out << " ";
				}
			out << "}," << endl;
		}
	} else if (argv[1] && QString("mobbits") == argv[1]) {
		for (int d=0; d<4; ++d) {
			const int u = (int[4]) { 1, 1, 0, -1 } [d];
			const int v = (int[4]) { 0, 1, 1, 1 } [d];
			for (int l=0; l<8; ++l)
				for (int r=0; r<8; ++r) {
					uint64_t bits=0;
					if (l+r<=7) {
						for (int ll = 1; ll<=l; ++ll) {
							int bitnr = (-ll*u - 8*ll*v) & 63;
							bits |= 1ULL << bitnr;
						}
						for (int rr = 1; rr<=r; ++rr) {
							int bitnr = (rr*u + 8*rr*v) & 63;
							bits |= 1ULL << bitnr;
						}
					}
					out << showbase << hex << bits << ",";
					if (r == 7)
						out << endl;
					else
						out << " ";
				}
			out << "}," << endl;
		}
	} else {
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
	}

	return 0;
}
