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

#ifdef __linux__
#include <sys/prctl.h>
#endif
#include "workthread.h"
#include "jobs.h"

QMultiMap<Key,Job*> WorkThread::jobs;
//std::condition_variable WorkThread::starting;
std::mutex WorkThread::runningMutex;
unsigned int WorkThread::running = 0;
volatile unsigned int WorkThread::sleeping = 0;
unsigned int WorkThread::nThreads = 0;
QVector<WorkThread*> WorkThread::threads;
__thread bool WorkThread::isMain = false;
volatile bool WorkThread::doStop = false;

WorkThread::WorkThread():
    isStopped(true),
    keepRunning(true),
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
#ifdef __linux__    
    std::stringstream name;
    name << "WorkThread " << threads.indexOf(this);
    prctl(PR_SET_NAME, name.str().c_str() );
#endif    
    while(keepRunning) {

        isStopped = true;
        if (doStop) stopped.notify_one();    //wake stop() function
        do {
            if (!doStop && jobs.count()>0 && running<4) {
                if (sleeping) {
                    QMultiMap<Key, Job*>::iterator i = jobs.upperBound(key);
                    if (i != jobs.begin()) {
                        --i;
//                        std::cerr << (key >> 56) << ":";
                        key = i.key();
//                        std::cerr << (key >> 56) << std::endl;
                        job = jobs.take(key);
                    }
                } else {
                    key = jobs.keys().first();
                    job = jobs.take(key);
//                    std::cerr << "deque " << i << " " <<(void*)job << " count " << jobs.count() << std::endl;
                }
            }
            if (!job) starting.wait(lock);//starting is signalled by StartJob() or QJob()
            stats.jobs++;
        } while(!job);
        isStopped = false;
        doStop = false;
        running++;
//        std::cerr << "start " << i << " " << (void*)job << " count " << jobs.count() << std::endl;
        runningMutex.unlock();
        job->job();             //returning the result must have happened here.
        runningMutex.lock();
//        std::cerr << "end   " << i << " " << (void*)job << " count " << jobs.count() << std::endl;
        delete job;
        job = NULL;
        running--;
    }
}

// Queue a job from a parent. This allows the parent later to start only its
// own jobs.
void WorkThread::queueJob(Key parent, Job *j) {
    ASSERT(j);
    runningMutex.lock();
/*    for (int i = 0; i<threads.count(); ++i) {
        WorkThread* th = threads[i];*/
    foreach (WorkThread* th, threads) {
        if (th->isStopped) {
            ASSERT(th->job == NULL);
            th->isStopped = false;
            th->job = j;
//            std::cerr << "notfy " << i << " " << (void*)j << " count " << jobs.count() << std::endl;
            runningMutex.unlock();
            th->starting.notify_one();
            return;
        }
    }

//    std::cerr << "queue " << (void*)j << " count " << jobs.count() << std::endl;
    jobs.insertMulti(parent, j);
    runningMutex.unlock();
//    starting.notify_all();
}

void WorkThread::idle(int n) {
//  TODO decreasing the number of running threads should only allow to start new
//  jobs in child nodes, otherwise a unrelated node may be started and not
//  finished, while join() continues, which increases the number of running and
//  executed threads beyond the number of cores
    std::unique_lock<std::mutex> lock(runningMutex);
    running -= n;
    sleeping += n;
    if (n>0 && running < nThreads && jobs.count() > 0)
        foreach (WorkThread* th, threads)
            if (th->isStopped) {
                th->starting.notify_one();
                break;
            }
    ASSERT(running <= 8);
}

void WorkThread::stop() {
    std::unique_lock<std::mutex> lock(stoppedMutex);
    while (!isStopped) {
        doStop = true;
        stopped.wait(lock);    //waits until the end of the run loop is reached.
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

Job* WorkThread::getChildJob(Key z) {
    std::unique_lock<std::mutex> lock(runningMutex);
    QMultiMap<Key, Job*>::iterator i = jobs.upperBound(z-0x100000000000000);
    if (i != jobs.begin()) {
        return jobs.take((--i).key());
    } else
        return 0;
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
//    return (unsigned)jobs.size() < nThreads;
}

void WorkThread::printJobs() {
    for (QMultiMap<Key, Job*>::iterator i=jobs.begin(); i!=jobs.end(); ++i) {
        std::cout << i.key() << ": " << i.value() << std::endl;
    }
}

void WorkThread::init() {
    nThreads = sysconf(_SC_NPROCESSORS_ONLN);
    if (nThreads == 0) nThreads = 1;

    for (unsigned int i=0; i<nThreads*2-1; ++i) {
        WorkThread* th = new WorkThread();
        new std::thread(&WorkThread::run, th);
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
