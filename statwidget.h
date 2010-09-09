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
#ifndef STATWIDGET_H
#define STATWIDGET_H

#ifndef PCH_H_
#include <pch.h>
#endif

#ifdef QT_GUI_LIB

#include "ui_stats.h"
#include "nodemodel.h"

class RootBoard;
class NodeModel;

class StatWidget: public QWidget, private Ui::Statsui
{
	Q_OBJECT

	const RootBoard& rb;
    int iRow;
    QImage wPieces[6], bPieces[6];
	QColor pal[256];
	QLabel* minipm[2][7];
private slots:
	void update();
    void updateBoard();
	void updateInfo(int, uint64_t, uint64_t, QString, QString);

public:
	NodeModel* volatile tree;
	void setMoveTree();
	StatWidget(const RootBoard&);
	virtual ~StatWidget();

public slots:
	void emptyTree();
};
#endif // QT_GUI_LIB
#endif // STATWIDGET_H
