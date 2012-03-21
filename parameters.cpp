/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Gunther Piez <gpiez@web.de>

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

#include "parameters.h"
#include "stringlist.h"
#include <random>

unsigned Parameters::maxIndex = 0;
bool Parameters::parmsLocked = false;
std::mt19937 mt;

std::map< std::string, unsigned > Parameters::index;
std::vector<Parm<float> > Parameters::base;

Parameters defaultParameters;

void Parameters::mutate(float mutationRate) {
    for(unsigned i = 0; i < maxIndex; ++i) {
        if (std::bernoulli_distribution(mutationRate)(mt)) {
            std::normal_distribution<float> ndist(base[i].value, base[i].var);
            parms[i].value = ndist(mt);
            parms[i].value = std::min(parms[i].value, parms[i].max);
            parms[i].value = std::max(parms[i].value, parms[i].min);
//             if (base[i].value != parms[i].value)
//             for (auto j=Parameters::index.begin(); j!=Parameters::index.end(); ++j) {
//                 if (j->second == i) {
//                     std::cout << "Parameter " << std::setw(20) << j->first << ":" << std::setw(5) <<  base[i].value << "->" << std::setw(5) << parms[i].value << std::endl;
//                 }
//             }
        } } }

Parameters Parameters::combine(const Parameters& father) const {
    Parameters ret = *this;
    for (unsigned i=0; i<maxIndex; ++i) {
        ret.parms[i].value = (parms[i].value + father.parms[i].value) / 2; }
    return ret; }

Parameters::Parameters() {}

Parameters::Parameters(const Parameters& p) {
#ifdef MYDEBUG
    parmsLocked = true;
#endif
    parms.resize(maxIndex);
    for (unsigned i=0; i<maxIndex; ++i) {
        parms[i] = p.parms[i]; } }

void Parameters::add(std::string s, float value) {
    s = toLower(s);
#ifdef MYDEBUG
    if (parmsLocked) {
        std::cerr << "Parameter list modified after created" << std::endl; }
#endif
    if (index.find(s) == index.end()) {
        index[s] = maxIndex++;
        base.resize(maxIndex); }
    base[ index[s] ] = Parm<float>(value); }

void Parameters::add(std::string s, float opening, float endgame) {
    s = toLower(s);
#ifdef MYDEBUG
    if (parmsLocked) {
        std::cerr << "Parameter list modified after created" << std::endl; }
#endif
    std::string so = s + ".opening";
    std::string se = s + ".endgame";
    ASSERT(index.find(so) == index.end());
    ASSERT(index.find(se) == index.end());
    base.resize(maxIndex+2);
    base[ index[so] = maxIndex++ ] = Parm<float>(opening);
    base[ index[se] = maxIndex++ ] = Parm<float>(endgame); }

void Parameters::add(std::string s, float value, float var, float max, float min) {
    s = toLower(s);
#ifdef MYDEBUG
    if (parmsLocked) {
        std::cerr << "Parameter list modified after created" << std::endl; }
#endif
    if (index.find(s) == index.end()) {
        index[s] = maxIndex++;
        base.resize(maxIndex); }
    base[ index[s] ] = Parm<float>(value, var, max, min); }

