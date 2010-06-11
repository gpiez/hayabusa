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
union LAttack {
    struct {
    uint8_t R:nRBits;
    uint8_t B:nBBits;
    uint8_t Q:nQBits;
    uint8_t KR:nCheckKRBits;
    uint8_t KB:nCheckKBBits;
    };
    uint8_t data;
    operator const uint8_t& () const {
        return data;
    }
};

/*
 * Structure which contains the number of knights, pawn, kings
 * which do short range attacks on this square/piece.
 */
union SAttack {
    struct {
    uint8_t N:nNBits;
    uint8_t PR:nPRBits;
    uint8_t PL:nPLBits;
    uint8_t K:nKBits;
    uint8_t KN:nKNAttackBits;
    uint8_t KP:nKPAttackBits;
    };
    uint8_t data;
    operator const uint8_t& () const {
        return data;
    }
};

union Attack {
    struct {
    LAttack l;
    SAttack s;
    };
    uint16_t data;
    operator const uint16_t& () const {
        return data;
    }
};

/* Attack constants should be defined as integral types rather than structs,
 * because the compiler doesn't use them as immediate constants otherway.
 */
static const SAttack attackN_ = {{ 1, 0, 0, 0 }};
static const uint8_t attackN = attackN_;
static const SAttack attackPR_ = {{ 0, 1, 0, 0 }};
static const uint8_t attackPR = attackPR_;
static const SAttack attackPL_ = {{ 0, 0, 1, 0 }};
static const uint8_t attackPL = attackPL_;
static const SAttack attackK_ = {{ 0, 0, 0, 1 }};
static const uint8_t attackK = attackK_;

static const LAttack attackMaskR_ = {{ (1<<nRBits)-1, 0, 0 }};
static const uint8_t attackMaskR = attackMaskR_;
static const LAttack attackMaskB_ = {{ 0, (1<<nBBits)-1, 0 }};
static const uint8_t attackMaskB = attackMaskB_;
static const LAttack attackMaskQ_ = {{ 0, 0, (1<<nQBits)-1 }};
static const uint8_t attackMaskQ = attackMaskQ_;

static const LAttack attackMaskLong_ = {{ (1<<nRBits)-1, (1<<nBBits)-1, (1<<nQBits)-1 }};
static const uint8_t attackMaskLong = attackMaskLong_.data;

static const SAttack attackMaskN_ = {{ (1<<nNBits)-1, 0, 0 }};
static const uint8_t attackMaskN = attackMaskN_;
static const SAttack attackMaskP_ = {{ 0, (1<<nPRBits)-1, (1<<nPLBits)-1, 0 }};
static const uint8_t attackMaskP = attackMaskP_;
static const SAttack attackMaskK_ = {{ 0, 0, 0, (1<<nKBits)-1 }};
static const uint8_t attackMaskK = attackMaskK_;

static const SAttack attackMaskShort_ = {{ (1<<nNBits)-1, (1<<nPRBits)-1, (1<<nPLBits)-1, (1<<nKBits)-1 }};
static const uint8_t attackMaskShort = attackMaskShort_.data;

static const LAttack attackR_ = {{ 1, 0, 0, 0, 0 }};
static const uint8_t attackR = attackR_;
static const LAttack attackB_ = {{ 0, 1, 0, 0, 0 }};
static const uint8_t attackB = attackB_;
static const LAttack attackQ_ = {{ 0, 0, 1, 0, 0 }};
static const uint8_t attackQ = attackQ_;
static const LAttack checkKR_ = {{ 0, 0, 0, 1, 0 }};
static const uint8_t checkKR = checkKR_;
static const LAttack checkKB_ = {{ 0, 0, 0, 0, 1 }};
static const uint8_t checkKB = checkKB_;

static const Attack attackMask_ = {{ attackMaskLong_, attackMaskShort_ }};
static const uint16_t attackMask = attackMask_;

static const LAttack doubleAttack_ = {{ 2, 2, 2 }};
static const uint16_t doubleAttack = doubleAttack_;

#endif /* ATTACKS_H_ */
