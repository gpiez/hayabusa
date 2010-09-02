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
#ifndef CONSOLE_H_
#define CONSOLE_H_

#ifndef PCH_H_
#include <pch.h>
#endif

#include "move.h"
#include "score.h"

class WorkThread;
class RootBoard;
class StringList;

class Console: public QObject {
    Q_OBJECT
    friend class TestRootBoard;
    void perft(StringList);
    void divide(StringList);
    void tryMove(StringList);
    void quit(StringList);
    void ponderhit(StringList);
    void stop(StringList);
    void go(StringList);
    void position(StringList);
    void ucinewgame(StringList);
    void reg(StringList);
    void setoption(StringList);
    void isready(StringList);
    void uci(StringList);
    void debug(StringList);
    void ordering(StringList);

private slots:
    void privateSend(std::string);

private:
    QCoreApplication* app;
    RootBoard* board;
    std::string answer;
    bool debugMode;
    std::map<std::string, std::string> option;
    std::map<std::string, StringList> parse(const StringList&, const StringList&);
    std::map<std::string, void (Console::*)(StringList)> dispatcher;

public:
    QTextStream cin;
    QTextStream cout;
    QSocketNotifier *notifier;

    Console(QCoreApplication* parent, StringList args);
    virtual ~Console();
    void iterationDone(unsigned int depth, uint64_t nodes, std::string line, int bestScore);
    void info(int depth, int seldepth, uint64_t time, uint64_t nodes,
                         std::string pv, RawScore score, Move currMove, int currMoveNumber,
                         int hashfull, int nps, int tbhits, int cpuload, std::string currline);
    void send(std::string);

public slots:
    void dataArrived();
    void delayedEnable();
    void getResult(std::string);
    std::string getAnswer();

signals:
    void signalSend(std::string);
    void signalInfo(int depth, int seldepth, uint64_t time, uint64_t nodes,
                         std::string pv, RawScore score, Move currMove, int currMoveNumber,
                         int hashfull, int nps, int tbhits, int cpuload, std::string currline);
    void signalIterationDone(unsigned int depth, uint64_t nodes, std::string line, int bestScore);
};

#endif /* CONSOLE_H_ */
