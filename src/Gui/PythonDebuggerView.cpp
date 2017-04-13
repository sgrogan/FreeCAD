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


#include "PythonDebuggerView.h"
#include "PythonDebugger.h"
//#include "Macro.h"
//#include "Application.h"
#include "BitmapFactory.h"
#include "EditorView.h"
#include "Window.h"
#include "MainWindow.h"
#include "PythonEditor.h"

#include <CXX/Extensions.hxx>
#include <frameobject.h>
#include <Base/Interpreter.h>
#include <Application.h>
#include <QVariant>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QHeaderView>
#include <QFileDialog>



/* TRANSLATOR Gui::DockWnd::PythonDebuggerView */

namespace Gui {
namespace DockWnd {

class PythonDebuggerViewP
{
public:
    QTableView  *m_stackView;
    QLabel      *m_stackLabel;
    QPushButton *m_startDebugBtn;
    QPushButton *m_stopDebugBtn;
    QPushButton *m_stepIntoBtn;
    QPushButton *m_stepOverBtn;
    QPushButton *m_stepOutBtn;
    QPushButton *m_haltOnNextBtn;
    QPushButton *m_continueBtn;
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

    d->m_stackLabel = new QLabel(this);
    d->m_stackLabel->setText(tr("Stack frames"));
    d->m_stackView = new QTableView(this);
    StackFramesModel *model = new StackFramesModel(this);
    d->m_stackView->setModel(model);
    d->m_stackView->verticalHeader()->hide();
    d->m_stackView->horizontalHeader()->setResizeMode(
                                    QHeaderView::ResizeToContents);
    d->m_stackView->setShowGrid(false);
    vLayout->addWidget(d->m_stackLabel);
    vLayout->addWidget(d->m_stackView);


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

    connect(debugger, SIGNAL(nextInstruction(PyFrameObject*)),
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
    beginRemoveRows(QModelIndex(), 0, 1000);
    endRemoveRows();

    Base::PyGILStateLocker locker;
    m_currentFrame = nullptr;
}

void StackFramesModel::updateFrames(PyFrameObject *frame)
{
    int oldRowCount = rowCount();

    Base::PyGILStateLocker locker;
    if (m_currentFrame != frame) {
        m_currentFrame = frame;
        beginRemoveRows(QModelIndex(), 0, oldRowCount-1);
        endRemoveRows();
        beginInsertRows(QModelIndex(), 0, rowCount()-1);
        endInsertRows();
    }
}


#include "moc_PythonDebuggerView.cpp"
