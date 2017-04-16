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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QGridLayout>
# include <QApplication>
# include <QMenu>
# include <QContextMenuEvent>
# include <QTextCursor>
# include <QTextStream>
#endif


#include <Python.h>
#include "PythonDebuggerView.h"
#include "PythonDebugger.h"
#include "BitmapFactory.h"
#include "EditorView.h"
#include "Window.h"
#include "MainWindow.h"
#include "PythonEditor.h"
#include "PythonCode.h"

#include <CXX/Extensions.hxx>
#include <frameobject.h>
#include <Base/Interpreter.h>
#include <Application.h>
#include <QVariant>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QTableView>
#include <QTreeView>
#include <QHeaderView>
#include <QFileDialog>
#include <QSplitter>



/* TRANSLATOR Gui::DockWnd::PythonDebuggerView */

namespace Gui {
namespace DockWnd {

class PythonDebuggerViewP
{
public:
    QPushButton *m_startDebugBtn;
    QPushButton *m_stopDebugBtn;
    QPushButton *m_stepIntoBtn;
    QPushButton *m_stepOverBtn;
    QPushButton *m_stepOutBtn;
    QPushButton *m_haltOnNextBtn;
    QPushButton *m_continueBtn;
    QLabel      *m_varLabel;
    QTreeView   *m_varView;
    QLabel      *m_stackLabel;
    QTableView  *m_stackView;
    PythonDebuggerViewP() { }
    ~PythonDebuggerViewP() { }
};

} // namespace DockWnd
} //namespace Gui

using namespace Gui;
using namespace Gui::DockWnd;

