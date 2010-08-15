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
#ifndef BOARDBASE_TCC_
#define BOARDBASE_TCC_

#include "boardbase.h"
#include "boardbase.tcc"
#include "rootboard.h"

#ifdef BITBOARD

inline __v2di BoardBase::build13Attack(const unsigned sq) const {
	const __v16qi swap16 = { 7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8 };

    __v2di maskedDirs = _mm_set1_epi64x(occupied1) & mask13x[sq];
    __v2di reverse = _mm_shuffle_epi8(maskedDirs, swap16);
    maskedDirs -= doublebits[sq];
    reverse -= doublereverse[sq];
    reverse = _mm_shuffle_epi8(reverse, swap16);
    maskedDirs ^= reverse;
    maskedDirs &= mask13x[sq];
    return maskedDirs;
}

inline __v2di BoardBase::build02Attack(const unsigned sq) const {
    const __v2di b02 = _mm_set_epi64x(0xff, 0x0101010101010101); // border

    __v2di maskedDir02 = (_mm_set1_epi64x(occupied1)|b02) & mask02b[sq];
    uint64_t d0 = _mm_cvtsi128_si64x(maskedDir02);
    uint64_t d2 = _mm_cvtsi128_si64x(_mm_unpackhi_epi64(maskedDir02,maskedDir02));
    d0 <<= 63-sq;
    d2 <<= 63-sq;
    uint64_t lo = rol(6, sq + __bsrq(d0));
    uint64_t hi = rol(6, sq + __bsrq(d2));
    maskedDir02 ^= maskedDir02 - _mm_set_epi64x(hi, lo);
    maskedDir02 &= mask02x[sq];
    return maskedDir02;
//    return _mm_set1_epi64x(att0Tab[attLen0[sq]][sq], att2Tab[attLen2[sq]][sq]);
}

template<Colors C>
void BoardBase::buildAttacks() {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };
    uint64_t p, a;
    const __v2di zero = _mm_set1_epi64x(0);

    p = getPieces<C, Knight>();
    a = 0;
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        a |= knightAttacks[sq];
    }
    getAttacks<C, Knight>() = a;
    getAttacks<C, All>() = a;

    MoveTemplateB* bs = bsingle[CI];
//    Move* pmove = moves[CI];
    __v2di dir13 = _mm_set1_epi64x(0);
    p = getPieces<C, Bishop>();
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;													// lat  #op
        __v2di a13 = build13Attack(sq);
        bs->move = Move(sq, 0, Bishop);
//        psingle->d02 = zero;
        bs->d13 = a13;
        bs++;
        dir13 |= a13;                                        // 1    1
    }															    // 18   22
    bs->move = Move(0,0,0);
    a = fold(dir13);
    getAttacks<C, Bishop>() = a;
    getAttacks<C, All>() |= a;

    MoveTemplateR* rs = rsingle[CI];
    __v2di dir02 = _mm_set1_epi64x(0);
    p = getPieces<C, Rook>();
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        __v2di a02 = build02Attack(sq);
        rs->move = Move(sq, 0, Rook);
        rs->d02 = a02;
        rs++;
        dir02 |= a02;
    }
    rs->move = Move(0,0,0);
    a = fold(dir02);
    getAttacks<C, Rook>() = a;
    getAttacks<C, All>() |= a;

    MoveTemplateQ* qs = qsingle[CI];
    p = getPieces<C, Queen>();
    a = 0;
    while(p) {
        uint64_t sq = bit(p);
        p &= p-1;
        __v2di a13 = build13Attack(sq);
        __v2di a02 = build02Attack(sq);
        qs->move = Move(sq, 0, Queen);
        qs->d02 = a02;
        qs->d13 = a13;
        qs++;
        dir02 |= a02;
        dir13 |= a13;
        a |= fold(a13 | a02);
    }
    qs->move = Move(0,0,0);
    getAttacks<C, Queen>() = a;
    getAttacks<C, All>() |= a;
    datt[CI].d02 = dir02;
    datt[CI].d13 = dir13;

    p = getPieces<C, Pawn>();
    if (C == White)
        getAttacks<C, All>() |= getAttacks<C, Pawn>() = (p << 7 & 0x7f7f7f7f7f7f7f7f) | (p << 9 & 0xfefefefefefefefe);
    else
        getAttacks<C, All>() |= getAttacks<C, Pawn>() = (p >> 9 & 0x7f7f7f7f7f7f7f7f) | (p >> 7 & 0xfefefefefefefefe);

}

