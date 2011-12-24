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


#ifndef EVOLUTION_H
#define EVOLUTION_H

#include <vector>
#include <future>
#include "parameters.h"

class Console;

class Evolution
{
    struct Individual {
        Parameters parm;
        int     score;
        bool operator < (const Individual& b) const {
            return b.score < score;
        }
        Individual(const Parameters& parm): parm(parm) {};
    };

    Console* console;
    static unsigned nThread;
    static unsigned maxThread;
    static std::mutex nThreadMutex;
    static std::condition_variable nThreadCond;
    std::string nodes;
    void setCurrentParameters(Parameters& p);
public:
    Evolution(Console*);
    ~Evolution();
    void init();
    void initFixed(int);
    void step();
    void selection();
    void mutate();
    void saveState(int);
    void evolve();
    void tournament();
    int findmax();
    void parmTest(std::string, float, float, int, const std::string&);
    void parmTest(std::string pname, float min, float max, int n, std::string pname2, float min2, float max2, int n2);
    void parmTest(std::string pname, float min, float max, int n, std::string pname2, float min2, float max2, int n2, std::string pname3, float min3, float max3, int n3);
    int game(const Individual& a, const Individual& b) const;

    int nIndi;
    int nIndiFixed;

    int currentNGames;
    
    std::vector< std::string > startPositions;

    std::vector< std::vector< std::future<int> > > results;
    std::mutex resultsMutex;
    
    std::vector<Individual> indi;
    std::vector<Individual> indiFixed;

    std::vector<int> firstGame;
    std::vector<double> minElo;
    std::vector<double> maxElo;
    std::vector<bool> recalc;

};

#endif // EVOLUTION_H
