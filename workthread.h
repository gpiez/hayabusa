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

#include "constants.h"
class Job;
class Game;
union Stats;

extern __thread bool isMain;

class WorkThread {
    static std::multimap<unsigned, Job*> jobs;
    static std::vector<WorkThread*> threads;

    static Mutex runningMutex;      // locked only for job list manipulations
    Condition starting;             // wait until this thread has a job to do
    static Condition stopped;       // wait until no thread is running
    static unsigned int waiting;
    static unsigned logWorkThreads;
    static unsigned nWorkThreads;
    static __thread unsigned reserved;
    static unsigned int running;
    static unsigned int nThreads;
    static __thread unsigned threadId;
    unsigned int parent;
//    Key key;
    unsigned* pThreadId;
    Stats* pStats;
    Job* job;
    
    void stop();
    unsigned& getThreadId() {
        return *pThreadId; }
    void run();
    static Job* findJob(unsigned, unsigned);

public:
    struct Unlock {
        Unlock() { runningMutex.unlock(); }
        ~Unlock() { runningMutex.lock(); }
    };
    static __thread Stats stats;

    WorkThread();
    virtual ~WorkThread();
    static unsigned getThisThreadId() { return threadId; }
    const Stats* getStats() const { return pStats; }
    Stats* getStats() { return pStats; }
    static unsigned findFreeChild(unsigned parent);
    static void stopAll();
    static void queueJob(Job*);
    static bool canQueued( int);
    static void idle(int);
    static WorkThread* findFree();
    static void init();
    static const std::vector<WorkThread*>& getThreads();
    static void printJobs();
    static void clearStats();
    static unsigned getReserved(unsigned);
    static void executeOldJobs(unsigned ply);
    static void reserve(unsigned); };

#endif /* WORKTHREAD_H_ */
