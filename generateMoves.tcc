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
#ifndef GENERATEMOVES_TCC_
#define GENERATEMOVES_TCC_

#include "generateCaptureMoves.tcc"

/*
 * Generate moves which place a piece at dst, used for check evasion generation
 */
template<Colors C>
void ColoredBoard<C>::generateTargetMove(Move* &list, uint8_t to ) const {
	ASSERT(to < nSquares);
	Attack a = attacks<CI>( to );
	int cap = pieces[to];
	uint8_t dir;
	uint8_t from;
	uint8_t pin;
	unsigned int nAttacks;
	uint64_t sources;
	/*
	 * If we are capturing, we need a special treatment if the captured piece is a pawn which
	 * has made a doublestep, because we can escape from a check by capturing e.p.
	 * Special treatment is also needed for pawn captures, they may be promotions
	 * And for captured rooks, as this may disable castling for the opponent
	 */
	if ( cap ) {
		if ( enPassant == to )
			for ( unsigned int i=0; i<pieceList[CI][Pawn]; ++i ) {
				from = pieceList[CI].getPawn(i);
				if ( (from == to + 1) | (from == to - 1) )
					/* The situation is now that a pawn is giving us check. This pawn has made a double
					 * step in the move before and can be captured en passant by one of our pawns.
					 * The move is only legal if our pawn is not pinned, since this pawn will move on
					 * a different vector than the pin in every possible case. Avoiding a check
					 * by moving a pawn in between with an e. p. capture is impossible 
					 * leave capture field empty, otherwise we would capture twice */
					if (!isValid(detectPin(from))) *list++ = (Move) { from, to+C*8, 0, EP };
			}

		if ( a.s.PR ) {
			from = to-C*9;
			ASSERT(pieces[from] == C*Pawn);
			if (!isValid(detectPin(from))) {
				if (isPromoRank(from)) {
					*list++ = (Move) { from, to, cap, promoteQ };
					*list++ = (Move) { from, to, cap, promoteN };
					*list++ = (Move) { from, to, cap, promoteR };
					*list++ = (Move) { from, to, cap, promoteB };
				} else
					*list++ = (Move) { from, to, cap, nothingSpecial };
			}
		}

		if ( a.s.PL ) {
			from = to-C*7;
			ASSERT(pieces[from] == C*Pawn);
			if (!isValid(detectPin(from))) {
				if (isPromoRank(from)) {
					*list++ = (Move) { from, to, cap, promoteQ };
					*list++ = (Move) { from, to, cap, promoteN };
					*list++ = (Move) { from, to, cap, promoteR };
					*list++ = (Move) { from, to, cap, promoteB };
				} else
					*list++ = (Move) { from, to, cap, nothingSpecial };
			}
		}

	} else {
		//special case where movin a pawn blocks a check
		//move is only legal if it is not pinned
		from = to - C*8;
		if ( from < nSquares && pieces[from] == C*Pawn ) {
			if (!isValid(detectPin(from))) {
				if (isPromoRank(from)) {
					*list++ = (Move) { from, to, 0, promoteQ };
					*list++ = (Move) { from, to, 0, promoteN };
					*list++ = (Move) { from, to, 0, promoteR };
					*list++ = (Move) { from, to, 0, promoteB };
				} else
					*list++ = (Move) { from, to, 0, 0 };

			}
		} else {
			from = to - C*16;
			if ( isRank<2>(from) & (pieces[from] == C*Pawn) & (pieces[to-C*8] == 0) )
				if (!isValid(detectPin(from))) {
					bool pawnadj = ((to & 7) != 7 && pieces[to+1] == -C*Pawn)
								|| ((to & 7) != 0 && pieces[to-1] == -C*Pawn);
					*list++ = (Move) { from, to, 0, pawnadj ? enableEP:nothingSpecial };
				}

		}
	}

	if (a.s.N) {
		nAttacks = a.s.N;
		sources = pieceList[CI].getAll(Knight);
		unsigned int n = 0;
		while (true) {
			ASSERT(++n <= pieceList[CI][Knight]);
			from = sources;
			sources >>= 8;
			if (isKnightDistance(from, to)) {
				if (!isValid(detectPin(from))) *list++ = (Move) {from, to, cap, nothingSpecial};
				if (!--nAttacks) break;
			}
		}
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
			if (isValid(dir) & dir & 1 && length(dir^4, to)*dirOffsets[dir] + from == to) {
				pin = detectPin(from);
				if (!isValid(pin))	*list++ = (Move) {from, to, cap, nothingSpecial};
				if (!--nAttacks) break;
			}
		}
	}

	if (a.l.R) {
		nAttacks = a.l.R;
		sources = pieceList[CI].getAll(Rook);
		unsigned int n = 0;
		while(true) {
			ASSERT(++n <= pieceList[CI][Rook]);
			from = sources;
			sources >>= 8;
			dir = vec2dir[from][to];
			if (~dir & 1 && length(dir^4, to)*dirOffsets[dir] + from == to) {
				pin = detectPin(from);
				if (!isValid(pin)) *list++ = (Move) {from, to, cap, nothingSpecial};
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
			if (isValid(dir) && from + length(dir^4, to)*dirOffsets[dir] == to) {
				pin = detectPin(from);
				if (!isValid(pin)) *list++ = (Move) {from, to, cap, nothingSpecial};
				if (!--nAttacks) break;
			}
		}
	}
}

