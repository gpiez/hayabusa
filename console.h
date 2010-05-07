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

class WorkThread;
class RootBoard;

class Console: public QObject {
	Q_OBJECT

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
	
private:
	QCoreApplication* app;
	RootBoard* board;
	QString answer;
	
public:
	QTextStream cin;
	QTextStream cout;
	QSocketNotifier *notifier;

	Console(QCoreApplication* parent);
	virtual ~Console();

public slots:
	void dataArrived();
	void delayedEnable();
	void getResult(QString);
	QString getAnswer();
	
signals:
	void send(QString);
};

#endif /* CONSOLE_H_ */
