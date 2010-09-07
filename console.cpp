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
#include "stringlist.h"

namespace Options {
unsigned int splitDepth = 1000;
int humanreadable = 0;
int hash = 0x1000000;
bool quiet = false;
}

Console::Console(int& argc, char** argv)
#ifdef QT_GUI_LIB
    :
    QApplication(argc, argv)/*,
    cin(stdin, QIODevice::ReadOnly)*/
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

#ifdef QT_GUI_LIB
    notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), this, SLOT(dataArrived()));
    connect(this, SIGNAL(signalSend(std::string)), this, SLOT(privateSend(std::string)));
#endif
}

Console::~Console()
{
}

#ifdef QT_GUI_LIB
int Console::exec() {
    std::string argStr = args.join(" ");
    StringList cmdsList = split(argStr, ":");
    for(auto cmdStr = cmdsList.begin(); cmdStr != cmdsList.end(); ++cmdStr) {
        parse(simplified(*cmdStr));
    }
    return QApplication::exec();
}
std::string Console::getAnswer() {
    answer = "";
    while(answer == "") {
        processEvents();
        sleep(1);
    }
    return answer;
}

void Console::privateSend(std::string str)
{
    if (!Options::quiet)
        std::cout << str << std::endl;
}

void Console::send(std::string str) {
    emit signalSend(str);
}

void Console::getResult(std::string result) {
    answer = result;
    std::cout << result << std::endl;
}

void Console::dataArrived() {
    notifier->setEnabled(false);
    std::string temp;
    std::cin >> temp;
    parse(temp);
    QTimer::singleShot(50, this, SLOT(delayedEnable()));
}

void Console::delayedEnable() {
    notifier->setEnabled(true);
}
#else
int Console::exec() {
    std::string argStr = args.join(" ");
    StringList cmdsList = split(argStr, ":");
    for(auto cmdStr = cmdsList.begin(); cmdStr != cmdsList.end(); ++cmdStr) {
        parse(simplified(*cmdStr));
    }
    poll();
}

void Console::send(std::string str) {
    std::cout << str << std::endl;
}

void Console::poll() {
    while(true) {
        std::string str;
        std::cin >> str;
        parse(str);
    }
}
#endif

void Console::parse(std::string str) {
    if (!str.empty()) {
        StringList cmds = split(str, " ");
        if (dispatcher.find(cmds[0]) != dispatcher.end()) {
            (this->*(dispatcher[cmds[0]]))(cmds);
        } else {
            tryMove(cmds);
        }
    }
}

void Console::tryMove(StringList cmds) {
	std::string mstr = toLower(cmds[0]);
	if (mstr.length() < 4 || mstr.length() > 5) {
		send("move '" + mstr + "' not understood");
		return;
	}
	int piece;
	if (mstr.length() == 5) {
		switch (mstr[5]) {
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
	} else
		piece = 0;
	Move m(mstr[0] -'a' + (mstr[1]-'1')*8, mstr[2]-'a' + (mstr[3]-'1')*8, piece);
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
}

void Console::debug(StringList cmds) {
    if (cmds[1] == "on")
        debugMode = true;
    else if (cmds[1]=="off")
        debugMode = false;
    else
        send("info string usage: debug on|off");
}

// engine is always ready as soon as the command dispatcher is working.
void Console::isready(StringList /*cmds*/) {
    std::cout << "readyok" << std::endl << std::flush;
}

void Console::setoption(StringList cmds) {
    std::map<std::string, StringList> o = parse(cmds, StringList() << "name" << "value");
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
    auto pos = parse(cmds, StringList() << "startpos" << "fen" << "test" << "moves");
    WorkThread::stopAll();
    if (pos.count("startpos"))
        board->setup();
    else if (pos.count("fen")) {
        board->setup(pos["fen"].join(" "));
    } else if (pos.count("test")) {
        if (cmds.size() < 3) return;
        std::string search = pos["test"].join();
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
                ; //TODO
    }
}

void Console::go(StringList cmds) {
    WorkThread::stopAll();
    std::map<std::string, StringList> subCmds = parse(cmds, StringList() << "searchmoves"
    << "ponder" << "wtime" << "btime" << "winc" << "binc" << "movestogo" << "depth"
    << "nodes" << "mate" << "movetime" << "infinite");

    board->go(subCmds);
}

void Console::stop(StringList /*cmds*/) {
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

std::map<std::string, StringList> Console::parse(const StringList& cmds, const StringList& tokens) {
    typedef StringList::const_iterator I;
    std::map<I, std::string> tokenPositions;
    for(auto token = tokens.begin(); token != tokens.end(); ++token) {
        I tp=std::find(cmds.begin(), cmds.end(), *token);
        if (tp != cmds.end())
            tokenPositions[tp] = *token;
    }
    tokenPositions[cmds.end()] = "";
    std::map<std::string, StringList> tokenValues;
    for(auto i=tokenPositions.begin(); (*i).first != cmds.end(); ++i) {
        ASSERT(*((*i).first) == (*i).second);
        auto j = i;
        ++j;
//        std::cout << "parse:" << (*i).second << ":" << *((*i).first+1) << std::endl;
        tokenValues[(*i).second].resize((*j).first - (*i).first - 1);
        std::copy ((*i).first+1, (*j).first, tokenValues[(*i).second].begin());
    }
    return tokenValues;
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
