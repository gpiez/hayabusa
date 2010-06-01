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
#include "stats.h"

#ifdef QT_GUI_LIB
#include "statwidget.h"
#include "rootboard.h"
#include "transpositiontable.tcc"
#include "console.h"
#include "workthread.h"

__thread Stats stats;
StatWidget::StatWidget(const RootBoard& rb):
	rb(rb)
{
	setupUi(this);
	QFont fixed;
	fixed.setFamily("Consolas");
	bestLine->setFont(fixed);
	QTimer* t = new QTimer(this);
	connect(t, SIGNAL(timeout()), this, SLOT(update()));
	qRegisterMetaType<uint64_t>("uint64_t");
	connect(rb.console, SIGNAL(signalIterationDone(unsigned int, uint64_t, QString, int)), this, SLOT(updateLine(unsigned int, uint64_t, QString, int)));
	t->setInterval(1000);
	update();
	t->start();
}

StatWidget::~StatWidget()
{
}

void StatWidget::updateLine(unsigned int depth, uint64_t nodes, QString line, int bestScore)
{
	static QString oldBestLine;
	static unsigned int oldDepth;
	
	if (line != oldBestLine || depth != oldDepth) {
		oldBestLine = line;
		oldDepth = depth;
		//bestLine->moveCursor(QTextCursor::Start);
		bestLine->appendPlainText(QString("%1 %2 %3 %4 %5").arg(depth, 2).arg(nodes, 15).arg(rb.getTime(), 15).arg(bestScore/100.0, 6, 'f', 2).arg(line));
		Ui_Statsui::depth->setText("Depth: " + QString::number(depth));
	}
}
/* Store the last 10 stats for a sliding average */
template<typename T>
QString number(T n) {
	if (n==0) return "0";
	int base = floor(log(n)/log(1000.0));
	if (base>4) base=4;
	if (base<-4) base=-4;
	double base10 = pow(1000.0, base);
	return QString(QString::number(n/base10, 'g', 3) % QString(" ") % "pnµm kMGT"[base+5]).trimmed();
}

void StatWidget::update()
{
	static QList<Stats> prev;
	prev.append(rb.getStats());
	if (prev.size() > 10)
		prev.removeFirst();

#define DISPLAYNUM(x) n##x->setText(number(prev.last().x)); if (prev.size() > 1) v##x->setText(number((prev.last().x - prev.first().x) / (prev.size()-1)));

	ninternalNode->setText(number(WorkThread::running));
	DISPLAYNUM(node)
	DISPLAYNUM(eval)
	DISPLAYNUM(tthit)
	DISPLAYNUM(ttuse)
	DISPLAYNUM(ttfree)
	DISPLAYNUM(ttalpha)
	DISPLAYNUM(ttbeta)
	DISPLAYNUM(ttoverwrite)
	DISPLAYNUM(ttinsufficient)
	DISPLAYNUM(ttstore)
	DISPLAYNUM(leafcut);
	DISPLAYNUM(pthit);
	DISPLAYNUM(ptmiss);
	DISPLAYNUM(ptuse);
}
#endif // QT_GUI_LIB
