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

#ifdef USE_GENETIC
#include "game.h"
#include "parameters.h"

class SelfGame {
    uint64_t wtime;
    uint64_t btime;
    int decisiveScore;
    int decisiveScoreMoves;
    int drawScoreMoves;
    int nDecisive;
    int nDraw;
    int result;
    std::string nodes;
    std::string endgame;
    int numGames;
public:
    Game*   wrb, *brb;
    std::chrono::system_clock::time_point start;
    SelfGame(Console* c, const Parameters& wp, const Parameters& bp, const std::string& nodes, std::string endgame);
    ~SelfGame();
    int nTests();
    int doGame(Game*, Game*);
    template<Colors C> bool checkResult(const Game&);
    void setupRootBoard(Game*);
    uint64_t cpuTime();
    int tournament(); };
#endif
#endif // SELFGAME_H
