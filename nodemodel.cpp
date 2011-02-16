/***************************************************************************
 *   Copyright (C) 2005 by Gunther Piez   *
 *   gpiez@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef QT_GUI_LIB

#include <pch.h>
#include "nodemodel.h"
#include "nodeitem.h"

NodeModel::NodeModel( QObject* parent ):
        QAbstractItemModel( parent ),
        rootItem(NULL)
{
    std::lock_guard<std::recursive_mutex> lock(NodeItem::m);
    init();
}

void NodeModel::init() {
    std::lock_guard<std::recursive_mutex> lock(NodeItem::m);
    beginResetModel();
    delete rootItem;
    NodeData rootData = {};
    rootItem = new NodeItem( rootData );
    endResetModel();
}

NodeModel::~NodeModel()
{
    std::lock_guard<std::recursive_mutex> lock(NodeItem::m);
    delete rootItem;
}

QModelIndex NodeModel::index( int row, int column, const QModelIndex& parent ) const
{
    std::lock_guard<std::recursive_mutex> lock(NodeItem::m);
    NodeItem* parentItem;

    if ( !parent.isValid() )
        parentItem = rootItem;//TODO sanity checks for rootItem == 0
    else
        parentItem = static_cast<NodeItem*>( parent.internalPointer() );

    const NodeItem* childItem = parentItem->child( row );
    if ( childItem )
        return createIndex( row, column, (void*)childItem );
    else
        return QModelIndex();
}

QModelIndex NodeModel::parent( const QModelIndex &index ) const
{
    std::lock_guard<std::recursive_mutex> lock(NodeItem::m);
    if ( !index.isValid() )
        return QModelIndex();

    const NodeItem* childItem = static_cast<NodeItem*>( index.internalPointer() );
    NodeItem* parentItem = childItem->getParent();

    if ( !parentItem || parentItem == rootItem )
        return QModelIndex();

    return createIndex( parentItem->row(), 0, ( void* )parentItem );
}


int NodeModel::rowCount( const QModelIndex &parent ) const
{
    std::lock_guard<std::recursive_mutex> lock(NodeItem::m);
    const NodeItem *parentItem;

    if ( !parent.isValid() )
        parentItem = rootItem;
    else
        parentItem = static_cast<NodeItem*>( parent.internalPointer() );

    return parentItem->childCount();
}

int NodeModel::columnCount( const QModelIndex &/*parent*/ ) const
{
    return 1;
}

Qt::ItemFlags NodeModel::flags( const QModelIndex &index ) const
{
    if ( !index.isValid() )
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant NodeModel::data(const QModelIndex& index, int role) const
{
    std::lock_guard<std::recursive_mutex> lock(NodeItem::m);
    if (role == Qt::ToolTipRole) {
        return static_cast<NodeItem*>(index.internalPointer())->id;
    }
    return QVariant();
}

QVariant NodeModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
    return QVariant();
}

void NodeModel::newData(NodeItem* node) {
    std::lock_guard<std::recursive_mutex> lock(NodeItem::m);
    QModelIndex i = createIndex( node->row(), 0, ( void* )node );
    emit(dataChanged( i, i));
}
#endif
