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
void ColoredBoard<C>::generateTargetMove(Move*& bad, uint64_t tobit ) const {
    //special case where movin a pawn blocks a check
    //move is only legal if it is not pinned
    for (uint64_t p = getPieces<C,Pawn>() & shift<-C*8>(tobit) & pins[CI] & rank<7>(); p; p &= p-1) {
        *bad++ = Move(bit(p), bit(p) + C*8, Queen, 0, true);
        *bad++ = Move(bit(p), bit(p) + C*8, Knight, 0, true);
        *bad++ = Move(bit(p), bit(p) + C*8, Rook, 0, true);
        *bad++ = Move(bit(p), bit(p) + C*8, Bishop, 0, true); }

    for (uint64_t p = getPieces<C,Pawn>() & shift<-C*8>(tobit) & pins[CI] & ~rank<7>(); p; p &= p-1)
        *bad++ = Move(bit(p), bit(p) + C*8, Pawn);

    for (uint64_t p = getPieces<C,Pawn>() & shift<-C*16>(tobit) & pins[CI] & rank<2>() & ~shift<-C*8>(occupied1); p; p &= p-1)
        *bad++ = Move(bit(p), bit(p) + C*16, Pawn);

    for ( uint64_t p = getAttacks<C,Knight>() & tobit; p; p &= p-1 ) {
        uint64_t to = bit(p);
        for ( uint64_t f = getPieces<C,Knight>() & pins[CI] & knightAttacks[to]; f; f &= f-1)
            *bad++ = Move(bit(f), to, Knight); }

    /* Check for attacks from sliding pieces. Bishop/Rook/Queen attacks are
     * stored in single[], with a mov template matching the attack in move[]
     */
    const __v2di zero = _mm_set1_epi64x(0);
//     const __v2di d2 = _mm_set1_epi64x(tobit);

    if unlikely((getAttacks<C,Bishop>() & tobit)) {
        const MoveTemplateB* bs = bsingle[CI];
        Move m = bs->move;
        do {
            __v2di from2 = bits[m.from()].doublebits;
            __v2di pin13 = from2 & dpins[CI].d13;
            pin13 = pcmpeqq(pin13, zero);
            pin13 = ~pin13 & bs->d13;
            for (uint64_t a=tobit & fold(pin13); a; a&=a-1) {
                Move n;
                n.data = m.data + Move(0, bit(a), 0).data;
                *bad++ = n; }
            m = (++bs)->move; }
        while (m.data); }

    if unlikely((getAttacks<C,Rook>() & tobit)) {
        const MoveTemplateR* rs = rsingle[CI];
        Move m = rs->move;
        do {
            __v2di from2 = bits[m.from()].doublebits;
            __v2di pin02 = from2 & dpins[CI].d02;
            pin02 = pcmpeqq(pin02, zero);
            pin02 = ~pin02 & rs->d02;
            for (uint64_t a=tobit & fold(pin02); a; a&=a-1) {
                Move n;
                n.data = m.data + Move(0, bit(a), 0).data;
                *bad++ = n; }
            m = (++rs)->move; }
        while (m.data); }

    if unlikely((getAttacks<C,Queen>() & tobit)) {
        const MoveTemplateQ* qs = qsingle[CI];
        Move m = qs->move;
        do {
            __v2di from2 = bits[m.from()].doublebits;
            __v2di pin02 = from2 & dpins[CI].d02;
            __v2di pin13 = from2 & dpins[CI].d13;
            pin02 = pcmpeqq(pin02, zero);
            pin13 = pcmpeqq(pin13, zero);
            pin02 = ~pin02 & qs->d02;
            pin13 = ~pin13 & qs->d13;
            for (uint64_t a=tobit & fold((pin02|pin13)); a; a&=a-1) {
                Move n;
                n.data = m.data + Move(0, bit(a), 0).data;
                *bad++ = n; }
            m = (++qs)->move; }
        while (m.data); } }

