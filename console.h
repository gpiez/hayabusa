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

#include <stdint.h>
#include "move.h"
#include "score.h"

class WorkThread;
class RootBoard;

class Console: public QObject {
	Q_OBJECT
	friend class TestRootBoard;
	void perft(QStringList);
	void divide(QStringList);
	void tryMove(QStringList);
	void quit(QStringList);
	void ponderhit(QStringList);
	void stop(QStringList);
	void go(QStringList);
	void position(QStringList);
	void ucinewgame(QStringList);
	void reg(QStringList);
	void setoption(QStringList);
	void isready(QStringList);
	void uci(QStringList);
	void debug(QStringList);
private slots:
	void privateSend(QString);
	
private:
	QCoreApplication* app;
	RootBoard* board;
	QString answer;
    bool debugMode;
    QMap<QString, QString> option;
	
	
public:
	QTextStream cin;
	QTextStream cout;
	QSocketNotifier *notifier;

	Console(QCoreApplication* parent);
	virtual ~Console();
    void iterationDone(unsigned int depth, uint64_t nodes, QString line, int bestScore);
	void info(int depth, int seldepth, uint64_t time, uint64_t nodes,
						 QString pv, RawScore score, Move currMove, int currMoveNumber,
						 int hashfull, int nps, int tbhits, int cpuload, QString currline);
	void send(QString);

public slots:
	void dataArrived();
	void delayedEnable();
	void getResult(QString);
	QString getAnswer();
	
signals:
	void signalSend(QString);
	void signalInfo(int depth, int seldepth, uint64_t time, uint64_t nodes,
						 QString pv, RawScore score, Move currMove, int currMoveNumber,
						 int hashfull, int nps, int tbhits, int cpuload, QString currline);						 
    void signalIterationDone(unsigned int depth, uint64_t nodes, QString line, int bestScore);
};

#endif /* CONSOLE_H_ */
