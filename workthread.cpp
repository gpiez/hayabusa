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
std::condition_variable WorkThread::starting;
std::mutex WorkThread::runningMutex;
unsigned int WorkThread::running = 0;
volatile unsigned int WorkThread::sleeping = 0;
unsigned int WorkThread::nThreads = 0;
QVector<WorkThread*> WorkThread::threads;
__thread bool WorkThread::isMain = false;

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

    std::unique_lock<std::mutex> lock(runningMutex);
    while(keepRunning) {

        isStopped = true;
        stopped.wakeAll();    //wake stop() function
        do {
            if (!doStop && jobs.count()>0 && running<4) {
/*                if (sleeping) {
                    QMultiMap<Key, Job*>::iterator i = jobs.upperBound(key);
                    if (i != jobs.begin()) {
                        --i;
                        key = i.key();
                        job = jobs.take(key);
                    }
                } else {*/
                    key = jobs.keys().first();
                    job = jobs.take(key);
//                }
            }
            if (!job) starting.wait(lock);//starting is signalled by StartJob() or QJob()
        } while(!job);
        isStopped = false;
        doStop = false;
        running++;
        runningMutex.unlock();
//        qDebug() << "execute" << (const void*)job;
        job->job();             //returning the result must have happened here.
        runningMutex.lock();
        delete job;
        job = NULL;
        running--;
    }
}

void WorkThread::stop() {
    QMutexLocker lock(&stoppedMutex);
    while (!isStopped) {
        doStop = true;
        stopped.wait(&stoppedMutex);    //waits until the end of the run loop is reached.
    }
}

void WorkThread::stopAll() {
    std::unique_lock<std::mutex> lock(runningMutex);
    foreach(Job* j, jobs)
        delete j;
    jobs.clear();
    foreach(WorkThread* th, threads)
        th->stop();
}

void WorkThread::startJob(Job *j) { // todo only used at root now, insert findfree() here
    ASSERT(j);
//    qDebug() << "startJob" << j;
    std::unique_lock<std::mutex> lock(runningMutex);
    delete job;
    job = j;
    starting.notify_all();
}

// Queue a job from a parent. This allows the parent later to start only its
// own jobs.
void WorkThread::queueJob(Key parent, Job *j) {
    ASSERT(j);
//    qDebug() << "queueJob" << j;
    std::unique_lock<std::mutex> lock(runningMutex);
    jobs.insertMulti(parent, j);
    starting.notify_all();
}

Job* WorkThread::getJob(Key parent) {
    std::unique_lock<std::mutex> lock(runningMutex);
    return jobs.take(parent);
}

Job* WorkThread::getAnyJob() {
    std::unique_lock<std::mutex> lock(runningMutex);
    const QList<Key> keys = jobs.keys();
    if (keys.isEmpty())
        return NULL;
    else
        return jobs.take(keys.first());
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
    isStopped = false;        // return true only once
    return ret;
}

bool WorkThread::canQueued(Key k) {
    std::unique_lock<std::mutex> lock(runningMutex);
    return (unsigned)jobs.count(k) < nThreads;
}

void WorkThread::printJobs() {
    for (QMultiMap<Key, Job*>::iterator i=jobs.begin(); i!=jobs.end(); ++i) {
        std::cout << i.key() << ": " << i.value() << std::endl;
    }
}

void WorkThread::idle(int n) {
//  TODO decreasing the number of running threads should only allow to start new
//  jobs in child nodes, otherwise a unrelated node may be started and not
//  finished, while join() continues, which increases the number of running and
//  executed threads beyond the number of cores
    std::unique_lock<std::mutex> lock(runningMutex);
    running -= n;
    sleeping += n;
    if (n<0) starting.notify_all();
    ASSERT(running <= 8);
}

void WorkThread::init() {
    nThreads = sysconf(_SC_NPROCESSORS_ONLN);
    if (nThreads == 0) nThreads = 1;

    for (unsigned int i=0; i<nThreads+1; ++i) {
        WorkThread* th = new WorkThread();
        th->start();
        threads.append(th);
    }
}

WorkThread* WorkThread::findFree() {
    std::unique_lock<std::mutex> lock(runningMutex);
    foreach(WorkThread* th, WorkThread::threads)
        if (th->isFree())
            return th;
    return 0;
}

const QVector<WorkThread*>& WorkThread::getThreads() {
    return threads;
}