template<Colors C>
void ColoredBoard<C>::generateCheckEvasions(Move*& good, Move*& bad) const {
    ASSERT(inCheck<C>());
    uint64_t king = getPieces<C,King>();
    uint64_t kingSq = bit(king);
    unsigned check;
    /*
     * First generate in-between moves and the move capturing one of checking
     * pieces if possible. Treat checks by pawns separatly, as there is only one
     * case of discovered check.
     */
    if (uint64_t pawn = pawnAttacks[CI][kingSq] & getPieces<-C,Pawn>()) {
        ASSERT(popcount(pawn)==1);
        // only way for a pawn to give check and to discover check is with an
        // vertical attacking piece behind it.
        if (datt[EI].d2 & king) {
            check = 2; }
        else {
            check = 0;
            generateTargetCapture<NoKingPawn>(good, bad, pawn, Pawn);
            if (pawn == cep.enPassant) {
                if (uint64_t p = getPieces<C,Pawn>() & ~file<'a'>() & cep.enPassant<<1 & getPins<C,2+C>())
                    *--good = Move(bit(p), bit(p) + C*8-1, Pawn, Pawn, true);

                if (uint64_t p = getPieces<C,Pawn>() & ~file<'h'>() & cep.enPassant>>1 & getPins<C,2-C>())
                    *--good = Move(bit(p), bit(p) + C*8+1, Pawn, Pawn, true); } }
        /*
         * All other cases of checks with sliding pieces and knights. Determine
         * directions of checking, set bits in "check" accordingly
         * If attacked by a single piece, blocking moves or capturing the
         * attacking piece is allowed, otherwise only escape moves are considered
         * Escape directions are determined by precalculated kingAttacks.
         */
    }
    else {
        __v2di king2 = _mm_set1_epi64x(king);
        __v2di zero = _mm_set1_epi64x(0);
        __v2di check02 = datt[EI].d02 & king2;
        __v2di check13 = datt[EI].d13 & king2;

        check02 = pcmpeqq(check02, zero);
        check13 = pcmpeqq(check13, zero);
        check02 = ~check02 & _mm_set_epi64x(2,1);
        check13 = ~check13 & _mm_set_epi64x(8,4);

        check = fold(check02|check13)
                + (king & getAttacks<-C, Knight>() ? 16:0);
        ASSERT(check);
        if (__builtin_parity(check)) {
            if (check == 16) {
                // attack by knight, try to capture it.
                if (uint64_t p = knightAttacks[kingSq] & getPieces<-C, Knight>())
                    generateTargetCapture<AllMoves>(good, bad, p, Knight); }
            else {
                ASSERT(check & 0xf);
                // attack by a single sliding piece. determine position
                // and try to capture it. generate blocking moves
                uint64_t kinc = kingIncoming[CI].d[bit(check)];
                if (uint64_t q = kinc & getAttacks<C,All>() & getPieces<-C,Queen>())
                    generateTargetCapture<AllMoves>(good, bad, q, Queen);
                else if (check & 3) {
                    if (uint64_t r = kinc & getAttacks<C,All>() & getPieces<-C,Rook>())
                        generateTargetCapture<AllMoves>(good, bad, r, Rook); }
                else if (uint64_t b = kinc & getAttacks<C,All>() & getPieces<-C,Bishop>())
                    generateTargetCapture<AllMoves>(good, bad, b, Bishop);

                if (uint64_t p = datt[EI].d[bit(check)] & kinc & ~occupied1)
                    generateTargetMove(bad, p); } }
        else {
            // test, if capturing an adjacent checking piece with the king is possible
            // this is not covered by non capturing king moves below.
            if (uint64_t p = kingAttacks[0b1100][kingSq] & ~getAttacks<-C, All>() & getPieces<-C, Rook>())
                *--good = Move(kingSq, bit(p), King, Rook);
            else if (uint64_t p = kingAttacks[0b0011][kingSq] & ~getAttacks<-C, All>() & getPieces<-C, Bishop>())
                *--good = Move(kingSq, bit(p), King, Bishop);
            else if (uint64_t p = kingAttacks[0b0000][kingSq] & ~getAttacks<-C, All>() & getPieces<-C, Queen>())
                *--good = Move(kingSq, bit(p), King, Queen); } }

    // non capturing check evasions
    ASSERT(popcount(check & 0xf) <= 2);
    for (uint64_t p = kingAttacks[check & 0xf][kingSq] & ~getAttacks<-C, All>() & ~occupied1; p; p &= p-1)
        *bad++ = Move(kingSq, bit(p), King);

    // capturing evasions of non checking adjacent pieces. checking pieces
    // have the direction disallowed by the check & 0xf mask, so they are
    // covered by generateTargetCapture in the single check sliding attack
    // or with the individual tests above
    for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                      & ~getAttacks<-C, All>()
            & getPieces<-C,Rook>(); p; p &= p-1)
        *--good = Move(kingSq, bit(p), King, Rook);

    for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                      & ~getAttacks<-C, All>()
            & getPieces<-C,Bishop>(); p; p &= p-1)
        *--good = Move(kingSq, bit(p), King, Bishop);

    for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                      & ~getAttacks<-C, All>()
            & getPieces<-C,Knight>(); p; p &= p-1)
        *--good = Move(kingSq, bit(p), King, Knight);

    for (uint64_t p = kingAttacks[check & 0xf][kingSq]
                      & ~getAttacks<-C, All>()
            & getPieces<-C,Pawn>(); p; p &= p-1)
        *--good = Move(kingSq, bit(p), King, Pawn);
    return; }

