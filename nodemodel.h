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

#ifndef NODEMODEL_H
#define NODEMODEL_H

#ifndef PCH_H_
#include <pch.h>
#endif

#ifdef QT_GUI_LIB

#include "nodeitem.h"

class NodeModel: public QAbstractItemModel {
Q_OBJECT

public:
    NodeModel( QObject* parent = 0);
    ~NodeModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    NodeItem* root() const {
        return rootItem;
    }
    void newData(NodeItem* node);
    void init();
    
private:
    void setupModelData(const QStringList &lines, NodeItem *parent);

    NodeItem *rootItem;
};

#endif

#endif
