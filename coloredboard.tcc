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
#ifndef COLOREDBOARD_TCC_
#define COLOREDBOARD_TCC_

#include "coloredboard.h"
#include "boardbase.tcc"
#include "rootboard.h"

/* Execute move m from position prev, both of the previous (opposite) color
 * to construct a board with C to move
 */
template<Colors C>
ColoredBoard<C>::ColoredBoard(const ColoredBoard<(Colors)-C>& prev, Move m, const Eval& e) {
    prev.doMove(this, m, e);
}

template<Colors C>
ColoredBoard<C>::ColoredBoard(const ColoredBoard<C>& prev, Move m, const Eval& e) {
    prev.doMove((ColoredBoard<(Colors)-C>*)this, m, e);
}

template<Colors C>
ColoredBoard<C>::ColoredBoard(const ColoredBoard<(Colors)-C>& prev, Move m, __v8hi est, uint64_t cep) {
    prev.doMoveEst(this, m, cep);
    keyScore.vector = est;
}

template<Colors C>
ColoredBoard<C>::ColoredBoard(const ColoredBoard<C>& prev, Move m, __v8hi est, uint64_t cep) {
    prev.doMoveEst((ColoredBoard<(Colors)-C>*)this, m, cep);
    keyScore.vector = est;
}

template<Colors C>
void ColoredBoard<C>::doMove(ColoredBoard<(Colors)-C>* next, Move m, const Eval& e) const {
    uint8_t piece = C*pieces[m.from];
    next->copyBoardClrPiece<C>(this, piece, m.from, e);
    next->cep.enPassant = 0;
    next->cep.castling.data4 = cep.castling.data4 & castlingMask[m.from].data4 & castlingMask[m.to].data4;
    
    next->fiftyMoves = (m.capture!=0) | (piece==5) ? 0:fiftyMoves+1;
    ASSERT(C*m.capture < King);
    
    switch (m.special) {
    case nothingSpecial:
        break;
    case shortCastling:
        next->clrPiece<C>(Rook, pov^h1, e);
        next->setPiece<C>(Rook, pov^f1, e);
        break;
    case longCastling:
        next->clrPiece<C>(Rook, pov^a1, e);
        next->setPiece<C>(Rook, pov^d1, e);
        break;
    case promoteQ:
        piece = Queen;
        break;
    case promoteR:
        piece = Rook;
        break;
    case promoteB:
        piece = Bishop;
        break;
    case promoteN:
        piece = Knight;
        break;
    case enableEP:
        next->cep.enPassant = m.to;
        break;
    case EP:
        next->clrPiece<(Colors)-C>(Pawn, cep.enPassant, e);
        break;
    default:
        __builtin_unreachable();
    }

    if (m.capture)
        next->chgPiece<C>(-C*m.capture, piece, m.to, e);
    else
        next->setPiece<C>(piece, m.to, e);
}

template<Colors C>
void ColoredBoard<C>::doMoveEst(ColoredBoard<(Colors)-C>* next, Move m, uint64_t cepdata) const {
    uint8_t piece = C*pieces[m.from];
    next->copyBoardClrPieceEst<C>(this, piece, m.from);
#ifdef MYDEBUG    
    next->cep.enPassant = 0;
    next->cep.castling.data4 = cep.castling.data4 & castlingMask[m.from].data4 & castlingMask[m.to].data4;
#endif
    next->fiftyMoves = (m.capture!=0) | (piece==5) ? 0:fiftyMoves+1;
    ASSERT(C*m.capture < King);

    switch (m.special) {
    case nothingSpecial:
        break;
    case shortCastling:
        next->clrPieceEst<C>(Rook, pov^h1);
        next->setPieceEst<C>(Rook, pov^f1);
        break;
    case longCastling:
        next->clrPieceEst<C>(Rook, pov^a1);
        next->setPieceEst<C>(Rook, pov^d1);
        break;
    case promoteQ:
        piece = Queen;
        break;
    case promoteR:
        piece = Rook;
        break;
    case promoteB:
        piece = Bishop;
        break;
    case promoteN:
        piece = Knight;
        break;
    case enableEP:
#ifdef MYDEBUG        
        next->cep.enPassant = m.to;
#endif
        break;
    case EP:
        next->clrPieceEst<(Colors)-C>(Pawn, cep.enPassant);
        break;
    default:
        __builtin_unreachable();
    }

    ASSERT(next->cep.data8 == cepdata);
    next->cep.data8 = cepdata;
    
    if (m.capture)
//        next->clrPiece<(Colors)-C>(-C*m.capture, m.to, rb);
        next->chgPieceEst<C>(-C*m.capture, piece, m.to);
    else
        next->setPieceEst<C>(piece, m.to);
}

