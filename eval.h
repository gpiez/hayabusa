/*
 * eval.h
 *
 *  Created on: 06.11.2009
 *      Author: gpiez
 */

#ifndef EVAL_H_
#define EVAL_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"
#include "score.h"
/*
 * A square attacked by a piece is higher evaluated if it inhibits moves from
 * enemy pieces to this square. For this to happen it must be either less
 * valuable than the inhibited piece or the SEE after placing the inhibited
 * piece on the square must be positive (this is both acuatally covered by the last
 * condition)
 * The inhibitor bonus depends on the number and kind of inhibited pieces, which is
 * a function of the attackTable and the distance to the enemy pieces
 *
 * v = (sum over pieces (v_inhPieces * n_inhPieces) + n_totalInhPieces^2 * v_totalInh) * squareValue
 * squareValue = attack_king(dist_king) + attack_piece(dist_piece) + attack_pawn(dist_pawn) + const
 */
struct SquareEval {
	RawScore	controls[nPieces*2 + 1];	//piece may move there unharmed.
	RawScore	attacks[nPieces*2 + 1];		//piece attacks/defends square.
};

template<typename T>
void sigmoid(T& p, double start, double end, double dcenter = 0, double width = 1.5986 );

class PieceList;
class BoardBase;
class Eval {
	static RawScore pawn, knight, bishop, rook, queen;
	static RawScore bishopPair;
	static RawScore knightAlone;
	static RawScore bishopAlone;
	static RawScore n_p_ram[9], n_p[17];
	static RawScore b_bad_own_p[9], b_p[17], b_p_ram[9], bpair_p[17];
	static RawScore r_p[17];
	static RawScore q_p[17];

	RawScore controls[nSquares];

public:
	Eval();
	unsigned int attackHash( unsigned int  pos );
	int squareControl();

	RawScore pieces(const PieceList&);
	RawScore pawns(const BoardBase&);
};

template<typename T>
void sigmoid(T& p, double start, double end, double dcenter, double width) {
	static const size_t n = sizeof(T)/sizeof(p[0])-1;
	double t0 = -dcenter;
	double t1 = n-dcenter;
	double l0 = 1/(1+exp(-t0/width));
	double l1 = 1/(1+exp(-t1/width));

	double r = (end - start)/(l1-l0);
	double a = start - l0*r;
	for (unsigned int i = 0; i <= n; ++i) {
		double t = i - dcenter;
		p[i] = lrint(a + r/(1.0 + exp(-t/width)));
	}
}

template<typename T>
void printSigmoid(T& p, QString str) {
	size_t n = sizeof(T)/sizeof(p[0])-1;
	QTextStream xout(stderr);
	xout << qSetFieldWidth(16) << str;
	for (unsigned int i = 0; i <= n; ++i) {
		xout << qSetFieldWidth(4) << p[i];
	}
	xout << endl;
}


#endif /* EVAL_H_ */
