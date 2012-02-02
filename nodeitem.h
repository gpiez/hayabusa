/*
 * nodeitem.h
 *
 *  Created on: 09.04.2009
 *      Author: gpiez
 */

#ifndef NODEITEM_H_
#define NODEITEM_H_

#ifdef QT_GUI_LIB

#include "move.h"
#include <mutex>
#include <QList>

#define MIN_NODES 0
#define MAX_NODES 50000000

struct NodeData {
    uint64_t key;
    Move move;
    int id;
    int nodes;
    short int alpha, beta, bestEval, gain, error, ps, real, pos;
    char nodeType;
    char searchType;
    char depth;
    char moveColor, nodeColor;
    char flags;
    char threadId;
    int8_t ply; };

class NodeItem: public NodeData {
public:
    NodeItem(const NodeData& _nodeData);
    NodeItem(const NodeData& _nodeData, NodeItem* _parent);
    ~NodeItem();

    static int nNodes;
    const NodeItem* child(int row);
    NodeItem* lastChild();
    int childCount() const;
    const NodeData& data() const;
    int row();
    NodeItem* getParent() const;
    void moveToEnd(NodeItem* node);
    static std::recursive_mutex m;

private:
    void appendChild( NodeItem* child);
    QList<NodeItem*> children;
    NodeItem* parent;

};

#endif
#endif /* NODEITEM_H_ */

