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
    Parameters::init();
}

void Evolution::init()
{
    nThread = 0;
    maxThread = 8;
    Options::quiet = true;
    Parameters adam;
    indi.push_back(adam);
    nIndi = 16;
    for (int i=1; i<nIndi; ++i) {
        Parameters eva(adam);
        eva.mutate();
        indi.push_back(eva);
    }

    results.resize(indi.size());
    for (unsigned i=0; i<results.size(); ++i)
        results[i].resize(indi.size());

}

void Evolution::parmTest(std::string pname, float min, float max, int n) {
    nThread = 0;
    maxThread = 8;
    Options::quiet = true;
    Parameters adam;
    nIndi = n;
    for (int i=0; i<nIndi; ++i) {
        adam[pname] = (max-min)*i/(n-1.0) + min;
        indi.push_back(adam);
    }

    results.resize(indi.size());
    for (unsigned i=0; i<results.size(); ++i)
        results[i].resize(indi.size());

    step();
    for (int i=0; i<nIndi; ++i) {
        cerr << indi[i].parm[pname].value << "," << indi[i].score << endl;
    }
}

void Evolution::parmTest(std::string pname, float min, float max, int n, std::string pname2, float min2, float max2, int n2) {
    nThread = 0;
    maxThread = 8;
    Options::quiet = true;
    Parameters adam;
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
    for (unsigned i=0; i<results.size()-1; ++i)
        for (unsigned j=i+1; j<results[i].size(); ++j) {
            {
                unique_lock<mutex> lock(nThreadMutex);
                while (nThread >= maxThread) {
                    nThreadCond.wait(lock);
                }
                ++nThread;
            }
//             lock_guard<mutex> lock(resultsMutex);
            results[i][j] = async(launch::async,&Evolution::game, this, indi[i], indi[j]);
        }    
}

void Evolution::step()
{
    thread th(&Evolution::tournament, this);
    th.detach();
    
    for (unsigned i=0; i<results.size(); ++i)
        indi[i].score = 0;
    
    for (unsigned i=0; i<results.size(); ++i) {
        cerr << setw(4) << i << ": " << flush;
        for (unsigned j=0; j<i+1; ++j) 
            cerr << "    ";
//         resultsMutex.lock();
        for (unsigned j=i+1; j<results[i].size(); ++j) {
            if (results[i][j].valid()) {
                int result = results[i][j].get();
                indi[i].score += result;
                indi[j].score -= result;
                cerr << setw(4) << result << flush;
            } else {
//                 resultsMutex.unlock();
                --j;
                usleep(10000);
//                 resultsMutex.lock();
            }
        }
//         resultsMutex.unlock();
        cerr << setw(5) << indi[i].score << endl;
    }
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
        for (map< string, unsigned >::const_iterator j=Parameters::index.begin(); j!=Parameters::index.end(); ++j) {
            file << "Parameter[\"" << j->first << "\"] = " << indi[i].parm.parms[ j->second ].value << ";" << endl;
        }
        file << endl;
    }
    file << endl;
}