void Parameters::init() {
    Parameters::add("bishop.attack", 60);
    Parameters::add("bishop.corner.endgame", -8);
    Parameters::add("bishop.corner.opening", 0);
    Parameters::add("bishop.defense", 15);
    Parameters::add("bishop.hor.inflection.endgame", 1.5);
    Parameters::add("bishop.hor.inflection.opening", 0.6875);
    Parameters::add("bishop.hor.value.endgame", 0);
    Parameters::add("bishop.hor.value.opening", 12);
    Parameters::add("bishop.mobility.endgame", 10);
    Parameters::add("bishop.mobility.opening", 30);
    Parameters::add("bishop.mobslope.endgame", 5.0);
    Parameters::add("bishop.mobslope.opening", 5.0);
    Parameters::add("bishop.value.endgame", 300, 20, 400);
    Parameters::add("bishop.value.opening", 310, 20, 400);
    Parameters::add("bishop.vcenter.endgame", 3.5);
    Parameters::add("bishop.vcenter.opening", 4.0);
    Parameters::add("bishop.vert.inflection.endgame", 1.5);
    Parameters::add("bishop.vert.inflection.opening", 1.34375);
    Parameters::add("bishop.vert.value.endgame", 0);
    Parameters::add("bishop.vert.value.opening", 22);
    Parameters::add("king.attack", 40);
    Parameters::add("king.corner.endgame", 3);                      //20120312
    Parameters::add("king.corner.opening", 0);
    Parameters::add("king.hor.inflection.endgame", 1.0);            
    Parameters::add("king.hor.inflection.opening", 2);
    Parameters::add("king.hor.value.endgame", -2);                  //20120309
    Parameters::add("king.hor.value.opening", -20);
    Parameters::add("king.vcenter.endgame", 3.5);
    Parameters::add("king.vcenter.opening", 3.5);
    Parameters::add("king.vert.inflection.endgame", 1.5);
    Parameters::add("king.vert.inflection.opening", 1);
    Parameters::add("king.vert.value.endgame", 4);                  //20120309
    Parameters::add("king.vert.value.opening", -24);
    Parameters::add("knight.attack", 40);
    Parameters::add("knight.corner.endgame", -12);                  //20120312
    Parameters::add("knight.corner.opening", 0);
    Parameters::add("knight.defense", 15);
    Parameters::add("knight.hor.inflection.endgame", 0.5);
    Parameters::add("knight.hor.inflection.opening", 0.5);
    Parameters::add("knight.hor.value.endgame", 0);                 //20120313
    Parameters::add("knight.hor.value.opening", 10);
    Parameters::add("knight.mobility.endgame", 6);
    Parameters::add("knight.mobility.opening", 30);
    Parameters::add("knight.mobslope.endgame", 3.0);
    Parameters::add("knight.mobslope.opening", 3.0);
    Parameters::add("knight.value.endgame", 288, 20, 400);
    Parameters::add("knight.value.opening", 290, 20, 400);
    Parameters::add("knight.vcenter.endgame", 4.0);
    Parameters::add("knight.vcenter.opening", 5.0);
    Parameters::add("knight.vert.inflection.endgame", 0);
    Parameters::add("knight.vert.inflection.opening", 0.75);
    Parameters::add("knight.vert.value.endgame", 2);
    Parameters::add("knight.vert.value.opening", 32);
    Parameters::add("pawn.attack", 30);
    Parameters::add("pawn.corner.endgame", 0);
    Parameters::add("pawn.corner.opening", -4);
    Parameters::add("pawn.hor.inflection.endgame", 1.5);
    Parameters::add("pawn.hor.inflection.opening", 1.5);
    Parameters::add("pawn.hor.value.endgame", -8);
    Parameters::add("pawn.hor.value.opening", 8);
    Parameters::add("pawn.value.endgame", 100, 10, 150);
    Parameters::add("pawn.value.opening", 90 , 10, 150);
    Parameters::add("pawn.vcenter.endgame", 6);
    Parameters::add("pawn.vcenter.opening", 6);
    Parameters::add("pawn.vert.inflection.endgame", 1.5);
    Parameters::add("pawn.vert.inflection.opening", 6);
    Parameters::add("pawn.vert.value.endgame", 14);
    Parameters::add("pawn.vert.value.opening", 12);
    Parameters::add("queen.attack", 160);
    Parameters::add("queen.corner.endgame", 0);
    Parameters::add("queen.corner.opening", 0);
    Parameters::add("queen.defense", 5);
    Parameters::add("queen.hor.inflection.endgame", 1.5);
    Parameters::add("queen.hor.inflection.opening", 1.0);
    Parameters::add("queen.hor.value.endgame", 4);
    Parameters::add("queen.hor.value.opening", 7);
    Parameters::add("queen.mobility.endgame", 4);
    Parameters::add("queen.mobility.opening", 0);
    Parameters::add("queen.mobslope.endgame", 10.0);
    Parameters::add("queen.mobslope.opening", 10.0);
    Parameters::add("queen.value.endgame", 890, 100, 1200);
    Parameters::add("queen.value.opening", 1000, 100, 1200);
    Parameters::add("queen.vcenter.endgame", 3.5);
    Parameters::add("queen.vcenter.opening", 3.5);
    Parameters::add("queen.vert.inflection.endgame", 1.5);
    Parameters::add("queen.vert.inflection.opening", 0.5);
    Parameters::add("queen.vert.value.endgame", 4);
    Parameters::add("queen.vert.value.opening", 5);
    Parameters::add("rook.attack", 110);
    Parameters::add("rook.corner.endgame", 0);
    Parameters::add("rook.corner.opening", 0);
    Parameters::add("rook.defense", 10);
    Parameters::add("rook.hor.inflection.endgame", 1.5);
    Parameters::add("rook.hor.inflection.opening", 1.2);
    Parameters::add("rook.hor.value.endgame", -6);
    Parameters::add("rook.hor.value.opening", 14);
    Parameters::add("rook.mobility.endgame", 4);
    Parameters::add("rook.mobility.opening", 16);
    Parameters::add("rook.mobslope.endgame", 6.0);
    Parameters::add("rook.mobslope.opening", 6.0);
    Parameters::add("rook.value.endgame", 480, 30, 600);
    Parameters::add("rook.value.opening", 480, 30, 600);
    Parameters::add("rook.vcenter.endgame", 3.5);
    Parameters::add("rook.vcenter.opening", 3.5);
    Parameters::add("rook.vert.inflection.endgame", 1.5);
    Parameters::add("rook.vert.inflection.opening", 1.2);
    Parameters::add("rook.vert.value.endgame", 0);
    Parameters::add("rook.vert.value.opening", 12);

    Parameters::add("knightBlockPasser", 10, 10);
    Parameters::add("bishopBlockPasser", 15, 15);
    Parameters::add("bishopPair", 28, 28);
    Parameters::add("bishopOwnPawn", 0, -2);
    Parameters::add("bishopOppPawn", 0, -8);
    Parameters::add("bishopNotOwnPawn", 2, 5);
    Parameters::add("bishopNotOppPawn", 1, 6);

    Parameters::add("endgameMaterial", 32);

    Parameters::add("rookTrapped", -50, -50);
    Parameters::add("rookOpen2", 16, 16);
    Parameters::add("rookOpenSlope", -8, -10);
    Parameters::add("rookHalfOpen2", 6, 8);
    Parameters::add("rookHalfOpenSlope", -4, -4);
    Parameters::add("rookWeakPawn",  6, 8);
    Parameters::add("rookOwnPasser", 5, 10);
    Parameters::add("rookOppPasser", 5, 12);

    Parameters::add("pawnBackward", -1, -1);
    Parameters::add("pawnBackwardOpen", -10, -10);
    Parameters::add("pawnIsolatedCenter", -8, -12);
    Parameters::add("pawnIsolatedEdge", 0, 0);
    Parameters::add("pawnIsolatedOpen", -4, -6);
    Parameters::add("pawnDouble", -25, -30);
    Parameters::add("pawnShoulder", 1, 1);
    Parameters::add("pawnPasser2", 0, 0);
    Parameters::add("pawnPasser7", 150, 150);
    Parameters::add("pawnPasserSlope", 1.0, 1.0);
    Parameters::add("pawnConnPasser", 36, 36 );
    Parameters::add("pawnCandidate", 12, 12);
    Parameters::add("pawnPiece", -40, -40);

    Parameters::add("dMaxCapture", 8, 4, 20, 2);
    Parameters::add("standardError", 30);
    Parameters::add("standardSigma", 2.0);
// odd extensions tend to blow up the tree. Average tree size over 2600 positions:
// ext  size, error margin ~5k
// 13   660k
// 12   541k
// 11   635k
// 10   522k
//  9   616k
//  8   505k
//  7   598k
//  6   490k
//  5   582k
//  4   475k
//  3   562k
//  2   462k
//  1   539k
//  0   427k
    Parameters::add("dMaxCheckExt", 7);
    Parameters::add("dMinDualExt", 2);
    Parameters::add("dMinSingleExt", 6);
    Parameters::add("dMinForkExt", 4);
    Parameters::add("dMinMateExt", 4);
    Parameters::add("dMinPawnExt", 4);

    Parameters::add("oppKingOwnPawn2", 20);  // only 1..7 used
    Parameters::add("ownKingOwnPawn2", 24);
    Parameters::add("oppKingOwnPasser2", 40);  // only 1..7 used
    Parameters::add("ownKingOwnPasser2", 32);
    Parameters::add("kingPawnRankFactor", 4.0);
    Parameters::add("kingPawnRankSlope", 2.0);
    Parameters::add("kingPawnDistSlope", 1.5);

    Parameters::add("prune1", 60);
    Parameters::add("prune2", 140);
    Parameters::add("prune3", 950);
    Parameters::add("prune1c", 60);
    Parameters::add("prune2c", 0);
    Parameters::add("prune3c", 0);
    Parameters::add("dNullIncr", 0b10000100001010, 0, 0xffffff, 0);
    Parameters::add("dVerifyIncr", 0b10000100001010, 0, 0xffffff, 0);
    Parameters::add("dMinReduction", 2, 0, 10, 1);

    Parameters::add("dRed[7]", 2);
    Parameters::add("dRed[6]", 2);
    Parameters::add("dRed[5]", 2);
    Parameters::add("dRed[4]", 3);
    Parameters::add("dRed[3]", 6);
    Parameters::add("dRed[2]", 16);
    Parameters::add("dRed[1]", 255);
    Parameters::add("dRed[0]", 255);


    Parameters::add("dRedCapture", 5);
    Parameters::add("dRedCheck", 20);

    Parameters::add("dMaxExtCheck", 6);
//    Parameters::add("dMaxExtPawn", 0);
//    Parameters::add("dMinExtDisco", 0);

    Parameters::add("aspirationLow", 40);
    Parameters::add("aspirationHigh", 5);
    Parameters::add("aspirationHigh2", 20);
    Parameters::add("tempo0", -4);
    Parameters::add("tempo64", 40);

    // sum of shield pawns is capped at 127, so the 3*(base+vdelta) should be
    // smaller than 127
    Parameters::add("kingShield.base", 20);
    Parameters::add("kingShield.vdelta", 0.625);
    Parameters::add("kingShield.idelta", 0.8125);
    Parameters::add("kingShield.odelta", 0.875);

    Parameters::add("kingShieldOpenFile",   -10);
    Parameters::add("kingShieldHalfOpenFile",-25);

    Parameters::add("pawnDefense", 384);
    Parameters::add("pieceDefense", 150);
    Parameters::add("pieceAttack", 125);
    Parameters::add("attackFirst", 75);
    Parameters::add("attackSlope", 1.0);
    Parameters::add("flags", 15);
    Parameters::add("attackTotal", 200);

    Parameters::add("castlingTempo", 5);
    Parameters::add("noEvalLimit", 5);
//    Parameters::add("matFlag", 0xff);
    Parameters::add("allowLazyRoot", 1);
    Parameters::add("endgameTransitionSlope", 36);
    Parameters::add("rookSeventh", 40, 20);
    Parameters::add("rookSeventh2", 0, 0);
    Parameters::add("sortPrev", 0);
    Parameters::add("sortNext", 128);
    Parameters::add("quant", 0);
    defaultParameters.parms.resize(maxIndex);
    for (unsigned i=0; i<maxIndex; ++i)
        defaultParameters.parms[i] = Parm<float>(base[i]);

#ifdef MYDEBUG
    std::multimap<float, std::string> sortedList;
    for (auto i=index.begin(); i!=index.end(); ++i)
        sortedList.insert ( std::pair<float, std::string>( fabs(base[i->second].value) , i->first ));

    for (auto i=sortedList.rbegin(); i!=sortedList.rend(); ++i)
        std::cout << std::setw(40) << std::left << i->second << i->first << std::endl;

#endif
}

Parm<float> Parameters::operator [] (std::string s) const {
    s = toLower(s);
    ASSERT(index.find(s) != index.end());
    ASSERT(index.find(s)->second < maxIndex);
    if (!index.count(s)) {
        std::cerr << "unknown paramter " << s << std::endl; }
//         std::cerr << index.find(s)->second << std::endl;
//         std::cerr << parms.size() << std::endl;
//         std::cerr << maxIndex << std::endl;
    return parms.at(index.find(s)->second); }

Parm<float>& Parameters::operator [] (std::string s) {
    s = toLower(s);
    std::map< std::string, unsigned >::iterator i = index.find(s);
    if (i == index.end()) {
        std::cerr << "unknown parameter" << std::endl; }
    return parms[i->second]; }

bool Parameters::exists(const std::string s) {
    return index.find(s) != index.end(); }

Parameters::~Parameters() {

}
