/*
 * test.h
 *
 *  Created on: 29.09.2009
 *      Author: gpiez
 */

#ifndef TEST_H_
#define TEST_H_

#include <QtCore>
#include <QtTest/QtTest>

class TestBoard: public QObject {
	Q_OBJECT
private slots:
	void setPiece();
	void add();
	void generateCaptures();
	void initTestCase();
};

#endif /* TEST_H_ */