/**
 *  Constructs a PythonDebuggerView which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
PythonDebuggerView::PythonDebuggerView(QWidget *parent)
   : QWidget(parent), d(new PythonDebuggerViewP())
{
    setObjectName(QLatin1String("DebuggerView"));

    QVBoxLayout *vLayout = new QVBoxLayout(this);

    initButtons(vLayout);

    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Vertical);
    vLayout->addWidget(splitter);

    // variables explorer view
    QFrame *varFrame = new QFrame(this);
    QVBoxLayout *varLayout = new QVBoxLayout(varFrame);
    varFrame->setLayout(varLayout);
    d->m_varLabel = new QLabel(this);
    d->m_varLabel->setText(tr("Variables"));
    varLayout->addWidget(d->m_varLabel);
    d->m_varView = new QTreeView(this);
    VariableTreeModel *varModel = new VariableTreeModel(this);
    d->m_varView->setModel(varModel);
    //d->m_varView->verticalHeader()->hide();
    //d->m_varView->horizontalHeader()->setResizeMode(
    //                                QHeaderView::ResizeToContents);
    //d->m_varView->setShowGrid(false);
    varLayout->addWidget(d->m_varView);
    splitter->addWidget(varFrame);


    // the stacktrace view
    QFrame *stackFr = new QFrame(this);
    QVBoxLayout *stackLayout = new QVBoxLayout(stackFr);
    stackFr->setLayout(stackLayout);
    splitter->addWidget(stackFr);
    d->m_stackLabel = new QLabel(this);
    d->m_stackLabel->setText(tr("Stack frames"));
    d->m_stackView = new QTableView(this);
    StackFramesModel *stackModel = new StackFramesModel(this);
    d->m_stackView->setModel(stackModel);
    d->m_stackView->verticalHeader()->hide();
    d->m_stackView->horizontalHeader()->setResizeMode(
                                    QHeaderView::ResizeToContents);
    d->m_stackView->setShowGrid(false);
    stackLayout->addWidget(d->m_stackLabel);
    stackLayout->addWidget(d->m_stackView);


    connect(d->m_stackView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex&)),
                                        this, SLOT(currentChanged(const QModelIndex&, const QModelIndex&)));


    setLayout(vLayout);
    // raise the tab page set in the preferences
    //ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    //int index = hGrp->GetInt("AutoloadTab", 0);
    //tabWidget->setCurrentIndex(index);
}

PythonDebuggerView::~PythonDebuggerView()
{
    delete d;
}

void PythonDebuggerView::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
//        tabOutput->setWindowTitle(trUtf8("Output"));
//        tabPython->setWindowTitle(trUtf8("Python console"));
//        for (int i=0; i<tabWidget->count();i++)
//            tabWidget->setTabText(i, tabWidget->widget(i)->windowTitle());
        d->m_stackLabel->setText(tr("Stack frames"));
    }
}

void PythonDebuggerView::startDebug()
{
    PythonEditorView* editView = 0;
    QList<QWidget*> mdis = getMainWindow()->windows();
    for (QList<QWidget*>::iterator it = mdis.begin(); it != mdis.end(); ++it) {
        editView = qobject_cast<PythonEditorView*>(*it);
        if (editView) break;
    }

    if (!editView) {
        WindowParameter param("PythonDebuggerView");
        std::string path = param.getWindowParameter()->GetASCII("MacroPath",
            App::Application::getUserMacroDir().c_str());
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open python file"),
                                                        QLatin1String(path.c_str()),
                                                        tr("Python (*.py *.FCMacro)"));
        if (!fileName.isEmpty()) {
            PythonEditor* editor = new PythonEditor();
            editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
            editView = new PythonEditorView(editor, getMainWindow());
            editView->open(fileName);
            editView->resize(400, 300);
            getMainWindow()->addWindow(editView);
        } else {
            return;
        }
    }

    getMainWindow()->setActiveWindow(editView);
    editView->startDebug();
}

void PythonDebuggerView::enableButtons()
{
    PythonDebugger *debugger = PythonDebugger::instance();
    bool running = debugger->isRunning();
    bool halted = debugger->isHalted();
    d->m_startDebugBtn->setEnabled(!running);
    d->m_continueBtn->setEnabled(running);
    d->m_stopDebugBtn->setEnabled(running);
    d->m_stepIntoBtn->setEnabled(halted);
    d->m_stepOverBtn->setEnabled(halted);
    d->m_stepOutBtn->setEnabled(halted);
    d->m_haltOnNextBtn->setEnabled(!halted);
}

void PythonDebuggerView::currentChanged(const QModelIndex &current,
                                        const QModelIndex &previous)
{
    Q_UNUSED(previous);

    //QItemSelectionModel *sel = d->m_stackView->selectionModel();
    QAbstractItemModel *viewModel = d->m_stackView->model();
    QModelIndex idxLeft = viewModel->index(current.row(), 0, QModelIndex());
    QModelIndex idxRight = viewModel->index(current.row(),
                                            viewModel->columnCount(),
                                            QModelIndex());

    QItemSelection sel(idxLeft, idxRight);
    QItemSelectionModel *selModel = d->m_stackView->selectionModel();
    selModel->select(sel, QItemSelectionModel::Rows);

    QModelIndex stackIdx = current.sibling(current.row(), 0);
    StackFramesModel *model = static_cast<StackFramesModel*>(d->m_stackView->model());
    QVariant idx = model->data(stackIdx, Qt::DisplayRole);

    PythonDebugger::instance()->setStackLevel(idx.toInt());
}

void PythonDebuggerView::initButtons(QVBoxLayout *vLayout)
{
    // debug buttons
    d->m_startDebugBtn = new QPushButton(this);
    d->m_continueBtn   = new QPushButton(this);
    d->m_stopDebugBtn  = new QPushButton(this);
    d->m_stepIntoBtn   = new QPushButton(this);
    d->m_stepOverBtn   = new QPushButton(this);
    d->m_stepOutBtn    = new QPushButton(this);
    d->m_haltOnNextBtn = new QPushButton(this);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(d->m_startDebugBtn);
    btnLayout->addWidget(d->m_stopDebugBtn);
    btnLayout->addWidget(d->m_haltOnNextBtn);
    btnLayout->addWidget(d->m_continueBtn);
    btnLayout->addWidget(d->m_stepIntoBtn);
    btnLayout->addWidget(d->m_stepOverBtn);
    btnLayout->addWidget(d->m_stepOutBtn);
    vLayout->addLayout(btnLayout);

    d->m_startDebugBtn->setIcon(BitmapFactory().iconFromTheme("debug-start").pixmap(16,16));
    d->m_continueBtn->setIcon(BitmapFactory().iconFromTheme("debug-continue").pixmap(16,16));
    d->m_stopDebugBtn->setIcon(BitmapFactory().iconFromTheme("debug-stop").pixmap(16,16));
    d->m_stepOverBtn->setIcon(BitmapFactory().iconFromTheme("debug-step-over").pixmap(16,16));
    d->m_stepOutBtn->setIcon(BitmapFactory().iconFromTheme("debug-step-out").pixmap(16,16));
    d->m_stepIntoBtn->setIcon(BitmapFactory().iconFromTheme("debug-step-into").pixmap(16,16));
    d->m_haltOnNextBtn->setIcon(BitmapFactory().iconFromTheme("debug-halt").pixmap(16,16));
    d->m_startDebugBtn->setToolTip(tr("Start debugging"));
    d->m_stopDebugBtn->setToolTip(tr("Stop debugger"));
    d->m_continueBtn->setToolTip(tr("Continue running"));
    d->m_stepIntoBtn->setToolTip(tr("Next instruction, steps into functions"));
    d->m_stepOverBtn->setToolTip(tr("Next instruction, dont step into functions"));
    d->m_stepOutBtn->setToolTip(tr("Continue until current function ends"));
    d->m_haltOnNextBtn->setToolTip(tr("Halt on any python code"));
    enableButtons();

    PythonDebugger *debugger = PythonDebugger::instance();

    connect(d->m_startDebugBtn, SIGNAL(clicked()), this, SLOT(startDebug()));
    connect(d->m_stopDebugBtn, SIGNAL(clicked()), debugger, SLOT(stop()));
    connect(d->m_haltOnNextBtn, SIGNAL(clicked()), debugger, SLOT(haltOnNext()));
    connect(d->m_stepIntoBtn, SIGNAL(clicked()), debugger, SLOT(stepInto()));
    connect(d->m_stepOverBtn, SIGNAL(clicked()), debugger, SLOT(stepOver()));
    connect(d->m_stepOutBtn, SIGNAL(clicked()), debugger, SLOT(stepOut()));
    connect(d->m_continueBtn, SIGNAL(clicked()), debugger, SLOT(stepContinue()));
    connect(debugger, SIGNAL(haltAt(QString,int)), this, SLOT(enableButtons()));
    connect(debugger, SIGNAL(releaseAt(QString,int)), this, SLOT(enableButtons()));
    connect(debugger, SIGNAL(started()), this, SLOT(enableButtons()));
    connect(debugger, SIGNAL(stopped()), this, SLOT(enableButtons()));

}



// ---------------------------------------------------------------------------
StackFramesModel::StackFramesModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_currentFrame(nullptr)
{

    PythonDebugger *debugger = PythonDebugger::instance();

    connect(debugger, SIGNAL(functionCalled(PyFrameObject*)),
               this, SLOT(updateFrames(PyFrameObject*)));
    connect(debugger, SIGNAL(functionExited(PyFrameObject*)),
               this, SLOT(updateFrames(PyFrameObject*)));
    connect(debugger, SIGNAL(stopped()), this, SLOT(clear()));
}

StackFramesModel::~StackFramesModel()
{
}

int StackFramesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return PythonDebugger::instance()->callDepth(m_currentFrame);
}

int StackFramesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return colCount +1;
}

QVariant StackFramesModel::data(const QModelIndex &index, int role) const
{
    if (m_currentFrame == nullptr)
        return QVariant();

    if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
        return QVariant();

    if (index.column() > colCount)
        return QVariant();

    Base::PyGILStateLocker locker;
    PyFrameObject *frame = m_currentFrame;

    int i = 0,
        j = PythonDebugger::instance()->callDepth(m_currentFrame);

    while (NULL != frame) {
        if (i == index.row()) {
            switch(index.column()) {
            case 0:
                return QString::number(j);
            case 1: { // function
                const char *funcname = PyString_AsString(frame->f_code->co_name);
                return QString(QLatin1String(funcname));
            }
            case 2: {// file
                const char *filename = PyString_AsString(frame->f_code->co_filename);
                return QString(QLatin1String(filename));
            }
            case 3: {// line
                int line = PyCode_Addr2Line(frame->f_code, frame->f_lasti);
                return QString::number(line);
            }
            default:
                return QVariant();
            }
        }

        frame = frame->f_back;
        ++i;
        --j; // need to reversed, display current on top
    }


    return QVariant();
}

QVariant StackFramesModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case 0:
        return QString(tr("Level"));
    case 1:
        return QString(tr("Function"));
    case 2:
        return QString(tr("File"));
    case 3:
        return QString(tr("Line"));
    default:
        return QVariant();
    }
}

void StackFramesModel::clear()
{
    int i = 0;
    Base::PyGILStateLocker locker;
    Py_XDECREF(m_currentFrame);
    while (nullptr != m_currentFrame) {
        m_currentFrame = m_currentFrame->f_back;
        ++i;
    }

    beginRemoveRows(QModelIndex(), 0, i);
    endRemoveRows();
}

void StackFramesModel::updateFrames(PyFrameObject *frame)
{

    Base::PyGILStateLocker locker;
    if (m_currentFrame != frame) {
        clear();
        m_currentFrame = frame;
        Py_XINCREF(frame);

        beginInsertRows(QModelIndex(), 0, rowCount()-1);
        endInsertRows();
    }
}


// --------------------------------------------------------------------



VariableTreeItem::VariableTreeItem(const QList<QVariant> &data, VariableTreeItem *parent)
{
    parentItem = parent;
    itemData = data;
}

VariableTreeItem::~VariableTreeItem()
{
    qDeleteAll(childItems);
}

void VariableTreeItem::appendChild(VariableTreeItem *item)
{
    childItems.append(item);
}

void VariableTreeItem::removeChild(int row)
{
    if (row > -1 && row < childItems.size()) {
        VariableTreeItem *item = childItems.takeAt(row);
        delete item;
    }
}

VariableTreeItem *VariableTreeItem::child(int row)
{
    return childItems.value(row);
}

int VariableTreeItem::childRowByName(const QString &name)
{
    for (int i = 0; i < childItems.size(); ++i) {
        VariableTreeItem *item = childItems[i];
        if (item->name() == name)
            return i;
    }
    return -1;
}

VariableTreeItem *VariableTreeItem::childByName(const QString &name)
{
    for (int i = 0; i < childItems.size(); ++i) {
        VariableTreeItem *item = childItems.at(i);
        if (item->name() == name)
            return item;
    }

    return nullptr;
}

int VariableTreeItem::childCount() const
{
    return childItems.count();
}

int VariableTreeItem::columnCount() const
{
    return itemData.count();
}

QVariant VariableTreeItem::data(int column) const
{
    return itemData.value(column);
}

VariableTreeItem *VariableTreeItem::parent()
{
    return parentItem;
}

const QString VariableTreeItem::name() const
{
    return itemData.value(0).toString();
}

const QString VariableTreeItem::value() const
{
    return itemData.value(1).toString();
}

void VariableTreeItem::setValue(const QString value)
{
    itemData[1] = value;
}

const QString VariableTreeItem::type() const
{
    return itemData.value(2).toString();
}

void VariableTreeItem::setType(const QString type)
{
    itemData[2] = type;
}

int VariableTreeItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<VariableTreeItem*>(this));

    return 0;
}

// --------------------------------------------------------------------------



VariableTreeModel::VariableTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    QList<QVariant> rootData, locals, globals, builtins;
    rootData << tr("Name") << tr("Value") << tr("Type");
    m_rootItem = new VariableTreeItem(rootData);

    locals << QLatin1String("locals") << QLatin1String("") << QLatin1String("");
    globals << QLatin1String("globals") << QLatin1String("") << QLatin1String("");
    builtins << QLatin1String("builtins") << QLatin1String("") << QLatin1String("");
    m_localsItem = new VariableTreeItem(locals, m_rootItem);
    m_globalsItem = new VariableTreeItem(globals, m_rootItem);
    m_builtinsItem = new VariableTreeItem(builtins, m_rootItem);
    m_rootItem->appendChild(m_localsItem);
    m_rootItem->appendChild(m_globalsItem);
    m_rootItem->appendChild(m_builtinsItem);

    PythonDebugger *debugger = PythonDebugger::instance();

    connect(debugger, SIGNAL(nextInstruction(PyFrameObject*)),
               this, SLOT(updateVariables(PyFrameObject*)));

    connect(debugger, SIGNAL(functionCalled(PyFrameObject*)),
               this, SLOT(updateVariables(PyFrameObject*)));

    connect(debugger, SIGNAL(functionExited(PyFrameObject*)),
               this, SLOT(clear()));

    connect(debugger, SIGNAL(stopped()), this, SLOT(clear()));


    //setupModelData(data.split(QLatin1String("\n")), rootItem);
}

VariableTreeModel::~VariableTreeModel()
{
    delete m_rootItem; // root item hadles delete of children
}

int VariableTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<VariableTreeItem*>(parent.internalPointer())->columnCount();
    else
        return m_rootItem->columnCount();
}

bool VariableTreeModel::removeRows(int firstRow, int lastRow, const QModelIndex &parent)
{
    beginRemoveRows(parent, firstRow, lastRow);
    VariableTreeItem *parentItem = static_cast<VariableTreeItem*>(
                                            parent.internalPointer());

    for (int i = firstRow; i < lastRow; ++i) {
        parentItem->removeChild(i);
    }

    endRemoveRows();

    return true;
}

bool VariableTreeModel::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent) > 0;
}


QVariant VariableTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    VariableTreeItem *item = static_cast<VariableTreeItem*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags VariableTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant VariableTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_rootItem->data(section);

    return QVariant();
}

QModelIndex VariableTreeModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    VariableTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<VariableTreeItem*>(parent.internalPointer());

    VariableTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex VariableTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    VariableTreeItem *childItem = static_cast<VariableTreeItem*>(index.internalPointer());
    VariableTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem || parentItem == nullptr)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int VariableTreeModel::rowCount(const QModelIndex &parent) const
{
    VariableTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<VariableTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}


void VariableTreeModel::clear()
{
    // locals
    QModelIndex localsIdx = createIndex(0, 0, m_localsItem);
    beginRemoveRows(localsIdx, 0, m_localsItem->childCount());
    while (m_localsItem->childCount())
        m_localsItem->removeChild(0);

    endRemoveRows();

    // globals
    QModelIndex globalsIdx = createIndex(0, 0, m_globalsItem);
    beginRemoveRows(globalsIdx, 0, m_globalsItem->childCount());
    while (m_globalsItem->childCount())
        m_globalsItem->removeChild(0);

    endRemoveRows();
}

void VariableTreeModel::updateVariables(PyFrameObject *frame)
{
    Base::PyGILStateLocker lock;


    PyFrame_FastToLocals(frame);

    // first locals
    VariableTreeItem *parentItem = m_localsItem;
    //Py::Dict rootObject(frame->f_locals);
    PyObject *rootObject = (PyObject*)frame;

    scanObject(rootObject, parentItem, 0, true);

    // then globals
    parentItem = m_globalsItem;
    PyObject *globalsDict = frame->f_globals;
    scanObject(globalsDict, parentItem, 0, true);

    // and the builtins
    parentItem = m_builtinsItem;
    scanObject(rootObject, parentItem, 0, false);

}

void VariableTreeModel::scanObject(PyObject *startObject, VariableTreeItem *parentItem,
                                   int depth, bool noBuiltins)
{
    // avoid cyclic recursion
    static const int maxDepth = 15;
    if (depth > maxDepth)
        return;

    QList<VariableTreeItem*> added;
    QList<QString> visited, updated, deleted;

    Py::List keys;
    Py::Dict object;

    // only scan object which has keys
    try {
        if (PyDict_Check(startObject))
            object = Py::Dict(startObject);
        else if (PyList_Check(startObject)) {
            Py::List lst(startObject);
            for (int i = 0; i < static_cast<int>(lst.size()); ++i) {
                Py::Int key(i);
                object[key.str()] = lst[i];
            }
        } else if (PyTuple_Check(startObject)) {
            Py::Tuple tpl(startObject);
            for (int i = 0; i < static_cast<int>(tpl.size()); ++i) {
                Py::Int key(i);
                object[key.str()] = tpl[i];
            }
        } else if (PyFrame_Check(startObject)){
            PyFrameObject *frame = (PyFrameObject*)startObject;
            object = Py::Dict(frame->f_locals);
        } else if (PyModule_Check(startObject)) {
            object = Py::Dict(PyModule_GetDict(startObject));
        }
        // let other types fail and return here

        keys = object.keys();

    } catch(...) {
        return;
    }


    // first we traverse and compare before we o anything so
    // view dont get updated to manys times

    // traverse and compare
    for (Py::List::iterator it = keys.begin(); it != keys.end(); ++it) {
        Py::String key(*it);
        QString name = QString::fromStdString(key);
        QString newValue = QString::fromStdString(object[key].str());
        QString newType = QString::fromStdString(object[key].type().str());

        VariableTreeItem *currentChild = parentItem->childByName(name);
        if (currentChild == nullptr) {
            // not found, should add to model
            if (noBuiltins) {
                if (newType.contains(QLatin1String("<built-in")))
                    continue;
            } else if (!newType.contains(QLatin1String("<built-in")))
                continue;
            QList<QVariant> data;
            data << name << newValue << newType;
            VariableTreeItem *createdItem = new VariableTreeItem(data, parentItem);
            added.append(createdItem);
            PyObject *pObj = object[key].ptr();
            if (PyDict_Check(pObj) || PyList_Check(pObj) || PyTuple_Check(pObj)) {
                // check for subobject recursively
                scanObject(pObj, createdItem, depth +1, true);
            }
        } else {
            visited.append(name);

            if (newType != currentChild->type() ||
                newValue != currentChild->value())
            {
                currentChild->setType(newType);
                currentChild->setValue(newValue);
                updated.append(name);
            }

        }
    }

    // maybe some values have gotten out of scope
    for (int i = 0; i < parentItem->childCount(); ++i) {
        VariableTreeItem *item = parentItem->child(i);
        if (!visited.contains(item->name()))
            deleted.append(item->name());
    }

    // finished compare now do changes


    // handle updates
    // this tries to find out how many consecutive rows thats changed
    int row = 0;
    for (const QString &item : updated) {
        row = parentItem->childRowByName(item);
        if (row < 0) continue;

        QModelIndex topLeftIdx = createIndex(row, 1, parentItem);
        QModelIndex bottomRightIdx = createIndex(row, 2, parentItem);
        Q_EMIT dataChanged(topLeftIdx, bottomRightIdx);
    }

    // handle deletes
    for (const QString &name : deleted) {
        row = parentItem->childRowByName(name);
        if (row < 0) continue;

        QModelIndex idx = createIndex(row, 0, parentItem);
        removeRows(row, row+1, idx);
    }

    // handle inserts
    // these are already created during compare phase
    // we insert last so variables dont jump around in the treeview
    if (added.size()) {
        int rowCount = parentItem->childCount();
        QModelIndex idx = createIndex(rowCount, 0, parentItem);
        beginInsertRows(idx, rowCount, rowCount + added.size()-1);
        for (VariableTreeItem *item : added) {
            beginInsertRows(idx, rowCount, rowCount+1);
            parentItem->appendChild(item);
            endInsertRows();
        }
        endInsertRows();
    }

    // notify view that parentItem might have children now
    if (updated.size() || added.size() || deleted.size()) {
        Q_EMIT layoutChanged();
    }
}

/*void VariableTreeModel::doesDiff(QModelIndex &parent, int rowNr)
{
    QModelIndex col0(rowNr, 0, parent);
    QModelIndex col1(rowNr, 1, parent);
    QModelIndex col2(rowNr, 2, parent);
}

bool VariableTreeModel::diffObjects(Py::Object *newObj, Py::Object *oldObj, QModelIndex &parent, int rowNr)
{
    bool result = false;
    try {
        if (newObj->isTuple()) {
            if (!oldObj->isTuple()) {
                QModelIndex idx;
                idx;
            }

            Py::Tuple newTuple(newObj);
        }
    } catch(Py::Exception e) {

    }
    return result;
}
*/
/*
void VariableTreeModel::setupModelData(const QStringList &lines, VariableTreeItem *parent)
{
    QList<VariableTreeItem*> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].mid(position, 1) != QLatin1String(" "))
                break;
            position++;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split(QLatin1String("\t"), QString::SkipEmptyParts);
            QList<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column)
                columnData << columnStrings[column];

            if (position > indentations.last()) {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

                if (parents.last()->childCount() > 0) {
                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            parents.last()->appendChild(new VariableTreeItem(columnData, parents.last()));
        }

        number++;
    }
}
*/



