/***************************************************************************
 *   Copyright (c) 2017 Fredrik Johansson github.com/mumme74               *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef PYTHONDEBUGGERVIEW_H
#define PYTHONDEBUGGERVIEW_H


#include <CXX/Extensions.hxx>
#include <QAbstractTableModel>
#include "DockWindow.h"
#include "Window.h"
#include <frameobject.h>
#include <QVariant>

class QVBoxLayout;
//namespace  Py {
//class Object;
//} // namespace Py

namespace Gui {
namespace DockWnd {

class PythonDebuggerViewP;
class VariableModelP;
/**
 * @brief PythonDebugger dockable widgets
 */
class PythonDebuggerView : public QWidget
{
    Q_OBJECT
public:
    PythonDebuggerView(QWidget *parent = 0);
    ~PythonDebuggerView();


protected:
    void changeEvent(QEvent *e);

private Q_SLOTS:
    void startDebug();
    void enableButtons();
    void currentChanged(const QModelIndex & current, const QModelIndex & previous);

private:
    void initButtons(QVBoxLayout *vLayout);
    PythonDebuggerViewP *d;
};


/**
 * @brief data model for stacktrace
 */
class StackFramesModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    StackFramesModel(QObject *parent = 0);
    ~StackFramesModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

public Q_SLOTS:
    void clear();

private Q_SLOTS:
    void updateFrames(PyFrameObject *frame);

private:
    PyFrameObject *m_currentFrame;
    static const int colCount = 3;
};

//class VariableModel : public QAbstractTableModel
//{
//    Q_OBJECT
//public:
//    VariableModel(QObject *parent = 0);
//    ~VariableModel();

//    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
//    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
//    QVariant data(const QModelIndex &index, int role) const;
//    QVariant headerData(int section, Qt::Orientation orientation,
//                        int role = Qt::DisplayRole) const;

//public Q_SLOTS:
//    void clear();

//private Q_SLOTS:
//    void updateVariables(PyFrameObject *frame);

//private:
//    bool diffObjects(Py::Object *newObj, Py::Object *oldObj, QModelIndex &parent, int rowNr);
//    void doesDiff(QModelIndex &parent, int rowNr);
//    VariableModelP *d;
//    static const int colCount = 2;
//};

class VariableTreeItem
{
public:
    VariableTreeItem(const QList<QVariant> &data, VariableTreeItem *parent = 0);
    ~VariableTreeItem();

    void appendChild(VariableTreeItem *child);
    void removeChild(int row);

    VariableTreeItem *child(int row);
    VariableTreeItem *childByName(const QString &name);
    int childRowByName(const QString &name);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    VariableTreeItem *parent();

    const QString name() const;
    const QString value() const;
    void setValue(const QString value);
    const QString type() const;
    void setType(const QString type);

private:
    QList<VariableTreeItem*> childItems;
    QList<QVariant> itemData;
    VariableTreeItem *parentItem;
};


class VariableTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    VariableTreeModel(QObject *parent = 0);
    ~VariableTreeModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    bool removeRows(int firstRow, int lastRow,
                     const QModelIndex & parent = QModelIndex());
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

public Q_SLOTS:
    void clear();

private Q_SLOTS:
    void updateVariables(PyFrameObject *frame);

private:
    //void setupModelData(const QStringList &lines, VariableTreeItem *parent);
    void scanObject(PyObject *startObject, VariableTreeItem *parentItem,
                    int depth, bool noBuiltins);
    VariableTreeItem *m_rootItem;
    VariableTreeItem *m_localsItem;
    VariableTreeItem *m_globalsItem;
    VariableTreeItem *m_builtinsItem;
};

} // namespace DockWnd
} // namespace Gui

#endif // PYTHONDEBUGGERVIEW_H
