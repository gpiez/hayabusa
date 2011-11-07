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

#include <random>
#include "parameters.h"
#include "stringlist.h"

unsigned Parameters::maxIndex = 0;
bool Parameters::parmsLocked = false;
std::mt19937 mt;

std::map< std::string, unsigned > Parameters::index;
std::vector<Parm<float> > Parameters::base;

Parameters defaultParameters;

void Parameters::mutate(float mutationRate)
{
    for(unsigned i = 0; i < maxIndex; ++i) {
        if (std::bernoulli_distribution(mutationRate)(mt)) {
            std::normal_distribution<float> ndist(parms[i].value, parms[i].var);
            parms[i].value = ndist(mt);
            parms[i].value = std::min(parms[i].value, parms[i].max);
            parms[i].value = std::max(parms[i].value, parms[i].min);
        }
    }
}

Parameters Parameters::combine(const Parameters& father) const
{
    Parameters ret = *this;
    for (unsigned i=0; i<maxIndex; ++i) {
        ret.parms[i].value = (parms[i].value + father.parms[i].value) / 2;
    }
    return ret;
}

Parameters::Parameters() {}

Parameters::Parameters(const Parameters& p)
{
#ifdef MYDEBUG
    parmsLocked = true;
#endif
    parms.resize(maxIndex);
    for (unsigned i=0; i<maxIndex; ++i) {
        parms[i] = p.parms[i];
    }
}

void Parameters::add(std::string s, float value)
{
    s = toLower(s);
#ifdef MYDEBUG
    if (parmsLocked) {
        std::cerr << "Parameter list modified after created" << std::endl;
    }
#endif    
    if (index.find(s) == index.end()) {
        index[s] = maxIndex++;
        base.resize(maxIndex);
    }
    base[ index[s] ] = Parm<float>(value);
}

void Parameters::add(std::string s, float value, float var, float max, float min)
{
    s = toLower(s);
#ifdef MYDEBUG
    if (parmsLocked) {
        std::cerr << "Parameter list modified after created" << std::endl;
    }
#endif
    if (index.find(s) == index.end()) {
        index[s] = maxIndex++;
        base.resize(maxIndex);
    }
    base[ index[s] ] = Parm<float>(value, var, max, min);
}

