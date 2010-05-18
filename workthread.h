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
	QWaitCondition startable, starting;
	QMutex* mutex;
	volatile bool doStop;
	volatile bool keepRunning;
	volatile bool isStopped;
	volatile int result;
	BoardBase board;
	Colors color;
	Job* job;
	
public:
	Stats* pstats;

	WorkThread();
	virtual ~WorkThread();

	void run();
	void stop();
	void end();
	void startJob(Job* = NULL);
	void setJob(Job*);
	void *operator new (size_t s);
	bool isFree();
};

#endif /* WORKTHREAD_H_ */
