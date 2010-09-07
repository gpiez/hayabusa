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

void printTemplate(int WR, int WB, int WQ, int WN, int WP,
                   int BR, int BB, int BQ, int BN, int BP) {
    if (abs(WR*5+WB*3+WQ*9+WN*3+WP - BR*5-BB*3-BQ*9-BN*3-BP) > 14) {
        std::cout << "template<int WP, int BP> struct Search<" <<
        WR << "," << WB << "," << WQ << "," << WN << "," << "WP" << "," <<
        BR << "," << BB << "," << BQ << "," << BN << "," << "BP" <<
        ">: SearchBase {" << std::endl;
        std::cout << "template<Colors C, Phase P, typename A, typename B>" << std::endl;
        std::cout << "static bool search(TranspositionTable<TTEntry, transpositionTableAssoc, Key>* tt, const Eval& e, const ColoredBoard<(Colors)-C>& b, Move m, unsigned int d, const A& a, B& beta) {" << std::endl;
        std::cout << "    return defaultSearch<C,P>(tt, e, b, m, d, a, beta); }};" << std::endl;
    }
}

int main(int, char** argv) {
	//std::cout << "pieceList = " << ld(sizeof(shortAttacks[0])) << '\n';

    if (argv[1] && std::string("templates") == argv[1]) {
        for (int WR = 0; WR <= 2; ++WR)
        for (int WB = 0; WB <= 2; ++WB)
        for (int WQ = 0; WQ <= 1; ++WQ)
        for (int WN = 0; WN <= 2; ++WN)
        for (int WP = 8; WP <= 8; ++WP)
        for (int BR = 0; BR <= 2; ++BR)
        for (int BB = 0; BB <= 2; ++BB)
        for (int BQ = 0; BQ <= 1; ++BQ)
        for (int BN = 0; BN <= 2; ++BN)
        for (int BP = 8; BP <= 8; ++BP)
        printTemplate(WR,WB,WQ,WN,WP,BR,BB,BQ,BN,BP);
    } else if (argv[1] && std::string("knightbits") == argv[1]) {
		for (int y=0; y<8; ++y)for (int x=0; x<8; ++x)
			 {
				uint64_t bits=0;
				for (int u=-2; u<=2; ++u)
					for (int v=-2; v<=2; ++v)
						if (abs(u*v)==2)
							if (x+u >= 0 && x+u<8 && y+v>=0 && y+v<8)
								bits |= 1ULL << (x+u+(y+v)*8);
				std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << bits << ",";
				if (x == 7)
					std::cout << std::endl;
				else
					std::cout << " ";
			}
    } else if (argv[1] && std::string("psqr") == argv[1]) {
        int xopen[8] = { 0, 0, 6, 8, 8, 6, 0, 0 };
        int xend[8] = { 0, 10, 11, 12, 12, 11, 10, 0};
        int yopen[8] = { 0, 0, 1, 2, 3, 5, 12, 4 };
        int yend[8] = { 0, 10, 11, 12, 12, 11, 10, 0};

        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xopen[x] + yopen[7-y];
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}," << std::endl;
        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xend[x] + yend[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}" << std::endl;
    } else if (argv[1] && std::string("psqb") == argv[1]) {
        int xopen[8] = { 0, 4, 8, 7, 7, 8, 4, 0 };
        int xend[8] = { 0, 3, 6, 9, 9, 6, 3, 0};
        int yopen[8] = { 0, 4, 8, 8, 8, 8, 4, 0 };
        int yend[8] = { 0, 3, 6, 9, 9, 6, 3, 0};

        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xopen[x] + yopen[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}," << std::endl;
        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xend[x] + yend[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}" << std::endl;
    } else if (argv[1] && std::string("psqn") == argv[1]) {
        int xopen[8] = { 0, 8, 10, 12, 12, 10, 8, 0 };
        int xend[8] = { 0, 6, 10, 12, 12, 10, 6, 0};
        int yopen[8] = { 0, 8, 10, 12, 13, 12, 10, 0 };
        int yend[8] = { 0, 6, 10, 12, 12, 10, 6, 0};

        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xopen[x] + yopen[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}," << std::endl;
        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xend[x] + yend[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}" << std::endl;
    } else if (argv[1] && std::string("psqq") == argv[1]) {
        int xopen[8] = { 0, 2, 7, 7, 7, 7, 2, 0 };
        int xend[8] = { 0, 6, 8, 10, 10, 8, 6, 0};
        int yopen[8] = { 0, 3, 5, 7, 9, 10, 10, 4 };
        int yend[8] = { 0, 6, 8, 10, 10, 8, 6, 0};

        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xopen[x] + yopen[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}," << std::endl;
        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xend[x] + yend[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}" << std::endl;
    } else if (argv[1] && std::string("psqp") == argv[1]) {
        int xopen[8] = { 0, 12, 16, 20, 20, 16, 12, 0 };
        int xend[8] =  { 0, 25, 25, 25, 25, 25, 25, 0 };
        int yopen[8] = { 0, 0, 1, 3,  6, 10, 15, 0 };
        int yend[8] =  { 0, 0, 2, 6, 12, 20, 30, 0 };

        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xopen[x] + yopen[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}," << std::endl;
        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xend[x] + yend[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}" << std::endl;
    } else if (argv[1] && std::string("psqk") == argv[1]) {
        int xopen[8] = { 10, 10, 5, 0, 0, 5, 10, 10 };
        int xend[8] =  { 0, 4, 8, 12, 12, 8, 4, 0 };
        int yopen[8] = { 0, 0, -10, -20, -50, -100, -100, -100 };
        int yend[8] =  { 0, 5, 10, 14, 17, 18, 9, 0 };

        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xopen[x] + yopen[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}," << std::endl;
        std::cout << "{" << std::endl;
        for (int y=0; y<8; ++y) {
            for (int x=0; x<8; ++x)  {
                std::cout << std::setw(2) << xend[x] + yend[7-y];
                if (x<7) std::cout << ", ";
            }
            if (y<7) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "}" << std::endl;
	} else if (argv[1] && std::string("pawnbits") == argv[1]) {
		for (int v=1; v>=-1; v-=2) {
			std::cout << "{";
			for (int y=0; y<8; ++y)for (int x=0; x<8; ++x)
				 {
					uint64_t bits=0;
					for (int u=-1; u<=1; u+=2)
						if (x+u >= 0 && x+u<8 && y+v>=0 && y+v<8)
							bits |= 1ULL << (x+u+(y+v)*8);
					std::cout << "0x" << std::hex << std::setw(16) << std::setfill('0') << bits << ",";
					if (x == 7)
						std::cout << std::endl;
					else
						std::cout << " ";
				}
			std::cout << "}," << std::endl;
		}
	} else if (argv[1] && std::string("doublebits") == argv[1]) {
		std::cout << "{";
		for (int y=0; y<8; ++y)for (int x=0; x<8; ++x)
			 {
				uint64_t bits= 1ULL << (x+y*8);
				std::cout << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << bits << ", 0x" << std::setw(16) << bits << " }, ";
				if (x == 7)
					std::cout << std::endl;
				else
					std::cout << " ";
			}
		std::cout << "}," << std::endl;
	} else if (argv[1] && std::string("doublereverse") == argv[1]) {
		std::cout << "{";
		for (int y=0; y<8; ++y)for (int x=0; x<8; ++x)
			 {
				uint64_t bits= 1ULL << (x+(7-y)*8);
				std::cout << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << bits << ", 0x" << std::setw(16) << bits << " }, ";
				if (x == 7)
					std::cout << std::endl;
				else
					std::cout << " ";
			}
		std::cout << "}," << std::endl;
	} else if (argv[1] && std::string("masks") == argv[1]) {
		for (int y = 0; y < (signed)nRows; ++y)
		for (int x = 0; x < (signed)nFiles; ++x) {
			uint64_t p=0;
			int dx=1, dy=0;
			for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
				p |= 1ULL << (x0+y0*nRows);
			dx=-1;
			for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
				p |= 1ULL << (x0+y0*nRows);
			if (x==0)
				p |= 1ULL << (x+y*nRows);
			uint64_t dir0x = p;

			p=0; dx=0; dy=1;
			for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
				p |= 1ULL << (x0+y0*nRows);
			dy=-1;
			for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
				p |= 1ULL << (x0+y0*nRows);
			if (y==0)
				p |= 1ULL << (x+y*nRows);
			uint64_t dir2x = p;

			p=0; dx=1; dy=1;
			for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
				p |= 1ULL << (x0+y0*nRows);
			dx=-1; dy=-1;
			for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
				p |= 1ULL << (x0+y0*nRows);

			p=0; dx=1; dy=-1;
			for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
				p |= 1ULL << (x0+y0*nRows);
			dx=-1; dy=1;
			for (int x0=x+dx, y0=y+dy; x0>=0 && x0<=7 && y0>=0 && y0<=7; x0+=dx, y0+=dy)
				p |= 1ULL << (x0+y0*nRows);

			std::cout << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << dir0x << ", 0x" << std::setw(16) << dir2x << " }, ";
			if (x == 7)
				std::cout << std::endl;
			else
				std::cout << " ";

//			mask02x[x+y*nRows] = _mm_set_epi64x(dir2, dir0);
//			mask02b[x+y*nRows] = _mm_set_epi64x(dir2x, dir0x);
//			mask13x[x+y*nRows] = _mm_set_epi64x(dir3, dir1);
		}
	} else if (argv[1] && std::string("mobbits") == argv[1]) {
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
					std::cout  << "0x" << std::hex << std::setw(16) << std::setfill('0') << bits << ",";
					if (r == 7)
						std::cout << std::endl;
					else
						std::cout << " ";
				}
			std::cout << "}," << std::endl;
		}
	} else {
#ifndef BITBOARD
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
#endif
	}

	return 0;
}
