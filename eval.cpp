/*
 * eval.cpp
 *
 *  Created on: 29.10.2009
 *      Author: gpiez
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
Eval eval;

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

	static RawScore bishopGoodD;
	static RawScore bishopKnightD;
	static RawScore bishopRookD;
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

RawScore Eval::pieces(const PieceList& pl) const {
	RawScore value = 0;
	if (pl.getCounts() == 0x01000100000000) 	//only one knight left
		value += knightAlone;
	if (pl.getCounts() == 0x01000000010000) 	//only one bishop left
		value += knightAlone;
	if (pl[Bishop] > 1)
		value += bishopPair;

	value += b_p[pl[Pawn]];

	return value;
}

RawScore Eval::pawns(const BoardBase& b) const {
	RawScore value = 0;

	uint64_t wp = b.pieceList[0].bitBoard<Pawn>();
	uint64_t bp = b.pieceList[1].bitBoard<Pawn>();

	return value;
}

RawScore Eval::eval(const BoardBase& b) const {
	return 0;
}
