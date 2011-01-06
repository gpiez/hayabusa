/*
 * nodeitem.cpp
 *
 *  Created on: 09.04.2009
 *      Author: gpiez
 */

#ifdef QT_GUI_LIB

#include <pch.h>

#include "nodeitem.h"
#include "stats.h"

std::recursive_mutex NodeItem::m;
int NodeItem::nNodes = 0;

NodeItem::NodeItem( const NodeData& nodeData_ ):
        NodeData( nodeData_ ),
        parent( 0 )
{}

NodeItem::NodeItem( const NodeData& nodeData_, NodeItem* parent_ ):
        NodeData( nodeData_ ),
        parent( parent_ )
{
    id = stats.node;
    m.lock();
    if (parent_)
        parent_->appendChild( this );
    else
        delete this;
    m.unlock();
}

NodeItem::~NodeItem()
{
    m.lock();
    qDeleteAll( children );
    m.unlock();
}

void NodeItem::appendChild( NodeItem* item )
{
    m.lock();
    children.append( item );
    m.unlock();
}

const NodeItem *NodeItem::child( int row )
{
    return children.value( row );
}

NodeItem *NodeItem::lastChild(  )
{
    return children.last();
}

int NodeItem::childCount() const
{
    return children.count();
}

int NodeItem::row()
{
    if ( parent )
        return parent->children.indexOf( this );
    return 0;
}

const NodeData& NodeItem::data() const
{
    return ( const NodeData& )( *this );
}

NodeItem* NodeItem::getParent() const
{
    return parent;
}

#endif
