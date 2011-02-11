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
#include "testpositions.h"
#include "transpositiontable.tcc"
#include "rootboard.tcc"
#include "rootsearch.tcc"
#include "stringlist.h"

namespace Options {
    unsigned int        splitDepth = 1000;
    int                 humanreadable = 0;
    int                 hash = 0x1000000;
    bool                quiet = false;
    bool                preCutIfNotThreatened = false;
    unsigned            veinDepth = 20;
    unsigned            leafDepth = 8;
    bool                reduction = false;
    unsigned            debug = 0;
#ifdef QT_NETWORK_LIB    
    bool                server = false;
#endif    
}

Console::Console(int& argc, char** argv)
#if defined(QT_GUI_LIB)
    :
    QApplication(argc, argv)
#else
#if defined(QT_NETWORK_LIB)
    :
    QCoreApplication(argc, argv)
#endif    
#endif
{
    args.resize(argc);
    std::copy(argv+1, argv+argc, args.begin());

    BoardBase::initTables();
    WorkThread::init();
    board = new RootBoard(this);
    board->setup();

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

    dispatcher["perft"] = &Console::perft;
    dispatcher["divide"] = &Console::divide;
    dispatcher["ordering"] = &Console::ordering;
    dispatcher["eval"] = &Console::eval;

#if defined(QT_GUI_LIB) || defined(QT_NETWORK_LIB)
    connect(this, SIGNAL(signalSend(std::string)), this, SLOT(privateSend(std::string)));
#endif
}

Console::~Console()
{
}

int Console::exec() {
    std::string argStr = args.join(" ");
    StringList cmdsList = split(argStr, ":");
    for(auto cmdStr = cmdsList.begin(); cmdStr != cmdsList.end(); ++cmdStr) {
        parse(simplified(*cmdStr));
    }
#if defined(QT_NETWORK_LIB)
    if (!Options::server) {
        notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
        connect(notifier, SIGNAL(activated(int)), this, SLOT(dataArrived()));
    }
#endif
#if defined(QT_GUI_LIB)
    return QApplication::exec();
#else
#if defined(QT_NETWORK_LIB)
    return QCoreApplication::exec();
#else
    while(true) {
        std::string str;
        std::getline(std::cin, str);
        if (str[str.length()-1] == 13 || str[str.length()-1] == 10) {
            str.erase(str.length()-1);
        }
        parse(str);
    }
    return 0;
#endif
#endif    
}

#if defined(QT_GUI_LIB) || defined(QT_NETWORK_LIB)
std::string Console::getAnswer() {
    answer = "";
    while(answer == "") {
        processEvents();
        sleep(1);
    }
    return answer;
}

void Console::send(std::string str) {
    emit signalSend(str);
}

void Console::getResult(std::string result) {
    answer = result;
    std::cout << result << std::endl;
}

void Console::dataArrived() {
    std::string temp;
    if (Options::server) {
        if (!socket->canReadLine()) return;
        char buffer[nMaxGameLength*10+100];
        qint64 lineLength = socket->readLine(buffer, sizeof(buffer));
        temp.append(buffer, lineLength);
//        for (int i=0; i<lineLength; ++i) std::cout << (int)buffer[i] << std::endl;
//        std::cout << temp << std::endl;
    } else
        std::getline(std::cin, temp);
    
    while (temp[temp.length()-1] == 13 || temp[temp.length()-1] == 10) {
        temp.erase(temp.length()-1);
    }
    parse(temp);
    if (Options::server && socket->canReadLine())
        dataArrived();
//    QTimer::singleShot(50, this, SLOT(delayedEnable()));
}

void Console::delayedEnable() { //TODO remove if not needed under windows
    notifier->setEnabled(true);
}

void Console::newConnection()
{
    socket = server->nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(dataArrived()));    
}
#else
void Console::send(std::string str) {
    privateSend(str);
}
#endif

void Console::privateSend(std::string str)
{
#if defined(QT_NETWORK_LIB)
    if (Options::server) {
        socket->write(str.c_str());
        socket->write("\n");
        socket->flush();
    } else
#endif
    {
        std::cout << str << std::endl;
    }
    answer = str;
}

void Console::parse(std::string str) {
    if (!str.empty()) {
        StringList cmds = split(str, " ");
        if (dispatcher.find(cmds[0]) != dispatcher.end()) {
            (this->*(dispatcher[cmds[0]]))(cmds);
        } else {
            tryMove(cmds[0]);
        }
    }
}

void Console::tryMove(std::string cmds) {
    std::string mstr = toLower(cmds);
    if (mstr.length() < 4 || mstr.length() > 5) {
        send("command '" + mstr + "' not understood");
        return;
    }
    int piece;
    bool special;
    if (mstr.length() == 5) {
        special = true;
        switch (mstr[4]) {
        case 'q':
            piece = Queen;
            break;
        case 'r':
            piece = Rook;
            break;
        case 'b':
            piece = Bishop;
            break;
        case 'n':
        case 'k':
            piece = Knight;
            break;
        default:
            send("move '" + mstr + "' not understood");
            return;
        }
    } else {
        piece = 0;
        special = false;
    }
    Move m(mstr[0] -'a' + (mstr[1]-'1')*8, mstr[2]-'a' + (mstr[3]-'1')*8, piece, 0, special);
    if (board->doMove(m)) {
        send("move '" + mstr + "' illegal");
    }
}

