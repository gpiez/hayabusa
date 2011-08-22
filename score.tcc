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
#include "score.h"
#include "workthread.h"

template<Colors C> Score<C>::Score (const Score& a) { v = a.v; }

template<Colors C> Score<C>::Score (int a) { v = a; }

template<Colors C> bool Score<C>::operator >= (int a) const {
    if ( C==White ) return v>=a;
    else            return v<=a;
}

template<Colors C> bool Score<C>::operator <= (int a) const {
    if ( C==White ) return v<=a;
    else            return v>=a;
}

template<Colors C> bool Score<C>::operator > (int a) const {
    if ( C==White ) return v>a;
    else            return v<a;
}

template<Colors C> bool Score<C>::operator < (int a) const {
    if ( C==White ) return v<a;
    else            return v>a;
}

template<Colors C> bool Score<C>::max(const int b) {
    if (*this < b) {
        v = b;
        return true;
    } 
    return false;
}

template<Colors C> std::string Score<C>::str(int v) {
    if (v == 0)
        return " = 0";

    std::stringstream s;
    int sign = v/abs(v);
    if (abs(v) < 50)
        s << (sign > 0 ? " ⩲":" ⩱");
    else if (abs(v) < 200)
        s << (sign > 0 ? " ±":" ∓");
    else if (abs(v) < infinity)
        s << (sign > 0 ? "+-":"-+");
    else {
        s << (sign > 0 ? "+M":"-M");
        s << abs(v)-infinity;
        return s.str();
    }

    s << std::fixed << std::setw(5) << std::setprecision(2) << abs(v)/100.0;
    return s.str();
}

template<Colors C> Score<C>& Score<C>::unshared() { return *this; }

template<Colors C> const Score<C>& Score<C>::unshared() const { return *this; }

template<Colors C> unsigned int Score<C>::isNotReady() const { return 0; }

template<Colors C>
void SharedScore<C>::join() {
    UniqueLock<Mutex> lock(readyMutex);
    if (notReady) {
        WorkThread::idle(1);
        while (notReady)
            readyCond.wait(lock);
        WorkThread::idle(-1);
    }
}

template<Colors C>
bool  SharedScore<C>::max(const int b)         {
    LockGuard<RecursiveMutex> lock(valueMutex);
    if (*this < b) {
        v = b;
        return true;
    }
    return false;
}
