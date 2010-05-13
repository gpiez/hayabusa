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

QMap<QString, void (Console::*)(QStringList)> dispatcher;

Console::Console(QCoreApplication* parent):
	QObject(parent),
	app(parent),
	cin(stdin, QIODevice::ReadOnly),
	cout(stdout, QIODevice::WriteOnly)
{
	BoardBase::initTables();
	ColoredBoard<White>::initTables();
	ColoredBoard<Black>::initTables();
	Zobrist::initTables();
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
	connect(this, SIGNAL(signalSend(QString)), this, SLOT(privateSend(QString)));
	
	QStringList cmds = QCoreApplication::arguments();
	cmds.removeFirst(); 
	if (!cmds.isEmpty()) {
		if (dispatcher.contains(cmds[0])) 
			(this->*(dispatcher[cmds[0]]))(cmds);
		else
			tryMove(cmds);
		
	}
}

Console::~Console()
{
}

void Console::getResult(QString result) {
	answer = result;
	cout << result << endl;
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
}

void Console::debug(QStringList /*cmds*/) {
}

void Console::isready(QStringList /*cmds*/) {
}

void Console::setoption(QStringList /*cmds*/) {
}

void Console::reg(QStringList /*cmds*/) {
}

void Console::ucinewgame(QStringList /*cmds*/) {
}

void Console::position(QStringList cmds) {
	if (cmds[1] == "startpos")
		board->setup();
	else {
		cmds.removeFirst();
		board->setup(cmds.join(" "));
	}
}

void Console::go(QStringList cmds) {
	board->go(cmds);
}

void Console::stop(QStringList /*cmds*/) {
}

void Console::ponderhit(QStringList /*cmds*/) {
}

void Console::quit(QStringList /*cmds*/) {
	app->quit();
}

QString Console::getAnswer() {
	answer = QString("");
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

void Console::iterationDone(unsigned int depth, uint64_t nodes, QString line, int bestScore) {
	emit signalIterationDone(depth, nodes, line, bestScore);
}

void Console::privateSend(QString str)
{
	cout << str << endl;
}

void Console::send(QString str) {
	emit signalSend(str);
}