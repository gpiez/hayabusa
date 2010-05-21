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
#include <pch.h>

#include "workthread.h"
#include "jobs.h"

QMultiMap<Key,Job*> WorkThread::jobs;
QWaitCondition WorkThread::starting;
QMutex WorkThread::mutex;
unsigned int WorkThread::running = 0;
unsigned int WorkThread::nThreads = 0;
QVector<WorkThread*> WorkThread::threads;

WorkThread::WorkThread():
	doStop(false),
	keepRunning(true),
	isStopped(true),
	job(NULL)
{
	board.init();
}

WorkThread::~WorkThread()
{
}

void WorkThread::run() {
	pstats = &stats;

	mutex.lock();
	while(keepRunning) {

		isStopped = true;
		stopped.wakeAll();	//wake stop() function
		do {
			if (!doStop && jobs.count()>0 && running<4) {
				job = jobs.take(jobs.keys().first());
			}
			if (!job) starting.wait(&mutex);//starting is signalled by StartJob() or QJob()
		} while(!job);
		isStopped = false;
		doStop = false;
		running++;
		mutex.unlock();
//        qDebug() << "execute" << (const void*)job;
        job->job();             //returning the result must have happened here.
		mutex.lock();
		delete job;
		job = NULL;
		running--;
	}
	mutex.unlock();
}

void WorkThread::stop() {
	QMutexLocker lock(&stoppedMutex);
	while (!isStopped) {
		doStop = true;
		stopped.wait(&stoppedMutex);	//waits until the end of the run loop is reached.
	}
}

void WorkThread::stopAll() {
	QMutexLocker lock(&mutex);
	foreach(Job* j, jobs)
		delete j;
	jobs.clear();
	foreach(WorkThread* th, threads)
		th->stop();
}

void WorkThread::startJob(Job *j) { // todo only used at root now, insert findfree() here
	ASSERT(j);
//	qDebug() << "startJob" << j;
	QMutexLocker lock(&mutex);
	delete job;
	job = j;
	starting.wakeAll();
}

// Queue a job from a parent. This allows the parent later to start only its
// own jobs.
void WorkThread::queueJob(Key parent, Job *j) {
	ASSERT(j);
//	qDebug() << "queueJob" << j;
	QMutexLocker lock(&mutex);
	jobs.insertMulti(parent, j);
	starting.wakeAll();
}

Job* WorkThread::getJob(Key parent) {
	QMutexLocker lock(&mutex);
	return jobs.take(parent);
}

void WorkThread::setJob(Job* j) {
	ASSERT(j);
	delete job;
	job = j;
}

void* WorkThread::operator new (size_t s) {
	void *p;
	posix_memalign(&p, CACHE_LINE_SIZE, s);
	return p;
}

bool WorkThread::isFree() {
	bool ret = isStopped;
	isStopped = false;		// return true only once
	return ret;
}

bool WorkThread::canQueued() {
	QMutexLocker lock(&mutex);
	return (unsigned)jobs.count() < nThreads-1;
}

void WorkThread::idle(int /*n*/) {
//  TODO decreasing the number of running threads should only allow to start new
//  jobs in child nodes, otherwise a unrelated node may be started and not
//  finished, while join() continues, which increases the number of running and
//  executed threads beyond the number of cores
//	QMutexLocker lock(&mutex);
//	running -= n;
//	if (n<0) starting.wakeAll();
//	ASSERT(running <= 8);
}

void WorkThread::init() {
	nThreads = sysconf(_SC_NPROCESSORS_ONLN);
	if (nThreads == 0) nThreads = 1;

	for (unsigned int i=0; i<nThreads; ++i) {
		WorkThread* th = new WorkThread();
		th->start();
		threads.append(th);
	}
}

WorkThread* WorkThread::findFree() {
	QMutexLocker lock(&mutex);
	foreach(WorkThread* th, WorkThread::threads)
		if (th->isFree())
			return th;
	return 0;
}

const QVector<WorkThread*>& WorkThread::getThreads() {
	return threads;
}