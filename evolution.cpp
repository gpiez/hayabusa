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

#include <future>
#include <algorithm>
#include <fstream>
#include "evolution.h"
#include "selfgame.h"
#include "options.h"

using namespace std;

unsigned Evolution::nThread;
unsigned Evolution::maxThread;
std::mutex Evolution::nThreadMutex;
std::condition_variable Evolution::nThreadCond;

Evolution::Evolution(Console* c):
    console(c)
{
}

void Evolution::init()
{
    initFixed(1024);
}

void Evolution::initFixed(int n)
{
    nThread = 0;
    maxThread = 8;
    nIndiFixed = n;
    srand(1);
    recalc.resize(n);
    minElo.resize(n);
    maxElo.resize(n);
    firstGame.resize(n);
    for (int i=0; i<nIndiFixed; ++i) {
        Parameters adam(defaultParameters);
        adam.mutate(0.5);
        indiFixed.push_back(adam);
    }
    
}

void Evolution::parmTest(std::string pname, float min, float max, int n) {
    Parameters adam(defaultParameters);
    indi.clear();
    nIndi = n;
    for (int i=0; i<nIndi; ++i) {
        recalc[i] = true;
        adam[pname] = n == 1 ? min : (max-min)*i/(n-1.0) + min;
        indi.push_back(adam);
        indi[i].score = 0;
    }

    results.resize(nIndi);
    for (int i=0; i<nIndi; ++i)
        results[i].resize(nIndiFixed);

    for (currentNGames=1; currentNGames<=nIndiFixed; currentNGames<<=1) {
        step();
        if (findmax() < 2) break;
    }

    cerr << pname << endl;
    for (int i=0; i<nIndi; ++i) {
        cerr << setw(5) << indi[i].parm[pname].value << ": " << setw(5) << minElo[i] << "-" << setw(5) << maxElo[i] << endl;
    }
}

void Evolution::parmTest(std::string pname, float min, float max, int n, std::string pname2, float min2, float max2, int n2) {
    Parameters adam(defaultParameters);
    indi.clear();
    nIndi = n*n2;
    for (int i=0; i<n; ++i) {
        adam[pname] = (max-min)*i/(n-1.0) + min;
        for (int j=0; j<n2; ++j) {
            adam[pname2] = (max2-min2)*j/(n2-1.0) + min2;
            indi.push_back(adam);
        }
    }

    results.resize(indi.size());
    for (unsigned i=0; i<results.size(); ++i)
        results[i].resize(indi.size());

    step();
    cerr << pname2 << endl << "    ,";
    for (int j=0; j<n2; ++j)
        cerr << setw(3) << indi[j].parm[pname2].value << ",";
    cerr << endl << pname << endl;
    for (int i=0; i<n; ++i) {
        cerr << setw(3) << indi[i*n2].parm[pname].value << ",";
        for (int j=0; j<n2; ++j) {
            cerr << setw(3) << indi[i*n2+j].score << ",";
        }
        cerr << endl;
    }
}

void Evolution::parmTest(std::string pname, float min, float max, int n, std::string pname2, float min2, float max2, int n2, std::string pname3, float min3, float max3, int n3) {
    Parameters adam(defaultParameters);
    indi.clear();
    nIndi = n*n2*n3;
    for (int i=0; i<n; ++i) {
        adam[pname] = (max-min)*i/(n-1.0) + min;
        for (int j=0; j<n2; ++j) {
            adam[pname2] = (max2-min2)*j/(n2-1.0) + min2;
            for (int k=0; k<n3; ++k) {
                adam[pname3] = (max3-min3)*k/(n3-1.0) + min3;
                indi.push_back(adam);
            }
        }
    }

    results.resize(indi.size());
    for (unsigned i=0; i<results.size(); ++i)
        results[i].resize(indi.size());

    step();
   
    for (int k=0; k<n3; ++k) {
        cerr << pname3 << ": " << indi[k].parm[pname3].value << "  ";
        cerr << pname2 << endl << "    ,";
        for (int j=0; j<n2; ++j)
            cerr << setw(3) << indi[j*n3].parm[pname2].value << ",";
        cerr << endl << pname << endl;
        for (int i=0; i<n; ++i) {
            cerr << setw(3) << indi[i*n2*n3].parm[pname].value << ",";
            for (int j=0; j<n2; ++j) {
                cerr << setw(3) << indi[i*n2*n3+j*n3+k].score << ",";
            }
            cerr << endl;
        }
        cerr << "------------------" << endl;
    }
}

int Evolution::game(const Evolution::Individual& a, const Evolution::Individual& b) const {
    int result;
    {
        SelfGame sg(console, a.parm, b.parm);
        result = sg.tournament();
    }
    {
        lock_guard<mutex> lock(nThreadMutex);
        --nThread;
    }
    nThreadCond.notify_one();
    return result;
}

