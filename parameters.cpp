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
    Parameters::add("PawnValue", 80);
    Parameters::add("KnightValue", 270);
    Parameters::add("BishopValue", 290);
    Parameters::add("RookValue", 480);
    Parameters::add("QueenValue", 900);

    Parameters::add("endgameMaterial", 32);

    Parameters::add("bishopPair", 30);
    Parameters::add("bishopBlockPasser", 20);
    Parameters::add("bishopAlone", -100);

    Parameters::add("knightAlone", -125);
    Parameters::add("knightBlockPasser", 30);

    Parameters::add("rookTrapped", -50);
    Parameters::add("rookOpen", 16);
    Parameters::add("rookHalfOpen", 4);
    Parameters::add("rookWeakPawn", 16);

    Parameters::add("pawnBackward", -4);
    Parameters::add("pawnBackwardOpen", -8);
    Parameters::add("pawnIsolatedCenter", -4);
    Parameters::add("pawnIsolatedEdge", -2);// - Eval::pawnBackwardOpen);
    Parameters::add("pawnIsolatedOpen", -8);// - Eval::pawnBackwardOpen);
    Parameters::add("pawnDouble", -18);
    Parameters::add("pawnShoulder", 4);

    Parameters::add("deltaAttack", 190);
    Parameters::add("deltaDefense", -75);

    Parameters::add("pawnPasser2", 25);
    Parameters::add("pawnPasser7", 100);
    Parameters::add("pawnPasserSlope", 1.0);

    Parameters::add("mobN1value", 10);
    Parameters::add("mobN1slope", 3.0);

    Parameters::add("mobN2value", 25);
    Parameters::add("mobN2slope", 8.0);

    defaultParameters.parms.resize(maxIndex);
    for (unsigned i=0; i<maxIndex; ++i) 
        defaultParameters.parms[i] = Parm<float>(base[i]);
}
