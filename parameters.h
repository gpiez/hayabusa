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


#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <string>
#include <map>
#include <vector>
#include "constants.h"

template<typename T>
struct Parm {
    T value;
    T min;
    T max;
    T var;

    Parm(T value, T var, T min, T max):
        value(value),
        min(min),
        max(max),
        var(var)
    {}

    Parm(T value, T var):
        value(value),
        min(0),
        max(2*value),
        var(var)
    {}
    
    Parm(T value):
        value(value),
        min(0),
        max(2*value),
        var(value/2)
    {}

    Parm() {}

//     void operator = (T value)
};

class Parameters
{
    static unsigned maxIndex;
    static bool parmsLocked;
    
public:
    struct Piece {
        struct Phase {
            float opening;
            float endgame;
        };
        struct Direction {
            Phase val;
            Phase infl;
        };
//         CompoundScore h[4];
//         CompoundScore v[8];
        Phase val;
        Direction hor, vert;
        float center;
    };
    Parameters();
    Parameters(const Parameters&);
    ~Parameters();
    static void init();
    static std::map< std::string, unsigned > index;
    static std::vector<float> base;

    std::vector< Parm<float> > parms;

    Parm<float> operator [] (const std::string s) const;
    Parm<float>& operator [] (const std::string s);
    static void add(const std::string s, float value);
    void mutate();
    Parameters combine(const Parameters& father) const;
};

extern Parameters defaultParameters;

#endif // PARAMETERS_H
