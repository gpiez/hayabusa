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
	QMutex stoppedMutex;
	QWaitCondition stopped;
	static QWaitCondition starting;
	static QMutex mutex;
	volatile bool doStop;
	volatile bool keepRunning;
	volatile bool isStopped;
	volatile int result;
	BoardBase board;
	Colors color;
	Job* job;
	static QMultiMap<Key, Job*> jobs;
	static QVector<WorkThread*> threads;
	static unsigned int nThreads;

	void stop();
    
public:
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
	static bool canQueued();
	static Job* getJob(Key);
	static void idle(int);
	static WorkThread* findFree();
	static void init();
	static const QVector<WorkThread*>& getThreads();
};

#endif /* WORKTHREAD_H_ */
