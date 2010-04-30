/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

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
	QMutex valueMutex;
	QWaitCondition readyCond;
	QMutex readyMutex;

	Result() {}
	Result(const Result&);
	
public:
	Result(T x):
		notReady(0)
	{
		value = x;
	}
	
	operator T () {
		readyMutex.lock();
		while (notReady)
			readyCond.wait(&readyMutex);
		readyMutex.unlock();
		return value;
	}

	void update(Result<T>& data) {
		T dataValue = data;
		QMutexLocker locker(&valueMutex);
		value += dataValue;
	}

	void update(T data) {
		QMutexLocker locker(&valueMutex);
		value += data;
	}

	void setReady() {
		readyMutex.lock();
		--notReady;
		readyMutex.unlock();
		readyCond.wakeOne();

	}

	void setNotReady() {
		readyMutex.lock();
		++notReady;
		readyMutex.unlock();
	}

};

#endif // RESULT_H
