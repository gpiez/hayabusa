/*
 * generateCaptureMoves.tcc
 *
 *  Created on: 09.11.2009
 *      Author: gpiez
 */

#ifndef GENERATECAPTUREMOVES_TCC_
#define GENERATECAPTUREMOVES_TCC_

#include "coloredboard.h"

extern uint8_t vec2dir[nSquares][nSquares];
extern uint8_t vec2diff[nSquares*2];

/* Returns -1 if the piece is not pinned, else the direction (0..7) of the attacking piece.
 * A piece is pinned, if it is on a check ray of the king and attacked by a sliding piece
 * in the same direction
 */
template<Colors C>
uint8_t ColoredBoard<C>::detectPin( unsigned int pos ) const {
	/*
	 * Pin by a rook/queen: (own & checkR) && (opp & attackMaskRQ), possible dirs 04 26
	 * Pin by a bish/queen: (own & checkB) && (opp & attackMaskB) | attackMaskQ), possible dirs 15 37
	 */
	if ( !(longAttack[CI][pos] & (checkKB|checkKR)) | !(longAttack[EI][pos]&attackMaskLong) )
		return ~0;
	uint8_t dir = vec2dir[pos][pieceList[CI].getKing()];
	ASSERT( dir <8 );

	if ( isLongAttack(dir, pos) )
		return dir&3;
	return ~0;
}
/*
 * Check if to is attacked by a pawn. If so, iterate through both possible from positions.
 * If a opposite ppiece is there, a capturing move is found. Calculate possible pins/checks
 * and insert it in list, if it is legal.
 */
template<Colors C>
void ColoredBoard<C>::generateTargetCapture(Move* &list, const uint8_t to, const int8_t cap, Attack a, const SpecialMoves spec) const {
	uint8_t from;
	uint8_t pin;
	uint8_t dir;
	unsigned int nAttacks;
	uint64_t sources;
	/*
	 * Pawn captures to the right. Only queen promotions.
	 */
	if ( a.s.PR ) {
		dir = 1;
		from = to - C*9;
		ASSERT(pieces[from] == C*Pawn);
		pin = detectPin(from);
		if ( !isValid(pin) | pin==dir ) {
			SpecialMoves sm = spec;
			if (isPromoRank(from)) sm = (SpecialMoves)(spec + promoteQ);
			*list++ = (Move) { from, to, cap, sm};
		}
	}
	/*
	 * Pawn captures to the left.
	 */
	if ( a.s.PL ) {
		dir = 3;
		from = to - C*7;
		ASSERT(pieces[from] == C*Pawn);
		pin = detectPin(from);
		if (  !isValid(pin) | pin==dir ) {
			SpecialMoves sm = spec;
			if (isPromoRank(from)) sm = (SpecialMoves)(spec + promoteQ);
			*list++ = (Move) { from, to, cap, sm};
		}
	}
	/*
	 * Knight captures something. Can't move at all if captured.
	 */
	if (a.s.N) {
		nAttacks = a.s.N;
		sources = pieceList[CI].getAll(Knight);
		unsigned int n = 0;
		while (true) {
			ASSERT(++n <= pieceList[CI][Knight]);
			from = sources;
			sources >>= 8;
			if (isKnightDistance(from, to)) {
				if (!isValid(detectPin(from))) *list++ = (Move) {from, to, cap, spec};
				if (!--nAttacks) break;
			}
		}
	}
	/*
	 * King captures something. If it could castle, it is on the starting position and
	 * so it can't be capturing an enemy castling-rook.
	 */
	if ( a.s.K )
		if (!(attacks<EI>(to) & attackMask)) {
			from = pieceList[CI].getKing();
			*list++ = (Move) {from, to, cap, castling[CI].k|castling[CI].q ? disableCastling : spec};
		}

	if (a.l.B) {
		nAttacks = a.l.B;
		sources = pieceList[CI].getAll(Bishop);
		unsigned int n = 0;
		while(true) {
			ASSERT(++n <= pieceList[CI][Bishop]);
			from = sources;
			sources >>= 8;
			dir = vec2dir[from][to];
			if (isValid(dir) & dir & 1 && length(dir, from)*dirOffsets[dir] + from == to) {
				pin = detectPin(from);
				if (!isValid(pin) | pin==(dir&3))	*list++ = (Move) {from, to, cap, spec};
				if (!--nAttacks) break;
			}
		}
	}
	/*
	 * Rook captures something. Check if it is a castling-rook and disable castling in that case.
	 * Always add spec, it could capture the other rook and siable castling for the opponent.
	 */
	if (a.l.R) {
		nAttacks = a.l.R;
		sources = pieceList[CI].getAll(Rook);
		unsigned int n = 0;
		while(true) {
			ASSERT(++n <= pieceList[CI][Rook]);
			from = sources;
			sources >>= 8;
			dir = vec2dir[from][to];
			if (~dir & 1 && length(dir, from)*dirOffsets[dir] + from == to) {
				SpecialMoves sm = spec;
				if (castling[CI].q && from == (pov^a1))
					sm = (SpecialMoves)(spec + disableLongCastling);
				else if (castling[CI].k && from == (pov^h1))
					sm = (SpecialMoves)(spec + disableShortCastling);
				pin = detectPin(from);
				if (!isValid(pin) | pin==(dir&3)) *list++ = (Move) {from, to, cap, sm};
				if (!--nAttacks) break;
			}
		}
	}

	if (a.l.Q) {
		nAttacks = a.l.Q;
		sources = pieceList[CI].getAll(Queen);
		unsigned int n = 0;
		while(true) {
			ASSERT(++n <= pieceList[CI][Queen]);
			from = sources;
			sources >>= 8;
			dir = vec2dir[from][to];
			if (isValid(dir) && from + length(dir, from)*dirOffsets[dir] == to) {
				pin = detectPin(from);
				if (!isValid(pin) | pin==(dir&3)) *list++ = (Move) {from, to, cap, spec};
				if (!--nAttacks) break;
			}
		}
	}
	return;
}
/*
 * Generate all pawn to queen promotions and capture moves for each kind of piece.
 * A pinned piece may only move on the line between king and pinning piece.
 * For a pawn on the last rank this can only happen if the pawn captures the pinning piece.
 */
