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
#ifdef BITBOARD
template<Colors C>
template<typename T>
ColoredBoard<C>::ColoredBoard(const T& prev, Move m, __v8hi est) {
    prev.doMove(this, m);
    keyScore.vector = est;
    buildAttacks();
}
#else
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
#endif

template<Colors C>
template<typename T>
void ColoredBoard<C>::doMove(T* next, Move m) const {
#ifdef BITBOARD
    uint64_t from = 1ULL << m.from();
    uint64_t to = 1ULL << m.to();

    __m128i xmm0 = _mm_load_si128((__m128i*)pieces);
    __m128i xmm1 = _mm_load_si128((__m128i*)pieces+1);
    __m128i xmm2 = _mm_load_si128((__m128i*)pieces+2);
    __m128i xmm3 = _mm_load_si128((__m128i*)pieces+3);
    _mm_store_si128((__m128i*)next->pieces  , xmm0);
    _mm_store_si128((__m128i*)next->pieces+1, xmm1);
    _mm_store_si128((__m128i*)next->pieces+2, xmm2);
    _mm_store_si128((__m128i*)next->pieces+3, xmm3);
    xmm0 = _mm_load_si128((__m128i*)pieces+4);
    xmm1 = _mm_load_si128((__m128i*)pieces+5);
    xmm2 = _mm_load_si128((__m128i*)pieces+6);
    xmm3 = _mm_load_si128((__m128i*)pieces+7);
    _mm_store_si128((__m128i*)next->pieces+4 , xmm0);
    _mm_store_si128((__m128i*)next->pieces+5, xmm1);
    _mm_store_si128((__m128i*)next->pieces+6, xmm2);
    _mm_store_si128((__m128i*)next->pieces+7, xmm3);

    next->cep.castling.data4 = cep.castling.data4 & castlingMask[m.from()].data4 & castlingMask[m.to()].data4;
    
    if (m.isSpecial()) {
        next->fiftyMoves = 0;
        next->cep.enPassant = 0;
        unsigned int piece = m.piece() & 7;
        if (piece == King) {
            ASSERT(m.capture() == 0);
            next->getPieces<C,King>() ^= from + to;
            if (m.to() == (pov^g1)) {
                // short castling
                next->occupied2 = _mm_set1_epi64x(occupied1 ^ (0b1111ULL << m.from()));
                next->getPieces<C,Rook>() ^= (from + to) << 1;
            } else {
                // long castling
                next->occupied2 = _mm_set1_epi64x(occupied1 ^ (0b11101ULL << (m.to() & 070)));
                next->getPieces<C,Rook>() ^= (from >> 1) + (from >> 4);
            }
            ASSERT(popcount(next->getPieces<C,Rook>()) == popcount(getPieces<C,Rook>()));
        } else {
            // promotion
            ASSERT(m.captureOffset() == 0);
            next->occupied2 = _mm_set1_epi64x(occupied1 ^ (from + (m.capture() ? 0:to)));
            next->getPieces<C,Pawn>() ^= from;
            next->getPieces<C>(piece) ^= to;
            next->getPieces<-C>(m.capture()) ^= to;
        }
    } else {
        // standard move, e. p. is handled by captureOffset
        next->fiftyMoves = (m.capture() & 7) | (m.piece()==Pawn) ? 0:fiftyMoves+1;
        next->cep.enPassant = m.piece()==Pawn ? to & shift<C*16>(from) & rank<4>() & (getPieces<-C,Pawn>() << 1 | getPieces<-C,Pawn>() >> 1) : 0;
        next->occupied2 = _mm_set1_epi64x(occupied1 ^ (from + (m.capture() & 7 ? 0:to)));
        next->getPieces<C>(m.piece()) ^= from + to;
        if (C == White)
            next->getPieces<-C>(m.capture() & 7) ^= to >> m.captureOffset();
        else
            next->getPieces<-C>(m.capture() & 7) ^= to << m.captureOffset();
        
    }
#else
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
#endif    
}

template<Colors C>
__v8hi ColoredBoard<C>::estimatedEval(const Move m, const Eval& eval) const {
    if (m.isSpecial()) {
    	unsigned piece = m.piece() & 7;
    	if (piece == King) {
            ASSERT(m.capture() == 0);
    		__v8hi estKing = keyScore.vector
				- eval.getKSVector(piece, m.from())
				+ eval.getKSVector(piece, m.to());
            if (m.to() == (pov^g1)) {
            	return estKing - eval.getKSVector(C*Rook, pov^h1)
            				   + eval.getKSVector(C*Rook, pov^f1);
            } else {
            	return estKing - eval.getKSVector(C*Rook, pov^a1)
            				   + eval.getKSVector(C*Rook, pov^d1);
            }
    	} else {
    		return keyScore.vector - eval.getKSVector(C*Pawn, m.from())
    						       + eval.getKSVector(C*piece, m.to())
    						       - eval.getKSVector(-C*m.capture(), m.to());
    	}
    } else {
        return keyScore.vector
            - eval.getKSVector(C*m.piece(), m.from())
            + eval.getKSVector(C*m.piece(), m.to())
            - eval.getKSVector(-C*(m.capture() & 7), m.to() - C*m.captureOffset());
    }
}

#ifndef BITBOARD
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
#endif
template<Colors C>
Key ColoredBoard<C>::getZobrist() const {
    return keyScore.key + cep.castling.data4 + cep.enPassant + (C+1);
}

//attacked by (opposite colored) piece.
// if color == White, attackedBy<-King> = b
template<Colors C>
template<int P>
bool ColoredBoard<C>::attackedBy(uint8_t pos) {
#ifdef BITBOARD
    return false;
#else
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
#endif    
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

#ifdef BITBOARD
#else
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
#endif
#endif /* COLOREDBOARD_TCC_ */
