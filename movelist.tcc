/*
 * movelist.tcc
 *
 *  Created on: Feb 17, 2012
 *      Author: gpiez
 */

#include "movelist.h"

template<Colors C, Phase P>
typename MoveList<C,P>::TT MoveList<C,P>::tt;
template<Colors C, Phase P>
typename MoveList<C,P>::Mates MoveList<C,P>::mates;
template<Colors C, Phase P>
typename MoveList<C,P>::Captures MoveList<C,P>::captures;
template<Colors C, Phase P>
typename MoveList<C,P>::Checks MoveList<C,P>::checks;
template<Colors C, Phase P>
typename MoveList<C,P>::Threats MoveList<C,P>::threats;
template<Colors C, Phase P>
typename MoveList<C,P>::Skewers MoveList<C,P>::skewers;
template<Colors C, Phase P>
typename MoveList<C,P>::NonCaptures MoveList<C,P>::nonCaptures;
template<Colors C, Phase P>
typename MoveList<C,P>::BadCaptures MoveList<C,P>::badCaptures;
template<Colors C, Phase P>
typename MoveList<C,P>::Evasions MoveList<C,P>::evasions;

template<Colors C, Phase P>
MoveList<C,P>::MoveList(const ColoredBoard<C>& board, Move ttMove):
    b(board),
    imove(0),
    ttMove(ttMove)
{
    if (ttMove.isValid()) {  // TODO move initialization out of constructor
        iterator = &tt;      // TODO use default constructor, pointer to b instead of reference
        currentMove = ttMove; // TODO move initialziation into retrieve in search
        p = l;
        pend = l+1;
        return; }
    tt.next(*this);
}
/*
 * Generates/Fetches the next move, which can be retrieved by dereferencing.
 * Returns false if no more moves can be generated.
 */
