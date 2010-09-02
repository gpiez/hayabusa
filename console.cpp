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

Console::Console(QCoreApplication* parent, StringList args):
    QObject(parent),
    app(parent),
    cin(stdin, QIODevice::ReadOnly),
    cout(stdout, QIODevice::WriteOnly)
{
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

    notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), this, SLOT(dataArrived()));
    connect(this, SIGNAL(signalSend(std::string)), this, SLOT(privateSend(std::string)));

    std::string argStr = args.join(" ");
    StringList cmdsList = split(argStr, ":");
    foreach(std::string cmdStr, cmdsList) {
        cmdStr = simplified(cmdStr);
        StringList cmds = split(cmdStr, " ");
        if (cmds.empty()) break;
        if (dispatcher.find(cmds[0]) != dispatcher.end())
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
    std::string temp = cin.readLine().toStdString();
    if (!temp.empty()) {
        StringList cmds = split(temp, " ");
        if (dispatcher.find(cmds[0]) != dispatcher.end()) {
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
    cout << "readyok" << endl << flush;
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
    WorkThread::stopAll();
    int m=std::find(cmds.begin(), cmds.end(), "moves") - cmds.begin();
    if (cmds[1] == "startpos")
        board->setup();
    else if (cmds[1] == "fen") {
        StringList fen;
        fen.resize(m-1);
        std::copy(cmds.begin()+1, cmds.begin()+m, fen.begin());
        board->setup(fen.join(" "));
    } else if (cmds[1] == "test") {
        std::string search = cmds[2];
        for (unsigned int i = 0; i < sizeof(testPositions)/sizeof(char*); ++i) {
            std::string pos(testPositions[i]);
            if (pos.find(search)) {
                board->setup(pos);
                break;
            }
        }
    }
    if (m>cmds.end()-cmds.begin()+1) {  //TODO FIXME
        StringList moves;
        moves.resize(cmds.end() - cmds.begin() - m - 1);
        std::copy(cmds.begin()+m+1, cmds.end(), moves.begin());
        foreach(std::string move, cmds)
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

void Console::iterationDone(unsigned int depth, uint64_t nodes, std::string line, int bestScore) {
    emit signalIterationDone(depth, nodes, line, bestScore);
}

void Console::privateSend(std::string str)
{
    if (!Options::quiet)
        std::cout << str << std::endl;
}

void Console::send(std::string str) {
    emit signalSend(str);
}

std::map<std::string, StringList> Console::parse(const StringList& cmds, const StringList& tokens) {
    typedef StringList::const_iterator I;
    std::map<I, std::string> tokenPositions;
    foreach(std::string token, tokens) {
        I tp=std::find(cmds.begin(), cmds.end(), token);
        if (tp != cmds.end())
            tokenPositions[tp] = token;
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
    static const unsigned nPos = sizeof(testPositions)/sizeof(char*);
    Options::quiet = true;
    if (cmds.size() > 1 && cmds[1] == "init") {
        for (unsigned int i = 0; i<nPos; ++i) {
            stats.node=0;
            board->setup(testPositions[i]);
            if (board->color == White)
                board->rootSearch<White>(32, 1000000);
            else
                board->rootSearch<Black>(32, 1000000);
            std::cout << board->depth-1 << "," << std::endl;
        }
    } else {
        double sum=0.0;
        double tested=0.0;
        for (unsigned int i = 0; i<nPos; ++i) {
            if (testDepths[i] == 51) continue;
            tested++;
            stats.node=0;
            board->setup(testPositions[i]);
            if (board->color == White)
                board->rootSearch<White>(testDepths[i]-20);
            else
                board->rootSearch<Black>(testDepths[i]-20);
            std::cout << std::setw(4) << i << "(" << testDepths[i]-20 << "):" << std::setw(10) << stats.node << std::endl;
            sum += log(stats.node);
        }
        std::cout << std::setw(20) << exp(sum/tested) << std::endl;
    }
}
