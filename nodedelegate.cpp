/*
 * nodedelegate.cpp
 *
 *  Created on: 09.04.2009
 *      Author: gpiez
 */
#include <pch.h>

#ifdef  QT_GUI_LIB

#include "nodedelegate.h"
#include "nodeitem.h"

void NodeDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    const NodeItem *item = static_cast<NodeItem*>( index.internalPointer() );
    // painter->begin();
    QString str1, str2;
    if ( item->move.fromto() ) {
        str1 = QString().fromStdString(item->move.string());

        str2 = QString( "D%3 [%1 %2]\n" ).arg( item->alpha ).arg( item->beta ).arg( (int)item->depth );
        if (item->nodeType != NodeStart)
            str2 += QString( "%1 " ).arg( item->bestEval );
        str2+=QString( " ply: %1 " ).arg( item->ply );
    } else {
        str1 = QString( "Depth %1" ).arg( item->ply );
        str2 = "";
    }

    switch(item->nodeType) {
    case NodePrecut1:
        str2 += "Precut1";
        break;
    case NodePrecut2:
        str2 += "Precut2";
        break;
    case NodePrecut3:
        str2 += "Precut3";
        break;
    case NodeTT:
        str2 += "TT";
        break;
    case NodeFutile1:
        str2 += "Futile1";
        break;
    case NodeFutile2:
        str2 += "Futile2";
        break;
    case NodeMate:
        str2 += "Mate";
        break;
    case NodeIllegal:
        str2 += "Illegal";
        break;
    case NodeRepetition:
        str2 += "Repetition";
        break;
    case NodeEndgameEval:
        str2 += "EndgameEval";
        break;
    case NodeStart:
        str2 += "SearchJobStart";
        break;
    default:
        str2 += "Full";
        break;
    }
    switch(item->searchType) {
    case root:
        str2 += "<Root>";
        break;
    case trunk:
        str2 += "<Trunk>";
        break;
    case tree:
        str2 += "<Tree>";
        break;
    case leaf:
        str2 += "<Leaf>";
        break;
    case vein:
        str2 += "<Vein>";
        break;
    }
    if (item->flags & Threatened)
        str2 += QString(" THR ");
    if (item->flags & Extend)
        str2 += QString(" EXT ");

    if (item->nodeType != NodeStart)
        str2 += QString( " size: %2" ).arg(item->nodes);
    str2 += QString( " thread: %3" ).arg((int)item->threadId);
/*
    if ( index.parent().isValid() )
        if ( static_cast<NodeItem*>( index.parent().internalPointer() )->bestMove.fromto() == item->move.fromto() )
            painter->setPen( Qt::red );
*/

    QRect coor = option.rect;
    QColor col;
    if (item->threadId)
        col.setHsv((int)pow(item->threadId, 1.3)*25-25, 255-128*(item->nodeType==NodeStart), 255);
    else
        col = Qt::gray;
    painter->setBrush(QBrush(col));
    painter->drawRect( option.rect );
    if (item->moveColor == White) {
        painter->setPen(Qt::white);
    } else {
        painter->setPen(Qt::black);
    }

    painter->drawText( coor, Qt::AlignVCenter | Qt::AlignLeft, str1 );
    coor.setLeft( coor.left() + 50 );
    if (item->nodeColor == White) {
        painter->setPen(Qt::white);
    } else {
        painter->setPen(Qt::black);
    }
    painter->drawText( coor, Qt::AlignTop | Qt::AlignLeft, str2 );
    painter->setPen(col);
}

QSize NodeDelegate::sizeHint ( const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/ ) const
{
    return QSize( 200,40 );
}

#endif