template<Colors C>
Move* ColoredBoard<C>::generateCaptureMoves( Move* list) const {
	uint8_t to;
 	Attack a;
	/*
	 * Generate non-capturing queen promotions. Capturing promotions are handled in
	 * generateTargetCaptures().
	 */
	for (unsigned int i = 0; i < pieceList[CI][Pawn]; ++i) {
		unsigned int pawn = pieceList[CI].getPawn(i);			//TODO load all 8 pieces at once and shift instead?
		if (isPromoRank(pawn)) {
			uint8_t pin=detectPin(pawn);

			to = pawn + C*dirOffsets[2];
			if ( !pieces[to] & !isValid(pin))
				*list++ = (Move) {pawn, to, 0, promoteQ};
		}
	}
	/*
	 * A queen can be captured.
	 */
	for (unsigned int i = 0; i < pieceList[EI][Queen]; ++i) {
		to = pieceList[EI].get(Queen, i);
		a = attacks<CI>(to);
		if (a) generateTargetCapture(list, to, -C*Queen, a, nothingSpecial);
	}
	/*
	 * A rook can be captured. Detect if it is a castling-rook and set disableOpponentCastling.
	 */
	for (unsigned int i = 0; i < pieceList[EI][Rook]; ++i) {
		to = pieceList[EI].get(Rook, i);
		a = attacks<CI>(to);
		if (a) {
			SpecialMoves spec;
			if (to == (pov^h8) & castling[EI].k)
				spec = disableOpponentShortCastling;
			else if (to == (pov^a8) & castling[EI].q)
				spec = disableOpponentLongCastling;
			else
				spec = nothingSpecial;
			generateTargetCapture(list, to, -C*Rook, a, spec);
		}
	}

	for (unsigned int i = 0; i < pieceList[EI][Bishop]; ++i) {
		to = pieceList[EI].get(Bishop, i);
		a = attacks<CI>(to);
		if (a) generateTargetCapture(list, to, -C*Bishop, a,  nothingSpecial);
	}

	for (unsigned int i = 0; i < pieceList[EI][Knight]; ++i) {
		to = pieceList[EI].get(Knight, i);
		a = attacks<CI>(to);
		if (a) generateTargetCapture(list, to, -C*Knight, a,  nothingSpecial);
	}

	for (unsigned int i = 0; i < pieceList[EI][Pawn]; ++i) {
		to = pieceList[EI].get(Pawn, i);
		a = attacks<CI>(to);
		if (a) generateTargetCapture(list, to, -C*Pawn, a,  nothingSpecial);
	}

	return list;
}
#endif /* GENERATECAPTUREMOVES_TCC_ */
