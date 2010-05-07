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

Stats stats;

#ifdef QT_GUI_LIB
#include "statwidget.h"
#include "rootboard.h"
#include "transpositiontable.tcc"

StatWidget::StatWidget(const RootBoard& rb):
	rb(rb)
{
	setupUi(this);
	QTimer* t = new QTimer(this);
	connect(t, SIGNAL(timeout()), this, SLOT(update()));
	t->setInterval(1000);
	t->start();
}

StatWidget::~StatWidget()
{
}

#define DISPLAYNUM(x) n##x->setText(QString::number(prev.last().x)); if (prev.size() > 1) v##x->setText(QString::number((prev.last().x - prev.first().x) / prev.size()));
/* Store the last 10 stats for a sliding average */
void StatWidget::update()
{
	static QList<Stats> prev;
	prev.append(stats);
	if (prev.size() > 10)
		prev.removeFirst();

	label->setText(rb.tt->bestLine(rb));

	DISPLAYNUM(node)
	DISPLAYNUM(eval)
	DISPLAYNUM(tthit)
	DISPLAYNUM(ttuse)
	DISPLAYNUM(ttfree)
	
}
#endif // QT_GUI_LIB
