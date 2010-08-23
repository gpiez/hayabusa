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
#include "nodemodel.h"
#include "nodedelegate.h"

static void splitImage( QImage* piecesSet, QImage piecesImage )
{
    int x = piecesImage.width()/6;
    int y = piecesImage.height();

    int size=2;
    while ( size < x || size < y )
        size <<= 1;

    for ( int i=0; i<6; i++ ) {
        QImage onePiece( size, size, QImage::Format_ARGB32 );
        onePiece.fill( 0x00000000 );
        QPainter p( &onePiece );
        p.drawImage( ( size-x )/2, ( size-y )/2, piecesImage, i*x,0, x, y );
        piecesSet[i] = onePiece;

    }
}


StatWidget::StatWidget(const RootBoard& rb):
    rb(rb)
{
    setupUi(this);
    QImage wset( ":/setwhite.png" );
    splitImage( wPieces, wset );
    QImage bset( ":/setblack.png" );
    splitImage( bPieces, bset );
    QFont fixed;
    fixed.setFamily("Consolas");
    bestLine->setFont(fixed);

    int pp = 0;
    for (int i=0; i<50; i++) {
    	pal[pp++] = QColor(i,i,255-i);
    }
    for (int i=0; i<50; i++) {
    	pal[pp++] = QColor(50, 50+2*i, 205-2*i);
    }
    for (int i=0; i<28; i++) {
    	pal[pp++] = QColor(50+(i*50)/28,150-(i*50)/28,105-(i*5)/55);
    }
    for (int i=0; i<28; i++) {
    	pal[pp++] = QColor(100+(i*50)/28,100+(i*50)/28,100-i*2);
    }
    for (int i=0; i<50; i++) {
    	pal[pp++] = QColor(150+i, 150+i, 44-(i*44)/50);
    }
    for (int i=0; i<50; i++) {
    	pal[pp++] = QColor(150+i, 150-2*i, i);
    }
    ASSERT(pp==256);

    minipm[0][Rook] = ww1;
    minipm[0][Bishop] = ww2;
    minipm[0][Queen] = ww3;
    minipm[0][Knight] = ww4;
    minipm[0][Pawn] = ww5;
    minipm[0][King] = ww6;

    minipm[1][Rook] = wb1;
    minipm[1][Bishop] = wb2;
    minipm[1][Queen] = wb3;
    minipm[1][Knight] = wb4;
    minipm[1][Pawn] = wb5;
    minipm[1][King] = wb6;

    tree = new NodeModel(this);
    treeView->setModel( tree );

	NodeDelegate* delegate = new NodeDelegate;
	treeView->setItemDelegate( delegate );

    QTimer* t = new QTimer(this);
    connect(t, SIGNAL(timeout()), this, SLOT(update()));
    qRegisterMetaType<uint64_t>("uint64_t");
    connect(rb.console, SIGNAL(signalSend(std::string)), this, SLOT(updateLine(std::string)));
    t->setInterval(1000);
    t->start();
}

StatWidget::~StatWidget()
{
}

void StatWidget::updateLine(std::string line)
{
//        bestLine->moveCursor(QTextCursor::Start);
        bestLine->appendPlainText(QString::fromStdString(line));
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
	treeView->doItemsLayout();
    updateBoard();
    Ui_Statsui::depth->setText("Depth: " + QString::number(rb.depth));
    setWindowTitle(QString::fromStdString(rb.getInfo()).left(20) + QString::fromStdString(rb.getLine()));
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
    DISPLAYNUM(ptcollision);
    DISPLAYNUM(jobs);
}

void StatWidget::updateBoard()
{
    static int oldsize = 0;
    int size=qMin( position->width(), position->height() )/8;
    if (size != oldsize) {
        oldsize = size;
        QImage wScaled[6], bScaled[6];
        for ( int i=0; i<6; i++ ) {
            wScaled[i] = wPieces[i].scaled( QSize( size*11/8, size*11/8 ), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ).copy(size*3/16, size*3/16, size, size);

        }
        for ( int i=0; i<6; i++ ) {
            bScaled[i] = bPieces[i].scaled( QSize( size*11/8, size*11/8 ), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ).copy(size*3/16, size*3/16, size, size);
        }

        QPixmap boardPixmap(position->size());
        boardPixmap.fill(QColor(0, 0, 0, 0));
        QPainter pa( &boardPixmap );
        pa.setPen(Qt::NoPen);

        for ( int x=0; x<8; x++ )
            for ( int y=0; y<8; y++ ) {
                int type = rb.currentBoard().getPiece(x+(7-y)*8);
                pa.setBrush(QBrush((x^y)&1 ? QColor(128,128,128):QColor(192,192,192) ));
                pa.drawRect(x*size, y*size, size, size);
                if ( type > 0 )
                    pa.drawImage( x*size, y*size, wScaled[type-1] );
                else if ( type < 0 )
                    pa.drawImage( x*size, y*size, bScaled[-type-1] );
            }

        position->setPixmap(boardPixmap);
    }

    size = (qMin(ww1->height(), ww1->width()))/8;
    QPixmap pm(size*8, size*8);
    QPainter pa( &pm );
    for (int c=-1; c<=1; c+=2)
        for (int p=Rook; p<=King; ++p) {
            pa.setPen(Qt::NoPen);
            for ( int x=0; x<8; x++ )
                for ( int y=0; y<8; y++ ) {
                    int ee;
                    if (radioButtonPSQ->isChecked())
                        ee = 2*(rb.eval.getPS(c*p, x+(7-y)*8).calc(rb.currentBoard(), rb.eval) - c*(int[]){0,450,300,925,300,100,0}[p]);
                    else
                        ee = rb.estimatedError[nPieces + c*p][x+(7-y)*8];

                    if (ee > 127) ee = 127;
                    if (ee<-128) ee = -128;
                    pa.setBrush(QBrush(pal[ee+128]));
                    pa.drawRect(x*size,y*size,size,size);
                }
            minipm[(1-c)/2][p]->setPixmap(pm);
        }
}

void StatWidget::emptyTree() {
	tree->init();
}

#endif // QT_GUI_LIB
