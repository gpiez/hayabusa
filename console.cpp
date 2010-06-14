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

#include <unistd.h>

#include "console.h"
#include "workthread.h"
#include "rootboard.h"
#include "coloredboard.tcc"
#include "options.h"

namespace Options {
unsigned int splitDepth = 5;
int humanreadable = 0;
}

Console::Console(QCoreApplication* parent):
    QObject(parent),
    app(parent),
    cin(stdin, QIODevice::ReadOnly),
    cout(stdout, QIODevice::WriteOnly)
{
    BoardBase::initTables();
    ColoredBoard<White>::initTables();
    ColoredBoard<Black>::initTables();
    WorkThread::init();
    board = new RootBoard(this);
    board->setup();
    
    dispatcher["perft"] = &Console::perft;
    dispatcher["divide"] = &Console::divide;
    dispatcher["uci"] = &Console::uci;
    dispatcher["debug"] = &Console::debug;
    dispatcher["isready"] = &Console::isready;
    dispatcher["setoption"] = &Console::setoption;
    dispatcher["register"] = &Console::reg;
    dispatcher["ucinewgame"] = &Console::ucinewgame;
    dispatcher["position"] = &Console::position;
    dispatcher["go"] = &Console::go;
    dispatcher["stop"] = &Console::stop;
    dispatcher["ponderhit"] = &Console::ponderhit;
    dispatcher["quit"] = &Console::quit;
    
    notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), this, SLOT(dataArrived()));
    connect(this, SIGNAL(signalSend(std::string)), this, SLOT(privateSend(std::string)));
    
    QStringList args = QCoreApplication::arguments();
    args.removeFirst();
    QString argStr = args.join(" ");
    QStringList cmdsList = argStr.split(":");
    foreach(QString cmdStr, cmdsList) {
        cmdStr = cmdStr.simplified();
        QStringList cmds = cmdStr.split(" ");
        if (cmds.isEmpty()) break;
        if (dispatcher.contains(cmds[0]))
            (this->*(dispatcher[cmds[0]]))(cmds);
        else
            tryMove(cmds);
    
    }
}

Console::~Console()
{
}

void Console::getResult(std::string result) {
    answer = result;
    std::cout << result << std::endl;
}

void Console::dataArrived() {
    notifier->setEnabled(false);
    QString temp = cin.readLine();
    if (!temp.isEmpty()) {
        QStringList cmds = temp.split(' ');
        if (dispatcher.contains(cmds[0])) {
            (this->*(dispatcher[cmds[0]]))(cmds);
        } else {
            tryMove(cmds);
        }

    }
    QTimer::singleShot(50, this, SLOT(delayedEnable()));
}

void Console::delayedEnable() {
    notifier->setEnabled(true);
}

void Console::tryMove(QStringList cmd) {
    qDebug() << "trymove" << cmd;
}

void Console::perft(QStringList cmds) {
    board->perft(cmds[1].toInt());
}

void Console::divide(QStringList cmds) {
    board->divide(cmds[1].toInt());
}

void Console::uci(QStringList /*cmds*/) {
    send("id name hayabusa 0.1");
    send("author Gunther Piez");
}

void Console::debug(QStringList cmds) {
    if (cmds[1] == "on")
        debugMode = true;
    else if (cmds[1]=="off")
        debugMode = false;
    else
        send("info string usage: debug on|off");
}

// engine is always ready as soon as the command dispatcher is working.
void Console::isready(QStringList /*cmds*/) {
    cout << "readyok" << endl << flush;
}

void Console::setoption(QStringList cmds) {
    const QHash<QString, QStringList> o = parse(cmds, QStringList() << "name" << "value");
    QString name = o["name"].join(" ").toLower();
    if (!name.isEmpty()) {
        QString data = o["value"].join(" ").toLower();
        if (name == "splitdepth") {
            Options::splitDepth = data.toInt();
        } else if (name == "humanreadable") {
            Options::humanreadable = data.toInt();
        }
    }
}

void Console::reg(QStringList /*cmds*/) {
}

void Console::ucinewgame(QStringList /*cmds*/) {
    WorkThread::stopAll();
    board->ttClear();
}

void Console::position(QStringList cmds) {
    WorkThread::stopAll();
    int m=cmds.indexOf("moves");
    if (cmds[1] == "startpos")
        board->setup();
    else if (cmds[1] == "fen") {
        QStringList fen = cmds.mid(2, m-2);
        board->setup(fen.join(" "));
    }
    if (m>0) {
        QStringList moves = cmds.mid(m+1);
        foreach(QString move, cmds)
            ; //TODO
    }
}

void Console::go(QStringList cmds) {
    WorkThread::stopAll();
    QHash<QString, QStringList> subCmds = parse(cmds, QStringList() << "searchmoves"
    << "ponder" << "wtime" << "btime" << "winc" << "binc" << "movestogo" << "depth"
    << "nodes" << "mate" << "movetime" << "infinite");
    board->go(cmds);
}

void Console::stop(QStringList /*cmds*/) {
}

void Console::ponderhit(QStringList /*cmds*/) {
}

void Console::quit(QStringList /*cmds*/) {
    app->quit();
}

std::string Console::getAnswer() {
    answer = "";
    while(answer == "") {
        app->processEvents();
        sleep(1);
    }
    return answer;
}

void Console::info(int depth, int seldepth, uint64_t time, uint64_t nodes,
                   QString pv, RawScore score, Move currMove, int currMoveNumber,
                   int hashfull, int nps, int tbhits, int cpuload, QString currline) {
    emit signalInfo(depth, seldepth, time, nodes,
                   pv, score, currMove, currMoveNumber,
                   hashfull, nps, tbhits, cpuload, currline);
}

void Console::iterationDone(unsigned int depth, uint64_t nodes, std::string line, int bestScore) {
    emit signalIterationDone(depth, nodes, line, bestScore);
}

void Console::privateSend(std::string str)
{
    std::cout << str << std::endl;
}

void Console::send(std::string str) {
    emit signalSend(str);
}

QHash<QString, QStringList> Console::parse(QStringList cmds, QStringList tokens) {
    QMap<int, QString> tokenPositions;
    foreach(QString token, tokens)
        if (cmds.indexOf(token)>0)
            tokenPositions[cmds.indexOf(token)] = token;
    tokenPositions[9999] = "";
    
    QHash<QString, QStringList> tokenValues;
    for(int i=0; i<tokenPositions.keys().count()-1; ++i) {
        tokenValues[tokenPositions.values().at(i)] =
            cmds.mid(tokenPositions.keys().at(i)+1, tokenPositions.keys().at(i+1)-tokenPositions.keys().at(i)-1);
    }
    return tokenValues;
}
