/*
 * console.h
 *
 *  Created on: 22.09.2009
 *      Author: gpiez
 */
#ifndef CONSOLE_H_
#define CONSOLE_H_

#ifndef PCH_H_
#include <pch.h>
#endif

class WorkThread;

class Console: public QObject {
	Q_OBJECT

	QList<WorkThread*> workThreads;
	int	numThreads;

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
	
	void initWorkThreads();
	void allocateWorkThreads();
	void stop();
	
private:
	QCoreApplication* app;
	
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
	
signals:
	void send(QString);
};

#endif /* CONSOLE_H_ */
