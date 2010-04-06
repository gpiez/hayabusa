/*
 * console.h
 *
 *  Created on: 22.09.2009
 *      Author: gpiez
 */
#ifndef CONSOLE_H_
#define CONSOLE_H_

#ifndef PGN_H
#include <pgn.h>
#endif

class WorkThread;

class Console: public QObject {
	Q_OBJECT

	QList<WorkThread*> workThreads;
	int	numThreads;

	void perft(QStringList);
	void divide(QStringList);
	void tryMove(QStringList);
	void initWorkThreads();
	void allocateWorkThreads();
	void stop();
public:
	QTextStream cin;
	QSocketNotifier *notifier;

	Console(QObject* parent);
	virtual ~Console();

public slots:
	void dataArrived();
	void delayedEnable();

signals:
	void send(QString);
};

#endif /* CONSOLE_H_ */