template<Colors C>
void ColoredBoard<C>::generateNonCap(Move*& good, Move*& bad) const {
    using namespace SquareIndex;
    uint64_t from, to;
    from = bit(getPieces<C, King>());
    for (uint64_t p = kingAttacks[0][from] & ~occupied1 & ~getAttacks<-C,All>(); p; p &= p-1)
        *--good = Move(from, bit(p), King);

    // pawn moves, non capture, non promo
    for (uint64_t p = getPieces<C,Pawn>() & ~rank<7>() & shift<-C*8>(~occupied1) & getPins<C,2>(); p; p &= p-1) {
        from = bit(p);
        to = from + C*dirOffsets[2];
        *--good = Move(from, to, Pawn); }
    // pawn double steps
    for (uint64_t p = getPieces<C,Pawn>() & rank<2>() & shift<-C*8>(~occupied1) & shift<-C*16>(~occupied1) & getPins<C,2>(); p; p &= p-1) {
        from = bit(p);
        to = from + 2*C*dirOffsets[2];
        *--good = Move (from, to, Pawn); }
    /*
     * Sliding pieces. If we detect a possible pinning, allow only the moves in
     * the directions of the pin. For masking the pinned directions use the
     * attack information stored in single[] in the same order as they were
     * generated earlier in buildAttacks(). Generating only legal moves comes at
     * a cost of a load+cmp+2*and here and a store+and earlier in buildattacks
     * per piece and twice that for the queen.
     */
    const __v2di zero = _mm_set1_epi64x(0);
    for(const MoveTemplateQ* qs = qsingle[CI]; ; qs++) {
        Move m = qs->move;
        if (!m.data) break;
        __v2di a02 = qs->d02;
        __v2di a13 = qs->d13;
        __v2di from2 = bits[m.from()].doublebits;
        __v2di pin02 = from2 & dpins[CI].d02;
        __v2di pin13 = from2 & dpins[CI].d13;
        pin02 = pcmpeqq(pin02, zero);
        pin13 = pcmpeqq(pin13, zero);
        pin02 = ~pin02 & a02;
        pin13 = ~pin13 & a13;
        for (uint64_t a=fold(pin02|pin13) & ~occupied1; a; a &= a-1 ) {
            Move n;
            to = bit(a);
            n.data = m.data + Move(0, to, 0).data;
            if ( a & -a & ( getAttacks<-C,Pawn>()
                            | getAttacks<-C,Knight>()
                            | getAttacks<-C,Bishop>()
                            | getAttacks<-C,Rook>()
                          )
                    /*                   || a & -a & getAttacks<-C,All>() &
                                                     ~( getAttacks<C,Rook>()
                                                      | getAttacks<C,Bishop>()
                                                      | getAttacks<C,King>()
                                                      | getAttacks<C,Knight>()
                                                      | getAttacks<C,Pawn>()
                                                      )*/
               ) *bad++ = n;
            else *--good = n; } }

    for(const MoveTemplateR* rs = rsingle[CI]; ; rs++) {
        Move m = rs->move;
        if (!m.data) break;
        __v2di a02 = rs->d02;
        __v2di from2 = bits[m.from()].doublebits;
        __v2di pin02 = from2 & dpins[CI].d02;
        pin02 = pcmpeqq(pin02, zero);
        pin02 = ~pin02 & a02;
        for (uint64_t a=fold(pin02) & ~occupied1; a; a &= a-1 ) {
            Move n;
            to = bit(a);
            n.data = m.data + Move(0, to, 0).data;
            if ( a & -a & ( getAttacks<-C,Pawn>()
                            | getAttacks<-C,Knight>()
                            | getAttacks<-C,Bishop>()
                          )
               ) *bad++ = n;
            else *--good = n; } }

    for(const MoveTemplateB* bs = bsingle[CI]; ; bs++) {
        Move m = bs->move;
        if (!m.data) break;
        __v2di a13 = bs->d13;
        __v2di from2 = bits[m.from()].doublebits;
        __v2di pin13 = from2 & dpins[CI].d13;
        pin13 = pcmpeqq(pin13, zero);
        pin13 = ~pin13 & a13;
        for (uint64_t a=fold(pin13) & ~occupied1; a; a &= a-1 ) {
            Move n;
            to = bit(a);
            n.data = m.data + Move(0, to, 0).data;
            if ( a & -a & ( getAttacks<-C,Pawn>() ) ) *bad++ = n;
            else *--good = n; } }

    //Knight moves. If a knight is pinned, it can't move at all.
    for (uint64_t p = getPieces<C, Knight>() & pins[CI]; p; p &= p-1) {
        from = bit(p);
        for (uint64_t q = knightAttacks[from] & ~occupied1; q; q &= q-1) {
            to = bit(q);
            /*            if ( q & -q & getAttacks<-C,Pawn>() )
                            *bad++ = Move(from, to, Knight);
                        else*/
            *--good = Move(from, to, Knight); } }
    /*
     * test the castling flags and if there is free room between k and r
     * additionally check for non threatened squares at the king
     */
    if ( cep.castling.color[CI].q
            && (occupied1 &             (C==White ? 0xe:0xe00000000000000)) == 0
            && (getAttacks<-C, All>() & (C==White ? 0xc:0xc00000000000000)) == 0 )
        *--good = Move(pov^e1, pov^c1, King, 0, true);

    if ( cep.castling.color[CI].k
            && (occupied1 &             (C==White ? 0x60:0x6000000000000000)) == 0
            && (getAttacks<-C, All>() & (C==White ? 0x60:0x6000000000000000)) == 0 )
        *--good = Move(pov^e1, pov^g1, King, 0, true);
    /*
     * En passant capture
     * Allow a pinned pawn to move, if it is moving in the direction of the pin. The captured pawn can never be pinned,
     * since it must have been moved before and so our king would have been in check while the enemy was on move.
     * Additionally check if we have reached the 5th rank, else we might capture our own pawn,
     * if we made two moves in a row and the last move was a double step of our own pawn
     * Handle special case, where king, capturing pawn, captured pawn and horizontal
     * attacking piece are on one line. Although neither pawn is pinned, the capture
     * is illegal, since both pieces are removed and the king will be in check
     */
    if (uint64_t p = getPieces<C,Pawn>() & ~file<'a'>() & rank<5>() & cep.enPassant<<1 & getPins<C,2+C>())
        if (((datt[EI].d0|kingIncoming[CI].d0) & (p|cep.enPassant)) != (p|cep.enPassant))
            *--good = Move(bit(p), bit(p) + C*8-1, Pawn, Pawn, true);

    if (uint64_t p = getPieces<C,Pawn>() & ~file<'h'>() & rank<5>() & cep.enPassant>>1 & getPins<C,2-C>())
        if (((datt[EI].d0|kingIncoming[CI].d0) & (p|cep.enPassant)) != (p|cep.enPassant))
            *--good = Move(bit(p), bit(p) + C*8+1, Pawn, Pawn, true); }

#endif /* GENERATEMOVES_TCC_ */
