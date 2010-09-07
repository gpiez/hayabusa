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
#ifndef RESULT_H
#define RESULT_H

#ifndef PCH_H_
#include <pch.h>
#endif

template< typename T > class Result
{
    T value;
    volatile unsigned int notReady;
    std::mutex valueMutex;
    std::condition_variable readyCond;
    std::mutex readyMutex;

    Result();
    Result(const Result&);

public:
    explicit Result(T x):
        notReady(0)
    {
        value = x;
    }
    operator T () {
        std::unique_lock<std::mutex> lock(readyMutex);
        while (notReady)
            readyCond.wait(lock);
        return value;
    }

    void update(Result<T>& data) {
        T dataValue = data;
        std::unique_lock<std::mutex> lock(valueMutex);
        value += dataValue;
    }

    void update(T data) {
        std::unique_lock<std::mutex> lock(valueMutex);
        value += data;
    }

    void setReady() {
        readyMutex.lock();
        --notReady;
        readyCond.notify_one();    // if the wakeup happens after the unlocking, this Result
        readyMutex.unlock();    // may be destroyed after the signaling the condition
    }                           // but before .wakeOne() is completly done.

    void setNotReady() {
        readyMutex.lock();
        ++notReady;
        readyMutex.unlock();
    }

};

#endif // RESULT_H
