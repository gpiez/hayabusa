/*
    hayabusa, chess engine
    Copyright (C) 2009-2010 Gunther Piez

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
#ifndef WORKTHREAD_H_
#define WORKTHREAD_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "coloredboard.h"
class Job;
class RootBoard;
union Stats;

extern __thread bool isMain;

class WorkThread {
    static std::multimap<unsigned, Job*> jobs;
    static std::vector<WorkThread*> threads;
    
    static Mutex runningMutex;
    Condition starting;        //locked by shared runningMutex
    Mutex stoppedMutex;
    Condition stopped;
    static volatile unsigned int waiting;
    static unsigned logWorkThreads;
    static unsigned nWorkThreads;
    static __thread unsigned reserved;
    
    volatile bool isStopped;

    volatile bool keepRunning;
    volatile int result;
    BoardBase board;
    Colors color;
    Job* job;
    Key key;
    Stats* stats;
    unsigned* pThreadId;
    void stop();

public:
    static volatile bool doStop;
    static unsigned int nThreads;
    static unsigned int running;
    static __thread unsigned threadId;
    unsigned int parent;

    WorkThread();
    virtual ~WorkThread();
    void run();
    const Stats* getStats() const {
        return stats;
    }
    Stats* getStats() {
        return stats;
    }
    unsigned& getThreadId() {
        return *pThreadId;
    }
    static unsigned findFreeChild(unsigned parent);
    static void stopAll();
    static void queueJob(unsigned, Job*);
    static bool canQueued(unsigned, int);
    static Job* getJob(unsigned, unsigned);
    static Job* findJob(unsigned, unsigned);
//     Job* findGoodJob(unsigned& parent);
    static void idle(int);
    static WorkThread* findFree();
    static void init();
    static const std::vector<WorkThread*>& getThreads();
    static void printJobs();
    static void clearStats();
    static unsigned getReserved(unsigned);
    static void reserve(unsigned);
};

#endif /* WORKTHREAD_H_ */
