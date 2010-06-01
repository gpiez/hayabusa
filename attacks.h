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

/* Attack constants should be defined as integral types rather than structs,
 * because the compiler doesn't use them as immediate constants otherway.
 */
static const SAttack attackN_ = { 1, 0, 0, 0 };
static const uint8_t attackN = 1;
static const SAttack attackPR_ = { 0, 1, 0, 0 };
static const uint8_t attackPR = 8;
static const SAttack attackPL_ = { 0, 0, 1, 0 };
static const uint8_t attackPL = 0x10;
static const SAttack attackK_ = { 0, 0, 0, 1 };
static const uint8_t attackK = 0x20;

static const LAttack attackMaskR_ = { (1<<nRBits)-1, 0, 0 };
static const uint8_t attackMaskR = 3;
static const LAttack attackMaskB_ = { 0, (1<<nBBits)-1, 0 };
static const uint8_t attackMaskB = 0xc;
static const LAttack attackMaskQ_ = { 0, 0, (1<<nQBits)-1 };
static const uint8_t attackMaskQ = 0x30;

static const LAttack attackMaskLong_ = { (1<<nRBits)-1, (1<<nBBits)-1, (1<<nQBits)-1 };
static const uint8_t attackMaskLong = 0x3f;

static const SAttack attackMaskN_ = { (1<<nNBits)-1, 0, 0 };
static const uint8_t attackMaskN = 7;
static const SAttack attackMaskP_ = { 0, (1<<nPRBits)-1, (1<<nPLBits)-1, 0 };
static const uint8_t attackMaskP = 0x18;
static const SAttack attackMaskK_ = { 0, 0, 0, (1<<nKBits)-1 };
static const uint8_t attackMaskK = 0x20;

static const SAttack attackMaskShort_ = { (1<<nNBits)-1, (1<<nPRBits)-1, (1<<nPLBits)-1, (1<<nKBits)-1 };
static const uint8_t attackMaskShort = 0x3f;

static const LAttack attackR_ = { 1, 0, 0, 0, 0 };
static const uint8_t attackR = 1;
static const LAttack attackB_ = { 0, 1, 0, 0, 0 };
static const uint8_t attackB = 4;
static const LAttack attackQ_ = { 0, 0, 1, 0, 0 };
static const uint8_t attackQ = 0x10;
static const LAttack checkKR_ = { 0, 0, 0, 1, 0 };
static const uint8_t checkKR = 0x40;
static const LAttack checkKB_ = { 0, 0, 0, 0, 1 };
static const uint8_t checkKB = 0x80;

static const Attack attackMask_ = { attackMaskLong_, attackMaskShort_, attackMaskLong_, attackMaskShort_ };
static const uint32_t attackMask = 0x3f3f3f3f;
#endif /* ATTACKS_H_ */