void Console::perft(StringList cmds) {
    board->perft(convert(cmds[1]));
}

void Console::divide(StringList cmds) {
    board->divide(convert(cmds[1]));
}

void Console::uci(StringList /*cmds*/) {
    send("id name hayabusa 0.1");
    send("author Gunther Piez");
    send("uciok");
}

void Console::debug(StringList cmds) {
    auto pos = cmds.parse(StringList() << "eval" << "mobility" << "search");
    if (pos.count("eval")) Options::debug |= debugEval;
    if (pos.count("mobility")) Options::debug |= debugMobility;
    if (pos.count("search")) Options::debug |= debugSearch;
}

// engine is always ready as soon as the command dispatcher is working.
void Console::isready(StringList /*cmds*/) {
    send("readyok");
}

void Console::setoption(StringList cmds) {
    std::map<std::string, StringList> o = cmds.parse(StringList() << "name" << "value");
    std::string name =  toLower(o["name"].join(" "));
    if (!name.empty()) {
        std::string data = toLower(o["value"].join(" "));
        if (name == "splitdepth") {
            Options::splitDepth = convert(data);
        } else if (name == "humanreadable") {
            Options::humanreadable = convert(data);
        } else if (name == "hash") {
            Options::hash = convert(data);
            if (Options::hash) board->tt->setSize(Options::hash*0x100000ULL);
        } else if (name == "quiet") {
            Options::quiet = convert(data);
        } else if (name == "reduction") {
            Options::reduction = true;
#ifdef QT_NETWORK_LIB
        } else if (name == "server") {
            Options::server = true;
            server = new QTcpServer();
            server->listen(QHostAddress::Any, 7788);
            connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));
#endif
        }
    }
}

void Console::reg(StringList /*cmds*/) {
}

void Console::ucinewgame(StringList /*cmds*/) {
    WorkThread::stopAll();
    board->ttClear();
}

void Console::position(StringList cmds) {
    if (cmds.size() < 2) return;
    auto pos = cmds.parse(StringList() << "startpos" << "fen" << "test" << "moves");
    WorkThread::stopAll();
    if (pos.count("startpos"))
        board->setup();
    else if (pos.count("fen")) {
        board->setup(pos["fen"].join(" "));
    } else if (pos.count("test")) {
        if (cmds.size() < 3) return;
        std::string search = pos["test"].join(" ");
        for (unsigned int i = 0; testPositions[i]; ++i) {
            std::string pos(testPositions[i]);
            if (pos.find(search) != pos.npos) {
                board->setup(pos);
                break;
            }
        }
    }

    if (pos.count("moves")) {
        StringList moves = pos["moves"];
        for (auto imove = moves.begin(); imove != moves.end(); ++imove) {
            tryMove(*imove);
        }
    }
}

void Console::go(StringList cmds) {
    WorkThread::stopAll();
    std::map<std::string, StringList> subCmds = cmds.parse(StringList() << "searchmoves"
    << "ponder" << "wtime" << "btime" << "winc" << "binc" << "movestogo" << "depth"
    << "nodes" << "mate" << "movetime" << "infinite");

    board->go(subCmds);
}

void Console::stop(StringList /*cmds*/) {
    board->stop(true);
    WorkThread::stopAll();
    board->stop(false);
}

void Console::ponderhit(StringList /*cmds*/) {
}

void Console::quit(StringList /*cmds*/) {
#ifdef QT_GUI_LIB
    QApplication::quit();
#else
    exit(0);
#endif
}

void Console::ordering(StringList cmds) {
    Options::quiet = true;
    board->infinite = true;
    if (cmds.size() > 1 && cmds[1] == "init") {
        for (unsigned int i = 0; testDepths[i]; ++i) {
            stats.node=0;
            board->maxSearchNodes = 1000000;
            board->setup(testPositions[i]);
            board->ttClear();
            if (board->color == White)
                board->rootSearch<White>(40);
            else
                board->rootSearch<Black>(40);
            if (!(i % 26)) std::cout << std::endl;
            if (stats.node < 1000000)
                std::cout << " 0,";
            else
                std::cout << std::setw(2) << board->depth - 21 << ",";
        }
    } else {
        double sum=0.0;
        double tested=0.0;
        for (int i = 0; testDepths[i]; ++i) {
            if (testDepths[i] < 0) continue;
            board->maxSearchNodes = ~0;
            tested++;
            stats.node=0;
            board->setup(testPositions[i]);
            board->ttClear();
            if (board->color == White)
                board->rootSearch<White>(testDepths[i]);
            else
                board->rootSearch<Black>(testDepths[i]);
            std::cout << std::setw(4) << i << "(" << std::setw(2) << testDepths[i]  << "):" << std::setw(10) << stats.node << std::endl;
            sum += log(stats.node);
        }
        std::cout << std::setw(20) << exp(sum/tested) << std::endl;
    }
}

void Console::eval(StringList)
{
    board->eval.eval(board->currentBoard());
}
