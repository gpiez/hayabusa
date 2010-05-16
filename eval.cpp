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
#include <xmmintrin.h>

#include "eval.h"
#include "boardbase.h"

int squareControl[nSquares];
//void sigmoid(Score* p, unsigned int n, double start, double end, double dcenter, double width) {
//	double t0 = -0.5*n-dcenter;
//	double t1 = 0.5*n-dcenter;
//	double l0 = 1/(1+exp(-t0/width));
//	double l1 = 1/(1+exp(-t1/width));
//
//	double r = (end - start)/(l1-l0);
//	double a = start - l0*r;
//	for (unsigned int i = 0; i < n; ++i) {
//		double t = i - 0.5*n - dcenter;
//		p[i] = lrint(a + r/(1.0 + exp(-t/width)));
//	}
//}

RawScore Eval::pawn, Eval::knight, Eval::bishop, Eval::rook, Eval::queen;
RawScore Eval::bishopPair;
RawScore Eval::knightAlone;
RawScore Eval::bishopAlone;
RawScore Eval::n_p_ram[9], Eval::n_p[17];
RawScore Eval::b_bad_own_p[9], Eval::b_p[17], Eval::b_p_ram[9], Eval::bpair_p[17];
RawScore Eval::r_p[17];
RawScore Eval::q_p[17];

Eval::Eval() {
    pawn = 100;
    knight = 325;			// + 16 Pawns * 3 = 342 == 3 7/16s
    bishop = 325;
    rook = 500;
    queen = 925;

    bishopPair = 50;
    knightAlone = -125;
    bishopAlone = -100;

    sigmoid(n_p_ram, 0, 75, 5); 			// bonus for each pawn ram, max is 75
    sigmoid(n_p, -25, 25, 12);

    sigmoid(b_bad_own_p, 0, -30, 6);
    sigmoid(b_p, 0, -30, 12);
    sigmoid(b_p_ram, 0, -30, 5);
    sigmoid(bpair_p, 15, -15, 12);

    sigmoid(r_p, 50, -50, 12);
    sigmoid(q_p, 50, -50, 12);

    initPS();
	initZobrist();

//	static RawScore bishopGoodD;
//	static RawScore bishopKnightD;
//	static RawScore bishopRookD;
#ifndef NDEBUG
    printSigmoid(n_p_ram, "n_p_ram"); 			// bonus for each pawn ram, max is 75
    printSigmoid(n_p, "n_p");

    printSigmoid(b_bad_own_p, "bad_own_p");
    printSigmoid(b_p, "b_p");
    printSigmoid(b_p_ram, "b_p_ram");
    printSigmoid(bpair_p, "bpair_p");

    printSigmoid(r_p, "r_p");
    printSigmoid(q_p, "r_p");

#endif
}

void Eval::initPS() {
    RawScore rook[nSquares] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        5, 10, 10, 10, 10, 10, 10,  5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        0,  0,  0,  5,  5,  0,  0,  0
    };
    RawScore bishop[nSquares] = {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    };
    RawScore queen[nSquares] = {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
        -5,  0,  5,  5,  5,  5,  0, -5,
        0,  0,  5,  5,  5,  5,  0, -5,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    };
    RawScore knight[nSquares] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };
    RawScore pawn[nSquares] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
        5,  5, 10, 25, 25, 10,  5,  5,
        0,  0,  0, 20, 20,  0,  0,  0,
        5, -5,-10,  0,  0,-10, -5,  5,
        5, 10, 10,-20,-20, 10, 10,  5,
        0,  0,  0,  0,  0,  0,  0,  0
    };
    RawScore king[nSquares] = {
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -10,-20,-20,-20,-20,-20,-20,-10,
        20, 20,  0,  0,  0,  0, 20, 20,
        20, 30, 10,  0,  0, 10, 30, 20
    };

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Pawn, sq) = Eval::pawn + pawn[sq ^ 0x38];
        getPS(-Pawn, sq)  = -Eval::pawn - pawn[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Rook, sq)  = Eval::rook + rook[sq ^ 0x38];
        getPS(-Rook, sq) = -Eval::rook - rook[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Bishop, sq) = Eval::bishop + bishop[sq ^ 0x38];
        getPS(-Bishop, sq) = -Eval::bishop - bishop[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Knight, sq) = Eval::knight + knight[sq ^ 0x38];
        getPS(-Knight, sq) = -Eval::knight - knight[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( Queen, sq) = Eval::queen + queen[sq ^ 0x38];
        getPS(-Queen, sq) = -Eval::queen - queen[sq];
    }

    for (unsigned int sq = 0; sq<nSquares; ++sq) {
        getPS( King, sq) = king[sq ^ 0x38];
        getPS(-King, sq) = - king[sq];
    }

}

