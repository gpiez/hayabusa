/*
 * nodeitem.h
 *
 *  Created on: 09.04.2009
 *      Author: gpiez
 */

#ifndef NODEITEM_H_
#define NODEITEM_H_

#ifdef QT_GUI_LIB

#ifndef PCH_H_
#include <pch.h>
#endif

#include "move.h"

enum NodeType { NodeFull, NodeTT, NodePrecut, NodeNull, NodeFutile, NodeMate, NodePresearch, NodeIllegal, NodeRepetition };

#define MIN_NODES 0
#define MAX_NODES 10000000

struct NodeData {
	uint64_t key;
	Move move;
	int id;
	short int alpha, beta, bestEval, ply;
	char nodeType;
	char searchType;
};

class NodeItem: public NodeData {
public:
	NodeItem(const NodeData& _nodeData);
	NodeItem(const NodeData& _nodeData, NodeItem* _parent);
	~NodeItem();

	const NodeItem* child(int row);
	NodeItem* lastChild();
	int childCount() const;
	const NodeData& data() const;
	int row();
	NodeItem* getParent() const;

private:
	static std::mutex m;
	void appendChild( NodeItem* child);
	QList<NodeItem*> children;
	NodeItem* parent;

};

#endif
#endif /* NODEITEM_H_ */