void Evolution::tournament() {
    for (int i=0; i<nIndi; ++i)
    if (recalc[i])
    for (int j=firstGame[i]; j<currentNGames; ++j) {
        {
            unique_lock<mutex> lock(nThreadMutex);
            while (nThread >= maxThread)
                nThreadCond.wait(lock);
            ++nThread;
        }
        results[i][j] = async(launch::async,&Evolution::game, this, indi[i], indiFixed[j]);
    }
}

void Evolution::step()
{
    thread th(&Evolution::tournament, this);
    th.detach();
    
    for (int i=0; i<nIndi; ++i)
    if (recalc[i]) {
        cerr << setw(4) << i << ": " << flush;
        for (int j=firstGame[i]; j<currentNGames; ++j) {
            if (results[i][j].valid()) {
                int result = results[i][j].get();
                indi[i].score += result;
                double n = j+1.0;
                double nGames = 110.0;  // from SelfGame
                double p = indi[i].score / (n*nGames*2.0) +0.5;
                double v = sqrt(p*(1-p)*(n*nGames))*2.0 *3.0;
                double pe = v / (n*nGames*2.0);
                double elo = -log10(1.0/p - 1.0)*400.0;
                double elol = -log10(1.0/(p-pe) - 1.0)*400.0;
                minElo[i] = elol;
                double eloh = -log10(1.0/(p+pe) - 1.0)*400.0;
                maxElo[i] = eloh;
                double eloe = (eloh - elol)/2;
                cerr << setprecision(0) << std::fixed << "\015" << setw(4) << i << ": " << "Abs Score: " << setw(5) << indi[i].score << "/" << setw(5) << n << "  Rel Score: " << setprecision(4) << setw(6) << p << " ±" << setw(6) << pe << "  Elo: " <<  setprecision(1) << setw(5) << elo << " ±" << setw(5) << eloe << "      " << flush;
            } else {
                --j;
                usleep(100000);
            }
        }
        cerr << endl;
        firstGame[i] = currentNGames;
        recalc[i] = false;
    } else {
                double n = firstGame[i];
                double nGames = 110.0;  // from SelfGame
                double p = indi[i].score / (n*nGames*2.0) +0.5;
                double v = sqrt(p*(1-p)*(n*nGames))*2.0 *3.0;
                double pe = v / (n*nGames*2.0);
                double elo = -log10(1.0/p - 1.0)*400.0;
                double elol = -log10(1.0/(p-pe) - 1.0)*400.0;
                double eloh = -log10(1.0/(p+pe) - 1.0)*400.0;
                double eloe = (eloh - elol)/2;
                cerr << setprecision(0) << std::fixed << setw(4) << i << ": " << "Abs Score: " << setw(5) << indi[i].score << "/" << setw(5) << n << "  Rel Score: " << setprecision(4) << setw(6) << p << " ±" << setw(6) << pe << "  Elo: " <<  setprecision(1) << setw(5) << elo << " ±" << setw(5) << eloe << "      " << endl;
        
    }
}

int Evolution::findmax()
{
    double maxelo = -400.0;
    int bestIndi;
    for (int i=0; i<nIndi; ++i) {
        if ((minElo[i]+maxElo[i])/2 > maxelo) {
            maxelo = (minElo[i]+maxElo[i])/2;
            bestIndi = i;
        }
    }
    int nMax=0;
    for (int i=0; i<nIndi; ++i) {
        if (maxElo[i] > maxelo) {
            recalc[i] = true;
            nMax++;
        }
    }
    return nMax;
}

void Evolution::selection()
{
    sort(indi.begin()+1, indi.end());
    for (unsigned i=0; i<results.size(); ++i) {
        cerr << setw(4) << i << ": ";
        cerr << setw(5) << indi[i].score << endl;
    }
    for (unsigned i=(indi.size()+1)/2; i<indi.size(); ++i) {
        Individual father = indi[(i & ~1) -indi.size()/2];
        Individual mother = indi[(i & ~1) -indi.size()/2 +1];
        indi[i].parm = mother.parm.combine(father.parm);
        indi[i].parm.mutate();
    }
}

void Evolution::evolve()
{
    for (int generation=0; ; ++generation) {
        step();
        cerr << endl << "Generation " << generation << endl;
        selection();
        saveState(generation);
    }
}

void Evolution::saveState(int g)
{
    cerr << "Evolution::saveState(int g)" << endl;
    ofstream file("evo.txt", ios_base::app);
    file << "Generation " << g << endl;
    for (unsigned i=0; i<indi.size(); ++i) {
        file << "Individual" << setw(4) << i << " Score" << setw(4) << indi[i].score << endl;
#if 0        
        for (map< string, unsigned >::const_iterator j=Parameters::index.begin(); j!=Parameters::index.end(); ++j) {
            file << "Parameter[\"" << j->first << "\"] = " << indi[i].parm.parms[ j->second ].value << ";" << endl;
        }
#endif        
        file << endl;
    }
    file << endl;
}

Evolution::~Evolution()
{

}