template<Colors C>
void ColoredBoard<C>::ray(Move* &list, uint8_t from, uint8_t dir) const {
	unsigned int l = length(dir, from);
	if (!l) return;
	uint8_t to = from + dirOffsets[dir];
	for (unsigned int i=l; i>1; --i) {			//TODO vecotrize and put 4 moves at once?
		*list++ = (Move) { from, to, 0, 0 };
		to += dirOffsets[dir];
	}
	if (!pieces[to])
		*list++ = (Move) { from, to, 0, 0 };
}

template<Colors C>
Move* ColoredBoard<C>::generateMoves(Move* list) const {

	unsigned int king = pieceList[CI].getKing();
	Attack kingAttacks;
	(uint32_t&)kingAttacks = attacks<EI>(king) & attackMask;

	if (kingAttacks) {
		unsigned int attDir = ~0; //used as disallowed move directions in evasion
		unsigned int attDir2 = ~0;
		/*
		 * Test if attacked by a single piece. The king can at most be attacked by two pieces, so
		 * here are at least one and at most two bits set. The two pieces are not guaranteed to be
		 * different, because a pawn may uncover a check and promote to the same piece. So a parity
		 * check does not work in deciding if we have a double check.
		 * If so, skip to the king moves, no other moves are legal
		 * first check for a attack from a sliding piece. in this case all moves with destination
		 * to a square between the king and the attacking piece or destination attacking piece are
		 * allowed.
		 */
		LAttack doubleAttack = { 2, 2, 2 };
		
		ASSERT(__builtin_popcount(kingAttacks) <= 2);
		if (__builtin_parity(kingAttacks) && !(kingAttacks.l & doubleAttack) ) {

			/* Test if attacked by a sliding piece. Find opposite direction of the attack.
			 * Besides evading moves (which are genreated later), the only Legal moves are moves
			 * blocking or capturing the piece */
			if (kingAttacks & attackMaskLong) {
				//TODO replace by a table lookup
				unsigned int len;
				for (unsigned int antiDir = 0;; ++antiDir) {
					ASSERT( antiDir < 4);
					if (isLongAttack(antiDir+4, king)) {
						len = attLen[antiDir][king].right;
						attDir = antiDir + 4;
						break;
					}

					if (isLongAttack(antiDir, king)) {
						len = attLen[antiDir][king].left;
						attDir = antiDir;
						break;
					}
				}

				unsigned int dst = king;
				for (unsigned int i = len; i > 0; --i) {
					dst -= dirOffsets[attDir];
					generateTargetMove(list, dst);
				}

				// Test if attacked by a knight. Only moves capturing the knight are legal
			} else if (kingAttacks.s.N) {
				unsigned int i;
				for (i = 0; i < pieceList[EI][Knight]; ++i)
					if (isKnightDistance(king, pieceList[EI].get(Knight, i))) break;

				ASSERT(isKnightDistance(king, pieceList[EI].get(Knight, i)));
				generateTargetMove(list, pieceList[EI].get(Knight, i));

				// No other pieces left, we are attacked by one pawn. only two possible destinations
			} else {
				ASSERT(kingAttacks.s.PR ^ kingAttacks.s.PL);
				if (kingAttacks.s.PR) {
					unsigned int to = king + dirOffsets[1 + CI * 4];
					ASSERT(pieces[to] == -C * Pawn);
					generateTargetMove(list, to);
				} else {
					unsigned int to = king + dirOffsets[3 + CI * 4];
					ASSERT(pieces[to] == -C * Pawn);
					generateTargetMove(list, to);
				}
			}
		} else {
			//double check. find if we are attacked by two sliding pieces, and caculate the direction of the second attack
			//king movement isn't allowd in neither of these directions
			for (unsigned int i = 0; i < 4; ++i) {
				if (isLongAttack(i, king)) {
					attDir2 = attDir;
					attDir = i;
				}
				if (isLongAttack(i+4, king)) {
					attDir2 = attDir;
					attDir = i + 4;
				}
			}
		}

		for (unsigned int i = 0; i < 8; ++i) {
			unsigned int to = king + dirOffsets[i];
			if (!isKingDistance(king, to)) continue;
			int cap = pieces[to];
			if (i != attDir && i != attDir2 && C * cap <= 0 && !((longAttack[EI][to] & attackMaskLong) | (shortAttack[EI][to] & attackMaskShort))) *list++ = (Move) {king, to, cap, /*disableCastling*/ nothingSpecial};
		}
		return list;
	}

	list = generateCaptureMoves( list );

	uint8_t from;
	uint8_t to;
	uint8_t pin;
	int8_t cap;
	for (unsigned int i = 0; i < pieceList[CI][Pawn]; ++i) {
		from = pieceList[CI].getPawn(i);
		pin = detectPin(from);

		//generate underpromotions. queening is already done by movegenq.
		//if a promoting pawn is pinned it must not move - except if it is capturing in the opposite direction of the pin.
		if (isPromoRank(from)) {
			to = from + C * 9;
			cap = pieces[to];
			if ((C*cap < 0) & ((from&7) != 7*EI) && (!isValid(pin) | (pin == 1))) {
				*list++ = (Move) {from, to, cap, promoteN};
				*list++ = (Move) {from, to, cap, promoteR};
				*list++ = (Move) {from, to, cap, promoteB};
			}

			to = from + C * 7;
			cap = pieces[to];
			if ((C*cap < 0) & ((from&7) != 7*CI) && (!isValid(pin) | (pin == 3))) {
				*list++ = (Move) {from, to, cap, promoteN};
				*list++ = (Move) {from, to, cap, promoteR};
				*list++ = (Move) {from, to, cap, promoteB};
			}

			to = from + C * 8;
			cap = pieces[to];
			if (!cap & !isValid(pin)) {
				*list++ = (Move) {from, to, 0, promoteN};
				*list++ = (Move) {from, to, 0, promoteR};
				*list++ = (Move) {from, to, 0, promoteB};
			}
			continue; // skip normal and en passant moves;
		}
		//normal pawn moves.
		//if the pawn is pinned, it is allowed to move if the pin is from direction 2 or 6 (vertically)
		//if there is an uncovered check, it can only be from an direction other than 2 or 6
		to = from + C * 8;
		if ((!pieces[to]) & (!isValid(pin) | (pin == 2))) {
			*list++ = (Move) {from, to, 0, 0};
			to = from + C * 16;
			if (isRank<2> (from) && !pieces[to]) {
				bool pawnadj = ((to & 7) != 7 && pieces[to+1] == -C*Pawn)
 							|| ((to & 7) != 0 && pieces[to-1] == -C*Pawn);
				*list++ = (Move) {from, to, 0, pawnadj ? enableEP:nothingSpecial};
				continue; // if a pawn has made a double step, skip the en passant check
			}
		}
/*
 * En passant capture
 * Allow a pinned pawn to move, if it is moving in the direction of the pin. The captured pawn can never be pinned,
 * since it must have been moved before and so our king would have been in check while the enemy was on move.
 * Additionally check if we have reached the 5th rank, else we might capture our own pawn,
 * if we made two moves in a row and the last move was a double step of our own pawn
 */
		if ( enPassant && isRank<5>(from))
		if (((( from+(int8_t)C == enPassant ) && ( !isValid(pin) | (pin == 1)))
		|| (( from-(int8_t)C == enPassant ) && ( !isValid(pin) | (pin == 3))))) {
			/*
			 * Handle special case, where king, capturing pawn, captured pawn and horizontal
			 * attacking piece are on one line. Although neither pawn is pinned, the capture
			 * is illegal, since both pieces are removed and the king will be in check
			 */
			if (((attVec[0][from].lIndex != C*King) & (attVec[0][from].rIndex != C*King)
				|| (attVec[0][enPassant].lIndex != -C*Rook) & (attVec[0][enPassant].rIndex != -C*Rook)
					& (attVec[0][enPassant].lIndex != -C*Queen) & (attVec[0][enPassant].rIndex != -C*Queen))
		    &&  ((attVec[0][enPassant].lIndex != C*King) & (attVec[0][enPassant].rIndex != C*King)
		    	|| (attVec[0][from].lIndex != -C*Rook) & (attVec[0][from].rIndex != -C*Rook)
		    	    & (attVec[0][from].lIndex != -C*Queen) & (attVec[0][from].rIndex != -C*Queen))) {
				to = enPassant + C*8;
				*list++ = (Move) { from, to, 0, EP};	//leave capture field empty, otherwise we would capture twice
			}
		}
	}
	/*
	 * Sliding pieces. If we detect a possible pinning, allow only the moves in
	 * the directions of the pin. In this case the difference between the
	 * direction
	 */
	for (unsigned int i = 0; i<pieceList[CI][Queen]; ++i) {
		from = pieceList[CI].get(Queen, i);
		pin = detectPin(from);

		if (!isValid(pin)) {
			ray(list, from, 0);
			ray(list, from, 4);
			ray(list, from, 1);
			ray(list, from, 5);
			ray(list, from, 2);
			ray(list, from, 6);
			ray(list, from, 3);
			ray(list, from, 7);
		} else {
			ray(list, from, pin);
			ray(list, from, pin+4);
		}
	}

	for (unsigned int i = 0; i<pieceList[CI][Bishop]; ++i) {
		from = pieceList[CI].get(Bishop, i);
		pin = detectPin(from);

		if (!isValid(pin)) {
			ray(list, from, 1);
			ray(list, from, 5);
			ray(list, from, 3);
			ray(list, from, 7);
		} else if (pin & 1) {
			ray(list, from, pin);
			ray(list, from, pin+4);
		}
	}

	for (unsigned int i = 0; i<pieceList[CI][Rook]; ++i) {
		from = pieceList[CI].get(Rook, i);
		pin = detectPin(from);

		if (!isValid(pin)) {
			ray(list, from, 0);
			ray(list, from, 4);
			ray(list, from, 2);
			ray(list, from, 6);
		} else if (~pin & 1) {
			ray(list, from, pin);
			ray(list, from, pin+4);
		}
	}

	//Knight moves. If a knight is pinned, it can't move at all.
	for (unsigned int i = 0; i<pieceList[CI][Knight]; ++i) {
		from = pieceList[CI].get(Knight, i);
		pin = detectPin(from);
		if (!isValid(pin)) {	//TODO this are 2^8 = 256 possible move combinations, depending on the 8 pieces[to] and from. replace by a table?
			to = from + 10;		// get whole board in 4 xmms, mask according to from, pshufb
			if (to < 64 && isKnightDistance(from, to) & !pieces[to])
				*list++ = (Move) { from, to, 0, 0 };
			to = from + 17;
			if (to < 64 && isKnightDistance(from, to) & !pieces[to])
				*list++ = (Move) { from, to, 0, 0 };
			to = from + 15;
			if (to < 64 && isKnightDistance(from, to) & !pieces[to])
				*list++ = (Move) { from, to, 0, 0 };
			to = from + 6;
			if (to < 64 && isKnightDistance(from, to) & !pieces[to])
				*list++ = (Move) { from, to, 0, 0 };
			to = from - 10;
			if (to < 64 && isKnightDistance(from, to) & !pieces[to])
				*list++ = (Move) { from, to, 0, 0 };
			to = from - 17;
			if (to < 64 && isKnightDistance(from, to) & !pieces[to])
				*list++ = (Move) { from, to, 0, 0 };
			to = from - 15;
			if (to < 64 && isKnightDistance(from, to) & !pieces[to])
				*list++ = (Move) { from, to, 0, 0 };
			to = from - 6;
			if (to < 64 && isKnightDistance(from, to) & !pieces[to])
				*list++ = (Move) { from, to, 0, 0 };
		}
	}

	/*castling
	test the castling flags,
	if there is enough free space besides the king (ATTACKLEN)
	if the king-squares are not under attack
	if there is a rook (it might have been captured and the flags are still live)
	so castling is still not handled correctly (the king rook might be captured, the queenrook moves to the place
	of the king rook and kingside castling is still possible*/
	if ( castling.castling[CI].q && length(0, pov^a1) == 4
	&& !(shortAttack[EI][pov^c1] & attackMaskShort)
	&& !(shortAttack[EI][pov^d1] & attackMaskShort)
	&& !(longAttack[EI][pov^c1] & attackMaskLong)
	&& !(longAttack[EI][pov^d1] & attackMaskLong)) {
		ASSERT(pieces[pov^a1] == C*Rook);
		ASSERT(pieces[pov^e1] == C*King);
		*list++ = (Move) { pov^e1, pov^c1, 0, longCastling };
	}

	if ( castling.castling[CI].k && length(0, pov^e1) == 3
	&& !(shortAttack[EI][pov^f1] & attackMaskShort)
	&& !(shortAttack[EI][pov^g1] & attackMaskShort)
	&& !(longAttack[EI][pov^f1] & attackMaskLong)
	&& !(longAttack[EI][pov^g1] & attackMaskLong)) {
		ASSERT(pieces[pov^h1] == C*Rook);
		ASSERT(pieces[pov^e1] == C*King);
		*list++ = (Move) { pov^e1, pov^g1, 0, shortCastling };
	}

	from = pieceList[CI].getKing();
	uint8_t spec = nothingSpecial;//castling.castling[CI].k|castling.castling[CI].q ? disableCastling:0;

	to = from + dirOffsets[0];
	if (isKingDistance(from, to) & !pieces[to] & !(shortAttack[EI][to] & attackMaskShort) & !(longAttack[EI][to] & attackMaskLong))
		*list++ = (Move) { from, to, 0, spec };
	to = from + dirOffsets[1];
	if (isKingDistance(from, to) & !pieces[to] & !(shortAttack[EI][to] & attackMaskShort) & !(longAttack[EI][to] & attackMaskLong))
		*list++ = (Move) { from, to, 0, spec };
	to = from + dirOffsets[2];
	if (isKingDistance(from, to) & !pieces[to] & !(shortAttack[EI][to] & attackMaskShort) & !(longAttack[EI][to] & attackMaskLong))
		*list++ = (Move) { from, to, 0, spec };
	to = from + dirOffsets[3];
	if (isKingDistance(from, to) & !pieces[to] & !(shortAttack[EI][to] & attackMaskShort) & !(longAttack[EI][to] & attackMaskLong))
		*list++ = (Move) { from, to, 0, spec };
	to = from + dirOffsets[4];
	if (isKingDistance(from, to) & !pieces[to] & !(shortAttack[EI][to] & attackMaskShort) & !(longAttack[EI][to] & attackMaskLong))
		*list++ = (Move) { from, to, 0, spec };
	to = from + dirOffsets[5];
	if (isKingDistance(from, to) & !pieces[to] & !(shortAttack[EI][to] & attackMaskShort) & !(longAttack[EI][to] & attackMaskLong))
		*list++ = (Move) { from, to, 0, spec };
	to = from + dirOffsets[6];
	if (isKingDistance(from, to) & !pieces[to] & !(shortAttack[EI][to] & attackMaskShort) & !(longAttack[EI][to] & attackMaskLong))
		*list++ = (Move) { from, to, 0, spec };
	to = from + dirOffsets[7];
	if (isKingDistance(from, to) & !pieces[to] & !(shortAttack[EI][to] & attackMaskShort) & !(longAttack[EI][to] & attackMaskLong))
		*list++ = (Move) { from, to, 0, spec };

	//king move is a uncovered check, iff the king is not moving in the direction of
	//the attack or in the opposite. in this case, (ucheckdir-i) & 3 becames 0.

	return list;
}

#endif /* GENERATEMOVES_TCC_ */
