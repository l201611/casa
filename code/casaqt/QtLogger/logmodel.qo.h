//# Copyright (C) 2005
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id:  $

#ifndef LOGMODEL_H
#define LOGMODEL_H


#include <graphics/X11/X_enter.h>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QTextStream>
#include <graphics/X11/X_exit.h>

namespace casa {

class LogItem;

class LogModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    LogModel(const QString &data, QObject *parent = 0);
    ~LogModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void writeData(QTextStream &os);
    QString stringData();
    QString stringData(int row);
    bool insert(int, const QString&);
    virtual bool removeRows (int row, int count, 
            const QModelIndex & parent = QModelIndex());
    virtual bool insertRows (int row, int count, 
            const QModelIndex & parent = QModelIndex());
    void appendData(const QString &data);
    void changeFont(int);
    int getFontSize();

private:
    void setupModelData(const QString &lines, LogItem *parent);
    LogItem *insertItem; 
    LogItem *rootItem;
    QString searchKey;
    int fontSize;

public slots:
    void searchKeyChanged(const QString &key = "");
};

}
#endif