void Eval::initZobrist() {
	srand(1);
	for (int p = -nPieces; p <= (signed int)nPieces; p++)
		for (unsigned int i = 0; i < nSquares; ++i)
			if (p) {
				uint64_t r;
				do {
					r = (uint64_t) rand()
					^ (uint64_t) rand() << 8
					^ (uint64_t) rand() << 16
					^ (uint64_t) rand() << 24
					^ (uint64_t) rand() << 32
					^ (uint64_t) rand() << 40
					^ (uint64_t) rand() << 48
					^ (uint64_t) rand() << 56;
				} while (popcount(r) >= 29 && popcount(r) <= 36);
				zobristPieceSquare[p+nPieces][i].key = r;
			} else {
				zobristPieceSquare[p+nPieces][i].key = 0;
			}
}

RawScore Eval::pieces(const PieceList&, int ) const {
// 	if (pl.getCounts() == 0x01000100000000) 	//only one knight left
// 		value += knightAlone;
// 	if (pl.getCounts() == 0x01000000010000) 	//only one bishop left
// 		value += knightAlone;
// 	if (pl[Bishop] > 1)
// 		value += bishopPair;
//
// 	value += b_p[pl[Pawn]];
	RawScore value = 0;

    return value;
}

RawScore Eval::pawns(const BoardBase& ) const {

    RawScore value = 0;

//	uint64_t wp = b.pieceList[0].bitBoard<Pawn>();
//	uint64_t bp = b.pieceList[1].bitBoard<Pawn>();

    return value;
}

RawScore Eval::eval(const BoardBase& b) const {
#if defined(MYDEBUG)
    RawScore value = 0;
	for (int pi = 0; pi <= 1; ++pi) {
		int C = 1-pi*2;
		uint8_t king = b.pieceList[pi].getKing();
		value += getPS(C*King, king);
		for (uint8_t i = b.pieceList[pi][Pawn]; i > 0;) {
			--i;
			uint8_t pawn = b.pieceList[pi].getPawn(i);
			value += getPS(C*Pawn, pawn);
		}
		for (uint8_t i = b.pieceList[pi][Rook]; i > 0;) {
			--i;
			uint8_t rook = b.pieceList[pi].get(Rook, i);
			value += getPS(C*Rook, rook);
		}
		for (uint8_t i = b.pieceList[pi][Bishop]; i > 0;) {
			--i;
			uint8_t bishop = b.pieceList[pi].get(Bishop, i);
			value += getPS(C*Bishop, bishop);
		}
		for (uint8_t i = b.pieceList[pi][Knight]; i > 0;) {
			--i;
			uint8_t knight = b.pieceList[pi].get(Knight, i);
			value += getPS(C*Knight, knight);
		}
		for (uint8_t i = b.pieceList[pi][Queen]; i > 0;) {
			--i;
			uint8_t queen = b.pieceList[pi].get(Queen, i);
			value += getPS(C*Queen, queen);
		}
	}
	if (value != b.keyScore.score) asm("int3");

#endif
	return b.keyScore.score;
}
