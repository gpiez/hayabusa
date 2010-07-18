/*
 * nodedelegate.h
 *
 *  Created on: 09.04.2009
 *      Author: gpiez
 */

#ifndef NODEDELEGATE_H_
#define NODEDELEGATE_H_

#ifdef QT_GUI_LIB

#ifndef PCH_H_
#include <pch.h>
#endif

class NodeDelegate: public QAbstractItemDelegate {
Q_OBJECT

public:
	virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
	virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
};

#endif
#endif /* NODEDELEGATE_H_ */
