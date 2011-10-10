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

unsigned Parameters::maxIndex = 0;
bool Parameters::parmsLocked = false;
std::mt19937 mt;

std::map< std::string, unsigned > Parameters::index;
std::vector<float> Parameters::base;

Parameters defaultParameters;

void Parameters::mutate()
{
    float mutationRate = 0.05;
    for(unsigned i = 0; i < maxIndex; ++i) {
        if (std::bernoulli_distribution(mutationRate)(mt)) {
            std::normal_distribution<float> ndist(parms[i].value, parms[i].var/2);
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

void Parameters::add(const std::string s, float value)
{
#ifdef MYDEBUG    
    if (parmsLocked) {
        std::cerr << "Parameter list modified after created" << std::endl;
    }
#endif    
    if (index.find(s) == index.end()) {
        index[s] = maxIndex++;
        base.resize(maxIndex);
    }
    base[ index[s] ] = value;
}

void Parameters::init()
{
    Parameters::add("pawnOpening", 96);
    Parameters::add("pawnEndgame", 106);
    Parameters::add("knightOpening", 304);
    Parameters::add("knightEndgame", 296);
    Parameters::add("bishopOpening", 292);
    Parameters::add("bishopEndgame", 300);
    Parameters::add("rookOpening", 480);
    Parameters::add("rookEndgame", 480);
    Parameters::add("queenOpening", 1000);
    Parameters::add("queenEndgame", 890);
    
    Parameters::add("pawnHValue", 8);
    Parameters::add("pawnVValue", 12);
    Parameters::add("pawnHEValue", -8);
    Parameters::add("pawnVEValue", 14);
    Parameters::add("kingHValue", -20);
    Parameters::add("kingVValue", -24);
    Parameters::add("kingHEValue", -4);
    Parameters::add("kingVEValue", 4);
    Parameters::add("rookHValue", 14);
    Parameters::add("rookVValue", 12);
    Parameters::add("rookHEValue", 0);
    Parameters::add("rookVEValue", 0);
    Parameters::add("queenHValue", 7);
    Parameters::add("queenVValue", 5);
    Parameters::add("knightHValue", 10);
    Parameters::add("knightVValue", 32);
    Parameters::add("knightHEValue", 2);
    Parameters::add("knightVEValue", 2);
    Parameters::add("bishopHValue", 12);
    Parameters::add("bishopVValue", 22);
    Parameters::add("bishopHEValue", 0);
    Parameters::add("bishopVEValue", 0);
    Parameters::add("bishopCenter", 3.5);
    Parameters::add("mobB1value", 6.5);
    Parameters::add("mobB1slope", 6.0);
    Parameters::add("mobN1value", 2);
    Parameters::add("mobN1slope", 3.0);
    Parameters::add("mobR1value", 2);
    Parameters::add("mobR1slope", 3.0);

    Parameters::add("mobB2value", 1.5);
    Parameters::add("mobB2slope", 5.0);
    Parameters::add("mobR2value", 0);
    Parameters::add("mobR2slope", 8.0);
    Parameters::add("mobN2value", 6);
    Parameters::add("mobN2slope", 3.0);
    Parameters::add("queenHEValue", 4);
    Parameters::add("queenVEValue", 4);
    Parameters::add("pawnPasser2", 0);
    Parameters::add("pawnPasser7", 150);
    Parameters::add("pawnPasserSlope", 1.0);
    Parameters::add("knightBlockPasser", 10);
    Parameters::add("bishopBlockPasser", 15);

    Parameters::add("endgameMaterial", 32);

    Parameters::add("bishopPair", 30);
    Parameters::add("bishopAlone", -100);

    Parameters::add("knightAlone", -125);

    Parameters::add("rookTrapped", -50);
    Parameters::add("rookOpen", 16);
    Parameters::add("rookHalfOpen", 7);
    Parameters::add("rookWeakPawn", 24);

    Parameters::add("pawnBackward", -1);
    Parameters::add("pawnBackwardOpen", -10);
    Parameters::add("pawnIsolatedCenter", 0);
    Parameters::add("pawnIsolatedEdge", 0);
    Parameters::add("pawnIsolatedOpen", -6);
    Parameters::add("pawnDouble", -12);
    Parameters::add("pawnShoulder", 1);

    Parameters::add("deltaAttack", 190);
    Parameters::add("deltaDefense", -75);








    Parameters::add("knightHInfl", 0.5);
    Parameters::add("knightVInfl", 0.75);
    Parameters::add("knightCenter", 4.0);

    Parameters::add("bishopHInfl", 0.6875);
    Parameters::add("bishopVInfl", 1.34375);

    Parameters::add("rookHInfl", 1.5);
    Parameters::add("rookVInfl", 1.5);
    Parameters::add("rookCenter", 3.5);

    Parameters::add("pawnHInfl", 1.5);
    Parameters::add("pawnVInfl", 6);
    Parameters::add("pawnCenter", 6);

    Parameters::add("kingHInfl", 1.5);
    Parameters::add("kingVInfl", 1.5);
    Parameters::add("kingCenter", 3.5);

    Parameters::add("queenHInfl", 1.5);
    Parameters::add("queenVInfl", 1.5);
    Parameters::add("queenCenter", 3.5);
    
    Parameters::add("knightHEInfl", 1.5);
    Parameters::add("knightVEInfl", 1.5);

    Parameters::add("bishopHEInfl", 1.5);
    Parameters::add("bishopVEInfl", 1.5);

    Parameters::add("rookHEInfl", 1.5);
    Parameters::add("rookVEInfl", 1.5);

    Parameters::add("pawnHEInfl", 1.5);
    Parameters::add("pawnVEInfl", 1.5);

    Parameters::add("kingHEInfl", 1.5);
    Parameters::add("kingVEInfl", 1.5);

    Parameters::add("queenHEInfl", 1.5);
    Parameters::add("queenVEInfl", 1.5);

    Parameters::add("dMaxCapture", 7);
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
    Parameters::add("dMaxExt", 4);
    Parameters::add("dMinDualExt", 3);
    Parameters::add("dMinSingleExt", 5);
    Parameters::add("dMinForkExt", 3);
    Parameters::add("dMinMateExt", 5);
    Parameters::add("dMinPawnExt", 3);
    Parameters::add("dEvenAlpha", 0);

    defaultParameters.parms.resize(maxIndex);
    for (unsigned i=0; i<maxIndex; ++i) 
        defaultParameters.parms[i] = Parm<float>(base[i]);
}

Parm<float> Parameters::operator [] (const std::string s) const {
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

Parm<float>& Parameters::operator [] (const std::string s) {

    std::map< std::string, unsigned >::iterator i = index.find(s);
    if (i == index.end()) {
        std::cerr << "unknown parameter" << std::endl;
    }
    return parms[i->second];
}

Parameters::~Parameters()
{

}
