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

		str2 = QString( "  [%1 %2]\n %3" ).arg( item->alpha ).arg( item->beta ).arg( item->bestEval );
	} else {
		str1 = QString( "Iteration %1" ).arg( item->ply );
		str2 = "";
	}

	str2+=QString( " psqv: %1 ply: %2 " ).arg( 0 /*FIXME*/ ).arg( item->ply );
	switch(item->nodeType) {
	case NodePrecut:
		str2 += "Precut";
		break;
	case NodeTT:
		str2 += "TT";
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
	str2 += QString( " id %1 key %2" ).arg( item->id ).arg(item->key);
/*
	if ( index.parent().isValid() )
		if ( static_cast<NodeItem*>( index.parent().internalPointer() )->bestMove.fromto() == item->move.fromto() )
			painter->setPen( Qt::red );
*/

	QRect coor = option.rect;
	painter->drawText( coor, Qt::AlignVCenter | Qt::AlignLeft, str1 );
	painter->setPen( Qt::black );
	coor.setLeft( coor.left() + 50 );
	painter->drawText( coor, Qt::AlignTop | Qt::AlignLeft, str2 );
}

QSize NodeDelegate::sizeHint ( const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/ ) const
{
	return QSize( 100,40 );
}

#endif