template<Colors C, Phase P> 
bool MoveList<C,P>::operator ++() {
    do {
        ++imove;
        if (++p == pend) { 
            iterator->next(*this);
            if (isEmpty()) return false;
        } else {
            currentMove = *p;
        }
    } while (currentMove.data == ttMove.data);
    ASSERT(!isEmpty());
    return true;
}
//template<Colors C, Phase P> 
//void MoveList<C,P>::TT::init(MoveList<C,P>&) {
//}
template<Colors C, Phase P> 
void MoveList<C,P>::TT::next(MoveList<C,P>& ml) {
    if (ml.b.template inCheck<C>()) {
        ml.evasions.init(ml);
        return; }
    if (P==vein && !ml.b.threatened) {
        ml.captures.init(ml);
        return; }
    ml.mates.init(ml);
}
template<Colors C, Phase P> 
void MoveList<C,P>::Mates::init(MoveList<C,P>& ml) {
    ml.p = ml.lend;
    ml.p3 = ml.l3;
    ml.b.template generateMateMoves<false, void>(&ml.p, &ml.p3); // mates, checks
#ifdef MYDEBUG
    if (ml.lend-ml.p > max) max = ml.lend-ml.p;
    if (ml.p3-ml.l3 > ml.checks.max) ml.checks.max = ml.p3-ml.l3;
#endif    
    if (ml.p != ml.lend) {
        ml.pend = ml.lend;
        ml.iterator = this;
        ml.currentMove = *ml.p;  
        return; }
    ml.captures.init(ml);
}
template<Colors C, Phase P> 
void MoveList<C,P>::Mates::next(MoveList<C,P>& ml) {
    ASSERT(ml.p == ml.lend);
    ml.captures.init(ml);
}
template<Colors C, Phase P> //TODO specialization for vein
void MoveList<C,P>::Captures::init(MoveList<C,P>& ml) {
    ml.p = ml.lend;
    ml.p2 = ml.l2;
    constexpr MoveType MT = (P==vein || P==leaf ? NoUnderPromo : AllMoves); 
    ml.b.template generateCaptureMoves<MT>(ml.p, ml.p2);

#ifdef MYDEBUG
    if (ml.lend-ml.p > max) max = ml.lend-ml.p;
    if (ml.p2-ml.l2 > ml.badCaptures.max) ml.badCaptures.max = ml.p2-ml.l2;
#endif
    if (ml.p != ml.lend) {
        ml.pend = ml.lend;
        ml.iterator = this;
        ml.currentMove = *ml.p; 
        return; }
    if (P==vein && !ml.b.threatened) { 
        ml.badCaptures.init(ml);
        return; }
    ml.checks.init(ml);
}
template<Colors C, Phase P> 
void MoveList<C,P>::Captures::next(MoveList<C,P>& ml) {
    ASSERT(ml.p == ml.pend);
//    if (++ml.p == ml.lend)
        if (P==vein && !ml.b.threatened)
            ml.badCaptures.init(ml);
        else
            ml.checks.init(ml);
//    else
//        ml.currentMove = *ml.p;
}
template<Colors C, Phase P> 
void MoveList<C,P>::Checks::init(MoveList<C,P>& ml) {
    if (ml.p3 != ml.l3) {
        ml.p = ml.l3;
        ml.pend = ml.p3;
        ml.iterator = this;
        ml.currentMove = *ml.p; 
        return; }
    ml.threats.init(ml);
}
template<Colors C, Phase P> 
void MoveList<C,P>::Checks::next(MoveList<C,P>& ml) {
    ASSERT(ml.p == ml.pend);
    ml.threats.init(ml);
}
template<Colors C, Phase P> 
void MoveList<C,P>::Threats::init(MoveList<C,P>& ml) {
    ml.p = ml.lend;
    ml.b.template generateDiscoveredCheck<false,void>(&ml.p); 
#ifdef MYDEBUG
    if (ml.lend-ml.p > max) max = ml.lend-ml.p;
#endif
    if (ml.p != ml.lend) {
        ml.pend = ml.lend;
        ml.iterator = this;
        ml.currentMove = *ml.p; 
        return; }
    ml.skewers.init(ml);
}
template<Colors C, Phase P> 
void MoveList<C,P>::Threats::next(MoveList<C,P>& ml) {
    ASSERT(ml.p == ml.pend);
    ml.skewers.init(ml);
}
template<Colors C, Phase P>
void MoveList<C,P>::Skewers::init(MoveList<C,P>& ml) {
    ml.p = ml.lend;
    ml.b.generateSkewers(&ml.p); 
#ifdef MYDEBUG
    if (ml.lend-ml.p > max) max = ml.lend-ml.p;
#endif
    if (ml.p != ml.lend) {
        ml.pend=ml.lend;
        ml.iterator = this;
        ml.currentMove = *ml.p; 
        return; }
    
    if (P==leaf && !ml.b.threatened) {
        ml.badCaptures.init(ml);
        return; }
    
    ml.nonCaptures.init(ml);
}
template<Colors C, Phase P>
void MoveList<C,P>::Skewers::next(MoveList<C,P>& ml) {
    ASSERT(ml.p == ml.pend);
    if (P==leaf && !ml.b.threatened)
        ml.badCaptures.init(ml);
    else
        ml.nonCaptures.init(ml);
}
template<Colors C, Phase P>
void MoveList<C,P>::NonCaptures::init(MoveList<C,P>& ml) {
    ml.p = ml.lend;
    ASSERT(ml.p2 >= ml.l2);   // p2 init in Captures, appened bad non captures
    ASSERT(ml.p2 < ml.l2end);
    ml.b.generateNonCap(ml.p, ml.p2);
#ifdef MYDEBUG
    if (ml.p2-ml.p > max) max = ml.p2-ml.p;
#endif
    Game::history.sort(ml.p, ml.lend-ml.p, ml.b.ply);
    if (ml.p != ml.p2) {
        ml.iterator = this;
        ml.pend = ml.p2;
        ml.currentMove = *ml.p; 
        return;
    }
    ml.iterator = NULL;
}
template<Colors C, Phase P>
void MoveList<C,P>::NonCaptures::next(MoveList<C,P>& ml) {
    ASSERT(ml.p == ml.pend);
    ml.iterator = NULL;
}
template<Colors C, Phase P>
void MoveList<C,P>::BadCaptures::init(MoveList<C,P>& ml) {
    if (ml.l2 != ml.p2)  {
        ml.p = ml.l2;
        ml.pend = ml.p2;
        ml.iterator = this;
        ml.currentMove = *ml.p; 
        return; }
    ml.iterator = NULL;
}
template<Colors C, Phase P>
void MoveList<C,P>::BadCaptures::next(MoveList<C,P>& ml) {
    ml.iterator = NULL;
}
template<Colors C, Phase P>
void MoveList<C,P>::Evasions::init(MoveList<C,P>& ml) {
    ml.p = ml.lend;
    ml.pend = ml.lend;
    ASSERT(ml.lend == ml.l2);
    if (P==vein && !ml.b.threatened)
        ml.b.template generateCheckEvasions<OnlyCaptures>(ml.p, ml.pend);
    else
        ml.b.template generateCheckEvasions(ml.p, ml.pend);
#ifdef MYDEBUG
    if (ml.lend-ml.p > max) max = ml.lend-ml.p;
#endif
        
    if (ml.p == ml.pend) {
        ml.iterator = NULL;
        return; }
    ml.iterator = this;
    ml.currentMove = *ml.p; 
}
template<Colors C, Phase P>
void MoveList<C,P>::Evasions::next(MoveList<C,P>& ml) {
    ml.iterator = NULL;
}
