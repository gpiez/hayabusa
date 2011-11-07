/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Gunther Piez <gpiez@web.de>

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


#ifndef SELFGAME_H
#define SELFGAME_H

#include <pch.h>
#include "rootboard.h"
#include "parameters.h"

class SelfGame
{
    uint64_t wtime;
    uint64_t btime;
    int decisiveScore;
    int decisiveScoreMoves;
    int drawScoreMoves;
    int nDecisive;
    int nDraw;
    int result;
public:
    RootBoard   *wrb, *brb;
    std::chrono::system_clock::time_point start;
    SelfGame(Console* c, const Parameters&, const Parameters&);
    ~SelfGame();
    int doGame(RootBoard*, RootBoard*);
    template<Colors C> bool checkResult(const RootBoard&);
    void setupRootBoard(RootBoard* );
    uint64_t cpuTime();
    int tournament();
};

#endif // SELFGAME_H
