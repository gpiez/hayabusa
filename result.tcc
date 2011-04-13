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
#ifndef RESULT_TCC_
#define RESULT_TCC_

#include "result.h"

template<typename T>
Result<T>::Result(T x):
    notReady(0)
{
    value = x;
}
template<typename T>
Result<T>::operator T () {
    UniqueLock<Mutex> lock(readyMutex);
    while (notReady)
        readyCond.wait(lock);
    return value;
}

template<typename T>
void Result<T>::update(Result<T>& data) {
    T dataValue = data;
    UniqueLock<Mutex> lock(valueMutex);
    value += dataValue;
}

template<typename T>
void Result<T>::update(T data) {
    UniqueLock<Mutex> lock(valueMutex);
    value += data;
}

template<typename T>
void Result<T>::setReady() {
    readyMutex.lock();
    --notReady;
    readyCond.notify_one();    // if the wakeup happens after the unlocking, this Result
    readyMutex.unlock();    // may be destroyed after the signaling the condition
}                           // but before .wakeOne() is completly done.

template<typename T>
void Result<T>::setNotReady() {
    readyMutex.lock();
    ++notReady;
    readyMutex.unlock();
}

#endif // RESULT_TCC_