/*
    Generate king attacks and pins on own king. Assuming after executing a move
    at least a exact eval is needed, all the parts need for the eval are
    calculated here too.
*/
template<Colors C>
void BoardBase::buildPins() {
    enum { CI = C == White ? 0:1, EI = C == White ? 1:0 };

    uint64_t p = getPieces<C, King>();
    uint64_t sq = bit(p);
    getAttacks<C, All>() |= getAttacks<C, King>() = kingAttacks[0][sq];

    __v2di maskedDir13 = build13Attack(sq);
    __v2di maskedDir02 = build02Attack(sq);
    kingIncoming[CI].d13 = maskedDir13;
    kingIncoming[CI].d02 = maskedDir02;
    __v2di dir02 = ~(maskedDir02 & datt[EI].d02);
    __v2di dir13 = ~(maskedDir13 & datt[EI].d13);
    __v2di dir20 = _mm_shuffle_epi32(dir02, 0x4e);
    __v2di dir31 = _mm_shuffle_epi32(dir13, 0x4e);
    dpins[CI].d02 = dir20 & dir13 & dir31;
    dpins[CI].d13 = dir31 & dir02 & dir20;
    __v2di pins2 = dpins[CI].d02 & dpins[CI].d13;
    pins[CI] = _mm_cvtsi128_si64(_mm_unpackhi_epi64(pins2, pins2))
         & _mm_cvtsi128_si64(pins2);
}

#else

namespace as {
extern "C" {
    void setPieceW(BoardBase*, int, int);
    void setPieceB(BoardBase*, int, int);
    void clrPieceAndCopyW(const BoardBase*, BoardBase*, int, int);
    void clrPieceAndCopyB(const BoardBase*, BoardBase*, int, int);
    void clrPieceW(BoardBase*, BoardBase*, int, int);
    void clrPieceB(BoardBase*, BoardBase*, int, int);
    void chgPieceW(BoardBase*, int, int, int);
    void chgPieceB(BoardBase*, int, int, int);
}
}

template<Colors C>
void BoardBase::setPiece(uint8_t piece, uint8_t pos, const Eval& e) {
    ASSERT(piece <= King && piece > 0);
    ASSERT(pos < 64);
    ASSERT(pieces[pos] == 0);
    pieceList[C<0].add(piece, pos);
//    PawnKey temp = keyScore.pawnKey;
    keyScore.vector += e.getKSVector(C*piece, pos);
//    keyScore.pawnKey = temp ^ rb.getPawnKey(C*piece, pos);
    if (C==White)
        as::setPieceW(this, C*piece, pos);
    else
        as::setPieceB(this, C*piece, pos);
    for (unsigned int i = 0; i < 256; ++i) {
        ASSERT(64 > attLen[0][i]);
    }
    pieces[pos] = C*piece;
}

template<Colors C>
void BoardBase::copyBoardClrPiece(const BoardBase* prev, uint8_t piece, uint8_t pos, const Eval& e) {
    ASSERT(piece <= King && piece > 0);
    ASSERT(pos < 64);
    prev->copyPieces(this);
    ASSERT(pieces[pos] == C*piece);
    pieceList[C<0].sub(piece, pos);
    keyScore.vector = prev->keyScore.vector - e.getKSVector(C*piece, pos);
//    keyScore.pawnKey = prev->keyScore.pawnKey ^ rb.getPawnKey(C*piece, pos);
    if (C==White)
        as::clrPieceAndCopyW(prev, this, C*piece, pos);
    else
        as::clrPieceAndCopyB(prev, this, C*piece, pos);
    for (unsigned int i = 0; i < 256; ++i) {
        ASSERT(64 > attLen[0][i]);
    }
    pieces[pos] = 0;
}

