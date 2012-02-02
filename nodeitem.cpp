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
    parent( 0 ) {
    nNodes = 0; //TODO increase/decrease counter in ctor/dtor
}

NodeItem::NodeItem( const NodeData& nodeData_, NodeItem* parent_ ):
    NodeData( nodeData_ ),
    parent( parent_ ) {
    id = stats.node;
//     std::lock_guard<std::recursive_mutex> lock(m);
    if (parent_)
        parent_->appendChild( this );
    else
        delete this; }

NodeItem::~NodeItem() {
    std::lock_guard<std::recursive_mutex> lock(m);
    qDeleteAll( children ); }

void NodeItem::appendChild( NodeItem* item ) {
    std::lock_guard<std::recursive_mutex> lock(m);
    children.append( item ); }

const NodeItem* NodeItem::child( int row ) {
//    std::lock_guard<std::recursive_mutex> lock(m);
    return children.value( row ); }

NodeItem* NodeItem::lastChild(  ) {
//    std::lock_guard<std::recursive_mutex> lock(m);
    return children.last(); }

int NodeItem::childCount() const {
//    std::lock_guard<std::recursive_mutex> lock(m);
    return children.count(); }

int NodeItem::row() {
//    std::lock_guard<std::recursive_mutex> lock(m);
    if ( parent )
        return parent->children.indexOf( this );
    return 0; }

const NodeData& NodeItem::data() const {
    return ( const NodeData& )( *this ); }

NodeItem* NodeItem::getParent() const {
//    std::lock_guard<std::recursive_mutex> lock(m);
    return parent; }

#endif

void NodeItem::moveToEnd(NodeItem* node) {
    int i=children.indexOf(node);
    ASSERT(i != -1);
    children.removeAt(i);
    children.append(node); }