/*
namespace Gui {
namespace DockWnd {

class VariableModelP
{
public:
    PythonCode        pyCode;
    PythonCodeObject *rootObj;
    VariableModelP() :
        rootObj(nullptr)
    {
    }

    ~VariableModelP()
    {
    }
};

} // namespace DockWnd
} // namespace Gui


VariableModel::VariableModel(QObject *parent):
    QAbstractTableModel(parent)
{
    d = new VariableModelP();
    PythonDebugger *debugger = PythonDebugger::instance();

    connect(debugger, SIGNAL(functionCalled(PyFrameObject*)),
               this, SLOT(updateVariables(PyFrameObject*)));
    connect(debugger, SIGNAL(functionExited(PyFrameObject*)),
               this, SLOT(updateVariables(PyFrameObject*)));
    connect(debugger, SIGNAL(stopped()), this, SLOT(clear()));
}

VariableModel::~VariableModel()
{
    delete d;
}

int VariableModel::rowCount(const QModelIndex &parent) const
{
    return 0;

}

int VariableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return colCount +1;
}

QVariant VariableModel::data(const QModelIndex &index, int role) const
{

    return QVariant();
}

QVariant VariableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case 0:
        return QString(tr("Name"));
    case 1:
        return QString(tr("Value"));
    case 2:
        return QString(tr("Type"));
    default:
        return QVariant();
    }
}

void VariableModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, 1000);
    endRemoveRows();
}

void VariableModel::updateVariables(PyFrameObject *frame)
{

    Base::PyGILStateLocker lock;

    // make a copy of this frame, lets us compare for changes
    PyObject *copied = d->pyCode.deepCopy((PyObject*)frame);
    PythonCodeObject *newFrame = new PythonCodeObject(copied);

    PyFrame_FastToLocals(current_frame);
    Py::Object obj = getDeepObject(current_frame->f_locals, varName, foundKey);
    if (obj.isNull())
        obj = getDeepObject(current_frame->f_globals, varName, foundKey);

    if (obj.isNull())
        obj = getDeepObject(current_frame->f_builtins, varName, foundKey);

    if (obj.isNull()) {
        return QString();
    }


    // replace our cached copy
    if (d->rootObj != nullptr)
        delete d->rootObj;
    d->rootObj = newFrame;

    beginRemoveRows(QModelIndex(), 0, 1000);
    endRemoveRows();
    beginInsertRows(QModelIndex(), 0, rowCount()-1);
    endInsertRows();
}


void VariableModel::doesDiff(QModelIndex &parent, int rowNr)
{

    QModelIndex col0(rowNr, 0, parent);
    QModelIndex col1(rowNr, 1, parent);
    QModelIndex col2(rowNr, 2, parent);
}

bool VariableModel::diffObjects(Py::Object *newObj, Py::Object *oldObj, QModelIndex &parent, int rowNr)
{
    bool result = false;
    try {
        if (newObj->isTuple()) {
            if (!oldObj->isTuple()) {
                QModelIndex idx;
                idx
            }

            Py::Tuple newTuple(newObj);
            if (newObj->)
        }
    } catch(Py::Exception e) {

    }
    return result;
}
*/



#include "moc_PythonDebuggerView.cpp"