void Parameters::init()
{
    Parameters::add("knight.mobility.opening", 24);
    Parameters::add("knight.mobility.endgame", 18);
    Parameters::add("bishop.mobility.opening", 12);
    Parameters::add("bishop.mobility.endgame", 0);
    Parameters::add("rook.mobility.opening", 16);
    Parameters::add("rook.mobility.endgame", 0);
    Parameters::add("queen.mobility.endgame", 0);
    Parameters::add("queen.mobility.opening", 0);

    Parameters::add("pawn.corner.opening", -4);
    Parameters::add("knight.corner.opening", 0);
    Parameters::add("rook.corner.opening", 0);
    Parameters::add("queen.corner.opening", 0);
    Parameters::add("bishop.corner.opening", 0);
    Parameters::add("king.corner.opening", 0);
    Parameters::add("pawn.corner.endgame", 0);
    Parameters::add("knight.corner.endgame", -16);
    Parameters::add("rook.corner.endgame", 0);
    Parameters::add("queen.corner.endgame", 0);
    Parameters::add("bishop.corner.endgame", -8);
    Parameters::add("king.corner.endgame", 0);

    Parameters::add("pawn.value.opening", 96 , 10, 150);
    Parameters::add("pawn.value.endgame", 106, 10, 150);
    Parameters::add("pawn.hor.value.opening", 8);
    Parameters::add("pawn.vert.value.opening", 12);
    Parameters::add("pawn.hor.value.endgame", -8);
    Parameters::add("pawn.vert.value.endgame", 14);
    Parameters::add("pawn.hor.inflection.opening", 1.5);
    Parameters::add("pawn.vert.inflection.opening", 6);
    Parameters::add("pawn.vcenter.opening", 6);
    Parameters::add("pawn.hor.inflection.endgame", 1.5);
    Parameters::add("pawn.vert.inflection.endgame", 1.5);

    Parameters::add("knight.value.opening", 312, 20, 400);
    Parameters::add("knight.value.endgame", 288, 20, 400);

    Parameters::add("bishop.value.opening", 292, 20, 400);
    Parameters::add("bishop.value.endgame", 300, 20, 400);
    
    Parameters::add("rook.value.opening", 480, 30, 600);
    Parameters::add("rook.value.endgame", 480, 30, 600);
    Parameters::add("rook.horizontal.value.opening", 14);
    Parameters::add("rook.vertical.value.opening", 12);
    Parameters::add("rook.horizontal.inflection.opening", 1.2);
    Parameters::add("rook.vertical.inflection.opening", 1.2);
    
    Parameters::add("queen.value.opening", 1000, 100, 1200);
    Parameters::add("queen.value.endgame", 890, 100, 1200);
    Parameters::add("queen.horizontal.value.opening", 7);
    Parameters::add("queen.vertical.value.opening", 5);
    Parameters::add("queen.horizontal.inflection.opening", 1.0);
    Parameters::add("queen.vertical.inflection.opening", 0.5);
    Parameters::add("queen.mobility.endgame", 4);

    Parameters::add("king.horizontal.value.opening", -20);
    Parameters::add("king.vertical.value.opening", -24);
    Parameters::add("king.horizontal.value.endgame", -4);
    Parameters::add("king.vertical.value.endgame", 4);
    
    Parameters::add("kingHValue", -20);
    Parameters::add("kingVValue", -24);
    Parameters::add("kingHEValue", -4);
    Parameters::add("kingVEValue", 4);
    
    Parameters::add("rookHValue", 14);
    Parameters::add("rookVValue", 12);
    Parameters::add("rookHInfl", 1.2);
    Parameters::add("rookVInfl", 1.2);
    Parameters::add("rookHEValue", 0);
    Parameters::add("rookVEValue", 0);
    Parameters::add("rookHEInfl", 1.5);
    Parameters::add("rookVEInfl", 1.5);
    Parameters::add("rookPair", 0);
    
    Parameters::add("queenHValue", 7);
    Parameters::add("queenVValue", 5);
    Parameters::add("queenHInfl", 1.0);
    Parameters::add("queenVInfl", 0.5);
    Parameters::add("queenPair", 0);
    
    Parameters::add("knightHValue", 10);
    Parameters::add("knightVValue", 32);
    Parameters::add("knightHEValue", 2);
    Parameters::add("knightVEValue", 2);
    Parameters::add("knightHEInfl", 0.5);
    Parameters::add("knightVEInfl", 0);
    Parameters::add("knightPair", -30);
    Parameters::add("bishopHValue", 12);
    Parameters::add("bishopVValue", 22);
    Parameters::add("bishopHEValue", 0);
    Parameters::add("bishopVEValue", 0);
    Parameters::add("bishopCenter", 3.5);
    Parameters::add("knight.mobslope", 3.0);
    Parameters::add("bishop.mobslope", 5.0);
    Parameters::add("rook.mobslope", 6.0);
    Parameters::add("queen.mobslope", 10.0);

    Parameters::add("queenHEValue", 4);
    Parameters::add("queenVEValue", 4);
    Parameters::add("pawnPasser2", 0);
    Parameters::add("pawnPasser7", 150);
    Parameters::add("pawnPasserSlope", 1.0);
    Parameters::add("knightBlockPasser", 10);
    Parameters::add("bishopBlockPasser", 15);
    Parameters::add("bishopPair", 28);
    Parameters::add("kingHEInfl", 1.0);
    Parameters::add("kingVEInfl", 1.5);
    Parameters::add("kingHInfl", 2);
    Parameters::add("kingVInfl", 1);

    Parameters::add("endgameMaterial", 32);

//     Parameters::add("bishopAlone", -100);

//     Parameters::add("knightAlone", -125);

    Parameters::add("rookTrapped", -50);
    Parameters::add("rookOpen", 16);
    Parameters::add("rookHalfOpen", 7);
    Parameters::add("rookWeakPawn", 24);

    Parameters::add("pawnBackward", -1);
    Parameters::add("pawnBackwardOpen", -10);
    Parameters::add("pawnIsolatedCenter", 0);
    Parameters::add("pawnIsolatedEdge", 0);
    Parameters::add("pawnIsolatedOpen", -6);
    Parameters::add("pawnDouble", -25);
    Parameters::add("pawnShoulder", 1);

    Parameters::add("deltaAttack", 190);
    Parameters::add("deltaDefense", -75);








    Parameters::add("knightHInfl", 0.5);
    Parameters::add("knightVInfl", 0.75);
    Parameters::add("knightCenter", 4.0);

    Parameters::add("bishopHInfl", 0.6875);
    Parameters::add("bishopVInfl", 1.34375);

    Parameters::add("rookCenter", 3.5);


    Parameters::add("kingCenter", 3.5);

    Parameters::add("queenCenter", 3.5);
    
    Parameters::add("bishopHEInfl", 1.5);
    Parameters::add("bishopVEInfl", 1.5);

    Parameters::add("queenHEInfl", 1.5);
    Parameters::add("queenVEInfl", 1.5);

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
    Parameters::add("dMaxExt", 6);
    Parameters::add("dMinDualExt", 2);
    Parameters::add("dMinSingleExt", 6);
    Parameters::add("dMinForkExt", 4);
    Parameters::add("dMinMateExt", 4);
    Parameters::add("dMinPawnExt", 4);
    Parameters::add("flags", 3);

    Parameters::add("oppKingOwnPawnV", 14);  // only 1..7 used
    Parameters::add("ownKingOwnPawnV", 8);
    Parameters::add("oppKingOwnPasserV",  26);  // only 1..7 used
    Parameters::add("ownKingOwnPasserV", 32);
    Parameters::add("pawnConnPasserV", 36 );

    Parameters::add("calcMeanError", 1);

    Parameters::add("prune1", 10);
    Parameters::add("prune2", 110);
    Parameters::add("prune1c", 60);
    Parameters::add("prune2c", 0);
    Parameters::add("dNullIncr", 0x2a, 0, 0xfffffff, 0);
    Parameters::add("dVerifyIncr", 0x2a, 0, 0xfffffff, 0);
    Parameters::add("dMinReduction", 2, 0, 10, 1);

    Parameters::add("pawnAttack", 112);
    Parameters::add("bishopAttack", 144);
    Parameters::add("knightAttack", 96);
    Parameters::add("kingAttack", 96);
    Parameters::add("rookAttack", 160);
    Parameters::add("queenAttack", 192);

    Parameters::add("attQMaMi", 200);
    Parameters::add("attQMiMi", 180);
    Parameters::add("attMaMiMi", 160);
    Parameters::add("attQMa", 80);
    Parameters::add("attQMi", 70);
    Parameters::add("attMaMi", 40);
    Parameters::add("attMiMi", 30);
    Parameters::add("attQ", 20);
    Parameters::add("attMa",15);
    Parameters::add("attMi",12);

    Parameters::add("dRed[7]", 2);
    Parameters::add("dRed[6]", 2);
    Parameters::add("dRed[5]", 2);
    Parameters::add("dRed[4]", 3);
    Parameters::add("dRed[3]", 6);
    Parameters::add("dRed[2]", 16);
    Parameters::add("dRed[1]", 255);
    Parameters::add("dRed[0]", 255);

    Parameters::add("bishopOwnPawn", 0);
    Parameters::add("bishopOppPawn", 0);

    Parameters::add("bishopNotOwnPawn", 2);
    Parameters::add("bishopNotOppPawn", 1);

    Parameters::add("dRedCapture", 5);
    Parameters::add("dRedCheck", 9);

    Parameters::add("dMaxExtCheck", 3);
    Parameters::add("dMaxExtPawn", 0);
    Parameters::add("dMinExtDisco", 0);

    Parameters::add("aspirationLow", 40);
    Parameters::add("aspirationHigh", 5);
    Parameters::add("aspirationHigh2", 20);
    Parameters::add("tempo", 10);

    Parameters::add("kingShield.center[0]", 25);
    Parameters::add("kingShield.center[1]", 20);
    Parameters::add("kingShield.center[2]", 12);
    Parameters::add("kingShield.outer[0]",  24);
    Parameters::add("kingShield.outer[1]",  20);
    Parameters::add("kingShield.outer[2]",  16);
    Parameters::add("kingShield.inner[0]",  26);
    Parameters::add("kingShield.inner[1]",  19);
    Parameters::add("kingShield.inner[2]",  15);
    Parameters::add("kingShield.openFile",   -25);
    Parameters::add("kingShield.halfOpenFile",-25);

    Parameters::add("pawnShield", 100);
    Parameters::add("pieceDefense", 100);
    Parameters::add("pieceAttack", 100);

    Parameters::add("attackTotal", 200);

    Parameters::add("rook.attack", 50);
    Parameters::add("bishop.attack", 40);
    Parameters::add("knight.attack", 40);
    Parameters::add("queen.attack", 120);
    Parameters::add("pawn.attack", 25);
    Parameters::add("king.attack", 40);
    Parameters::add("rook.defense", 10);
    Parameters::add("bishop.defense", 15);
    Parameters::add("knight.defense", 15);
    Parameters::add("queen.defense", 5);

    Parameters::add("castlingTempo", 5);
    
    defaultParameters.parms.resize(maxIndex);
    for (unsigned i=0; i<maxIndex; ++i) 
        defaultParameters.parms[i] = Parm<float>(base[i]);
}

Parm<float> Parameters::operator [] (std::string s) const {
    s = toLower(s);
    ASSERT(index.find(s) != index.end());
    ASSERT(index.find(s)->second < maxIndex);
    if (!index.count(s)) {
        std::cerr << "unknown paramter " << s << std::endl;
    }
//         std::cerr << index.find(s)->second << std::endl;
//         std::cerr << parms.size() << std::endl;
//         std::cerr << maxIndex << std::endl;
    return parms.at(index.find(s)->second);
}

Parm<float>& Parameters::operator [] (std::string s) {
    s = toLower(s);
    std::map< std::string, unsigned >::iterator i = index.find(s);
    if (i == index.end()) {
        std::cerr << "unknown parameter" << std::endl;
    }
    return parms[i->second];
}

bool Parameters::exists(const std::string s)
{
    return index.find(s) != index.end();
}

Parameters::~Parameters()
{

}
