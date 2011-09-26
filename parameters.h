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
    Parameters();
    Parameters(const Parameters&);
    static void init();
    static std::map< std::string, unsigned > index;
    static std::vector<float> base;

    std::vector< Parm<float> > parms;

    Parm<float> operator [] (const std::string s) const {
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

    Parm<float>& operator [] (const std::string s) {
        
        std::map< std::string, unsigned >::iterator i = index.find(s);
        if (i == index.end()) {
            std::cerr << "unknown parameter" << std::endl;
        }
        return parms[i->second];
    }

    static void add(const std::string s, float value);
    void mutate();
    Parameters combine(const Parameters& father) const;
};

extern Parameters defaultParameters;

#endif // PARAMETERS_H
