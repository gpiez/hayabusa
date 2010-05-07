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
#ifndef ATTACKS_H_
#define ATTACKS_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "constants.h"
/*
 * Structure which contains the number of rooks, bishops, queens
 * which do long range attacks on this square/piece.
 * It should fit in one byte, this makes it neccessary to have
 * a different structure if there are more queens/rooks/sameColoredBishops
 * than the bitfield can hold.
 */
struct LAttack {
	uint8_t R:nRBits;
	uint8_t B:nBBits;
	uint8_t Q:nQBits;
	uint8_t KR:nCheckKRBits;
	uint8_t KB:nCheckKBBits;

	operator uint8_t& () {
		return *(uint8_t*)this;
	}
	operator uint8_t () const {
		return *(uint8_t*)this;
	}
};

/*
 * Structure which contains the number of knights, pawn, kings
 * which do short range attacks on this square/piece.
 */
struct SAttack {
	uint8_t N:nNBits;
	uint8_t PR:nPRBits;
	uint8_t PL:nPLBits;
	uint8_t K:nKBits;
	uint8_t KN:nKNAttackBits;
	uint8_t KP:nKPAttackBits;

	operator uint8_t& () {
		return *(uint8_t*)this;
	}
	operator uint8_t () const {
		return *(uint8_t*)this;
	}
};

struct Attack {
	LAttack l;
	SAttack s;
	LAttack ll;
	SAttack ss;

	operator uint32_t () const {
		return *(uint32_t*)this;
	}
	operator uint32_t& () {
		return *(uint32_t*)this;
	}
};

static const SAttack attackN = { 1, 0, 0, 0 };
static const SAttack attackPR = { 0, 1, 0, 0 };
static const SAttack attackPL = { 0, 0, 1, 0 };
static const SAttack attackK = { 0, 0, 0, 1 };

static const LAttack attackMaskR = { (1<<nRBits)-1, 0, 0 };
static const LAttack attackMaskB = { 0, (1<<nBBits)-1, 0 };
static const LAttack attackMaskQ = { 0, 0, (1<<nQBits)-1 };

static const LAttack attackMaskLong = { (1<<nRBits)-1, (1<<nBBits)-1, (1<<nQBits)-1 };

static const SAttack attackMaskN = { (1<<nNBits)-1, 0, 0 };
static const SAttack attackMaskP = { 0, (1<<nPRBits)-1, (1<<nPLBits)-1, 0 };
static const SAttack attackMaskK = { 0, 0, 0, (1<<nKBits)-1 };

static const SAttack attackMaskShort = { (1<<nNBits)-1, (1<<nPRBits)-1, (1<<nPLBits)-1, (1<<nKBits)-1 };

static const LAttack attackR = { 1, 0, 0, 0, 0 };
static const LAttack attackB = { 0, 1, 0, 0, 0 };
static const LAttack attackQ = { 0, 0, 1, 0, 0 };
static const LAttack checkKR = { 0, 0, 0, 1, 0 };
static const LAttack checkKB = { 0, 0, 0, 0, 1 };

static const Attack attackMask = { attackMaskLong, attackMaskShort, attackMaskLong, attackMaskShort };
#endif /* ATTACKS_H_ */