template<Colors C>
void BoardBase::clrPiece(uint8_t piece, uint8_t pos, const Eval& e) {
    ASSERT(piece <= King && piece > 0);
    ASSERT(pos < 64);
    ASSERT(pieces[pos] == C*piece);
    pieceList[C<0].sub(piece, pos);
//    PawnKey temp = keyScore.pawnKey;
    keyScore.vector -= e.getKSVector(C*piece, pos);
//    keyScore.pawnKey = temp ^ rb.getPawnKey(C*piece, pos);
    if (C==White)
        as::clrPieceW(this, this, C*piece, pos);
    else
        as::clrPieceB(this, this, C*piece, pos);
    for (unsigned int i = 0; i < 256; ++i) {
        ASSERT(64 > attLen[0][i]);
    }
    pieces[pos] = 0;
}

template<Colors C>
void BoardBase::chgPiece(uint8_t oldpiece, uint8_t piece, uint8_t pos, const Eval& e) {
    ASSERT(piece <= King && piece > 0);
    ASSERT(oldpiece < King && oldpiece > 0);
    ASSERT(pos < 64);
    ASSERT(pieces[pos] == -C*oldpiece);
    pieceList[C>0].sub(oldpiece, pos);
    pieceList[C<0].add(piece, pos);
//    PawnKey temp = keyScore.pawnKey;
    keyScore.vector += e.getKSVector(C*piece, pos) - e.getKSVector(-C*oldpiece, pos);
//    keyScore.pawnKey = temp ^ rb.getPawnKey(C*piece, pos) ^ rb.getPawnKey(-C*oldpiece, pos);
    if (C==White) {
        as::chgPieceW(this, -C*oldpiece, C*piece, pos);
    } else {
        as::chgPieceB(this, -C*oldpiece, C*piece, pos);
    }
    for (unsigned int i = 0; i < 256; ++i) {
        ASSERT(64 > attLen[0][i]);
    }
    pieces[pos] = C*piece;
}

template<Colors C>
void BoardBase::setPieceEst(uint8_t piece, uint8_t pos) {
    ASSERT(piece <= King && piece > 0);
    ASSERT(pos < 64);
    ASSERT(pieces[pos] == 0);
    pieceList[C<0].add(piece, pos);
    if (C==White)
        as::setPieceW(this, C*piece, pos);
    else
        as::setPieceB(this, C*piece, pos);
    for (unsigned int i = 0; i < 256; ++i) {
        ASSERT(64 > attLen[0][i]);
    }
    pieces[pos] = C*piece;
}

template<Colors C>
void BoardBase::copyBoardClrPieceEst(const BoardBase* prev, uint8_t piece, uint8_t pos) {
    ASSERT(piece <= King && piece > 0);
    ASSERT(pos < 64);
    prev->copyPieces(this);
    ASSERT(pieces[pos] == C*piece);
    pieceList[C<0].sub(piece, pos);
    if (C==White)
        as::clrPieceAndCopyW(prev, this, C*piece, pos);
    else
        as::clrPieceAndCopyB(prev, this, C*piece, pos);
    for (unsigned int i = 0; i < 256; ++i) {
        ASSERT(64 > attLen[0][i]);
    }
    pieces[pos] = 0;
}

template<Colors C>
void BoardBase::clrPieceEst(uint8_t piece, uint8_t pos) {
    ASSERT(piece <= King && piece > 0);
    ASSERT(pos < 64);
    ASSERT(pieces[pos] == C*piece);
    pieceList[C<0].sub(piece, pos);
    if (C==White)
        as::clrPieceW(this, this, C*piece, pos);
    else
        as::clrPieceB(this, this, C*piece, pos);
    for (unsigned int i = 0; i < 256; ++i) {
        ASSERT(64 > attLen[0][i]);
    }
    pieces[pos] = 0;
}

template<Colors C>
void BoardBase::chgPieceEst(uint8_t oldpiece, uint8_t piece, uint8_t pos) {
    ASSERT(piece <= King && piece > 0);
    ASSERT(oldpiece < King && oldpiece > 0);
    ASSERT(pos < 64);
    ASSERT(pieces[pos] == -C*oldpiece);
    pieceList[C>0].sub(oldpiece, pos);
    pieceList[C<0].add(piece, pos);
    if (C==White) {
        as::chgPieceW(this, -C*oldpiece, C*piece, pos);
    } else {
        as::chgPieceB(this, -C*oldpiece, C*piece, pos);
    }
    for (unsigned int i = 0; i < 256; ++i) {
        ASSERT(64 > attLen[0][i]);
    }
    pieces[pos] = C*piece;
}
#endif
#endif
