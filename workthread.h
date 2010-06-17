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

class WorkThread: public QThread {
	std::mutex stoppedMutex;
	std::condition_variable stopped;
    volatile bool isStopped;
    
	std::condition_variable starting;
    static std::mutex runningMutex;
    static volatile unsigned int sleeping;
	volatile bool doStop;
	volatile bool keepRunning;
	volatile int result;
	BoardBase board;
	Colors color;
	Job* job;
    Key key;
	static QMultiMap<Key, Job*> jobs;
	static QVector<WorkThread*> threads;

	void stop();
    
public:
    static unsigned int nThreads;
    static __thread bool isMain;
	Stats* pstats;
	static unsigned int running;

	WorkThread();
	virtual ~WorkThread();
	
	void run();
	static void stopAll();
	void startJob(Job*);
	static void queueJob(Key, Job*);
	void setJob(Job*);
	void *operator new (size_t s);
	bool isFree();
	static bool canQueued(Key);
	static Job* getJob(Key);
    static Job* getAnyJob();
    static Job* getChildJob(Key z);
    static void idle(int);
	static WorkThread* findFree();
	static void init();
	static const QVector<WorkThread*>& getThreads();
    static void printJobs();
};

#endif /* WORKTHREAD_H_ */
