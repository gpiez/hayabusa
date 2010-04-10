/*
 * console.cpp
 *
 *  Created on: 22.09.2009
 *      Author: gpiez
 */

#include <pch.h>

#include <unistd.h>

#include "console.h"
#include "workthread.h"
#include "jobs.h"

QMap<QString, void (Console::*)(QStringList)> dispatcher;

Console::Console(QCoreApplication* parent):
	QObject(parent),
	app(parent),
	cin(stdin, QIODevice::ReadOnly),
	cout(stdout, QIODevice::WriteOnly)
{
	BoardBase::initTables();
	initWorkThreads();
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
}

Console::~Console()
{
}

void Console::getResult(QString result) {
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
//	stop();
	workThreads.first()->startJob(new PerftJob(cmds[1].toInt(), this));
//	workThreads.first()->startJob();
}

void Console::divide(QStringList cmds) {
//	qDebug() << cmds;
//	stop();
	workThreads.first()->startJob(new DivideJob(cmds[1].toInt()));
//	workThreads.first()->startJob();
}

void Console::initWorkThreads() {
	numThreads = sysconf(_SC_NPROCESSORS_ONLN);
	if (numThreads < 0) numThreads = 1;
	allocateWorkThreads();
}

void Console::allocateWorkThreads() {
	int i=0;
	foreach(WorkThread* th, workThreads) {
		if (++i > numThreads) {
			th->stop();
			th->wait();
			delete th;
		}
	}

	while (++i <= numThreads) {
		WorkThread* th = new WorkThread;
		th->start();
		workThreads.append(th);
	}

}

void Console::stop() {
	foreach(WorkThread* th, workThreads) {
		th->stop();
	}
}

void Console::uci(QStringList cmds) {
}

void Console::debug(QStringList cmds) {
}

void Console::isready(QStringList cmds) {
}

void Console::setoption(QStringList cmds) {
}

void Console::reg(QStringList cmds) {
}

void Console::ucinewgame(QStringList cmds) {
}

void Console::position(QStringList cmds) {
	if (cmds[1] == "startpos")
		workThreads.first()->b.setup();
	else
		workThreads.first()->b.setup(cmds[1]);
}

void Console::go(QStringList cmds) {
}

void Console::stop(QStringList cmds) {
}

void Console::ponderhit(QStringList cmds) {
}

void Console::quit(QStringList cmds) {
	app->quit();
}
