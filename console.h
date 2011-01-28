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
#include "stringlist.h"

class WorkThread;
class RootBoard;

class Console
#ifdef QT_GUI_LIB
    : public QApplication {
    Q_OBJECT
#else
{
#endif
    friend class TestRootBoard;
    void perft(StringList);
    void divide(StringList);
    void tryMove(std::string);
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
    void parse(std::string);
    void eval(StringList);
private
#ifdef QT_GUI_LIB
    slots
#endif
             :
    void privateSend(std::string);

private:
    RootBoard* board;
    StringList args;
    std::string answer;
    bool debugMode;
    std::map<std::string, std::string> option;
    std::map<std::string, void (Console::*)(StringList)> dispatcher;

public:
#ifdef QT_GUI_LIB
    QSocketNotifier *notifier;
    void info(int, uint64_t, uint64_t, QString, QString);
#endif
    Console(int& argc, char** argv);
    virtual ~Console();
    int exec();
    void send(std::string);

#ifdef QT_GUI_LIB
public slots:
    void dataArrived();
    void delayedEnable();
    void getResult(std::string);
    std::string getAnswer();

signals:
    void signalSend(std::string);
    void signalInfo(int, uint64_t, uint64_t, QString, QString);
#else
    void poll();
#endif
};

#endif /* CONSOLE_H_ */