template<Colors C>
Key ColoredBoard<C>::getZobrist() const {
    return keyScore.key + cep.data8 + (C+1);
}

//attacked by (opposite colored) piece.
// if color == White, attackedBy<-King> = b
template<Colors C>
template<int P>
bool ColoredBoard<C>::attackedBy(uint8_t pos) {
    switch(C*P) {
    case King:     // if color == Black, attackedBy<-King> = w
        return shortAttack[0][pos] & attackMaskK;
        break;
    case Pawn:
        return shortAttack[0][pos] & attackMaskP;
        break;
    case Knight:
        return shortAttack[0][pos] & attackMaskN;
        break;
    case -King:     // if color == Black, attackedBy<-King> = w
        return shortAttack[1][pos] & attackMaskK;
        break;
    case -Pawn:
        return shortAttack[1][pos] & attackMaskP;
        break;
    case -Knight:
        return shortAttack[1][pos] & attackMaskN;
        break;
    case Queen:
        return longAttack[0][pos] & attackMaskQ;
        break;
    case Bishop:
        return longAttack[0][pos] & attackMaskB;
        break;
    case Rook:
        return longAttack[0][pos] & attackMaskR;
        break;
    case -Queen:
        return longAttack[1][pos] & attackMaskQ;
        break;
    case -Bishop:
        return longAttack[1][pos] & attackMaskB;
        break;
    case -Rook:
        return longAttack[1][pos] & attackMaskR;
        break;
    }
}

template<Colors C>
uint8_t ColoredBoard<C>::diaPinTable[nDirs][256];

template<Colors C>
void ColoredBoard<C>::initTables() {
    for (unsigned int dir = 0; dir<nDirs; dir++)
    for (int8_t l = -King; l<=King; ++l)
    for (int8_t r = -King; r<=King; ++r) {
        LongIndex i = {{ l, r }};
        diaPinTable[dir][i] = ~0;
        if ( (dir&1) && l == C*King && (r == -C*Bishop || r == -C*Queen) )
            diaPinTable[dir][i] = dir;
        if ( (dir&1) && r == C*King && (l == -C*Bishop || l == -C*Queen) )
            diaPinTable[dir][i] = dir;
        if ( !(dir&1) && l == C*King && (r == -C*Rook || r == -C*Queen) )
            diaPinTable[dir][i] = dir;
        if ( !(dir&1) && r == C*King && (l == -C*Rook || l == -C*Queen) )
            diaPinTable[dir][i] = dir;
    }
}

template<Colors C>
__v8hi ColoredBoard<C>::estimatedEval(const Move m, const Eval& eval, uint64_t& cepdata) const {
    int8_t piece = pieces[m.from];
    __v8hi estimate;
    estimate = keyScore.vector - eval.getKSVector(piece, m.from);
    CastlingAndEP nextcep;
    nextcep.enPassant = 0;
    nextcep.castling.data4 = cep.castling.data4 & castlingMask[m.from].data4 & castlingMask[m.to].data4;

    switch (m.special) {
    case nothingSpecial:
        break;
    case shortCastling:
        estimate +=  eval.getKSVector(C*Rook, pov^f1)-eval.getKSVector(C*Rook, pov^h1);
        break;
    case longCastling:
        estimate +=  eval.getKSVector(C*Rook, pov^d1)-eval.getKSVector(C*Rook, pov^a1);
        break;
    case promoteQ:
        piece = C*Queen;
        break;
    case promoteR:
        piece = C*Rook;
        break;
    case promoteB:
        piece = C*Bishop;
        break;
    case promoteN:
        piece = C*Knight;
        break;
    case enableEP:
        nextcep.enPassant = m.to;
        break;
    case EP:
        estimate -= eval.getKSVector(-C*Pawn, cep.enPassant);
        break;
    default:
        __builtin_unreachable();
    }

    if (m.capture)
        estimate -= eval.getKSVector(m.capture, m.to);
    estimate += eval.getKSVector(piece, m.to);
    cepdata = nextcep.data8;
    return estimate;
}
#endif /* COLOREDBOARD_TCC_ */
